#include "tour.h"

#if defined(PLATFORMER_TOUR)

#include "bn_core.h"
#include "bn_log.h"

#include "data_manager.h"
#include "player.h"
#include "replay.h"
#include "replay_data.h"
#include "tour_data.h"
#include "world_index.h"

namespace {

int part = 0;
int frame = 0;
int next_shot = 0;

// Every bn::core::update(): one frame of the keypad commands has passed.
void on_update() {
    ++frame;
    if (next_shot < tour_data::SHOT_COUNT &&
        frame >= tour_data::SHOT_FRAMES[next_shot]) {
        BN_LOG("TOUR SHOT ", next_shot);
        ++next_shot;
    }
}
replay::Player buttons(nullptr, 0);

// A level played a second time starts this many frames late, so that its
// ghost (the first run, ghost_session.h) is ahead of the duck on the pictures.
constexpr int REVISIT_DELAY = 40;
bool visited[64] = {};
int delay = 0;
bool mirrored = false;

void play(InputState& input) {
    if (delay > 0) {
        --delay;
        input = InputState{};
        return;
    }
    input = buttons.next();
    if (mirrored) {
        const bool left = input.left_held;
        input.left_held = input.right_held;
        input.right_held = left;
    }
}

void seed(DataManager& data_manager, int slot, int level, int furthest) {
    GameState state{};
    state.level = int16_t(level);
    state.furthest_level = int16_t(furthest);
    data_manager.set_slot_index(slot);
    data_manager.state() = state;
    data_manager.save();
}

}  // namespace

namespace tour {

void boot() {
    bn::core::set_update_callback(on_update);
}

void start_screen(DataManager& data_manager) {
    const int index =
        part < tour_data::SLOT_1_COUNT ? part : tour_data::SLOT_1_COUNT - 1;
    seed(
        data_manager, 0, tour_data::SLOT_1[index].level,
        tour_data::SLOT_1[index].furthest);
    if (part == 0) {
        seed(data_manager, 1, 0, WorldIndex::LEVEL_COUNT - 1);
        // Something to show on slot 2's statistics screen.
        GameState& state = data_manager.state();
        state.play_frames = (2u * 3600 + 14 * 60 + 5) * 60;
        for (int i = 0; i < WorldIndex::LEVEL_COUNT; ++i) {
            state.level_deaths[i] = uint16_t((i * 7) % 23);
            // Every third level within its target time (6 s or more), the
            // others not (30 s): the level select shows both goal states.
            state.best_time_frames[i] = i % 3 == 0 ? 300 : 1800;
        }
        state.best_time_frames[WorldIndex::LEVEL_COUNT - 1] = 0;
        state.no_death_clears = 0x2D | (1u << 13) | (1u << 15);
        // Slot 2 has finished the game: Mirror Mode can be switched on.
        state.hard_mode_unlocked = true;
        data_manager.save();
    }
    data_manager.set_slot_index(0);
    ++part;
}

void boss_music(int order) {
    BN_LOG("BOSS MUSIC ", order);
}

void level_started(Player& player, unsigned level_index, bool mirror) {
    mirrored = mirror;
    // The level's first entry (its plain recording when it has one).
    const replay_data::Entry* found = nullptr;
    for (const replay_data::Entry& candidate : replay_data::ENTRIES) {
        if (candidate.level == int(level_index)) {
            found = &candidate;
            break;
        }
    }
    const replay_data::Entry& entry = *found;
    if (entry.runs == nullptr) {
        player.set_input_hook(nullptr);
        return;
    }
    buttons = replay::Player(entry.runs, entry.run_count);
    delay = 0;
    // A mirrored level has its own ghost.
    const unsigned key = level_index + (mirror ? 32 : 0);
    if (key < 64) {
        delay = visited[key] ? REVISIT_DELAY : 0;
        visited[key] = true;
    }
    player.set_input_hook(play);
    if (entry.has_start) {
        player.teleport_to(
            mirror ? -entry.start_x : entry.start_x, entry.start_y);
    }
}

}  // namespace tour

#endif

#include "replay_runner.h"

#if defined(PLATFORMER_REPLAY)

#include "bn_core.h"
#include "bn_log.h"

#include "data_manager.h"
#include "level_manager.h"
#include "player.h"
#include "replay.h"
#include "replay_data.h"

namespace {

replay::Player* playing = nullptr;

void play(InputState& input) {
    input = playing->next();
}

// Frames a level may run past the end of its recording before it counts as
// failed (the duck may still be on its way into the door).
constexpr int GRACE_FRAMES = 120;

}  // namespace

void run_replays(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager) {
    int passed = 0;
    int failed = 0;
    int missing = 0;
    player.set_visible(true);
    player.set_hud_visible(true);

    for (const replay_data::Entry& entry : replay_data::ENTRIES) {
        const int index = entry.level;
        if (entry.runs == nullptr) {
            BN_LOG("REPLAY ", index, " ", entry.name, " none");
            ++missing;
            continue;
        }

        // A fresh save at this level: no Hard / Assist Mode, the dash as in
        // normal play (LevelManager reads the level index).
        GameState fresh;
        fresh.level = int16_t(index);
        fresh.furthest_level = int16_t(index);
        data_manager.state() = fresh;

        replay::Player buttons(entry.runs, entry.run_count);
        playing = &buttons;
        player.set_input_hook(play);
        level_manager.load(levels[index]);
        // The layout the recording began with (death windows, a door that
        // moves): as after that many deaths in the level.
        if (entry.deaths > 0) {
            level_manager.set_deaths_in_level(entry.deaths);
        }
        if (entry.has_start) {
            player.teleport_to(entry.start_x, entry.start_y);
        }

        bool door = false;
        int frames = 0;
        int grace = 0;
        const int limit = entry.steps * 2 + 1200;
        while (frames < limit) {
            ++frames;
            if (level_manager.update() ==
                LevelManager::UpdateResult::LevelComplete) {
                door = true;
                break;
            }
            if (buttons.finished() && ++grace > GRACE_FRAMES) {
                break;
            }
        }
        BN_LOG(
            "REPLAY ", index, " ", entry.name, door ? " pass " : " FAIL ",
            buttons.steps_played(), " ", frames);
        door ? ++passed : ++failed;
        level_manager.unload();
    }

    BN_LOG("REPLAY DONE ", passed, " ", failed, " ", missing);
    while (true) {
        bn::core::update();
    }
}

#endif

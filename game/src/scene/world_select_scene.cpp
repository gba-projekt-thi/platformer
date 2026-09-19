#include "world_select_scene.h"

#include "audio_settings.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "data_manager.h"
#include "level_manager.h"
#include "level_select_scene.h"
#include "player.h"
#include "start_scene.h"
#include "world_index.h"

namespace {
// Appends the decimal digits of `value` into buf starting at *pos,
// advancing *pos past the written digits. Not a hot path (menu build
// only, same reasoning as frames_to_time()'s comment on plain division).
void append_uint(char* buf, int* pos, unsigned int value) {
    char digits[6];
    int digit_count = 0;
    do {
        digits[digit_count++] = char('0' + (value % 10));
        value /= 10;
    } while (value > 0 && digit_count < 6);
    while (digit_count > 0) {
        buf[(*pos)++] = digits[--digit_count];
    }
}
}  // namespace

WorldSelectScene::WorldSelectScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager)
    : _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager) {}

bool WorldSelectScene::_is_unlocked(int world_index) const {
    const int furthest = _data_manager.state().furthest_level;
    return WorldIndex::WORLDS[world_index].start_index <= furthest;
}

void WorldSelectScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _text_gen->set_blending_enabled(true);

    _selected_index = 0;
    _transition_requested = false;

    _rebuild_menu();
}

void WorldSelectScene::_rebuild_menu() {
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(false);
    }
    _menu_sprites.clear();

    for (int i = 0; i < WorldIndex::WORLD_COUNT; ++i) {
        char buf[32];
        int pos = 0;
        buf[pos++] = (i == _selected_index) ? '>' : ' ';
        buf[pos++] = ' ';
        for (const char* p = WorldIndex::WORLDS[i].name; *p; ++p) {
            buf[pos++] = *p;
        }
        if (!_is_unlocked(i)) {
            const char* locked = " (Locked)";
            for (const char* p = locked; *p; ++p) {
                buf[pos++] = *p;
            }
        }
        buf[pos] = '\0';

        _text_gen->generate(
            Cfg::StartScreen::X, Cfg::StartScreen::Y + i * 16, buf,
            _menu_sprites);
    }

    // -------------------------------------------------------------------------
    // Aggregate stats line, from already-tracked GameState fields only -
    // no new save data. `deaths` is the running total for this save slot
    // (not per-level); `Cleared` counts levels with a recorded best time.
    // -------------------------------------------------------------------------

    const GameState& game_state = _data_manager.state();
    const int level_count = _levels.size();

    int cleared = 0;
    for (int i = 0; i < level_count && i < GameState::MAX_LEVELS; ++i) {
        if (game_state.best_time_frames[i] > 0) {
            ++cleared;
        }
    }

    char stats_buf[40];
    int spos = 0;
    const char* deaths_label = "Deaths:";
    for (const char* p = deaths_label; *p; ++p) {
        stats_buf[spos++] = *p;
    }
    append_uint(stats_buf, &spos, game_state.deaths);

    const char* cleared_label = "  Cleared:";
    for (const char* p = cleared_label; *p; ++p) {
        stats_buf[spos++] = *p;
    }
    append_uint(stats_buf, &spos, static_cast<unsigned int>(cleared));
    stats_buf[spos++] = '/';
    append_uint(stats_buf, &spos, static_cast<unsigned int>(level_count));
    stats_buf[spos] = '\0';

    _text_gen->generate(
        Cfg::StartScreen::X, Cfg::StartScreen::Y + WorldIndex::WORLD_COUNT * 16,
        stats_buf, _menu_sprites);

    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_blending_enabled(true);
    }
}

void WorldSelectScene::update() {
    if (!_transition_requested) {
        bool changed = false;

        if (bn::keypad::down_pressed()) {
            _selected_index = (_selected_index + 1) % WorldIndex::WORLD_COUNT;
            changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        } else if (bn::keypad::up_pressed()) {
            _selected_index = (_selected_index + WorldIndex::WORLD_COUNT - 1) %
                              WorldIndex::WORLD_COUNT;
            changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        }

        if (changed) {
            _rebuild_menu();
        }

        if (bn::keypad::a_pressed()) {
            if (_is_unlocked(_selected_index)) {
                _transition_requested = true;

                auto next = bn::make_unique<LevelSelectScene>(
                    _player, _levels, _data_manager, _level_manager,
                    _selected_index);
                core::SceneManager::instance().set_next_scene(bn::move(next));

                AudioSettings::instance().play_sfx(bn::sound_items::confirm);
            } else {
                AudioSettings::instance().play_sfx(bn::sound_items::cancel);
            }
        } else if (bn::keypad::b_pressed()) {
            _transition_requested = true;

            auto next = bn::make_unique<StartScene>(
                _player, _levels, _data_manager, _level_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next));

            AudioSettings::instance().play_sfx(bn::sound_items::cancel);
        }
    }

    bn::core::update();
}
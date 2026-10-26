#include "level_select_scene.h"

#include "audio_settings.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "data_manager.h"
#include "level_manager.h"
#include "level_scene.h"
#include "player.h"
#include "text_buffer.h"
#include "timer.h"
#include "world_index.h"
#include "world_select_scene.h"

LevelSelectScene::LevelSelectScene(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager,
    int world_index)
    : _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager),
      _world_index(world_index) {}

bool LevelSelectScene::_is_unlocked(int level_absolute_index) const {
    return level_absolute_index <= _data_manager.state().furthest_level;
}

void LevelSelectScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _text_gen->set_blending_enabled(true);

    _selected_index = 0;
    _transition_requested = false;

    _rebuild_menu();
}

void LevelSelectScene::_rebuild_menu() {
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(false);
    }
    _menu_sprites.clear();

    const WorldEntry& world = WorldIndex::WORLDS[_world_index];
    const GameState& game_state = _data_manager.state();

    for (int i = 0; i < world.level_count; ++i) {
        const int absolute_index = world.start_index + i;

        TextBuffer<40> line;
        line.append(i == _selected_index ? "> " : "  ")
            .append("Level ")
            .append_uint(unsigned(i + 1));

        if (!_is_unlocked(absolute_index)) {
            line.append(" (Locked)");
        } else if (
            absolute_index < GameState::MAX_LEVELS &&
            game_state.best_time_frames[absolute_index] > 0) {
            // The personal-best clear time, e.g. " 01:23.45", plus a "*"
            // if that level has ever been cleared without dying.
            line.append(' ').append_time(
                frames_to_time(game_state.best_time_frames[absolute_index]));
            if (has_no_death_clear(game_state, absolute_index)) {
                line.append('*');
            }
        }

        _text_gen->generate(
            Cfg::StartScreen::X, Cfg::StartScreen::Y + i * 16, line.c_str(),
            _menu_sprites);
    }

    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_blending_enabled(true);
    }
}

void LevelSelectScene::update() {
    if (!_transition_requested) {
        const WorldEntry& world = WorldIndex::WORLDS[_world_index];
        bool changed = false;

        if (bn::keypad::down_pressed()) {
            _selected_index = (_selected_index + 1) % world.level_count;
            changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        } else if (bn::keypad::up_pressed()) {
            _selected_index =
                (_selected_index + world.level_count - 1) % world.level_count;
            changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        }

        if (changed) {
            _rebuild_menu();
        }

        if (bn::keypad::a_pressed()) {
            const int absolute_index = world.start_index + _selected_index;

            if (_is_unlocked(absolute_index)) {
                _transition_requested = true;

                // Jump directly to the chosen level. Mirrors the
                // progression write LevelScene::update() performs when
                // advancing normally, but does not touch deaths/timer -
                // those stay whatever they were in the loaded save slot.
                _data_manager.state().level =
                    static_cast<int16_t>(absolute_index);
                _data_manager.save();

                auto next = bn::make_unique<LevelScene>(
                    _player, _levels, _data_manager, _level_manager);
                core::SceneManager::instance().set_next_scene(bn::move(next));

                AudioSettings::instance().play_sfx(bn::sound_items::confirm);
            } else {
                AudioSettings::instance().play_sfx(bn::sound_items::cancel);
            }
        } else if (bn::keypad::b_pressed()) {
            _transition_requested = true;

            auto next = bn::make_unique<WorldSelectScene>(
                _player, _levels, _data_manager, _level_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next));

            AudioSettings::instance().play_sfx(bn::sound_items::cancel);
        }
    }

    bn::core::update();
}
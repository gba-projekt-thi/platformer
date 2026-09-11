#include "level_select_scene.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "data_manager.h"
#include "level_manager.h"
#include "level_scene.h"
#include "player.h"
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
    return level_absolute_index <= _data_manager.state().level;
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

    for (int i = 0; i < world.level_count; ++i) {
        const int absolute_index = world.start_index + i;

        char buf[32];
        int pos = 0;
        buf[pos++] = (i == _selected_index) ? '>' : ' ';
        buf[pos++] = ' ';
        const char* label = "Level ";
        for (const char* p = label; *p; ++p) {
            buf[pos++] = *p;
        }
        buf[pos++] = char('1' + i);
        if (!_is_unlocked(absolute_index)) {
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
            bn::sound_items::select.play();
        } else if (bn::keypad::up_pressed()) {
            _selected_index =
                (_selected_index + world.level_count - 1) % world.level_count;
            changed = true;
            bn::sound_items::select.play();
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

                bn::sound_items::confirm.play();
            } else {
                bn::sound_items::cancel.play();
            }
        } else if (bn::keypad::b_pressed()) {
            _transition_requested = true;

            auto next = bn::make_unique<WorldSelectScene>(
                _player, _levels, _data_manager, _level_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next));

            bn::sound_items::cancel.play();
        }
    }

    bn::core::update();
}
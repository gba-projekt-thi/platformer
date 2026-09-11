#include "world_select_scene.h"

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
    const int furthest = _data_manager.state().level;
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
            bn::sound_items::select.play();
        } else if (bn::keypad::up_pressed()) {
            _selected_index = (_selected_index + WorldIndex::WORLD_COUNT - 1) %
                              WorldIndex::WORLD_COUNT;
            changed = true;
            bn::sound_items::select.play();
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

                bn::sound_items::confirm.play();
            } else {
                bn::sound_items::cancel.play();
            }
        } else if (bn::keypad::b_pressed()) {
            _transition_requested = true;

            auto next = bn::make_unique<StartScene>(
                _player, _levels, _data_manager, _level_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next));

            bn::sound_items::cancel.play();
        }
    }

    bn::core::update();
}
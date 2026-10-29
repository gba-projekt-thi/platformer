#include "statistics_scene.h"

#include "audio_settings.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "data_manager.h"
#include "level_manager.h"
#include "player.h"
#include "start_scene.h"
#include "statistics.h"
#include "text_buffer.h"

StatisticsScene::StatisticsScene(
    int slot,
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager)
    : _slot(slot),
      _player(player),
      _levels(levels),
      _data_manager(data_manager),
      _level_manager(level_manager) {}

void StatisticsScene::init() {
    _player.set_visible(false);
    _player.set_hud_visible(false);
    _peek_slot();

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _text_gen->set_blending_enabled(true);

    _rebuild();
}

void StatisticsScene::_peek_slot() {
    // An empty slot has no statistics yet: all zeros.
    if (!_data_manager.peek_state(_slot, _state)) {
        _state = GameState{};
    }
}

void StatisticsScene::_rebuild() {
    _sprites.clear();

    TextBuffer<24> title;
    title.append("< Slot ").append_uint(unsigned(_slot + 1)).append(" stats >");
    _text_gen->generate(
        Cfg::Statistics::X, Cfg::Statistics::TITLE_Y, title.c_str(), _sprites);

    for (int row = 0; row < statistics::VISIBLE_LINES; ++row) {
        _text_gen->generate(
            Cfg::Statistics::X,
            Cfg::Statistics::FIRST_LINE_Y + row * Cfg::Statistics::LINE_SPACING,
            statistics::line(_state, _top + row).c_str(), _sprites);
    }

    _text_gen->generate(
        Cfg::Statistics::X, Cfg::Statistics::HINT_Y, "D-pad: scroll  B: back",
        _sprites);

    for (bn::sprite_ptr& sprite : _sprites) {
        sprite.set_blending_enabled(true);
    }
}

void StatisticsScene::update() {
    if (!_transition_requested) {
        int delta = 0;
        if (bn::keypad::down_pressed()) {
            delta = 1;
        } else if (bn::keypad::up_pressed()) {
            delta = -1;
        }

        if (delta != 0) {
            const int top = statistics::scroll(_top, delta);
            if (top != _top) {
                _top = top;
                _rebuild();
                AudioSettings::instance().play_sfx(bn::sound_items::select);
            }
        }

        // Left/Right: the previous/next save slot, from the top of its list.
        int slot_delta = 0;
        if (bn::keypad::right_pressed()) {
            slot_delta = 1;
        } else if (bn::keypad::left_pressed()) {
            slot_delta = Cfg::StartScreen::SAVE_SLOT_COUNT - 1;
        }
        if (slot_delta != 0) {
            _slot = (_slot + slot_delta) % Cfg::StartScreen::SAVE_SLOT_COUNT;
            _top = 0;
            _peek_slot();
            _rebuild();
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        }

        if (bn::keypad::b_pressed()) {
            _transition_requested = true;

            auto next_scene = bn::make_unique<StartScene>(
                _player, _levels, _data_manager, _level_manager);
            core::SceneManager::instance().set_next_scene(bn::move(next_scene));

            AudioSettings::instance().play_sfx(bn::sound_items::cancel);
        }
    }

    bn::core::update();
}

#include "pause_controller.h"

#include "audio_settings.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_music.h"
#include "bn_sprite_text_generator.h"
#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"
#include "data_manager.h"

namespace {
constexpr const char* MENU_LABELS[4] = {
    "Continue", "Restart Level", "Options", "Title Screen"};
}  // namespace

PauseController::PauseController(DataManager& data_manager)
    : _data_manager(data_manager) {}

void PauseController::_init_pause_menu() {
    if (_pause_menu_initialized) {
        return;
    }

    _text_gen.emplace(common::variable_8x16_sprite_font);
    _text_gen->set_z_order(Cfg::ZOrder::PAUSE_MENU);
    _text_gen->set_blending_enabled(true);

    _text_gen->generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_TITLE, "Paused", _title_sprites);
    for (bn::sprite_ptr& sprite : _title_sprites) {
        sprite.set_visible(false);
        sprite.set_blending_enabled(true);
    }

    _pause_menu_initialized = true;
}

void PauseController::_set_visible(bool visible) {
    for (bn::sprite_ptr& sprite : _title_sprites) {
        sprite.set_visible(visible);
    }
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(visible);
    }
}

void PauseController::_rebuild_menu() {
    // Rebuild the option text so the ">" cursor tracks the current
    // selection, the same pattern used by the save-slot list in
    // StartScene.
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(false);
    }
    _menu_sprites.clear();

    for (int i = 0; i < int(MenuOption::Count); ++i) {
        char buf[24];
        int pos = 0;
        buf[pos++] = (i == _selected_index) ? '>' : ' ';
        buf[pos++] = ' ';
        for (const char* p = MENU_LABELS[i]; *p; ++p) {
            buf[pos++] = *p;
        }
        buf[pos] = '\0';

        _text_gen->generate(
            Cfg::PauseMenu::X,
            Cfg::PauseMenu::Y_OPTION_0 + i * Cfg::PauseMenu::OPTION_SPACING,
            buf, _menu_sprites);
    }

    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(_paused);
        sprite.set_blending_enabled(true);
    }
}

void PauseController::_rebuild_options_menu() {
    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(false);
    }
    _menu_sprites.clear();

    const AudioSettings& audio = AudioSettings::instance();
    const uint8_t levels[2] = {audio.music_level(), audio.sfx_level()};
    const char* labels[2] = {"Music", "SFX"};

    for (int i = 0; i < 2; ++i) {
        char buf[24];
        int pos = 0;
        buf[pos++] = (i == _option_row) ? '>' : ' ';
        buf[pos++] = ' ';
        for (const char* p = labels[i]; *p; ++p) {
            buf[pos++] = *p;
        }
        buf[pos++] = ' ';
        buf[pos++] = '<';
        buf[pos++] = ' ';
        buf[pos++] = char('0' + levels[i]);
        buf[pos++] = '/';
        buf[pos++] = char('0' + AudioSettings::MAX_LEVEL);
        buf[pos++] = ' ';
        buf[pos++] = '>';
        buf[pos] = '\0';

        _text_gen->generate(
            Cfg::PauseMenu::X,
            Cfg::PauseMenu::Y_OPTION_0 + i * Cfg::PauseMenu::OPTION_SPACING,
            buf, _menu_sprites);
    }

    for (bn::sprite_ptr& sprite : _menu_sprites) {
        sprite.set_visible(_paused);
        sprite.set_blending_enabled(true);
    }
}

void PauseController::_change_option_level(int delta) {
    AudioSettings& audio = AudioSettings::instance();
    GameState& state = _data_manager.state();

    if (_option_row == 0) {
        int level = int(audio.music_level()) + delta;
        if (level < 0) {
            level = 0;
        }
        if (level > int(AudioSettings::MAX_LEVEL)) {
            level = AudioSettings::MAX_LEVEL;
        }
        audio.set_music_level(uint8_t(level));
        state.music_volume = audio.music_level();

        // Apply immediately so the player hears the change live. Safe to
        // call directly here since the scene manager sits idle while
        // paused (no fade in progress to fight with).
        bn::music::set_volume(audio.music_scale());
    } else {
        int level = int(audio.sfx_level()) + delta;
        if (level < 0) {
            level = 0;
        }
        if (level > int(AudioSettings::MAX_LEVEL)) {
            level = AudioSettings::MAX_LEVEL;
        }
        audio.set_sfx_level(uint8_t(level));
        state.sfx_volume = audio.sfx_level();

        // Audible preview at the just-set level, so raising SFX from 0
        // is itself audible feedback.
        audio.play_sfx(bn::sound_items::select);
    }
}

void PauseController::reset() {
    _init_pause_menu();
    _paused = false;
    _prev_paused = false;
    _selected_index = 0;
    _showing_options = false;
    _option_row = 0;
    _set_visible(false);
}

PauseController::Action PauseController::update() {
    if (bn::keypad::start_pressed()) {
        _paused = !_paused;
    }

    if (_prev_paused != _paused) {
        _prev_paused = _paused;
        _set_visible(_paused);

        if (_paused) {
            _selected_index = 0;
            _showing_options = false;
            _rebuild_menu();
            _set_visible(true);
            bn::music::pause();
            AudioSettings::instance().play_sfx(bn::sound_items::menu);
        } else {
            _set_visible(false);
            AudioSettings::instance().play_sfx(bn::sound_items::cancel);
            bn::music::resume();
        }
    }

    if (_paused) {
        if (_showing_options) {
            bool changed = false;

            if (bn::keypad::down_pressed() || bn::keypad::up_pressed()) {
                _option_row = 1 - _option_row;
                changed = true;
                AudioSettings::instance().play_sfx(bn::sound_items::select);
            } else if (bn::keypad::right_pressed()) {
                _change_option_level(1);
                changed = true;
            } else if (bn::keypad::left_pressed()) {
                _change_option_level(-1);
                changed = true;
            }

            if (changed) {
                _rebuild_options_menu();
            }

            if (bn::keypad::b_pressed()) {
                // Commit to SRAM once, on leaving the sub-menu - not per
                // keypress inside it. SRAM writes are relatively
                // expensive (see save-sync policy elsewhere in the code).
                _data_manager.save();
                _showing_options = false;
                _rebuild_menu();
                AudioSettings::instance().play_sfx(bn::sound_items::cancel);
            }

            bn::core::update();
            return Action::None;
        }

        bool selection_changed = false;

        if (bn::keypad::down_pressed()) {
            _selected_index = (_selected_index + 1) % int(MenuOption::Count);
            selection_changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        } else if (bn::keypad::up_pressed()) {
            _selected_index = (_selected_index + int(MenuOption::Count) - 1) %
                              int(MenuOption::Count);
            selection_changed = true;
            AudioSettings::instance().play_sfx(bn::sound_items::select);
        }

        if (selection_changed) {
            _rebuild_menu();
        }

        if (bn::keypad::a_pressed()) {
            const MenuOption option = static_cast<MenuOption>(_selected_index);

            AudioSettings::instance().play_sfx(bn::sound_items::confirm);

            if (option == MenuOption::Options) {
                // Stay paused, switch into the sub-menu - no Action
                // returned, no scene transition involved.
                _showing_options = true;
                _option_row = 0;
                _rebuild_options_menu();
                bn::core::update();
                return Action::None;
            }

            // Leave the pause state immediately so the menu closes on the
            // same frame the action is confirmed, regardless of what the
            // caller does with the returned action afterwards (e.g. a
            // scene transition that destroys this controller).
            _paused = false;
            _prev_paused = false;
            _set_visible(false);
            bn::music::resume();

            switch (option) {
                case MenuOption::Continue:
                    return Action::None;
                case MenuOption::Restart:
                    return Action::DeathRequested;
                case MenuOption::TitleScreen:
                    return Action::ReturnToTitleRequested;
                default:
                    return Action::None;
            }
        }

        bn::core::update();
    }

    return Action::None;
}

bool PauseController::paused() const {
    return _paused;
}
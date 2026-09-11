#include "pause_controller.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_music.h"
#include "bn_sprite_text_generator.h"
#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"

namespace {
constexpr const char* MENU_LABELS[3] = {
    "Continue", "Restart Level", "Title Screen"};
}  // namespace

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

void PauseController::reset() {
    _init_pause_menu();
    _paused = false;
    _prev_paused = false;
    _selected_index = 0;
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
            _rebuild_menu();
            _set_visible(true);
            bn::music::pause();
            bn::sound_items::menu.play();
        } else {
            _set_visible(false);
            bn::sound_items::cancel.play();
            bn::music::resume();
        }
    }

    if (_paused) {
        bool selection_changed = false;

        if (bn::keypad::down_pressed()) {
            _selected_index = (_selected_index + 1) % int(MenuOption::Count);
            selection_changed = true;
            bn::sound_items::select.play();
        } else if (bn::keypad::up_pressed()) {
            _selected_index = (_selected_index + int(MenuOption::Count) - 1) %
                              int(MenuOption::Count);
            selection_changed = true;
            bn::sound_items::select.play();
        }

        if (selection_changed) {
            _rebuild_menu();
        }

        if (bn::keypad::a_pressed()) {
            const MenuOption option = static_cast<MenuOption>(_selected_index);

            bn::sound_items::confirm.play();

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
#include "pause_controller.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_music.h"
#include "bn_sprite_text_generator.h"
#include "cfg.h"
#include "common_variable_8x16_sprite_font.h"

void PauseController::init_pause_menu() {
    if (_pause_menu_initialized) {
        return;
    }

    bn::sprite_text_generator text_gen(common::variable_8x16_sprite_font);
    text_gen.set_z_order(Cfg::ZOrder::PAUSE_MENU);
    text_gen.set_blending_enabled(true);

    text_gen.generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_0, "Paused", _pause_sprites);
    text_gen.generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_1, "Continue: Start",
        _pause_sprites);
    text_gen.generate(
        Cfg::PauseMenu::X, Cfg::PauseMenu::Y_2, "Die: Select", _pause_sprites);
    for (bn::sprite_ptr& sprite : _pause_sprites) {
        sprite.set_visible(false);
        sprite.set_blending_enabled(true);
    }
    _pause_menu_initialized = true;
}

void PauseController::set_visible(bool visible) {
    for (bn::sprite_ptr& sprite : _pause_sprites) {
        sprite.set_visible(visible);
    }
}

void PauseController::reset() {
    init_pause_menu();
    _paused = false;
    _prev_paused = false;
    set_visible(false);
}

PauseController::Action PauseController::update() {
    if (bn::keypad::start_pressed()) {
        _paused = !_paused;
    }

    if (_prev_paused != _paused) {
        _prev_paused = _paused;
        set_visible(_paused);

        if (_paused) {
            bn::music::pause();
            bn::sound_items::menu.play();
        } else {
            bn::sound_items::cancel.play();
            bn::music::resume();
        }
    }
    if (_paused) {
        if (bn::keypad::select_released()) {
            _paused = false;
            return Action::DeathRequested;
        }
        bn::core::update();
    }

    return Action::None;
}

bool PauseController::paused() const {
    return _paused;
}
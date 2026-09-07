#include "player_hud.h"

#include "bn_keypad.h"

PlayerHud::PlayerHud()
    : _death_counter(),
      _death_counter_text_gen(common::variable_8x16_sprite_font),
      _death_counter_hud(_death_counter_text_gen, _death_counter),
      _timer(),
      _timer_hud(_timer) {
    _death_counter_text_gen.set_blending_enabled(true);
}

void PlayerHud::tick() {
    if (bn::keypad::select_pressed()) {
        _timer_hud.set_visible(!_timer_hud.visible());
    }
    _timer.tick();
    _timer_hud.update();
}

void PlayerHud::set_visible(bool visible) {
    _death_counter_hud.set_visible(visible);
    _timer_hud.set_visible(visible);
}

unsigned int PlayerHud::deaths() const {
    return _death_counter.count();
}

void PlayerHud::set_deaths(unsigned int deaths) {
    _death_counter.set_count(deaths);
    _death_counter_hud.update();
}

void PlayerHud::on_player_death() {
    _death_counter.on_player_death();
    _death_counter_hud.update();
}

Timer& PlayerHud::timer() {
    return _timer;
}
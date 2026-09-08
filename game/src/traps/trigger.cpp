#include "trigger.h"

Trigger::Trigger(
    bn::fixed t_x,
    bn::fixed t_y,
    bn::fixed t_width,
    bn::fixed t_height,
    bool t_default_on)
    : PhysicsBody(
          t_x,
          t_y,
          t_width,
          t_height,
          Cfg::Layer::TRIGGER,
          Cfg::Layer::PLAYER,
          0),
      _triggered(t_default_on),
      _default_on(t_default_on) {}

bool Trigger::is_triggered() const {
    return _triggered;
}

void Trigger::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    _triggered = !_default_on;
}

void Trigger::reset() {
    _triggered = _default_on;
}
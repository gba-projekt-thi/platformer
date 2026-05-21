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
      triggered(t_default_on),
      default_on(t_default_on) {}

bool Trigger::is_triggered() const {
    return triggered;
}

void Trigger::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    // Activate trigger once entered.
    triggered = !default_on;
}

void Trigger::reset() {
    triggered = default_on;
}
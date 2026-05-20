#include "trigger.h"

Trigger::Trigger(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    bool default_on)
    : PhysicsBody(
          start_x,
          start_y,
          width,
          height,
          Cfg::Layer::TRIGGER,
          Cfg::Layer::PLAYER,
          0),
      triggered(default_on),
      default_on(default_on) {}

bool Trigger::is_triggered() const {
    return triggered;
}

void Trigger::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    activate();
}

void Trigger::activate() {
    triggered = true;
}

void Trigger::deactivate() {
    triggered = false;
}

void Trigger::reset() {
    triggered = default_on;
}
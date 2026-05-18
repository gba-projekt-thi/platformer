#include "trigger.h"

Trigger::Trigger(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    bool trigger_default_on)
    : PhysicsBody(
          start_x,
          start_y,
          width,
          height,
          Cfg::Layer::TRIGGER,
          Cfg::Layer::PLAYER,
          0),
      triggered(trigger_default_on),
      default_on(trigger_default_on) {}

bool Trigger::is_triggered() const {
    return triggered;
}

void Trigger::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    // The player has entered this trigger area, so activate any linked traps.
    triggered = !default_on;
}

void Trigger::reset() {
    // Reset the trigger for the next respawn or level retry.
    triggered = default_on;
}
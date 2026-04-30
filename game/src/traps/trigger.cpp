#include "trigger.h"

Trigger::Trigger(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height)
    : PhysicsBody(t_start_x, t_start_y, t_width, t_height, LAYERS, MASK, BLOCK),
      triggered(false) {}

bool Trigger::is_triggered() {
    return triggered;
}

void Trigger::on_enter([[maybe_unused]] StaticBody& body) {
    // The player has entered this trigger area, so activate any linked traps.
    triggered = true;
}

void Trigger::reset() {
    // Reset the trigger for the next respawn or level retry.
    triggered = false;
}

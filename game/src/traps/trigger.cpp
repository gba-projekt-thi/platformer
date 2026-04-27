#include "trigger.h"

Trigger::Trigger(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed t_width,
    bn::fixed t_height)
    : PhysicsBody(start_x, start_y, t_width, t_height, LAYERS, MASK, BLOCK),
      triggered(false) {}

bool Trigger::is_triggered() {
    return triggered;
}

void Trigger::on_enter(StaticBody& body) {
    // no need to check the mask because we only check the player layer anyways
    triggered = true;
}

void Trigger::reset() {
    triggered = false;
}

#include "trigger.h"

Trigger::Trigger(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height)
    : PhysicsBody(x, y, width, height, LAYERS, MASK, BLOCK), triggered(false) {}

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

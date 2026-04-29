#include "door.h"

Door::Door(bn::fixed in_x, bn::fixed in_y)
    : PhysicsBody(in_x, in_y, 32, 32, LAYERS, MASK, 0),
      trap_sprite(
          bn::sprite_items::door32x32.create_sprite(in_x, in_y),
          in_x,
          in_y),
      action(
          bn::create_sprite_animate_action_forever(
              trap_sprite.sprite(),
              8,
              bn::sprite_items::door32x32.tiles_item(),
              0,
              2,
              4,
              6,
              8,
              10,
              12)) {
    this->sprite = &trap_sprite;
}

void Door::update() {
    action.update();
}

void Door::on_enter([[maybe_unused]] StaticBody& body) {
    _reached = 1;
}

bool Door::reached() {
    return _reached;
}
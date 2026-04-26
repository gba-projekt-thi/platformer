#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    const bn::sprite_item& sprite,
    uint16_t block)
    : PhysicsBody(start_x, start_y, width, height, LAYERS, MASK, block),
      trap_sprite(sprite.create_sprite(start_x, start_y), start_x, start_y),
      action(bn::create_sprite_animate_action_forever(
          trap_sprite.sprite(),
          26,
          sprite.tiles_item(),
          0,
          1)) {
    this->sprite = &trap_sprite;
}

void BaseTrap::update() {
    action.update();
}

void BaseTrap::on_enter(StaticBody& body) {
    static_cast<Player&>(body).death();
}

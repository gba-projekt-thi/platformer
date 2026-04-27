#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite,
    uint16_t t_block)
    : PhysicsBody(start_x, start_y, t_width, t_height, LAYERS, MASK, t_block),
      trap_sprite(t_sprite.create_sprite(start_x, start_y), start_x, start_y),
      action(
          bn::create_sprite_animate_action_forever(
              trap_sprite.sprite(),
              26,
              t_sprite.tiles_item(),
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

#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite,
    uint16_t t_block)
    : PhysicsBody(
          t_start_x,
          t_start_y,
          t_width,
          t_height,
          LAYERS,
          MASK,
          t_block),
      trap_sprite(
          t_sprite.create_sprite(t_start_x, t_start_y),
          t_start_x,
          t_start_y),
      action(bn::create_sprite_animate_action_forever(
          trap_sprite.sprite(),
          26,
          t_sprite.tiles_item(),
          0,
          1,
          2)) {
    // Base trap uses a physics body with an animated sprite.
    this->sprite = &trap_sprite;
}

void BaseTrap::update() {
    // Animate the trap each frame.
    action.update();
}

void BaseTrap::on_enter(StaticBody& body) {
    // When the player enters a trap, trigger death.
    static_cast<Player&>(body).death();
}

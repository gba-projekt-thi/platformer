#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite,
    uint16_t t_block,
    bn::fixed t_max_vel)
    : PhysicsBody(
          t_start_x,
          t_start_y,
          t_width,
          t_height,
          Cfg::Layer::TRAP,
          Cfg::Layer::PLAYER,
          t_block,
          t_max_vel),
      trap_sprite(
          t_sprite.create_sprite(t_start_x, t_start_y),
          t_start_x,
          t_start_y) {
    this->sprite = &trap_sprite;

    int count = t_sprite.tiles_item().graphics_count();

    if (count > 1) {
        if (count == 2) {
            action.emplace(bn::create_sprite_animate_action_forever(
                trap_sprite.sprite(), 26, t_sprite.tiles_item(), 0, 1));
        } else if (count == 3) {
            action.emplace(bn::create_sprite_animate_action_forever(
                trap_sprite.sprite(), 26, t_sprite.tiles_item(), 0, 1, 2));
        } else if (count == 4) {
            action.emplace(bn::create_sprite_animate_action_forever(
                trap_sprite.sprite(), 26, t_sprite.tiles_item(), 0, 1, 2, 3));
        } else if (count == 5) {
            action.emplace(bn::create_sprite_animate_action_forever(
                trap_sprite.sprite(), 26, t_sprite.tiles_item(), 0, 1, 2, 3,
                4));
        } else if (count == 6) {
            action.emplace(bn::create_sprite_animate_action_forever(
                trap_sprite.sprite(), 26, t_sprite.tiles_item(), 0, 1, 2, 3, 4,
                5));
        } else if (count == 7) {
            action.emplace(bn::create_sprite_animate_action_forever(
                trap_sprite.sprite(), 26, t_sprite.tiles_item(), 0, 1, 2, 3, 4,
                5, 6));
        } else if (count >= 8) {
            action.emplace(bn::create_sprite_animate_action_forever(
                trap_sprite.sprite(), 26, t_sprite.tiles_item(), 0, 1, 2, 3, 4,
                5, 6, 7));
        }
    }
}

void BaseTrap::update() {
    if (action) {
        action->update();
    }
}

void BaseTrap::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    StaticBody* body) {
    static_cast<Player*>(body)->death();
}
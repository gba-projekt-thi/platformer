#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed t_x,
    bn::fixed t_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite_item,
    int t_sprite_waits,
    bn::span<const uint16_t> t_graphics_indexes,
    uint16_t t_blocking_layers,
    bn::fixed t_max_vel)
    : PhysicsBody(
          t_x,
          t_y,
          t_width,
          t_height,
          Cfg::Layer::TRAP,
          Cfg::Layer::PLAYER,
          t_blocking_layers,
          t_max_vel),
      trap_sprite(t_sprite_item.create_sprite(t_x, t_y), t_x, t_y) {
    sprite = &trap_sprite;

    trap_sprite.sprite().set_blending_enabled(true);

    // Create animation only if animation frames exist.
    if (!t_graphics_indexes.empty()) {
        _animation_action =
            bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>::forever(
                trap_sprite.sprite(), t_sprite_waits,
                t_sprite_item.tiles_item(), t_graphics_indexes);
    }
}

void BaseTrap::update() {
    // Update animation if enabled.
    if (_animation_action) {
        _animation_action->update();
    }
}

void BaseTrap::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    StaticBody* body) {
    // Only react to player collisions.
    if (body && (body->layers & Cfg::Layer::PLAYER)) {
        static_cast<Player*>(body)->death();
    }
}

BaseTrap::~BaseTrap() {
    SpriteRegistry::instance().unregister_sprite(&trap_sprite);
}
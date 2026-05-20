#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed x,
    bn::fixed y,
    bn::fixed width,
    bn::fixed height,
    const bn::sprite_item& sprite_item,
    int sprite_waits,
    bn::span<const uint16_t> graphics_indexes,
    uint16_t blocking_layers,
    bn::fixed max_vel)
    : PhysicsBody(
          x,
          y,
          width,
          height,
          Cfg::Layer::TRAP,
          Cfg::Layer::PLAYER,
          blocking_layers,
          max_vel),
      trap_sprite(sprite_item.create_sprite(x, y), x, y),
      _graphics_indexes(graphics_indexes) {
    sprite = &trap_sprite;

    trap_sprite.sprite().set_blending_enabled(true);

    if (!_graphics_indexes.empty()) {
        _animation_action =
            bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>::forever(
                trap_sprite.sprite(), sprite_waits, sprite_item.tiles_item(),
                _graphics_indexes);
    }
}

void BaseTrap::update() {
    if (_animation_action) {
        _animation_action->update();
    }
}

void BaseTrap::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    StaticBody* body) {
    // Traps only react to player collisions.
    if (body && (body->layers & Cfg::Layer::PLAYER)) {
        static_cast<Player*>(body)->death();
    }
}

BaseTrap::~BaseTrap() {
    SpriteRegistry::instance().unregister_sprite(&trap_sprite);
}
#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed x,
    bn::fixed y,
    bn::fixed width,
    bn::fixed height,
    const bn::sprite_item& sprite_item,
    int sprite_waits,
    bn::span<const uint16_t> trap_graphics_indexes,
    uint16_t trap_block,
    bn::fixed max_vel)
    : PhysicsBody(
          x,
          y,
          width,
          height,
          Cfg::Layer::TRAP,
          Cfg::Layer::PLAYER,
          trap_block,
          max_vel),
      trap_sprite(sprite_item.create_sprite(x, y), x, y),
      graphics_indexes(trap_graphics_indexes) {
    this->sprite = &trap_sprite;

    if (!trap_graphics_indexes.empty()) {
        action = bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>::forever(
            trap_sprite.sprite(), sprite_waits, sprite_item.tiles_item(),
            graphics_indexes);
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

BaseTrap::~BaseTrap() {
    SpriteRegistry::instance().unregister_sprite(&trap_sprite);
}
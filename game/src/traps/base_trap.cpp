#include "base_trap.h"
#include "player.h"

BaseTrap::BaseTrap(const TrapData& data, bn::fixed max_vel)
    : BaseTrap(data, data.x, data.y, max_vel) {}

BaseTrap::BaseTrap(
    const TrapData& data,
    bn::fixed x,
    bn::fixed y,
    bn::fixed max_vel)
    : PhysicsBody(
          x + data.offset_x,
          y + data.offset_y,
          data.width,
          data.height,
          Cfg::Layer::TRAP,
          Cfg::Layer::PLAYER,
          0,  // nothing blocks a trap
          max_vel),
      _trap_sprite(data.sprite.create_sprite(x, y), x, y) {
    sprite = &_trap_sprite;
    if (data.sprite_scale != 1) {
        _trap_sprite.sprite().set_scale(data.sprite_scale);
    }

    // Empty graphics span means:
    // this trap is static/non-animated.
    if (!data.graphic_indexes.empty()) {
        BN_ASSERT(
            data.sprite_waits >= 1,
            "Animated trap requires animation_wait >= 1");
        _animation_action =
            bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>::forever(
                _trap_sprite.sprite(), data.sprite_waits,
                data.sprite.tiles_item(), data.graphic_indexes);
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
    if (body && (body->layers & Cfg::Layer::PLAYER)) {
        static_cast<Player*>(body)->death();
    }
}

BaseTrap::~BaseTrap() {
    SpriteRegistry::instance().unregister_sprite(&_trap_sprite);
}
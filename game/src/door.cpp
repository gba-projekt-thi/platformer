#include "door.h"

Door::Door(
    bn::fixed in_x,
    bn::fixed in_y,
    const bn::sprite_item& sprite_item,
    bn::span<const uint16_t> graphics_indexes,
    int animation_wait,
    bool flip_horizontal)
    : PhysicsBody(in_x, in_y, 2, 2, Cfg::Layer::DOOR, Cfg::Layer::PLAYER, 0),
      _door_sprite(sprite_item.create_sprite(in_x, in_y), in_x, in_y) {
    _door_sprite.sprite().set_horizontal_flip(flip_horizontal);

    // Empty graphics span means: static/non-animated (same convention as
    // TrapData's graphics_indexes, see BaseTrap).
    if (!graphics_indexes.empty()) {
        BN_ASSERT(
            animation_wait >= 1, "Animated door requires animation_wait >= 1");
        _action = bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>::forever(
            _door_sprite.sprite(), animation_wait, sprite_item.tiles_item(),
            graphics_indexes);
    }
    _door_sprite.sprite().set_z_order(Cfg::ZOrder::DOOR);
    _door_sprite.sprite().set_blending_enabled(true);
    this->sprite = &_door_sprite;
    this->shape_pos.offset_y = +6;
}

void Door::update() {
    if (_action.has_value()) {
        _action->update();
    }
}

void Door::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    _reached = true;
}

bool Door::reached() const {
    return _reached;
}

Door::~Door() {
    SpriteRegistry::instance().unregister_sprite(&_door_sprite);
}
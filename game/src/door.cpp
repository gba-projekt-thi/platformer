#include "door.h"

Door::Door(const DoorData& data)
    : PhysicsBody(
          data.x,
          data.y,
          2,
          2,
          Cfg::Layer::DOOR,
          Cfg::Layer::PLAYER,
          0),
      _origin_x(data.x),
      _origin_y(data.y),
      _flee(
          data.flee_path.data(),
          data.flee_path.size(),
          data.flee_range,
          data.flee_frames),
      _door_sprite(data.sprite.create_sprite(data.x, data.y), data.x, data.y) {
    _door_sprite.sprite().set_horizontal_flip(data.sprite_flip_horizontal);

    // Empty graphics span means: static/non-animated (same convention as
    // TrapData's graphics_indexes, see BaseTrap).
    if (!data.sprite_graphics_indexes.empty()) {
        BN_ASSERT(
            data.sprite_animation_wait >= 1,
            "Animated door requires animation_wait >= 1");
        _action = bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>::forever(
            _door_sprite.sprite(), data.sprite_animation_wait,
            data.sprite.tiles_item(), data.sprite_graphics_indexes);
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

void Door::flee_from(bn::fixed duck_x, bn::fixed duck_y) {
    if (_flee.done() && !_flee.moving()) {
        return;
    }
    _flee.update(duck_x - _origin_x, duck_y - _origin_y);
    _place();
}

void Door::reset_flee() {
    _flee.reset();
    _place();
}

// Direct position writes bypass PhysicsBody::move(): the collision shape and
// the sprite follow by hand (as in PathTrap).
void Door::_place() {
    pos.x = _origin_x + _flee.x();
    pos.y = _origin_y + _flee.y();
    shape_pos.move(pos.x, pos.y);
    _door_sprite.pos.move(pos.x, pos.y);
}

bool Door::reached() const {
    return _reached;
}

Door::~Door() {
    SpriteRegistry::instance().unregister_sprite(&_door_sprite);
}
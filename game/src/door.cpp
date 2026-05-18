#include "door.h"

Door::Door(bn::fixed x, bn::fixed y)
    : PhysicsBody(x, y, 6, 8, Cfg::Layer::DOOR, Cfg::Layer::PLAYER, 0),
      door_sprite(bn::sprite_items::door32x32.create_sprite(x, y), x, y),
      action(bn::create_sprite_animate_action_forever(
          door_sprite.sprite(),
          8,
          bn::sprite_items::door32x32.tiles_item(),
          0,
          2,
          4,
          6,
          8,
          10,
          12)) {
    // The door is a static physics body and animated sprite.
    door_sprite.sprite().set_z_order(Cfg::ZOrder::DOOR);
    this->sprite = &door_sprite;
}

void Door::update() {
    // Animate the door each frame.
    action.update();
}

void Door::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    reached_flag = true;
}

bool Door::reached() const {
    return reached_flag;
}

Door::~Door() {
    SpriteRegistry::instance().unregister_sprite(&door_sprite);
}
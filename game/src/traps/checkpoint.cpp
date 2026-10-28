#include "checkpoint.h"

#include "bn_sprite_items_checkpoint16x16.h"

namespace {
constexpr int TOUCH_WIDTH = 16;
constexpr int TOUCH_HEIGHT = 16;
constexpr int FRAME_OFF = 0;
constexpr int FRAME_ON = 1;
}  // namespace

Checkpoint::Checkpoint(bn::fixed t_x, bn::fixed t_y)
    : PhysicsBody(
          t_x,
          t_y,
          TOUCH_WIDTH,
          TOUCH_HEIGHT,
          Cfg::Layer::TRIGGER,
          Cfg::Layer::PLAYER,
          0),
      _flag(
          bn::sprite_items::checkpoint16x16.create_sprite(t_x, t_y),
          t_x,
          t_y),
      _tiles_off(bn::sprite_items::checkpoint16x16.tiles_item().create_tiles(
          FRAME_OFF)),
      _tiles_on(bn::sprite_items::checkpoint16x16.tiles_item().create_tiles(
          FRAME_ON)) {
    _flag.sprite().set_z_order(Cfg::ZOrder::CHECKPOINT);
    _flag.sprite().set_blending_enabled(true);
    sprite = &_flag;
}

void Checkpoint::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    _hit = true;
}

auto Checkpoint::take_hit() -> bool {
    const bool hit = _hit;
    _hit = false;
    return hit;
}

void Checkpoint::set_active(bool active) {
    _flag.sprite().set_tiles(active ? _tiles_on : _tiles_off);
}

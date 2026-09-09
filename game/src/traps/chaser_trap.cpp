#include "chaser_trap.h"

ChaserTrap::ChaserTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    bn::fixed t_offset_x,
    bn::fixed t_offset_y,
    const bn::sprite_item& t_sprite_item,
    int t_sprite_waits,
    bn::span<const uint16_t> t_graphics_indexes,
    uint16_t t_blocking_layers,
    bn::fixed t_follow_distance,
    bn::fixed t_chase_speed,
    Player& t_player)
    : BaseTrap(
          t_start_x,
          t_start_y,
          t_width,
          t_height,
          t_offset_x,
          t_offset_y,
          t_sprite_item,
          t_sprite_waits,
          t_graphics_indexes,
          t_blocking_layers,
          // ChaserTrap uses direct positional movement, not velocity.
          Cfg::Physics::NO_MAX_VELOCITY),
      _start_x(t_start_x),
      _start_y(t_start_y),
      _follow_distance(t_follow_distance),
      _chase_speed(t_chase_speed),
      _player(t_player) {}

void ChaserTrap::update() {
    BaseTrap::update();

    const bn::fixed target_x = _player.pos.x - _follow_distance;

    // Only ever flies right, chasing the player - never retreats left, so
    // standing still or moving left just widens the gap instead of the
    // trap backing off.
    if (target_x > pos.x) {
        bn::fixed step = target_x - pos.x;
        if (step > _chase_speed) {
            step = _chase_speed;
        }
        pos.x += step;

        // Direct position writes bypass PhysicsBody::move(), so sync the
        // collision shape and sprite manually.
        shape_pos.move(pos.x, pos.y);
        if (sprite) {
            sprite->pos.move(pos.x, pos.y);
        }
    }
}

void ChaserTrap::reset() {
    pos.x = _start_x;
    pos.y = _start_y;

    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(pos.x, pos.y);
    }
}
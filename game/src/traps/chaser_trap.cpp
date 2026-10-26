#include "chaser_trap.h"

ChaserTrap::ChaserTrap(
    const TrapData& data,
    bn::fixed chase_speed,
    Player& player)
    // ChaserTrap uses direct positional movement, not velocity.
    : BaseTrap(data, Cfg::Physics::NO_MAX_VELOCITY),
      _start_x(data.x),
      _start_y(data.y),
      _follow_distance(data.chase_follow_distance),
      _chase_speed(chase_speed),
      _player(player) {}

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
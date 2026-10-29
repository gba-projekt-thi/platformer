#include "chaser_trap.h"

#include "trap_timing.h"

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
      _limit(data.chase_limit),
      _player(player) {}

void ChaserTrap::update() {
    BaseTrap::update();

    const bn::fixed target_x = trap_timing::chase_target(
        _player.pos.x, _follow_distance, _start_x, _limit);

    // Only ever flies away from where it trails (right; left in a mirrored
    // level) - never retreats, so standing still or turning round just
    // widens the gap instead of the trap backing off.
    const bn::fixed next_x = trap_timing::chase_step(
        pos.x, target_x, _follow_distance, _chase_speed);
    if (next_x != pos.x) {
        pos.x = next_x;

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
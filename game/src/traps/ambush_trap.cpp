#include "ambush_trap.h"

AmbushTrap::AmbushTrap(const TrapData& data, bn::fixed speed, Player& player)
    // AmbushTrap uses direct positional movement, not velocity.
    : BaseTrap(data, Cfg::Physics::NO_MAX_VELOCITY),
      _start_x(pos.x),
      _start_y(pos.y),
      _vertical(data.ambush_vertical),
      _motion(data.ambush_range, speed, data.ambush_duration, data.ambush_rest),
      _player(player) {}

void AmbushTrap::update() {
    BaseTrap::update();
    bn::fixed distance = _player.pos.x - _start_x;
    if (distance < 0) {
        distance = -distance;
    }
    const bool was_dormant = _motion.state() == ambush::State::Dormant;
    _motion.update(distance);
    if (!was_dormant) {
        _place();
    }
}

void AmbushTrap::reset() {
    _motion.reset();
    _place();
}

// Direct position writes bypass PhysicsBody::move(), so the collision shape
// and the sprite follow by hand - same pattern as ChaserTrap/PathTrap.
void AmbushTrap::_place() {
    pos.x = _vertical ? _start_x : _start_x + _motion.offset();
    pos.y = _vertical ? _start_y + _motion.offset() : _start_y;
    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(pos.x, pos.y);
    }
}

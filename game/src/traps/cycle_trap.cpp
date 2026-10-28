#include "cycle_trap.h"

CycleTrap::CycleTrap(
    const TrapData& data,
    trap_timing::CycleTiming timing,
    unsigned phase_frames,
    Trigger* trigger)
    // CycleTrap uses direct positional movement, not velocity.
    : BaseTrap(data, Cfg::Physics::NO_MAX_VELOCITY),
      _origin_x(data.x + data.offset_x),
      _origin_y(data.y + data.offset_y),
      _dx(data.cycle_dx),
      _dy(data.cycle_dy),
      _clock(timing, phase_frames),
      _trigger(trigger) {
    // Already in the right place for the first frame of its round.
    _apply(_clock.progress());
}

void CycleTrap::update() {
    BaseTrap::update();

    const bool running = _trigger == nullptr || _trigger->is_triggered();
    const int progress = _clock.tick(running);
    if (progress != trap_timing::CycleClock::FROZEN) {
        _apply(progress);
    }
}

void CycleTrap::reset() {
    _apply(_clock.reset());
}

void CycleTrap::_apply(int permille) {
    const bn::fixed ratio = bn::fixed(permille) / 1000;
    pos.x = _origin_x + _dx * ratio;
    pos.y = _origin_y + _dy * ratio;

    // Direct position writes bypass PhysicsBody::move(), so sync the
    // collision shape and sprite manually - same pattern as PathTrap.
    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(pos.x, pos.y);
    }
}

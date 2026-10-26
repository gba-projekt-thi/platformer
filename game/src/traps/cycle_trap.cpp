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
      _timing(timing),
      _start_frame(trap_timing::cycle_start_frame(timing, phase_frames)),
      _frame(_start_frame),
      _trigger(trigger) {
    // Already in the right place for the first frame of its round.
    _apply(trap_timing::cycle_progress(_timing, _frame));
}

void CycleTrap::update() {
    BaseTrap::update();

    if (_trigger != nullptr && !_trigger->is_triggered()) {
        return;
    }

    _apply(trap_timing::cycle_progress(_timing, _frame));
    _frame = trap_timing::next_cycle_frame(_timing, _frame);
}

void CycleTrap::reset() {
    _frame = _start_frame;
    _apply(trap_timing::cycle_progress(_timing, _frame));
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

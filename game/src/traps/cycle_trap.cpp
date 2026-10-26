#include "cycle_trap.h"

CycleTrap::CycleTrap(
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
    bn::fixed t_dx,
    bn::fixed t_dy,
    trap_timing::CycleTiming t_timing,
    unsigned t_phase_frames,
    Trigger* t_trigger)
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
          // CycleTrap uses direct positional movement, not velocity.
          Cfg::Physics::NO_MAX_VELOCITY),
      _origin_x(t_start_x + t_offset_x),
      _origin_y(t_start_y + t_offset_y),
      _dx(t_dx),
      _dy(t_dy),
      _timing(t_timing),
      _start_frame(trap_timing::cycle_start_frame(t_timing, t_phase_frames)),
      _frame(_start_frame),
      _trigger(t_trigger) {
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

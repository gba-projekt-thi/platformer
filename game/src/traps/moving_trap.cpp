#include "moving_trap.h"

MovingTrap::MovingTrap(
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
    bn::fixed t_x_accel,
    bn::fixed t_y_accel,
    bn::fixed t_max_vel,
    Trigger& t_trigger)
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
          t_max_vel),
      _x_accel(t_x_accel),
      _y_accel(t_y_accel),
      _start_x(t_start_x),
      _start_y(t_start_y),
      _trigger(t_trigger) {}

void MovingTrap::update() {
    BaseTrap::update();
    if (_trigger.is_triggered()) {
        inc_velocity(_x_accel, _y_accel);
    }
}

void MovingTrap::reset() {
    _trigger.reset();
    set_velocity(0, 0);
    pos.x = _start_x;
    pos.y = _start_y;
}
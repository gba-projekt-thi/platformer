
#include "moving_trap.h"

MovingTrap::MovingTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite,
    uint16_t t_block,
    bn::fixed t_x_accel,
    bn::fixed t_y_accel,
    bn::fixed t_max_vel,
    bn::fixed t_range,
    Trigger& t_trigger)
    : BaseTrap(t_start_x, t_start_y, t_width, t_height, t_sprite, t_block),
      x_accel(t_x_accel),
      y_accel(t_y_accel),
      max_vel(t_max_vel),
      range(t_range),
      start_x(t_start_x),
      start_y(t_start_y),
      trigger(t_trigger) {}

void MovingTrap::update() {
    BaseTrap::update();
    if (trigger.is_triggered() &&
        // Check max speed
        (bn::abs(vel_x) + bn::abs(vel_y)) < max_vel) {
        inc_velocity(x_accel, y_accel);
    }
}

void MovingTrap::reset() {
    trigger.reset();
    set_velocity(0, 0);
    x = start_x;
    y = start_y;
}

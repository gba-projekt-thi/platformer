#include "moving_trap.h"

MovingTrap::MovingTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite,
    int t_sprite_waits,
    const bn::span<const uint16_t> t_graphics_indexes,
    uint16_t t_block,
    bn::fixed t_x_accel,
    bn::fixed t_y_accel,
    bn::fixed t_max_vel,
    bn::fixed t_range,
    Trigger& t_trigger)
    : BaseTrap(
          t_start_x,
          t_start_y,
          t_width,
          t_height,
          t_sprite,
          t_sprite_waits,
          t_graphics_indexes,
          t_block,
          t_max_vel),
      x_accel(t_x_accel),
      y_accel(t_y_accel),
      range(t_range),
      start_x(t_start_x),
      start_y(t_start_y),
      trigger(t_trigger) {}

void MovingTrap::update() {
    BaseTrap::update();

    // When its trigger is activated, accelerate the trap
    if (trigger.is_triggered()) {
        inc_velocity(x_accel, y_accel);
    }
}

void MovingTrap::reset() {
    // Reset the trap back to its starting position and stop movement.
    trigger.reset();
    set_velocity(0, 0);
    pos.x = start_x;
    pos.y = start_y;
}

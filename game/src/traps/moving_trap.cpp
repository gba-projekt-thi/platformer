#include "moving_trap.h"

MovingTrap::MovingTrap(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    const bn::sprite_item& sprite,
    int sprite_waits,
    const bn::span<const uint16_t> graphics_indexes,
    uint16_t block,
    bn::fixed x_accel_val,
    bn::fixed y_accel_val,
    bn::fixed max_vel,
    bn::fixed range_val,
    Trigger& trigger_ref_arg)
    : BaseTrap(
          start_x,
          start_y,
          width,
          height,
          sprite,
          sprite_waits,
          graphics_indexes,
          block,
          max_vel),
      x_accel(x_accel_val),
      y_accel(y_accel_val),
      range(range_val),
      start_x(start_x),
      start_y(start_y),
      trigger_ref(trigger_ref_arg) {}

void MovingTrap::update() {
    BaseTrap::update();

    // When its trigger is activated, accelerate the trap
    if (trigger_ref.is_triggered()) {
        inc_velocity(x_accel, y_accel);
    }
}

void MovingTrap::reset() {
    // Reset the trap back to its starting position and stop movement.
    trigger_ref.reset();
    set_velocity(0, 0);
    pos.x = start_x;
    pos.y = start_y;
}
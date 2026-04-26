#include "moving_trap.h"
#include "bn_math.h"

MovingTrap::MovingTrap(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    bn::sprite_item sprite,
    uint16_t block,
    bn::fixed x_accel,
    bn::fixed y_accel,
    bn::fixed max_vel,
    Trigger& trigger)
    : BaseTrap(start_x, start_y, width, height, sprite, block),
      x_accel(x_accel),
      y_accel(y_accel),
      max_vel(max_vel),
      trigger(trigger) {}

void MovingTrap::update() {
    BaseTrap::update();
    if (trigger.is_triggered() &&
        // Check max speed
        (bn::abs(vel_x) + bn::abs(vel_y)) < max_vel) {
        inc_velocity(x_accel, y_accel);
    }
}

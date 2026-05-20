#include "moving_trap.h"

MovingTrap::MovingTrap(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    const bn::sprite_item& sprite,
    int sprite_waits,
    bn::span<const uint16_t> animation_frames,
    uint16_t block,
    bn::fixed x_accel,
    bn::fixed y_accel,
    bn::fixed max_vel,
    Trigger& trigger)
    : BaseTrap(
          start_x,
          start_y,
          width,
          height,
          sprite,
          sprite_waits,
          animation_frames,
          block,
          max_vel),
      x_accel(x_accel),
      y_accel(y_accel),
      start_x(start_x),
      start_y(start_y),
      trigger(trigger) {}

void MovingTrap::update() {
    BaseTrap::update();

    // Only move once activated.
    if (trigger.is_triggered()) {
        inc_velocity(x_accel, y_accel);
    }
}

void MovingTrap::reset() {
    trigger.reset();

    // Stop all movement.
    set_velocity(0, 0);

    // Restore original position.
    pos.x = start_x;
    pos.y = start_y;
}
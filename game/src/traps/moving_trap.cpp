#include "moving_trap.h"

MovingTrap::MovingTrap(
    const TrapData& data,
    bn::fixed x_accel,
    bn::fixed y_accel,
    bn::fixed max_vel,
    Trigger& trigger)
    : BaseTrap(data, max_vel),
      _x_accel(x_accel),
      _y_accel(y_accel),
      _start_x(data.x),
      _start_y(data.y),
      _trigger(trigger) {}

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
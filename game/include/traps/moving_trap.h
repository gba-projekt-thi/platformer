#pragma once

#include "base_trap.h"
#include "trigger.h"

// ----------------------------------------------------------------------------
// MovingTrap
//
// Trigger-activated trap using acceleration-based movement.
//
// Once triggered:
// - Velocity increases every frame
// - Movement handled by PhysicsBody
//
// Good for:
// - Falling hazards
// - Rising spikes
// - Charging traps
// ----------------------------------------------------------------------------
class MovingTrap : public BaseTrap {
   public:
    // x_accel / y_accel / max_vel already scaled for Hard / Assist Mode.
    MovingTrap(
        const TrapData& data,
        bn::fixed x_accel,
        bn::fixed y_accel,
        bn::fixed max_vel,
        Trigger& trigger);

    void update() override;

    void reset() override;

   private:
    // Per-frame acceleration.
    bn::fixed _x_accel;
    bn::fixed _y_accel;

    // Reset position.
    bn::fixed _start_x;
    bn::fixed _start_y;

    // Activation trigger.
    Trigger& _trigger;
};
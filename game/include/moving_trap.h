#pragma once

#include "base_trap.h"
#include "trigger.h"

// -----------------------------------------------------------------------------
// MovingTrap
//
// Trap accelerated by a trigger.
//
// Uses velocity/acceleration from PhysicsBody.
// Good for falling spikes, launched hazards, etc.
// -----------------------------------------------------------------------------
class MovingTrap final : public BaseTrap {
   public:
    MovingTrap(
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
        Trigger& trigger);

    void update() override;

    // Resets trap state after player death/retry.
    void reset();

   private:
    bn::fixed x_accel;
    bn::fixed y_accel;

    bn::fixed start_x;
    bn::fixed start_y;

    Trigger& trigger;
};
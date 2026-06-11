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
    MovingTrap(
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
        Trigger& t_trigger);

    void update() override;

    void reset() override;

   private:
    // Per-frame acceleration.
    bn::fixed x_accel;
    bn::fixed y_accel;

    // Reset position.
    bn::fixed start_x;
    bn::fixed start_y;

    // Activation trigger.
    Trigger& trigger;
};
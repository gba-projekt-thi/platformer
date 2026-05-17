#pragma once

#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_vector.h"

#include "base_trap.h"
#include "trigger.h"

class MovingTrap : public BaseTrap {
   public:
    MovingTrap(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed width,
        bn::fixed height,
        const bn::sprite_item& sprite,
        int sprite_waits,
        const bn::span<const uint16_t> graphics_indexes,
        uint16_t block,
        bn::fixed x_accel,
        bn::fixed y_accel,
        bn::fixed max_vel,
        bn::fixed range,
        Trigger& trigger_ref);

    // Update trap movement and animation each frame.
    void update() override;
    // Reset the trap and its trigger when the player dies.
    void reset();

   private:
    bn::fixed x_accel;
    bn::fixed y_accel;
    bn::fixed range;  // unused
    bn::fixed start_x;
    bn::fixed start_y;
    Trigger& trigger_ref;
};
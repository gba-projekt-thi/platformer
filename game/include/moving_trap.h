#pragma once

#include <bn_fixed.h>
#include <bn_math.h>
#include <bn_vector.h>

#include "base_trap.h"
#include "trigger.h"

class MovingTrap : public BaseTrap {
   public:
    MovingTrap(
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
        Trigger& t_trigger);
    // Update trap movement and animation each frame.
    void update() override;
    // Reset the trap and its trigger when the player dies.
    void reset();

   private:
    bn::fixed x_accel;
    bn::fixed y_accel;
    bn::fixed max_vel;
    bn::fixed range;  // unused
    bn::fixed start_x;
    bn::fixed start_y;
    Trigger& trigger;
};
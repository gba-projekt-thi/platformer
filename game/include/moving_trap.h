#pragma once

#include <bn_fixed.h>
#include <bn_vector.h>

#include "base_trap.h"
#include "trigger.h"

class MovingTrap : public BaseTrap {
   public:
    MovingTrap(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed width,
        bn::fixed height,
        bn::sprite_item sprite,
        uint16_t block,
        bn::fixed accel_x,
        bn::fixed y_accel,
        bn::fixed max_vel,
        Trigger& trigger);
    void update() override;

   private:
    bn::fixed x_accel;
    bn::fixed y_accel;
    bn::fixed max_vel;
    Trigger& trigger;
};
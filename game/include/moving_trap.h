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
        bn::fixed t_width,
        bn::fixed t_height,
        const bn::sprite_item& t_sprite,
        uint16_t t_block,
        bn::fixed t_x_accel,
        bn::fixed t_y_accel,
        bn::fixed t_max_vel,
        bn::fixed t_range,
        Trigger* t_trigger);

    void update() override;

   private:
    bn::fixed x_accel;
    bn::fixed y_accel;
    bn::fixed max_vel;
    bn::fixed range;
    bn::fixed start_x;
    bn::fixed start_y;

    Trigger* trigger;
};
#pragma once

#include "base_trap.h"
#include "bn_algorithm.h"
#include "trigger.h"

class PathTrap : public BaseTrap {
   public:
    PathTrap(
        bn::fixed t_start_x,
        bn::fixed t_start_y,
        bn::fixed t_width,
        bn::fixed t_height,
        const bn::sprite_item& t_sprite,
        int t_sprite_waits,
        const bn::span<const uint16_t> t_graphics_indexes,
        uint16_t t_block,
        bn::span<const bn::fixed_point> t_path,
        unsigned int t_path_waits,
        Trigger& t_trigger);


    // Update trap movement and animation each frame.
    void update() override;
    // Reset the trap and its trigger when the player dies.
    void reset();

   private:
    bn::fixed start_x;
    bn::fixed start_y;
    Trigger& trigger;
    bn::span<const bn::fixed_point> path;
    unsigned int path_waits;
    unsigned int current_frame;
    unsigned int current_index;
};

#pragma once

#include "base_trap.h"
#include "bn_algorithm.h"
#include "trigger.h"

class PathTrap : public BaseTrap {
   public:
    PathTrap(
        bn::fixed trap_start_x,
        bn::fixed trap_start_y,
        bn::fixed width,
        bn::fixed height,
        const bn::sprite_item& sprite_item,
        int sprite_waits,
        const bn::span<const uint16_t> trap_graphics_indexes,
        uint16_t trap_block,
        bn::span<const bn::fixed_point> path_points,
        unsigned int path_waits_val,
        Trigger& trigger_ref);

    // Update trap movement and animation each frame.
    void update() override;
    // Reset the trap and its trigger when the player dies.
    void reset();

   private:
    bn::fixed start_x;
    bn::fixed start_y;
    Trigger& trigger_ref;
    bn::span<const bn::fixed_point> path_points;
    unsigned int path_waits;
    unsigned int current_frame;
    unsigned int current_index;
};
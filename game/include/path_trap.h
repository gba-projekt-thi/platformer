#pragma once

#include "base_trap.h"
#include "trigger.h"

// ----------------------------------------------------------------------------
// PathTrap
//
// Trap moving along predefined points.
//
// Uses interpolation between path nodes.
//
// Good for:
// - Patrol hazards
// - Rotating movement
// - Figure-8 motion
// - Floating enemies
// ----------------------------------------------------------------------------
class PathTrap : public BaseTrap {
   public:
    PathTrap(
        bn::fixed t_start_x,
        bn::fixed t_start_y,
        bn::fixed t_width,
        bn::fixed t_height,
        const bn::sprite_item& t_sprite_item,
        int t_sprite_waits,
        bn::span<const uint16_t> t_graphics_indexes,
        uint16_t t_blocking_layers,
        bn::span<const bn::fixed_point> t_path,
        unsigned t_path_waits,
        Trigger& t_trigger);

    void update() override;

    void reset() override;

   private:
    // Base world position.
    bn::fixed start_x;
    bn::fixed start_y;

    // Activation trigger.
    Trigger& trigger;

    // Relative movement path.
    bn::span<const bn::fixed_point> path;

    // Frames between nodes.
    unsigned path_waits = 1;

    // Current interpolation frame.
    unsigned current_frame = 0;

    // Current path node.
    unsigned current_index = 0;
};
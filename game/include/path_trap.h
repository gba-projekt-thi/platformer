#pragma once

#include "base_trap.h"
#include "trigger.h"

// -----------------------------------------------------------------------------
// PathTrap
//
// Moves along a looping path using linear interpolation.
//
// Optimized for GBA:
// - Avoids division in update()
// - Uses lightweight fixed-point interpolation
// - Avoids dynamic allocation
// -----------------------------------------------------------------------------
class PathTrap final : public BaseTrap {
   public:
    PathTrap(
        bn::fixed start_x,
        bn::fixed start_y,
        bn::fixed width,
        bn::fixed height,
        const bn::sprite_item& sprite,
        int sprite_waits,
        bn::span<const uint16_t> animation_frames,
        uint16_t block,
        bn::span<const bn::fixed_point> path,
        unsigned path_waits,
        Trigger& trigger);

    void update() override;

    void reset();

   private:
    bn::fixed start_x;
    bn::fixed start_y;

    Trigger& trigger;

    bn::span<const bn::fixed_point> path;

    // Cached reciprocal to avoid division every frame.
    bn::fixed inverse_path_waits;

    unsigned current_frame = 0;
    unsigned current_index = 0;

    unsigned path_waits = 1;
};
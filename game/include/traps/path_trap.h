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
    // path_waits already scaled for Hard / Assist Mode.
    PathTrap(const TrapData& data, unsigned path_waits, Trigger& trigger);

    void update() override;

    void reset() override;

   private:
    // Base world position.
    bn::fixed _start_x;
    bn::fixed _start_y;

    // Activation trigger.
    Trigger& _trigger;

    // Relative movement path.
    bn::span<const bn::fixed_point> _path;

    // Frames between nodes.
    unsigned _path_waits = 1;

    // Current interpolation frame.
    unsigned _current_frame = 0;

    // Current path node.
    unsigned _current_index = 0;
};
#pragma once

#include "ambush_logic.h"
#include "base_trap.h"
#include "player.h"

// ----------------------------------------------------------------------------
// AmbushTrap
//
// A hazard that stays motionless (and can still animate, like a disguised
// mimic) until the player comes within a configured horizontal range, then
// lunges once in a fixed direction for a fixed duration, and finally eases
// back to its start position before re-arming - sideways, or (with
// ambush_vertical) up or down: arms or spikes shooting out of the ground the
// moment the duck is above them.
//
// Unlike ChaserTrap it does not track the player continuously and does not
// only move right - it triggers off proximity and moves in whatever
// direction its data specifies. Unlike MovingTrap/PathTrap it needs no
// Trigger; activation is purely distance-based, read directly from the
// player each frame (same pattern as ChaserTrap).
//
// Movement bypasses physics/collision (like ChaserTrap/PathTrap) since the
// lunge should not stop at platform edges.
// ----------------------------------------------------------------------------
class AmbushTrap : public BaseTrap {
   public:
    // speed already scaled for Hard / Assist Mode.
    AmbushTrap(const TrapData& data, bn::fixed speed, Player& player);

    void update() override;

    void reset() override;

   private:
    void _place();

    // Reset position (the hitbox's, like pos).
    bn::fixed _start_x;
    bn::fixed _start_y;
    // Lunges along y (TrapData::ambush_vertical) instead of x.
    bool _vertical;
    // Wait, lunge, return (ambush_logic.h); the trigger is the horizontal
    // distance to the duck either way.
    ambush::Motion<bn::fixed> _motion;

    Player& _player;
};

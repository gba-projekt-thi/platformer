#pragma once

#include "base_trap.h"
#include "player.h"

// ----------------------------------------------------------------------------
// ChaserTrap
//
// A flying hazard that trails some distance behind (to the left of) the
// player and only ever moves rightward to close in when the player moves
// further right - it never backs off, so standing still or moving left
// just widens the gap instead of the trap retreating.
//
// Movement bypasses physics/collision (like PathTrap) since it's meant to
// fly freely regardless of platform layout.
// ----------------------------------------------------------------------------
class ChaserTrap : public BaseTrap {
   public:
    // chase_speed already scaled for Hard / Assist Mode.
    ChaserTrap(const TrapData& data, bn::fixed chase_speed, Player& player);

    void update() override;

    void reset() override;

   private:
    // Reset position.
    bn::fixed _start_x;
    bn::fixed _start_y;

    // How far behind (to the left of) the player this trap eases toward.
    bn::fixed _follow_distance;

    // Max horizontal distance closed per frame while chasing.
    bn::fixed _chase_speed;

    Player& _player;
};
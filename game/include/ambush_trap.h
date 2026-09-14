#pragma once

#include "base_trap.h"
#include "player.h"

// ----------------------------------------------------------------------------
// AmbushTrap
//
// A hazard that stays motionless (and can still animate, like a disguised
// mimic) until the player comes within a configured horizontal range, then
// lunges once in a fixed direction for a fixed duration, and finally eases
// back to its start position before re-arming.
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
    AmbushTrap(
        bn::fixed t_start_x,
        bn::fixed t_start_y,
        bn::fixed t_width,
        bn::fixed t_height,
        bn::fixed t_offset_x,
        bn::fixed t_offset_y,
        const bn::sprite_item& t_sprite_item,
        int t_sprite_waits,
        bn::span<const uint16_t> t_graphics_indexes,
        uint16_t t_blocking_layers,
        bn::fixed t_range,
        bn::fixed t_speed,
        unsigned t_duration,
        Player& t_player);

    void update() override;

    void reset() override;

   private:
    enum class State { Dormant, Lunging, Returning };

    // Reset position.
    bn::fixed _start_x;
    bn::fixed _start_y;

    // Horizontal proximity that triggers the lunge.
    bn::fixed _range;

    // Per-frame step while lunging/returning. Sign gives lunge direction.
    bn::fixed _speed;

    // Frames spent lunging before returning.
    unsigned _duration;
    unsigned _elapsed_frames = 0;

    State _state = State::Dormant;

    Player& _player;
};
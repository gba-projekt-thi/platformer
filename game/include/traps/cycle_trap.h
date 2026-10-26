#pragma once

#include "base_trap.h"
#include "trap_timing.h"
#include "trigger.h"

// ----------------------------------------------------------------------------
// CycleTrap
//
// A trap that moves out from its start position by a fixed offset, stays out,
// moves back and rests, over and over on its own clock - the classic spikes
// that rise out of the floor. The timeline is trap_timing::CycleTiming; this
// class only turns it into a position.
//
// With a trigger it only cycles while the trigger is on (a Timer trigger makes
// it start on its own after a while, a lever freezes it); without one it
// cycles from the start of the level. A trap that is frozen stays where it is.
//
// Like PathTrap/ChaserTrap it writes its position directly instead of using
// PhysicsBody::move(), so it syncs shape_pos and the sprite itself.
// ----------------------------------------------------------------------------
class CycleTrap : public BaseTrap {
   public:
    // `t_trigger` may be nullptr (always running).
    CycleTrap(
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
        bn::fixed t_dx,
        bn::fixed t_dy,
        trap_timing::CycleTiming t_timing,
        unsigned t_phase_frames,
        Trigger* t_trigger);

    void update() override;

    void reset() override;

   private:
    // Puts the trap `permille` / 1000 of the way out.
    void _apply(int permille);

    // Resting position of the collision box (start + offset).
    bn::fixed _origin_x;
    bn::fixed _origin_y;

    // Offset at full extension.
    bn::fixed _dx;
    bn::fixed _dy;

    trap_timing::CycleTiming _timing;

    // Frame of the round the trap starts (and restarts after a death) in.
    unsigned _start_frame;
    unsigned _frame;

    // Optional activation trigger (nullptr = always on).
    Trigger* _trigger;
};

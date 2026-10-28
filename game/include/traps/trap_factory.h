#pragma once

#include "ambush_trap.h"
#include "base_trap.h"
#include "bn_unique_ptr.h"
#include "boss_trap.h"
#include "chaser_trap.h"
#include "cycle_trap.h"
#include "level_manager.h"
#include "level_structure.h"
#include "moving_trap.h"
#include "path_trap.h"
#include "trap_timing.h"

namespace TrapFactory {

namespace detail {

// Picks the concrete trap class of a TrapData and scales its speed values.
inline auto create_trap(const TrapData& trap_data, LevelManager& level_manager)
    -> bn::unique_ptr<BaseTrap> {
    switch (trap_data.type) {
        case TrapType::BASE:
            return bn::make_unique<BaseTrap>(trap_data);

        case TrapType::MOVING: {
            const bn::fixed mult = level_manager.trap_speed_multiplier();
            return bn::make_unique<MovingTrap>(
                trap_data, trap_data.velocity_x * mult,
                trap_data.velocity_y * mult, trap_data.max_vel * mult,
                level_manager.get_trigger_by_name(trap_data.trigger_name));
        }

        case TrapType::PATH: {
            // A path trap's speed is a frame count per node (path_waits), not
            // a velocity: Hard / Assist Mode divide it by the multiplier
            // instead of multiplying it - see trap_timing::scaled_frames().
            const int speed_percent = trap_timing::speed_percent(
                level_manager.trap_speed_multiplier());
            return bn::make_unique<PathTrap>(
                trap_data,
                trap_timing::scaled_frames(trap_data.path_waits, speed_percent),
                level_manager.get_trigger_by_name(trap_data.trigger_name));
        }

        case TrapType::CHASE:
            return bn::make_unique<ChaserTrap>(
                trap_data,
                trap_data.chase_speed * level_manager.trap_speed_multiplier(),
                level_manager.player());

        case TrapType::AMBUSH:
            return bn::make_unique<AmbushTrap>(
                trap_data,
                trap_data.ambush_speed * level_manager.trap_speed_multiplier(),
                level_manager.player());

        case TrapType::CYCLE: {
            // Optional trigger: without one the trap cycles from level start.
            Trigger* trigger = trap_data.trigger_name == nullptr
                                   ? nullptr
                                   : &level_manager.get_trigger_by_name(
                                         trap_data.trigger_name);
            // Every phase of the round (and the start offset) is a frame
            // count, so Hard / Assist Mode scale them like path_waits.
            const int speed_percent = trap_timing::speed_percent(
                level_manager.trap_speed_multiplier());
            const trap_timing::CycleSetup setup = trap_timing::cycle_setup(
                {trap_data.cycle_extend_frames, trap_data.cycle_hold_frames,
                 trap_data.cycle_retract_frames, trap_data.cycle_wait_frames},
                trap_data.cycle_phase_frames, speed_percent);
            BN_ASSERT(
                trap_timing::cycle_length(setup.timing) > 0,
                "CYCLE trap needs a round of at least one frame");
            return bn::make_unique<CycleTrap>(
                trap_data, setup.timing, setup.phase, trigger);
        }

        case TrapType::BOSS: {
            // Like PATH: a frame count per segment, scaled by the modes.
            const int speed_percent = trap_timing::speed_percent(
                level_manager.trap_speed_multiplier());
            return bn::make_unique<BossTrap>(
                trap_data,
                trap_timing::scaled_frames(trap_data.path_waits, speed_percent),
                level_manager);
        }

        default:
            BN_ERROR(
                "Unimplemented or invalid trap type passed to TrapFactory");
            return bn::make_unique<BaseTrap>(trap_data);
    }
}

}  // namespace detail

// Factory function that translates TrapData into a concrete Trap
// Defined in header to allow inlining
inline auto create(const TrapData& trap_data, LevelManager& level_manager)
    -> bn::unique_ptr<BaseTrap> {
    bn::unique_ptr<BaseTrap> trap =
        detail::create_trap(trap_data, level_manager);
    // Common to every trap type: where the sprite is drawn relative to the
    // duck (a fake door stands behind it like the real one).
    if (trap_data.sprite_z_order != 0) {
        trap->set_sprite_z_order(trap_data.sprite_z_order);
    }
    return trap;
}
}  // namespace TrapFactory

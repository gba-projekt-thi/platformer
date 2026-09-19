#pragma once

#include "ambush_trap.h"
#include "base_trap.h"
#include "bn_unique_ptr.h"
#include "chaser_trap.h"
#include "level_manager.h"
#include "level_structure.h"
#include "moving_trap.h"
#include "path_trap.h"

namespace TrapFactory {

// Factory function that translates TrapData into a concrete Trap
// Defined in header to allow inlining
inline auto create(const TrapData& trap_data, LevelManager& level_manager)
    -> bn::unique_ptr<BaseTrap> {
    switch (trap_data.type) {
        case TrapType::BASE:
            return bn::make_unique<BaseTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0);

        case TrapType::MOVING: {
            Trigger& trigger =
                level_manager.get_trigger_by_name(trap_data.trigger_name);
            const bn::fixed mult = level_manager.hard_mode_multiplier();
            return bn::make_unique<MovingTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0,
                trap_data.velocity_x * mult, trap_data.velocity_y * mult,
                trap_data.max_vel * mult, trigger);
        }

        case TrapType::PATH: {
            // Speed not scaled by Hard Mode here - see
            // Cfg::HardMode::SPEED_MULTIPLIER's comment
            Trigger& trigger =
                level_manager.get_trigger_by_name(trap_data.trigger_name);
            return bn::make_unique<PathTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0,
                trap_data.path, trap_data.path_waits, trigger);
        }

        case TrapType::CHASE:
            return bn::make_unique<ChaserTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0,
                trap_data.chase_follow_distance,
                trap_data.chase_speed * level_manager.hard_mode_multiplier(),
                level_manager.player());

        case TrapType::AMBUSH:
            return bn::make_unique<AmbushTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0,
                trap_data.ambush_range,
                trap_data.ambush_speed * level_manager.hard_mode_multiplier(),
                trap_data.ambush_duration, level_manager.player());

        default:
            BN_ERROR(
                "Unimplemented or invalid trap type passed to TrapFactory");
            return bn::make_unique<BaseTrap>(
                0, 0, 0, 0, 0, 0, trap_data.sprite, trap_data.sprite_waits,
                trap_data.graphic_indexes, 0);
    }
}
}  // namespace TrapFactory
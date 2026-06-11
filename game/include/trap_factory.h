#pragma once

#include "base_trap.h"
#include "bn_unique_ptr.h"
#include "level_structure.h"
#include "moving_trap.h"
#include "path_trap.h"

class LevelManager;

namespace TrapFactory {
// Factory function that translates TrapData into a concrete Trap
// Defiend in header to allow inlining
inline bn::unique_ptr<BaseTrap> create(
    const TrapData& trap_data,
    LevelManager& level_manager) {
    switch (trap_data.type) {
        case TrapType::BASE:
            return bn::make_unique<BaseTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0);

        case TrapType::MOVING: {
            Trigger& trigger =
                level_manager.get_trigger(trap_data.trigger_index);
            return bn::make_unique<MovingTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0,
                trap_data.velocity_x, trap_data.velocity_y, trap_data.max_vel,
                trigger);
        }

        case TrapType::PATH: {
            Trigger& trigger =
                level_manager.get_trigger(trap_data.trigger_index);
            return bn::make_unique<PathTrap>(
                trap_data.x, trap_data.y, trap_data.width, trap_data.height,
                trap_data.offset_x, trap_data.offset_y, trap_data.sprite,
                trap_data.sprite_waits, trap_data.graphic_indexes, 0,
                trap_data.path, trap_data.path_waits, trigger);
        }

        default:
            BN_ERROR(
                "Unimplemented or invalid trap type passed to TrapFactory");
            // fallback
            return bn::make_unique<BaseTrap>(
                0, 0, 0, 0, 0, 0, trap_data.sprite, trap_data.sprite_waits,
                trap_data.graphic_indexes, 0);
    }
}
}  // namespace TrapFactory

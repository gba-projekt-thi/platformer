#pragma once

#include "level_common.h"

constexpr bn::fixed_point LEVEL3_BRANCH_PATH[] = {
    bn::fixed_point(0, 0),    bn::fixed_point(-24, 0),
    bn::fixed_point(-48, -8), bn::fixed_point(-24, -16),
    bn::fixed_point(0, -8),   bn::fixed_point(24, -16),
    bn::fixed_point(48, -8),  bn::fixed_point(24, 0)};

constexpr PlatformData level3_platforms[] = {
    // Start area
    {-96, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {-80, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1},
    {-66, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},
    // First jump section
    {-24, 48, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {8, 32, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {64, 48, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {80, 48, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1},
    {96, 48, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},
    // Upper fog climb
    {40, 16, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-16, 0, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-64, -8, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-96, -24, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-48, -40, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {8, -32, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    // Final section
    {56, -48, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {72, -48, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1},
    {88, -48, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},
};

constexpr TriggerData level3_triggers[] = {{40, 8, 32, 24}};

constexpr TrapData level3_traps[] = {
    {TrapType::BASE, 0, 48, 32, 32, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::MOVING, 56, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_3, 0, 0,
     -1.8, 3.3, NO_PATH_TRAP, 0},

    {TrapType::PATH, -16, -24, 32, 16, 0, 0, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, LEVEL3_BRANCH_PATH, 20},

    {TrapType::BASE, 80, 48, 32, 32, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_3 = {
    level3_platforms,
    sizeof(level3_platforms) / sizeof(level3_platforms[0]),

    level3_triggers,
    sizeof(level3_triggers) / sizeof(level3_triggers[0]),

    level3_traps,
    sizeof(level3_traps) / sizeof(level3_traps[0]),

    bn::regular_bg_items::level3,
    bn::music_items::world3,
    {-96, 40},
    {72, -64}};

#pragma once

#include "level_structure.h"

// -----------------------------------------------------------------------------
// Animation Frame Sequences
// -----------------------------------------------------------------------------

constexpr uint16_t TRAP_GRAPHICS_INDEXES_3[] = {0, 1, 2};

constexpr uint16_t TRAP_GRAPHICS_INDEXES_8[] = {0, 1, 2, 3, 4, 5, 6, 7};

// Empty animation sequence.
constexpr bn::span<const uint16_t> TRAP_GRAPHICS_INDEXES_0(nullptr, 0);

// -----------------------------------------------------------------------------
// Empty Path
// -----------------------------------------------------------------------------

constexpr bn::span<const bn::fixed_point> NO_PATH_TRAP(nullptr, 0);

// -----------------------------------------------------------------------------
// Shared Paths
// -----------------------------------------------------------------------------

constexpr bn::fixed_point FIGURE_8_PATH[] = {

    bn::fixed_point(0, 0),     bn::fixed_point(38, 21),
    bn::fixed_point(71, 30),   bn::fixed_point(92, 21),
    bn::fixed_point(100, 0),   bn::fixed_point(92, -21),
    bn::fixed_point(71, -30),  bn::fixed_point(38, -21),

    bn::fixed_point(0, 0),     bn::fixed_point(-38, 21),
    bn::fixed_point(-71, 30),  bn::fixed_point(-92, 21),
    bn::fixed_point(-100, 0),  bn::fixed_point(-92, -21),
    bn::fixed_point(-71, -30), bn::fixed_point(-38, -21)};

// =============================================================================
// LEVEL 0
// =============================================================================

constexpr PlatformData level0_platforms[] = {

    {-96, 60, bn::sprite_items::platformslvl1, 0},
    {-80, 60, bn::sprite_items::platformslvl1, 1},
    {-48, 60, bn::sprite_items::platformslvl1, 2},
    {-16, 60, bn::sprite_items::platformslvl1, 3},

    {0, 40, bn::sprite_items::platformslvl1, 0}};

constexpr TriggerData level0_triggers[] = {

    {-50, 30, 30, 30}};

constexpr TrapData level0_traps[] = {

    // Moving bubble trap.
    {TrapType::MOVING,

     10, 10, 32, 32,

     bn::sprite_items::bubbles, 26, TRAP_GRAPHICS_INDEXES_3,

     0,

     -0.1, 0.1, 1,

     NO_PATH_TRAP, 0},

    // Static bubble trap.
    {TrapType::BASE,

     -20, -20, 32, 32,

     bn::sprite_items::bubbles, 26, TRAP_GRAPHICS_INDEXES_3,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0},

    // Figure-8 path trap.
    {TrapType::PATH,

     0, 0, 32, 32,

     bn::sprite_items::bubbles, 10, TRAP_GRAPHICS_INDEXES_3,

     0,

     0, 0, 0,

     FIGURE_8_PATH, 15}};

const LevelData LEVEL_0 = {

    level0_platforms,
    sizeof(level0_platforms) / sizeof(level0_platforms[0]),

    level0_triggers,
    sizeof(level0_triggers) / sizeof(level0_triggers[0]),

    level0_traps,
    sizeof(level0_traps) / sizeof(level0_traps[0]),

    bn::regular_bg_items::level1,
    bn::music_items::level1,

    {-96, 0},

    {32, 0}};

// =============================================================================
// LEVEL 1
// =============================================================================

constexpr PlatformData level1_platforms[] = {

    {-96, 60, bn::sprite_items::platformslvl1, 0},
    {-80, 60, bn::sprite_items::platformslvl1, 1},
    {-48, 60, bn::sprite_items::platformslvl1, 3},

    {-16, 60, bn::sprite_items::platformslvl1, 0},
    {-16, 44, bn::sprite_items::platformslvl1, 2},

    {32, 60, bn::sprite_items::platformslvl1, 1},
    {48, 60, bn::sprite_items::platformslvl1, 1},

    {64, 60, bn::sprite_items::platformslvl1, 0},
    {80, 60, bn::sprite_items::platformslvl1, 1},
    {96, 60, bn::sprite_items::platformslvl1, 0},
    {112, 60, bn::sprite_items::platformslvl1, 2}};

constexpr TriggerData level1_triggers[] = {

    {80, 30, 32, 30}};

constexpr TrapData level1_traps[] = {

    // Falling bubble trap.
    {TrapType::MOVING,

     80, -32, 32, 32,

     bn::sprite_items::bubbles, 26, TRAP_GRAPHICS_INDEXES_3,

     0,

     0.0, 4, 4,

     NO_PATH_TRAP, 0},

    {TrapType::BASE,

     -48, 8, 32, 32,

     bn::sprite_items::bubbles, 26, TRAP_GRAPHICS_INDEXES_3,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0},

    {TrapType::BASE,

     -64, 60, 16, 16,

     bn::sprite_items::connector16x16, 16, TRAP_GRAPHICS_INDEXES_8,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0},

    {TrapType::BASE,

     -32, 60, 16, 16,

     bn::sprite_items::connector16x16, 16, TRAP_GRAPHICS_INDEXES_8,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0}};

const LevelData LEVEL_1 = {

    level1_platforms,
    sizeof(level1_platforms) / sizeof(level1_platforms[0]),

    level1_triggers,
    sizeof(level1_triggers) / sizeof(level1_triggers[0]),

    level1_traps,
    sizeof(level1_traps) / sizeof(level1_traps[0]),

    bn::regular_bg_items::level1,
    bn::music_items::level1,

    {-96, 0},

    {112, 40}};

// =============================================================================
// LEVEL 2
// =============================================================================

constexpr PlatformData level2_platforms[] = {

    {-96, -16, bn::sprite_items::platformslvl2, 1},
    {-72, 40, bn::sprite_items::platformslvl2, 2},
    {-24, 40, bn::sprite_items::platformslvl2, 0},
    {24, 40, bn::sprite_items::platformslvl2, 0},
    {64, 24, bn::sprite_items::platformslvl2, 1},
    {96, 16, bn::sprite_items::platformslvl2, 1},
    {112, 16, bn::sprite_items::platformslvl2, 1}};

constexpr TriggerData level2_triggers[] = {

    {-48, 16, 32, 40}};

constexpr TrapData level2_traps[] = {

    {TrapType::BASE,

     -112, -16, 16, 16,

     bn::sprite_items::pipe16x16, 26, TRAP_GRAPHICS_INDEXES_0,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0},

    {TrapType::MOVING,

     -48, 88, 32, 32,

     bn::sprite_items::can32x32, 26, TRAP_GRAPHICS_INDEXES_3,

     0,

     0.0, -3, 3,

     NO_PATH_TRAP, 0},

    {TrapType::BASE,

     0, 40, 32, 16,

     bn::sprite_items::rustynails32x16, 26, TRAP_GRAPHICS_INDEXES_3,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0},

    {TrapType::BASE,

     80, 24, 16, 16,

     bn::sprite_items::pipe16x16, 26, TRAP_GRAPHICS_INDEXES_0,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0}};

const LevelData LEVEL_2 = {

    level2_platforms,
    sizeof(level2_platforms) / sizeof(level2_platforms[0]),

    level2_triggers,
    sizeof(level2_triggers) / sizeof(level2_triggers[0]),

    level2_traps,
    sizeof(level2_traps) / sizeof(level2_traps[0]),

    bn::regular_bg_items::level2,
    bn::music_items::level2,

    {-96, -32},

    {104, 0}};

// =============================================================================
// LEVEL 3
// =============================================================================

constexpr bn::fixed_point LEVEL3_BRANCH_PATH[] = {

    bn::fixed_point(0, 0),    bn::fixed_point(-24, 0),
    bn::fixed_point(-48, -8), bn::fixed_point(-24, -16),

    bn::fixed_point(0, -8),   bn::fixed_point(24, -16),
    bn::fixed_point(48, -8),  bn::fixed_point(24, 0)};

constexpr PlatformData level3_platforms[] = {

    {-96, 64, bn::sprite_items::baumstamm, 0},
    {-80, 64, bn::sprite_items::baumstamm, 1},
    {-66, 64, bn::sprite_items::baumstamm, 2},

    {-24, 48, bn::sprite_items::nebel, 0},
    {8, 32, bn::sprite_items::nebel, 0},

    {64, 48, bn::sprite_items::baumstamm, 0},
    {80, 48, bn::sprite_items::baumstamm, 1},
    {96, 48, bn::sprite_items::baumstamm, 2},

    {40, 16, bn::sprite_items::nebel, 0},
    {-16, 0, bn::sprite_items::nebel, 0},
    {-64, -8, bn::sprite_items::nebel, 0},
    {-96, -24, bn::sprite_items::nebel, 0},

    {-48, -40, bn::sprite_items::nebel, 0},
    {8, -32, bn::sprite_items::nebel, 0},

    {56, -48, bn::sprite_items::baumstamm, 0},
    {72, -48, bn::sprite_items::baumstamm, 1},
    {88, -48, bn::sprite_items::baumstamm, 2},
};

constexpr TriggerData level3_triggers[] = {

    {40, 8, 32, 24}};

constexpr TrapData level3_traps[] = {

    {TrapType::BASE,

     0, 48, 32, 32,

     bn::sprite_items::mushroom32x32, 16, TRAP_GRAPHICS_INDEXES_0,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0},

    {TrapType::MOVING,

     56, 96, 32, 32,

     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_3,

     0,

     0, -1.8, 3.3,

     NO_PATH_TRAP, 0},

    {TrapType::PATH,

     -16, -24, 32, 16,

     bn::sprite_items::branch32x16, 10, TRAP_GRAPHICS_INDEXES_0,

     0,

     0, 0, 0,

     LEVEL3_BRANCH_PATH, 20},

    {TrapType::BASE,

     80, 48, 32, 32,

     bn::sprite_items::mushroom32x32, 16, TRAP_GRAPHICS_INDEXES_0,

     -1,

     0, 0, 0,

     NO_PATH_TRAP, 0}};

const LevelData LEVEL_3 = {

    level3_platforms,
    sizeof(level3_platforms) / sizeof(level3_platforms[0]),

    level3_triggers,
    sizeof(level3_triggers) / sizeof(level3_triggers[0]),

    level3_traps,
    sizeof(level3_traps) / sizeof(level3_traps[0]),

    bn::regular_bg_items::level3,
    bn::music_items::level2,

    {-96, 40},

    {72, -64}};
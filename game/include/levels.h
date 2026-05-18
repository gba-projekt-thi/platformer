#pragma once
#include "level_structure.h"

// Animation order for the different sprites, add here as needed
constexpr uint16_t TRAP_GRAPHICS_INDEXES_3[] = {0, 1, 2};
constexpr uint16_t TRAP_GRAPHICS_INDEXES_8[] = {0, 1, 2, 3, 4, 5, 6, 7};
constexpr bn::span<const uint16_t> TRAP_GRAPHICS_INDEXES_0(nullptr, 0);
constexpr bn::span<bn::fixed_point> NO_PATH_TRAP(nullptr, 0);

constexpr bn::fixed_point FIGURE_8_PATH[] = {
    bn::fixed_point(0, 0),     bn::fixed_point(38, 21),
    bn::fixed_point(71, 30),   bn::fixed_point(92, 21),
    bn::fixed_point(100, 0),   bn::fixed_point(92, -21),
    bn::fixed_point(71, -30),  bn::fixed_point(38, -21),
    bn::fixed_point(0, 0),     bn::fixed_point(-38, 21),
    bn::fixed_point(-71, 30),  bn::fixed_point(-92, 21),
    bn::fixed_point(-100, 0),  bn::fixed_point(-92, -21),
    bn::fixed_point(-71, -30), bn::fixed_point(-38, -21)};

constexpr PlatformData level0_platforms[] = {
    {-96, 60, bn::sprite_items::platformslvl1, 0},
    {-80, 60, bn::sprite_items::platformslvl1, 1},
    {-48, 60, bn::sprite_items::platformslvl1, 2},
    {-16, 60, bn::sprite_items::platformslvl1, 3},
    {0, 40, bn::sprite_items::platformslvl1, 0}};

constexpr TriggerData level0_triggers[] = {{-50, 30, 30, 30}};

constexpr TrapData level0_traps[] = {
    {TrapType::Moving, 10, 10, 32, 32, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, -0.1, 0.1, 1, 5, 0, NO_PATH_TRAP, 1},
    {TrapType::Base, -20, -20, 32, 32, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1},
    {TrapType::Path, 0, 0, 32, 32, bn::sprite_items::bubbles, 10,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, 0, 0, FIGURE_8_PATH, 10}};

const LevelData LEVEL_0 = {
    level0_platforms,
    sizeof(level0_platforms) / sizeof(PlatformData),
    level0_triggers,
    1,
    level0_traps,
    3,
    bn::regular_bg_items::level1,
    bn::music_items::level1,
    {-96, 0},
    {32, 0}};

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

constexpr TriggerData level1_triggers[] = {{80, 30, 32, 30}};

constexpr TrapData level1_traps[] = {
    {TrapType::Moving, 80, -32, 32, 32, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, 4, 4, 5, 0, NO_PATH_TRAP, 1},
    {TrapType::Base, -48, 8, 32, 32, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1},
    {TrapType::Base, -64, 60, 16, 16, bn::sprite_items::connector16x16, 16,
     TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1},
    {TrapType::Base, -32, 60, 16, 16, bn::sprite_items::connector16x16, 16,
     TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1}};

const LevelData LEVEL_1 = {
    level1_platforms,
    sizeof(level1_platforms) / sizeof(PlatformData),
    level1_triggers,
    1,
    level1_traps,
    4,
    bn::regular_bg_items::level1,
    bn::music_items::level1,
    {-96, 0},
    {112, 40}};

constexpr PlatformData level2_platforms[] = {
    {-96, -16, bn::sprite_items::platformslvl2, 1},
    {-72, 40, bn::sprite_items::platformslvl2, 2},
    {-24, 40, bn::sprite_items::platformslvl2, 0},
    {24, 40, bn::sprite_items::platformslvl2, 0},
    {64, 24, bn::sprite_items::platformslvl2, 1},
    {96, 16, bn::sprite_items::platformslvl2, 1},
    {112, 16, bn::sprite_items::platformslvl2, 1}};

constexpr TrapData level2_traps[] = {
    {TrapType::Base, -112, -16, 16, 16, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1},
    {TrapType::Moving, -48, 88, 32, 32, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, -3, 3, 0, 0, NO_PATH_TRAP, 1},
    {TrapType::Base, 0, 40, 32, 16, bn::sprite_items::rustynails32x16, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1},
    {TrapType::Base, 80, 24, 16, 16, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1}};

constexpr TriggerData level2_triggers[] = {{-48, 16, 32, 40}};

const LevelData LEVEL_2 = {
    level2_platforms,
    sizeof(level2_platforms) / sizeof(PlatformData),
    level2_triggers,
    1,
    level2_traps,
    4,
    bn::regular_bg_items::level2,
    bn::music_items::level2,
    {-96, -32},
    {104, 0}};

constexpr bn::fixed_point LEVEL3_BRANCH_PATH[] = {
    // Horizontal patrol near upper climb
    bn::fixed_point(0, 0),    bn::fixed_point(-24, 0),
    bn::fixed_point(-48, -8), bn::fixed_point(-24, -16),
    bn::fixed_point(0, -8),   bn::fixed_point(24, -16),
    bn::fixed_point(48, -8),  bn::fixed_point(24, 0)};

constexpr PlatformData level3_platforms[] = {
    // Start area
    {-96, 64, bn::sprite_items::baumstamm, 0},
    {-80, 64, bn::sprite_items::baumstamm, 1},
    {-66, 64, bn::sprite_items::baumstamm, 2},

    // First jump section
    {-24, 48, bn::sprite_items::nebel, 0},
    {8, 32, bn::sprite_items::nebel, 0},

    {64, 48, bn::sprite_items::baumstamm, 0},
    {80, 48, bn::sprite_items::baumstamm, 1},
    {96, 48, bn::sprite_items::baumstamm, 2},

    // Upper fog climb
    {40, 16, bn::sprite_items::nebel, 0},
    {-16, 0, bn::sprite_items::nebel, 0},
    {-64, -8, bn::sprite_items::nebel, 0},
    {-96, -24, bn::sprite_items::nebel, 0},

    {-48, -40, bn::sprite_items::nebel, 0},
    {8, -32, bn::sprite_items::nebel, 0},

    // Final section
    {56, -48, bn::sprite_items::baumstamm, 0},
    {72, -48, bn::sprite_items::baumstamm, 1},
    {88, -48, bn::sprite_items::baumstamm, 2}};

constexpr TriggerData level3_triggers[] = {
    // Trigger activated during upper fog climb
    {40, 8, 32, 24}};

constexpr TrapData level3_traps[] = {
    // Early mushroom forcing careful jump
    {TrapType::Base, 0, 48, 32, 32, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1},

    // Slither arms rise upward after upper trigger
    {TrapType::Moving, 56, 96, 32, 32, bn::sprite_items::slithermanarms32x32, 8,
     TRAP_GRAPHICS_INDEXES_3, 0, -1.8, 3.3, 0, 0, NO_PATH_TRAP, 1},

    // Branch patrol trap activated during upper climb
    {TrapType::Path, -16, -24, 32, 16, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, 0, LEVEL3_BRANCH_PATH, 20},

    // Final mushroom guarding exit
    {TrapType::Base, 80, 48, 32, 32, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, -1, NO_PATH_TRAP, 1}};

const LevelData LEVEL_3 = {
    level3_platforms,
    sizeof(level3_platforms) / sizeof(PlatformData),
    level3_triggers,
    2,
    level3_traps,
    4,
    bn::regular_bg_items::level3,
    bn::music_items::level2,
    // Player spawn
    {-96, 40},
    // Door position
    {72, -64}};
#pragma once

#include "bn_music_items.h"
#include "bn_regular_bg_items_world1.h"
#include "bn_sprite_items_bubbles.h"
#include "bn_sprite_items_connector16x16.h"
#include "bn_sprite_items_kugelfisch16x16.h"
#include "bn_sprite_items_platforms_world1.h"

#include "level_structure.h"
#include "levels_common.h"

// =============================================================================
// LEVEL 0 (World 1) - unchanged
// =============================================================================

constexpr PlatformData level0_platforms[] = {

    {-96, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {-80, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-48, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},
    {-16, 60, 16, 14, 0, 0, bn::sprite_items::platforms_world1, 3},

    {0, 40, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0}};

// Named so both traps below can bind to it explicitly instead of via a
// raw array index - see level0_traps' comments.
constexpr TriggerData level0_triggers[] = {
    {-50, 30, 30, 30, false, "level0_gate"}};

constexpr TrapData level0_traps[] = {

    // Moving bubble trap. Shares the "level0_gate" trigger with the
    // figure-8 path trap below - both activate together when the player
    // enters level0_triggers[0]'s area (trigger-chain demo: one trigger,
    // multiple traps, bound by name instead of a shared raw index).
    {TrapType::MOVING, 10, 10, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, -0.1, 0.1, 1, NO_PATH_TRAP, 0, 0, 0,
     "level0_gate"},

    // Static bubble trap.
    {TrapType::BASE, -20, -20, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, NO_PATH_TRAP, 0},

    // Figure-8 path trap. Shares "level0_gate" with the moving bubble
    // trap above - see its comment.
    {TrapType::PATH, 0, 0, 32, 32, 0, 0, bn::sprite_items::bubbles, 10,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, FIGURE_8_PATH, 15, 0, 0, "level0_gate"}};

const LevelData LEVEL_0 = {

    level0_platforms,
    sizeof(level0_platforms) / sizeof(level0_platforms[0]),

    level0_triggers,
    sizeof(level0_triggers) / sizeof(level0_triggers[0]),

    level0_traps,
    sizeof(level0_traps) / sizeof(level0_traps[0]),

    bn::regular_bg_items::world1,
    bn::music_items::world1,
    // Player spawn
    {-96, 0},
    // Door position
    {32, 0}};

// =============================================================================
// LEVEL 1 (World 1) - unchanged
// =============================================================================

constexpr PlatformData level1_platforms[] = {

    {-96, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {-80, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-48, 60, 16, 14, 0, 0, bn::sprite_items::platforms_world1, 3},

    {-16, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {-16, 44, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},

    {32, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {48, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},

    {64, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {80, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {96, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {112, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2}};

constexpr TriggerData level1_triggers[] = {
    {80, 40, 32, 30, false, "fall_trigger"}};

constexpr TrapData level1_traps[] = {

    // Falling bubble trap.
    {TrapType::MOVING, 80, -32, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "fall_trigger"},

    {TrapType::BASE, -48, 0, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -64, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -32, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_1 = {

    level1_platforms,
    sizeof(level1_platforms) / sizeof(level1_platforms[0]),

    level1_triggers,
    sizeof(level1_triggers) / sizeof(level1_triggers[0]),

    level1_traps,
    sizeof(level1_traps) / sizeof(level1_traps[0]),

    bn::regular_bg_items::world1,
    bn::music_items::world1,
    // Player spawn
    {-96, 0},
    // Door position
    {112, 40}};

// =============================================================================
// WORLD 1 SCROLL - new, uses the scrolling camera (world_width=384)
// =============================================================================

constexpr PlatformData world1_scroll_platforms[] = {

    {-176, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {-160, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-144, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-128, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-112, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},

    {-80, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {-48, 44, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},

    {-8, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {8, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {24, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {40, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},

    {80, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {96, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {112, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},
    {128, 44, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {144, 44, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {160, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},
    {176, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0}};

constexpr TriggerData world1_scroll_triggers[] = {
    {-50, 30, 30, 30, false, "release1"},
    {130, 30, 30, 30, false, "release2"}};

constexpr TrapData world1_scroll_traps[] = {

    // Falling bubble, triggered by "release1".
    {TrapType::MOVING, 80, -32, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "release1"},

    {TrapType::BASE, -48, 0, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -64, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -32, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, NO_PATH_TRAP, 0},

    // Second falling bubble in the extended section, triggered by
    // "release2".
    {TrapType::MOVING, 160, -32, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "release2"}};

const LevelData LEVEL_WORLD1_SCROLL = {

    world1_scroll_platforms,
    sizeof(world1_scroll_platforms) / sizeof(world1_scroll_platforms[0]),

    world1_scroll_triggers,
    sizeof(world1_scroll_triggers) / sizeof(world1_scroll_triggers[0]),

    world1_scroll_traps,
    sizeof(world1_scroll_traps) / sizeof(world1_scroll_traps[0]),

    bn::regular_bg_items::world1,
    bn::music_items::world1,
    // Player spawn
    {-176, 0},
    // Door position
    {176, 40},
    // World size (enables scrolling)
    384,
    160};

// =============================================================================
// WORLD 1 BOSS - "The Kugelfisch King" (world_width=480)
// A Hollow-Knight-style dodge gauntlet: a patrolling bubble sweep overlays
// the whole arena, four trigger-gated "attacks" fire in sequence as the
// duck advances, and a disguised pufferfish punishes lingering too long
// near the midpoint. Reaching the door on the right is "winning" the fight.
// =============================================================================

constexpr PlatformData world1_boss_platforms[] = {

    {-224, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {-192, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-160, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-128, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},
    {-88, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {-56, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {-24, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {8, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},
    {48, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {80, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {112, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {144, 60, 16, 10, 0, 1, bn::sprite_items::platforms_world1, 2},
    {176, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0},
    {208, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 1},
    {220, 60, 16, 8, 0, 0, bn::sprite_items::platforms_world1, 0}};

constexpr TriggerData world1_boss_triggers[] = {
    {-160, 40, 32, 40, false, "wave1"},
    {-50, 40, 32, 40, false, "wave2"},
    {50, 40, 32, 40, false, "wave3"},
    {150, 40, 32, 40, false, "wave4"}};

constexpr TrapData world1_boss_traps[] = {

    // Idle patrol - the boss's passive figure-8 drift, active the whole
    // fight (Hollow Knight bosses rarely stand still between attacks).
    // Shares "wave1" with Attack 1 below - starts once the fight begins.
    {TrapType::PATH, -180, -10, 32, 32, 0, 0, bn::sprite_items::bubbles, 30,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, FIGURE_8_PATH, 15, 0, 0, "wave1"},

    // Attack 1: telegraphed overhead slam, drops once the duck enters wave1.
    {TrapType::MOVING, -150, -40, 32, 32, 0, 0, bn::sprite_items::bubbles, 40,
     TRAP_GRAPHICS_INDEXES_3, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "wave1"},

    // Attack 2: charging pufferfish dash, triggered by wave2.
    {TrapType::MOVING, 200, 50, 24, 16, 0, 0, bn::sprite_items::kugelfisch16x16,
     30, TRAP_GRAPHICS_INDEXES_8, -2.2, 0, 3, NO_PATH_TRAP, 0, 0, 0, "wave2"},

    // Mid-arena ambush: a smaller pufferfish disguised as scenery, lunges
    // if the duck lingers nearby - punishes hesitation, not advancing.
    {TrapType::AMBUSH,
     40,
     60,
     16,
     16,
     0,
     0,
     bn::sprite_items::kugelfisch16x16,
     10,
     TRAP_GRAPHICS_INDEXES_8,
     0,
     0,
     0,
     NO_PATH_TRAP,
     0,
     0,
     0,
     nullptr,
     40,
     1.5,
     20},

    // Attack 3: second overhead slam, triggered by wave3.
    {TrapType::MOVING, 120, -40, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "wave3"},

    // Attack 4: final charge before the door, triggered by wave4.
    {TrapType::MOVING, 260, 50, 24, 16, 0, 0, bn::sprite_items::kugelfisch16x16,
     1, TRAP_GRAPHICS_INDEXES_8, -2.5, 0, 3, NO_PATH_TRAP, 0, 0, 0, "wave4"}};

const LevelData LEVEL_WORLD1_BOSS = {

    world1_boss_platforms,
    sizeof(world1_boss_platforms) / sizeof(world1_boss_platforms[0]),

    world1_boss_triggers,
    sizeof(world1_boss_triggers) / sizeof(world1_boss_triggers[0]),

    world1_boss_traps,
    sizeof(world1_boss_traps) / sizeof(world1_boss_traps[0]),

    bn::regular_bg_items::world1,
    bn::music_items::world1,
    // Player spawn
    {-224, 30},
    // Door position
    {228, 40},
    // World size (enables scrolling)
    480,
    160};
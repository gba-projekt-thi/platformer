#pragma once

#include "bn_music_items.h"
#include "bn_regular_bg_items_world2.h"
#include "bn_sprite_items_box32x32.h"
#include "bn_sprite_items_can32x32.h"
#include "bn_sprite_items_pipe16x16.h"
#include "bn_sprite_items_platforms_world2.h"
#include "bn_sprite_items_rustynails32x16.h"

#include "level_structure.h"
#include "levels_common.h"

// =============================================================================
// LEVEL 2 (World 2) - unchanged
// =============================================================================

constexpr PlatformData level2_platforms[] = {

    {-96, -16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-72, 40, 16, 18, 0, 0, bn::sprite_items::platforms_world2, 2},
    {-24, 40, 16, 11, 0, 0, bn::sprite_items::platforms_world2, 0},
    {24, 40, 16, 11, 0, 0, bn::sprite_items::platforms_world2, 0},
    {64, 24, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {96, 16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {112, 16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1}};

constexpr TriggerData level2_triggers[] = {
    {-48, 16, 32, 40, false, "can_release"}};

constexpr TrapData level2_traps[] = {

    {TrapType::BASE, -112, -16, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::MOVING, -48, 88, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, -3, 3, NO_PATH_TRAP, 0, 0, 0, "can_release"},

    {TrapType::BASE, 0, 40, 28, 16, 0, 0, bn::sprite_items::rustynails32x16, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, 80, 24, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_2 = {

    level2_platforms,
    sizeof(level2_platforms) / sizeof(level2_platforms[0]),

    level2_triggers,
    sizeof(level2_triggers) / sizeof(level2_triggers[0]),

    level2_traps,
    sizeof(level2_traps) / sizeof(level2_traps[0]),

    bn::regular_bg_items::world2,
    bn::music_items::world2,
    // Player spawn
    {-96, -32},
    // Door position
    {104, 0}};

// =============================================================================
// WORLD 2 SCROLL - new, uses the scrolling camera (world_width=432)
// =============================================================================

constexpr PlatformData world2_scroll_platforms[] = {

    {-208, -16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-184, -16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-160, -16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-136, -16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},

    {-96, -16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-72, 40, 16, 18, 0, 0, bn::sprite_items::platforms_world2, 2},
    {-24, 40, 16, 11, 0, 0, bn::sprite_items::platforms_world2, 0},
    {24, 40, 16, 11, 0, 0, bn::sprite_items::platforms_world2, 0},
    {64, 24, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {96, 16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {112, 16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},

    {144, 16, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {176, 0, 16, 8, 0, 0, bn::sprite_items::platforms_world2, 0},
    {200, 0, 16, 8, 0, 0, bn::sprite_items::platforms_world2, 0}};

constexpr TriggerData world2_scroll_triggers[] = {
    {-48, 16, 32, 40, false, "can_release1"},
    {160, 8, 32, 24, false, "can_release2"}};

constexpr TrapData world2_scroll_traps[] = {

    {TrapType::BASE, -224, -16, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::MOVING, -48, 88, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, -3, 3, NO_PATH_TRAP, 0, 0, 0,
     "can_release1"},

    {TrapType::BASE, 0, 40, 28, 16, 0, 0, bn::sprite_items::rustynails32x16, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, 80, 24, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Same can, same trigger pattern as can_release1 above, retuned 20%
    // faster (3 -> 3.6px/frame) now that the player has already met the
    // slower version once in this level - reuses the existing sprite and
    // MOVING behavior for a felt escalation from pure data, no new art.
    {TrapType::MOVING, 160, -32, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 0.0, -3.6, 3.6, NO_PATH_TRAP, 0, 0, 0,
     "can_release2"}};

const LevelData LEVEL_WORLD2_SCROLL = {

    world2_scroll_platforms,
    sizeof(world2_scroll_platforms) / sizeof(world2_scroll_platforms[0]),

    world2_scroll_triggers,
    sizeof(world2_scroll_triggers) / sizeof(world2_scroll_triggers[0]),

    world2_scroll_traps,
    sizeof(world2_scroll_traps) / sizeof(world2_scroll_traps[0]),

    bn::regular_bg_items::world2,
    bn::music_items::world2,
    // Player spawn
    {-200, -32},
    // Door position
    {200, -16},
    // World size (enables scrolling)
    432,
    160};

// =============================================================================
// WORLD 2 BOSS - "The Rusty Behemoth" (world_width=480)
// =============================================================================

constexpr PlatformData world2_boss_platforms[] = {

    {-224, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-192, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-160, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-128, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-96, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-64, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {-32, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {0, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {32, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {64, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {96, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {128, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {160, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {192, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1},
    {224, 60, 16, 6, 0, 0, bn::sprite_items::platforms_world2, 1}};

constexpr TriggerData world2_boss_triggers[] = {
    {-150, 40, 32, 40, false, "wave1"},
    {-50, 40, 32, 40, false, "wave2"},
    {50, 40, 32, 40, false, "wave3"},
    {150, 40, 32, 40, false, "wave4"}};

constexpr TrapData world2_boss_traps[] = {

    // Idle patrol: a rusty pipe swept back and forth across the corridor.
    // Shares "wave1" with Attack 1 below - starts once the fight begins.
    {TrapType::PATH, -180, 20, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, LEVEL3_BRANCH_PATH, 15, 0, 0, "wave1"},

    // Attack 1: a can slams down from above, triggered by wave1.
    {TrapType::MOVING, -145, -40, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_8, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "wave1"},

    // Attack 2: a second can charges in sideways, triggered by wave2.
    {TrapType::MOVING, 200, 64, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_8, -2.0, 0, 3, NO_PATH_TRAP, 0, 0, 0, "wave2"},

    // A crate disguised as scenery - punishes lingering mid-arena.
    {TrapType::AMBUSH,
     55,
     65,
     20,
     20,
     0,
     0,
     bn::sprite_items::box32x32,
     1,
     TRAP_GRAPHICS_INDEXES_8,
     0,
     0,
     0,
     NO_PATH_TRAP,
     0,
     0,
     0,
     nullptr,
     20,
     2.0,
     18},

    // Attack 3: second slam, triggered by wave3.
    {TrapType::MOVING, 100, -40, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_8, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "wave3"},

    // A row of rusty nails, always dangerous - forces a precise landing
    // right before the final attack.
    {TrapType::BASE, 180, 65, 28, 16, 0, 0, bn::sprite_items::rustynails32x16,
     26, TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, NO_PATH_TRAP, 0},

    // Attack 4: final charge before the door, triggered by wave4.
    {TrapType::MOVING, 260, 60, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_8, -2.4, 0, 3.4, NO_PATH_TRAP, 0, 0, 0, "wave4"}};

const LevelData LEVEL_WORLD2_BOSS = {

    world2_boss_platforms,
    sizeof(world2_boss_platforms) / sizeof(world2_boss_platforms[0]),

    world2_boss_triggers,
    sizeof(world2_boss_triggers) / sizeof(world2_boss_triggers[0]),

    world2_boss_traps,
    sizeof(world2_boss_traps) / sizeof(world2_boss_traps[0]),

    bn::regular_bg_items::world2,
    bn::music_items::world2,
    // Player spawn
    {-224, 30},
    // Door position
    {228, 40},
    // World size (enables scrolling)
    480,
    160};
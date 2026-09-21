#pragma once

#include "bn_music_items.h"
#include "bn_regular_bg_items_world3.h"
#include "bn_sprite_items_baumstamm.h"
#include "bn_sprite_items_branch32x16.h"
#include "bn_sprite_items_mushroom32x32.h"
#include "bn_sprite_items_nebel.h"
#include "bn_sprite_items_slithermanarms32x32.h"

#include "level_structure.h"
#include "levels_common.h"

// =============================================================================
// LEVEL 3 (World 3) - unchanged
// =============================================================================

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

constexpr TriggerData level3_triggers[] = {
    // Trigger activated during upper fog climb
    {40, 8, 32, 24, false, "climb_trigger"}};

constexpr TrapData level3_traps[] = {
    // Early mushroom forcing careful jump
    {TrapType::BASE, 0, 48, 32, 32, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Slither arms rise upward after upper trigger. Shares
    // "climb_trigger" with the branch patrol below (trigger-chain: one
    // trigger, two traps).
    {TrapType::MOVING, 56, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_3, 0, -1.8,
     3.3, NO_PATH_TRAP, 0, 0, 0, "climb_trigger"},

    // Branch patrol trap activated during upper climb. Shares
    // "climb_trigger" with the slither arms above.
    {TrapType::PATH, -16, -24, 32, 16, 0, 0, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, LEVEL3_BRANCH_PATH, 20, 0, 0,
     "climb_trigger"},

    // Final mushroom guarding exit
    {TrapType::BASE, 80, 48, 32, 32, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_3 = {

    level3_platforms,
    sizeof(level3_platforms) / sizeof(level3_platforms[0]),

    level3_triggers,
    sizeof(level3_triggers) / sizeof(level3_triggers[0]),

    level3_traps,
    sizeof(level3_traps) / sizeof(level3_traps[0]),

    bn::regular_bg_items::world3,
    bn::music_items::world3,
    // Player spawn
    {-96, 40},
    // Door position
    {72, -64},
    // World size (single-screen; spelled out so ground_friction below can
    // be set - the same 240x160 default every non-scrolling level uses)
    240,
    160,
    // Ground friction: mossy forest floor, noticeably slippery to stop on
    0.55};

// =============================================================================
// WORLD 3 SCROLL - new, uses the scrolling camera (world_width=480)
// =============================================================================

constexpr PlatformData world3_scroll_platforms[] = {

    {-224, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {-208, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1},
    {-192, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},

    {-160, 48, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-112, 32, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-64, 42, 32, 8, 0, 0, bn::sprite_items::nebel, 0},

    {-16, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {0, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1},
    {16, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},

    {48, 48, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {96, 32, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {144, 16, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {192, 0, 32, 8, 0, 0, bn::sprite_items::nebel, 0},

    {224, 0, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0}};

constexpr TriggerData world3_scroll_triggers[] = {
    {0, 40, 32, 24, false, "mid_trigger"}};

constexpr TrapData world3_scroll_traps[] = {

    {TrapType::BASE, -36, 48, 16, 16, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Slither arms rise upward once "mid_trigger" fires. Shares it with
    // the patrolling branch below (trigger-chain: one trigger, two traps).
    {TrapType::MOVING, 24, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_3, 0, -1.8,
     3.3, NO_PATH_TRAP, 0, 0, 0, "mid_trigger"},

    // Patrolling branch hazard over the second climb. Shares
    // "mid_trigger" with the slither arms above.
    {TrapType::PATH, 96, 32, 32, 16, 0, 0, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, LEVEL3_BRANCH_PATH, 20, 0, 0,
     "mid_trigger"},

    // Second branch, one step higher up the same staircase, sweeping the
    // identical loop exactly out of phase with the one above (same
    // path_waits, same trigger, LEVEL3_BRANCH_PATH_PHASE2 instead of
    // LEVEL3_BRANCH_PATH) - two branches to read at once instead of one,
    // periodically crossing rather than moving together.
    {TrapType::PATH, 144, 16, 32, 16, 0, 0, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, LEVEL3_BRANCH_PATH_PHASE2, 20, 0, 0,
     "mid_trigger"},

    {TrapType::BASE, 168, 6, 12, 16, 0, 0, bn::sprite_items::branch32x16, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_WORLD3_SCROLL = {

    world3_scroll_platforms,
    sizeof(world3_scroll_platforms) / sizeof(world3_scroll_platforms[0]),

    world3_scroll_triggers,
    sizeof(world3_scroll_triggers) / sizeof(world3_scroll_triggers[0]),

    world3_scroll_traps,
    sizeof(world3_scroll_traps) / sizeof(world3_scroll_traps[0]),

    bn::regular_bg_items::world3,
    bn::music_items::world3,
    // Player spawn
    {-224, 40},
    // Door position
    {224, -16},
    // World size (enables scrolling)
    480,
    160,
    // Ground friction: mossy forest floor, noticeably slippery to stop on
    0.55};

// =============================================================================
// WORLD 3 BOSS - "Slitherman, Heart of the Forest" (world_width=520)
// =============================================================================

constexpr PlatformData world3_boss_platforms[] = {

    {-240, 60, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {-208, 60, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},
    {-176, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-128, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-80, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-32, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {16, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {64, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {112, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {160, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {208, 60, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {236, 60, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {258, 60, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1}};

constexpr TriggerData world3_boss_triggers[] = {
    {-160, 40, 32, 40, false, "wave1"},
    {-40, 40, 32, 40, false, "wave2"},
    {80, 40, 32, 40, false, "wave3"},
    {200, 40, 32, 40, false, "wave4"}};

constexpr TrapData world3_boss_traps[] = {

    // Idle patrol: a branch sweeping the corridor throughout the fight.
    // Shares "wave1" with Attack 1 below - starts once the fight begins.
    {TrapType::PATH, -190, 0, 32, 16, 0, 0, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, LEVEL3_BRANCH_PATH, 20, 0, 0, "wave1"},

    // Attack 1: an arm rises up out of the mist, triggered by wave1.
    {TrapType::MOVING, -150, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_8, 0, -1.8,
     3.3, NO_PATH_TRAP, 0, 0, 0, "wave1"},

    // A disguised mushroom - punishes lingering near the middle of the
    // arena instead of pressing forward.
    {TrapType::AMBUSH,
     25,
     64,
     24,
     24,
     0,
     0,
     bn::sprite_items::mushroom32x32,
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
     3,
     1.8,
     10},

    // Attack 2: a second arm, further along, triggered by wave2.
    {TrapType::MOVING, -20, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_8, 0, -1.8,
     3.3, NO_PATH_TRAP, 0, 0, 0, "wave2"},

    // Attack 3: a third arm, triggered by wave3.
    {TrapType::MOVING, 100, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_8, 0, -2.0,
     3.6, NO_PATH_TRAP, 0, 0, 0, "wave3"},

    // Attack 4: final arm right before the door, triggered by wave4.
    {TrapType::MOVING, 220, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_8, 0, -2.2,
     3.8, NO_PATH_TRAP, 0, 0, 0, "wave4"}};

const LevelData LEVEL_WORLD3_BOSS = {

    world3_boss_platforms,
    sizeof(world3_boss_platforms) / sizeof(world3_boss_platforms[0]),

    world3_boss_triggers,
    sizeof(world3_boss_triggers) / sizeof(world3_boss_triggers[0]),

    world3_boss_traps,
    sizeof(world3_boss_traps) / sizeof(world3_boss_traps[0]),

    bn::regular_bg_items::world3,
    bn::music_items::world3,
    // Player spawn
    {-240, 30},
    // Door position
    {246, 40},
    // World size (enables scrolling)
    520,
    160};
// Deliberately NOT given the slippery ground_friction the other two
// World 3 levels use (see LEVEL_3 / LEVEL_WORLD3_SCROLL above) - a
// dodge-heavy attack gauntlet is a bad place to also fight the floor,
// so this boss keeps the default (1) traction.
#pragma once

#include "bn_music_items.h"
#include "bn_regular_bg_items_world4.h"
#include "bn_sprite_items_beet.h"
#include "bn_sprite_items_brick16x16.h"
#include "bn_sprite_items_bug16x16.h"
#include "bn_sprite_items_flower1.h"
#include "bn_sprite_items_flower2.h"
#include "bn_sprite_items_flower3.h"
#include "bn_sprite_items_korb.h"
#include "bn_sprite_items_nest32x16.h"
#include "bn_sprite_items_strauch32x32.h"

#include "level_structure.h"
#include "levels_common.h"

// =============================================================================
// WORLD 4 SCROLL - garden theme (world_width=528)
// =============================================================================

constexpr PlatformData world4_scroll_platforms[] = {

    {-248, 60, 32, 16, 0, 0, bn::sprite_items::beet, 0},
    {-192, 60, 16, 10, 0, 0, bn::sprite_items::flower1, 0},
    {-160, 44, 16, 10, 0, 0, bn::sprite_items::flower2, 0},
    {-120, 60, 32, 16, 0, 0, bn::sprite_items::korb, 0},
    {-64, 44, 16, 10, 0, 0, bn::sprite_items::flower3, 0},
    {-32, 28, 16, 10, 0, 0, bn::sprite_items::flower1, 0},
    {8, 44, 32, 16, 0, 0, bn::sprite_items::beet, 0},
    {64, 60, 16, 10, 0, 0, bn::sprite_items::flower2, 0},
    {104, 44, 32, 16, 0, 0, bn::sprite_items::korb, 0},
    {152, 28, 16, 10, 0, 0, bn::sprite_items::flower3, 0},
    {192, 44, 32, 16, 0, 0, bn::sprite_items::beet, 0},
    {240, 60, 16, 10, 0, 0, bn::sprite_items::flower1, 0},
    {264, 60, 32, 16, 0, 0, bn::sprite_items::korb, 0}};

constexpr TriggerData world4_scroll_triggers[] = {
    {8, 44, 32, 16, false, "brick_release"},
    {176, 44, 32, 24, false, "bug_release"}};

constexpr TrapData world4_scroll_traps[] = {

    // Static ground hazard sitting in the gap after the start platform -
    // clear of any platform, forces a full jump over the gap.
    {TrapType::BASE, -216, 60, 24, 16, 0, 0, bn::sprite_items::nest32x16, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Static bush hazard in a later gap.
    {TrapType::BASE, -88, 56, 24, 24, 0, 0, bn::sprite_items::strauch32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Falling brick, released when "brick_release" fires.
    {TrapType::MOVING, 52, 60, 16, 16, 0, 0, bn::sprite_items::brick16x16, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0,
     "brick_release"},

    // Flying pest, drifts right once "bug_release" fires.
    {TrapType::MOVING, 192, 50, 12, 12, 0, 0, bn::sprite_items::bug16x16, 1,
     TRAP_GRAPHICS_INDEXES_0, 1.2, 0, 2, NO_PATH_TRAP, 0, 0, 0, "bug_release"}};

const LevelData LEVEL_WORLD4_SCROLL = {

    world4_scroll_platforms,
    sizeof(world4_scroll_platforms) / sizeof(world4_scroll_platforms[0]),

    world4_scroll_triggers,
    sizeof(world4_scroll_triggers) / sizeof(world4_scroll_triggers[0]),

    world4_scroll_traps,
    sizeof(world4_scroll_traps) / sizeof(world4_scroll_traps[0]),

    bn::regular_bg_items::world4,
    bn::music_items::world4,
    // Player spawn
    {-248, 0},
    // Door position
    {254, 40},
    // World size (enables scrolling)
    528,
    160};

// =============================================================================
// WORLD 4 SCROLL 2 - garden theme, second level (world_width=320)
// Introduces AmbushTrap: a bug lurking in the gap between the korb
// platforms that darts out once the duck gets close. Also gives the
// CHASE mechanic its first appearance outside a boss fight - a second,
// gentler bug trails the duck the whole level - so the Hive Queen's
// relentless pursuit in the World 4 boss isn't the player's first time
// reading a chaser's pressure alongside a lunging ambush.
// =============================================================================

constexpr PlatformData world4b_scroll_platforms[] = {

    {-144, 60, 32, 16, 0, 0, bn::sprite_items::beet, 0},
    {-88, 60, 16, 10, 0, 0, bn::sprite_items::flower1, 0},
    {-48, 44, 16, 10, 0, 0, bn::sprite_items::flower2, 0},
    {0, 60, 32, 16, 0, 0, bn::sprite_items::korb, 0},
    {56, 44, 16, 10, 0, 0, bn::sprite_items::flower3, 0},
    {104, 60, 32, 16, 0, 0, bn::sprite_items::beet, 0},
    {152, 60, 16, 10, 0, 0, bn::sprite_items::flower1, 0}};

constexpr TrapData world4b_scroll_traps[] = {

    // Trailing bug, introducing CHASE outside a boss fight: eases toward
    // 65px behind the duck at a gentle 1.3px/frame - slower and with more
    // slack than the Hive Queen's 50px/1.8px/frame in the World 4 boss.
    // Combined with the ambush bug below, standing still to plan the
    // ambush dodge lets the chaser close the gap - the level's first
    // taste of "pursuer pressure plus dodge timing" at once.
    {TrapType::CHASE, -180, 24, 12, 12, 0, 0, bn::sprite_items::bug16x16, 1,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, NO_PATH_TRAP, 0, 65, 1.3},

    // Static bush hazard right after the first platform.
    {TrapType::BASE, -104, 56, 24, 24, 0, 0, bn::sprite_items::strauch32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Ambush bug lurking near the korb platform - lunges right once the
    // duck comes within 40px, for 20 frames, then retreats and re-arms.
    {TrapType::AMBUSH,
     12,
     50,
     12,
     12,
     0,
     0,
     bn::sprite_items::bug16x16,
     1,
     TRAP_GRAPHICS_INDEXES_3,
     0,
     0,
     0,
     NO_PATH_TRAP,
     0,
     0,
     0,
     nullptr,
     110,
     1.5,
     30}};

const LevelData LEVEL_WORLD4_SCROLL_2 = {

    world4b_scroll_platforms,
    sizeof(world4b_scroll_platforms) / sizeof(world4b_scroll_platforms[0]),

    nullptr,
    0,

    world4b_scroll_traps,
    sizeof(world4b_scroll_traps) / sizeof(world4b_scroll_traps[0]),

    bn::regular_bg_items::world4,
    bn::music_items::world4,
    // Player spawn
    {-144, 0},
    // Door position
    {152, 40},
    // World size (enables scrolling)
    320,
    160};

// =============================================================================
// WORLD 4 BOSS - "The Hive Queen" (world_width=480)
// =============================================================================

constexpr PlatformData world4_boss_platforms[] = {

    {-224, 60, 32, 16, 0, 0, bn::sprite_items::beet, 0},
    {-168, 60, 16, 10, 0, 0, bn::sprite_items::flower1, 0},
    {-128, 44, 16, 10, 0, 0, bn::sprite_items::flower2, 0},
    {-88, 60, 32, 16, 0, 0, bn::sprite_items::korb, 0},
    {-56, 44, 16, 10, 0, 0, bn::sprite_items::flower3, 0},
    {8, 60, 32, 16, 0, 0, bn::sprite_items::beet, 0},
    {64, 44, 16, 10, 0, 0, bn::sprite_items::flower1, 0},
    {104, 60, 32, 16, 0, 0, bn::sprite_items::korb, 0},
    {160, 60, 16, 10, 0, 0, bn::sprite_items::flower2, 0},
    {200, 60, 32, 16, 0, 0, bn::sprite_items::beet, 0}};

constexpr TriggerData world4_boss_triggers[] = {
    {-140, 40, 32, 40, false, "wave1"},
    {-20, 40, 32, 40, false, "wave2"},
    {90, 40, 32, 40, false, "wave3"}};

constexpr TrapData world4_boss_traps[] = {

    // The Hive Queen herself: a bug that trails the duck the whole fight,
    // closing the gap whenever the duck advances - constant pressure, like
    // an enrage timer forcing forward movement.
    {TrapType::CHASE, -260, 20, 12, 12, 0, 0, bn::sprite_items::bug16x16, 1,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, NO_PATH_TRAP, 0, 50, 1.8},

    // A bush disguised as scenery, lunging if the duck lingers near it.
    {TrapType::AMBUSH,
     -100,
     56,
     24,
     24,
     0,
     0,
     bn::sprite_items::strauch32x32,
     16,
     TRAP_GRAPHICS_INDEXES_0,
     0,
     0,
     0,
     NO_PATH_TRAP,
     0,
     0,
     0,
     nullptr,
     30,
     1.6,
     20},

    // Attack 1: a brick dive-bombs down, triggered by wave1.
    {TrapType::MOVING, -140, -90, 16, 16, 0, 0, bn::sprite_items::brick16x16, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 5, 5, NO_PATH_TRAP, 0, 0, 0, "wave1"},

    // Attack 2: a second dive-bomb, triggered by wave2.
    {TrapType::MOVING, -20, -90, 16, 16, 0, 0, bn::sprite_items::brick16x16, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 5, 5, NO_PATH_TRAP, 0, 0, 0, "wave2"},

    // A second, faster worker bug lunges in once the duck nears the door.
    {TrapType::AMBUSH,
     90,
     50,
     12,
     12,
     0,
     0,
     bn::sprite_items::bug16x16,
     1,
     TRAP_GRAPHICS_INDEXES_3,
     0,
     0,
     0,
     NO_PATH_TRAP,
     0,
     0,
     0,
     nullptr,
     32,
     2.2,
     16},

    // Attack 3: final dive-bomb right before the door, triggered by wave3.
    {TrapType::MOVING, 100, -90, 16, 16, 0, 0, bn::sprite_items::brick16x16, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 4.5, NO_PATH_TRAP, 0, 0, 0, "wave3"}};

const LevelData LEVEL_WORLD4_BOSS = {

    world4_boss_platforms,
    sizeof(world4_boss_platforms) / sizeof(world4_boss_platforms[0]),

    world4_boss_triggers,
    sizeof(world4_boss_triggers) / sizeof(world4_boss_triggers[0]),

    world4_boss_traps,
    sizeof(world4_boss_traps) / sizeof(world4_boss_traps[0]),

    bn::regular_bg_items::world4,
    bn::music_items::world4,
    // Player spawn
    {-224, 0},
    // Door position
    {216, 40},
    // World size (enables scrolling)
    480,
    160};
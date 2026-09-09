#pragma once

#include "bn_fixed_point.h"
#include "bn_music_items.h"
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_items_level2.h"
#include "bn_regular_bg_items_level3.h"
#include "bn_regular_bg_items_world4.h"
#include "bn_span.h"
#include "bn_sprite_items_baumstamm.h"
#include "bn_sprite_items_box32x32.h"
#include "bn_sprite_items_branch32x16.h"
#include "bn_sprite_items_brick16x16.h"
#include "bn_sprite_items_bubbles.h"
#include "bn_sprite_items_can32x32.h"
#include "bn_sprite_items_connector16x16.h"
#include "bn_sprite_items_door32x32.h"
#include "bn_sprite_items_kugelfisch.h"
#include "bn_sprite_items_ladder.h"
#include "bn_sprite_items_mimic32x32.h"
#include "bn_sprite_items_mushroom32x32.h"
#include "bn_sprite_items_nebel.h"
#include "bn_sprite_items_pipe16x16.h"
#include "bn_sprite_items_platformslvl1.h"
#include "bn_sprite_items_platformslvl2.h"
#include "bn_sprite_items_rostybar32x32.h"
#include "bn_sprite_items_rustynails32x16.h"
#include "bn_sprite_items_slithermanarms32x32.h"
#include "bn_sprite_items_thwomp32x32.h"

#include "level_structure.h"

// -----------------------------------------------------------------------------
// Level design notes (physics-derived spacing limits)
//
// In this coordinate system, smaller y = higher up (gravity increases y,
// jumping decreases it). Based on Cfg::Player (ACCELERATION=0.3,
// MAX_SPEED=2, JUMP_SPEED=-3, GRAVITY=0.22, MAX_FALL_SPEED=3), a single
// fully-held jump reaches:
//   - max CLIMB (y decrease):        ~22px above the takeoff point
//   - max horizontal travel:         ~50px over the flight time
// Descending (y increase) is effectively unconstrained in height as long
// as the player's absolute y stays below Cfg::Player::DEATH_HEIGHT (100)
// and the horizontal gap stays within the same ~40-50px envelope.
//
// All platform spacing below keeps climbs <=16px and horizontal gaps
// <=40px per jump, matching the proven-safe World 1-3 layouts.
// -----------------------------------------------------------------------------

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

constexpr bn::fixed_point LEVEL3_BRANCH_PATH[] = {

    // Horizontal patrol near upper climb
    bn::fixed_point(0, 0),    bn::fixed_point(-24, 0),
    bn::fixed_point(-48, -8), bn::fixed_point(-24, -16),
    bn::fixed_point(0, -8),   bn::fixed_point(24, -16),
    bn::fixed_point(48, -8),  bn::fixed_point(24, 0)};

// =============================================================================
// LEVEL 0 (World 1) - unchanged
// =============================================================================

constexpr PlatformData level0_platforms[] = {

    {-96, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {-80, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {-48, 60, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2},
    {-16, 60, 16, 14, 0, 0, bn::sprite_items::platformslvl1, 3},

    {0, 40, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0}};

constexpr TriggerData level0_triggers[] = {{-50, 30, 30, 30}};

constexpr TrapData level0_traps[] = {

    // Moving bubble trap.
    {TrapType::MOVING, 10, 10, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, -0.1, 0.1, 1, NO_PATH_TRAP, 0},

    // Static bubble trap.
    {TrapType::BASE, -20, -20, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    // Figure-8 path trap.
    {TrapType::PATH, 0, 0, 32, 32, 0, 0, bn::sprite_items::bubbles, 10,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, 0, FIGURE_8_PATH, 15}};

const LevelData LEVEL_0 = {

    level0_platforms,
    sizeof(level0_platforms) / sizeof(level0_platforms[0]),

    level0_triggers,
    sizeof(level0_triggers) / sizeof(level0_triggers[0]),

    level0_traps,
    sizeof(level0_traps) / sizeof(level0_traps[0]),

    bn::regular_bg_items::level1,
    bn::music_items::level1,
    // Player spawn
    {-96, 0},
    // Door position
    {32, 0}};

// =============================================================================
// LEVEL 1 (World 1) - unchanged
// =============================================================================

constexpr PlatformData level1_platforms[] = {

    {-96, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {-80, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {-48, 60, 16, 14, 0, 0, bn::sprite_items::platformslvl1, 3},

    {-16, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {-16, 44, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2},

    {32, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {48, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},

    {64, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {80, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {96, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {112, 60, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2}};

constexpr TriggerData level1_triggers[] = {{80, 40, 32, 30}};

constexpr TrapData level1_traps[] = {

    // Falling bubble trap.
    {TrapType::MOVING, 80, -32, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0.0, 4, 4, NO_PATH_TRAP, 0},

    {TrapType::BASE, -48, 0, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -64, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -32, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, -1, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_1 = {

    level1_platforms,
    sizeof(level1_platforms) / sizeof(level1_platforms[0]),

    level1_triggers,
    sizeof(level1_triggers) / sizeof(level1_triggers[0]),

    level1_traps,
    sizeof(level1_traps) / sizeof(level1_traps[0]),

    bn::regular_bg_items::level1,
    bn::music_items::level1,
    // Player spawn
    {-96, 0},
    // Door position
    {112, 40}};

// =============================================================================
// LEVEL 2 (World 2) - unchanged
// =============================================================================

constexpr PlatformData level2_platforms[] = {

    {-96, -16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {-72, 40, 16, 18, 0, 0, bn::sprite_items::platformslvl2, 2},
    {-24, 40, 16, 11, 0, 0, bn::sprite_items::platformslvl2, 0},
    {24, 40, 16, 11, 0, 0, bn::sprite_items::platformslvl2, 0},
    {64, 24, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {96, 16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {112, 16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1}};

constexpr TriggerData level2_triggers[] = {{-48, 16, 32, 40}};

constexpr TrapData level2_traps[] = {

    {TrapType::BASE, -112, -16, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::MOVING, -48, 88, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0.0, -3, 3, NO_PATH_TRAP, 0},

    {TrapType::BASE, 0, 40, 28, 16, 0, 0, bn::sprite_items::rustynails32x16, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, 80, 24, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_2 = {

    level2_platforms,
    sizeof(level2_platforms) / sizeof(level2_platforms[0]),

    level2_triggers,
    sizeof(level2_triggers) / sizeof(level2_triggers[0]),

    level2_traps,
    sizeof(level2_traps) / sizeof(level2_traps[0]),

    bn::regular_bg_items::level2,
    bn::music_items::level2,
    // Player spawn
    {-96, -32},
    // Door position
    {104, 0}};

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
    {40, 8, 32, 24}};

constexpr TrapData level3_traps[] = {
    // Early mushroom forcing careful jump
    {TrapType::BASE, 0, 48, 32, 32, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    // Slither arms rise upward after upper trigger
    {TrapType::MOVING, 56, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_3, 0, 0,
     -1.8, 3.3, NO_PATH_TRAP, 0},

    // Branch patrol trap activated during upper climb
    {TrapType::PATH, -16, -24, 32, 16, 0, 0, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, LEVEL3_BRANCH_PATH, 20},

    // Final mushroom guarding exit
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
    // Player spawn
    {-96, 40},
    // Door position
    {72, -64}};

// =============================================================================
// WORLD 1 SCROLL - new, uses the scrolling camera (world_width=384)
// =============================================================================

constexpr PlatformData world1_scroll_platforms[] = {

    {-176, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {-160, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {-144, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {-128, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {-112, 60, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2},

    {-80, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {-48, 44, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2},

    {-8, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {8, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {24, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {40, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},

    {80, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {96, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {112, 60, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2},
    {128, 44, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {144, 44, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {160, 60, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2},
    {176, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0}};

constexpr TriggerData world1_scroll_triggers[] = {
    {-50, 30, 30, 30},
    {130, 30, 30, 30}};

constexpr TrapData world1_scroll_traps[] = {

    // Falling bubble, triggered by trigger 0.
    {TrapType::MOVING, 80, -32, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0.0, 4, 4, NO_PATH_TRAP, 0},

    {TrapType::BASE, -48, 0, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -64, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -32, 60, 16, 16, 0, 0, bn::sprite_items::connector16x16,
     16, TRAP_GRAPHICS_INDEXES_8, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    // Second falling bubble in the extended section, triggered by trigger 1.
    {TrapType::MOVING, 160, -32, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 1, 0.0, 4, 4, NO_PATH_TRAP, 0}};

const LevelData LEVEL_WORLD1_SCROLL = {

    world1_scroll_platforms,
    sizeof(world1_scroll_platforms) / sizeof(world1_scroll_platforms[0]),

    world1_scroll_triggers,
    sizeof(world1_scroll_triggers) / sizeof(world1_scroll_triggers[0]),

    world1_scroll_traps,
    sizeof(world1_scroll_traps) / sizeof(world1_scroll_traps[0]),

    bn::regular_bg_items::level1,
    bn::music_items::level1,
    // Player spawn
    {-176, 0},
    // Door position
    {176, 40},
    // World size (enables scrolling)
    384,
    160};

// =============================================================================
// WORLD 2 SCROLL - new, uses the scrolling camera (world_width=432)
// =============================================================================

constexpr PlatformData world2_scroll_platforms[] = {

    {-208, -16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {-184, -16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {-160, -16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {-136, -16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},

    {-96, -16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {-72, 40, 16, 18, 0, 0, bn::sprite_items::platformslvl2, 2},
    {-24, 40, 16, 11, 0, 0, bn::sprite_items::platformslvl2, 0},
    {24, 40, 16, 11, 0, 0, bn::sprite_items::platformslvl2, 0},
    {64, 24, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {96, 16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {112, 16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},

    {144, 16, 16, 6, 0, 0, bn::sprite_items::platformslvl2, 1},
    {176, 0, 16, 8, 0, 0, bn::sprite_items::platformslvl2, 0},
    {208, 0, 16, 8, 0, 0, bn::sprite_items::platformslvl2, 0}};

constexpr TriggerData world2_scroll_triggers[] = {
    {-48, 16, 32, 40},
    {160, 8, 32, 24}};

constexpr TrapData world2_scroll_traps[] = {

    {TrapType::BASE, -224, -16, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::MOVING, -48, 88, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0.0, -3, 3, NO_PATH_TRAP, 0},

    {TrapType::BASE, 0, 40, 28, 16, 0, 0, bn::sprite_items::rustynails32x16, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, 80, 24, 12, 8, 0, 0, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::MOVING, 160, -32, 28, 28, 0, 0, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 1, 0.0, -3, 3, NO_PATH_TRAP, 0}};

const LevelData LEVEL_WORLD2_SCROLL = {

    world2_scroll_platforms,
    sizeof(world2_scroll_platforms) / sizeof(world2_scroll_platforms[0]),

    world2_scroll_triggers,
    sizeof(world2_scroll_triggers) / sizeof(world2_scroll_triggers[0]),

    world2_scroll_traps,
    sizeof(world2_scroll_traps) / sizeof(world2_scroll_traps[0]),

    bn::regular_bg_items::level2,
    bn::music_items::level2,
    // Player spawn
    {-208, -32},
    // Door position
    {208, -16},
    // World size (enables scrolling)
    432,
    160};

// =============================================================================
// WORLD 3 SCROLL - new, uses the scrolling camera (world_width=480)
// =============================================================================

constexpr PlatformData world3_scroll_platforms[] = {

    {-224, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {-208, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1},
    {-192, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},

    {-160, 48, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-112, 32, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {-64, 48, 32, 8, 0, 0, bn::sprite_items::nebel, 0},

    {-16, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0},
    {0, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 1},
    {16, 64, 16, 8, 0, 0, bn::sprite_items::baumstamm, 2},

    {48, 48, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {96, 32, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {144, 16, 32, 8, 0, 0, bn::sprite_items::nebel, 0},
    {192, 0, 32, 8, 0, 0, bn::sprite_items::nebel, 0},

    {224, 0, 16, 8, 0, 0, bn::sprite_items::baumstamm, 0}};

constexpr TriggerData world3_scroll_triggers[] = {{0, 40, 32, 24}};

constexpr TrapData world3_scroll_traps[] = {

    {TrapType::BASE, -36, 48, 16, 16, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    // Slither arms rise upward once the mid trigger fires.
    {TrapType::MOVING, 0, 96, 32, 32, 0, 0,
     bn::sprite_items::slithermanarms32x32, 8, TRAP_GRAPHICS_INDEXES_3, 0, 0,
     -1.8, 3.3, NO_PATH_TRAP, 0},

    // Patrolling branch hazard over the second climb.
    {TrapType::PATH, 96, 32, 32, 16, 0, 0, bn::sprite_items::branch32x16, 10,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, 0, LEVEL3_BRANCH_PATH, 20},

    {TrapType::BASE, 168, 6, 12, 16, 0, 0, bn::sprite_items::mushroom32x32, 16,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_WORLD3_SCROLL = {

    world3_scroll_platforms,
    sizeof(world3_scroll_platforms) / sizeof(world3_scroll_platforms[0]),

    world3_scroll_triggers,
    sizeof(world3_scroll_triggers) / sizeof(world3_scroll_triggers[0]),

    world3_scroll_traps,
    sizeof(world3_scroll_traps) / sizeof(world3_scroll_traps[0]),

    bn::regular_bg_items::level3,
    bn::music_items::world3,
    // Player spawn
    {-224, 40},
    // Door position
    {224, -16},
    // World size (enables scrolling)
    480,
    160};

// =============================================================================
// WORLD 4 SCROLL - new, first use of the World 4 asset set (world_width=528)
// =============================================================================

constexpr PlatformData world4_scroll_platforms[] = {

    {-248, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 0},
    {-232, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 1},
    {-216, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 1},

    {-176, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 0},
    {-136, 44, 16, 10, 0, 0, bn::sprite_items::brick16x16, 2},
    {-96, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 0},
    {-56, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 1},
    {-16, 44, 16, 10, 0, 0, bn::sprite_items::brick16x16, 2},
    {24, 28, 16, 10, 0, 0, bn::sprite_items::brick16x16, 2},
    {64, 44, 16, 8, 0, 0, bn::sprite_items::brick16x16, 1},
    {104, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 0},
    {144, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 1},
    {184, 44, 16, 10, 0, 0, bn::sprite_items::brick16x16, 2},
    {224, 44, 16, 8, 0, 0, bn::sprite_items::brick16x16, 1},
    {264, 60, 16, 8, 0, 0, bn::sprite_items::brick16x16, 0}};

constexpr TriggerData world4_scroll_triggers[] = {
    {-96, 40, 32, 30},
    {104, 40, 32, 30}};

constexpr TrapData world4_scroll_traps[] = {

    // Falling thwomp, triggered by trigger 0.
    {TrapType::MOVING, -96, -40, 32, 32, 0, 0, bn::sprite_items::thwomp32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0.0, 4, 4, NO_PATH_TRAP, 0},

    {TrapType::BASE, -16, 20, 32, 32, 0, 0, bn::sprite_items::rostybar32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, 64, 20, 32, 32, 0, 0, bn::sprite_items::mimic32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    // Second falling thwomp, triggered by trigger 1.
    {TrapType::MOVING, 104, -40, 32, 32, 0, 0, bn::sprite_items::thwomp32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 1, 0.0, 4, 4, NO_PATH_TRAP, 0},

    {TrapType::BASE, 224, 20, 32, 32, 0, 0, bn::sprite_items::rostybar32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0}};

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
    {-248, 20},
    // Door position
    {264, 20},
    // World size (enables scrolling)
    528,
    160};
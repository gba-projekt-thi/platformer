#pragma once

#include "bn_music_items.h"
#include "bn_regular_bg_items_world5.h"
#include "bn_sprite_items_axe64x32.h"
#include "bn_sprite_items_barrel32x32.h"
#include "bn_sprite_items_bars32x32.h"
#include "bn_sprite_items_chest32x32.h"
#include "bn_sprite_items_mimic32x32.h"
#include "bn_sprite_items_rostybar32x32.h"
#include "bn_sprite_items_susanne16x16.h"
#include "bn_sprite_items_thwomp32x32.h"

#include "level_structure.h"
#include "levels_common.h"

// =============================================================================
// WORLD 5 SCROLL - dungeon theme, hardest/final level (world_width=624)
// All platforms kept at y=60 (flat run) per request; gaps stay <=32px,
// well inside the safe horizontal envelope even without any climb.
// =============================================================================

constexpr PlatformData world5_scroll_platforms[] = {

    {-272, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-216, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {-176, 60, 16, 10, 0, 0, bn::sprite_items::chest32x32, 0},
    {-128, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-72, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {-24, 60, 16, 10, 0, 0, bn::sprite_items::chest32x32, 0},
    {24, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {80, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {128, 60, 32, 16, 0, 0, bn::sprite_items::chest32x32, 0},
    {176, 70, 16, 10, 0, -10, bn::sprite_items::barrel32x32, 0},
    {224, 60, 32, 16, 0, 0, bn::sprite_items::bars32x32, 0},
    {272, 60, 16, 10, 0, 0, bn::sprite_items::chest32x32, 0},
    {296, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0}};

constexpr TriggerData world5_scroll_triggers[] = {
    // Fires just before the thwomp's drop zone near x=156.
    {140, 48, 32, 24, false, "thwomp_gate"}};

constexpr TrapData world5_scroll_traps[] = {

    // Axe chases the player from behind - starts just left of the spawn
    // point and only ever flies right, easing toward a position roughly
    // 40px behind the player at up to 2.5px/frame. Standing still or
    // moving left just widens the gap; running right is what closes it.
    {TrapType::CHASE, -300, 20, 32, 16, 0, 0, bn::sprite_items::axe64x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0, 40, 2.5},

    // Static rusty bar hazard sitting in the first gap.
    {TrapType::BASE, -240, 60, 16, 16, 0, 0, bn::sprite_items::rostybar32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Disguised, animated mimic sitting in a mid gap (mimic32x32 has 8
    // animation frames).
    {TrapType::BASE, 56, 60, 16, 16, 0, 0, bn::sprite_items::mimic32x32, 16,
     TRAP_GRAPHICS_INDEXES_8, 0, 0, 0, NO_PATH_TRAP, 0},

    // Falling thwomp guarding the approach to the final platforms,
    // triggered by "thwomp_gate" above.
    {TrapType::MOVING, 200, -20, 32, 32, 0, 0, bn::sprite_items::thwomp32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 4, NO_PATH_TRAP, 0, 0, 0, "thwomp_gate"}};

const LevelData LEVEL_WORLD5_SCROLL = {

    world5_scroll_platforms,
    sizeof(world5_scroll_platforms) / sizeof(world5_scroll_platforms[0]),

    world5_scroll_triggers,
    sizeof(world5_scroll_triggers) / sizeof(world5_scroll_triggers[0]),

    world5_scroll_traps,
    sizeof(world5_scroll_traps) / sizeof(world5_scroll_traps[0]),

    bn::regular_bg_items::world5,
    bn::music_items::world5,
    // Player spawn
    {-272, 30},
    // Door position
    {296, 40},
    // World size (enables scrolling)
    624,
    160};

// =============================================================================
// WORLD 5 SCROLL 2 - dungeon theme, second level (world_width=380)
// Introduces the AmbushTrap variant of the mimic: previously purely
// decorative animation, it now actually lunges once the duck gets close.
// The axe from the first World 5 level also returns here, tuned a notch
// tighter than there and a notch gentler than the boss - filling in the
// missing middle rung of its own escalation (scroll1 -> scroll2 -> boss)
// and giving CHASE+AMBUSH their first pairing in World 5 outside the
// boss fight.
// =============================================================================

constexpr PlatformData world5b_scroll_platforms[] = {

    {-168, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-112, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {-72, 60, 16, 10, 0, 0, bn::sprite_items::chest32x32, 0},
    {-24, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {32, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {72, 60, 32, 16, 0, 0, bn::sprite_items::chest32x32, 0},
    {128, 70, 16, 10, 0, -10, bn::sprite_items::barrel32x32, 0},
    {176, 60, 32, 16, 0, 0, bn::sprite_items::bars32x32, 0}};

constexpr TrapData world5b_scroll_traps[] = {

    // The axe, back for a second round: 34px follow distance / 2.75px per
    // frame, between LEVEL_WORLD5_SCROLL's introductory 40/2.5 and
    // LEVEL_WORLD5_BOSS's tighter 28/3.0. Same sprite, no new graphics -
    // just closes the gap a little faster and sits a little closer than
    // the first time the duck met it.
    {TrapType::CHASE, -196, 20, 32, 16, 0, 0, bn::sprite_items::axe64x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0, 34, 2.75},

    // Static rusty bar hazard in the first gap.
    {TrapType::BASE, -140, 60, 16, 16, 0, 0, bn::sprite_items::rostybar32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Ambush mimic disguised among the real chests - lunges left/right
    // (negative speed = leftward) once the duck is within 36px, for 15
    // frames, then retreats. Placed just before the last chest so it
    // reads as "which chest is the fake one?".
    {TrapType::AMBUSH,
     48,
     60,
     16,
     16,
     0,
     0,
     bn::sprite_items::mimic32x32,
     16,
     TRAP_GRAPHICS_INDEXES_8,
     0,
     0,
     0,
     NO_PATH_TRAP,
     0,
     0,
     0,
     nullptr,
     36,
     -2.0,
     15}};

const LevelData LEVEL_WORLD5_SCROLL_2 = {

    world5b_scroll_platforms,
    sizeof(world5b_scroll_platforms) / sizeof(world5b_scroll_platforms[0]),

    nullptr,
    0,

    world5b_scroll_traps,
    sizeof(world5b_scroll_traps) / sizeof(world5b_scroll_traps[0]),

    bn::regular_bg_items::world5,
    bn::music_items::world5,
    // Player spawn
    {-168, 30},
    // Door position
    {176, 40},
    // World size (enables scrolling)
    380,
    160};

// =============================================================================
// WORLD 5 BOSS - "The Mimic Lord" (world_width=650)
// The final gauntlet: the axe from World 5's first level returns, closer
// and hungrier, while a trio of thwomps slam down in sequence and two
// mimics lie in wait among the real chests.
// =============================================================================

constexpr PlatformData world5_boss_platforms[] = {

    {-296, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-240, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {-200, 60, 16, 10, 0, 0, bn::sprite_items::chest32x32, 0},
    {-152, 60, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-96, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {-48, 60, 16, 10, 0, 0, bn::sprite_items::chest32x32, 0},
    {0, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {56, 60, 16, 10, 0, 0, bn::sprite_items::bars32x32, 0},
    {104, 60, 32, 16, 0, 0, bn::sprite_items::chest32x32, 0},
    {152, 70, 16, 10, 0, -10, bn::sprite_items::barrel32x32, 0},
    {200, 60, 32, 16, 0, 0, bn::sprite_items::bars32x32, 0},
    {240, 60, 16, 10, 0, 0, bn::sprite_items::chest32x32, 0},
    {272, 70, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0}};

constexpr TriggerData world5_boss_triggers[] = {
    {-180, 48, 32, 24, false, "wave1"},
    {-80, 48, 32, 24, false, "wave2"},
    {70, 48, 32, 24, false, "wave3"}};

constexpr TrapData world5_boss_traps[] = {

    // The Mimic Lord's axe - relentless, closer and faster than the one
    // in the first World 5 level. Standing still is not an option.
    {TrapType::CHASE, -324, 20, 32, 16, 0, 0, bn::sprite_items::axe64x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0, 28, 3.0},

    // Static rusty bar in the first gap - always dangerous.
    {TrapType::BASE, -264, 60, 16, 16, 0, 0, bn::sprite_items::rostybar32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0, 0, 0, NO_PATH_TRAP, 0},

    // Attack 1: first thwomp slam, triggered by wave1.
    {TrapType::MOVING, -175, -20, 32, 32, 0, 0, bn::sprite_items::thwomp32x32,
     1, TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 5, NO_PATH_TRAP, 0, 0, 0, "wave1"},

    // First mimic, disguised among the real chests.
    {TrapType::AMBUSH,
     24,
     60,
     16,
     16,
     0,
     0,
     bn::sprite_items::mimic32x32,
     16,
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
     -2.2,
     16},

    // Attack 2: second thwomp slam, triggered by wave2.
    {TrapType::MOVING, -75, -20, 32, 32, 0, 0, bn::sprite_items::thwomp32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 4, 5, NO_PATH_TRAP, 0, 0, 0, "wave2"},

    // Second mimic, closer to the door.
    {TrapType::AMBUSH,
     180,
     60,
     16,
     16,
     0,
     0,
     bn::sprite_items::mimic32x32,
     16,
     TRAP_GRAPHICS_INDEXES_8,
     0,
     0,
     0,
     NO_PATH_TRAP,
     0,
     0,
     0,
     nullptr,
     36,
     2.0,
     15},

    // Attack 3: final thwomp slam right before the door, triggered by
    // wave3.
    {TrapType::MOVING, 80, -20, 32, 32, 0, 0, bn::sprite_items::thwomp32x32, 1,
     TRAP_GRAPHICS_INDEXES_0, 0.0, 4.5, 4.5, NO_PATH_TRAP, 0, 0, 0, "wave3"}};

const LevelData LEVEL_WORLD5_BOSS = {

    world5_boss_platforms,
    sizeof(world5_boss_platforms) / sizeof(world5_boss_platforms[0]),

    world5_boss_triggers,
    sizeof(world5_boss_triggers) / sizeof(world5_boss_triggers[0]),

    world5_boss_traps,
    sizeof(world5_boss_traps) / sizeof(world5_boss_traps[0]),

    bn::regular_bg_items::world5,
    bn::music_items::world5,
    // Player spawn
    {-296, 30},
    // Door position
    {284, 40},
    // World size (enables scrolling)
    650,
    160};

// =============================================================================
// WORLD 5 FINALE - "Reunited" (world_width=360, 1.5 screens)
// A quiet victory lap after the boss: one unbroken barrel32x32 walkway,
// no traps of any kind, ending with Susanne waiting on the platform.
// Walking up to her (the level's "door", rendered as a static susanne16x16
// sprite instead of the usual animated door) ends the level - and since
// this is the last entry in main.cpp's levels[] array, the game
// transitions straight into KissingScene.
// =============================================================================

constexpr PlatformData world5_finale_platforms[] = {

    {-160, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-128, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-96, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-64, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {-32, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {0, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {32, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {64, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {96, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {128, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0},
    {160, 64, 32, 16, 0, -10, bn::sprite_items::barrel32x32, 0}};

const LevelData LEVEL_WORLD5_FINALE = {

    world5_finale_platforms,
    sizeof(world5_finale_platforms) / sizeof(world5_finale_platforms[0]),

    // No triggers - nothing in this level needs one.
    nullptr,
    0,

    // No traps, as requested - this is a walk, not a challenge.
    nullptr,
    0,

    bn::regular_bg_items::world5,
    bn::music_items::world5,
    // Player spawn - left end of the walkway.
    {-160, 30},
    // Door position - Susanne, standing on the rightmost platform,
    // animated through her own 8-frame loop and flipped to face left,
    // toward the player approaching from spawn. Flip the last bool if she
    // ends up facing the wrong way.
    {160, 40, bn::sprite_items::susanne16x16, TRAP_GRAPHICS_INDEXES_8, 8, true},
    // World size (1.5 screen widths)
    360,
    160};
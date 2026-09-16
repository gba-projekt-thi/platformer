#pragma once

#include "bn_fixed_point.h"
#include "bn_span.h"

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
//
// This file holds constants shared across more than one world's level data
// (animation sequences, the empty-path sentinel, and any patrol path reused
// by multiple worlds). World-local paths that are only ever referenced by a
// single world belong in that world's own levels_worldN.h instead.
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
//
// FIGURE_8_PATH: used by World 1 (LEVEL_0 and LEVEL_WORLD1_BOSS).
// LEVEL3_BRANCH_PATH: used by World 2 (world2_boss_traps) and World 3
// (LEVEL_3, LEVEL_WORLD3_SCROLL, LEVEL_WORLD3_BOSS) - crosses a world
// boundary, so it must live here rather than in a single levels_worldN.h.
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
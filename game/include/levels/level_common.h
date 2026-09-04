#pragma once

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_music_items.h"
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_items_level2.h"
#include "bn_regular_bg_items_level3.h"
#include "bn_span.h"

#include "bn_sprite_items_baumstamm.h"
#include "bn_sprite_items_box32x32.h"
#include "bn_sprite_items_branch32x16.h"
#include "bn_sprite_items_bubbles.h"
#include "bn_sprite_items_can32x32.h"
#include "bn_sprite_items_connector16x16.h"
#include "bn_sprite_items_door32x32.h"
#include "bn_sprite_items_kugelfisch.h"
#include "bn_sprite_items_ladder.h"
#include "bn_sprite_items_mushroom32x32.h"
#include "bn_sprite_items_nebel.h"
#include "bn_sprite_items_pipe16x16.h"
#include "bn_sprite_items_platformslvl1.h"
#include "bn_sprite_items_platformslvl2.h"
#include "bn_sprite_items_rustynails32x16.h"
#include "bn_sprite_items_slithermanarms32x32.h"

#include "level_structure.h"

// -----------------------------------------------------------------------------
// Shared animation sequences
// -----------------------------------------------------------------------------

constexpr uint16_t TRAP_GRAPHICS_INDEXES_3[] = {0, 1, 2};
constexpr uint16_t TRAP_GRAPHICS_INDEXES_8[] = {0, 1, 2, 3, 4, 5, 6, 7};

constexpr bn::span<const uint16_t> TRAP_GRAPHICS_INDEXES_0(nullptr, 0);

// -----------------------------------------------------------------------------
// Shared path data
// -----------------------------------------------------------------------------

constexpr bn::span<const bn::fixed_point> NO_PATH_TRAP(nullptr, 0);

constexpr bn::fixed_point FIGURE_8_PATH[] = {
    bn::fixed_point(0, 0),     bn::fixed_point(38, 21),
    bn::fixed_point(71, 30),   bn::fixed_point(92, 21),
    bn::fixed_point(100, 0),   bn::fixed_point(92, -21),
    bn::fixed_point(71, -30),  bn::fixed_point(38, -21),

    bn::fixed_point(0, 0),     bn::fixed_point(-38, 21),
    bn::fixed_point(-71, 30),  bn::fixed_point(-92, 21),
    bn::fixed_point(-100, 0),  bn::fixed_point(-92, -21),
    bn::fixed_point(-71, -30), bn::fixed_point(-38, -21)};

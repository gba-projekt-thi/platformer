#pragma once

// This file is the single include point for all level data (kept for
// backward compatibility with main.cpp and any other existing includers).
// Level data itself lives in per-world headers under game/include/:
//
//   levels_common.h  - constants shared across more than one world
//                       (animation sequences, empty-path sentinel, and any
//                       patrol path reused by multiple worlds)
//   levels_world1.h  - LEVEL_0, LEVEL_1, LEVEL_WORLD1_SCROLL,
//                       LEVEL_WORLD1_BOSS
//   levels_world2.h  - LEVEL_2, LEVEL_WORLD2_SCROLL, LEVEL_WORLD2_BOSS
//   levels_world3.h  - LEVEL_3, LEVEL_WORLD3_SCROLL, LEVEL_WORLD3_BOSS
//   levels_world4.h  - LEVEL_WORLD4_SCROLL, LEVEL_WORLD4_SCROLL_2,
//                       LEVEL_WORLD4_BOSS
//   levels_world5.h  - LEVEL_WORLD5_SCROLL, LEVEL_WORLD5_SCROLL_2,
//                       LEVEL_WORLD5_BOSS
//
// Adding a level to an existing world: add it to that world's own header.
// Adding a new world: create levels_worldN.h following the same pattern
// and include it below.

#include "level_structure.h"
#include "levels_common.h"
#include "levels_world1.h"
#include "levels_world2.h"
#include "levels_world3.h"
#include "levels_world4.h"
#include "levels_world5.h"
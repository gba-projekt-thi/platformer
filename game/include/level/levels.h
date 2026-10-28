#pragma once

// Single include point for all level data. levels_world1..5.h are generated
// from game/levels/world*.json by tools/Generate_levels.py (never edit them by
// hand); levels_common.h holds the constants shared by more than one world.
// A new world needs its levels_worldN.h included below.

#include "level_structure.h"
#include "levels_common.h"
#include "levels_world1.h"
#include "levels_world2.h"
#include "levels_world3.h"
#include "levels_world4.h"
#include "levels_world5.h"
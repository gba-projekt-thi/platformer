#pragma once

// -----------------------------------------------------------------------------
// WorldIndex
//
// Describes how the flat levels[] array in main.cpp is grouped into worlds,
// for use by WorldSelectScene / LevelSelectScene. This table is NOT derived
// from levels[] automatically - it must be kept in sync by hand whenever
// main.cpp's levels[] array changes (add/remove/reorder a level). Same
// maintenance burden as the 'Ordered old-then-new per world in main.cpp's
// levels[] array' convention already governing levels.h.
//
// start_index/level_count are indices into the SAME bn::span<const LevelData>
// every scene already receives (StartScene, LevelScene, KissingScene) - not
// a separate array.
// -----------------------------------------------------------------------------

struct WorldEntry {
    const char* name;
    int start_index;
    int level_count;
};

namespace WorldIndex {

inline constexpr WorldEntry WORLDS[] = {
    {"World 1", 0,
     4},  // LEVEL_0, LEVEL_1, LEVEL_WORLD1_SCROLL, LEVEL_WORLD1_BOSS
    {"World 2", 4, 3},  // LEVEL_2, LEVEL_WORLD2_SCROLL, LEVEL_WORLD2_BOSS
    {"World 3", 7, 3},  // LEVEL_3, LEVEL_WORLD3_SCROLL, LEVEL_WORLD3_BOSS
    {"World 4", 10,
     3},  // LEVEL_WORLD4_SCROLL, LEVEL_WORLD4_SCROLL_2, LEVEL_WORLD4_BOSS
    {"World 5", 13,
     3},  // LEVEL_WORLD5_SCROLL, LEVEL_WORLD5_SCROLL_2, LEVEL_WORLD5_BOSS
};

inline constexpr int WORLD_COUNT = sizeof(WORLDS) / sizeof(WORLDS[0]);

}  // namespace WorldIndex
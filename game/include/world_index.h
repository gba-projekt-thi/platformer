#pragma once

// -----------------------------------------------------------------------------
// WorldIndex
//
// Describes how the flat levels[] array in main.cpp is grouped into worlds,
// for use by WorldSelectScene / LevelSelectScene. This table is NOT derived
// from levels[] automatically - it must be kept in sync by hand whenever
// main.cpp's levels[] array changes (add/remove/reorder a level). The
// compiler catches gaps, overlaps and a wrong total (static_asserts below and
// in main.cpp); tools/Check_world_index.py also catches duplicates.
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
    {"World 1", 0, 5},   // LEVEL_0, LEVEL_1, LEVEL_WORLD1_SCROLL,
                         // LEVEL_WORLD1_TRICK, LEVEL_WORLD1_BOSS
    {"World 2", 5, 4},   // LEVEL_2, LEVEL_WORLD2_SCROLL, LEVEL_WORLD2_TRICK,
                         // LEVEL_WORLD2_BOSS
    {"World 3", 9, 4},   // LEVEL_3, LEVEL_WORLD3_SCROLL, LEVEL_WORLD3_TRICK,
                         // LEVEL_WORLD3_BOSS
    {"World 4", 13, 4},  // LEVEL_WORLD4_POGO, LEVEL_WORLD4_SCROLL,
                         // LEVEL_WORLD4_SCROLL_2, LEVEL_WORLD4_BOSS
    {"World 5", 17, 8},  // LEVEL_WORLD5_SCROLL, LEVEL_WORLD5_SCROLL_2,
                         // LEVEL_WORLD5_DASH, LEVEL_WORLD5_LEVERS,
                         // LEVEL_WORLD5_CLIMB, LEVEL_WORLD5_TRICK,
                         // LEVEL_WORLD5_BOSS, LEVEL_WORLD5_FINALE
};

inline constexpr int WORLD_COUNT = sizeof(WORLDS) / sizeof(WORLDS[0]);

// The worlds follow each other without gaps or overlaps, starting at level 0,
// and none is empty. Checked at compile time below (and by
// tools/Check_world_index.py against main.cpp's levels[]).
constexpr auto contiguous() -> bool {
    int expected_start = 0;
    for (const WorldEntry& world : WORLDS) {
        if (world.start_index != expected_start || world.level_count <= 0) {
            return false;
        }
        expected_start += world.level_count;
    }
    return true;
}
static_assert(contiguous(), "WorldIndex::WORLDS must be contiguous");

// Levels covered by all worlds: main.cpp static_asserts levels[] against it.
inline constexpr int LEVEL_COUNT =
    WORLDS[WORLD_COUNT - 1].start_index + WORLDS[WORLD_COUNT - 1].level_count;

// Returns the index into WORLDS of the world that `level_index` belongs to
// - the highest-indexed world whose start_index is <= level_index. Same
// boundary logic WorldSelectScene::_is_unlocked() already uses, factored
// out so other screens (e.g. a save-slot progress preview) can answer
// "which world is this save at" without duplicating it. Clamped to world
// 0 for any level_index before the first world (i.e. never negative).
inline int world_for_level(int level_index) {
    int result = 0;
    for (int i = 0; i < WORLD_COUNT; ++i) {
        if (WORLDS[i].start_index <= level_index) {
            result = i;
        }
    }
    return result;
}

}  // namespace WorldIndex
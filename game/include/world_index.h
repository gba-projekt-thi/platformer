#pragma once

#include "level_roster.h"

// -----------------------------------------------------------------------------
// WorldIndex
//
// Describes how the flat levels[] array in main.cpp is grouped into worlds,
// for use by WorldSelectScene / LevelSelectScene. Both are derived from
// game/levels/world*.json (tools/Generate_levels.py writes level_roster.h:
// world N = world<N>.json, its levels in file order), so adding a level is one
// edit in the JSON.
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

inline constexpr int WORLD_COUNT = sizeof(level_roster::WORLD_LEVEL_COUNTS) /
                                   sizeof(level_roster::WORLD_LEVEL_COUNTS[0]);

struct WorldTable {
    WorldEntry entries[WORLD_COUNT];

    constexpr const WorldEntry& operator[](int world) const {
        return entries[world];
    }
    constexpr const WorldEntry* begin() const { return entries; }
    constexpr const WorldEntry* end() const { return entries + WORLD_COUNT; }
};

constexpr auto make_worlds() -> WorldTable {
    WorldTable table{};
    int start = 0;
    for (int i = 0; i < WORLD_COUNT; ++i) {
        const int count = level_roster::WORLD_LEVEL_COUNTS[i];
        table.entries[i] = {level_roster::WORLD_NAMES[i], start, count};
        start += count;
    }
    return table;
}

inline constexpr WorldTable WORLDS = make_worlds();

// Levels covered by all worlds (= the size of levels[] in main.cpp).
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
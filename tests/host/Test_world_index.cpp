#include "test_framework.h"

#include "world_index.h"

TEST_CASE(world_for_level_world_count_is_five) {
    // Regression guard: if a world gets added/removed without updating
    // this test, that's a signal the sweep test below needs new bounds
    // too, not just a silent pass.
    CHECK_EQ(WorldIndex::WORLD_COUNT, 5);
}

TEST_CASE(world_for_level_first_level_of_each_world) {
    CHECK_EQ(WorldIndex::world_for_level(0), 0);   // World 1
    CHECK_EQ(WorldIndex::world_for_level(4), 1);   // World 2
    CHECK_EQ(WorldIndex::world_for_level(7), 2);   // World 3
    CHECK_EQ(WorldIndex::world_for_level(10), 3);  // World 4
    CHECK_EQ(WorldIndex::world_for_level(13), 4);  // World 5
}

TEST_CASE(world_for_level_last_level_of_each_world) {
    CHECK_EQ(WorldIndex::world_for_level(3), 0);   // World 1
    CHECK_EQ(WorldIndex::world_for_level(6), 1);   // World 2
    CHECK_EQ(WorldIndex::world_for_level(9), 2);   // World 3
    CHECK_EQ(WorldIndex::world_for_level(12), 3);  // World 4
    CHECK_EQ(WorldIndex::world_for_level(16), 4);  // World 5 (the finale)
}

TEST_CASE(world_for_level_exhaustive_sweep_over_every_real_level) {
    // Every one of the 17 levels currently in the roster (see
    // levels_common.h / main.cpp's levels[]), checked against the exact
    // world boundaries WORLDS itself declares - not hardcoded numbers
    // that could silently drift from WORLDS.
    for (int world = 0; world < WorldIndex::WORLD_COUNT; ++world) {
        const WorldEntry& entry = WorldIndex::WORLDS[world];
        for (int i = 0; i < entry.level_count; ++i) {
            const int level_index = entry.start_index + i;
            CHECK_EQ(WorldIndex::world_for_level(level_index), world);
        }
    }
}

TEST_CASE(world_for_level_clamps_to_world_zero_before_the_first_world) {
    // Documented behavior: never negative, clamps to world 0.
    CHECK_EQ(WorldIndex::world_for_level(-1), 0);
    CHECK_EQ(WorldIndex::world_for_level(-100), 0);
}

TEST_CASE(world_for_level_beyond_the_last_world_stays_on_the_last_world) {
    // GameState::MAX_LEVELS (24) has headroom above the current 17-level
    // roster for future levels - furthest_level could plausibly exceed
    // the last currently-defined world's range without WORLDS itself
    // having grown yet. Should saturate at the last world, not go out
    // of bounds or wrap.
    CHECK_EQ(WorldIndex::world_for_level(20), 4);
    CHECK_EQ(WorldIndex::world_for_level(100), 4);
}
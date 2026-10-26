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
    CHECK_EQ(WorldIndex::world_for_level(11), 3);  // World 4
    CHECK_EQ(WorldIndex::world_for_level(15), 4);  // World 5
}

TEST_CASE(world_for_level_last_level_of_each_world) {
    CHECK_EQ(WorldIndex::world_for_level(3), 0);   // World 1
    CHECK_EQ(WorldIndex::world_for_level(6), 1);   // World 2
    CHECK_EQ(WorldIndex::world_for_level(10), 2);  // World 3
    CHECK_EQ(WorldIndex::world_for_level(14), 3);  // World 4
    CHECK_EQ(WorldIndex::world_for_level(21), 4);  // World 5 (the finale)
}

TEST_CASE(world_for_level_new_world_5_levels_belong_to_world_5) {
    // The levels added to World 5 after its first two sit between the second
    // scroll level and the boss.
    CHECK_EQ(WorldIndex::world_for_level(17), 4);  // LEVEL_WORLD5_DASH
    CHECK_EQ(WorldIndex::world_for_level(18), 4);  // LEVEL_WORLD5_LEVERS
    CHECK_EQ(WorldIndex::world_for_level(19), 4);  // LEVEL_WORLD5_CLIMB
    CHECK_EQ(WorldIndex::world_for_level(20), 4);  // LEVEL_WORLD5_BOSS
}

TEST_CASE(world_index_worlds_are_contiguous_and_cover_the_roster) {
    // Same invariant tools/Check_world_index.py checks against main.cpp's
    // levels[]: no gaps or overlaps, total equals the roster size.
    int expected_start = 0;
    for (int world = 0; world < WorldIndex::WORLD_COUNT; ++world) {
        const WorldEntry& entry = WorldIndex::WORLDS[world];
        CHECK_EQ(entry.start_index, expected_start);
        CHECK(entry.level_count > 0);
        expected_start += entry.level_count;
    }
    CHECK_EQ(expected_start, 22);
}

TEST_CASE(world_index_contiguity_rule_and_level_count) {
    // The same rule as a constant expression: world_index.h static_asserts
    // it, and main.cpp static_asserts levels[] against LEVEL_COUNT.
    CHECK(WorldIndex::contiguous());
    CHECK_EQ(WorldIndex::LEVEL_COUNT, 22);
}

TEST_CASE(world_for_level_exhaustive_sweep_over_every_real_level) {
    // Every one of the 22 levels currently in the roster (see
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
    // GameState::MAX_LEVELS (24) has headroom above the current 22-level
    // roster for future levels - furthest_level could plausibly exceed
    // the last currently-defined world's range without WORLDS itself
    // having grown yet. Should saturate at the last world, not go out
    // of bounds or wrap.
    CHECK_EQ(WorldIndex::world_for_level(22), 4);
    CHECK_EQ(WorldIndex::world_for_level(100), 4);
}
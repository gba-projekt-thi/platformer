#include "test_framework.h"

#include "cfg.h"
#include "menu_nav.h"
#include "world_index.h"

TEST_CASE(menu_nav_next_moves_down_and_wraps_to_the_top) {
    CHECK_EQ(menu_nav::next_index(0, 4), 1);
    CHECK_EQ(menu_nav::next_index(2, 4), 3);
    CHECK_EQ(menu_nav::next_index(3, 4), 0);
}

TEST_CASE(menu_nav_prev_moves_up_and_wraps_to_the_bottom) {
    CHECK_EQ(menu_nav::prev_index(3, 4), 2);
    CHECK_EQ(menu_nav::prev_index(1, 4), 0);
    CHECK_EQ(menu_nav::prev_index(0, 4), 3);
}

TEST_CASE(menu_nav_single_entry_stays_put) {
    CHECK_EQ(menu_nav::next_index(0, 1), 0);
    CHECK_EQ(menu_nav::prev_index(0, 1), 0);
}

TEST_CASE(menu_nav_empty_menu_returns_zero) {
    CHECK_EQ(menu_nav::next_index(0, 0), 0);
    CHECK_EQ(menu_nav::prev_index(0, 0), 0);
    CHECK_EQ(menu_nav::next_index(3, -1), 0);
}

TEST_CASE(menu_nav_full_cycle_returns_to_the_start) {
    // Start screen: 3 slots + Controls = 4 entries; pause menu: 5 entries.
    for (int count = 1; count <= 5; ++count) {
        int index = 0;
        for (int i = 0; i < count; ++i) {
            index = menu_nav::next_index(index, count);
        }
        CHECK_EQ(index, 0);
        for (int i = 0; i < count; ++i) {
            index = menu_nav::prev_index(index, count);
        }
        CHECK_EQ(index, 0);
    }
}

TEST_CASE(menu_nav_next_and_prev_are_inverses) {
    for (int index = 0; index < 5; ++index) {
        CHECK_EQ(
            menu_nav::prev_index(menu_nav::next_index(index, 5), 5), index);
        CHECK_EQ(
            menu_nav::next_index(menu_nav::prev_index(index, 5), 5), index);
    }
}

// ---- list layout
// -------------------------------------------------------------

TEST_CASE(menu_nav_a_short_list_starts_where_it_always_did) {
    CHECK_EQ(menu_nav::first_row_y(4, -10, 16, 70), -10);
    CHECK_EQ(menu_nav::first_row_y(6, -10, 16, 70), -10);
}

TEST_CASE(menu_nav_a_long_list_moves_up_to_keep_its_last_row_on_screen) {
    CHECK_EQ(menu_nav::first_row_y(7, -10, 16, 70), -26);
    // The level select with the longest world (8x16 font: a row reaches 8 px
    // below its center; the screen ends at y = 80).
    int longest = 0;
    for (const WorldEntry& world : WorldIndex::WORLDS) {
        longest = world.level_count > longest ? world.level_count : longest;
    }
    const int first = menu_nav::first_row_y(
        longest, Cfg::StartScreen::Y, Cfg::LevelSelect::ROW_SPACING,
        Cfg::LevelSelect::LAST_ROW_Y);
    CHECK(first + (longest - 1) * Cfg::LevelSelect::ROW_SPACING + 8 <= 80);
    CHECK(first - 8 >= -80);
}

#include "test_framework.h"

#include "switch_logic.h"

using switch_logic::boxes_overlap;
using switch_logic::platform_active;

TEST_CASE(switch_platform_active_truth_table) {
    // active_when_on = true: appears when the trigger turns on.
    CHECK(platform_active(true, true));
    CHECK(!platform_active(false, true));
    // active_when_on = false: vanishes when the trigger turns on.
    CHECK(!platform_active(true, false));
    CHECK(platform_active(false, false));
}

TEST_CASE(switch_two_platforms_on_one_trigger_are_always_opposite) {
    // "Switch opens path A, closes path B".
    for (bool trigger_on : {false, true}) {
        CHECK(
            platform_active(trigger_on, true) !=
            platform_active(trigger_on, false));
    }
}

TEST_CASE(switch_boxes_overlap_when_centers_are_close) {
    CHECK(boxes_overlap(0, 0, 16, 16, 4, 4, 8, 8));
    CHECK(boxes_overlap(0, 0, 16, 16, 0, 0, 16, 16));
}

TEST_CASE(switch_boxes_overlap_when_one_contains_the_other) {
    CHECK(boxes_overlap(0, 0, 32, 32, 2, -3, 4, 4));
    CHECK(boxes_overlap(2, -3, 4, 4, 0, 0, 32, 32));
}

TEST_CASE(switch_boxes_that_only_touch_do_not_overlap) {
    // Same strict comparisons as CollisionRegistry::check_rect().
    CHECK(!boxes_overlap(0, 0, 16, 16, 16, 0, 16, 16));   // side by side
    CHECK(!boxes_overlap(0, 0, 16, 16, 0, 16, 16, 16));   // stacked
    CHECK(!boxes_overlap(0, 0, 16, 16, -16, 0, 16, 16));  // mirrored
}

TEST_CASE(switch_boxes_apart_on_either_axis_do_not_overlap) {
    CHECK(!boxes_overlap(0, 0, 16, 16, 40, 0, 16, 16));
    CHECK(!boxes_overlap(0, 0, 16, 16, 0, 40, 16, 16));
    // Overlapping in x but not in y, and vice versa.
    CHECK(!boxes_overlap(0, 0, 16, 16, 4, 30, 16, 16));
    CHECK(!boxes_overlap(0, 0, 16, 16, 30, 4, 16, 16));
}

TEST_CASE(switch_boxes_overlap_works_with_fractional_values) {
    CHECK(boxes_overlap(0.0, 0.0, 8.0, 8.0, 7.5, 0.0, 8.0, 8.0));
    CHECK(!boxes_overlap(0.0, 0.0, 8.0, 8.0, 8.0, 0.0, 8.0, 8.0));
}

TEST_CASE(switch_boxes_overlap_is_symmetric) {
    for (int dx = -20; dx <= 20; dx += 4) {
        for (int dy = -20; dy <= 20; dy += 4) {
            CHECK_EQ(
                boxes_overlap(0, 0, 8, 8, dx, dy, 16, 12),
                boxes_overlap(dx, dy, 16, 12, 0, 0, 8, 8));
        }
    }
}

TEST_CASE(switch_player_sized_box_against_platform_sized_box) {
    // Duck (8x8) standing on top of a 16x10 platform: touching, not inside.
    CHECK(!boxes_overlap(0, -9, 8, 8, 0, 0, 16, 10));
    // One pixel lower the duck would be inside the platform.
    CHECK(boxes_overlap(0, -8, 8, 8, 0, 0, 16, 10));
}

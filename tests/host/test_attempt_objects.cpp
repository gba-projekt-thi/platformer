#include "test_framework.h"

#include "attempt_objects.h"

using attempt_objects::has_window;
using attempt_objects::platform_action;
using attempt_objects::PlatformAction;

TEST_CASE(attempt_objects_zero_zero_has_no_window) {
    // Every trap and platform authored before the fields existed.
    CHECK(!has_window(0, 0));
    CHECK(has_window(3, 0));
    CHECK(has_window(0, 3));
    CHECK(has_window(2, 5));
}

TEST_CASE(attempt_objects_absent_platform_is_hidden_whatever_it_is) {
    // Switched or not: no collision, no picture on this attempt.
    CHECK(platform_action(false, false) == PlatformAction::Hide);
    CHECK(platform_action(false, true) == PlatformAction::Hide);
}

TEST_CASE(attempt_objects_present_plain_platform_is_shown) {
    // Plain, crumbling and fleeing ones: their own reset puts them back
    // untouched, so they are simply there again.
    CHECK(platform_action(true, false) == PlatformAction::Show);
}

TEST_CASE(attempt_objects_present_switched_platform_is_left_to_its_trigger) {
    // Showing it here would make it solid for a frame even with its trigger
    // saying "gone", and could put it inside the duck: DynamicPlatforms'
    // sync_switched() decides, as for every switched platform.
    CHECK(platform_action(true, true) == PlatformAction::LeaveToSwitch);
}

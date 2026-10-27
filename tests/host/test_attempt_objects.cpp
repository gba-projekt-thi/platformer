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

// -- layout_changes() --------------------------------------------------------

namespace {

using attempt_objects::layout_changes;
using attempt_objects::window_flips;

// Stand-ins for PlatformData / TrapData / DoorData / LevelData: only the
// fields layout_changes() reads.
struct Windowed {
    unsigned from_death = 0;
    unsigned until_death = 0;
};

struct Spot {
    int px = 0;
    int py = 0;
    friend bool operator==(const Spot&, const Spot&) = default;
};

struct Spots {
    const Spot* first = nullptr;
    unsigned count = 0;
    auto data() const -> const Spot* { return first; }
    auto size() const -> unsigned { return count; }
};

struct Door {
    Spots attempt_positions;
    unsigned attempt_every = 0;
};

struct Level {
    const Windowed* platforms = nullptr;
    int platform_count = 0;
    const Windowed* traps = nullptr;
    int trap_count = 0;
    Door door;
};

}  // namespace

TEST_CASE(attempt_objects_a_window_flips_where_it_opens_or_closes) {
    CHECK(!window_flips(0, 0, 0, 1));
    CHECK(!window_flips(3, 0, 1, 2));
    CHECK(window_flips(3, 0, 2, 3));
    CHECK(window_flips(0, 3, 2, 3));
    CHECK(!window_flips(0, 3, 3, 4));
    // Restart Level during a death freeze skips an attempt: what counts is
    // the layout that was there and the one that comes, not the one between.
    CHECK(window_flips(2, 0, 1, 3));
    CHECK(!window_flips(2, 3, 1, 3));
}

TEST_CASE(attempt_objects_a_level_without_windows_never_changes) {
    const Windowed plain[] = {{}, {}};
    const Level level{plain, 2, plain, 2, {}};
    for (unsigned deaths = 1; deaths < 10; ++deaths) {
        CHECK(!layout_changes(level, deaths - 1, deaths));
    }
}

TEST_CASE(attempt_objects_a_platform_or_trap_window_changes_the_layout) {
    const Windowed plain[] = {{}};
    const Windowed after_two[] = {{}, {2, 0}};
    const Level platforms{after_two, 2, plain, 1, {}};
    const Level traps{plain, 1, after_two, 2, {}};
    for (const Level& level : {platforms, traps}) {
        CHECK(!layout_changes(level, 0, 1));
        CHECK(layout_changes(level, 1, 2));
        CHECK(!layout_changes(level, 2, 3));
    }
}

TEST_CASE(attempt_objects_two_copies_swapping_places_is_a_change) {
    // "After the 3rd death the trap sits somewhere else."
    const Windowed copies[] = {{0, 3}, {3, 0}};
    const Level level{nullptr, 0, copies, 2, {}};
    CHECK(!layout_changes(level, 1, 2));
    CHECK(layout_changes(level, 2, 3));
    CHECK(!layout_changes(level, 3, 4));
}

TEST_CASE(attempt_objects_the_door_moving_changes_the_layout) {
    const Spot spots[] = {{0, 0}, {40, 0}, {40, 0}};
    const Level level{nullptr, 0, nullptr, 0, {{spots, 3}, 2}};
    CHECK(!layout_changes(level, 0, 1));
    CHECK(layout_changes(level, 1, 2));
    // A new variant at the same spot is no change the player could see.
    CHECK(!layout_changes(level, 3, 4));
    // The last spot stays.
    CHECK(!layout_changes(level, 9, 10));
}

TEST_CASE(attempt_objects_the_same_attempt_is_no_change) {
    // Level load (0 deaths) and the replay ROM's set-up compare an attempt
    // with itself.
    const Windowed copies[] = {{0, 1}, {1, 0}};
    const Level level{copies, 2, copies, 2, {}};
    CHECK(!layout_changes(level, 0, 0));
    CHECK(!layout_changes(level, 5, 5));
}

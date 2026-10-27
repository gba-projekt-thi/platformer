#include "test_framework.h"

#include "bn_fixed.h"

#include "door_logic.h"

// False exits (door_logic.h): a door that kills while its trap trigger is on,
// and a door that stands somewhere else after enough deaths.

namespace {

struct Point {
    bn::fixed px = 0;
    bn::fixed py = 0;
    [[nodiscard]] auto x() const -> bn::fixed { return px; }
    [[nodiscard]] auto y() const -> bn::fixed { return py; }
};

struct FakeTrigger {
    bool on = false;
    [[nodiscard]] auto is_triggered() const -> bool { return on; }
};

using door_logic::Touch;

// The first spot is the door itself: 0 deaths keep the authored door.
constexpr Point SPOTS[] = {{0, 0}, {-120, 0}, {80, -40}};

}  // namespace

TEST_CASE(door_logic_an_untouched_door_does_nothing) {
    CHECK(door_logic::on_touch(false, false, false) == Touch::None);
    CHECK(door_logic::on_touch(false, false, true) == Touch::None);
}

TEST_CASE(door_logic_an_ordinary_door_finishes_the_level) {
    CHECK(door_logic::on_touch(true, false, false) == Touch::Finish);
}

TEST_CASE(door_logic_a_trapped_door_kills_the_duck) {
    CHECK(door_logic::on_touch(true, false, true) == Touch::Kill);
}

TEST_CASE(door_logic_a_locked_door_is_never_entered) {
    // The boss lock wins over both: a hidden door neither finishes nor kills.
    CHECK(door_logic::on_touch(true, true, false) == Touch::None);
    CHECK(door_logic::on_touch(true, true, true) == Touch::None);
}

TEST_CASE(door_logic_trapped_only_while_its_trigger_is_on) {
    FakeTrigger trigger;
    CHECK(!door_logic::trapped(&trigger));
    trigger.on = true;
    CHECK(door_logic::trapped(&trigger));
}

TEST_CASE(door_logic_without_a_trap_trigger_the_door_is_never_trapped) {
    const FakeTrigger* none = nullptr;
    CHECK(!door_logic::trapped(none));
}

TEST_CASE(door_logic_without_attempt_positions_the_door_stays) {
    const Point offset = door_logic::attempt_offset<Point>(nullptr, 0, 5, 1);
    CHECK_EQ(offset.x(), bn::fixed(0));
    CHECK_EQ(offset.y(), bn::fixed(0));
}

TEST_CASE(door_logic_the_door_moves_on_every_few_deaths) {
    // A new spot every 2 deaths: 0-1 the first, 2-3 the second, 4+ the last.
    CHECK_EQ(door_logic::attempt_offset(SPOTS, 3, 0, 2).x(), bn::fixed(0));
    CHECK_EQ(door_logic::attempt_offset(SPOTS, 3, 1, 2).x(), bn::fixed(0));
    CHECK_EQ(door_logic::attempt_offset(SPOTS, 3, 2, 2).x(), bn::fixed(-120));
    CHECK_EQ(door_logic::attempt_offset(SPOTS, 3, 4, 2).x(), bn::fixed(80));
    CHECK_EQ(door_logic::attempt_offset(SPOTS, 3, 4, 2).y(), bn::fixed(-40));
    CHECK_EQ(door_logic::attempt_offset(SPOTS, 3, 99, 2).x(), bn::fixed(80));
}

TEST_CASE(door_logic_attempt_every_zero_keeps_the_first_spot) {
    CHECK_EQ(door_logic::attempt_offset(SPOTS, 3, 7, 0).x(), bn::fixed(0));
}

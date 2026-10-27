#pragma once

#include <cstdint>

#include "attempt_variant.h"

// -----------------------------------------------------------------------------
// door_logic
//
// Level Devil's false exits, beyond the fake-door trap:
//
//   * The door that turns into a trap (DoorData::trap_trigger_name): while
//     that trigger is on, touching the real door kills the duck instead of
//     finishing the level. on_touch() decides, trapped() reads the trigger.
//     No tell: the door looks the same either way - the joke is that the one
//     door that was safe a moment ago is not any more.
//   * The door that moves between attempts (DoorData::attempt_positions /
//     attempt_every): attempt_offset() is where it stands, as an offset from
//     (x, y), after `deaths` deaths in the level.
//
// No Butano runtime: host-tested in tests/host/test_door_logic.cpp. Point is
// anything with x() / y() that a default construction puts at (0, 0)
// (bn::fixed_point in the game); Trigger anything with is_triggered().
// -----------------------------------------------------------------------------
namespace door_logic {

enum class Touch : uint8_t { None, Finish, Kill };

// reached: the duck touched the door this frame; locked: a boss still lives
// (the door is hidden and cannot be entered); trapped: see trapped(); died:
// something else already killed the duck this frame - a trapped door then does
// not kill it a second time.
inline auto on_touch(bool reached, bool locked, bool trapped, bool died = false)
    -> Touch {
    if (!reached || locked) {
        return Touch::None;
    }
    if (!trapped) {
        return Touch::Finish;
    }
    return died ? Touch::None : Touch::Kill;
}

// nullptr = the door has no trap trigger and is always a door.
template <typename Trigger>
auto trapped(const Trigger* trigger) -> bool {
    return trigger != nullptr && trigger->is_triggered();
}

// positions[variant_for_attempt(deaths, every, count)]; (0, 0) without any.
template <typename Point>
auto attempt_offset(
    const Point* positions,
    unsigned count,
    unsigned deaths,
    unsigned every) -> Point {
    if (positions == nullptr || count == 0) {
        return Point();
    }
    return positions[attempt_variant::variant_for_attempt(
        deaths, every, count)];
}

}  // namespace door_logic

#pragma once

#include "attempt_variant.h"
#include "door_logic.h"

// -----------------------------------------------------------------------------
// attempt_objects
//
// Level Devil: traps and platforms that are only there for some attempts
// (TrapData / PlatformData::from_death, until_death - see attempt_variant.h
// for the window itself). LevelManager asks these at level load and after
// every death or Restart Level; split out so the rules have no Butano
// dependency and are tested on the host (tests/host/test_attempt_objects.cpp).
//
// layout_changes() tells LevelManager whether a respawn brings a different
// layout than the attempt before it (any window opens or closes, or the
// moving door stands somewhere else), so the player gets a cue.
// -----------------------------------------------------------------------------

namespace attempt_objects {

// from 0, until 0 = always there, as every object before the fields existed:
// LevelManager never touches those, so old levels behave exactly as before.
inline auto has_window(unsigned from, unsigned until) -> bool {
    return from != 0 || until != 0;
}

// What a platform's start of an attempt does to its body and picture.
//
// Hide          -> not there this attempt: no collision, no picture, and it is
//                  kept out of DynamicPlatforms / FleeingPlatforms so nothing
//                  brings it back before the next attempt.
// Show          -> there: solid and visible (a crumbling or fleeing one is
//                  also put back untouched by its own reset).
// LeaveToSwitch -> there, but switched by a trigger: sync_switched() decides,
//                  as always (it also keeps it from appearing inside the duck).
enum class PlatformAction { Show, Hide, LeaveToSwitch };

inline auto platform_action(bool present, bool switched) -> PlatformAction {
    if (!present) {
        return PlatformAction::Hide;
    }
    return switched ? PlatformAction::LeaveToSwitch : PlatformAction::Show;
}

// Whether an object with this window is there on one of the two attempts
// (deaths in the level) and not on the other.
inline auto
window_flips(unsigned from, unsigned until, unsigned before, unsigned after)
    -> bool {
    return attempt_variant::active_for_deaths(before, from, until) !=
           attempt_variant::active_for_deaths(after, from, until);
}

// Whether the level looks different after `after` deaths than after `before`:
// a platform or trap window flips, or the door that moves between attempts
// (DoorData::attempt_positions / attempt_every) stands somewhere else. A new
// door variant at the same spot is no change. Level is LevelData in the game
// (platforms / traps with from_death / until_death, a door whose
// attempt_positions has data() / size() of points with ==); host stand-ins in
// the test.
template <typename Level>
auto layout_changes(const Level& level, unsigned before, unsigned after)
    -> bool {
    if (before == after) {
        return false;
    }
    for (int i = 0; i < level.platform_count; ++i) {
        if (window_flips(
                level.platforms[i].from_death, level.platforms[i].until_death,
                before, after)) {
            return true;
        }
    }
    for (int i = 0; i < level.trap_count; ++i) {
        if (window_flips(
                level.traps[i].from_death, level.traps[i].until_death, before,
                after)) {
            return true;
        }
    }
    const auto& spots = level.door.attempt_positions;
    const unsigned count = static_cast<unsigned>(spots.size());
    const unsigned every = level.door.attempt_every;
    return !(
        door_logic::attempt_offset(spots.data(), count, before, every) ==
        door_logic::attempt_offset(spots.data(), count, after, every));
}

}  // namespace attempt_objects

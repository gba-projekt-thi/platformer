#pragma once

// -----------------------------------------------------------------------------
// attempt_objects
//
// Level Devil: traps and platforms that are only there for some attempts
// (TrapData / PlatformData::from_death, until_death - see attempt_variant.h
// for the window itself). LevelManager asks these at level load and after
// every death or Restart Level; split out so the rules have no Butano
// dependency and are tested on the host (tests/host/test_attempt_objects.cpp).
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

}  // namespace attempt_objects

#pragma once

// -----------------------------------------------------------------------------
// switch_logic
//
// Pure rules for platforms bound to a trigger (PlatformData::trigger_name),
// split out of LevelManager so they can be unit-tested on the host (see
// tests/host/test_switch_logic.cpp). Dependency-free; boxes_overlap() is a
// template so LevelManager can use it with bn::fixed and tests with int/float.
// -----------------------------------------------------------------------------

namespace switch_logic {

// A switched platform is solid and visible while the trigger's state equals
// `active_when_on`: true = appears when the trigger is on, false = vanishes
// when it turns on.
inline auto platform_active(bool trigger_on, bool active_when_on) -> bool {
    return trigger_on == active_when_on;
}

// Center-based box overlap with the same strict comparisons as
// CollisionRegistry::check_rect() - boxes that merely touch do not overlap.
// LevelManager uses it to hold back a re-appearing platform until it would
// no longer materialize inside the player (which would trap the duck).
template <typename T>
auto boxes_overlap(T ax, T ay, T aw, T ah, T bx, T by, T bw, T bh) -> bool {
    const T a_half_w = aw / 2;
    const T a_half_h = ah / 2;
    const T b_half_w = bw / 2;
    const T b_half_h = bh / 2;
    return ax - a_half_w < bx + b_half_w && ax + a_half_w > bx - b_half_w &&
           ay - a_half_h < by + b_half_h && ay + a_half_h > by - b_half_h;
}

}  // namespace switch_logic

#pragma once

// -----------------------------------------------------------------------------
// level_limits
//
// The one rule LevelManager::_validate_level() applies to a level's entity
// counts against Cfg::Level::Limits, split out so it has no Butano dependency
// and can be unit-tested on the host (see tests/host/test_level_limits.cpp).
//
// The limits are CAPACITIES of fixed-size containers, so a level with exactly
// `limit` entries is valid and one more is not.
// -----------------------------------------------------------------------------

namespace level_limits {

// True when `count` entries fit a container of `limit` slots. A negative count
// (corrupt level data) never fits.
inline auto within(int count, unsigned int limit) -> bool {
    return count >= 0 && static_cast<unsigned int>(count) <= limit;
}

}  // namespace level_limits

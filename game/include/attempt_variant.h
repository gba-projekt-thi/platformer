#pragma once

// -----------------------------------------------------------------------------
// attempt_variant
//
// Level Devil: a level that changes the more often the duck dies in it. Pure
// helpers, split out of LevelManager so they have no Butano dependency and can
// be unit-tested on the host (see tests/host/test_attempt_variant.cpp).
//
//   * deaths_in_level(): the deaths of the current visit of a level, from the
//     run's total and the total when the level was entered.
//   * active_for_deaths(): whether something authored for the deaths
//     [from, until) is there on this attempt (a trap that moves after the
//     3rd death: one copy with until 3, one with from 3).
//   * variant_for_attempt(): which of `count` variants is in use when a new
//     one comes every `every` deaths (the last one stays).
// -----------------------------------------------------------------------------

namespace attempt_variant {

inline auto deaths_in_level(unsigned total_now, unsigned total_at_entry)
    -> unsigned {
    return total_now >= total_at_entry ? total_now - total_at_entry : 0u;
}

// until 0 = no end. from 0, until 0 = always there (every object so far).
inline auto active_for_deaths(unsigned deaths, unsigned from, unsigned until)
    -> bool {
    return deaths >= from && (until == 0 || deaths < until);
}

inline auto variant_for_attempt(unsigned deaths, unsigned every, unsigned count)
    -> unsigned {
    if (every == 0 || count <= 1) {
        return 0;
    }
    const unsigned variant = deaths / every;
    return variant < count ? variant : count - 1;
}

}  // namespace attempt_variant

#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// trap_timing
//
// Pure frame-count helpers for traps whose speed is authored as a number of
// frames instead of a velocity, split out of the Butano-dependent trap classes
// so they can be unit-tested on the host (see tests/host/test_trap_timing.cpp).
//
//   * scaled_frames(): Hard Mode / Assist Mode scaling of a frame count
//     (PathTrap::path_waits and the phases of a CycleTrap).
// -----------------------------------------------------------------------------

namespace trap_timing {

// Scales a duration in frames for a trap speed of `speed_percent` percent
// (135 = 1.35x faster, 50 = half speed, 100 = unchanged): a faster trap needs
// fewer frames. Rounds to the nearest frame and never turns a non-zero
// duration into 0 (that would freeze or skip the phase). 0 stays 0 - for a
// PathTrap that means "does not move", which scaling must not change. A
// non-positive percentage leaves the value alone.
inline auto scaled_frames(unsigned frames, int speed_percent) -> unsigned {
    if (frames == 0 || speed_percent <= 0 || speed_percent == 100) {
        return frames;
    }
    const unsigned percent = static_cast<unsigned>(speed_percent);
    const unsigned scaled = (frames * 100u + percent / 2u) / percent;
    return scaled < 1u ? 1u : scaled;
}

}  // namespace trap_timing

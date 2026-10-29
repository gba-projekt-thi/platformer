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
//   * CycleTiming and cycle_progress(): the timeline of a CycleTrap, e.g.
//     spikes that rise out of the floor, stay up, sink and wait.
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

// One round of a CycleTrap, in frames: it moves out over `extend` frames, stays
// out for `hold` frames, moves back over `retract` frames and rests for `wait`
// frames before the next round. An extend or retract of 0 is a jump, not a
// movement (spikes that pop up at once).
struct CycleTiming {
    unsigned extend = 0;
    unsigned hold = 0;
    unsigned retract = 0;
    unsigned wait = 0;
};

inline auto cycle_length(const CycleTiming& t) -> unsigned {
    return t.extend + t.hold + t.retract + t.wait;
}

// How far out the trap is at `frame` (0 = start of a round), in permille:
// 0 = at its start position, 1000 = fully out. Continuous from frame to frame
// except for a 0-frame extend / retract. A round of 0 frames never moves.
inline auto cycle_progress(const CycleTiming& t, unsigned frame) -> int {
    if (cycle_length(t) == 0) {
        return 0;
    }
    if (frame < t.extend) {
        return static_cast<int>(frame * 1000u / t.extend);
    }
    frame -= t.extend;
    if (frame < t.hold) {
        return 1000;
    }
    frame -= t.hold;
    if (frame < t.retract) {
        return 1000 - static_cast<int>(frame * 1000u / t.retract);
    }
    return 0;
}

// The frame after `frame`: wraps to 0 at the end of a round (no division, this
// runs every frame).
inline auto next_cycle_frame(const CycleTiming& t, unsigned frame) -> unsigned {
    ++frame;
    return frame >= cycle_length(t) ? 0u : frame;
}

// Where in the round the trap starts: the phase offset, wrapped into the
// round. Two traps with the same timing and different phases never move in
// lockstep.
inline auto cycle_start_frame(const CycleTiming& t, unsigned phase_frames)
    -> unsigned {
    const unsigned length = cycle_length(t);
    return length == 0 ? 0u : phase_frames % length;
}

// Hard / Assist Mode: every phase scaled with scaled_frames(), so the whole
// round runs faster (or slower) and the proportions stay.
inline auto scaled_cycle(const CycleTiming& t, int speed_percent)
    -> CycleTiming {
    return {
        scaled_frames(t.extend, speed_percent),
        scaled_frames(t.hold, speed_percent),
        scaled_frames(t.retract, speed_percent),
        scaled_frames(t.wait, speed_percent)};
}

// The trap speed multiplier (1.35 = Hard Mode) as the percentage the helpers
// above take. Works with bn::fixed.
template <typename T>
inline auto speed_percent(T multiplier) -> int {
    return (multiplier * 100).round_integer();
}

// What TrapFactory builds a CycleTrap from: the authored phases and start
// offset, all frame counts, scaled for Hard / Assist Mode.
struct CycleSetup {
    CycleTiming timing;
    unsigned phase;
};

inline auto cycle_setup(
    const CycleTiming& authored,
    unsigned phase_frames,
    int percent) -> CycleSetup {
    return {
        scaled_cycle(authored, percent), scaled_frames(phase_frames, percent)};
}

// Where a CycleTrap is in its round. A trap that is frozen (its trigger is
// off) stays where it is; a reset starts the round over from the start frame.
class CycleClock {
   public:
    static constexpr int FROZEN = -1;

    CycleClock(const CycleTiming& timing, unsigned phase_frames)
        : _timing(timing),
          _start_frame(cycle_start_frame(timing, phase_frames)),
          _frame(_start_frame) {}

    // How far out the trap is now (permille, see cycle_progress()).
    [[nodiscard]] auto progress() const -> int {
        return cycle_progress(_timing, _frame);
    }

    // One physics frame. A running clock returns the progress to show this
    // frame and moves on; a frozen one returns FROZEN and stays.
    auto tick(bool running) -> int {
        if (!running) {
            return FROZEN;
        }
        const int shown = progress();
        _frame = next_cycle_frame(_timing, _frame);
        return shown;
    }

    // Death / restart: back to the start frame; returns its progress.
    auto reset() -> int {
        _frame = _start_frame;
        return progress();
    }

   private:
    CycleTiming _timing;
    unsigned _start_frame;
    unsigned _frame;
};

// Where a ChaserTrap heads: `follow` px behind the duck, but never more than
// `limit` px right of its start (0 = no limit) - so a boss arena can be kept
// free of it. A negative `follow` is the mirrored chaser (Mirror Mode): it
// trails to the right of the duck and flies left, `limit` px left of its
// start at most.
template <typename T>
inline auto chase_target(T duck_x, T follow, T start_x, T limit) -> T {
    const T target = duck_x - follow;
    if (follow < 0) {
        if (limit > 0 && target < start_x - limit) {
            return start_x - limit;
        }
        return target;
    }
    if (limit > 0 && target > start_x + limit) {
        return start_x + limit;
    }
    return target;
}

// The chaser's x after one frame: it only ever flies away from where it
// trails (right, or left for a negative `follow`), by at most `speed`, and
// never retreats.
template <typename T>
inline auto chase_step(T x, T target, T follow, T speed) -> T {
    T step = follow < 0 ? x - target : target - x;
    if (step <= 0) {
        return x;
    }
    if (step > speed) {
        step = speed;
    }
    return follow < 0 ? x - step : x + step;
}

}  // namespace trap_timing

#pragma once

#include "cfg_boss.h"

// -----------------------------------------------------------------------------
// boss
//
// The rules of a boss fight, without Butano (host-tested in
// tests/host/test_boss_logic.cpp). Bosses are the one enemy the duck can
// beat: only a pogo strike from above hurts one, each hit starts the next
// phase (faster, new traps attack - BossTrap switches on the phase's
// trigger), and the last hit defeats it. A death starts the fight over.
// -----------------------------------------------------------------------------
namespace boss {

enum class Hit { Ignored, Hurt, Defeated };

class Fight {
   public:
    explicit Fight(int hits) : _hits(hits), _left(hits) {}

    // A pogo strike landed on the boss.
    auto strike() -> Hit {
        if (_left == 0 || _invulnerable > 0) {
            return Hit::Ignored;
        }
        --_left;
        if (_left == 0) {
            return Hit::Defeated;
        }
        _invulnerable = Cfg::Boss::INVULNERABLE_FRAMES;
        return Hit::Hurt;
    }

    // One frame.
    void tick() {
        if (_invulnerable > 0) {
            --_invulnerable;
        }
    }

    void reset() {
        _left = _hits;
        _invulnerable = 0;
    }

    // 1 at the start, + 1 per hit taken.
    [[nodiscard]] auto phase() const -> int { return _hits - _left + 1; }
    [[nodiscard]] auto hits_left() const -> int { return _left; }
    [[nodiscard]] auto defeated() const -> bool { return _left == 0; }
    [[nodiscard]] auto invulnerable() const -> bool {
        return _invulnerable > 0;
    }
    // Blinks while invulnerable.
    [[nodiscard]] auto visible() const -> bool {
        return _invulnerable == 0 ||
               (_invulnerable / Cfg::Boss::BLINK_FRAMES) % 2 == 0;
    }

   private:
    int _hits;
    int _left;
    int _invulnerable = 0;
};

// Whether a boss lost a hit since the last frame (a reset gives hits back:
// that is no hit).
inline auto was_hit(int hits_left_before, int hits_left_now) -> bool {
    return hits_left_now < hits_left_before;
}

// Frames per path segment in `phase` (1 = `base`): every phase is faster,
// phase 5 twice as fast as phase 1.
inline auto phase_frames(unsigned base, int phase) -> unsigned {
    const unsigned frames = base * 4 / unsigned(phase + 3);
    return frames > 0 ? frames : 1;
}

}  // namespace boss

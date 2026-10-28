#pragma once

#include "bn_fixed.h"

// -----------------------------------------------------------------------------
// clear_logic
//
// What the door finish and the opening of a boss door look like over time,
// without Butano (host-tested in tests/host/test_clear_logic.cpp).
//
//   * Moment: the short level-clear pause on every door finish - the world
//     freezes, the door blinks, "Clear!" stands on the screen, then the level
//     switches. Short on purpose: the game is about quick retries.
//   * Glow: the white flash of a door that has just been unlocked, fading out.
//   * LOCKED_GREY: how far a still locked boss door is faded to grey.
//   * BLINK_WHITE: how far a blinking door is faded to white.
// -----------------------------------------------------------------------------
namespace clear_logic {

inline constexpr int MOMENT_FRAMES = 40;
// The door is lit on the first BLINK_FRAMES of every 2 * BLINK_FRAMES.
inline constexpr int BLINK_FRAMES = 5;
inline constexpr int GLOW_FRAMES = 30;
inline constexpr bn::fixed LOCKED_GREY = bn::fixed(0.7);
inline constexpr bn::fixed BLINK_WHITE = bn::fixed(0.85);

class Moment {
   public:
    explicit Moment(int frames = MOMENT_FRAMES) : _frames(frames) {}

    void start() {
        _elapsed = 0;
        _running = true;
    }
    // One frame; the moment is over after `frames` of them.
    void tick() {
        if (_running && ++_elapsed >= _frames) {
            _running = false;
        }
    }

    // Started and not over yet.
    [[nodiscard]] auto running() const -> bool { return _running; }
    [[nodiscard]] auto elapsed() const -> int { return _elapsed; }
    // The door is lit this frame (it blinks).
    [[nodiscard]] auto lit() const -> bool {
        return _running && (_elapsed / BLINK_FRAMES) % 2 == 0;
    }

   private:
    int _frames;
    int _elapsed = 0;
    bool _running = false;
};

// Starts at full intensity and fades out linearly.
class Glow {
   public:
    explicit Glow(int frames = GLOW_FRAMES) : _frames(frames) {}

    void start() { _left = _frames; }
    void tick() {
        if (_left > 0) {
            --_left;
        }
    }

    [[nodiscard]] auto active() const -> bool { return _left > 0; }
    // 1 right after start(), 0 when it has faded.
    [[nodiscard]] auto intensity() const -> bn::fixed {
        return bn::fixed(_left) / _frames;
    }

   private:
    int _frames;
    int _left = 0;
};

}  // namespace clear_logic

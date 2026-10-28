#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// crumble
//
// Pure state machine of a crumbling platform (PlatformData::crumble_frames),
// split out of LevelManager so it has no Butano dependency and can be
// unit-tested on the host (see tests/host/test_crumble_logic.cpp).
//
//   Solid   - an ordinary platform until the duck stands on it.
//   Shaking - the duck stood on it: the platform shakes for crumble_frames
//             frames and is still solid. Stepping off does not stop it.
//   Falling - the platform is no longer solid and drops out of the picture
//             for FALL_FRAMES frames (only a visual).
//   Gone    - invisible and not solid. With respawn_frames > 0 it comes back
//             after that many frames, but only once the duck is not inside its
//             box (it would be stuck); with 0 it stays gone until a death or
//             restart resets it.
//
// update() reports what LevelManager has to do with the body and the sprite.
//
// Per-frame protocol (LevelManager::_sync_crumble_platforms()):
//   event = state.update(duck_stands_on_it, duck_overlaps_its_box)
//   Fell      -> disable the body (the duck falls through)
//   Vanished  -> hide the sprite
//   Respawned -> enable body and sprite
//   then sprite offset = (shake_offset(), fall_offset())
// -----------------------------------------------------------------------------

namespace crumble {

// Frames the platform visibly falls after it has stopped being solid.
inline constexpr int FALL_FRAMES = 20;

enum class Phase : uint8_t { Solid, Shaking, Falling, Gone };

enum class Event : uint8_t { None, Started, Fell, Vanished, Respawned };

class State {
   public:
    // crumble_frames: frames from the first contact until the platform gives
    // way (at least 1). respawn_frames: frames until it comes back, 0 = never.
    State(unsigned crumble_frames, unsigned respawn_frames)
        : _crumble(crumble_frames < 1u ? 1u : crumble_frames),
          _respawn(respawn_frames) {}

    auto update(bool standing, bool blocked) -> Event {
        switch (_phase) {
            case Phase::Solid:
                if (standing) {
                    _phase = Phase::Shaking;
                    _counter = _crumble;
                    return Event::Started;
                }
                return Event::None;

            case Phase::Shaking:
                if (--_counter == 0) {
                    _phase = Phase::Falling;
                    _counter = static_cast<unsigned>(FALL_FRAMES);
                    return Event::Fell;
                }
                return Event::None;

            case Phase::Falling:
                if (--_counter == 0) {
                    _phase = Phase::Gone;
                    _counter = _respawn;
                    return Event::Vanished;
                }
                return Event::None;

            case Phase::Gone:
            default:
                if (_respawn == 0) {
                    return Event::None;
                }
                if (_counter > 0) {
                    --_counter;
                }
                // Waits (counter stays 0) while the duck is inside the box.
                if (_counter == 0 && !blocked) {
                    _phase = Phase::Solid;
                    return Event::Respawned;
                }
                return Event::None;
        }
    }

    // Death / Restart Level: an untouched platform again.
    void reset() {
        _phase = Phase::Solid;
        _counter = 0;
    }

    [[nodiscard]] auto phase() const -> Phase { return _phase; }

    // Blocks the duck.
    [[nodiscard]] auto solid() const -> bool {
        return _phase == Phase::Solid || _phase == Phase::Shaking;
    }

    // Drawn at all.
    [[nodiscard]] auto visible() const -> bool { return _phase != Phase::Gone; }

    // Horizontal shake of the sprite in px (-1 / +1 every two frames).
    [[nodiscard]] auto shake_offset() const -> int {
        if (_phase != Phase::Shaking) {
            return 0;
        }
        return (_counter & 2u) != 0u ? 1 : -1;
    }

    // How far the sprite has dropped in px (0 .. ~45, accelerating).
    [[nodiscard]] auto fall_offset() const -> int {
        if (_phase != Phase::Falling) {
            return 0;
        }
        const int elapsed = FALL_FRAMES - static_cast<int>(_counter);
        return (elapsed * elapsed) >> 3;
    }

   private:
    unsigned _crumble;
    unsigned _respawn;
    Phase _phase = Phase::Solid;
    unsigned _counter = 0;
};

}  // namespace crumble

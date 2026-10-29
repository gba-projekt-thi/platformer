#pragma once

#include <cstdint>

#include "cfg_death_fx.h"

// -----------------------------------------------------------------------------
// death_fx
//
// Pure bookkeeping of the death feedback: a short freeze of the world (hitstop)
// on the frame the duck dies, a screen shake that fades out, and a flicker of
// the duck when it reappears. Split out of LevelManager and Player so it has no
// Butano dependency and can be unit-tested on the host (see
// tests/host/test_death_fx.cpp).
//
// What the game does with it:
//   - Sequence says whether the world is frozen and what the shake offset of
//     this frame is. LevelManager skips the simulation while frozen, hands the
//     offset to Camera::set_shake() and ends the freeze when advance() reports
//     it (deferred trap reset, camera cut, the duck comes back).
//   - blink_visible() drives the flicker of the duck after the freeze.
//
// Per-frame protocol (LevelManager::update()):
//   death detected:          start(hitstop, shake)
//   layout changed:          shake(frames)
//   every rendered frame:    shake_offset(amplitude) -> render -> advance()
// -----------------------------------------------------------------------------

namespace death_fx {

enum class Event : uint8_t { None, FreezeEnded };

struct Offset {
    int x;
    int y;
};

// Screen-shake offset of a frame with `frames_left` frames of shake to go
// (`total` when it starts, 0 = no shake). The amplitude fades linearly from
// `amplitude` down to 1px; the direction changes every frame, so it reads as a
// shake rather than a push.
inline auto shake_offset(int frames_left, int total, int amplitude) -> Offset {
    if (frames_left <= 0 || total <= 0 || amplitude <= 0) {
        return {0, 0};
    }
    const int left = frames_left > total ? total : frames_left;
    const int size = (left * amplitude + total - 1) / total;  // ceil
    const int x = (left % 2 != 0) ? size : -size;
    const int y = ((left / 2) % 2 != 0) ? size / 2 : -(size / 2);
    return {x, y};
}

// Flicker of the duck after the freeze: `frames_left` counts down to 0, which
// is visible. Two frames hidden, two shown.
inline auto blink_visible(int frames_left) -> bool {
    return frames_left <= 0 || ((frames_left + 1) / 2) % 2 == 0;
}

// The duck's side of the feedback, without the sprites: while the world is
// frozen it is replaced by its ghost (begin()), and when the freeze ends (or
// is cancelled) it is shown again - after end() with a flicker of
// `blink_frames` physics frames. Player acts on what these report.
class DuckFx {
   public:
    enum class Blink : uint8_t { Idle, Show, Hide };

    // The duck dies into a freeze: ghost up, duck hidden.
    void begin() {
        _active = true;
        _blink = 0;
    }

    // The freeze ended: true = show the duck again and drop the ghost; the
    // flicker follows. False when no freeze was running.
    auto end(int blink_frames) -> bool {
        if (!cancel()) {
            return false;
        }
        _blink = blink_frames;
        return true;
    }

    // The freeze ended: the flicker of Cfg::DeathFx::RESPAWN_BLINK_FRAMES.
    auto end() -> bool { return end(Cfg::DeathFx::RESPAWN_BLINK_FRAMES); }

    // Like end() without the flicker (level load / unload, Restart Level).
    auto cancel() -> bool {
        if (!_active) {
            return false;
        }
        _active = false;
        return true;
    }

    // The game shows or hides the duck itself (a menu): no flicker on top.
    void stop_blink() { _blink = 0; }

    // One physics frame. Only counts while the duck is shown at all; says
    // what to do with its sprite this frame.
    auto tick(bool shown) -> Blink {
        if (_blink <= 0 || !shown) {
            return Blink::Idle;
        }
        --_blink;
        return blink_visible(_blink) ? Blink::Show : Blink::Hide;
    }

   private:
    bool _active = false;
    int _blink = 0;
};

// The duck dies: where it was is remembered for the ghost, and it is moved
// back to the restart point (the teleport would overwrite it).
template <typename T>
void leave_death_spot(
    T& pos_x,
    T& pos_y,
    T restart_x,
    T restart_y,
    T& death_x,
    T& death_y) {
    death_x = pos_x;
    death_y = pos_y;
    pos_x = restart_x;
    pos_y = restart_y;
}

class Sequence {
   public:
    // A death: freeze for `hitstop_frames` (at least 1) rendered frames, shake
    // for `shake_frames`. A new death restarts both.
    void start(int hitstop_frames, int shake_frames) {
        _hitstop_left = hitstop_frames < 1 ? 1 : hitstop_frames;
        _shake_total = shake_frames < 0 ? 0 : shake_frames;
        _shake_left = _shake_total;
    }

    // A shake on its own (the cue that the layout changed between attempts):
    // the world keeps running, a running freeze is left as it is.
    void shake(int shake_frames) {
        _shake_total = shake_frames < 0 ? 0 : shake_frames;
        _shake_left = _shake_total;
    }

    // Level load / unload: nothing running any more.
    void clear() {
        _hitstop_left = 0;
        _shake_left = 0;
        _shake_total = 0;
    }

    // The world stands still: no simulation, no camera movement.
    [[nodiscard]] auto frozen() const -> bool { return _hitstop_left > 0; }

    [[nodiscard]] auto shaking() const -> bool { return _shake_left > 0; }

    [[nodiscard]] auto shake_offset(int amplitude) const -> Offset {
        return death_fx::shake_offset(_shake_left, _shake_total, amplitude);
    }

    // Call once per rendered frame, after rendering. Reports the frame on which
    // the freeze ends.
    auto advance() -> Event {
        Event event = Event::None;
        if (_shake_left > 0) {
            --_shake_left;
        }
        if (_hitstop_left > 0) {
            --_hitstop_left;
            if (_hitstop_left == 0) {
                event = Event::FreezeEnded;
            }
        }
        return event;
    }

   private:
    int _hitstop_left = 0;
    int _shake_left = 0;
    int _shake_total = 0;
};

}  // namespace death_fx

#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// TriggerMode / TriggerState
//
// Pure state logic of a Trigger, split out of the Butano-dependent Trigger
// class so it can be unit-tested on the host (see
// tests/host/test_trigger_logic.cpp).
//
//   Latch  - the original behavior and the default for every existing level:
//            entering the zone sets the state to !default_on and it stays
//            there until reset() (death/restart). Leaving does nothing.
//   Toggle - every ENTRY into the zone flips the state (a switch). Standing
//            inside does not flip it again; leaving does nothing.
//   Hold   - on only while the player is inside: entering sets !default_on,
//            leaving restores default_on (a pressure plate).
//   Timer  - ignores the player completely: !default_on is set by itself
//            `delay_frames` frames after the level (re)starts and stays until
//            the next reset(). A trap bound to it starts on its own.
//
// Delay: with delay_frames > 0 the state seen by traps and platforms
// (triggered()) follows the state the player caused (pressed()) only after
// delay_frames frames, in both directions. A change that is undone before the
// delay has elapsed never shows (the lever flips back in time). A visible
// switch sprite follows pressed(), so the player sees the lever flip at once
// and only feels the trap later. delay_frames has no extra effect on Timer
// triggers - it is their timer length.
//
// Action: what makes the duck's presence count as an "entry". Enter (default)
// is the original behavior; Jump and Dash fire while the duck is inside the
// zone and jumps or dashes (see action_fires()). Everything above (modes,
// delay) works the same on top of it; a Hold plate with Jump is pressed by a
// jump inside it and released when the duck leaves.
//
// In every mode reset() restores default_on.
// -----------------------------------------------------------------------------

enum class TriggerMode : uint8_t { Latch, Toggle, Hold, Timer };

enum class TriggerAction : uint8_t { Enter, Jump, Dash };

// Whether an action trigger fires this frame. `inside`: the duck overlaps the
// zone. `jumped` / `dashed`: it started a jump / a dash this frame (see
// PlayerEvents). Enter triggers fire from the overlap callback instead, never
// from here.
inline auto
action_fires(TriggerAction action, bool inside, bool jumped, bool dashed)
    -> bool {
    switch (action) {
        case TriggerAction::Jump:
            return inside && jumped;
        case TriggerAction::Dash:
            return inside && dashed;
        case TriggerAction::Enter:
        default:
            return false;
    }
}

class TriggerState {
   public:
    TriggerState(bool default_on, TriggerMode mode, unsigned delay_frames = 0)
        : _effective(default_on),
          _pressed(default_on),
          _default_on(default_on),
          _mode(mode),
          _delay(delay_frames) {}

    // The state traps and switched platforms follow (after the delay).
    [[nodiscard]] auto triggered() const -> bool { return _effective; }

    // The state the player caused, ahead of triggered() by the delay. What a
    // visible switch shows.
    [[nodiscard]] auto pressed() const -> bool { return _pressed; }

    // A change of pressed() is on its way to triggered().
    [[nodiscard]] auto pending() const -> bool { return _countdown > 0; }

    void on_enter() {
        if (_mode == TriggerMode::Timer) {
            return;
        }
        if (_mode == TriggerMode::Toggle) {
            _set_pressed(!_pressed);
        } else {
            // Latch and Hold: same entry behavior as the legacy Trigger.
            _set_pressed(!_default_on);
        }
    }

    void on_exit() {
        if (_mode == TriggerMode::Hold) {
            _set_pressed(_default_on);
        }
    }

    // One frame (Trigger::update()): runs the delay and the timer.
    void tick() {
        if (_mode == TriggerMode::Timer) {
            if (_pressed == _default_on) {
                ++_clock;
                if (_clock >= _delay) {
                    _pressed = !_default_on;
                    _effective = _pressed;
                }
            }
            return;
        }
        if (_countdown > 0) {
            --_countdown;
            if (_countdown == 0) {
                _effective = _pressed;
            }
        }
    }

    void reset() {
        _pressed = _default_on;
        _effective = _default_on;
        _countdown = 0;
        _clock = 0;
    }

    // Puts the state back to `triggered` (a checkpoint's snapshot, see
    // checkpoint_logic.h) without any delay. Hold triggers are left alone:
    // they are only on while the duck stands in them and would stay on
    // forever, because the exit that turns them off never comes. A Timer
    // trigger that had fired stays fired, one that had not starts counting
    // again from the respawn.
    void restore(bool triggered) {
        if (_mode == TriggerMode::Hold) {
            return;
        }
        _pressed = triggered;
        _effective = triggered;
        _countdown = 0;
        _clock = (_mode == TriggerMode::Timer && triggered != _default_on)
                     ? _delay
                     : 0;
    }

   private:
    void _set_pressed(bool pressed) {
        _pressed = pressed;
        if (_delay == 0) {
            _effective = pressed;
            _countdown = 0;
        } else if (pressed == _effective) {
            _countdown = 0;  // changed back in time: nothing to follow
        } else {
            _countdown = _delay;
        }
    }

    bool _effective;
    bool _pressed;
    bool _default_on;
    TriggerMode _mode;
    unsigned _delay;
    unsigned _countdown = 0;
    unsigned _clock = 0;
};

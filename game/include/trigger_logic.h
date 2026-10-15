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
//
// In every mode reset() restores default_on.
// -----------------------------------------------------------------------------

enum class TriggerMode : uint8_t { Latch, Toggle, Hold };

class TriggerState {
   public:
    TriggerState(bool default_on, TriggerMode mode)
        : _triggered(default_on), _default_on(default_on), _mode(mode) {}

    [[nodiscard]] auto triggered() const -> bool { return _triggered; }

    void on_enter() {
        if (_mode == TriggerMode::Toggle) {
            _triggered = !_triggered;
        } else {
            // Latch and Hold: same entry behavior as the legacy Trigger.
            _triggered = !_default_on;
        }
    }

    void on_exit() {
        if (_mode == TriggerMode::Hold) {
            _triggered = _default_on;
        }
    }

    void reset() { _triggered = _default_on; }

    // Puts the state back to `triggered` (a checkpoint's snapshot, see
    // checkpoint_logic.h). Hold triggers are left alone: they are only on
    // while the duck stands in them and would stay on forever, because the
    // exit that turns them off never comes.
    void restore(bool triggered) {
        if (_mode != TriggerMode::Hold) {
            _triggered = triggered;
        }
    }

   private:
    bool _triggered;
    bool _default_on;
    TriggerMode _mode;
};

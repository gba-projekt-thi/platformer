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
// Follower (TriggerData::follows): a trigger in a delayed trap chain. It
// ignores the duck completely (and its own mode) and is pressed by another
// trigger instead: follow(true) when the source is activated (its triggered()
// differs from its default_on), follow(false) when it is back at its default.
// Its delay_frames run as usual on top. TriggerChains below hands the
// sources' state to the followers.
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
    // `follower`: pressed by its source (follow()), never by the duck; its
    // mode is then ignored.
    TriggerState(
        bool default_on,
        TriggerMode mode,
        unsigned delay_frames = 0,
        bool follower = false)
        : _effective(default_on),
          _pressed(default_on),
          _default_on(default_on),
          _mode(follower ? TriggerMode::Latch : mode),
          _delay(delay_frames),
          _follower(follower) {}

    // The state traps and switched platforms follow (after the delay).
    [[nodiscard]] auto triggered() const -> bool { return _effective; }

    // The state the player caused, ahead of triggered() by the delay. What a
    // visible switch shows.
    [[nodiscard]] auto pressed() const -> bool { return _pressed; }

    // A change of pressed() is on its way to triggered().
    [[nodiscard]] auto pending() const -> bool { return _countdown > 0; }

    void on_enter() {
        if (_follower || _mode == TriggerMode::Timer) {
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

    // A follower's source was activated (or still is) / is back at its
    // default: pressed() becomes !default_on / default_on. Called every frame
    // with the source's state; only a change starts the delay. No effect on a
    // trigger that is not a follower.
    void follow(bool source_on) {
        if (!_follower) {
            return;
        }
        const bool pressed = source_on ? !_default_on : _default_on;
        if (pressed != _pressed) {
            _set_pressed(pressed);
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
    bool _follower;
    unsigned _countdown = 0;
    unsigned _clock = 0;
};

// -----------------------------------------------------------------------------
// TriggerZone
//
// A Trigger without Butano: the state plus what the duck does in the zone.
// The overlap callbacks call enter() / exit(), Trigger::update() calls
// update() once per frame with the jump / dash PlayerEvents report. An Enter
// trigger is pressed by enter(); a Jump / Dash trigger only by update()
// (action_fires()), whatever the mode.
// -----------------------------------------------------------------------------
class TriggerZone {
   public:
    TriggerZone(
        bool default_on,
        TriggerMode mode,
        unsigned delay_frames,
        TriggerAction action,
        bool follower = false)
        : _state(default_on, mode, delay_frames, follower), _action(action) {}

    [[nodiscard]] auto triggered() const -> bool { return _state.triggered(); }
    [[nodiscard]] auto pressed() const -> bool { return _state.pressed(); }

    // The duck overlaps the zone.
    void enter() {
        _inside = true;
        if (_action == TriggerAction::Enter) {
            _state.on_enter();
        }
    }

    void exit() {
        _inside = false;
        _state.on_exit();
    }

    // One frame: runs the delay and the timer, then the duck's action.
    void update(bool jumped, bool dashed) {
        _state.tick();
        if (action_fires(_action, _inside, jumped, dashed)) {
            _state.on_enter();
        }
    }

    void follow(bool source_on) { _state.follow(source_on); }
    void reset() { _state.reset(); }
    void restore(bool triggered) { _state.restore(triggered); }

   private:
    TriggerState _state;
    TriggerAction _action;
    bool _inside = false;
};

// -----------------------------------------------------------------------------
// TriggerChains
//
// Which trigger follows which (TriggerData::follows, resolved to indexes) and
// the once-per-frame hand-over from the sources to the followers. A follower
// follows whether its source is ACTIVATED - triggered() differs from the
// source's default_on - not the source's raw state, so the follower of a
// source that starts on stays quiet at load and fires when the duck switches
// the source off. Works on any
// container with size() and operator[] whose elements have is_triggered() and
// follow(bool) (Trigger in the game, a stand-in in
// tests/host/test_trigger_chain.cpp).
//
// Timing: propagate() runs once per frame, after every trigger has ticked and
// the duck has pressed / left its triggers. It hands over the sources' state
// as it was at the end of the PREVIOUS frame, so every hop costs exactly one
// frame (plus the follower's delay_frames), whether the source is listed
// before or after the follower and however long the chain is: a follower is
// pressed as if the duck had stepped into it one frame after its source's
// triggered() changed.
//
// After the triggers are reset or restored (death, checkpoint respawn, level
// load) capture() takes their state as the new "previous frame", so nothing
// from before the reset reaches a follower. The first capture() after
// clear() is the level load: every trigger is still at its default_on then,
// and that state is kept as the triggers' defaults (the elements need no
// default_on() of their own).
//
// A cycle of followers never fires: at load nobody in it is activated, and
// nothing outside the cycle can start it.
// -----------------------------------------------------------------------------
class TriggerChains {
   public:
    // Matches Cfg::Level::Limits::TRIGGERS (checked in LevelManager).
    static constexpr int MAX_TRIGGERS = 16;
    static constexpr int8_t NO_SOURCE = -1;

    TriggerChains() {
        for (int8_t& source : _source) {
            source = NO_SOURCE;
        }
    }

    // Trigger `follower` follows trigger `source`. Out-of-range indexes and a
    // trigger following itself are ignored.
    void link(int follower, int source) {
        if (follower < 0 || follower >= MAX_TRIGGERS || source < 0 ||
            source >= MAX_TRIGGERS || follower == source) {
            return;
        }
        _source[follower] = static_cast<int8_t>(source);
    }

    // Level load: no trigger follows any other.
    void clear() { *this = TriggerChains(); }

    // Takes the triggers' current state as the previous frame's; the first
    // call after clear() (the level load) also takes it as their defaults.
    template <typename Triggers>
    void capture(const Triggers& triggers) {
        const int count = _count(triggers);
        if (!_defaults_known) {
            for (int i = 0; i < count; ++i) {
                _default_on[i] = triggers[i].is_triggered();
            }
            _defaults_known = true;
        }
        for (int i = 0; i < count; ++i) {
            _was_active[i] = triggers[i].is_triggered() != _default_on[i];
        }
    }

    // A death or checkpoint respawn: `restore` puts the triggers back (their
    // default state, or the checkpoint's snapshot), and only then is their
    // state taken as the previous frame - the other way round, a source that
    // was activated before the death would still press its follower.
    template <typename Triggers, typename Restore>
    void respawn(Triggers& triggers, Restore&& restore) {
        restore(triggers);
        capture(triggers);
    }

    // Once per frame: every follower follows whether its source was
    // activated in the previous frame, then the current state is taken for
    // the next frame.
    template <typename Triggers>
    void propagate(Triggers& triggers) {
        const int count = _count(triggers);
        for (int i = 0; i < count; ++i) {
            const int source = _source[i];
            if (source != NO_SOURCE && source < count) {
                triggers[i].follow(_was_active[source]);
            }
        }
        capture(triggers);
    }

   private:
    template <typename Triggers>
    static auto _count(const Triggers& triggers) -> int {
        const int size = static_cast<int>(triggers.size());
        return size < MAX_TRIGGERS ? size : MAX_TRIGGERS;
    }

    int8_t _source[MAX_TRIGGERS];
    bool _default_on[MAX_TRIGGERS] = {};
    bool _defaults_known = false;
    bool _was_active[MAX_TRIGGERS] = {};
};

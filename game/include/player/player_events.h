#pragma once

// -----------------------------------------------------------------------------
// PlayerEvents
//
// One-frame notifications of what the duck just did, for things that react to
// an action instead of a position (triggers with TriggerAction::Jump / Dash).
// Header-only singleton in the style of GameSession; Butano-free, so it is
// unit-tested on the host (see tests/host/test_player_events.cpp).
//
// PlayerLocomotion calls begin_frame() at the start of its update and
// note_jump() / note_dash() when the jump or the dash actually starts. A reader
// sees an event during the update of the same frame (if it runs after the
// player) or of the next one (if it runs before the player has cleared it), so
// every event is seen exactly once, whatever the order of the collision shapes
// in CollisionRegistry. Level load calls clear().
// -----------------------------------------------------------------------------
class PlayerEvents {
   public:
    static auto instance() -> PlayerEvents& {
        static PlayerEvents events;
        return events;
    }

    // Start of the player's update: events of the previous frame are over.
    void begin_frame() {
        _jumped = false;
        _dashed = false;
    }

    void note_jump() { _jumped = true; }
    void note_dash() { _dashed = true; }

    void clear() { begin_frame(); }

    [[nodiscard]] auto jumped() const -> bool { return _jumped; }
    [[nodiscard]] auto dashed() const -> bool { return _dashed; }

   private:
    PlayerEvents() = default;

    bool _jumped = false;
    bool _dashed = false;
};

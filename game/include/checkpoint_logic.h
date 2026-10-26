#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// checkpoint_logic
//
// Pure rules of the level checkpoints (LevelData::checkpoints), split out of
// LevelManager so they have no Butano dependency and can be unit-tested on the
// host (see tests/host/test_checkpoint_logic.cpp).
//
// * A checkpoint is reached by touching it. The duck then respawns there
//   instead of at the level's spawn point - until the level is restarted from
//   the pause menu or loaded again.
// * Checkpoints are listed in the level data in the order the level is played,
//   so reaching one only counts when it lies FURTHER along than the active one
//   (CheckpointTracker): walking back to an earlier flag never loses progress.
// * Everything a death resets (traps, triggers) is reset as always; the only
//   thing a checkpoint adds is a snapshot of which triggers were ON when it
//   was reached (a 16-bit mask, one bit per trigger index, see
//   Cfg::Level::Limits::TRIGGERS). After a respawn those triggers are switched
//   on again, so a boss patrol or an already released attack keeps running
//   instead of staying dormant behind the checkpoint.
// -----------------------------------------------------------------------------

class CheckpointTracker {
   public:
    static constexpr int NONE = -1;

    // Registers that checkpoint `index` was touched. Returns true when it
    // becomes the active one (further along than the current one).
    auto reach(int index) -> bool {
        if (index <= _active) {
            return false;
        }
        _active = index;
        return true;
    }

    // Level (re)load or Restart Level: no checkpoint is active any more.
    void reset() { _active = NONE; }

    [[nodiscard]] auto active() const -> int { return _active; }
    [[nodiscard]] auto has_active() const -> bool { return _active != NONE; }

   private:
    int _active = NONE;
};

inline constexpr int MAX_MASK_BITS = 16;

// Bit i of the result is flags[i]; flags beyond MAX_MASK_BITS are ignored.
inline auto pack_flags(const bool* flags, int count) -> uint16_t {
    uint16_t mask = 0;
    for (int i = 0; i < count && i < MAX_MASK_BITS; ++i) {
        if (flags[i]) {
            mask = static_cast<uint16_t>(mask | (1u << i));
        }
    }
    return mask;
}

inline auto flag_at(uint16_t mask, int index) -> bool {
    return index >= 0 && index < MAX_MASK_BITS && ((mask >> index) & 1u) != 0;
}

// -----------------------------------------------------------------------------
// CheckpointRun
//
// The checkpoint state of one attempt at a level, as LevelManager applies it:
// which flag is active (CheckpointTracker) and which triggers were on when it
// was reached. Works on any containers with size() and operator[] (bn::vector
// in the game), host-tested with stand-ins
// (tests/host/test_checkpoint_run.cpp):
//   checkpoint: take_hit() (a touch, reported once), set_active(bool)
//   trigger:    is_triggered(), restore(bool)
// -----------------------------------------------------------------------------
class CheckpointRun {
   public:
    // Takes this frame's touches. A flag further along than the active one
    // becomes the only active flag and the triggers that are on are
    // remembered. Returns its index (the new respawn point), or -1.
    template <typename Checkpoints, typename Triggers>
    auto sync(Checkpoints& checkpoints, const Triggers& triggers) -> int {
        int reached = -1;
        for (int i = 0; i < int(checkpoints.size()); ++i) {
            if (!checkpoints[i].take_hit() || !_tracker.reach(i)) {
                continue;
            }
            reached = i;
            _triggers_on = 0;
            for (int t = 0; t < int(triggers.size()) && t < MAX_MASK_BITS;
                 ++t) {
                if (triggers[t].is_triggered()) {
                    _triggers_on =
                        static_cast<uint16_t>(_triggers_on | (1u << t));
                }
            }
            for (int c = 0; c < int(checkpoints.size()); ++c) {
                checkpoints[c].set_active(c == _tracker.active());
            }
        }
        return reached;
    }

    // After a death reset: switches the remembered triggers on again.
    template <typename Triggers>
    void restore(Triggers& triggers) const {
        if (!_tracker.has_active()) {
            return;
        }
        for (int t = 0; t < int(triggers.size()); ++t) {
            if (flag_at(_triggers_on, t)) {
                triggers[t].restore(true);
            }
        }
    }

    // Restart Level / level load: no checkpoint, every flag inactive.
    template <typename Checkpoints>
    void clear(Checkpoints& checkpoints) {
        _tracker.reset();
        _triggers_on = 0;
        for (int c = 0; c < int(checkpoints.size()); ++c) {
            checkpoints[c].set_active(false);
        }
    }

    [[nodiscard]] auto has_active() const -> bool {
        return _tracker.has_active();
    }

   private:
    CheckpointTracker _tracker;
    uint16_t _triggers_on = 0;
};

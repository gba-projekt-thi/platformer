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

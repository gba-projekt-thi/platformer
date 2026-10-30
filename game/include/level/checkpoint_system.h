#pragma once

#include "checkpoint_logic.h"

// -----------------------------------------------------------------------------
// CheckpointSystem
//
// The checkpoints of a level and where the duck respawns: the flags
// (LevelData::checkpoints), the run's state (CheckpointRun) and the level's own
// spawn point to fall back to. LevelManager creates the picture and plays the
// sound; this decides. Templated over the game's types so it is host-tested
// with stand-ins (tests/host/test_checkpoint_system.cpp):
//   Checkpoints: container with emplace_back(x, y), clear(), size(),
//                operator[] of a checkpoint (see CheckpointRun) with pos.x /
//                pos.y
//   Coord:       the type of a coordinate (bn::fixed)
//   Player:      set_spawn_point(x, y)
// -----------------------------------------------------------------------------
template <typename Checkpoints, typename Coord>
class CheckpointSystem {
   public:
    // The level's own spawn point (LevelData::player_data), set on every load.
    void set_level_spawn(Coord x, Coord y) {
        _spawn_x = x;
        _spawn_y = y;
    }

    // One inactive flag per LevelData::checkpoints entry.
    template <typename Level>
    void load(const Level& level) {
        for (int i = 0; i < level.checkpoint_count; ++i) {
            _checkpoints.emplace_back(
                level.checkpoints[i].x, level.checkpoints[i].y);
            _checkpoints[i].set_active(false);
        }
    }

    // Takes this frame's touches; a newly reached flag becomes the respawn
    // point. True then (LevelManager plays the cue).
    template <typename Triggers, typename Player>
    auto sync(const Triggers& triggers, Player& player) -> bool {
        const int reached = _run.sync(_checkpoints, triggers);
        if (reached < 0) {
            return false;
        }
        player.set_spawn_point(
            _checkpoints[reached].pos.x, _checkpoints[reached].pos.y);
        return true;
    }

    // Restart Level: no checkpoint, respawn at the level spawn.
    template <typename Player>
    void clear(Player& player) {
        _run.clear(_checkpoints);
        player.set_spawn_point(_spawn_x, _spawn_y);
    }

    // After a death reset: the triggers that were on at the flag are on again.
    template <typename Triggers>
    void restore_triggers(Triggers& triggers) const {
        _run.restore(triggers);
    }

    // The trigger the snapshot never keeps (CheckpointRun::forget_trigger()).
    void forget_trigger(int index) { _run.forget_trigger(index); }

    // Level unload, in the steps level_flow.h gives: the flags, then the run.
    void clear_flags() { _checkpoints.clear(); }
    void forget_run() { _run = CheckpointRun(); }

    [[nodiscard]] auto has_active() const -> bool { return _run.has_active(); }

   private:
    Checkpoints _checkpoints;
    CheckpointRun _run;
    Coord _spawn_x = 0;
    Coord _spawn_y = 0;
};

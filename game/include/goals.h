#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// goals
//
// The two goals of a level, shown in the level select:
//   - "No death": cleared once without dying (the no-death badge),
//   - "Under X seconds": a personal best within LevelData::target_time_s.
// Both take the saved values (best time, badge mask) instead of a GameState,
// so a second set of levels with its own arrays can use them as well.
// Nothing is recorded in Assist Mode (record_clear), so neither goal can be
// met there. Host-tested in tests/host/test_goals.cpp.
// -----------------------------------------------------------------------------
namespace goals {

inline constexpr uint32_t FRAMES_PER_SECOND = 60;

[[nodiscard]] inline auto time_goal_met(
    uint32_t best_time_frames,
    int target_time_s) -> bool {
    return target_time_s > 0 && best_time_frames > 0 &&
           best_time_frames <= uint32_t(target_time_s) * FRAMES_PER_SECOND;
}

[[nodiscard]] inline auto no_death_goal_met(
    uint32_t no_death_mask,
    unsigned level_index) -> bool {
    return level_index < 32 &&
           (no_death_mask & (uint32_t(1) << level_index)) != 0;
}

}  // namespace goals

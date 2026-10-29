#pragma once

#include <cstdint>

#include "frame_time.h"
#include "game_state.h"
#include "text_buffer.h"
#include "world_index.h"

// -----------------------------------------------------------------------------
// statistics
//
// The numbers behind the statistics screen (StatisticsScene): what is counted
// into the save while playing (deaths per level, play time) and how the save
// is read back into the lines of the screen. Deaths and play time outlive a
// finished game (DataManager::reset()), like the records. Butano-free,
// unit-tested on the host (tests/host/test_statistics.cpp).
// -----------------------------------------------------------------------------
namespace statistics {

inline constexpr int LEVELS = WorldIndex::LEVEL_COUNT;
static_assert(LEVELS <= GameState::MAX_LEVELS);

// Lines of the screen: five totals, a heading, one line per level.
inline constexpr int SUMMARY_LINES = 5;
inline constexpr int LINE_COUNT = SUMMARY_LINES + 1 + LEVELS;
// Lines shown at once.
inline constexpr int VISIBLE_LINES = 7;
inline constexpr int LINE_CHARS = 28;

inline void record_death(GameState& state, unsigned level_index) {
    if (level_index < unsigned(LEVELS) &&
        state.level_deaths[level_index] < UINT16_MAX) {
        ++state.level_deaths[level_index];
    }
}

inline void tick_play_time(GameState& state) {
    if (state.play_frames < UINT32_MAX) {
        ++state.play_frames;
    }
}

[[nodiscard]] inline auto total_deaths(const GameState& state) -> uint32_t {
    uint32_t sum = 0;
    for (int i = 0; i < LEVELS; ++i) {
        sum += state.level_deaths[i];
    }
    return sum;
}

// Levels with a best time, i.e. cleared at least once.
[[nodiscard]] inline auto levels_cleared(const GameState& state) -> int {
    int count = 0;
    for (int i = 0; i < LEVELS; ++i) {
        count += state.best_time_frames[i] != 0 ? 1 : 0;
    }
    return count;
}

[[nodiscard]] inline auto no_death_badges(const GameState& state) -> int {
    int count = 0;
    for (int i = 0; i < LEVELS; ++i) {
        count += has_no_death_clear(state, unsigned(i)) ? 1 : 0;
    }
    return count;
}

// The level with the most deaths (the first of equals); -1 without a death.
[[nodiscard]] inline auto deadliest_level(const GameState& state) -> int {
    int best = -1;
    for (int i = 0; i < LEVELS; ++i) {
        if (state.level_deaths[i] > 0 &&
            (best < 0 || state.level_deaths[i] > state.level_deaths[best])) {
            best = i;
        }
    }
    return best;
}

// "H:MM:SS"; the hours are not capped.
inline void append_play_time(TextBuffer<LINE_CHARS>& out, uint32_t frames) {
    const FrameTime time = frames_to_time(frames);
    out.append_uint(time.minutes / 60)
        .append(':')
        .append_uint(time.minutes % 60, 2)
        .append(':')
        .append_uint(time.seconds, 2);
}

// "1-3": the world and the level's number in it (both counted from 1).
inline void append_level_name(TextBuffer<LINE_CHARS>& out, int level_index) {
    const int world = WorldIndex::world_for_level(level_index);
    out.append_uint(unsigned(world + 1))
        .append('-')
        .append_uint(
            unsigned(level_index - WorldIndex::WORLDS[world].start_index + 1));
}

// Line `index` (0..LINE_COUNT-1) of the screen.
[[nodiscard]] inline auto line(const GameState& state, int index)
    -> TextBuffer<LINE_CHARS> {
    TextBuffer<LINE_CHARS> out;
    switch (index) {
        case 0:
            out.append("Deaths: ").append_uint(total_deaths(state));
            break;
        case 1:
            out.append("Play time: ");
            append_play_time(out, state.play_frames);
            break;
        case 2:
            out.append("Cleared: ")
                .append_uint(unsigned(levels_cleared(state)))
                .append('/')
                .append_uint(LEVELS);
            break;
        case 3:
            out.append("No-death clears: ")
                .append_uint(unsigned(no_death_badges(state)));
            break;
        case 4: {
            out.append("Deadliest: ");
            const int level = deadliest_level(state);
            if (level < 0) {
                out.append("none");
            } else {
                append_level_name(out, level);
                out.append(" (")
                    .append_uint(state.level_deaths[level])
                    .append(')');
            }
            break;
        }
        case SUMMARY_LINES:
            out.append("Deaths per level");
            break;
        default: {
            const int level = index - SUMMARY_LINES - 1;
            if (level >= 0 && level < LEVELS) {
                append_level_name(out, level);
                out.append(":  ").append_uint(state.level_deaths[level]);
            }
            break;
        }
    }
    return out;
}

// The first shown line after moving by `delta` lines, kept inside the list.
[[nodiscard]] inline auto scroll(int top, int delta) -> int {
    const int next = top + delta;
    const int max_top = LINE_COUNT - VISIBLE_LINES;
    return next < 0 ? 0 : (next > max_top ? max_top : next);
}

}  // namespace statistics

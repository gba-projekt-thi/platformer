#include "test_framework.h"

#include <cstring>

#include "game_state.h"
#include "statistics.h"

namespace {
auto text(const GameState& state, int index)
    -> TextBuffer<statistics::LINE_CHARS> {
    return statistics::line(state, index);
}
}  // namespace

TEST_CASE(statistics_a_death_counts_for_its_level) {
    GameState state{};
    statistics::record_death(state, 3);
    statistics::record_death(state, 3);
    statistics::record_death(state, 7);
    CHECK_EQ(int(state.level_deaths[3]), 2);
    CHECK_EQ(int(state.level_deaths[7]), 1);
    CHECK_EQ(statistics::total_deaths(state), 3u);
}

TEST_CASE(statistics_deaths_in_assist_mode_do_not_count) {
    GameState state{};
    state.assist_mode_enabled = true;
    statistics::record_death(state, 3);
    CHECK_EQ(statistics::total_deaths(state), 0u);
}

TEST_CASE(statistics_death_counts_stop_at_their_limit) {
    GameState state{};
    state.level_deaths[0] = UINT16_MAX;
    statistics::record_death(state, 0);
    CHECK_EQ(int(state.level_deaths[0]), int(UINT16_MAX));
    statistics::record_death(state, 99);  // beyond the roster: ignored
    CHECK_EQ(statistics::total_deaths(state), uint32_t(UINT16_MAX));
}

TEST_CASE(statistics_play_time_counts_frames_and_stops_at_the_limit) {
    GameState state{};
    statistics::tick_play_time(state);
    statistics::tick_play_time(state);
    CHECK_EQ(state.play_frames, 2u);
    state.play_frames = UINT32_MAX;
    statistics::tick_play_time(state);
    CHECK_EQ(state.play_frames, UINT32_MAX);
}

TEST_CASE(statistics_cleared_levels_and_badges_are_counted) {
    GameState state{};
    state.best_time_frames[0] = 100;
    state.best_time_frames[5] = 100;
    set_no_death_clear(state, 0);
    CHECK_EQ(statistics::levels_cleared(state), 2);
    CHECK_EQ(statistics::no_death_badges(state), 1);
}

TEST_CASE(statistics_deadliest_level_is_the_first_with_the_most_deaths) {
    GameState state{};
    CHECK_EQ(statistics::deadliest_level(state), -1);
    state.level_deaths[4] = 6;
    state.level_deaths[9] = 6;
    state.level_deaths[2] = 3;
    CHECK_EQ(statistics::deadliest_level(state), 4);
    state.level_deaths[9] = 7;
    CHECK_EQ(statistics::deadliest_level(state), 9);
}

TEST_CASE(statistics_play_time_reads_hours_minutes_seconds) {
    TextBuffer<statistics::LINE_CHARS> out;
    statistics::append_play_time(out, 0);
    CHECK(std::strcmp(out.c_str(), "0:00:00") == 0);
    out.clear();
    statistics::append_play_time(out, (3600u + 2 * 60 + 5) * 60 + 59);
    CHECK(std::strcmp(out.c_str(), "1:02:05") == 0);
    out.clear();
    statistics::append_play_time(out, 120u * 3600 * 60);
    CHECK(std::strcmp(out.c_str(), "120:00:00") == 0);
}

TEST_CASE(statistics_level_names_count_worlds_and_levels_from_one) {
    TextBuffer<statistics::LINE_CHARS> out;
    statistics::append_level_name(out, 0);
    CHECK(std::strcmp(out.c_str(), "1-1") == 0);
    out.clear();
    statistics::append_level_name(out, 5);  // World 2's first
    CHECK(std::strcmp(out.c_str(), "2-1") == 0);
    out.clear();
    statistics::append_level_name(out, statistics::LEVELS - 1);
    CHECK(std::strcmp(out.c_str(), "5-8") == 0);
}

TEST_CASE(statistics_lines_show_the_totals) {
    GameState state{};
    state.level_deaths[1] = 12;
    state.level_deaths[6] = 30;
    state.play_frames = 61 * 60;
    state.best_time_frames[0] = 1;
    state.best_time_frames[1] = 1;
    set_no_death_clear(state, 0);
    CHECK(std::strcmp(text(state, 0).c_str(), "Deaths: 42") == 0);
    CHECK(std::strcmp(text(state, 1).c_str(), "Play time: 0:01:01") == 0);
    CHECK(std::strcmp(text(state, 2).c_str(), "Cleared: 2/26") == 0);
    CHECK(std::strcmp(text(state, 3).c_str(), "No-death clears: 1") == 0);
    CHECK(std::strcmp(text(state, 4).c_str(), "Deadliest: 2-2 (30)") == 0);
}

TEST_CASE(statistics_without_a_death_nothing_is_deadliest) {
    GameState state{};
    CHECK(std::strcmp(text(state, 4).c_str(), "Deadliest: none") == 0);
}

TEST_CASE(statistics_lines_list_every_level_after_the_heading) {
    GameState state{};
    state.level_deaths[0] = 3;
    state.level_deaths[statistics::LEVELS - 1] = 8;
    CHECK(
        std::strcmp(
            text(state, statistics::SUMMARY_LINES).c_str(),
            "Deaths per level") == 0);
    CHECK(std::strcmp(text(state, 6).c_str(), "1-1:  3") == 0);
    CHECK(
        std::strcmp(
            text(state, statistics::LINE_COUNT - 1).c_str(), "5-8:  8") == 0);
}

TEST_CASE(statistics_longest_lines_fit_their_buffer) {
    GameState state{};
    state.play_frames = UINT32_MAX;
    for (int i = 0; i < statistics::LEVELS; ++i) {
        state.level_deaths[i] = UINT16_MAX;
        state.best_time_frames[i] = 1;
        set_no_death_clear(state, unsigned(i));
    }
    for (int i = 0; i < statistics::LINE_COUNT; ++i) {
        CHECK(text(state, i).size() < statistics::LINE_CHARS - 1);
    }
}

TEST_CASE(statistics_any_screenful_stays_within_the_sprite_budget) {
    // One sprite per glyph; the title ("< Slot 3 stats >") and the hint
    // ("D-pad: scroll  B: back") take 13 + 18 of the 128.
    GameState state{};
    state.play_frames = UINT32_MAX;
    for (int i = 0; i < statistics::LEVELS; ++i) {
        state.level_deaths[i] = 9999;
        state.best_time_frames[i] = 1;
        set_no_death_clear(state, unsigned(i));
    }
    int worst = 0;
    const int max_top = statistics::LINE_COUNT - statistics::VISIBLE_LINES;
    for (int top = 0; top <= max_top; ++top) {
        int glyphs = 0;
        for (int i = top; i < top + statistics::VISIBLE_LINES; ++i) {
            const auto line = text(state, i);
            for (const char* c = line.c_str(); *c != '\0'; ++c) {
                glyphs += *c != ' ' ? 1 : 0;
            }
        }
        worst = glyphs > worst ? glyphs : worst;
    }
    CHECK(worst <= 128 - 13 - 18);
}

TEST_CASE(statistics_scrolling_stays_inside_the_list) {
    const int max_top = statistics::LINE_COUNT - statistics::VISIBLE_LINES;
    CHECK_EQ(statistics::scroll(0, -1), 0);
    CHECK_EQ(statistics::scroll(0, 1), 1);
    CHECK_EQ(statistics::scroll(max_top, 1), max_top);
    CHECK_EQ(statistics::scroll(max_top - 1, 5), max_top);
    CHECK_EQ(statistics::scroll(3, -2), 1);
}

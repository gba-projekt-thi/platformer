#include "test_framework.h"

#include <cstring>

#include "cfg_dash.h"
#include "controls_text.h"

TEST_CASE(controls_text_has_lines_within_the_pause_overlay_limit) {
    CHECK(ControlsText::LINE_COUNT > 0);
    CHECK(ControlsText::LINE_COUNT <= ControlsText::MAX_LINES);
}

TEST_CASE(controls_text_lines_are_short_printable_ascii) {
    for (int i = 0; i < ControlsText::LINE_COUNT; ++i) {
        const char* line = ControlsText::LINES[i];
        CHECK(line != nullptr);
        const int length = static_cast<int>(std::strlen(line));
        CHECK(length > 0);
        CHECK(length <= ControlsText::MAX_LINE_CHARS);
        for (int c = 0; c < length; ++c) {
            CHECK(line[c] >= 32 && line[c] <= 126);
        }
    }
}

TEST_CASE(controls_text_total_stays_within_the_sprite_budget) {
    int total = 0;
    for (int i = 0; i < ControlsText::LINE_COUNT; ++i) {
        total += static_cast<int>(std::strlen(ControlsText::LINES[i]));
    }
    CHECK(total <= ControlsText::MAX_TOTAL_CHARS);
}

TEST_CASE(controls_text_lists_every_button_the_game_uses) {
    const char* keys[] = {"D-Pad", "A ", "B ", "Start", "Select"};
    for (const char* key : keys) {
        bool found = false;
        for (int i = 0; i < ControlsText::LINE_COUNT; ++i) {
            if (std::strncmp(ControlsText::LINES[i], key, std::strlen(key)) ==
                0) {
                found = true;
            }
        }
        CHECK(found);
    }
}

TEST_CASE(controls_text_dash_line_matches_the_unlock_world) {
    // The dash line spells the unlock world out as "W<n>+"; if
    // Cfg::Dash::UNLOCK_WORLD changes, the text has to follow.
    char expected[] = "W0+";
    expected[1] = static_cast<char>('0' + Cfg::Dash::UNLOCK_WORLD + 1);

    bool found = false;
    for (int i = 0; i < ControlsText::LINE_COUNT; ++i) {
        const char* line = ControlsText::LINES[i];
        if (std::strstr(line, "Dash") != nullptr) {
            found = std::strstr(line, expected) != nullptr;
        }
    }
    CHECK(found);
}

#include "test_framework.h"

#include <cstring>

#include "cfg.h"
#include "cfg_dash.h"
#include "cfg_wall_jump.h"
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
    const char* keys[] = {"D-Pad", "A ", "B ", "R ", "Start", "Select"};
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

TEST_CASE(controls_text_wall_jump_line_matches_the_unlock_world) {
    char expected[] = "Wall W0+";
    expected[6] = static_cast<char>('0' + Cfg::WallJump::UNLOCK_WORLD + 1);

    bool found = false;
    for (int i = 0; i < ControlsText::LINE_COUNT; ++i) {
        found =
            found || std::strstr(ControlsText::LINES[i], expected) != nullptr;
    }
    CHECK(found);
}

TEST_CASE(controls_text_pogo_line_matches_the_unlock_world) {
    char expected[] = "Pogo W0+";
    expected[6] = static_cast<char>('0' + Cfg::Pogo::UNLOCK_WORLD + 1);
    bool found = false;
    for (int i = 0; i < ControlsText::LINE_COUNT; ++i) {
        found =
            found || std::strstr(ControlsText::LINES[i], expected) != nullptr;
    }
    CHECK(found);
}

// Both places that show the list keep every row on the 160 px screen (an
// 8x16 font row reaches 8 px below its y; the screen ends at y = 80).
TEST_CASE(controls_text_fits_the_pause_overlay) {
    const int last =
        Cfg::PauseMenu::Y_OPTION_0 +
        (ControlsText::LINE_COUNT - 1) * Cfg::PauseMenu::OPTION_SPACING;
    CHECK(last + 8 <= 80);
}

TEST_CASE(controls_text_fits_the_controls_screen) {
    // Title, the lines, then the "Press B to go back" prompt.
    const int prompt = Cfg::Summary::Y + (ControlsText::LINE_COUNT + 1) *
                                             Cfg::Summary::LINE_SPACING;
    CHECK(prompt + 8 <= 80);
}

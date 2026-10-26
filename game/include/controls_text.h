#pragma once

// -----------------------------------------------------------------------------
// ControlsText
//
// The controls overview's text, shared by the start-screen ControlsScene and
// the pause menu's Controls view so both always show the same thing. Lines
// are kept short and plain ASCII: the pause menu draws them on top of a
// running level where the 128-sprite OAM budget is tight (see MAX_* below,
// enforced by tests/host/test_controls_text.cpp). The unlock worlds in the
// jump and dash lines follow Cfg::WallJump::UNLOCK_WORLD / Cfg::Dash::
// UNLOCK_WORLD ("W3" = World 3) - the tests check that too.
// -----------------------------------------------------------------------------

namespace ControlsText {

inline constexpr const char* LINES[] = {
    "D-Pad Move",  "A Jump, Wall W5+", "B Dash (W3+)",
    "Start Pause", "Select Timer",
};

inline constexpr int LINE_COUNT = sizeof(LINES) / sizeof(LINES[0]);

// Pause overlay: title row plus MAX_LINES rows fit above the screen's
// bottom edge at the pause menu's row spacing.
inline constexpr int MAX_LINES = 5;
inline constexpr int MAX_LINE_CHARS = 16;
// Combined characters across all lines (spaces counted, like the other
// sprite-capacity comments in this project).
inline constexpr int MAX_TOTAL_CHARS = 64;

}  // namespace ControlsText

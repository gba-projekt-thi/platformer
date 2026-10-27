#pragma once

#include "game_state.h"
#include "world_index.h"

// -----------------------------------------------------------------------------
// story
//
// Hildegard's journey: flushed out of the bathtub, she travels through the
// worlds to reach her beloved Susanne (the World 5 finale). StoryScene shows
// a page of a few lines - the intro when a new game starts, a page the first
// time the duck reaches each later world, and the ending before the reunion. No
// Butano: host-tested in tests/host/test_story.cpp (which page when, and that
// every page fits).
// -----------------------------------------------------------------------------
namespace story {

inline constexpr int MAX_LINES = 5;
// The 8x16 font is about 7 px a character: 28 fit the 240 px screen.
inline constexpr int MAX_LINE_CHARS = 28;
// Every drawn glyph is at most one sprite; no level runs meanwhile, so the
// page and its prompt have the 128 hardware sprites to themselves
// (StoryScene's buffer: MAX_GLYPHS + the prompt).
inline constexpr int MAX_GLYPHS = 110;

struct Page {
    const char* lines[MAX_LINES];
    int line_count;
};

inline constexpr Page INTRO = {
    {"Hildegard's best bath ever.", "Then someone pulls the plug!",
     "Down the drain she swirls,", "far away from Susanne.",
     "Hold on, Hildegard!"},
    5};

// One page per world, by index into WorldIndex::WORLDS. World 1 starts with
// the INTRO instead. A world that unlocks an ability (Cfg::...::
// UNLOCK_WORLD) teaches it here too - the tests check that.
inline constexpr Page WORLDS[] = {
    INTRO,
    {{"Dark, damp, rusty: the", "sewers. Every pipe leads",
      "somewhere - one of them", "must lead to Susanne."},
     4},
    {{"A pipe spits Hildegard out", "into a misty forest. Far",
      "too far to jump? Then dash:", "press B in mid-air!"},
     4},
    {{"A village! The ducks here", "saw Susanne pass by - and",
      "show Hildegard a trick:", "press R to strike down."},
     4},
    {{"Susanne's trail leads on", "into a deep, steep castle.",
      "Hold toward a wall and", "press A: wall jump!"},
     4},
};

// After the finale, before the reunion (KissingScene).
inline constexpr Page ENDING = {
    {"Bath, sewers, forest,", "village and castle -",
     "Hildegard made it through", "all of them. Susanne!",
     "Never pull the plug again."},
    5};

inline constexpr int WORLD_PAGE_COUNT = sizeof(WORLDS) / sizeof(WORLDS[0]);

// The page to show when a save slot is continued: the intro for a game that
// has not begun (nothing reached, no death, no record), else none.
inline auto page_on_continue(const GameState& state) -> const Page* {
    const bool fresh = state.level == 0 && state.furthest_level == 0 &&
                       state.deaths == 0 && state.best_time_frames[0] == 0;
    return fresh ? &INTRO : nullptr;
}

// The page to show when the door leads on to `next_level`: the next world's,
// the first time the duck gets there (`furthest_level` before the advance).
inline auto page_on_advance(int furthest_level, int next_level) -> const Page* {
    if (next_level <= furthest_level) {
        return nullptr;
    }
    for (int world = 1; world < WorldIndex::WORLD_COUNT; ++world) {
        if (WorldIndex::WORLDS[world].start_index == next_level) {
            return &WORLDS[world];
        }
    }
    return nullptr;
}

}  // namespace story

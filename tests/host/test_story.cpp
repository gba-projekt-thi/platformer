#include "test_framework.h"

#include <cstring>

#include "cfg.h"
#include "game_state.h"
#include "story.h"
#include "world_index.h"

// Hildegard's journey to Susanne (story.h): which page shows when, and that
// every page fits the screen and its sprite buffer.

namespace {

auto glyphs(const story::Page& page) -> int {
    int count = 0;
    for (int i = 0; i < page.line_count; ++i) {
        for (const char* c = page.lines[i]; *c != '\0'; ++c) {
            count += *c != ' ' ? 1 : 0;
        }
    }
    return count;
}

void check_fits(const story::Page& page) {
    CHECK(page.line_count > 0);
    CHECK(page.line_count <= story::MAX_LINES);
    for (int i = 0; i < page.line_count; ++i) {
        CHECK(page.lines[i] != nullptr);
        const int length = int(std::strlen(page.lines[i]));
        CHECK(length <= story::MAX_LINE_CHARS);
        for (int c = 0; c < length; ++c) {
            CHECK(page.lines[i][c] >= 32 && page.lines[i][c] <= 126);
        }
    }
    CHECK(glyphs(page) <= story::MAX_GLYPHS);
}

}  // namespace

TEST_CASE(story_a_new_game_starts_with_the_intro) {
    const GameState fresh{};
    CHECK(story::page_on_continue(fresh) == &story::INTRO);
}

TEST_CASE(story_a_game_under_way_skips_the_intro) {
    GameState died{};
    died.deaths = 3;
    CHECK(story::page_on_continue(died) == nullptr);

    GameState later{};
    later.level = 5;
    later.furthest_level = 5;
    CHECK(story::page_on_continue(later) == nullptr);

    GameState finished_before{};  // DataManager::reset() keeps the records
    finished_before.best_time_frames[0] = 1234;
    CHECK(story::page_on_continue(finished_before) == nullptr);
}

TEST_CASE(story_reaching_a_new_world_shows_its_page_once) {
    for (int world = 1; world < WorldIndex::WORLD_COUNT; ++world) {
        const int first = WorldIndex::WORLDS[world].start_index;
        CHECK(
            story::page_on_advance(first - 1, first) == &story::WORLDS[world]);
        // Coming back (furthest already past it): no page again.
        CHECK(story::page_on_advance(first + 1, first) == nullptr);
    }
}

TEST_CASE(story_no_page_inside_a_world) {
    const int second = WorldIndex::WORLDS[1].start_index + 1;
    CHECK(story::page_on_advance(second - 1, second) == nullptr);
}

TEST_CASE(story_every_page_fits_the_screen) {
    check_fits(story::INTRO);
    check_fits(story::ENDING);
    for (int world = 1; world < WorldIndex::WORLD_COUNT; ++world) {
        check_fits(story::WORLDS[world]);
    }
}

TEST_CASE(story_has_a_page_for_every_world) {
    CHECK_EQ(story::WORLD_PAGE_COUNT, WorldIndex::WORLD_COUNT);
}

TEST_CASE(story_tells_of_hildegard_and_susanne) {
    bool hildegard = false;
    bool susanne = false;
    for (int i = 0; i < story::INTRO.line_count; ++i) {
        hildegard |= std::strstr(story::INTRO.lines[i], "Hildegard") != nullptr;
        susanne |= std::strstr(story::INTRO.lines[i], "Susanne") != nullptr;
    }
    CHECK(hildegard);
    CHECK(susanne);
}

TEST_CASE(story_layout_keeps_the_page_and_its_prompt_on_screen) {
    // 8x16 font: a row reaches 8 px above and below its y; the screen is
    // y = -80 .. 80.
    const int last = Cfg::Story::FIRST_LINE_Y +
                     (story::MAX_LINES - 1) * Cfg::Story::LINE_SPACING;
    CHECK(Cfg::Story::FIRST_LINE_Y - 8 >= -80);
    CHECK(last + 8 <= Cfg::Story::PROMPT_Y - 8);
    CHECK(Cfg::Story::PROMPT_Y + 8 <= 80);
}

namespace {

auto mentions(const story::Page& page, const char* word) -> bool {
    for (int i = 0; i < page.line_count; ++i) {
        if (std::strstr(page.lines[i], word) != nullptr) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST_CASE(story_the_ending_reunites_hildegard_and_susanne) {
    CHECK(mentions(story::ENDING, "Susanne"));
    CHECK(mentions(story::ENDING, "Hildegard"));
}

TEST_CASE(story_a_world_page_names_the_ability_it_unlocks) {
    // Same idea as the controls text: if Cfg moves an unlock, the story has
    // to follow.
    CHECK(mentions(story::WORLDS[Cfg::Dash::UNLOCK_WORLD], "dash"));
    CHECK(mentions(story::WORLDS[Cfg::Pogo::UNLOCK_WORLD], "R"));
    CHECK(mentions(story::WORLDS[Cfg::WallJump::UNLOCK_WORLD], "wall"));
}

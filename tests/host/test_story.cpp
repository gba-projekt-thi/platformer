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

void check_fits(const story::Chapter& chapter) {
    CHECK(chapter.page_count > 0);
    CHECK(chapter.page_count <= story::MAX_PAGES);
    for (int p = 0; p < chapter.page_count; ++p) {
        const story::Page& page = chapter.pages[p];
        CHECK(page.line_count > 0);
        CHECK(page.line_count <= story::MAX_LINES);
        for (int i = 0; i < page.line_count; ++i) {
            CHECK(page.lines[i] != nullptr);
            const int length = int(std::strlen(page.lines[i]));
            CHECK(length > 0);
            CHECK(length <= story::MAX_LINE_CHARS);
            for (int c = 0; c < length; ++c) {
                CHECK(page.lines[i][c] >= 32 && page.lines[i][c] <= 126);
            }
        }
        CHECK(glyphs(page) <= story::MAX_GLYPHS);
    }
}

auto mentions(const story::Chapter& chapter, const char* word) -> bool {
    for (int p = 0; p < chapter.page_count; ++p) {
        for (int i = 0; i < chapter.pages[p].line_count; ++i) {
            if (std::strstr(chapter.pages[p].lines[i], word) != nullptr) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

TEST_CASE(story_a_new_game_starts_with_the_intro) {
    const GameState fresh{};
    CHECK(story::chapter_on_continue(fresh) == &story::INTRO);
}

TEST_CASE(story_a_game_under_way_skips_the_intro) {
    GameState died{};
    died.deaths = 3;
    CHECK(story::chapter_on_continue(died) == nullptr);

    GameState later{};
    later.level = 5;
    later.furthest_level = 5;
    CHECK(story::chapter_on_continue(later) == nullptr);

    GameState finished_before{};  // DataManager::reset() keeps the records
    finished_before.best_time_frames[0] = 1234;
    CHECK(story::chapter_on_continue(finished_before) == nullptr);
}

TEST_CASE(story_reaching_a_new_world_shows_its_page_once) {
    for (int world = 1; world < WorldIndex::WORLD_COUNT; ++world) {
        const int first = WorldIndex::WORLDS[world].start_index;
        CHECK(
            story::chapter_on_advance(first - 1, first) ==
            &story::WORLDS[world]);
        // Coming back (furthest already past it): no page again.
        CHECK(story::chapter_on_advance(first + 1, first) == nullptr);
    }
}

TEST_CASE(story_no_page_inside_a_world) {
    const int second = WorldIndex::WORLDS[1].start_index + 1;
    CHECK(story::chapter_on_advance(second - 1, second) == nullptr);
}

TEST_CASE(story_every_page_fits_the_screen) {
    check_fits(story::INTRO);
    check_fits(story::ENDING);
    for (int world = 1; world < WorldIndex::WORLD_COUNT; ++world) {
        check_fits(story::WORLDS[world]);
    }
}

TEST_CASE(story_pages_follow_each_other_until_the_last) {
    CHECK(story::has_next_page(story::ENDING, 0));
    CHECK(story::has_next_page(story::ENDING, story::ENDING.page_count - 2));
    CHECK(!story::has_next_page(story::ENDING, story::ENDING.page_count - 1));
}

TEST_CASE(story_every_chapter_has_its_own_picture) {
    // The intro and the worlds in order, then the ending: six pictures.
    for (int world = 1; world < WorldIndex::WORLD_COUNT; ++world) {
        CHECK(int(story::WORLDS[world].picture) == world);
    }
    CHECK(story::WORLDS[0].picture == story::Picture::Intro);
    CHECK(story::ENDING.picture == story::Picture::Ending);
}

TEST_CASE(story_has_a_chapter_for_every_world) {
    CHECK_EQ(story::WORLD_CHAPTER_COUNT, WorldIndex::WORLD_COUNT);
}

TEST_CASE(story_tells_of_hildegard_and_susanne) {
    CHECK(mentions(story::INTRO, "Hildegard"));
    CHECK(mentions(story::INTRO, "Susanne"));
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

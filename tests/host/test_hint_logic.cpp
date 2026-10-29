#include "test_framework.h"

#include "hint_logic.h"

TEST_CASE(hint_is_shown_for_an_uncleared_level_with_text) {
    // No best time, progression has only reached this level (7 <= 7).
    CHECK(hint::should_show("New: Dash with B", 0, 7, 7));
    CHECK(hint::should_show("New: Dash with B", 0, 0, 7));
}

TEST_CASE(hint_is_not_shown_without_text) {
    CHECK(!hint::should_show(nullptr, 0, 0, 0));
    CHECK(!hint::should_show("", 0, 0, 0));
}

TEST_CASE(hint_disappears_after_a_recorded_best_time) {
    CHECK(!hint::should_show("New: Dash with B", 1, 7, 7));
    CHECK(!hint::should_show("New: Dash with B", 3600, 7, 7));
}

TEST_CASE(hint_disappears_once_progression_moved_past_the_level) {
    // An Assist Mode clear records no best time but advances furthest_level.
    CHECK(!hint::should_show("New: Dash with B", 0, 8, 7));
    CHECK(!hint::should_show("New: Dash with B", 0, 16, 7));
}

TEST_CASE(hint_stays_for_levels_ahead_of_the_progress) {
    CHECK(hint::should_show("Step on a lever to flip it", 0, 14, 16));
}

TEST_CASE(hint_stays_hidden_after_a_finished_run_reset_furthest_level) {
    // DataManager::reset() clears furthest_level but keeps best times.
    CHECK(!hint::should_show("New: Dash with B", 1200, 0, 7));
}

TEST_CASE(hint_sprites_exist_only_while_visible_and_not_paused) {
    CHECK(hint::should_draw(true, false));
    CHECK(!hint::should_draw(true, true));    // released for the pause overlay
    CHECK(!hint::should_draw(false, false));  // expired or dismissed
    CHECK(!hint::should_draw(false, true));
}

TEST_CASE(hint_countdown_does_not_run_while_paused) {
    // LevelScene only ticks the timer while the game runs, so a pause never
    // eats the five seconds.
    hint::Timer timer(3);
    timer.start();
    timer.tick();
    CHECK(!hint::should_draw(timer.visible(), true));  // paused: no tick
    CHECK(timer.visible());
    CHECK(hint::should_draw(timer.visible(), false));
    timer.tick();
    timer.tick();
    CHECK(!timer.visible());
}

TEST_CASE(hint_max_chars_fits_the_screen_at_the_widest_glyph) {
    CHECK(hint::MAX_CHARS * 8 <= 240);
}

TEST_CASE(hint_timer_starts_invisible) {
    hint::Timer timer(5);
    CHECK(!timer.visible());
}

TEST_CASE(hint_timer_is_visible_for_exactly_its_frame_count) {
    hint::Timer timer(5);
    timer.start();
    for (int frame = 0; frame < 5; ++frame) {
        CHECK(timer.visible());
        timer.tick();
    }
    CHECK(!timer.visible());
}

TEST_CASE(hint_timer_stays_off_when_ticked_past_the_end) {
    hint::Timer timer(2);
    timer.start();
    for (int i = 0; i < 10; ++i) {
        timer.tick();
    }
    CHECK(!timer.visible());
}

TEST_CASE(hint_timer_tick_before_start_does_nothing) {
    hint::Timer timer(3);
    timer.tick();
    timer.tick();
    CHECK(!timer.visible());
    timer.start();
    CHECK(timer.visible());
}

TEST_CASE(hint_timer_dismiss_hides_it_at_once) {
    hint::Timer timer(100);
    timer.start();
    timer.dismiss();
    CHECK(!timer.visible());
}

TEST_CASE(hint_timer_can_be_restarted) {
    hint::Timer timer(3);
    timer.start();
    timer.tick();
    timer.tick();
    timer.start();  // full duration again
    timer.tick();
    timer.tick();
    CHECK(timer.visible());
    timer.tick();
    CHECK(!timer.visible());
}

TEST_CASE(hint_of_a_normal_level_starts_at_once) {
    CHECK(hint::should_start(false, false));
}

TEST_CASE(hint_of_a_boss_level_waits_for_the_arena) {
    CHECK(!hint::should_start(true, false));
    CHECK(hint::should_start(true, true));
}

TEST_CASE(hint_text_to_show_hands_on_the_levels_own_text) {
    const char* text = "Jump over it";
    // the very pointer, not a copy
    CHECK(hint::text_to_show(text, 0, 0, 2) == text);
}

TEST_CASE(hint_text_to_show_hands_on_nothing_once_the_level_is_cleared) {
    const char* text = "Jump over it";
    CHECK(hint::text_to_show(text, 120, 0, 2) == nullptr);
    CHECK(hint::text_to_show(text, 0, 3, 2) == nullptr);
    CHECK(hint::text_to_show(nullptr, 0, 0, 2) == nullptr);
    CHECK(hint::text_to_show("", 0, 0, 2) == nullptr);
}

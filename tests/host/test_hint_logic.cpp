#include "test_framework.h"

#include "hint_logic.h"

TEST_CASE(hint_is_shown_for_an_uncleared_level_with_text) {
    CHECK(hint::should_show("New: Dash with B", 0));
}

TEST_CASE(hint_is_not_shown_without_text) {
    CHECK(!hint::should_show(nullptr, 0));
    CHECK(!hint::should_show("", 0));
}

TEST_CASE(hint_disappears_after_the_first_clear) {
    // best_time_frames > 0 means the level has been cleared at least once.
    CHECK(!hint::should_show("New: Dash with B", 1));
    CHECK(!hint::should_show("New: Dash with B", 3600));
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

#include "test_framework.h"

#include "clear_logic.h"

// The level-clear moment and the unlock glow (clear_logic.h).

TEST_CASE(clear_moment_is_idle_until_started) {
    clear_logic::Moment moment(10);
    moment.tick();
    CHECK(!moment.running());
    CHECK(!moment.lit());
}

TEST_CASE(clear_moment_runs_for_exactly_its_frames) {
    clear_logic::Moment moment(10);
    moment.start();
    for (int i = 0; i < 9; ++i) {
        moment.tick();
        CHECK(moment.running());
    }
    moment.tick();
    CHECK(!moment.running());
}

TEST_CASE(clear_moment_is_shorter_than_a_second) {
    CHECK(clear_logic::MOMENT_FRAMES >= 30);
    CHECK(clear_logic::MOMENT_FRAMES <= 45);
}

TEST_CASE(clear_moment_door_blinks) {
    clear_logic::Moment moment;
    moment.start();
    CHECK(moment.lit());
    for (int i = 0; i < clear_logic::BLINK_FRAMES; ++i) {
        moment.tick();
    }
    CHECK(!moment.lit());
    for (int i = 0; i < clear_logic::BLINK_FRAMES; ++i) {
        moment.tick();
    }
    CHECK(moment.lit());
}

TEST_CASE(clear_moment_can_start_over) {
    clear_logic::Moment moment(3);
    moment.start();
    moment.tick();
    moment.tick();
    moment.start();
    CHECK(moment.elapsed() == 0);
    moment.tick();
    moment.tick();
    CHECK(moment.running());
}

TEST_CASE(glow_fades_from_full_to_nothing) {
    clear_logic::Glow glow(4);
    CHECK(!glow.active());
    glow.start();
    CHECK(glow.active());
    CHECK(glow.intensity() == 1);
    glow.tick();
    glow.tick();
    CHECK(glow.intensity() == bn::fixed(0.5));
    glow.tick();
    glow.tick();
    CHECK(!glow.active());
    CHECK(glow.intensity() == 0);
    glow.tick();
    CHECK(glow.intensity() == 0);
}

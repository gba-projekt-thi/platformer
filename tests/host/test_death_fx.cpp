#include "test_framework.h"

#include <cstdlib>

#include "cfg_death_fx.h"
#include "death_fx.h"

namespace {

namespace C = Cfg::DeathFx;
using death_fx::Event;
using death_fx::Sequence;

auto magnitude(death_fx::Offset o) -> int {
    return std::abs(o.x) > std::abs(o.y) ? std::abs(o.x) : std::abs(o.y);
}

}  // namespace

TEST_CASE(death_fx_constants_are_sane) {
    CHECK(C::HITSTOP_FRAMES >= 1);
    // The shake outlasts the freeze: it is still settling when the duck is
    // back.
    CHECK(C::SHAKE_FRAMES > C::HITSTOP_FRAMES);
    CHECK(C::SHAKE_AMPLITUDE >= 1 && C::SHAKE_AMPLITUDE <= 8);
    CHECK(C::RESPAWN_BLINK_FRAMES >= 2);
    // About a tenth of a second of freeze at 60 frames per second.
    CHECK(C::HITSTOP_FRAMES <= 12);
}

TEST_CASE(death_fx_layout_shake_is_short_and_subtle) {
    // The cue that the level changed between attempts: a bump, not a quake.
    CHECK(C::LAYOUT_SHAKE_FRAMES >= 2);
    CHECK(C::LAYOUT_SHAKE_FRAMES <= C::SHAKE_FRAMES);
}

TEST_CASE(death_fx_shake_is_zero_without_frames_or_amplitude) {
    CHECK_EQ(magnitude(death_fx::shake_offset(0, 14, 3)), 0);
    CHECK_EQ(magnitude(death_fx::shake_offset(-2, 14, 3)), 0);
    CHECK_EQ(magnitude(death_fx::shake_offset(5, 0, 3)), 0);
    CHECK_EQ(magnitude(death_fx::shake_offset(5, 14, 0)), 0);
}

TEST_CASE(death_fx_shake_starts_at_the_full_amplitude) {
    CHECK_EQ(magnitude(death_fx::shake_offset(14, 14, 3)), 3);
}

TEST_CASE(death_fx_shake_never_exceeds_the_amplitude) {
    for (int left = 1; left <= 14; ++left) {
        CHECK(magnitude(death_fx::shake_offset(left, 14, 3)) <= 3);
    }
    // frames_left above total is clamped
    CHECK(magnitude(death_fx::shake_offset(99, 14, 3)) <= 3);
}

TEST_CASE(death_fx_shake_fades_out_but_keeps_moving_until_the_last_frame) {
    int previous = 1000;
    for (int left = 14; left >= 1; --left) {
        const auto o = death_fx::shake_offset(left, 14, 3);
        CHECK(std::abs(o.x) <= previous);  // never grows
        CHECK(std::abs(o.x) >= 1);         // still shaking on the last frame
        previous = std::abs(o.x);
    }
}

TEST_CASE(death_fx_shake_changes_direction_every_frame) {
    for (int left = 14; left > 1; --left) {
        const auto a = death_fx::shake_offset(left, 14, 3);
        const auto b = death_fx::shake_offset(left - 1, 14, 3);
        CHECK((a.x > 0) != (b.x > 0));
    }
}

TEST_CASE(death_fx_blink_is_visible_when_done_and_flickers_in_pairs) {
    CHECK(death_fx::blink_visible(0));
    CHECK(death_fx::blink_visible(-3));
    bool pattern[8];
    for (int i = 0; i < 8; ++i) {
        pattern[i] = death_fx::blink_visible(8 - i);  // 8, 7, ... 1
    }
    // 8,7 visible / 6,5 hidden / 4,3 visible / 2,1 hidden
    CHECK(pattern[0] && pattern[1]);
    CHECK(!pattern[2] && !pattern[3]);
    CHECK(pattern[4] && pattern[5]);
    CHECK(!pattern[6] && !pattern[7]);
}

TEST_CASE(death_fx_blink_ends_visible_whatever_its_length) {
    // The countdown always finishes on frames_left == 0, which is visible.
    CHECK(death_fx::blink_visible(0));
    // ... and shows no run longer than two frames on the way there.
    for (int start = 2; start <= 40; ++start) {
        int run = 0;
        for (int left = start; left >= 1; --left) {
            const bool shown = death_fx::blink_visible(left);
            run = (left != start && shown == death_fx::blink_visible(left + 1))
                      ? run + 1
                      : 1;
            CHECK(run <= 2);
        }
    }
}

TEST_CASE(death_fx_sequence_is_idle_until_started) {
    Sequence seq;
    CHECK(!seq.frozen());
    CHECK(!seq.shaking());
    CHECK(seq.advance() == Event::None);
    CHECK_EQ(magnitude(seq.shake_offset(3)), 0);
}

TEST_CASE(death_fx_sequence_freezes_for_exactly_the_hitstop_frames) {
    Sequence seq;
    seq.start(6, 14);
    int frozen_frames = 0;
    int ended_on = -1;
    for (int frame = 1; frame <= 20; ++frame) {
        if (seq.frozen()) {
            ++frozen_frames;
        }
        if (seq.advance() == Event::FreezeEnded) {
            ended_on = frame;
        }
    }
    CHECK_EQ(frozen_frames, 6);
    CHECK_EQ(ended_on, 6);  // reported on the last frozen frame
    CHECK(!seq.frozen());
}

TEST_CASE(death_fx_sequence_reports_the_end_of_the_freeze_only_once) {
    Sequence seq;
    seq.start(3, 8);
    int ends = 0;
    for (int i = 0; i < 30; ++i) {
        if (seq.advance() == Event::FreezeEnded) {
            ++ends;
        }
    }
    CHECK_EQ(ends, 1);
}

TEST_CASE(death_fx_sequence_shake_outlasts_the_freeze) {
    Sequence seq;
    seq.start(6, 14);
    for (int i = 0; i < 6; ++i) {
        seq.advance();
    }
    CHECK(!seq.frozen());
    CHECK(seq.shaking());
    for (int i = 0; i < 8; ++i) {
        seq.advance();
    }
    CHECK(!seq.shaking());
}

TEST_CASE(death_fx_sequence_shake_offset_starts_at_the_full_amplitude) {
    Sequence seq;
    seq.start(6, 14);
    CHECK_EQ(magnitude(seq.shake_offset(3)), 3);
    seq.advance();
    CHECK(magnitude(seq.shake_offset(3)) <= 3);
}

TEST_CASE(death_fx_sequence_hitstop_is_at_least_one_frame) {
    Sequence seq;
    seq.start(0, 5);
    CHECK(seq.frozen());
    CHECK(seq.advance() == Event::FreezeEnded);
    CHECK(!seq.frozen());
}

TEST_CASE(death_fx_sequence_negative_shake_is_no_shake) {
    Sequence seq;
    seq.start(2, -4);
    CHECK(!seq.shaking());
    CHECK_EQ(magnitude(seq.shake_offset(3)), 0);
}

TEST_CASE(death_fx_sequence_a_new_death_restarts_both) {
    Sequence seq;
    seq.start(6, 14);
    for (int i = 0; i < 4; ++i) {
        seq.advance();
    }
    seq.start(6, 14);
    CHECK(seq.frozen());
    CHECK_EQ(magnitude(seq.shake_offset(3)), 3);
    int frozen = 0;
    for (int i = 0; i < 20; ++i) {
        if (seq.frozen()) {
            ++frozen;
        }
        seq.advance();
    }
    CHECK_EQ(frozen, 6);
}

TEST_CASE(death_fx_sequence_clear_stops_everything) {
    Sequence seq;
    seq.start(6, 14);
    seq.clear();
    CHECK(!seq.frozen());
    CHECK(!seq.shaking());
    CHECK_EQ(magnitude(seq.shake_offset(3)), 0);
    CHECK(seq.advance() == Event::None);
}

TEST_CASE(death_fx_sequence_a_shake_alone_does_not_freeze) {
    // The layout cue after a respawn: the picture shakes, the world runs.
    Sequence seq;
    seq.shake(4);
    CHECK(!seq.frozen());
    CHECK(seq.shaking());
    CHECK_EQ(magnitude(seq.shake_offset(3)), 3);
    for (int i = 0; i < 4; ++i) {
        CHECK(seq.advance() == Event::None);
    }
    CHECK(!seq.shaking());
}

TEST_CASE(death_fx_sequence_a_shake_keeps_a_running_freeze) {
    Sequence seq;
    seq.start(2, 14);
    seq.shake(4);
    CHECK(seq.frozen());
    CHECK(seq.advance() == Event::None);
    CHECK(seq.advance() == Event::FreezeEnded);
    CHECK(seq.shaking());
    seq.advance();
    seq.advance();
    CHECK(!seq.shaking());
}

// --- DuckFx: the duck's side of the death feedback --------------------------

using death_fx::DuckFx;
using Blink = DuckFx::Blink;

TEST_CASE(death_fx_duck_comes_back_when_the_freeze_ends_and_then_flickers) {
    DuckFx duck;
    duck.begin();
    CHECK(duck.end(6));  // show the duck, drop the ghost
    int hidden = 0;
    for (int i = 0; i < 6; ++i) {
        const Blink blink = duck.tick(true);
        CHECK(blink != Blink::Idle);
        hidden += blink == Blink::Hide ? 1 : 0;
    }
    CHECK(hidden > 0);
    // the flicker is over: the last frame showed the duck, now nothing to do
    CHECK(duck.tick(true) == Blink::Idle);
}

TEST_CASE(death_fx_duck_flicker_ends_with_the_duck_shown) {
    DuckFx duck;
    duck.begin();
    duck.end(C::RESPAWN_BLINK_FRAMES);
    Blink last = Blink::Idle;
    for (int i = 0; i < C::RESPAWN_BLINK_FRAMES; ++i) {
        last = duck.tick(true);
    }
    CHECK(last == Blink::Show);
}

TEST_CASE(death_fx_duck_flicker_matches_the_blink_pattern) {
    DuckFx duck;
    duck.begin();
    duck.end(8);
    for (int left = 7; left >= 0; --left) {
        const Blink expected =
            death_fx::blink_visible(left) ? Blink::Show : Blink::Hide;
        CHECK(duck.tick(true) == expected);
    }
}

TEST_CASE(death_fx_duck_cancel_shows_it_again_without_a_flicker) {
    DuckFx duck;
    duck.begin();
    CHECK(duck.cancel());
    CHECK(duck.tick(true) == Blink::Idle);
}

TEST_CASE(death_fx_duck_end_and_cancel_do_nothing_without_a_freeze) {
    DuckFx duck;
    CHECK(!duck.end(6));
    CHECK(!duck.cancel());
    CHECK(duck.tick(true) == Blink::Idle);
}

TEST_CASE(death_fx_duck_end_or_cancel_after_the_first_does_nothing) {
    DuckFx duck;
    duck.begin();
    CHECK(duck.end(6));
    CHECK(!duck.end(6));
    CHECK(!duck.cancel());
}

TEST_CASE(death_fx_duck_hidden_for_a_menu_does_not_use_up_the_flicker) {
    DuckFx duck;
    duck.begin();
    duck.end(4);
    for (int i = 0; i < 10; ++i) {
        CHECK(duck.tick(false) == Blink::Idle);
    }
    int ticks = 0;
    while (duck.tick(true) != Blink::Idle) {
        ++ticks;
    }
    CHECK_EQ(ticks, 4);
}

TEST_CASE(death_fx_duck_a_new_death_stops_a_running_flicker) {
    DuckFx duck;
    duck.begin();
    duck.end(10);
    duck.tick(true);
    duck.begin();
    CHECK(duck.tick(true) == Blink::Idle);
}

TEST_CASE(death_fx_duck_show_or_hide_by_the_game_stops_the_flicker) {
    DuckFx duck;
    duck.begin();
    duck.end(10);
    duck.stop_blink();
    CHECK(duck.tick(true) == Blink::Idle);
}

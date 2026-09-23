#include "test_framework.h"

#include "frame_time.h"

TEST_CASE(frame_time_zero_frames_is_all_zero) {
    const FrameTime t = frames_to_time(0);
    CHECK_EQ(t.minutes, 0);
    CHECK_EQ(t.seconds, 0);
    CHECK_EQ(t.centis, 0);
}

TEST_CASE(frame_time_just_under_one_second) {
    const FrameTime t = frames_to_time(59);
    CHECK_EQ(t.minutes, 0);
    CHECK_EQ(t.seconds, 0);
    CHECK_EQ(t.centis, 98);
}

TEST_CASE(frame_time_exactly_one_second) {
    const FrameTime t = frames_to_time(60);
    CHECK_EQ(t.minutes, 0);
    CHECK_EQ(t.seconds, 1);
    CHECK_EQ(t.centis, 0);
}

TEST_CASE(frame_time_just_under_one_minute) {
    const FrameTime t = frames_to_time(3599);
    CHECK_EQ(t.minutes, 0);
    CHECK_EQ(t.seconds, 59);
    CHECK_EQ(t.centis, 98);
}

TEST_CASE(frame_time_exactly_one_minute) {
    const FrameTime t = frames_to_time(3600);
    CHECK_EQ(t.minutes, 1);
    CHECK_EQ(t.seconds, 0);
    CHECK_EQ(t.centis, 0);
}

TEST_CASE(frame_time_twenty_five_minutes) {
    const FrameTime t = frames_to_time(90000);
    CHECK_EQ(t.minutes, 25);
    CHECK_EQ(t.seconds, 0);
    CHECK_EQ(t.centis, 0);
}

TEST_CASE(frame_time_past_ninety_nine_minutes_does_not_wrap) {
    // Relevant to SummaryScene: a sum across 17 levels' best times can
    // plausibly exceed the 99-minute range a single level's mm:ss.cc
    // display assumes. minutes is uint16_t, so 100 must read as 100, not
    // wrap to 0 or truncate to a 2-digit value.
    const FrameTime t = frames_to_time(360000);
    CHECK_EQ(t.minutes, 100);
    CHECK_EQ(t.seconds, 0);
    CHECK_EQ(t.centis, 0);
}
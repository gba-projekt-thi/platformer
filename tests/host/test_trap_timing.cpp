#include "test_framework.h"

#include "trap_timing.h"

using trap_timing::scaled_frames;

TEST_CASE(trap_timing_scaled_frames_unchanged_at_100_percent) {
    CHECK_EQ(scaled_frames(15, 100), 15u);
    CHECK_EQ(scaled_frames(1, 100), 1u);
}

TEST_CASE(trap_timing_scaled_frames_hard_mode_needs_fewer_frames) {
    // x1.35: 15 / 1.35 = 11.1 -> 11, 20 / 1.35 = 14.8 -> 15
    CHECK_EQ(scaled_frames(15, 135), 11u);
    CHECK_EQ(scaled_frames(20, 135), 15u);
    CHECK(scaled_frames(30, 135) < 30u);
}

TEST_CASE(trap_timing_scaled_frames_assist_mode_needs_more_frames) {
    CHECK_EQ(scaled_frames(15, 50), 30u);
    CHECK_EQ(scaled_frames(10, 50), 20u);
}

TEST_CASE(trap_timing_scaled_frames_never_turns_a_duration_into_zero) {
    CHECK_EQ(scaled_frames(1, 135), 1u);
    CHECK_EQ(scaled_frames(1, 400), 1u);
    CHECK_EQ(scaled_frames(2, 1000), 1u);
}

TEST_CASE(trap_timing_scaled_frames_keeps_zero_as_zero) {
    // path_waits 0 means 'does not move'; scaling must not change that.
    CHECK_EQ(scaled_frames(0, 135), 0u);
    CHECK_EQ(scaled_frames(0, 50), 0u);
}

TEST_CASE(trap_timing_scaled_frames_ignores_a_non_positive_percentage) {
    CHECK_EQ(scaled_frames(15, 0), 15u);
    CHECK_EQ(scaled_frames(15, -20), 15u);
}

TEST_CASE(trap_timing_scaled_frames_is_monotonic_in_the_speed) {
    for (unsigned frames = 1; frames <= 60; ++frames) {
        unsigned previous = scaled_frames(frames, 25);
        for (int percent = 50; percent <= 300; percent += 25) {
            const unsigned now = scaled_frames(frames, percent);
            CHECK(now <= previous);  // faster never needs more frames
            previous = now;
        }
    }
}

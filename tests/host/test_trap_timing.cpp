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

// --- CycleTiming ------------------------------------------------------------

using trap_timing::cycle_length;
using trap_timing::cycle_progress;
using trap_timing::cycle_start_frame;
using trap_timing::CycleTiming;
using trap_timing::next_cycle_frame;
using trap_timing::scaled_cycle;

namespace {

// extend 4 / hold 3 / retract 5 / wait 2 = a round of 14 frames
constexpr CycleTiming SPIKES{4, 3, 5, 2};

}  // namespace

TEST_CASE(cycle_length_is_the_sum_of_the_phases) {
    CHECK_EQ(cycle_length(SPIKES), 14u);
    CHECK_EQ(cycle_length(CycleTiming{}), 0u);
}

TEST_CASE(cycle_progress_runs_out_stays_back_and_rests) {
    CHECK_EQ(cycle_progress(SPIKES, 0), 0);     // start of the round: in
    CHECK_EQ(cycle_progress(SPIKES, 2), 500);   // halfway out
    CHECK_EQ(cycle_progress(SPIKES, 4), 1000);  // fully out
    CHECK_EQ(cycle_progress(SPIKES, 6), 1000);  // still out (hold)
    CHECK_EQ(cycle_progress(SPIKES, 7), 1000);  // first retract frame
    CHECK_EQ(cycle_progress(SPIKES, 9), 600);   // on the way back
    CHECK_EQ(cycle_progress(SPIKES, 12), 0);    // resting
    CHECK_EQ(cycle_progress(SPIKES, 13), 0);
}

TEST_CASE(cycle_progress_stays_in_range_and_moves_by_small_steps) {
    int previous = cycle_progress(SPIKES, 0);
    for (unsigned frame = 1; frame < cycle_length(SPIKES); ++frame) {
        const int now = cycle_progress(SPIKES, frame);
        CHECK(now >= 0 && now <= 1000);
        const int step = now > previous ? now - previous : previous - now;
        CHECK(step <= 1000 / 4);  // no frame jumps more than one extend step
        previous = now;
    }
}

TEST_CASE(cycle_progress_zero_length_extend_pops_out_at_once) {
    const CycleTiming pop{0, 10, 0, 20};
    CHECK_EQ(cycle_progress(pop, 0), 1000);
    CHECK_EQ(cycle_progress(pop, 9), 1000);
    CHECK_EQ(cycle_progress(pop, 10), 0);
    CHECK_EQ(cycle_progress(pop, 29), 0);
}

TEST_CASE(cycle_progress_without_a_round_never_moves) {
    CHECK_EQ(cycle_progress(CycleTiming{}, 0), 0);
    CHECK_EQ(cycle_progress(CycleTiming{}, 123), 0);
}

TEST_CASE(cycle_progress_after_the_round_is_resting) {
    CHECK_EQ(cycle_progress(SPIKES, 99), 0);
}

TEST_CASE(cycle_next_frame_wraps_at_the_end_of_the_round) {
    unsigned frame = 0;
    for (unsigned i = 0; i < cycle_length(SPIKES) - 1; ++i) {
        frame = next_cycle_frame(SPIKES, frame);
    }
    CHECK_EQ(frame, 13u);
    CHECK_EQ(next_cycle_frame(SPIKES, frame), 0u);
}

TEST_CASE(cycle_next_frame_of_an_empty_round_stays_at_zero) {
    CHECK_EQ(next_cycle_frame(CycleTiming{}, 0), 0u);
}

TEST_CASE(cycle_repeats_exactly_every_round) {
    const unsigned length = cycle_length(SPIKES);
    int first_round[14];
    unsigned frame = 0;
    for (unsigned i = 0; i < length; ++i) {
        first_round[i] = cycle_progress(SPIKES, frame);
        frame = next_cycle_frame(SPIKES, frame);
    }
    // five more rounds follow the very same curve
    for (unsigned round = 0; round < 5; ++round) {
        for (unsigned i = 0; i < length; ++i) {
            CHECK_EQ(cycle_progress(SPIKES, frame), first_round[i]);
            frame = next_cycle_frame(SPIKES, frame);
        }
    }
}

TEST_CASE(cycle_start_frame_wraps_the_phase_into_the_round) {
    CHECK_EQ(cycle_start_frame(SPIKES, 0), 0u);
    CHECK_EQ(cycle_start_frame(SPIKES, 5), 5u);
    CHECK_EQ(cycle_start_frame(SPIKES, 14), 0u);
    CHECK_EQ(cycle_start_frame(SPIKES, 30), 2u);
    CHECK_EQ(cycle_start_frame(CycleTiming{}, 7), 0u);
}

TEST_CASE(cycle_phase_shifted_traps_are_never_fully_out_together) {
    // Two spikes with the same timing, half a round apart: whenever one is
    // fully out the other is in or on its way - the duck can pass between.
    const CycleTiming t{2, 18, 2, 18};  // 40 frames
    unsigned a = cycle_start_frame(t, 0);
    unsigned b = cycle_start_frame(t, 20);
    for (unsigned i = 0; i < 80; ++i) {
        CHECK(!(cycle_progress(t, a) == 1000 && cycle_progress(t, b) == 1000));
        a = next_cycle_frame(t, a);
        b = next_cycle_frame(t, b);
    }
}

TEST_CASE(cycle_scaled_for_hard_mode_runs_a_shorter_round) {
    const CycleTiming hard = scaled_cycle(SPIKES, 135);
    CHECK(cycle_length(hard) < cycle_length(SPIKES));
    CHECK(
        hard.extend >= 1 && hard.hold >= 1 && hard.retract >= 1 &&
        hard.wait >= 1);
}

TEST_CASE(cycle_scaled_for_assist_mode_runs_a_longer_round) {
    const CycleTiming easy = scaled_cycle(SPIKES, 50);
    CHECK_EQ(easy.extend, 8u);
    CHECK_EQ(easy.hold, 6u);
    CHECK_EQ(easy.retract, 10u);
    CHECK_EQ(easy.wait, 4u);
}

TEST_CASE(cycle_scaling_keeps_a_zero_phase_at_zero) {
    const CycleTiming pop{0, 10, 0, 20};
    const CycleTiming hard = scaled_cycle(pop, 135);
    CHECK_EQ(hard.extend, 0u);
    CHECK_EQ(hard.retract, 0u);
    CHECK(hard.hold >= 1u);
}

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

TEST_CASE(cycle_with_a_wait_longer_than_any_attempt_stabs_exactly_once) {
    // The one-shot spike of a delayed trap chain: a CYCLE trap bound to the
    // chain's last trigger, whose rest outlasts any attempt (a death resets
    // the trap to the start of its round). Out over 4 frames, out for 20,
    // back over 8, then 10 minutes of rest - even in Hard Mode.
    for (int speed : {100, 135, 50}) {
        const CycleTiming stab = scaled_cycle({4, 20, 8, 36000}, speed);
        int stabs = 0;
        bool out = false;
        unsigned frame = 0;
        for (int i = 0; i < 60 * 60 * 3; ++i) {  // three minutes
            const bool now_out = cycle_progress(stab, frame) > 0;
            if (now_out && !out) {
                ++stabs;
            }
            out = now_out;
            frame = next_cycle_frame(stab, frame);
        }
        CHECK_EQ(stabs, 1);
        CHECK(!out);  // and back in its hiding place afterwards
    }
}

// ---- chaser target ----------------------------------------------------------

TEST_CASE(chase_target_follows_behind_the_duck) {
    CHECK_EQ(trap_timing::chase_target(100, 28, -400, 0), 72);
}

TEST_CASE(chase_target_stops_at_its_limit) {
    // A limit keeps the chaser out of a boss arena: at most `limit` px right
    // of where it started.
    CHECK_EQ(trap_timing::chase_target(300, 28, -400, 600), 200);
    CHECK_EQ(trap_timing::chase_target(100, 28, -400, 600), 72);
}

// --- What the factory builds from the level data ----------------------------

#include "bn_fixed.h"

using trap_timing::cycle_setup;
using trap_timing::CycleClock;
using trap_timing::speed_percent;

TEST_CASE(speed_percent_is_the_trap_speed_multiplier_in_percent) {
    CHECK_EQ(speed_percent(bn::fixed(1)), 100);
    CHECK_EQ(speed_percent(bn::fixed(1.35)), 135);
    CHECK_EQ(speed_percent(bn::fixed(0.5)), 50);
}

TEST_CASE(path_waits_of_a_hard_mode_trap_are_scaled_by_the_multiplier) {
    // PathTrap / BossTrap get the scaled count, not the authored one.
    CHECK_EQ(scaled_frames(20, speed_percent(bn::fixed(1.35))), 15u);
    CHECK_EQ(scaled_frames(20, speed_percent(bn::fixed(1))), 20u);
}

TEST_CASE(cycle_setup_scales_every_phase_and_the_start_offset) {
    const auto setup = cycle_setup({4, 3, 5, 2}, 10, 50);
    CHECK_EQ(setup.timing.extend, 8u);
    CHECK_EQ(setup.timing.hold, 6u);
    CHECK_EQ(setup.timing.retract, 10u);
    CHECK_EQ(setup.timing.wait, 4u);
    CHECK_EQ(setup.phase, 20u);
}

TEST_CASE(cycle_setup_at_normal_speed_keeps_the_authored_numbers) {
    const auto setup = cycle_setup({4, 3, 5, 2}, 7, 100);
    CHECK_EQ(cycle_length(setup.timing), 14u);
    CHECK_EQ(setup.phase, 7u);
}

TEST_CASE(cycle_setup_keeps_a_zero_phase_a_zero_phase) {
    CHECK_EQ(cycle_setup({4, 3, 5, 2}, 0, 135).phase, 0u);
}

TEST_CASE(cycle_setup_of_an_empty_round_stays_empty_so_the_load_can_reject_it) {
    CHECK_EQ(cycle_length(cycle_setup({0, 0, 0, 0}, 0, 135).timing), 0u);
}

// --- CycleClock -------------------------------------------------------------

TEST_CASE(cycle_clock_starts_in_the_phase_it_was_given) {
    const CycleClock clock(SPIKES, 2);
    CHECK_EQ(clock.progress(), cycle_progress(SPIKES, 2));
    CHECK_EQ(CycleClock(SPIKES, 14 + 2).progress(), clock.progress());
}

TEST_CASE(cycle_clock_follows_the_timeline_one_frame_per_tick) {
    CycleClock clock(SPIKES, 0);
    unsigned frame = 0;
    for (int i = 0; i < 40; ++i) {
        CHECK_EQ(clock.tick(true), cycle_progress(SPIKES, frame));
        frame = next_cycle_frame(SPIKES, frame);
    }
}

TEST_CASE(cycle_clock_stands_still_while_the_trap_is_frozen) {
    CycleClock clock(SPIKES, 0);
    clock.tick(true);
    clock.tick(true);
    const int where = clock.progress();
    CHECK_EQ(clock.tick(false), CycleClock::FROZEN);
    CHECK_EQ(clock.tick(false), CycleClock::FROZEN);
    CHECK_EQ(clock.progress(), where);
    // and picks up where it stopped
    CHECK_EQ(clock.tick(true), where);
}

TEST_CASE(cycle_clock_reset_returns_to_the_start_frame_of_the_round) {
    CycleClock clock(SPIKES, 3);
    const int start = clock.progress();
    for (int i = 0; i < 9; ++i) {
        clock.tick(true);
    }
    CHECK_EQ(clock.reset(), start);
    CHECK_EQ(clock.progress(), start);
    CHECK_EQ(clock.tick(true), start);
}

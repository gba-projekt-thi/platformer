#include "test_framework.h"

#include "checkpoint_logic.h"

TEST_CASE(checkpoint_tracker_starts_without_an_active_checkpoint) {
    CheckpointTracker tracker;
    CHECK(!tracker.has_active());
    CHECK_EQ(tracker.active(), CheckpointTracker::NONE);
}

TEST_CASE(checkpoint_first_touch_activates_it) {
    CheckpointTracker tracker;
    CHECK(tracker.reach(0));
    CHECK(tracker.has_active());
    CHECK_EQ(tracker.active(), 0);
}

TEST_CASE(checkpoint_touching_the_active_one_again_changes_nothing) {
    CheckpointTracker tracker;
    tracker.reach(1);
    CHECK(!tracker.reach(1));
    CHECK_EQ(tracker.active(), 1);
}

TEST_CASE(checkpoint_only_further_ones_take_over) {
    CheckpointTracker tracker;
    CHECK(tracker.reach(0));
    CHECK(tracker.reach(2));
    CHECK_EQ(tracker.active(), 2);
}

TEST_CASE(checkpoint_walking_back_to_an_earlier_one_keeps_progress) {
    CheckpointTracker tracker;
    tracker.reach(2);
    CHECK(!tracker.reach(0));
    CHECK(!tracker.reach(1));
    CHECK_EQ(tracker.active(), 2);
}

TEST_CASE(checkpoint_skipping_one_is_fine) {
    // The duck can jump past a flag; the later one still counts.
    CheckpointTracker tracker;
    CHECK(tracker.reach(1));
    CHECK_EQ(tracker.active(), 1);
}

TEST_CASE(checkpoint_reset_clears_the_active_one_and_allows_it_again) {
    CheckpointTracker tracker;
    tracker.reach(2);
    tracker.reset();
    CHECK(!tracker.has_active());
    CHECK(tracker.reach(0));
    CHECK_EQ(tracker.active(), 0);
}

TEST_CASE(checkpoint_pack_flags_sets_one_bit_per_true_flag) {
    const bool flags[] = {true, false, true, true};
    const uint16_t mask = pack_flags(flags, 4);
    CHECK_EQ(mask, 0b1101u);
}

TEST_CASE(checkpoint_pack_flags_of_nothing_is_zero) {
    const bool none[] = {false, false};
    CHECK_EQ(pack_flags(none, 2), 0u);
    CHECK_EQ(pack_flags(nullptr, 0), 0u);
}

TEST_CASE(checkpoint_pack_flags_uses_all_sixteen_bits_and_ignores_the_rest) {
    bool flags[20];
    for (bool& flag : flags) {
        flag = true;
    }
    CHECK_EQ(pack_flags(flags, 20), 0xFFFFu);
    CHECK_EQ(MAX_MASK_BITS, 16);
}

TEST_CASE(checkpoint_flag_at_reads_back_what_was_packed) {
    const bool flags[] = {true, false, false, true, true};
    const uint16_t mask = pack_flags(flags, 5);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(flag_at(mask, i), flags[i]);
    }
    CHECK(!flag_at(mask, 5));
}

TEST_CASE(checkpoint_flag_at_out_of_range_is_false) {
    CHECK(!flag_at(0xFFFF, -1));
    CHECK(!flag_at(0xFFFF, 16));
    CHECK(flag_at(0xFFFF, 15));
}

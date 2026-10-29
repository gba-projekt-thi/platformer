#include "test_framework.h"

#include <vector>

#include "ghost_logic.h"

namespace {

using ghost::Playback;
using ghost::Pose;

using Rec = ghost::Recorder<ghost::RECORD_BYTES>;
using SmallStore = ghost::Store<64, 4>;

auto pose(int x, int y, int tile = 0, bool flip = false) -> Pose {
    return Pose{int16_t(x), int16_t(y), uint8_t(tile), flip};
}

// A duck walking right 1 px per frame, frame 0 .. frames (inclusive).
void walk(Rec& rec, int frames) {
    for (int f = 0; f <= frames; ++f) {
        rec.sample(f, pose(100 + f, 50, f / 8 % 4));
    }
}

}  // namespace

TEST_CASE(ghost_walk_costs_three_bytes_per_sample) {
    Rec rec;
    rec.clear();
    walk(rec, 100);
    CHECK(rec.usable());
    // 51 samples: the first absolute (6), 50 deltas (3).
    CHECK_EQ(int(rec.size()), 6 + 50 * 3);
}

TEST_CASE(ghost_playback_returns_every_recorded_pose) {
    Rec rec;
    rec.clear();
    walk(rec, 100);
    Playback play(rec.data(), rec.size(), 100);
    for (int f = 0; f < 100; f += 2) {
        Pose p;
        CHECK(play.pose_at(f, p));
        CHECK_EQ(int(p.x), 100 + f);
        CHECK_EQ(int(p.y), 50);
        CHECK_EQ(int(p.tile), f / 8 % 4);
    }
}

TEST_CASE(ghost_odd_frames_are_interpolated) {
    Rec rec;
    rec.clear();
    walk(rec, 100);
    Playback play(rec.data(), rec.size(), 100);
    for (int f = 0; f < 100; ++f) {
        Pose p;
        CHECK(play.pose_at(f, p));
        CHECK_EQ(int(p.x), 100 + f);
    }
}

TEST_CASE(ghost_is_gone_at_its_finish_frame) {
    Rec rec;
    rec.clear();
    walk(rec, 41);
    Playback play(rec.data(), rec.size(), 41);
    Pose p;
    CHECK(play.pose_at(40, p));
    CHECK(!play.pose_at(41, p));
    CHECK(!play.pose_at(500, p));
    // A ghost that does not exist never shows.
    CHECK(!Playback().pose_at(0, p));
}

TEST_CASE(ghost_keeps_facing_and_picture) {
    Rec rec;
    rec.clear();
    rec.sample(0, pose(10, 10, 5, false));
    rec.sample(2, pose(11, 10, 127, true));
    Playback play(rec.data(), rec.size(), 4);
    Pose p;
    CHECK(play.pose_at(0, p));
    CHECK_EQ(int(p.tile), 5);
    CHECK(!p.flip);
    CHECK(play.pose_at(2, p));
    CHECK_EQ(int(p.tile), 127);
    CHECK(p.flip);
}

TEST_CASE(ghost_a_respawn_jumps_without_gliding) {
    // A death: the duck is back at the spawn 300 px away, also off the
    // 16-bit-delta range of nothing - and negative coordinates survive.
    Rec rec;
    rec.clear();
    rec.sample(0, pose(-120, -30));
    rec.sample(2, pose(-118, -30));
    rec.sample(4, pose(200, 90));  // respawn
    rec.sample(6, pose(201, 90));
    Playback play(rec.data(), rec.size(), 8);
    Pose p;
    // Frame 3 lies between the walk and the jump: it stays on the old pose.
    CHECK(play.pose_at(3, p));
    CHECK_EQ(int(p.x), -118);
    CHECK(play.pose_at(4, p));
    CHECK_EQ(int(p.x), 200);
    CHECK_EQ(int(p.y), 90);
    CHECK(play.pose_at(5, p));
    CHECK_EQ(int(p.x), 200);  // (200 + 201) / 2
    CHECK(play.pose_at(6, p));
    CHECK_EQ(int(p.x), 201);
}

TEST_CASE(ghost_big_but_possible_steps_stay_deltas) {
    Rec rec;
    rec.clear();
    rec.sample(0, pose(0, 0));
    rec.sample(2, pose(ghost::MAX_STEP, -ghost::MAX_STEP));
    CHECK_EQ(int(rec.size()), 6 + 3);
    rec.sample(
        4, pose(ghost::MAX_STEP + ghost::MAX_STEP + 1, -ghost::MAX_STEP));
    CHECK_EQ(int(rec.size()), 6 + 3 + 6);
}

TEST_CASE(ghost_playback_can_start_over) {
    Rec rec;
    rec.clear();
    walk(rec, 100);
    Playback play(rec.data(), rec.size(), 100);
    Pose p;
    CHECK(play.pose_at(60, p));
    CHECK_EQ(int(p.x), 160);
    CHECK(play.pose_at(10, p));
    CHECK_EQ(int(p.x), 110);
}

TEST_CASE(ghost_playback_skips_frames_the_clock_skipped) {
    Rec rec;
    rec.clear();
    walk(rec, 100);
    Playback play(rec.data(), rec.size(), 100);
    Pose p;
    CHECK(play.pose_at(0, p));
    CHECK(play.pose_at(90, p));
    CHECK_EQ(int(p.x), 190);
}

TEST_CASE(ghost_recorder_drops_a_run_that_does_not_fit) {
    ghost::Recorder<24> rec;
    rec.clear();
    for (int f = 0; f < 40; ++f) {
        rec.sample(f, pose(f, 0));
    }
    CHECK(!rec.usable());
    rec.clear();
    CHECK(!rec.usable());  // nothing recorded yet
    rec.sample(0, pose(0, 0));
    CHECK(rec.usable());
}

TEST_CASE(ghost_recorder_drops_a_run_with_a_gap) {
    Rec rec;
    rec.clear();
    rec.sample(0, pose(0, 0));
    rec.sample(6, pose(3, 0));  // frames 2 and 4 never came
    CHECK(!rec.usable());
}

TEST_CASE(ghost_recorder_ignores_odd_frames) {
    Rec rec;
    rec.clear();
    rec.sample(0, pose(0, 0));
    rec.sample(1, pose(1, 0));
    rec.sample(2, pose(2, 0));
    CHECK(rec.usable());
    CHECK_EQ(int(rec.size()), 6 + 3);
}

TEST_CASE(ghost_a_long_run_fits_the_record_buffer) {
    // 4000 frames at 3 bytes per two frames, plus the first sample.
    CHECK(ghost::RECORD_BYTES >= 6 + 2000 * 3);
}

TEST_CASE(ghost_store_keeps_the_fastest_run_of_a_level) {
    SmallStore store;
    const uint8_t a[] = {1, 2, 3};
    const uint8_t b[] = {4, 5};
    CHECK(!store.has(1));
    CHECK(store.offer(1, 500, a, 3));
    CHECK(store.has(1));
    CHECK_EQ(int(store.frames(1)), 500);
    // Slower or equal: the stored ghost stays.
    CHECK(!store.offer(1, 500, b, 2));
    CHECK(!store.offer(1, 600, b, 2));
    CHECK_EQ(int(store.bytes_used()), 3);
    // Faster: replaced.
    CHECK(store.offer(1, 400, b, 2));
    CHECK_EQ(int(store.frames(1)), 400);
    CHECK_EQ(int(store.bytes_used()), 2);
}

TEST_CASE(ghost_store_levels_do_not_disturb_each_other) {
    ghost::Store<256, 4> store;
    Rec rec;
    rec.clear();
    walk(rec, 20);
    const size_t n = rec.size();
    CHECK(store.offer(0, 20, rec.data(), n));
    Rec other;
    other.clear();
    for (int f = 0; f <= 20; ++f) {
        other.sample(f, pose(-f, 7));
    }
    CHECK(store.offer(2, 20, other.data(), other.size()));
    CHECK(store.offer(3, 20, other.data(), other.size()));

    // Replace the first one by a longer-coded faster run: the second moves
    // down and still plays.
    Rec faster;
    faster.clear();
    for (int f = 0; f <= 16; ++f) {
        faster.sample(f, pose(5 * f, 0));
    }
    CHECK(store.offer(0, 16, faster.data(), faster.size()));
    Playback two = store.open(2);
    Pose p;
    CHECK(two.pose_at(10, p));
    CHECK_EQ(int(p.x), -10);
    CHECK_EQ(int(p.y), 7);
    Playback one = store.open(0);
    CHECK(one.pose_at(8, p));
    CHECK_EQ(int(p.x), 40);
}

TEST_CASE(ghost_store_is_full_keeps_the_old_ghost) {
    SmallStore store;
    uint8_t big[40] = {};
    CHECK(store.offer(0, 100, big, 40));
    // 40 + 30 > 64: no room for another level.
    CHECK(!store.offer(1, 100, big, 30));
    CHECK(!store.has(1));
    // A faster run of level 0 that grows by a little still fits (it replaces).
    CHECK(store.offer(0, 90, big, 60));
    CHECK_EQ(int(store.bytes_used()), 60);
    // One that cannot fit even when replacing leaves the old one alone.
    uint8_t huge[65] = {};
    CHECK(!store.offer(0, 80, huge, 65));
    CHECK_EQ(int(store.frames(0)), 90);
}

TEST_CASE(ghost_store_rejects_nonsense) {
    SmallStore store;
    const uint8_t a[] = {1};
    CHECK(!store.offer(1, 0, a, 1));   // no time
    CHECK(!store.offer(1, 10, a, 0));  // no data
    CHECK(!store.offer(9, 10, a, 1));  // no such key
    CHECK(!store.open(1).valid());
}

TEST_CASE(ghost_keys_leave_room_for_mirrored_levels) {
    CHECK_EQ(int(ghost::key_for(5)), 5);
    CHECK(ghost::key_for(5, true) != ghost::key_for(5));
    CHECK(ghost::key_for(31, true) < ghost::KEYS);
    CHECK_EQ(int(ghost::key_for(0, true)), int(ghost::LEVELS));
}

TEST_CASE(ghost_budget_numbers) {
    // 48 KiB of ghosts: about 9 minutes of play at 1.5 bytes a frame.
    CHECK(ghost::STORE_BYTES / 3 * 2 >= 60 * 60 * 9 - 1000);
    CHECK(ghost::STORE_BYTES <= 0xFFFF);
}

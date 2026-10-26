#include "test_framework.h"

#include "door_flee.h"

// The Level Devil door that runs away (door_flee.h): it waits at its origin
// and slides on to the next stop of its path whenever the duck comes close.

namespace {

struct Point {
    bn::fixed px;
    bn::fixed py;
    [[nodiscard]] auto x() const -> bn::fixed { return px; }
    [[nodiscard]] auto y() const -> bn::fixed { return py; }
};

using Flee = door_flee::Flee<Point>;

// Two stops: 60 px right, then 40 px up from there.
constexpr Point PATH[] = {{60, 0}, {60, -40}};
constexpr int RANGE = 24;
constexpr int FRAMES = 10;

auto far_away() -> Point {
    return {200, 200};
}

// Runs the door until it stands still again, the duck far away.
void settle(Flee& flee) {
    for (int i = 0; i < 100 && flee.moving(); ++i) {
        flee.update(far_away().x(), far_away().y());
    }
}

}  // namespace

TEST_CASE(door_flee_without_a_path_never_moves) {
    Flee flee(nullptr, 0, RANGE, FRAMES);
    flee.update(0, 0);
    CHECK(!flee.moving());
    CHECK_EQ(flee.x(), bn::fixed(0));
    CHECK_EQ(flee.y(), bn::fixed(0));
}

TEST_CASE(door_flee_waits_while_the_duck_is_far) {
    Flee flee(PATH, 2, RANGE, FRAMES);
    for (int i = 0; i < 30; ++i) {
        flee.update(RANGE + 1, 0);
    }
    CHECK(!flee.moving());
    CHECK_EQ(flee.x(), bn::fixed(0));
    CHECK_EQ(flee.stop(), 0);
}

TEST_CASE(door_flee_runs_when_the_duck_comes_close) {
    Flee flee(PATH, 2, RANGE, FRAMES);
    flee.update(-RANGE, RANGE);  // range is inclusive, on both axes
    CHECK(flee.moving());
    CHECK(flee.x() > 0);  // on its way, 1 / FRAMES of it
    CHECK(flee.x() < 60);
}

TEST_CASE(door_flee_slides_evenly_to_the_next_stop) {
    Flee flee(PATH, 2, RANGE, FRAMES);
    flee.update(0, 0);
    for (int i = 1; i < FRAMES; ++i) {
        flee.update(far_away().x(), far_away().y());
    }
    CHECK(!flee.moving());
    CHECK_EQ(flee.stop(), 1);
    CHECK_EQ(flee.x(), bn::fixed(60));
    CHECK_EQ(flee.y(), bn::fixed(0));
}

TEST_CASE(door_flee_runs_again_from_where_it_stands) {
    Flee flee(PATH, 2, RANGE, FRAMES);
    flee.update(0, 0);
    settle(flee);
    flee.update(0, 0);  // the duck at the old spot: nothing happens
    CHECK(!flee.moving());
    flee.update(60, 0);
    CHECK(flee.moving());
    settle(flee);
    CHECK_EQ(flee.stop(), 2);
    CHECK_EQ(flee.x(), bn::fixed(60));
    CHECK_EQ(flee.y(), bn::fixed(-40));
}

TEST_CASE(door_flee_stays_at_the_last_stop) {
    Flee flee(PATH, 2, RANGE, FRAMES);
    flee.update(0, 0);
    settle(flee);
    flee.update(60, 0);
    settle(flee);
    flee.update(60, -40);
    CHECK(!flee.moving());
    CHECK(flee.done());
    CHECK_EQ(flee.y(), bn::fixed(-40));
}

TEST_CASE(door_flee_reset_brings_it_back_to_the_origin) {
    Flee flee(PATH, 2, RANGE, FRAMES);
    flee.update(0, 0);
    flee.update(far_away().x(), far_away().y());
    flee.reset();
    CHECK(!flee.moving());
    CHECK_EQ(flee.stop(), 0);
    CHECK_EQ(flee.x(), bn::fixed(0));
    flee.update(0, 0);
    CHECK(flee.moving());
}

TEST_CASE(door_flee_zero_frames_jumps_at_once) {
    Flee flee(PATH, 2, RANGE, 0);
    flee.update(0, 0);
    CHECK(!flee.moving());
    CHECK_EQ(flee.stop(), 1);
    CHECK_EQ(flee.x(), bn::fixed(60));
}

TEST_CASE(door_flee_without_a_path_is_done_at_once) {
    Flee flee(nullptr, 0, RANGE, FRAMES);
    CHECK(flee.done());
    Flee running(PATH, 2, RANGE, FRAMES);
    CHECK(!running.done());
}

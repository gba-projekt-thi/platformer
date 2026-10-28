#include "test_framework.h"

#include "ambush_logic.h"

// The lurking trap's motion (ambush_logic.h): it waits until the duck comes
// within range, lunges along its axis, comes back and waits again. AmbushTrap
// moves along x (a disguised enemy) or y (arms or spikes from the ground).

namespace {

using Motion = ambush::Motion<int>;

constexpr int RANGE = 20;
constexpr int SPEED = -3;  // up / left
constexpr unsigned DURATION = 4;

}  // namespace

TEST_CASE(ambush_waits_while_the_duck_is_out_of_range) {
    Motion m(RANGE, SPEED, DURATION);
    for (int i = 0; i < 10; ++i) {
        m.update(RANGE + 1);
    }
    CHECK(m.state() == ambush::State::Dormant);
    CHECK_EQ(m.offset(), 0);
}

TEST_CASE(ambush_springs_when_the_duck_is_in_range) {
    Motion m(RANGE, SPEED, DURATION);
    m.update(RANGE);  // seen: starts next frame
    CHECK(m.state() == ambush::State::Lunging);
    CHECK_EQ(m.offset(), 0);
    m.update(500);  // the duck may run away: it lunges anyway
    CHECK_EQ(m.offset(), SPEED);
}

TEST_CASE(ambush_lunges_for_its_duration_then_returns) {
    Motion m(RANGE, SPEED, DURATION);
    m.update(0);
    for (unsigned i = 0; i < DURATION; ++i) {
        m.update(0);
    }
    CHECK_EQ(m.offset(), SPEED * int(DURATION));
    CHECK(m.state() == ambush::State::Returning);
    for (unsigned i = 0; i < DURATION; ++i) {
        m.update(500);
    }
    CHECK_EQ(m.offset(), 0);
    CHECK(m.state() == ambush::State::Dormant);
}

TEST_CASE(ambush_return_never_overshoots) {
    ambush::Motion<int> m(RANGE, 5, 1);
    m.update(0);
    m.update(0);  // offset 5, returning
    m.update(500);
    CHECK_EQ(m.offset(), 0);
}

TEST_CASE(ambush_reset_puts_it_back_to_waiting) {
    Motion m(RANGE, SPEED, DURATION);
    m.update(0);
    m.update(0);
    m.reset();
    CHECK(m.state() == ambush::State::Dormant);
    CHECK_EQ(m.offset(), 0);
}

TEST_CASE(ambush_rests_after_sinking_even_with_the_duck_in_range) {
    ambush::Motion<int> m(RANGE, SPEED, DURATION, 5);
    m.update(0);
    for (unsigned i = 0; i < 2 * DURATION; ++i) {
        m.update(0);
    }
    CHECK_EQ(m.offset(), 0);
    CHECK(m.state() == ambush::State::Resting);
    for (int i = 0; i < 4; ++i) {
        m.update(0);  // still resting although the duck stands at the edge
        CHECK_EQ(m.offset(), 0);
    }
    CHECK(m.state() == ambush::State::Resting);
    m.update(0);  // the 5th rest frame is over
    CHECK(m.state() == ambush::State::Dormant);
    m.update(0);
    CHECK(m.state() == ambush::State::Lunging);
}

TEST_CASE(ambush_without_rest_springs_again_right_away) {
    Motion m(RANGE, SPEED, DURATION);
    m.update(0);
    for (unsigned i = 0; i < 2 * DURATION; ++i) {
        m.update(0);
    }
    CHECK(m.state() == ambush::State::Dormant);
    m.update(0);
    CHECK(m.state() == ambush::State::Lunging);
}

TEST_CASE(ambush_reset_ends_the_rest) {
    ambush::Motion<int> m(RANGE, SPEED, 1, 5);
    m.update(0);
    m.update(0);
    m.update(0);
    CHECK(m.state() == ambush::State::Resting);
    m.reset();
    CHECK(m.state() == ambush::State::Dormant);
    m.update(0);
    CHECK(m.state() == ambush::State::Lunging);
}

#include "test_framework.h"

#include "move_resolution.h"

// PhysicsBody::move() resolves each axis with resolve_axis(): a blocked move
// goes up to the obstacle instead of being dropped.

namespace {

// An 8 px tall body over a floor whose top is at y = 10: the body (centre p)
// overlaps it once its bottom p + 4 is below 10.
auto floor_at_10(bn::fixed p) -> bool {
    return p + 4 > 10;
}

// A ceiling whose bottom is at y = -10.
auto ceiling_at_minus_10(bn::fixed p) -> bool {
    return p - 4 < -10;
}

}  // namespace

TEST_CASE(move_resolution_a_free_move_goes_all_the_way) {
    const AxisMove move =
        resolve_axis(bn::fixed(0), bn::fixed(2.5), floor_at_10);
    CHECK_EQ(move.position, bn::fixed(2.5));
    CHECK(!move.blocked);
}

TEST_CASE(move_resolution_a_fall_ends_right_on_the_floor) {
    // Bottom at 7.5, falling 3 px: the floor is 2.5 px away. The old move
    // dropped the whole step and left the body hovering 2.5 px above it.
    const AxisMove move =
        resolve_axis(bn::fixed(3.5), bn::fixed(3), floor_at_10);
    CHECK(move.blocked);
    CHECK(!floor_at_10(move.position));            // never inside it
    CHECK(move.position + 4 > bn::fixed(10) - 1);  // less than 1 px gap
}

TEST_CASE(move_resolution_a_jump_ends_right_under_the_ceiling) {
    const AxisMove move =
        resolve_axis(bn::fixed(-3.25), bn::fixed(-3), ceiling_at_minus_10);
    CHECK(move.blocked);
    CHECK(!ceiling_at_minus_10(move.position));
    CHECK(move.position - 4 < bn::fixed(-10) + 1);
}

TEST_CASE(move_resolution_touching_stays_put) {
    const AxisMove move =
        resolve_axis(bn::fixed(6), bn::fixed(0.25), floor_at_10);
    CHECK(move.blocked);
    CHECK_EQ(move.position, bn::fixed(6));
}

TEST_CASE(move_resolution_a_small_blocked_step_does_not_move) {
    // Less than 1 px from the floor: the only step would end inside it.
    const AxisMove move =
        resolve_axis(bn::fixed(5.5), bn::fixed(0.75), floor_at_10);
    CHECK(move.blocked);
    CHECK_EQ(move.position, bn::fixed(5.5));
}

TEST_CASE(move_resolution_no_move_is_never_blocked) {
    const AxisMove move = resolve_axis(bn::fixed(6), bn::fixed(0), floor_at_10);
    CHECK(!move.blocked);
    CHECK_EQ(move.position, bn::fixed(6));
}

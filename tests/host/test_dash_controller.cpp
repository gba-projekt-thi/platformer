#include "test_framework.h"

#include "cfg_dash.h"
#include "dash_controller.h"
#include "world_index.h"

namespace {

constexpr int DUR = Cfg::Dash::DURATION_FRAMES;
constexpr int CD = Cfg::Dash::COOLDOWN_FRAMES;

auto make(bool enabled = true) -> DashController {
    DashController dash(DUR, CD);
    dash.set_enabled(enabled);
    return dash;
}

auto press() -> DashInput {
    DashInput in;
    in.dash_pressed = true;
    return in;
}

// Runs idle frames (no input) until the dash reports ended; returns how many
// update() calls moved the dash, counting `moved_so_far` from the start call.
auto run_to_end(DashController& dash, int moved_so_far) -> int {
    int moved = moved_so_far;
    for (int i = 0; i < DUR + 5; ++i) {
        const DashStep step = dash.update(DashInput{});
        if (step.moving) {
            ++moved;
        }
        if (step.ended) {
            return moved;
        }
    }
    return -1;  // never ended
}

}  // namespace

TEST_CASE(dash_constants_are_sane) {
    CHECK(Cfg::Dash::SPEED > 0);
    // PhysicsBody::move_velocity() clamps by vel_max (5 for the player).
    CHECK(Cfg::Dash::SPEED < 5);
    CHECK_EQ(Cfg::Dash::SPEED * Cfg::Dash::DURATION_FRAMES, 32);
    CHECK(Cfg::Dash::COOLDOWN_FRAMES >= Cfg::Dash::DURATION_FRAMES);
    CHECK(Cfg::Dash::UNLOCK_WORLD >= 0);
    CHECK(Cfg::Dash::UNLOCK_WORLD < WorldIndex::WORLD_COUNT);
}

TEST_CASE(dash_disabled_never_starts) {
    DashController dash = make(false);
    const DashStep step = dash.update(press());
    CHECK(!step.started);
    CHECK(!step.moving);
    CHECK(!dash.dashing());
}

TEST_CASE(dash_starts_on_press_and_already_moves) {
    DashController dash = make();
    const DashStep step = dash.update(press());
    CHECK(step.started);
    CHECK(step.moving);
    CHECK(!step.ended);
    CHECK(dash.dashing());
}

TEST_CASE(dash_does_nothing_without_a_press) {
    DashController dash = make();
    const DashStep step = dash.update(DashInput{});
    CHECK(!step.started);
    CHECK(!step.moving);
}

TEST_CASE(dash_direction_follows_held_direction) {
    DashController left = make();
    DashInput in = press();
    in.left_held = true;
    CHECK_EQ(left.update(in).dir, -1);

    DashController right = make();
    in = press();
    in.right_held = true;
    CHECK_EQ(right.update(in).dir, 1);
}

TEST_CASE(dash_direction_left_wins_when_both_held) {
    DashController dash = make();
    DashInput in = press();
    in.left_held = true;
    in.right_held = true;
    CHECK_EQ(dash.update(in).dir, -1);
}

TEST_CASE(dash_direction_falls_back_to_facing) {
    DashController facing_left = make();
    DashInput in = press();
    in.facing_left = true;
    CHECK_EQ(facing_left.update(in).dir, -1);

    DashController facing_right = make();
    CHECK_EQ(facing_right.update(press()).dir, 1);
}

TEST_CASE(dash_direction_stays_fixed_for_the_whole_dash) {
    DashController dash = make();
    DashInput in = press();
    in.left_held = true;
    dash.update(in);

    DashInput other;
    other.right_held = true;  // changing the held direction mid-dash
    const DashStep step = dash.update(other);
    CHECK(step.moving);
    CHECK_EQ(step.dir, -1);
}

TEST_CASE(dash_moves_for_exactly_duration_frames) {
    DashController dash = make();
    const DashStep first = dash.update(press());
    CHECK(first.moving);
    CHECK_EQ(run_to_end(dash, 1), DUR);
}

TEST_CASE(dash_ending_frame_does_not_move_and_starts_cooldown) {
    DashController dash = make();
    dash.update(press());
    DashStep step;
    for (int i = 0; i < DUR; ++i) {
        step = dash.update(DashInput{});
    }
    CHECK(step.ended);
    CHECK(!step.moving);
    CHECK(!dash.dashing());
    CHECK_EQ(dash.cooldown_remaining(), CD);
}

TEST_CASE(dash_cooldown_blocks_restart_until_elapsed) {
    DashController dash = make();
    dash.update(press());
    run_to_end(dash, 1);

    for (int k = 1; k < CD; ++k) {
        dash.land();  // ground dash: charge is not the limiting factor
        CHECK(!dash.update(press()).started);
    }
    dash.land();
    CHECK(dash.update(press()).started);
}

TEST_CASE(dash_air_dash_needs_landing_to_recharge) {
    DashController dash = make();
    dash.update(press());
    run_to_end(dash, 1);
    CHECK(!dash.available());

    // Wait out the cooldown without touching the ground.
    for (int k = 0; k < CD + 5; ++k) {
        CHECK(!dash.update(press()).started);
    }

    dash.land();
    CHECK(dash.available());
    CHECK(dash.update(press()).started);
}

TEST_CASE(dash_start_spends_the_charge) {
    DashController dash = make();
    CHECK(dash.available());
    dash.update(press());
    CHECK(!dash.available());
}

TEST_CASE(dash_wall_hit_ends_it_after_the_first_frame) {
    DashController dash = make();
    dash.update(press());

    DashInput blocked;
    blocked.vel_x_zero = true;
    const DashStep step = dash.update(blocked);
    CHECK(step.ended);
    CHECK(!step.moving);
    CHECK_EQ(dash.cooldown_remaining(), CD);
}

TEST_CASE(dash_ignores_zero_velocity_on_its_first_frame) {
    // vel_x is still 0 on the frame the dash starts - that is not a wall.
    DashController dash = make();
    DashInput in = press();
    in.vel_x_zero = true;
    const DashStep step = dash.update(in);
    CHECK(step.started);
    CHECK(step.moving);
    CHECK(!step.ended);
}

TEST_CASE(dash_ends_at_the_bounce_edge_in_its_direction) {
    DashController right = make();
    DashInput in = press();
    in.right_held = true;
    right.update(in);
    DashInput edge;
    edge.at_right_edge = true;
    CHECK(right.update(edge).ended);

    DashController left = make();
    in = press();
    in.left_held = true;
    left.update(in);
    edge = DashInput{};
    edge.at_left_edge = true;
    CHECK(left.update(edge).ended);
}

TEST_CASE(dash_ignores_the_opposite_bounce_edge) {
    DashController dash = make();
    DashInput in = press();
    in.right_held = true;
    dash.update(in);

    DashInput edge;
    edge.at_left_edge = true;  // behind the dash, not in front of it
    const DashStep step = dash.update(edge);
    CHECK(!step.ended);
    CHECK(step.moving);
}

TEST_CASE(dash_buffered_jump_on_ground_cancels_into_a_jump) {
    DashController dash = make();
    dash.update(press());

    DashInput jump;
    jump.jump_buffered = true;
    jump.on_ground = true;
    const DashStep step = dash.update(jump);
    CHECK(step.ended);
    CHECK(!step.moving);
}

TEST_CASE(dash_buffered_jump_in_the_air_does_not_cancel) {
    DashController dash = make();
    dash.update(press());

    DashInput jump;
    jump.jump_buffered = true;
    jump.on_ground = false;
    const DashStep step = dash.update(jump);
    CHECK(!step.ended);
    CHECK(step.moving);
}

TEST_CASE(dash_on_ground_without_buffered_jump_does_not_cancel) {
    DashController dash = make();
    dash.update(press());

    DashInput ground;
    ground.on_ground = true;
    CHECK(dash.update(ground).moving);
}

TEST_CASE(dash_start_and_jump_cancel_can_share_a_frame) {
    DashController dash = make();
    DashInput in = press();
    in.jump_buffered = true;
    in.on_ground = true;
    const DashStep step = dash.update(in);
    CHECK(step.started);
    CHECK(step.ended);
    CHECK(!step.moving);
    CHECK(!dash.dashing());
}

TEST_CASE(dash_reset_cancels_dash_and_clears_cooldown_and_recharges) {
    DashController dash = make();
    dash.update(press());
    dash.reset();

    CHECK(!dash.dashing());
    CHECK_EQ(dash.cooldown_remaining(), 0);
    CHECK(dash.available());
    CHECK(dash.update(press()).started);
}

TEST_CASE(dash_set_enabled_false_cancels_a_running_dash) {
    DashController dash = make();
    dash.update(press());
    dash.set_enabled(false);

    CHECK(!dash.dashing());
    CHECK(!dash.update(press()).started);
}

TEST_CASE(dash_dash_does_not_cancel_without_condition_over_full_duration) {
    // Regression guard: a plain dash with idle input must run its whole
    // duration - no spurious early end from default-initialized inputs.
    DashController dash = make();
    dash.update(press());
    CHECK_EQ(run_to_end(dash, 1), DUR);
}

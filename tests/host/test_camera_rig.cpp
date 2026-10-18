#include "test_framework.h"

#include <cmath>

#include "camera_rig.h"

namespace {

namespace C = Cfg::CameraFollow;
using Rig = CameraRig<double>;

// Integer fixed-point stand-in for bn::fixed (8 fractional bits, truncating
// division): CameraRig must also converge cleanly when the arithmetic rounds.
struct Q8 {
    int v = 0;
    Q8() = default;
    Q8(int integer) : v(integer * 256) {}  // NOLINT: like bn::fixed(int)
    static auto raw(int value) -> Q8 {
        Q8 q;
        q.v = value;
        return q;
    }
    auto operator+=(Q8 o) -> Q8& {
        v += o.v;
        return *this;
    }
    auto operator-() const -> Q8 { return raw(-v); }
    friend auto operator+(Q8 a, Q8 b) -> Q8 { return raw(a.v + b.v); }
    friend auto operator-(Q8 a, Q8 b) -> Q8 { return raw(a.v - b.v); }
    friend auto operator*(Q8 a, int b) -> Q8 { return raw(a.v * b); }
    friend auto operator/(Q8 a, int b) -> Q8 { return raw(a.v / b); }
    friend auto operator<(Q8 a, Q8 b) -> bool { return a.v < b.v; }
    friend auto operator>(Q8 a, Q8 b) -> bool { return a.v > b.v; }
    friend auto operator<=(Q8 a, Q8 b) -> bool { return a.v <= b.v; }
    friend auto operator>=(Q8 a, Q8 b) -> bool { return a.v >= b.v; }
    [[nodiscard]] auto px() const -> double { return v / 256.0; }
};

void run(Rig& rig, int frames, double x, double y, double vel, int look = 0) {
    for (int i = 0; i < frames; ++i) {
        rig.update(x, y, vel, look);
    }
}

}  // namespace

TEST_CASE(camera_rig_constants_are_sane) {
    // A fraction below 1 per frame never overshoots its aim.
    CHECK(C::FOLLOW_X_NUM > 0 && C::FOLLOW_X_NUM < C::FOLLOW_X_DEN);
    CHECK(C::FOLLOW_Y_NUM > 0 && C::FOLLOW_Y_NUM < C::FOLLOW_Y_DEN);
    CHECK(C::LOOKAHEAD_NUM > 0 && C::LOOKAHEAD_NUM < C::LOOKAHEAD_DEN);
    CHECK(C::LOOK_Y_NUM > 0 && C::LOOK_Y_NUM < C::LOOK_Y_DEN);
    // The look stays well inside the screen (240x160).
    CHECK(C::LOOKAHEAD_PX > 0 && C::LOOKAHEAD_PX < 120 / 2);
    CHECK(C::LOOK_Y_PX > 0 && C::LOOK_Y_PX < 80 / 2);
    CHECK(C::LOOKAHEAD_MIN_SPEED >= 1);
    CHECK(C::LOOK_Y_HOLD_FRAMES >= 1);
}

TEST_CASE(camera_rig_max_offset_is_half_the_world_minus_half_the_screen) {
    CHECK_EQ(camera_rig::max_offset(624.0, 240), 192.0);
    CHECK_EQ(camera_rig::max_offset(380.0, 240), 70.0);
    CHECK_EQ(camera_rig::max_offset(160.0, 160), 0.0);
}

TEST_CASE(camera_rig_max_offset_is_never_negative) {
    CHECK_EQ(camera_rig::max_offset(240.0, 240), 0.0);
    CHECK_EQ(camera_rig::max_offset(100.0, 240), 0.0);
}

TEST_CASE(camera_rig_snap_puts_it_on_the_target_without_any_look) {
    Rig rig;
    run(rig, 60, 100, 20, 2);  // builds a look-ahead
    rig.snap(-50, 7);
    CHECK_EQ(rig.x(), -50.0);
    CHECK_EQ(rig.y(), 7.0);
    CHECK_EQ(rig.look_x(), 0.0);
    CHECK_EQ(rig.look_y(), 0.0);
}

TEST_CASE(camera_rig_standing_duck_is_reached_without_overshoot) {
    Rig rig;
    rig.snap(0, 0);
    double previous_gap = 1e9;
    for (int i = 0; i < 200; ++i) {
        rig.update(80, 30, 0, 0);
        const double gap = std::fabs(80 + rig.look_x() - rig.x());
        CHECK(rig.x() <= 80 + rig.look_x() + 1e-9);  // never past the aim
        previous_gap = gap;
    }
    CHECK(previous_gap < 0.01);
    CHECK(std::fabs(rig.y() - 30) < 0.01);
}

TEST_CASE(camera_rig_trails_a_running_duck_but_is_ahead_of_it_in_the_end) {
    Rig rig;
    rig.snap(0, 0);
    double duck = 0;
    for (int i = 0; i < 300; ++i) {
        duck += 2;
        rig.update(duck, 0, 2, 0);
    }
    // Steady state: look-ahead 20px, lag 2px/frame * (1 - k) / k = 10px.
    const double ahead = rig.x() - duck;
    CHECK(ahead > 5 && ahead < 15);
}

TEST_CASE(camera_rig_a_dash_is_followed_with_a_larger_trail) {
    Rig walk;
    Rig dash;
    walk.snap(0, 0);
    dash.snap(0, 0);
    double w = 0;
    double d = 0;
    for (int i = 0; i < 300; ++i) {
        w += 2;
        d += 4;
        walk.update(w, 0, 2, 0);
        dash.update(d, 0, 4, 0);
    }
    CHECK((w - walk.x()) < (d - dash.x()));
}

TEST_CASE(camera_rig_look_ahead_is_kept_when_the_duck_stops) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, 200, 0, 0, 2);
    const double look = rig.look_x();
    CHECK(look > 15);
    run(rig, 200, 0, 0, 0);  // standing still
    CHECK(std::fabs(rig.look_x() - look) < 1.0);
}

TEST_CASE(camera_rig_look_ahead_flips_when_the_duck_turns) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, 200, 0, 0, 2);
    CHECK(rig.look_x() > 0);
    run(rig, 200, 0, 0, -2);
    CHECK(rig.look_x() < -15);
}

TEST_CASE(camera_rig_creeping_does_not_change_the_direction) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, 200, 0, 0, 2);
    run(rig, 200, 0, 0, -0.5);  // below LOOKAHEAD_MIN_SPEED
    CHECK(rig.look_x() > 15);
}

TEST_CASE(camera_rig_look_up_needs_the_button_to_be_held) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, C::LOOK_Y_HOLD_FRAMES - 1, 0, 0, 0, -1);
    CHECK_EQ(rig.look_y(), 0.0);  // a tap does nothing
    run(rig, 200, 0, 0, 0, -1);
    CHECK(rig.look_y() < -C::LOOK_Y_PX + 1.0);
    CHECK(rig.y() < -C::LOOK_Y_PX + 1.0);
}

TEST_CASE(camera_rig_look_down_mirrors_look_up) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, 200, 0, 0, 0, 1);
    CHECK(rig.look_y() > C::LOOK_Y_PX - 1.0);
}

TEST_CASE(camera_rig_look_returns_to_the_duck_on_release) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, 200, 0, 0, 0, -1);
    run(rig, 300, 0, 0, 0, 0);
    CHECK(std::fabs(rig.look_y()) < 0.01);
    CHECK(std::fabs(rig.y()) < 0.1);
}

TEST_CASE(camera_rig_switching_look_direction_restarts_the_hold) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, 100, 0, 0, 0, -1);
    run(rig, 1, 0, 0, 0, 1);  // the first frame of looking down: not yet
    CHECK(rig.look_y() < 0);
}

TEST_CASE(camera_rig_bounds_keep_the_aim_inside_the_world) {
    Rig rig;
    rig.set_bounds(100, 0);
    rig.snap(0, 0);
    run(rig, 400, 500, 0, 2);  // duck far beyond the right edge
    CHECK(rig.x() <= 100.0 + 1e-9);
    CHECK(rig.x() > 99.0);
    CHECK_EQ(rig.y(), 0.0);
}

TEST_CASE(camera_rig_turning_at_a_world_edge_is_followed_at_once) {
    Rig rig;
    rig.set_bounds(100, 0);
    rig.snap(0, 0);
    run(rig, 400, 500, 0, 2);
    const double at_edge = rig.x();
    // The duck walks back left: without bounds the aim (still ~500) would have
    // to travel down from beyond the edge before the picture moved.
    run(rig, 30, 60, 0, -2);
    CHECK(rig.x() < at_edge - 5);
}

TEST_CASE(camera_rig_snap_respects_the_bounds) {
    Rig rig;
    rig.set_bounds(100, 0);
    rig.snap(500, 40);
    CHECK_EQ(rig.x(), 100.0);
    CHECK_EQ(rig.y(), 0.0);
}

TEST_CASE(camera_rig_without_bounds_follows_anywhere) {
    Rig rig;
    rig.snap(0, 0);
    run(rig, 400, 1000, -300, 0);
    CHECK(rig.x() > 990);
    CHECK(rig.y() < -290);
}

TEST_CASE(camera_rig_works_with_truncating_fixed_point_numbers) {
    CameraRig<Q8> rig;
    rig.snap(Q8(0), Q8(0));
    double previous = -1;
    for (int i = 0; i < 400; ++i) {
        rig.update(Q8(90), Q8(-20), Q8(0), 0);
        CHECK(rig.x().px() >= previous - 1e-9);  // monotonic, no oscillation
        previous = rig.x().px();
    }
    CHECK(std::fabs(rig.y().px() - (-20)) < 0.1);
    CHECK(rig.x().px() <= 90 + 1e-9);  // never past the aim
}

TEST_CASE(camera_rig_fixed_point_look_ahead_settles_on_the_full_distance) {
    CameraRig<Q8> rig;
    rig.snap(Q8(0), Q8(0));
    for (int i = 0; i < 600; ++i) {
        rig.update(Q8(0), Q8(0), Q8(2), 0);
    }
    CHECK(std::fabs(rig.look_x().px() - C::LOOKAHEAD_PX) < 0.5);
}

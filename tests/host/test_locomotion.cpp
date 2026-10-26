#include "test_framework.h"

#include "locomotion.h"
#include "world_index.h"

// The duck's per-frame movement rules (locomotion::Core), with the real
// bn::fixed arithmetic and Cfg constants of the game. FakeBody / FakeWorld
// stand in for the PhysicsBody and its collision probes.

namespace {

using locomotion::Core;
using locomotion::Effects;
using locomotion::Facing;

struct FakeBody {
    struct {
        bn::fixed x = 0;
        bn::fixed y = 0;
    } pos;
    bn::fixed vel_x = 0;
    bn::fixed vel_y = 0;
};

struct FakeWorld {
    bool ground = true;
    bool ceiling = false;
    bool wall_left = false;
    bool wall_right = false;
    int wall_probes = 0;
    int deaths = 0;
    FakeBody* body = nullptr;
    Core* core = nullptr;

    auto probe_ground() -> bool { return ground; }
    auto probe_ceiling() -> bool { return ceiling; }
    auto probe_left() -> bool {
        ++wall_probes;
        return wall_left;
    }
    auto probe_right() -> bool {
        ++wall_probes;
        return wall_right;
    }

    // Like Player::death(): back to the spawn point, standing still.
    void die() {
        ++deaths;
        body->pos.x = 0;
        body->pos.y = 0;
        body->vel_x = 0;
        body->vel_y = 0;
        core->reset_dash();
    }
};

struct Rig {
    Core core;
    FakeBody body;
    FakeWorld world;

    Rig() {
        world.body = &body;
        world.core = &core;
        core.set_horizontal_bound(1000);
    }

    auto step(const InputState& input = {}) -> Effects {
        return core.step(body, input, world);
    }

    // Several frames with the same input; the effects of the last one.
    auto steps(int frames, const InputState& input = {}) -> Effects {
        Effects last;
        for (int i = 0; i < frames; ++i) {
            last = step(input);
        }
        return last;
    }
};

auto right() -> InputState {
    InputState input;
    input.right_held = true;
    return input;
}

auto left() -> InputState {
    InputState input;
    input.left_held = true;
    return input;
}

auto jump() -> InputState {
    InputState input;
    input.a_pressed = true;
    input.a_held = true;
    return input;
}

auto hold_jump() -> InputState {
    InputState input;
    input.a_held = true;
    return input;
}

auto dash_right() -> InputState {
    InputState input;
    input.right_held = true;
    input.b_pressed = true;
    return input;
}

auto jump_right() -> InputState {
    InputState input = jump();
    input.right_held = true;
    return input;
}

// In the air, falling, at a wall on the right (wall jump enabled).
void fall_at_right_wall(Rig& rig) {
    rig.core.set_wall_jump_enabled(true);
    rig.world.ground = false;
    rig.world.wall_right = true;
    rig.steps(10);
}

}  // namespace

// ---- running
// -----------------------------------------------------------------

TEST_CASE(locomotion_holding_right_accelerates) {
    Rig rig;
    rig.step(right());
    CHECK_EQ(rig.body.vel_x, Cfg::Player::ACCELERATION);
    CHECK(rig.core.facing() == Facing::Right);
}

TEST_CASE(locomotion_running_is_capped_at_max_speed) {
    Rig rig;
    rig.steps(60, right());
    CHECK_EQ(rig.body.vel_x, bn::fixed(Cfg::Player::MAX_SPEED));
}

TEST_CASE(locomotion_holding_left_mirrors_right) {
    Rig rig;
    rig.step(left());
    CHECK_EQ(rig.body.vel_x, -Cfg::Player::ACCELERATION);
    CHECK(rig.core.facing() == Facing::Left);
    rig.steps(60, left());
    CHECK_EQ(rig.body.vel_x, -bn::fixed(Cfg::Player::MAX_SPEED));
}

TEST_CASE(locomotion_releasing_the_dpad_brakes_without_overshooting) {
    Rig rig;
    rig.body.vel_x = Cfg::Player::MAX_SPEED;
    rig.step();
    // The brake is DECELERATION * ground friction (1): bn::fixed's
    // multiplication drops low bits, so it is ~0.297 px/frame, not 0.3.
    CHECK_EQ(
        rig.body.vel_x, bn::fixed(Cfg::Player::MAX_SPEED) -
                            Cfg::Player::DECELERATION * bn::fixed(1));
    rig.steps(60);
    CHECK_EQ(rig.body.vel_x, bn::fixed(0));

    rig.body.vel_x = -Cfg::Player::MAX_SPEED;
    rig.steps(60);
    CHECK_EQ(rig.body.vel_x, bn::fixed(0));
}

TEST_CASE(locomotion_slippery_floor_brakes_less) {
    Rig rig;
    rig.core.set_ground_friction(bn::fixed(0.5));
    rig.body.vel_x = Cfg::Player::MAX_SPEED;
    rig.step();
    CHECK_EQ(
        rig.body.vel_x, bn::fixed(Cfg::Player::MAX_SPEED) -
                            Cfg::Player::DECELERATION * bn::fixed(0.5));
}

TEST_CASE(locomotion_friction_never_changes_acceleration) {
    Rig rig;
    rig.core.set_ground_friction(bn::fixed(0.25));
    rig.step(right());
    CHECK_EQ(rig.body.vel_x, Cfg::Player::ACCELERATION);
}

TEST_CASE(locomotion_footstep_sound_every_ten_frames_on_the_ground) {
    Rig rig;
    int sounds = 0;
    for (int frame = 0; frame < 30; ++frame) {
        if (rig.step(right()).step_sound) {
            ++sounds;
        }
    }
    CHECK_EQ(sounds, 3);
}

TEST_CASE(locomotion_no_footsteps_in_the_air) {
    Rig rig;
    rig.world.ground = false;
    rig.steps(10);  // coyote time over
    CHECK(!rig.steps(20, right()).step_sound);
}

// ---- falling
// -----------------------------------------------------------------

TEST_CASE(locomotion_gravity_pulls_every_frame) {
    Rig rig;
    rig.world.ground = false;
    rig.step();
    CHECK_EQ(rig.body.vel_y, Cfg::Player::GRAVITY);
    rig.step();
    CHECK_EQ(rig.body.vel_y, Cfg::Player::GRAVITY * 2);
}

TEST_CASE(locomotion_fall_speed_is_capped) {
    Rig rig;
    rig.world.ground = false;
    rig.steps(100);
    CHECK_EQ(rig.body.vel_y, bn::fixed(Cfg::Player::MAX_FALL_SPEED));
}

TEST_CASE(locomotion_falling_below_the_death_height_dies) {
    Rig rig;
    rig.world.ground = false;
    rig.body.pos.y = Cfg::Player::DEATH_HEIGHT + 1;
    rig.step();
    CHECK_EQ(rig.world.deaths, 1);
    CHECK_EQ(rig.body.pos.y, bn::fixed(0));
}

TEST_CASE(locomotion_at_the_death_height_is_still_alive) {
    Rig rig;
    rig.body.pos.y = Cfg::Player::DEATH_HEIGHT;
    rig.step();
    CHECK_EQ(rig.world.deaths, 0);
}

// ---- jumping
// -----------------------------------------------------------------

TEST_CASE(locomotion_a_on_the_ground_jumps) {
    Rig rig;
    rig.step();  // stand on the ground
    const Effects effects = rig.step(jump());
    CHECK(effects.jumped);
    CHECK_EQ(rig.body.vel_y, bn::fixed(Cfg::Player::JUMP_SPEED));
    CHECK(!rig.core.on_ground());
}

TEST_CASE(locomotion_no_jump_in_the_air) {
    Rig rig;
    rig.world.ground = false;
    rig.steps(Cfg::Player::COYOTE_FRAMES + 1);
    CHECK(!rig.step(jump()).jumped);
}

TEST_CASE(locomotion_releasing_a_cuts_the_jump) {
    Rig rig;
    rig.step();
    rig.step(jump());
    rig.world.ground = false;
    const bn::fixed rising = rig.body.vel_y;
    rig.step();  // A released
    bn::fixed expected = rising + Cfg::Player::GRAVITY;
    expected *= 0.5;
    CHECK_EQ(rig.body.vel_y, expected);
}

TEST_CASE(locomotion_holding_a_keeps_the_full_jump) {
    Rig rig;
    rig.step();
    rig.step(jump());
    rig.world.ground = false;
    const bn::fixed rising = rig.body.vel_y;
    rig.step(hold_jump());
    CHECK_EQ(rig.body.vel_y, rising + Cfg::Player::GRAVITY);
}

TEST_CASE(locomotion_a_ceiling_stops_the_jump) {
    Rig rig;
    rig.step();
    rig.world.ceiling = true;
    CHECK(rig.step(jump()).jumped);
    CHECK_EQ(rig.body.vel_y, bn::fixed(0));
}

TEST_CASE(locomotion_jump_buffer_jumps_on_landing) {
    // A pressed shortly before landing still jumps when the duck lands.
    for (int early = 1; early < Cfg::Player::JUMP_BUFFER_FRAMES; ++early) {
        Rig rig;
        rig.world.ground = false;
        rig.steps(10);
        rig.step(jump());
        rig.steps(early - 1, hold_jump());
        rig.world.ground = true;
        CHECK(rig.step(hold_jump()).jumped);
    }
}

TEST_CASE(locomotion_jump_buffer_expires) {
    Rig rig;
    rig.world.ground = false;
    rig.steps(10);
    rig.step(jump());
    rig.steps(Cfg::Player::JUMP_BUFFER_FRAMES - 1, hold_jump());
    rig.world.ground = true;
    CHECK(!rig.step(hold_jump()).jumped);
}

TEST_CASE(locomotion_coyote_time_allows_a_late_jump) {
    Rig rig;
    rig.steps(5);  // on the ground
    rig.world.ground = false;
    rig.steps(Cfg::Player::COYOTE_FRAMES - 1);
    CHECK(rig.step(jump()).jumped);
}

TEST_CASE(locomotion_coyote_time_runs_out) {
    Rig rig;
    rig.steps(5);
    rig.world.ground = false;
    rig.steps(Cfg::Player::COYOTE_FRAMES);
    CHECK(!rig.step(jump()).jumped);
}

TEST_CASE(locomotion_assist_mode_widens_coyote_time) {
    Rig rig;
    rig.core.set_assist_mode(true);
    rig.steps(5);
    rig.world.ground = false;
    rig.steps(Cfg::AssistMode::COYOTE_FRAMES - 1);
    CHECK(rig.step(jump()).jumped);
}

TEST_CASE(locomotion_assist_mode_widens_the_jump_buffer) {
    Rig rig;
    rig.core.set_assist_mode(true);
    rig.world.ground = false;
    rig.steps(20);
    rig.step(jump());
    rig.steps(Cfg::AssistMode::JUMP_BUFFER_FRAMES - 2, hold_jump());
    rig.world.ground = true;
    CHECK(rig.step(hold_jump()).jumped);
}

// ---- world edges ------------------------------------------------------------

TEST_CASE(locomotion_bounces_back_at_the_world_edge) {
    Rig rig;
    rig.core.set_horizontal_bound(100);
    rig.body.pos.x = 101;
    rig.body.vel_x = 1;
    rig.step();
    CHECK_EQ(
        rig.body.vel_x,
        -(bn::fixed(1) - Cfg::Player::DECELERATION * bn::fixed(1)));
}

// ---- dash ------------------------------------------------------------------

TEST_CASE(locomotion_dash_needs_to_be_enabled) {
    Rig rig;
    rig.world.ground = false;
    CHECK(!rig.step(dash_right()).dash_started);
    CHECK(!rig.core.dashing());
}

TEST_CASE(locomotion_dash_is_a_flat_burst) {
    Rig rig;
    rig.core.set_dash_enabled(true);
    rig.world.ground = false;
    rig.steps(10);
    const Effects effects = rig.step(dash_right());
    CHECK(effects.dash_started);
    CHECK(rig.core.dashing());
    CHECK_EQ(rig.body.vel_x, bn::fixed(Cfg::Dash::SPEED));
    CHECK_EQ(rig.body.vel_y, bn::fixed(0));
    rig.steps(2, right());
    CHECK_EQ(rig.body.vel_y, bn::fixed(0));  // no gravity while dashing
}

TEST_CASE(locomotion_dash_hands_over_at_running_speed) {
    Rig rig;
    rig.core.set_dash_enabled(true);
    rig.world.ground = false;
    rig.steps(10);
    rig.step(dash_right());
    rig.steps(Cfg::Dash::DURATION_FRAMES + 1, right());
    CHECK(!rig.core.dashing());
    CHECK_EQ(rig.body.vel_x, bn::fixed(Cfg::Player::MAX_SPEED));
}

TEST_CASE(locomotion_dash_turns_the_duck) {
    Rig rig;
    rig.core.set_dash_enabled(true);
    rig.step(left());
    InputState dash = dash_right();
    rig.step(dash);
    CHECK(rig.core.facing() == Facing::Right);
}

TEST_CASE(locomotion_dying_ends_the_dash) {
    Rig rig;
    rig.core.set_dash_enabled(true);
    rig.world.ground = false;
    rig.steps(10);
    rig.step(dash_right());
    rig.body.pos.y = Cfg::Player::DEATH_HEIGHT + 1;
    rig.step(right());
    CHECK(!rig.core.dashing());
}

TEST_CASE(locomotion_one_dash_per_air_time) {
    Rig rig;
    rig.core.set_dash_enabled(true);
    rig.world.ground = false;
    rig.steps(10);
    CHECK(rig.step(dash_right()).dash_started);
    rig.steps(Cfg::Dash::DURATION_FRAMES + Cfg::Dash::COOLDOWN_FRAMES + 5);
    CHECK(!rig.step(dash_right()).dash_started);
}

TEST_CASE(locomotion_landing_refills_the_air_dash) {
    Rig rig;
    rig.core.set_dash_enabled(true);
    rig.world.ground = false;
    rig.steps(10);
    rig.step(dash_right());
    rig.steps(Cfg::Dash::DURATION_FRAMES + Cfg::Dash::COOLDOWN_FRAMES + 5);
    rig.world.ground = true;
    rig.step();
    rig.world.ground = false;
    rig.steps(10);
    CHECK(rig.step(dash_right()).dash_started);
}

// ---- wall slide and wall jump
// ------------------------------------------------

TEST_CASE(wall_jump_needs_to_be_enabled) {
    Rig rig;
    rig.world.ground = false;
    rig.world.wall_right = true;
    rig.steps(10);
    CHECK(!rig.step(jump_right()).jumped);
    rig.steps(40, right());
    CHECK_EQ(rig.body.vel_y, bn::fixed(Cfg::Player::MAX_FALL_SPEED));
}

TEST_CASE(wall_jump_disabled_never_probes_walls) {
    // Two extra collision queries per frame on the GBA; only worth it where
    // wall jumps exist.
    Rig rig;
    rig.world.ground = false;
    rig.steps(10, right());
    rig.step(jump_right());
    CHECK_EQ(rig.world.wall_probes, 0);
}

TEST_CASE(wall_slide_caps_the_fall_speed) {
    Rig rig;
    fall_at_right_wall(rig);
    rig.steps(40, right());
    CHECK_EQ(rig.body.vel_y, bn::fixed(Cfg::WallJump::SLIDE_SPEED));
}

TEST_CASE(wall_slide_needs_the_duck_to_hold_toward_the_wall) {
    Rig rig;
    fall_at_right_wall(rig);
    rig.steps(40);
    CHECK_EQ(rig.body.vel_y, bn::fixed(Cfg::Player::MAX_FALL_SPEED));

    Rig away;
    fall_at_right_wall(away);
    away.steps(40, left());
    CHECK_EQ(away.body.vel_y, bn::fixed(Cfg::Player::MAX_FALL_SPEED));
}

TEST_CASE(wall_slide_never_slows_a_rise) {
    Rig rig;
    fall_at_right_wall(rig);
    rig.body.vel_y = -3;
    rig.step(hold_jump());
    CHECK_EQ(rig.body.vel_y, bn::fixed(-3) + Cfg::Player::GRAVITY);
}

TEST_CASE(wall_jump_pushes_away_from_the_wall) {
    Rig rig;
    fall_at_right_wall(rig);
    const Effects effects = rig.step(jump_right());
    CHECK(effects.jumped);
    CHECK_EQ(rig.body.vel_y, bn::fixed(Cfg::Player::JUMP_SPEED));
    CHECK_EQ(rig.body.vel_x, -bn::fixed(Cfg::WallJump::PUSH_SPEED));
    CHECK(rig.core.facing() == Facing::Left);
}

TEST_CASE(wall_jump_off_a_left_wall_mirrors) {
    Rig rig;
    rig.core.set_wall_jump_enabled(true);
    rig.world.ground = false;
    rig.world.wall_left = true;
    rig.steps(10);
    InputState input = jump();
    input.left_held = true;
    CHECK(rig.step(input).jumped);
    CHECK_EQ(rig.body.vel_x, bn::fixed(Cfg::WallJump::PUSH_SPEED));
    CHECK(rig.core.facing() == Facing::Right);
}

TEST_CASE(wall_jump_ignores_the_dpad_briefly) {
    Rig rig;
    fall_at_right_wall(rig);
    rig.step(jump_right());
    rig.world.wall_right = false;  // left the wall
    InputState toward = right();
    toward.a_held = true;
    rig.steps(Cfg::WallJump::LOCK_FRAMES, toward);
    CHECK_EQ(rig.body.vel_x, -bn::fixed(Cfg::WallJump::PUSH_SPEED));
    rig.step(toward);
    CHECK_EQ(
        rig.body.vel_x,
        -bn::fixed(Cfg::WallJump::PUSH_SPEED) + Cfg::Player::ACCELERATION);
}

TEST_CASE(wall_jump_ground_jump_wins_next_to_a_wall) {
    Rig rig;
    rig.core.set_wall_jump_enabled(true);
    rig.world.wall_right = true;
    rig.step();
    CHECK(rig.step(jump()).jumped);
    CHECK_EQ(rig.body.vel_x, bn::fixed(0));
}

TEST_CASE(wall_jump_uses_the_jump_buffer) {
    // A pressed shortly before touching the wall still jumps off it.
    Rig rig;
    rig.core.set_wall_jump_enabled(true);
    rig.world.ground = false;
    rig.steps(10);
    rig.step(jump_right());
    InputState toward = right();
    toward.a_held = true;
    rig.steps(2, toward);
    rig.world.wall_right = true;
    CHECK(rig.step(toward).jumped);
    CHECK_EQ(rig.body.vel_x, -bn::fixed(Cfg::WallJump::PUSH_SPEED));
}

TEST_CASE(wall_jump_once_per_press) {
    Rig rig;
    fall_at_right_wall(rig);
    CHECK(rig.step(jump_right()).jumped);
    InputState toward = right();
    toward.a_held = true;
    CHECK(!rig.step(toward).jumped);
}

TEST_CASE(wall_jump_keeps_the_air_dash_used) {
    // Only real ground refills the air dash; a wall does not.
    Rig rig;
    rig.core.set_dash_enabled(true);
    rig.world.ground = false;
    rig.steps(10);
    CHECK(rig.step(dash_right()).dash_started);
    rig.steps(Cfg::Dash::DURATION_FRAMES + Cfg::Dash::COOLDOWN_FRAMES + 5);
    fall_at_right_wall(rig);
    rig.step(jump_right());
    rig.steps(Cfg::WallJump::LOCK_FRAMES + 1);
    CHECK(!rig.step(dash_right()).dash_started);
}

TEST_CASE(wall_jump_constants_keep_the_slide_slower_than_a_fall) {
    CHECK(Cfg::WallJump::SLIDE_SPEED > 0);
    CHECK(Cfg::WallJump::SLIDE_SPEED < Cfg::Player::MAX_FALL_SPEED);
    CHECK(Cfg::WallJump::PUSH_SPEED <= Cfg::Player::MAX_SPEED);
    CHECK(Cfg::WallJump::LOCK_FRAMES > 0);
    CHECK(Cfg::WallJump::UNLOCK_WORLD >= 0);
    CHECK(Cfg::WallJump::UNLOCK_WORLD < WorldIndex::WORLD_COUNT);
}

TEST_CASE(wall_jump_landing_ends_the_dpad_lock) {
    Rig rig;
    fall_at_right_wall(rig);
    rig.step(jump_right());
    rig.world.wall_right = false;
    rig.world.ground = true;
    rig.step();  // lands
    rig.step(right());
    CHECK_EQ(
        rig.body.vel_x,
        -bn::fixed(Cfg::WallJump::PUSH_SPEED) + Cfg::Player::ACCELERATION);
}

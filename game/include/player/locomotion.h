#pragma once

#include "bn_fixed.h"

#include "cfg.h"
#include "dash_controller.h"

// -----------------------------------------------------------------------------
// InputState
//
// The buttons of one frame, read once from bn::keypad by PlayerLocomotion so
// the movement rules below never touch the hardware (and tests can press
// buttons).
// -----------------------------------------------------------------------------
struct InputState {
    bool left_held = false;
    bool right_held = false;
    bool a_pressed = false;
    bool a_held = false;
    bool b_pressed = false;
};

namespace locomotion {

enum class Facing { Forward, Back, Left, Right };

// What a frame did that PlayerLocomotion turns into sound and events.
struct Effects {
    bool jumped = false;
    bool dash_started = false;
    bool step_sound = false;
};

// -----------------------------------------------------------------------------
// Core
//
// The duck's per-frame movement rules: running and braking, gravity, jump
// buffer and coyote time, variable jump height, the world-edge bounce, the
// fall death and the dash. No Butano runtime and no hardware: step() works on
// any Body with `pos.x`, `pos.y`, `vel_x`, `vel_y` (PhysicsBody in the game)
// and asks a World for the collision probes and the death:
//
//   bool probe_ground();   // something solid 1 px below the body
//   bool probe_ceiling();  // something solid 1 px above the body
//   void die();            // respawn (may move the body, calls reset_dash())
//
// Host-tested in tests/host/test_locomotion.cpp with the game's bn::fixed.
// -----------------------------------------------------------------------------
class Core {
   public:
    template <typename Body, typename World>
    auto step(Body& body, const InputState& in, World& world) -> Effects {
        Effects effects;

        // Store jump input in a buffer to allow forgiving timing. Done before
        // the dash step: a buffered jump on the ground also cancels a running
        // dash into a jump (see DashController).
        if (in.a_pressed) {
            _jump_buffer_timer = _jump_buffer_frames;
        }

        // While a dash runs it owns the whole frame: input, gravity, variable
        // jump, clamping, bounds and jumping are skipped (only the fall-death
        // check and the ground state still run). On the frame a dash ends
        // _update_dash() returns false, so normal movement resumes at once.
        const bool dash_owns_frame = _update_dash(body, in, effects);

        if (!dash_owns_frame) {
            _run(body, in, effects);

            body.vel_y += _gravity;
            // Variable jump height: releasing A cuts the rise.
            if (!in.a_held && body.vel_y < 0) {
                body.vel_y *= 0.5;
            }
            if (body.vel_y > _max_fall_speed) {
                body.vel_y = _max_fall_speed;
            }

            // Bounce back when leaving the level's play area.
            if (body.pos.x < -_horizontal_bound ||
                body.pos.x > _horizontal_bound) {
                body.vel_x = -body.vel_x;
            }
        }

        if (body.pos.y > _death_height) {
            world.die();
        }

        _update_ground(world);

        if (!dash_owns_frame) {
            _jump(body, world, effects);
        }

        if (_jump_buffer_timer > 0) {
            --_jump_buffer_timer;
        }
        return effects;
    }

    [[nodiscard]] auto on_ground() const -> bool { return _on_ground; }
    [[nodiscard]] auto facing() const -> Facing { return _facing; }
    [[nodiscard]] auto dashing() const -> bool { return _dash.dashing(); }

    // The world-edge bounce (world_width / 2 - Cfg::Screen::EDGE_MARGIN).
    void set_horizontal_bound(bn::fixed bound) { _horizontal_bound = bound; }

    // Scales braking only (1 = normal, < 1 = slippery). Acceleration, gravity
    // and jump speed stay, so it never changes how far a jump reaches.
    void set_ground_friction(bn::fixed scale) {
        _ground_friction_scale = scale;
    }

    // Assist Mode: the wider Cfg::AssistMode coyote / jump buffer windows.
    // Only forgiveness timing changes, never the jump envelope.
    void set_assist_mode(bool enabled) {
        _coyote_frames = enabled ? Cfg::AssistMode::COYOTE_FRAMES
                                 : Cfg::Player::COYOTE_FRAMES;
        _jump_buffer_frames = enabled ? Cfg::AssistMode::JUMP_BUFFER_FRAMES
                                      : Cfg::Player::JUMP_BUFFER_FRAMES;
    }

    // Changing the flag also cancels a running dash and refills the air dash.
    void set_dash_enabled(bool enabled) { _dash.set_enabled(enabled); }

    // Cancels a running dash and clears its cooldown / air use (death, level
    // load).
    void reset_dash() { _dash.reset(); }

   private:
    // Feeds DashController and applies what it decides. Returns true while
    // the dash owns the frame.
    template <typename Body>
    auto _update_dash(Body& body, const InputState& in, Effects& effects)
        -> bool {
        DashInput dash;
        dash.dash_pressed = in.b_pressed;
        dash.left_held = in.left_held;
        dash.right_held = in.right_held;
        dash.facing_left = _facing == Facing::Left;
        dash.jump_buffered = _jump_buffer_timer > 0;
        dash.on_ground = _on_ground;
        // A wall zeroes vel_x inside PhysicsBody::move(); step() runs before
        // the move, so this is the result of the previous frame's step.
        dash.vel_x_zero = body.vel_x == 0;
        dash.at_left_edge = body.pos.x <= -_horizontal_bound;
        dash.at_right_edge = body.pos.x >= _horizontal_bound;

        const DashStep step = _dash.update(dash);

        if (step.started) {
            _facing = step.dir < 0 ? Facing::Left : Facing::Right;
            _walk_sound_counter = 0;
            effects.dash_started = true;
        }
        if (step.ended) {
            // Hand over to normal movement at walking top speed, no vertical
            // velocity (a buffered jump on the ground follows in _jump()).
            body.vel_x = bn::fixed(step.dir) * _max_speed;
            body.vel_y = 0;
        }
        if (step.moving) {
            // Gravity is suspended: flat horizontal burst.
            body.vel_x = bn::fixed(step.dir) * Cfg::Dash::SPEED;
            body.vel_y = 0;
        }
        return step.moving;
    }

    template <typename Body>
    void _run(Body& body, const InputState& in, Effects& effects) {
        if (in.left_held) {
            body.vel_x -= _acceleration;
            if (body.vel_x < -_max_speed) {
                body.vel_x = -_max_speed;
            }
            _facing = Facing::Left;
        } else if (in.right_held) {
            body.vel_x += _acceleration;
            if (body.vel_x > _max_speed) {
                body.vel_x = _max_speed;
            }
            _facing = Facing::Right;
        } else {
            // Braking toward zero, never past it. Uses _deceleration (not
            // _acceleration) so a level's ground friction only changes the
            // stopping feel.
            const bn::fixed brake = _deceleration * _ground_friction_scale;
            if (body.vel_x > 0) {
                body.vel_x -= brake;
                if (body.vel_x < 0) {
                    body.vel_x = 0;
                }
            } else if (body.vel_x < 0) {
                body.vel_x += brake;
                if (body.vel_x > 0) {
                    body.vel_x = 0;
                }
            }
        }

        // A footstep every WALK_SOUND_FRAMES while running on the ground.
        if ((in.left_held || in.right_held) && _on_ground) {
            effects.step_sound = _walk_sound_counter == 0;
            if (++_walk_sound_counter >= WALK_SOUND_FRAMES) {
                _walk_sound_counter = 0;
            }
        } else {
            _walk_sound_counter = 0;
        }
    }

    // Ground contact and coyote time.
    template <typename World>
    void _update_ground(World& world) {
        if (world.probe_ground()) {
            _on_ground = true;
            _coyote_timer = _coyote_frames;
            // Real ground contact (never coyote time) refills the air dash.
            _dash.land();
        } else if (_coyote_timer > 0) {
            --_coyote_timer;
            _on_ground = true;
        } else {
            _on_ground = false;
        }
    }

    // A buffered jump while grounded; a ceiling stops any upward motion.
    template <typename Body, typename World>
    void _jump(Body& body, World& world, Effects& effects) {
        if (_jump_buffer_timer > 0 && _on_ground) {
            body.vel_y = _jump_speed;
            _on_ground = false;
            _jump_buffer_timer = 0;
            _coyote_timer = 0;
            effects.jumped = true;
        }
        if (body.vel_y < 0 && world.probe_ceiling()) {
            body.vel_y = 0;
        }
    }

    static constexpr int WALK_SOUND_FRAMES = 10;

    bn::fixed _acceleration = Cfg::Player::ACCELERATION;
    // Braking constant, separate from _acceleration - see the comment on
    // Cfg::Player::DECELERATION. Scaled per level by _ground_friction_scale.
    bn::fixed _deceleration = Cfg::Player::DECELERATION;
    bn::fixed _max_speed = Cfg::Player::MAX_SPEED;
    bn::fixed _jump_speed = Cfg::Player::JUMP_SPEED;
    bn::fixed _gravity = Cfg::Player::GRAVITY;
    bn::fixed _max_fall_speed = Cfg::Player::MAX_FALL_SPEED;
    int _death_height = Cfg::Player::DEATH_HEIGHT;

    bool _on_ground = true;
    Facing _facing = Facing::Forward;

    int _coyote_timer = 0;
    int _jump_buffer_timer = 0;
    int _coyote_frames = Cfg::Player::COYOTE_FRAMES;
    int _jump_buffer_frames = Cfg::Player::JUMP_BUFFER_FRAMES;

    int _walk_sound_counter = 0;

    bn::fixed _horizontal_bound = Cfg::Screen::HORIZONTAL_EDGE;
    bn::fixed _ground_friction_scale = 1;

    DashController _dash{
        Cfg::Dash::DURATION_FRAMES, Cfg::Dash::COOLDOWN_FRAMES};
};

}  // namespace locomotion

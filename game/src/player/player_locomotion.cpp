#include "player_locomotion.h"

#include "audio_settings.h"
#include "bn_keypad.h"
#include "bn_sound_items.h"
#include "player_events.h"

PlayerLocomotion::PlayerLocomotion(
    PhysicsBody& body,
    Sprite& sprite,
    uint16_t block_mask,
    PlayerDeathHandler& death_handler)
    : _body(body),
      _sprite(sprite),
      _block_mask(block_mask),
      _death_handler(death_handler) {}

void PlayerLocomotion::update() {
    // Events of the previous frame are over (see PlayerEvents).
    PlayerEvents::instance().begin_frame();

    // Store jump input in a buffer to allow forgiving timing. Done before the
    // dash step: a buffered jump on the ground also cancels a running dash
    // into a jump (see DashController).
    if (bn::keypad::a_pressed()) {
        _jump_buffer_timer = _jump_buffer_frames;
    }

    // The dash is checked first. While one runs it owns the whole frame:
    // input, gravity, variable jump, clamping, bounds and jumping are skipped
    // (only the fall-death check and the ground state still run). On the frame
    // a dash ends update_dash() returns false, so normal movement resumes at
    // once.
    const bool dash_owns_frame = update_dash();

    if (!dash_owns_frame) {
        // Handle left/right movement.
        handle_horizontal_input();

        // Apply continuous vertical motion effects.
        apply_gravity();
        apply_variable_jump();
        clamp_velocity();

        // Keep the player inside bounds.
        check_bounds();
    }

    // Detect death conditions.
    check_death();

    update_ground_state();

    if (!dash_owns_frame) {
        handle_jump();
    }

    // Decrease jump buffer timer
    if (_jump_buffer_timer > 0) {
        _jump_buffer_timer--;
    }
}

// Dash (B): feeds this frame's input and surroundings to DashController and
// applies what it decides (velocity, facing, sound). Returns true while the
// dash owns the frame.
bool PlayerLocomotion::update_dash() {
    DashInput in;
    in.dash_pressed = bn::keypad::b_pressed();
    in.left_held = bn::keypad::left_held();
    in.right_held = bn::keypad::right_held();
    in.facing_left = _facing == Facing::Left;
    in.jump_buffered = _jump_buffer_timer > 0;
    in.on_ground = _on_ground;
    // A wall zeroes vel_x inside PhysicsBody::move(); update() runs before
    // the move, so this is the result of the previous frame's step.
    in.vel_x_zero = _body.vel_x == 0;
    in.at_left_edge = _body.pos.x <= -_horizontal_bound;
    in.at_right_edge = _body.pos.x >= _horizontal_bound;

    const DashStep step = _dash.update(in);

    if (step.started) {
        _facing = step.dir < 0 ? Facing::Left : Facing::Right;
        _sprite.sprite().set_horizontal_flip(step.dir < 0);
        _walk_sound_counter = 0;

        AudioSettings::instance().play_sfx(bn::sound_items::dash);
        PlayerEvents::instance().note_dash();
    }

    if (step.ended) {
        // Hand over to normal movement at walking top speed, no vertical
        // velocity (a buffered jump on the ground follows in handle_jump()).
        _body.set_velocity(bn::fixed(step.dir) * _max_speed, 0);
    }

    if (step.moving) {
        // Gravity is suspended: flat horizontal burst.
        _body.set_velocity(bn::fixed(step.dir) * Cfg::Dash::SPEED, 0);
    }

    return step.moving;
}

// Horizontal input handling
void PlayerLocomotion::handle_horizontal_input() {
    if (bn::keypad::left_held()) {
        _body.inc_velocity(-_acceleration, 0);

        if (_body.vel_x < -_max_speed) {
            _body.set_velocity(-_max_speed, _body.vel_y);
        }

        _facing = Facing::Left;
        _sprite.sprite().set_horizontal_flip(true);

    } else if (bn::keypad::right_held()) {
        _body.inc_velocity(_acceleration, 0);

        if (_body.vel_x > _max_speed) {
            _body.set_velocity(_max_speed, _body.vel_y);
        }

        _facing = Facing::Right;
        _sprite.sprite().set_horizontal_flip(false);

    } else {
        // No horizontal input: apply friction to slow the player down.
        // Uses _deceleration (not _acceleration) so a level's
        // ground_friction only changes stopping feel, never how quickly
        // the duck speeds up or how far a jump reaches.
        _body.dec_velocity(_deceleration * _ground_friction_scale, 0);
    }

    bool moving = bn::keypad::left_held() || bn::keypad::right_held();
    if (moving && _on_ground) {
        if (_walk_sound_counter == 0) {
            AudioSettings::instance().play_sfx(bn::sound_items::duck_step);
        }
        if (++_walk_sound_counter >= 10) {
            _walk_sound_counter = 0;
        }
    } else {
        _walk_sound_counter = 0;
    }
}

// Jump logic: perform jump when buffered input exists and the player is
// grounded.
void PlayerLocomotion::handle_jump() {
    if (_jump_buffer_timer > 0 && _on_ground) {
        _body.set_velocity(_body.vel_x, _jump_speed);
        _on_ground = false;
        _jump_buffer_timer = 0;
        _coyote_timer = 0;

        AudioSettings::instance().play_sfx(bn::sound_items::jump);
        PlayerEvents::instance().note_jump();
    }

    // Cancel upward motion when hitting a ceiling.
    if (_body.vel_y < 0 && _body.probe_top(_block_mask).any()) {
        _body.set_velocity(_body.vel_x, 0);
    }
}

// Add gravity to vertical velocity each frame.
void PlayerLocomotion::apply_gravity() {
    _body.inc_velocity(0, _gravity);
}

// Variable jump height: cut the jump short when the button is released.
void PlayerLocomotion::apply_variable_jump() {
    if (!bn::keypad::a_held() && _body.vel_y < 0) {
        _body.vel_y *= 0.5;  // cut upward velocity for shorter jumps
    }
}

// Prevent the player from falling too fast.
void PlayerLocomotion::clamp_velocity() {
    if (_body.vel_y > _max_fall_speed) {
        _body.set_velocity(_body.vel_x, _max_fall_speed);
    }
}

// Bounce the player back when leaving the current level's play area.
void PlayerLocomotion::check_bounds() {
    if (_body.pos.x < -_horizontal_bound || _body.pos.x > _horizontal_bound) {
        _body.set_velocity(-_body.vel_x, _body.vel_y);
    }
}

// Determine if the player is grounded and manage coyote time.
void PlayerLocomotion::update_ground_state() {
    if (_body.probe_bottom(_block_mask).any()) {
        _on_ground = true;
        _coyote_timer = _coyote_frames;
        // Real ground contact (never coyote time) refills the air dash.
        _dash.land();
    } else {
        if (_coyote_timer > 0) {
            _coyote_timer--;
            _on_ground = true;
        } else {
            _on_ground = false;
        }
    }
}

// Fall below the death height triggers a respawn.
void PlayerLocomotion::check_death() {
    if (_body.pos.y > _death_height) {
        _death_handler.on_locomotion_death();
    }
}
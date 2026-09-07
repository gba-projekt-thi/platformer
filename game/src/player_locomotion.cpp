#include "player_locomotion.h"

#include "bn_keypad.h"
#include "bn_sound_items.h"

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
    // Handle left/right movement first.
    handle_horizontal_input();

    // Store jump input in a buffer to allow forgiving timing.
    if (bn::keypad::a_pressed()) {
        jump_buffer_timer = Cfg::Player::JUMP_BUFFER_FRAMES;
    }

    // Apply continuous vertical motion effects.
    apply_gravity();
    apply_variable_jump();
    clamp_velocity();

    // Keep the player inside bounds and detect death conditions.
    check_bounds();
    check_death();

    update_ground_state();
    handle_jump();

    // Decrease jump buffer timer
    if (jump_buffer_timer > 0) {
        jump_buffer_timer--;
    }
}

// Horizontal input handling
void PlayerLocomotion::handle_horizontal_input() {
    if (bn::keypad::left_held()) {
        _body.inc_velocity(-acceleration, 0);

        if (_body.vel_x < -max_speed) {
            _body.set_velocity(-max_speed, _body.vel_y);
        }

        _facing = Facing::Left;
        _sprite.sprite().set_horizontal_flip(true);

    } else if (bn::keypad::right_held()) {
        _body.inc_velocity(acceleration, 0);

        if (_body.vel_x > max_speed) {
            _body.set_velocity(max_speed, _body.vel_y);
        }

        _facing = Facing::Right;
        _sprite.sprite().set_horizontal_flip(false);

    } else {
        // No horizontal input: apply friction to slow the player down.
        _body.dec_velocity(acceleration, 0);
    }

    bool moving = bn::keypad::left_held() || bn::keypad::right_held();
    if (moving && _onGround) {
        if (_walk_sound_counter == 0) {
            bn::sound_items::duck_step.play();
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
    if (jump_buffer_timer > 0 && _onGround) {
        _body.set_velocity(_body.vel_x, jump_speed);
        _onGround = false;
        jump_buffer_timer = 0;
        coyote_timer = 0;

        bn::sound_items::jump.play();
    }

    // Cancel upward motion when hitting a ceiling.
    if (_body.vel_y < 0 && _body.probe_top(_block_mask).any()) {
        _body.set_velocity(_body.vel_x, 0);
    }
}

// Add gravity to vertical velocity each frame.
void PlayerLocomotion::apply_gravity() {
    _body.inc_velocity(0, gravity);
}

// Variable jump height: cut the jump short when the button is released.
void PlayerLocomotion::apply_variable_jump() {
    if (!bn::keypad::a_held() && _body.vel_y < 0) {
        _body.vel_y *= 0.5;  // cut upward velocity for shorter jumps
    }
}

// Prevent the player from falling too fast.
void PlayerLocomotion::clamp_velocity() {
    if (_body.vel_y > max_fall_speed) {
        _body.set_velocity(_body.vel_x, max_fall_speed);
    }
}

// Bounce the player back when leaving the horizontal play area.
void PlayerLocomotion::check_bounds() {
    if (_body.pos.x < -Cfg::Screen::HORIZONTAL_EDGE ||
        _body.pos.x > Cfg::Screen::HORIZONTAL_EDGE) {
        _body.set_velocity(-_body.vel_x, _body.vel_y);
    }
}

// Determine if the player is grounded and manage coyote time.
void PlayerLocomotion::update_ground_state() {
    if (_body.probe_bottom(_block_mask).any()) {
        _onGround = true;
        coyote_timer = Cfg::Player::COYOTE_FRAMES;
    } else {
        if (coyote_timer > 0) {
            coyote_timer--;
            _onGround = true;
        } else {
            _onGround = false;
        }
    }
}

// Fall below the death height triggers a respawn.
void PlayerLocomotion::check_death() {
    if (_body.pos.y > deathHeight) {
        _death_handler.on_locomotion_death();
    }
}
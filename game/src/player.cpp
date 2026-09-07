#include "player.h"

Player::Player(
    bn::fixed in_start_x,
    bn::fixed in_start_y,
    bn::fixed in_width,
    bn::fixed in_height)
    : PhysicsBody(
          in_start_x,
          in_start_y,
          in_width,
          in_height,
          Cfg::Layer::PLAYER,
          MASK,
          BLOCK),

      player_sprite(
          bn::sprite_items::ente.create_sprite(in_start_x, in_start_y),
          in_start_x,
          in_start_y),

      _animator(player_sprite, bn::sprite_items::ente),

      _hud(),

      restart_x(in_start_x),
      restart_y(in_start_y),

      acceleration(Cfg::Player::ACCELERATION),
      max_speed(Cfg::Player::MAX_SPEED),
      jump_speed(Cfg::Player::JUMP_SPEED),
      gravity(Cfg::Player::GRAVITY),
      max_fall_speed(Cfg::Player::MAX_FALL_SPEED),
      deathHeight(Cfg::Player::DEATH_HEIGHT),

      onGround(true),
      facing(Facing::Forward) {
    // Link the physics body with the player sprite for rendering.
    sprite = &player_sprite;
    player_sprite.sprite().set_blending_enabled(true);
}

void Player::update() {
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

    // Update the current movement state and animation.
    _state_machine.update(onGround, vel_y);
    _animator.update(onGround);

    // Decrease jump buffer timer
    if (jump_buffer_timer > 0) {
        jump_buffer_timer--;
    }

    // Update HUD (timer ticks every frame; death counter redraws on change).
    _hud.tick();
}

// sets spawnpoint
void Player::set_spawn_point(bn::fixed in_x, bn::fixed in_y) {
    restart_x = in_x;
    restart_y = in_y;
}
//
void Player::teleport_to(bn::fixed in_x, bn::fixed in_y) {
    set_velocity(0, 0);
    pos.x = in_x;
    pos.y = in_y;
}

unsigned int Player::get_deaths() const {
    return _hud.deaths();
}

void Player::set_deaths(unsigned int deaths) {
    _hud.set_deaths(deaths);
}

Timer& Player::get_timer() {
    return _hud.timer();
}

void Player::set_visible(bool visible) {
    if (visible) {
        player_sprite.enable();
    } else {
        player_sprite.disable();
    }
}

bool Player::visible() const {
    return player_sprite.is_enabled();
}

void Player::set_hud_visible(bool visible) {
    _hud.set_visible(visible);
}

// Horizontal input handling
void Player::handle_horizontal_input() {
    if (bn::keypad::left_held()) {
        inc_velocity(-acceleration, 0);

        if (vel_x < -max_speed) {
            set_velocity(-max_speed, vel_y);
        }

        facing = Facing::Left;
        player_sprite.sprite().set_horizontal_flip(true);

    } else if (bn::keypad::right_held()) {
        inc_velocity(acceleration, 0);

        if (vel_x > max_speed) {
            set_velocity(max_speed, vel_y);
        }

        facing = Facing::Right;
        player_sprite.sprite().set_horizontal_flip(false);

    } else {
        // No horizontal input: apply friction to slow the player down.
        dec_velocity(acceleration, 0);
    }

    bool moving = bn::keypad::left_held() || bn::keypad::right_held();
    if (moving && onGround) {
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
void Player::handle_jump() {
    if (jump_buffer_timer > 0 && onGround) {
        set_velocity(vel_x, jump_speed);
        onGround = false;
        jump_buffer_timer = 0;
        coyote_timer = 0;

        bn::sound_items::jump.play();
    }

    // Cancel upward motion when hitting a ceiling.
    if (vel_y < 0 && probe_top(BLOCK).any()) {
        set_velocity(vel_x, 0);
    }
}

// Add gravity to vertical velocity each frame.
void Player::apply_gravity() {
    inc_velocity(0, gravity);
}

// Variable jump height: cut the jump short when the button is released.
void Player::apply_variable_jump() {
    if (!bn::keypad::a_held() && vel_y < 0) {
        vel_y *= 0.5;  // cut upward velocity for shorter jumps
    }
}

// Prevent the player from falling too fast.
void Player::clamp_velocity() {
    if (vel_y > max_fall_speed) {
        set_velocity(vel_x, max_fall_speed);
    }
}

// Bounce the player back when leaving the horizontal play area.
void Player::check_bounds() {
    if (pos.x < -Cfg::Screen::HORIZONTAL_EDGE ||
        pos.x > Cfg::Screen::HORIZONTAL_EDGE) {
        set_velocity(-vel_x, vel_y);
    }
}

// Determine if the player is grounded and manage coyote time.
void Player::update_ground_state() {
    if (probe_bottom(BLOCK).any()) {
        onGround = true;
        coyote_timer = Cfg::Player::COYOTE_FRAMES;
    } else {
        if (coyote_timer > 0) {
            coyote_timer--;
            onGround = true;
        } else {
            onGround = false;
        }
    }
}

// Fall below the death height triggers a respawn.
void Player::check_death() {
    if (pos.y > deathHeight) {
        death();
    }
}

// Handle player death, increment the counter and respawn.
void Player::death() {
    _hud.on_player_death();

    set_velocity(0, 0);

    pos.x = restart_x;
    pos.y = restart_y;

    bn::sound_items::duck_death.play();
}
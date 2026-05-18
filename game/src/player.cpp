#include "player.h"

Player::Player(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height)
    : PhysicsBody(
          start_x,
          start_y,
          width,
          height,
          Cfg::Layer::PLAYER,
          MASK,
          BLOCK),

      player_sprite(
          bn::sprite_items::ente.create_sprite(start_x, start_y),
          start_x,
          start_y),

      walk_action(bn::create_sprite_animate_action_forever(
          player_sprite.sprite(),
          Cfg::Player::WAIT_UPDATE,
          bn::sprite_items::ente.tiles_item(),
          Cfg::Player::RIGHT_FRAMES[0],
          Cfg::Player::RIGHT_FRAMES[1])),

      jump_action(bn::create_sprite_animate_action_forever(
          player_sprite.sprite(),
          Cfg::Player::WAIT_UPDATE,
          bn::sprite_items::ente.tiles_item(),
          Cfg::Player::JUMP_RIGHT_FRAMES[0],
          Cfg::Player::JUMP_RIGHT_FRAMES[1])),

      death_counter(),
      death_counter_text_gen(common::variable_8x16_sprite_font),
      death_counter_hud(death_counter_text_gen, death_counter),

      timer(),
      timer_hud(timer),

      restart_x(start_x),
      restart_y(start_y),

      acceleration(Cfg::Player::ACCELERATION),
      max_speed(Cfg::Player::MAX_SPEED),
      jump_speed(Cfg::Player::JUMP_SPEED),
      gravity(Cfg::Player::GRAVITY),
      max_fall_speed(Cfg::Player::MAX_FALL_SPEED),
      death_height(Cfg::Player::DEATH_HEIGHT),

      on_ground(true),
      facing(Facing::Forward),
      state(PlayerState::Idle) {
    // Link the physics body with the player sprite for rendering.
    sprite = &player_sprite;

    // Preload ALL tile frames once (ZERO runtime allocation)
    const auto& tiles = bn::sprite_items::ente.tiles_item();

    for (int i = 0; i < Cfg::Player::PLAYER_TILE_CACHE_SIZE; ++i) {
        cached_tiles.push_back(tiles.create_tiles(i));
    }
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

    // Update the current animation and state machine.
    update_state();
    update_animation();

    // Decrease jump buffer timer
    if (jump_buffer_timer > 0) {
        jump_buffer_timer--;
    }

    // Update timer
    if (bn::keypad::select_pressed()) {
        timer_hud.set_visible(!timer_hud.visible());
    }
    timer.tick();
    timer_hud.update();
}

// sets spawnpoint
void Player::set_spawn_point(bn::fixed x, bn::fixed y) {
    restart_x = x;
    restart_y = y;
}

void Player::teleport_to(bn::fixed x, bn::fixed y) {
    set_velocity(0, 0);
    pos.x = x;
    pos.y = y;
}

unsigned int Player::get_deaths() const {
    return death_counter.count();
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
}

// Jump logic: perform jump when buffered input exists and the player is
// grounded.
void Player::handle_jump() {
    if (jump_buffer_timer > 0 && on_ground) {
        set_velocity(vel_x, jump_speed);
        on_ground = false;
        jump_buffer_timer = 0;
        coyote_timer = 0;
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
        on_ground = true;
        coyote_timer = Cfg::Player::COYOTE_FRAMES;
    } else {
        if (coyote_timer > 0) {
            coyote_timer--;
            on_ground = true;
        } else {
            on_ground = false;
        }
    }
}

// Fall below the death height triggers a respawn.
void Player::check_death() {
    if (pos.y > death_height) {
        death();
    }
}

// Handle player death, increment the counter and respawn.
void Player::death() {
    death_counter.on_player_death();
    death_counter_hud.update();

    set_velocity(0, 0);

    pos.x = restart_x;
    pos.y = restart_y;
}

// Update the player's state machine based on movement and grounding.
void Player::update_state() {
    PlayerState new_state;

    if (!on_ground) {
        new_state = (vel_y < 0) ? PlayerState::Jump : PlayerState::Fall;
    } else {
        // Hold down to enter the idle pose while grounded.
        if (bn::keypad::down_held()) {
            new_state = PlayerState::Idle;
        } else {
            // No input → stay in Run (but no animation update)
            new_state = PlayerState::Run;
        }
    }

    if (new_state != state) {
        enter_state(new_state);
    }
}

// Transition into a new player animation state.
void Player::enter_state(PlayerState new_state) {
    state = new_state;
}

// Update the player sprite based on current state and input.
void Player::update_animation() {
    bool moving = bn::keypad::left_held() || bn::keypad::right_held();

    bool jumping = !on_ground;

    // Moving -> start walk animation instantly
    if (moving && !was_moving) {
        walk_action.reset();
        walk_action.update();
    }

    // Jump started -> start jump animation instantly
    if (jumping && !was_jumping) {
        jump_action.reset();
        jump_action.update();
    }

    // Stop walk animation when not moving
    if (!moving) {
        walk_action.reset();
    }

    // Stop jump animation when grounded
    if (!jumping && was_jumping) {
        jump_action.reset();

        // Restore default ground frame after landing
        player_sprite.sprite().set_tiles(
            cached_tiles[Cfg::Player::RIGHT_FRAMES[0]]);
    }

    // BACK FRAME
    if (on_ground && bn::keypad::up_held()) {
        player_sprite.sprite().set_tiles(cached_tiles[Cfg::Player::BACK_FRAME]);

        was_moving = moving;
        was_jumping = jumping;
        return;
    }

    // JUMP ANIMATION
    if (!on_ground) {
        jump_action.update();

        was_moving = moving;
        was_jumping = jumping;
        return;
    }

    // IDLE FRAME
    if (bn::keypad::down_held()) {
        player_sprite.sprite().set_tiles(cached_tiles[Cfg::Player::IDLE_FRAME]);

        was_moving = moving;
        was_jumping = jumping;
        return;
    }

    // WALK ANIMATION
    if (moving) {
        walk_action.update();
    }

    was_moving = moving;
    was_jumping = jumping;
}
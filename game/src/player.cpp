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
          Cfg::Layer::TRAP,
          BLOCK),

      player_sprite(
          bn::sprite_items::ente.create_sprite(in_start_x, in_start_y),
          in_start_x,
          in_start_y),

      action(
          bn::create_sprite_animate_action_forever(
              player_sprite.sprite(),
              26,
              bn::sprite_items::ente.tiles_item(),
              Cfg::Player::RIGHT_FRAMES[0],
              Cfg::Player::RIGHT_FRAMES[1])),

      deathCounter(),
      deathCounterTextGen(common::variable_8x16_sprite_font),
      deathCounterHud(deathCounterTextGen, deathCounter),

      restart_x(in_start_x),
      restart_y(in_start_y),

      acceleration(Cfg::Player::ACCELERATION),
      max_speed(Cfg::Player::MAX_SPEED),
      jump_speed(Cfg::Player::JUMP_SPEED),
      gravity(Cfg::Player::GRAVITY),
      max_fall_speed(Cfg::Player::MAX_FALL_SPEED),
      deathHeight(Cfg::Player::DEATH_HEIGHT),

      onGround(true),
      facing(Facing::Forward),
      state(PlayerState::Idle) {
    // Link the physics body with the player sprite for rendering.
    sprite = &player_sprite;
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
    if (jump_buffer_timer > 0)
        jump_buffer_timer--;

    //     BN_LOG(
    //         "x:", x, " y:", y, "vel_x:", vel_x, " vel_y:", vel_y,
    //         "state:", (int)state);
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
    return deathCounter.count();
}

// Horizontal input handling
void Player::handle_horizontal_input() {
    if (bn::keypad::left_held()) {
        inc_velocity(-acceleration, 0);
        if (vel_x < -max_speed) {
            set_velocity(-max_speed, vel_y);
        }

        if (facing != Facing::Left) {
            set_direction(Facing::Left, Cfg::Player::LEFT_FRAMES[0]);
            set_walk_animation(Cfg::Player::LEFT_FRAMES);
        }
    } else if (bn::keypad::right_held()) {
        inc_velocity(acceleration, 0);
        if (vel_x > max_speed) {
            set_velocity(max_speed, vel_y);
        }

        if (facing != Facing::Right) {
            set_direction(Facing::Right, Cfg::Player::RIGHT_FRAMES[0]);
            set_walk_animation(Cfg::Player::RIGHT_FRAMES);
        }
    } else {
        // No horizontal input: apply friction to slow the player down.
        dec_velocity(acceleration, 0);
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
    deathCounter.on_player_death();
    deathCounterHud.update();
    set_velocity(0, 0);
    pos.x = restart_x;
    pos.y = restart_y;
}

// Update the player's state machine based on movement and grounding.
void Player::update_state() {
    PlayerState new_state;

    if (!onGround) {
        new_state = (vel_y < 0) ? PlayerState::Jump : PlayerState::Fall;
    } else {
        // Hold down to enter the idle pose while grounded.
        if (bn::keypad::down_held()) {
            new_state = PlayerState::Idle;
        } else {
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

    switch (state) {
        case PlayerState::Jump:
            set_frame(
                (facing == Facing::Left) ? Cfg::Player::LEFT_FRAMES[0]
                                         : Cfg::Player::RIGHT_FRAMES[0]);
            break;
        default:
            break;
    }
}

// Update the player sprite based on current state and input.
void Player::update_animation() {
    if (onGround && bn::keypad::up_held()) {
        set_frame(Cfg::Player::BACK_FRAME);
        facing = Facing::Back;
        return;
    }

    if (onGround && bn::keypad::down_held()) {
        set_frame(Cfg::Player::IDLE_FRAME);
        facing = Facing::Forward;
        return;
    }

    switch (state) {
        case PlayerState::Idle:
            set_frame(Cfg::Player::IDLE_FRAME);
            break;

        case PlayerState::Run:
            if (bn::keypad::left_held() || bn::keypad::right_held()) {
                action.update();
            }
            break;

        case PlayerState::Jump:
            break;

        case PlayerState::Fall:
            break;

        default:
            break;
    }
}

// Change facing direction and update the sprite frame.
void Player::set_direction(Facing new_facing, int tile_index) {
    facing = new_facing;
    set_frame(tile_index);
}

// Set the player sprite to a specific tile frame.
void Player::set_frame(int tile_index) {
    player_sprite.sprite().set_tiles(
        bn::sprite_items::ente.tiles_item().create_tiles(tile_index));
}

// Update the walking animation using a pair of frames.
void Player::set_walk_animation(const int frames[2]) {
    action = bn::create_sprite_animate_action_forever(
        player_sprite.sprite(), 10, bn::sprite_items::ente.tiles_item(),
        frames[0], frames[1]);
}

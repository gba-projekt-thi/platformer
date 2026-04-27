#include "player.h"

Player::Player(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height)
    : PhysicsBody(start_x, start_y, width, height, LAYERS, MASK, BLOCK),

      player_sprite(
          bn::sprite_items::ente.create_sprite(start_x, start_y),
          start_x,
          start_y),

      action(bn::create_sprite_animate_action_forever(
          player_sprite.sprite(),
          26,
          bn::sprite_items::ente.tiles_item(),
          RIGHT_FRAMES[0],
          RIGHT_FRAMES[1])),

      deathCounter(),
      deathCounterTextGen(common::variable_8x16_sprite_font),
      deathCounterHud(deathCounterTextGen, deathCounter),

      restart_x(start_x),
      restart_y(start_y),

      acceleration(0.3),
      max_speed(2),
      jump_speed(-2.8),
      gravity(0.22),
      max_fall_speed(3),
      deathHeight(DEFAULT_DEATH_HEIGHT),

      onGround(true),
      facing(Facing::Forward),
      state(PlayerState::Idle) {
    sprite = &player_sprite;
}

void Player::update() {
    handle_horizontal_input();

    // Store jump input (jump buffer)
    if (bn::keypad::a_pressed()) {
        jump_buffer_timer = JUMP_BUFFER_FRAMES;
    }

    apply_gravity();
    apply_variable_jump();
    clamp_velocity();

    check_bounds();
    check_death();

    update_ground_state();

    handle_jump();

    update_state();
    update_animation();

    // Decrease jump buffer timer
    if (jump_buffer_timer > 0)
        jump_buffer_timer--;

    BN_LOG(
        "x:", x, " y:", y, "vel_x:", vel_x, " vel_y:", vel_y,
        "state:", (int)state);
}

// Horizontal input handling
void Player::handle_horizontal_input() {
    if (bn::keypad::left_held()) {
        inc_velocity(-acceleration, 0);
        if (vel_x < -max_speed)
            set_velocity(-max_speed, vel_y);

        if (facing != Facing::Left) {
            set_direction(Facing::Left, LEFT_FRAMES[0]);
            set_walk_animation(LEFT_FRAMES);
        }
    } else if (bn::keypad::right_held()) {
        inc_velocity(acceleration, 0);
        if (vel_x > max_speed)
            set_velocity(max_speed, vel_y);

        if (facing != Facing::Right) {
            set_direction(Facing::Right, RIGHT_FRAMES[0]);
            set_walk_animation(RIGHT_FRAMES);
        }
    } else {
        // Apply friction
        dec_velocity(acceleration, 0);
    }
}

// Jump handling with buffer and coyote time
void Player::handle_jump() {
    if (jump_buffer_timer > 0 && onGround) {
        set_velocity(vel_x, jump_speed);
        onGround = false;
        jump_buffer_timer = 0;
        coyote_timer = 0;
    }

    // Stop upward movement if hitting ceiling
    if (vel_y < 0 && probe_top() & BLOCK) {
        set_velocity(vel_x, 0);
    }
}

// Apply gravity
void Player::apply_gravity() {
    inc_velocity(0, gravity);
}

// Variable jump height logic
void Player::apply_variable_jump() {
    // If jump button released while moving upward → cut jump short
    if (!bn::keypad::a_held() && vel_y < 0) {
        vel_y *= 0.5;  // cut upward velocity
    }
}

// Clamp falling speed
void Player::clamp_velocity() {
    if (vel_y > max_fall_speed)
        set_velocity(vel_x, max_fall_speed);
}

// Keep player inside horizontal bounds
void Player::check_bounds() {
    if (x < -110 || x > 110)
        set_velocity(0, vel_y);
}

// Ground detection with layer filtering and coyote time
void Player::update_ground_state() {
    if (probe_bottom() & BLOCK) {
        onGround = true;
        coyote_timer = COYOTE_FRAMES;
    } else {
        if (coyote_timer > 0) {
            coyote_timer--;
            onGround = true;
        } else {
            onGround = false;
        }
    }
}

// Death check
void Player::check_death() {
    if (y > deathHeight)
        death();
}

// Respawn player
void Player::death() {
    deathCounter.on_player_death();
    deathCounterHud.update();
    set_velocity(0, 0);
    x = restart_x;
    y = restart_y;
}

// Update player state
void Player::update_state() {
    PlayerState new_state;

    if (!onGround) {
        new_state = (vel_y < 0) ? PlayerState::Jump : PlayerState::Fall;
    } else {
        // Idle ONLY when pressing down
        if (bn::keypad::down_held()) {
            new_state = PlayerState::Idle;
        } else {
            new_state = PlayerState::Run;
        }
    }

    if (new_state != state)
        enter_state(new_state);
}

// Enter a new state
void Player::enter_state(PlayerState new_state) {
    state = new_state;

    switch (state) {
        case PlayerState::Jump:
            set_frame(
                (facing == Facing::Left) ? LEFT_FRAMES[0] : RIGHT_FRAMES[0]);
            break;
        default:
            break;
    }
}

// Update animation based on state
void Player::update_animation() {
    // Up/Down look only when grounded
    if (onGround && bn::keypad::up_held()) {
        set_frame(BACK_FRAME);
        facing = Facing::Back;
        return;
    }

    if (onGround && bn::keypad::down_held()) {
        set_frame(IDLE_FRAME);
        facing = Facing::Forward;
        return;
    }

    switch (state) {
        case PlayerState::Idle:
            set_frame(IDLE_FRAME);
            break;

        case PlayerState::Run:
            // If no input, keep last frame (no animation update)
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

// Change facing direction
void Player::set_direction(Facing new_facing, int tile_index) {
    facing = new_facing;
    set_frame(tile_index);
}

// Set a specific sprite frame
void Player::set_frame(int tile_index) {
    player_sprite.sprite().set_tiles(
        bn::sprite_items::ente.tiles_item().create_tiles(tile_index));
}

// Set walking animation
void Player::set_walk_animation(const int frames[2]) {
    action = bn::create_sprite_animate_action_forever(
        player_sprite.sprite(), 10, bn::sprite_items::ente.tiles_item(),
        frames[0], frames[1]);
}
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

      restart_x(start_x),
      restart_y(start_y),

      acceleration(0.3),
      max_speed(2),
      jump_speed(-3.5),
      gravity(0.2),
      max_fall_speed(3),
      deathHeight(DEFAULT_DEATH_HEIGHT),

      onGround(true),
      facing(Facing::Forward),
      state(PlayerState::Idle) {
    sprite = &player_sprite;
}

void Player::update() {
    handle_horizontal_input();
    handle_jump();
    apply_gravity();
    clamp_velocity();
    update_ground_state();
    check_bounds();
    check_death();

    update_state();
    update_animation();

    BN_LOG(
        "x:", x, " y:", y, "vel_x:", vel_x, " vel_y:", vel_y,
        "state:", (int)state);
}

// Horizontal input
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
        // Friction
        dec_velocity(acceleration, 0);
    }
}

// Jump input
void Player::handle_jump() {
    if (bn::keypad::a_pressed() && onGround) {
        set_velocity(vel_x, jump_speed);
        onGround = false;
    }
}

// Gravity and limits
void Player::apply_gravity() {
    inc_velocity(0, gravity);
}
void Player::clamp_velocity() {
    if (vel_y > max_fall_speed)
        set_velocity(vel_x, max_fall_speed);
}

// Map boundaries
void Player::check_bounds() {
    if (x < -110 || x > 110)
        set_velocity(0, vel_y);
}

// Ground detection
void Player::update_ground_state() {
    onGround = probe_bottom() != 0;
}

// Death / respawn
void Player::check_death() {
    if (y > deathHeight)
        death();
}
void Player::death() {
    set_velocity(0, 0);
    x = restart_x;
    y = restart_y;
}

// State machine
void Player::update_state() {
    PlayerState new_state;

    if (!onGround) {
        new_state = (vel_y < 0) ? PlayerState::Jump : PlayerState::Fall;
    } else {
        new_state = (vel_x != 0) ? PlayerState::Run : PlayerState::Idle;
    }

    if (new_state != state)
        enter_state(new_state);
}

void Player::enter_state(PlayerState new_state) {
    state = new_state;
    // Optional: specific frame for Jump/Fall
    switch (state) {
        case PlayerState::Jump:
            set_frame(
                (facing == Facing::Left) ? LEFT_FRAMES[0] : RIGHT_FRAMES[0]);
            break;
        case PlayerState::Fall:
            set_frame(IDLE_FRAME);
            break;
        default:
            break;
    }
}

// Animation by state
void Player::update_animation() {
    // Up/down overrides
    if (bn::keypad::up_held()) {
        set_frame(BACK_FRAME);
        facing = Facing::Back;
        return;
    }
    if (bn::keypad::down_held()) {
        set_frame(IDLE_FRAME);
        facing = Facing::Forward;
        return;
    }

    switch (state) {
        case PlayerState::Idle:
            set_frame(IDLE_FRAME);
            break;
        case PlayerState::Run:
            action.update();
            break;
        case PlayerState::Jump: /* frame already set */
            break;
        case PlayerState::Fall: /* frame already set */
            break;
        default:
            break;
    }
}

// Helpers
void Player::set_direction(Facing new_facing, int tile_index) {
    facing = new_facing;
    set_frame(tile_index);
}
void Player::set_frame(int tile_index) {
    player_sprite.sprite().set_tiles(
        bn::sprite_items::ente.tiles_item().create_tiles(tile_index));
}
void Player::set_walk_animation(const int frames[2]) {
    action = bn::create_sprite_animate_action_forever(
        player_sprite.sprite(), 10, bn::sprite_items::ente.tiles_item(),
        frames[0], frames[1]);
}
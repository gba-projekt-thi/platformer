#include "player.h"

Player::Player(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height)
    :

      // Initialize physics body base.
      PhysicsBody(
          start_x,
          start_y,
          width,
          height,
          Cfg::Layer::PLAYER,
          MASK,
          BLOCK),

      // Create player sprite instance.
      player_sprite(
          bn::sprite_items::ente.create_sprite(start_x, start_y),
          start_x,
          start_y),

      // Walking animation.
      walk_action(bn::create_sprite_animate_action_forever(
          player_sprite.sprite(),
          Cfg::Player::WAIT_UPDATE,
          bn::sprite_items::ente.tiles_item(),
          Cfg::Player::RIGHT_FRAMES[0],
          Cfg::Player::RIGHT_FRAMES[1])),

      // Jump animation.
      jump_action(bn::create_sprite_animate_action_forever(
          player_sprite.sprite(),
          Cfg::Player::WAIT_UPDATE,
          bn::sprite_items::ente.tiles_item(),
          Cfg::Player::JUMP_RIGHT_FRAMES[0],
          Cfg::Player::JUMP_RIGHT_FRAMES[1])),

      death_counter(),

      death_text_generator(common::variable_8x16_sprite_font),

      death_hud(death_text_generator, death_counter),

      timer(),

      timer_hud(timer),

      restart_x(start_x),
      restart_y(start_y) {
    // Connect render sprite to physics body.
    sprite = &player_sprite;

    // Enable alpha blending.
    player_sprite.sprite().set_blending_enabled(true);

    death_text_generator.set_blending_enabled(true);

    // Preload all animation tiles once.
    // Avoid runtime tile allocation during gameplay.
    const auto& tiles = bn::sprite_items::ente.tiles_item();

    cached_tiles.emplace(
        bn::array<bn::sprite_tiles_ptr, Cfg::Player::PLAYER_TILE_CACHE_SIZE>{
            tiles.create_tiles(0), tiles.create_tiles(1), tiles.create_tiles(2),
            tiles.create_tiles(3), tiles.create_tiles(4), tiles.create_tiles(5),
            tiles.create_tiles(6), tiles.create_tiles(7), tiles.create_tiles(8),
            tiles.create_tiles(9)});

    set_tile(Cfg::Player::IDLE_FRAME);
}

void Player::update() {
    // Cached frame input.
    Input input;

    process_input(input);

    // Buffer jump input for forgiving jump timing.
    if (input.jump_pressed) {
        jump_buffer_timer = Cfg::Player::JUMP_BUFFER_FRAMES;
    }

    // Horizontal movement.
    handle_horizontal_input(input);

    // Vertical movement.
    apply_gravity();

    apply_variable_jump(input);

    clamp_velocity();

    // Collision/ground checks.
    update_ground_state();

    handle_jump();

    // World checks.
    check_bounds();

    check_death();

    // Update state machine.
    update_state(input);

    // Update sprite visuals.
    update_animation();

    // Countdown jump buffer.
    if (jump_buffer_timer > 0) {
        --jump_buffer_timer;
    }

    // Toggle timer visibility.
    if (bn::keypad::select_pressed()) {
        timer_hud.set_visible(!timer_hud.visible());
    }

    // Update timer HUD.
    timer.tick();

    timer_hud.update();
}

// Read all player input once for this frame.
void Player::process_input(Input& input) {
    input.left = bn::keypad::left_held();

    input.right = bn::keypad::right_held();

    input.up = bn::keypad::up_held();

    input.down = bn::keypad::down_held();

    input.jump_pressed = bn::keypad::a_pressed();

    input.jump_held = bn::keypad::a_held();
}

/**
 * Handle left/right acceleration and friction.
 *
 * Uses a compact direction calculation:
 *  right =  1
 *  left  = -1
 *  none  =  0
 *
 * This reduces branching in the movement hot-path.
 */
void Player::handle_horizontal_input(const Input& input) {
    const int direction = int(input.right) - int(input.left);

    if (direction != 0) {
        vel_x += Cfg::Player::ACCELERATION * direction;

        // Clamp horizontal speed.
        if (vel_x > Cfg::Player::MAX_SPEED) {
            vel_x = Cfg::Player::MAX_SPEED;
        } else if (vel_x < -Cfg::Player::MAX_SPEED) {
            vel_x = -Cfg::Player::MAX_SPEED;
        }

        // Flip sprite based on facing direction.
        player_sprite.sprite().set_horizontal_flip(direction < 0);
    } else {
        // Apply friction when no input is held.
        dec_velocity(Cfg::Player::ACCELERATION, 0);
    }
}

/**
 * Execute jump logic using:
 * - jump buffering
 * - coyote time
 *
 * Also cancels upward movement when
 * colliding with ceilings.
 */
void Player::handle_jump() {
    if (jump_buffer_timer > 0 && on_ground) {
        vel_y = Cfg::Player::JUMP_SPEED;

        on_ground = false;

        jump_buffer_timer = 0;

        coyote_timer = 0;
    }

    // Stop upward motion on ceiling collision.
    if (vel_y < 0 && probe_top(BLOCK).any()) {
        vel_y = 0;
    }
}

// Apply downward acceleration every frame.
inline void Player::apply_gravity() {
    vel_y += Cfg::Player::GRAVITY;
}

/**
 * Variable jump height.
 *
 * Releasing jump early cuts upward momentum,
 * allowing short-hop jumps.
 */
inline void Player::apply_variable_jump(const Input& input) {
    if (!input.jump_held && vel_y < 0) {
        vel_y /= 2;
    }
}

/**
 * Prevent excessive falling speed.
 *
 * Important for:
 * - gameplay feel
 * - collision stability
 * - preventing tunneling
 */
inline void Player::clamp_velocity() {
    if (vel_y > Cfg::Player::MAX_FALL_SPEED) {
        vel_y = Cfg::Player::MAX_FALL_SPEED;
    }
}

/**
 * Update grounded state.
 *
 * Includes coyote time support:
 * the player remains grounded briefly
 * after leaving a platform.
 */
void Player::update_ground_state() {
    if (probe_bottom(BLOCK).any()) {
        on_ground = true;

        coyote_timer = Cfg::Player::COYOTE_FRAMES;
    } else if (coyote_timer > 0) {
        --coyote_timer;

        on_ground = true;
    } else {
        on_ground = false;
    }
}

/**
 * Determine the current player state.
 *
 * State drives:
 * - animation
 * - future gameplay behavior
 * - transitions
 */
void Player::update_state(const Input& input) {
    PlayerState new_state;

    if (!on_ground) {
        new_state = vel_y < 0 ? PlayerState::Jump : PlayerState::Fall;
    } else if (input.up) {
        new_state = PlayerState::LookUp;
    } else if (input.down) {
        new_state = PlayerState::Crouch;
    } else if (input.moving()) {
        new_state = PlayerState::Run;
    } else {
        new_state = PlayerState::Idle;
    }

    // Only transition when state changes.
    if (new_state != state) {
        enter_state(new_state);
    }
}

/**
 * Handle state transitions.
 *
 * Animation resets occur here instead of
 * every frame to reduce redundant work.
 */
void Player::enter_state(PlayerState new_state) {
    previous_state = state;

    state = new_state;

    switch (state) {
        case PlayerState::Run:
            walk_action.reset();
            break;

        case PlayerState::Jump:
            jump_action.reset();
            break;

        default:
            break;
    }
}

/**
 * Update sprite visuals from current state.
 *
 * Animation is fully state-driven.
 */
void Player::update_animation() {
    switch (state) {
        case PlayerState::Idle:
        case PlayerState::Crouch:

            set_tile(Cfg::Player::IDLE_FRAME);
            break;

        case PlayerState::LookUp:

            set_tile(Cfg::Player::BACK_FRAME);
            break;

        case PlayerState::Run:

            walk_action.update();
            break;

        case PlayerState::Jump:
        case PlayerState::Fall:

            jump_action.update();
            break;

        default:
            break;
    }
}

/**
 * Keep the player inside the horizontal
 * gameplay boundaries.
 */
void Player::check_bounds() {
    if (pos.x < -Cfg::Screen::HORIZONTAL_EDGE ||
        pos.x > Cfg::Screen::HORIZONTAL_EDGE) {
        vel_x = -vel_x;
    }
}

// Detect death conditions.
void Player::check_death() {
    if (pos.y > Cfg::Player::DEATH_HEIGHT) {
        death();
    }
}

// Reset player state after death.
void Player::death() {
    death_counter.on_player_death();

    death_hud.update();

    // Reset movement.
    vel_x = 0;
    vel_y = 0;

    // Restore spawn position.
    pos.x = restart_x;
    pos.y = restart_y;
}

// Update respawn position.
void Player::set_spawn_point(bn::fixed x, bn::fixed y) {
    restart_x = x;
    restart_y = y;
}

// Instantly move player and clear velocity.
void Player::teleport_to(bn::fixed x, bn::fixed y) {
    vel_x = 0;
    vel_y = 0;

    pos.x = x;
    pos.y = y;
}

// Current death counter value
unsigned int Player::get_deaths() const {
    return death_counter.count();
}

// Restore death counter state.
void Player::set_deaths(unsigned int deaths) {
    death_counter.set_count(deaths);

    death_hud.update();
}

// Access gameplay timer.
Timer& Player::get_timer() {
    return timer;
}

/**
 * Change sprite tiles only when necessary.
 *
 * Prevents redundant VRAM tile swaps,
 * helping reduce VBlank workload.
 */
inline void Player::set_tile(int index) {
    if (current_tile != index) {
        current_tile = index;

        player_sprite.sprite().set_tiles((*cached_tiles)[index]);
    }
}
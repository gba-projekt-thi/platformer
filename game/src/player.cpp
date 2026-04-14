#include "player.h"

#include "bn_keypad.h"

#include "bn_sprite_items_ente.h"

#include "bn_log.h"

Player::Player(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height)
    : PhysicsBody(start_x, start_y, width, height, LAYERS, MASK, BLOCK),

      // Configuration values
      player_sprite(
          bn::sprite_items::ente.create_sprite(start_x, start_y),
          start_x,
          start_y),
      restart_x(start_x),
      restart_y(start_y),
      deathHeight(100),
      acceleration(0.3),
      max_speed(2),
      jump_speed(-3.5),  // This should be negative
      gravity(0.2),
      max_fall_speed(3),

      // Working values
      onGround(true),
      action(
          bn::create_sprite_animate_action_forever(
              player_sprite.sprite(),
              26,
              bn::sprite_items::ente.tiles_item(),
              0,
              1)),
      facing(Facing::Forward) {
    this->sprite = &player_sprite;
}

void Player::update() {
    // Handle player input for left and right
    if (bn::keypad::left_held()) {
        // Update animation on direction change
        if (facing != Facing::Left) {
            action = bn::create_sprite_animate_action_forever(
                player_sprite.sprite(), 10, bn::sprite_items::ente.tiles_item(),
                4, 5);
            facing = Facing::Left;
            // Flip sprite manually in case we're in the air
            player_sprite.sprite().set_tiles(
                bn::sprite_items::ente.tiles_item().create_tiles(4));
        }

        // Calculate speed
        inc_velocity(-acceleration, 0);
        if (vel_x < -max_speed) {
            set_velocity(-max_speed, vel_y);
        }
    } else if (bn::keypad::right_held()) {
        // Update animation on direction change
        if (facing != Facing::Right) {
            action = bn::create_sprite_animate_action_forever(
                player_sprite.sprite(), 10, bn::sprite_items::ente.tiles_item(),
                0, 1);
            facing = Facing::Right;
            // Flip sprite manually in case we're in the air
            player_sprite.sprite().set_tiles(
                bn::sprite_items::ente.tiles_item().create_tiles(0));
        }

        // Calculate speed
        inc_velocity(acceleration, 0);
        if (vel_x > max_speed) {
            set_velocity(max_speed, vel_y);
        }
    } else {
        // Slow down when there is no input
        dec_velocity(acceleration, 0);
    }

    // Check if we're at the edge of the map
    if (x < -110 || x > 110) {
        set_velocity(0, vel_y);
    }

    // Jump
    if (bn::keypad::a_pressed() && onGround) {
        set_velocity(vel_x, jump_speed);
        onGround = false;
    }

    // Gravity
    inc_velocity(0, gravity);
    if (vel_y > max_fall_speed) {
        set_velocity(vel_x, max_fall_speed);
    }

    // Check if we're on the Ground
    onGround = probe_bottom() != 0;

    // Check if we've fallen off the map
    if (y > deathHeight) {
        death();
    }

    // Only play animation when we're moving but not falling
    if (vel_x != 0 && onGround) {
        action.update();
    }
    // Face forward when we're neither moving nor falling
    else if (onGround) {
        player_sprite.sprite().set_tiles(
            bn::sprite_items::ente.tiles_item().create_tiles(8));
        facing = Facing::Forward;
    }

    // Player input for up and down
    if (bn::keypad::up_held()) {
        player_sprite.sprite().set_tiles(
            bn::sprite_items::ente.tiles_item().create_tiles(9));
        facing = Facing::Back;
    } else if (bn::keypad::down_held()) {
        player_sprite.sprite().set_tiles(
            bn::sprite_items::ente.tiles_item().create_tiles(8));
        facing = Facing::Forward;
    }

    BN_LOG("x:", x, " y:", y, "vel_x:", vel_x, " vel_y:", vel_y);
}

void Player::death() {
    set_velocity(0, 0);
    x = restart_x;
    y = restart_y;
}

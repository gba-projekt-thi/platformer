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
      player_sprite(bn::sprite_items::ente.create_sprite(start_x, start_y)),
      groundLevel(60),
      acceleration(0.3),
      max_speed(2),
      jump_speed(-3.5),  // This should be negative
      gravity(0.2),
      max_fall_speed(3),

      // Working values
      x_coord(start_x),
      y_coord(start_y),
      x_velocity(0),
      y_velocity(0),
      canJump(true),
      action(bn::create_sprite_animate_action_forever(
          player_sprite,
          26,
          bn::sprite_items::ente.tiles_item(),
          0,
          1)),
      facing(Facing::Forward) {}

void Player::update() {
    // Handle player input for left and right
    if (bn::keypad::left_held()) {
        // Update animation on direction change
        if (facing != Facing::Left) {
            action = bn::create_sprite_animate_action_forever(
                player_sprite, 10, bn::sprite_items::ente.tiles_item(), 4, 5);
            facing = Facing::Left;
            // Flip sprite manually in case we're in the air
            player_sprite.set_tiles(
                bn::sprite_items::ente.tiles_item().create_tiles(4));
        }

        // Calculate speed
        x_velocity -= acceleration;
        if (x_velocity < -max_speed) {
            x_velocity = -max_speed;
        }
    } else if (bn::keypad::right_held()) {
        // Update animation on direction change
        if (facing != Facing::Right) {
            action = bn::create_sprite_animate_action_forever(
                player_sprite, 10, bn::sprite_items::ente.tiles_item(), 0, 1);
            facing = Facing::Right;
            // Flip sprite manually in case we're in the air
            player_sprite.set_tiles(
                bn::sprite_items::ente.tiles_item().create_tiles(0));
        }

        // Calculate speed
        x_velocity += acceleration;
        if (x_velocity > max_speed) {
            x_velocity = max_speed;
        }
    } else {
        // Slow down when there is no input
        if (x_velocity < 0) {
            x_velocity += acceleration;
            if (x_velocity > 0)
                x_velocity = 0;
        } else if (x_velocity > 0) {
            x_velocity -= acceleration;
            if (x_velocity < 0)
                x_velocity = 0;
        }
    }

    // Calculate new x_coordinate
    x_coord += x_velocity;

    // Check if we're at the edge of the map
    if (x_coord < -110) {
        x_coord = -110;
        x_velocity = 0;
    } else if (x_coord > 110) {
        x_coord = 110;
        x_velocity = 0;
    }

    // Jump
    if (bn::keypad::a_pressed() && canJump) {
        y_velocity = jump_speed;
        canJump = false;
    }

    // Gravity
    y_velocity += gravity;
    if (y_velocity > max_fall_speed) {
        y_velocity = max_fall_speed;
    }

    // Calculate new y_coordinate
    y_coord += y_velocity;

    // Check if we're on the Ground
    if (y_coord >= groundLevel) {
        y_coord = groundLevel;
        y_velocity = 0;
        canJump = true;
    }

    // Update player sprite
    player_sprite.set_position(x_coord, y_coord);
    // Only play animation when we're moving but not falling
    if (x_velocity != 0 && y_velocity == 0) {
        action.update();
    }
    // Face forward when we're neither moving nor falling
    else if (y_velocity == 0) {
        player_sprite.set_tiles(
            bn::sprite_items::ente.tiles_item().create_tiles(8));
        facing = Facing::Forward;
    }

    // Player input for up and down
    if (bn::keypad::up_held()) {
        player_sprite.set_tiles(
            bn::sprite_items::ente.tiles_item().create_tiles(9));
        facing = Facing::Back;
    } else if (bn::keypad::down_held()) {
        player_sprite.set_tiles(
            bn::sprite_items::ente.tiles_item().create_tiles(8));
        facing = Facing::Forward;
    }

    BN_LOG("Player x:", x_coord, " y:", y_coord);
}

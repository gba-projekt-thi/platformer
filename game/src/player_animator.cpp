#include "player_animator.h"

#include "bn_keypad.h"

PlayerAnimator::PlayerAnimator(
    Sprite& sprite,
    const bn::sprite_item& tiles_source)
    : _sprite(sprite),
      _walk_action(bn::create_sprite_animate_action_forever(
          sprite.sprite(),
          Cfg::Player::WAIT_UPDATE,
          tiles_source.tiles_item(),
          Cfg::Player::RIGHT_FRAMES[0],
          Cfg::Player::RIGHT_FRAMES[1])),
      _jump_action(bn::create_sprite_animate_action_forever(
          sprite.sprite(),
          Cfg::Player::WAIT_UPDATE,
          tiles_source.tiles_item(),
          Cfg::Player::JUMP_RIGHT_FRAMES[0],
          Cfg::Player::JUMP_RIGHT_FRAMES[1])) {
    // Preload ALL tile frames once (ZERO runtime allocation)
    const auto& tiles = tiles_source.tiles_item();

    for (int i = 0; i < Cfg::Player::PLAYER_TILE_CACHE_SIZE; ++i) {
        _cached_tiles.push_back(tiles.create_tiles(i));
    }
}

void PlayerAnimator::update(bool onGround) {
    bool moving = bn::keypad::left_held() || bn::keypad::right_held();
    bool jumping = !onGround;

    // Moving -> start walk animation instantly
    if (moving && !_wasMoving) {
        _walk_action.reset();
        _walk_action.update();
    }

    // Jump started -> start jump animation instantly
    if (jumping && !_wasJumping) {
        _jump_action.reset();
        _jump_action.update();
    }

    // Stop walk animation when not moving
    if (!moving) {
        _walk_action.reset();
    }

    // Stop jump animation when grounded
    if (!jumping && _wasJumping) {
        _jump_action.reset();

        // Restore default ground frame after landing
        _sprite.sprite().set_tiles(_cached_tiles[Cfg::Player::RIGHT_FRAMES[0]]);
    }

    // BACK FRAME
    if (onGround && bn::keypad::up_held()) {
        _sprite.sprite().set_tiles(_cached_tiles[Cfg::Player::BACK_FRAME]);

        _wasMoving = moving;
        _wasJumping = jumping;
        return;
    }

    // JUMP ANIMATION
    if (!onGround) {
        _jump_action.update();

        _wasMoving = moving;
        _wasJumping = jumping;
        return;
    }

    // IDLE FRAME
    if (bn::keypad::down_held()) {
        _sprite.sprite().set_tiles(_cached_tiles[Cfg::Player::IDLE_FRAME]);

        _wasMoving = moving;
        _wasJumping = jumping;
        return;
    }

    // WALK ANIMATION
    if (moving) {
        _walk_action.update();
    }

    _wasMoving = moving;
    _wasJumping = jumping;
}
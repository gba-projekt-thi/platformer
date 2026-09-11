#include "player.h"

#include "bn_sound_items.h"
#include "bn_sprite_items_ente.h"

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

      _player_sprite(
          bn::sprite_items::ente.create_sprite(in_start_x, in_start_y),
          in_start_x,
          in_start_y),

      _locomotion(*this, _player_sprite, BLOCK, *this),

      _animator(_player_sprite, bn::sprite_items::ente),

      _hud(),

      _state_machine(),

      _restart_x(in_start_x),
      _restart_y(in_start_y) {
    // Link the physics body with the player sprite for rendering.
    sprite = &_player_sprite;
    _player_sprite.sprite().set_blending_enabled(true);
}

void Player::update() {
    _locomotion.update();

    _state_machine.update(_locomotion.on_ground(), vel_y);
    _animator.update(_locomotion.on_ground());

    _hud.tick();
}

void Player::set_spawn_point(bn::fixed in_x, bn::fixed in_y) {
    _restart_x = in_x;
    _restart_y = in_y;
}

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
        _player_sprite.enable();
    } else {
        _player_sprite.disable();
    }
}

bool Player::visible() const {
    return _player_sprite.is_enabled();
}

void Player::set_hud_visible(bool visible) {
    _hud.set_visible(visible);
}

void Player::set_horizontal_bound(bn::fixed bound) {
    _locomotion.set_horizontal_bound(bound);
}

void Player::death() {
    _hud.on_player_death();

    set_velocity(0, 0);

    pos.x = _restart_x;
    pos.y = _restart_y;

    bn::sound_items::duck_death.play();
}
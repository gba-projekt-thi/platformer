#include "player.h"

#include "audio_settings.h"
#include "bn_sound_items.h"
#include "bn_sprite_items_ente.h"
#include "bn_sprite_items_pogo16x16.h"

#include "death_fx.h"

namespace {
// The pogo graphic's frames (swoosh, then impact star) and how many ticks
// each shows.
constexpr int POGO_FRAMES = 3;
constexpr int POGO_FRAME_TICKS = 2;
}  // namespace

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
    _player_sprite.pos.offset_y = Cfg::Player::SPRITE_OFFSET_Y;
    _player_sprite.sprite().set_blending_enabled(true);
}

void Player::update() {
    _locomotion.update();

    _state_machine.update(_locomotion.on_ground(), vel_y);
    // A dash plays the jump pose, also when started on the ground.
    _animator.update(
        _locomotion.on_ground() && !_locomotion.dashing(),
        _locomotion.wall_sliding());

    _hud.tick();
    _update_dash_trail();
    _update_pogo_slash();
    _update_respawn_blink();
}

void Player::_update_respawn_blink() {
    // Only counts while the game runs (this is called from the physics step)
    // and while the duck is shown at all (not hidden for a menu).
    switch (_duck_fx.tick(_player_sprite.is_enabled())) {
        case death_fx::DuckFx::Blink::Show:
            _player_sprite.sprite().set_visible(true);
            break;
        case death_fx::DuckFx::Blink::Hide:
            _player_sprite.sprite().set_visible(false);
            break;
        case death_fx::DuckFx::Blink::Idle:
        default:
            break;
    }
}

void Player::_update_dash_trail() {
    const bool dashing = _locomotion.dashing();
    if (dashing && !_was_dashing) {
        _trail.begin();
    }
    _was_dashing = dashing;

    if (dashing) {
        const int slot = _trail.on_dash_frame();
        if (slot >= 0) {
            _spawn_ghost(slot);
        }
    }
    _trail.tick();
    _refresh_ghosts();
}

void Player::_spawn_ghost(int slot) {
    // A copy of the duck's current frame, left behind in world space. Created
    // hidden: _refresh_ghosts() shows it from the next frame on, once the duck
    // has moved on and it no longer sits exactly behind it.
    _ghosts[slot].reset();

    const bn::fixed x = _player_sprite.pos.get_x();
    const bn::fixed y = _player_sprite.pos.get_y();
    bn::sprite_ptr raw_sprite = bn::sprite_items::ente.create_sprite(x, y);
    raw_sprite.set_tiles(_player_sprite.sprite().tiles());
    raw_sprite.set_horizontal_flip(_player_sprite.sprite().horizontal_flip());
    raw_sprite.set_z_order(_player_sprite.sprite().z_order() + 1);  // behind
    raw_sprite.set_blending_enabled(true);
    raw_sprite.set_visible(false);

    // Wrapped in Sprite, so SpriteRegistry keeps it at its world position
    // while the camera scrolls.
    _ghosts[slot].emplace(bn::move(raw_sprite), x, y);
}

void Player::_refresh_ghosts() {
    for (int slot = 0; slot < DashTrail::SLOTS; ++slot) {
        if (!_ghosts[slot]) {
            continue;
        }
        if (!_trail.active(slot)) {
            _ghosts[slot].reset();
        } else if (_trail.visible(slot)) {
            _ghosts[slot]->enable();
        } else {
            _ghosts[slot]->disable();
        }
    }
}

void Player::clear_dash_trail() {
    _trail.clear();
    _was_dashing = false;
    for (bn::optional<Sprite>& ghost : _ghosts) {
        ghost.reset();
    }
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
    _duck_fx.stop_blink();
    if (visible) {
        _player_sprite.enable();
    } else {
        _player_sprite.disable();
        clear_dash_trail();
        _pogo_slash.reset();
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

void Player::set_ground_friction(bn::fixed scale) {
    _locomotion.set_ground_friction(scale);
}

void Player::set_assist_mode(bool enabled) {
    _locomotion.set_assist_mode(enabled);
}

void Player::set_dash_enabled(bool enabled) {
    _locomotion.set_dash_enabled(enabled);
}

void Player::set_wall_jump_enabled(bool enabled) {
    _locomotion.set_wall_jump_enabled(enabled);
}

void Player::set_pogo_enabled(bool enabled) {
    _locomotion.set_pogo_enabled(enabled);
}

void Player::_update_pogo_slash() {
    if (!_locomotion.pogo_visible() || !_player_sprite.is_enabled()) {
        _pogo_slash.reset();
        _pogo_age = 0;
        return;
    }
    // Centered on the strike's box below the body.
    const bn::fixed x = pos.x;
    const bn::fixed y =
        pos.y + pos.height / 2 + bn::fixed(Cfg::Pogo::REACH) / 2;
    if (!_pogo_slash) {
        bn::sprite_ptr raw_sprite =
            bn::sprite_items::pogo16x16.create_sprite(x, y);
        raw_sprite.set_z_order(_player_sprite.sprite().z_order());
        raw_sprite.set_blending_enabled(true);
        _pogo_slash.emplace(bn::move(raw_sprite), x, y);
    }
    _pogo_slash->pos.move(x, y);
    // Swoosh, then the impact star: a new frame every few ticks.
    const int frame = bn::min(_pogo_age / POGO_FRAME_TICKS, POGO_FRAMES - 1);
    _pogo_slash->sprite().set_tiles(
        bn::sprite_items::pogo16x16.tiles_item().create_tiles(frame));
    ++_pogo_age;
}

void Player::begin_death_fx() {
    _death_ghost.reset();

    // A copy of the duck's last frame, left where it died. Wrapped in Sprite
    // like the dash afterimages, so SpriteRegistry keeps it at its world
    // position while the camera scrolls (and shakes). The dash trail was
    // cleared by death(), so this takes one of its sprite slots.
    bn::sprite_ptr raw_sprite =
        bn::sprite_items::ente.create_sprite(_death_x, _death_y);
    raw_sprite.set_tiles(_player_sprite.sprite().tiles());
    raw_sprite.set_horizontal_flip(_player_sprite.sprite().horizontal_flip());
    raw_sprite.set_z_order(_player_sprite.sprite().z_order());
    raw_sprite.set_blending_enabled(true);
    _death_ghost.emplace(bn::move(raw_sprite), _death_x, _death_y);
    _death_ghost->pos.offset_y = Cfg::Player::SPRITE_OFFSET_Y;

    _player_sprite.disable();
    _duck_fx.begin();
}

void Player::end_death_fx() {
    if (_duck_fx.end(Cfg::DeathFx::RESPAWN_BLINK_FRAMES)) {
        _show_after_death_fx();
    }
}

void Player::cancel_death_fx() {
    if (_duck_fx.cancel()) {
        _show_after_death_fx();
    }
}

void Player::_show_after_death_fx() {
    _death_ghost.reset();
    _player_sprite.enable();
}

void Player::death() {
    // Where the duck died, for the death feedback's ghost (the teleport below
    // overwrites it).
    _death_x = pos.x;
    _death_y = pos.y;

    _hud.on_player_death();

    set_velocity(0, 0);
    // A dash must not survive the respawn.
    _locomotion.reset_dash();
    clear_dash_trail();
    _pogo_slash.reset();

    pos.x = _restart_x;
    pos.y = _restart_y;

    AudioSettings::instance().play_sfx(bn::sound_items::duck_death);
}
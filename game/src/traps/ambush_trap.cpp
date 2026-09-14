#include "ambush_trap.h"

AmbushTrap::AmbushTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    bn::fixed t_offset_x,
    bn::fixed t_offset_y,
    const bn::sprite_item& t_sprite_item,
    int t_sprite_waits,
    bn::span<const uint16_t> t_graphics_indexes,
    uint16_t t_blocking_layers,
    bn::fixed t_range,
    bn::fixed t_speed,
    unsigned t_duration,
    Player& t_player)
    : BaseTrap(
          t_start_x,
          t_start_y,
          t_width,
          t_height,
          t_offset_x,
          t_offset_y,
          t_sprite_item,
          t_sprite_waits,
          t_graphics_indexes,
          t_blocking_layers,
          // AmbushTrap uses direct positional movement, not velocity.
          Cfg::Physics::NO_MAX_VELOCITY),
      _start_x(t_start_x),
      _start_y(t_start_y),
      _range(t_range),
      _speed(t_speed),
      _duration(t_duration),
      _player(t_player) {}

void AmbushTrap::update() {
    BaseTrap::update();

    switch (_state) {
        case State::Dormant: {
            bn::fixed dx = _player.pos.x - pos.x;
            if (dx < 0) {
                dx = -dx;
            }
            if (dx <= _range) {
                _state = State::Lunging;
                _elapsed_frames = 0;
            }
            return;  // No movement this frame while still dormant.
        }

        case State::Lunging: {
            pos.x += _speed;
            ++_elapsed_frames;
            if (_elapsed_frames >= _duration) {
                _state = State::Returning;
                _elapsed_frames = 0;
            }
            break;
        }

        case State::Returning: {
            bn::fixed step = _speed < 0 ? -_speed : _speed;
            if (pos.x < _start_x) {
                pos.x += step;
                if (pos.x > _start_x) {
                    pos.x = _start_x;
                }
            } else {
                pos.x -= step;
                if (pos.x < _start_x) {
                    pos.x = _start_x;
                }
            }
            if (pos.x == _start_x) {
                _state = State::Dormant;
            }
            break;
        }
    }

    // Direct position writes bypass PhysicsBody::move(), so sync the
    // collision shape and sprite manually - same pattern as
    // ChaserTrap/PathTrap.
    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(pos.x, pos.y);
    }
}

void AmbushTrap::reset() {
    pos.x = _start_x;
    pos.y = _start_y;
    _state = State::Dormant;
    _elapsed_frames = 0;

    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(pos.x, pos.y);
    }
}
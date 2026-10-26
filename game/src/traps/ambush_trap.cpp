#include "ambush_trap.h"

AmbushTrap::AmbushTrap(const TrapData& data, bn::fixed speed, Player& player)
    // AmbushTrap uses direct positional movement, not velocity.
    : BaseTrap(data, Cfg::Physics::NO_MAX_VELOCITY),
      _start_x(data.x),
      _start_y(data.y),
      _range(data.ambush_range),
      _speed(speed),
      _duration(data.ambush_duration),
      _player(player) {}

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

        default:
            BN_LOG_ERROR(
                LogCategory::Trap, "Not a defined state of AmbushTrap.");
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
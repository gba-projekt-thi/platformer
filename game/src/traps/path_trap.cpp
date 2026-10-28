#include "path_trap.h"

PathTrap::PathTrap(const TrapData& data, unsigned path_waits, Trigger& trigger)
    : BaseTrap(
          data,
          data.path.empty() ? data.x : data.x + data.path[0].x(),
          data.path.empty() ? data.y : data.y + data.path[0].y(),
          Cfg::Physics::NO_MAX_VELOCITY),
      _start_x(data.x),
      _start_y(data.y),
      _trigger(trigger),
      _path(data.path),
      _path_waits(path_waits) {}

void PathTrap::update() {
    BaseTrap::update();
    if (!_trigger.is_triggered() || _path.size() < 2 || _path_waits == 0) {
        return;
    }

    ++_current_frame;
    const unsigned path_size = unsigned(_path.size());
    unsigned next_index = _current_index + 1;
    if (next_index >= path_size) {
        next_index = 0;
    }

    // Linear interpolation ratio.
    bn::fixed ratio = bn::fixed(_current_frame) / _path_waits;
    const bn::fixed_point& current = _path[_current_index];
    const bn::fixed_point& next = _path[next_index];

    // Interpolate between path nodes.
    pos.x = _start_x + current.x() + (next.x() - current.x()) * ratio;
    pos.y = _start_y + current.y() + (next.y() - current.y()) * ratio;

    // Direct position writes bypass PhysicsBody::move(), so sync the
    // collision shape and sprite manually.
    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(pos.x, pos.y);
    }

    // Advance to next segment.
    if (_current_frame >= _path_waits) {
        _current_frame = 0;
        _current_index = next_index;
    }
}

void PathTrap::reset() {
    _trigger.reset();

    _current_frame = 0;
    _current_index = 0;

    pos.x = _path.empty() ? _start_x : _start_x + _path[0].x();
    pos.y = _path.empty() ? _start_y : _start_y + _path[0].y();

    shape_pos.move(pos.x, pos.y);
    if (sprite) {
        sprite->pos.move(pos.x, pos.y);
    }
}
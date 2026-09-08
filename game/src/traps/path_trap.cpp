#include "path_trap.h"

PathTrap::PathTrap(
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
    bn::span<const bn::fixed_point> t_path,
    unsigned t_path_waits,
    Trigger& t_trigger)
    : BaseTrap(
          t_path.empty() ? t_start_x : t_start_x + t_path[0].x(),
          t_path.empty() ? t_start_y : t_start_y + t_path[0].y(),
          t_width,
          t_height,
          t_offset_x,
          t_offset_y,
          t_sprite_item,
          t_sprite_waits,
          t_graphics_indexes,
          t_blocking_layers,
          Cfg::Physics::NO_MAX_VELOCITY),
      _start_x(t_start_x),
      _start_y(t_start_y),
      _trigger(t_trigger),
      _path(t_path),
      _path_waits(t_path_waits) {}

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

    bn::fixed ratio = bn::fixed(_current_frame) / _path_waits;
    const bn::fixed_point& current = _path[_current_index];
    const bn::fixed_point& next = _path[next_index];

    pos.x = _start_x + current.x() + (next.x() - current.x()) * ratio;
    pos.y = _start_y + current.y() + (next.y() - current.y()) * ratio;

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
}
#include "path_trap.h"

PathTrap::PathTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
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
          t_sprite_item,
          t_sprite_waits,
          t_graphics_indexes,
          t_blocking_layers,
          // PathTrap uses positional interpolation, not velocity.
          Cfg::Physics::NO_MAX_VELOCITY),
      start_x(t_start_x),
      start_y(t_start_y),
      trigger(t_trigger),
      path(t_path),
      path_waits(t_path_waits) {}

void PathTrap::update() {
    BaseTrap::update();

    if (!trigger.is_triggered() || path.size() < 2 || path_waits == 0) {
        return;
    }

    ++current_frame;

    const unsigned path_size = unsigned(path.size());

    unsigned next_index = current_index + 1;

    if (next_index >= path_size) {
        next_index = 0;
    }

    // Linear interpolation ratio.
    bn::fixed ratio = bn::fixed(current_frame) / path_waits;

    const bn::fixed_point& current = path[current_index];
    const bn::fixed_point& next = path[next_index];

    // Interpolate between path nodes.
    pos.x = start_x + current.x() + (next.x() - current.x()) * ratio;

    pos.y = start_y + current.y() + (next.y() - current.y()) * ratio;

    // Advance to next segment.
    if (current_frame >= path_waits) {
        current_frame = 0;
        current_index = next_index;
    }
}

void PathTrap::reset() {
    trigger.reset();

    current_frame = 0;
    current_index = 0;

    pos.x = path.empty() ? start_x : start_x + path[0].x();

    pos.y = path.empty() ? start_y : start_y + path[0].y();
}
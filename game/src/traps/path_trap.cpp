#include "path_trap.h"

PathTrap::PathTrap(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    const bn::sprite_item& sprite,
    int sprite_waits,
    bn::span<const uint16_t> animation_frames,
    uint16_t block,
    bn::span<const bn::fixed_point> path,
    unsigned path_waits,
    Trigger& trigger)
    : BaseTrap(
          path.empty() ? start_x : start_x + path[0].x(),
          path.empty() ? start_y : start_y + path[0].y(),
          width,
          height,
          sprite,
          sprite_waits,
          animation_frames,
          block,
          100),
      start_x(start_x),
      start_y(start_y),
      trigger(trigger),
      path(path),
      inverse_path_waits(path_waits > 0 ? bn::fixed(1) / path_waits : 0),
      path_waits(path_waits) {}

void PathTrap::update() {
    BaseTrap::update();

    // Path movement disabled if:
    // - trigger inactive
    // - not enough path points
    // - invalid wait duration
    if (!trigger.is_triggered() || path.size() < 2 || path_waits == 0) {
        return;
    }

    ++current_frame;

    unsigned next_index = current_index + 1;

    if (next_index >= path.size()) {
        next_index = 0;
    }

    // Cache references to reduce repeated span access.
    const bn::fixed_point& current = path[current_index];
    const bn::fixed_point& next = path[next_index];

    // Multiplication is much cheaper than division on ARM7TDMI.
    bn::fixed ratio = current_frame * inverse_path_waits;

    // Linear interpolation between path nodes.
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

    if (path.empty()) {
        pos.x = start_x;
        pos.y = start_y;
    } else {
        pos.x = start_x + path[0].x();
        pos.y = start_y + path[0].y();
    }
}
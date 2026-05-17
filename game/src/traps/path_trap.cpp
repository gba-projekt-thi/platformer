#include "path_trap.h"

PathTrap::PathTrap(
    bn::fixed start_x,
    bn::fixed start_y,
    bn::fixed width,
    bn::fixed height,
    const bn::sprite_item& sprite,
    int sprite_waits,
    const bn::span<const uint16_t> graphics_indexes,
    uint16_t block,
    bn::span<const bn::fixed_point> path_points_arg,
    unsigned int path_waits_val,  // frames between current path index and next
    Trigger& trigger_ref_arg)
    : BaseTrap::BaseTrap(
          (path_points_arg.empty()) ? start_x
                                    : start_x + path_points_arg[0].x(),
          (path_points_arg.empty()) ? start_y
                                    : start_y + path_points_arg[0].y(),
          width,
          height,
          sprite,
          sprite_waits,
          graphics_indexes,
          block,
          100),
      start_x(start_x),
      start_y(start_y),
      trigger_ref(trigger_ref_arg),
      path_points(path_points_arg),
      path_waits(path_waits_val),
      current_frame(0),
      current_index(0) {}

void PathTrap::update() {
    BaseTrap::update();
    if (!trigger_ref.is_triggered() || path_points.size() < 2 ||
        path_waits <= 0) {
        return;
    }

    current_frame++;

    int next_index = current_index + 1;
    if (next_index >= path_points.size()) {
        next_index = 0;
    }

    bn::fixed ratio = bn::fixed(current_frame) / path_waits;

    bn::fixed current_x = path_points[current_index].x();
    bn::fixed current_y = path_points[current_index].y();

    bn::fixed target_x =
        current_x + (path_points[next_index].x() - current_x) * ratio;
    bn::fixed target_y =
        current_y + (path_points[next_index].y() - current_y) * ratio;

    pos.x = start_x + target_x;
    pos.y = start_y + target_y;

    if (current_frame >= path_waits) {
        current_frame = 0;
        current_index = next_index;
    }
}

void PathTrap::reset() {
    // Reset the trap back to its starting position and stop movement.
    trigger_ref.reset();
    current_index = 0;
    current_frame = 0;
    pos.x = (path_points.empty()) ? start_x : start_x + path_points[0].x();
    pos.y = (path_points.empty()) ? start_y : start_y + path_points[0].y();
}
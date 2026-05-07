#include "path_trap.h"

PathTrap::PathTrap(
    bn::fixed t_start_x,
    bn::fixed t_start_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite,
    int t_sprite_waits,
    const bn::span<const uint16_t> t_graphics_indexes,
    uint16_t t_block,
    bn::span<const bn::fixed_point> t_path,
    unsigned int t_path_waits,  // how many frames between current path iondex and next
    Trigger& t_trigger)
    : BaseTrap::BaseTrap(
          (t_path.empty()) ? t_start_x : t_start_x + t_path[0].x(),
          (t_path.empty()) ? t_start_y : t_start_y + t_path[0].y(),
          t_width,
          t_height,
          t_sprite,
          t_sprite_waits,
          t_graphics_indexes,
          t_block,
          100),
      start_x(t_start_x),
      start_y(t_start_y),
      trigger(t_trigger),
      path(t_path),
      path_waits(t_path_waits),
      current_frame(0),
      current_index(0) {}

void PathTrap::update() {
    BaseTrap::update();
    if (!trigger.is_triggered() || path.size() < 2 || path_waits <= 0) {
        return;
    }
    
    current_frame++;

    int next_index = current_index + 1;
    if (next_index >= path.size()) {
        next_index = 0;
    }

    bn::fixed ratio = bn::fixed(current_frame) / path_waits;


    bn::fixed current_x = path[current_index].x();
    bn::fixed current_y = path[current_index].y();

    bn::fixed target_x =
        current_x + (path[next_index].x() - current_x) * ratio;
    bn::fixed target_y =
        current_y + (path[next_index].y() - current_y) * ratio;


    pos.x = (start_x + target_x);
    pos.y = (start_y + target_y);

    if (current_frame >= path_waits) {
        current_frame = 0;
        current_index = next_index;
    }
    
}

void PathTrap::reset() {
    // Reset the trap back to its starting position and stop movement.
    trigger.reset();
    current_index = 0;
    current_frame = 0;
    pos.x = (path.empty()) ? start_x : start_x + path[0].x();
    pos.y = (path.empty()) ? start_y : start_y + path[0].y();
}

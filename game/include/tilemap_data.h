#pragma once

#include "bn_regular_bg_map_cell.h"

// This empty tilemap exists to overwrite the tilemap from the Collision System
// for now

constexpr int map_width = 0;
constexpr int map_height = 0;

alignas(4) constexpr bn::regular_bg_map_cell map_cells[1] = {0};

constexpr int map_rect_count = 0;

constexpr MapRect map_rects[1] = {};

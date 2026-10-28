#pragma once

// -----------------------------------------------------------------------------
// menu_nav
//
// Wrap-around cursor movement shared by the list menus (start screen, pause
// menu). Dependency-free so it can be unit-tested on the host (see
// tests/host/test_menu_nav.cpp).
// -----------------------------------------------------------------------------

namespace menu_nav {

inline auto next_index(int current, int count) -> int {
    return count <= 0 ? 0 : (current + 1) % count;
}

inline auto prev_index(int current, int count) -> int {
    return count <= 0 ? 0 : (current + count - 1) % count;
}

// The y of a list's first row: `top` as usual, higher up when the list
// would otherwise put its last row below `last_row_y`.
inline auto first_row_y(int rows, int top, int spacing, int last_row_y) -> int {
    const int fit = last_row_y - (rows - 1) * spacing;
    return fit < top ? fit : top;
}

}  // namespace menu_nav

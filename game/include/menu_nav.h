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

}  // namespace menu_nav

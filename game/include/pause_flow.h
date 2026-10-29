#pragma once

// -----------------------------------------------------------------------------
// pause_flow
//
// When the pause overlay's sprites exist. Butano counts a hidden sprite like a
// visible one, and the overlay needs up to ~63 of the 128 sprites while it is
// open, so it is built when the game pauses and RELEASED (not hidden) on every
// way out; a level that runs holds none of them (tools/Check_sprite_budget.py).
//
// The sprites themselves need Butano; the order of building and releasing does
// not. PauseController is the real Host (PauseController::FlowOps in
// pause_controller.cpp); tests/host/test_pause_flow.cpp plays the same code
// with a Host that counts the sprites alive.
// -----------------------------------------------------------------------------

namespace pause_flow {

// The rows of the pause menu, in the order they are listed.
enum class Option : int {
    Continue = 0,
    Restart = 1,
    Options = 2,
    Controls = 3,
    TitleScreen = 4,
    Count = 5
};

// What the level has to do once the menu closed on a confirmed row
// (PauseController::Action).
enum class Action { None, RestartRequested, ReturnToTitleRequested };

inline auto action_for(Option option) -> Action {
    switch (option) {
        case Option::Restart:
            return Action::RestartRequested;
        case Option::TitleScreen:
            return Action::ReturnToTitleRequested;
        default:
            return Action::None;
    }
}

// Frees the title and the menu rows (any container with clear()).
template <typename Title, typename Rows>
void release(Title& title, Rows& rows) {
    title.clear();
    rows.clear();
}

// The game just paused: the menu rows, then the title above them. The level
// behind is dimmed (text sprites have blending off, so they stay white); every
// way out undims it again.
template <typename Host>
void open(Host& h) {
    h.select_first_option();
    h.leave_submenus();
    h.rebuild_menu();
    h.show_title();
    h.dim_level();
    h.pause_music();
    h.sfx_menu();
}

// The game just resumed (Start, also from inside a sub-menu).
template <typename Host>
void close(Host& h) {
    h.release_sprites();
    h.undim_level();
    h.sfx_cancel();
    h.resume_music();
}

// A, B in the controls view: back to the menu.
template <typename Host>
void leave_controls(Host& h) {
    h.show_title();
    h.rebuild_menu();
    h.sfx_cancel();
}

// The controls list replaces the menu rows; it has no title of its own.
template <typename Host>
void open_controls(Host& h) {
    h.set_showing_controls();
    h.release_title();
    h.rebuild_controls_view();
}

// Continue / Restart Level / Title Screen: the menu closes on the very frame
// the action is confirmed, whatever the caller does with the action (a scene
// transition may destroy the controller).
template <typename Host>
void leave_for_action(Host& h) {
    h.set_unpaused();
    h.release_sprites();
    h.undim_level();
    h.resume_music();
}

// Level load: only the text generator is made (it owns no sprites); the title
// and the rows come when the game pauses.
template <typename Host>
void reset(Host& h) {
    h.init_text_generator();
    h.reset_state();
    h.release_sprites();
    h.undim_level();
}

}  // namespace pause_flow

#include "test_framework.h"

#include <string>
#include <vector>

#include "pause_flow.h"

// When the pause overlay's sprites exist (pause_flow.h): built when the game
// pauses, released on every way out, never kept while the level runs.

namespace {

struct Host {
    std::vector<std::string> log;
    int title_sprites = 0;
    int menu_sprites = 0;
    bool paused = false;
    bool controls_view = false;
    int music_paused = 0;
    bool dimmed = false;

    [[nodiscard]] auto alive() const -> int {
        return title_sprites + menu_sprites;
    }
    [[nodiscard]] auto before(const std::string& a, const std::string& b) const
        -> bool {
        int ia = -1;
        int ib = -1;
        for (int i = 0; i < int(log.size()); ++i) {
            if (log[i] == a && ia < 0)
                ia = i;
            if (log[i] == b && ib < 0)
                ib = i;
        }
        return ia >= 0 && ib >= 0 && ia < ib;
    }

    void select_first_option() {}
    void leave_submenus() { controls_view = false; }
    void rebuild_menu() {
        log.emplace_back("rebuild_menu");
        menu_sprites = 5;
    }
    void show_title() {
        log.emplace_back("show_title");
        title_sprites = 6;
    }
    void pause_music() { ++music_paused; }
    void resume_music() { --music_paused; }
    void dim_level() { dimmed = true; }
    void undim_level() { dimmed = false; }
    void sfx_menu() {}
    void sfx_cancel() {}
    void release_sprites() {
        log.emplace_back("release_sprites");
        title_sprites = 0;
        menu_sprites = 0;
    }
    void set_showing_controls() { controls_view = true; }
    void release_title() { title_sprites = 0; }
    void rebuild_controls_view() {
        log.emplace_back("rebuild_controls_view");
        menu_sprites = 9;
    }
    void set_unpaused() {
        log.emplace_back("set_unpaused");
        paused = false;
    }
    void init_text_generator() {}  // owns no sprites
    void reset_state() { paused = false; }
};

}  // namespace

TEST_CASE(pause_flow_opening_builds_the_rows_then_the_title) {
    Host h;
    pause_flow::open(h);
    CHECK(h.before("rebuild_menu", "show_title"));
    CHECK(h.title_sprites > 0);
    CHECK(h.menu_sprites > 0);
    CHECK_EQ(h.music_paused, 1);
}

TEST_CASE(pause_flow_the_level_is_dimmed_while_paused_and_clear_after) {
    Host h;
    pause_flow::open(h);
    CHECK(h.dimmed);
    pause_flow::open_controls(h);
    CHECK(h.dimmed);  // sub-views keep the dimming
    pause_flow::leave_controls(h);
    CHECK(h.dimmed);
    pause_flow::close(h);
    CHECK(!h.dimmed);
}

TEST_CASE(pause_flow_every_way_out_undims_the_level) {
    Host h;
    pause_flow::open(h);
    pause_flow::leave_for_action(h);  // Continue, Restart, Title Screen
    CHECK(!h.dimmed);

    pause_flow::open(h);
    pause_flow::reset(h);  // level load while paused
    CHECK(!h.dimmed);
}

TEST_CASE(pause_flow_resuming_releases_everything) {
    Host h;
    pause_flow::open(h);
    pause_flow::close(h);
    CHECK_EQ(h.alive(), 0);
    CHECK_EQ(h.music_paused, 0);
}

TEST_CASE(pause_flow_resuming_from_the_controls_view_releases_everything) {
    Host h;
    pause_flow::open(h);
    pause_flow::open_controls(h);
    pause_flow::close(h);  // Start inside the sub-menu
    CHECK_EQ(h.alive(), 0);
}

TEST_CASE(pause_flow_the_controls_view_drops_the_title) {
    Host h;
    pause_flow::open(h);
    pause_flow::open_controls(h);
    CHECK_EQ(h.title_sprites, 0);
    CHECK(h.menu_sprites > 0);
    CHECK(h.controls_view);
}

TEST_CASE(pause_flow_leaving_the_controls_view_brings_the_title_back) {
    Host h;
    pause_flow::open(h);
    pause_flow::open_controls(h);
    pause_flow::leave_controls(h);
    CHECK(h.title_sprites > 0);
    CHECK(h.menu_sprites > 0);
}

TEST_CASE(
    pause_flow_every_confirmed_action_releases_the_sprites_with_the_pause_off) {
    // Continue, Restart Level, Title Screen: released before the caller sees
    // the action.
    Host h;
    h.paused = true;
    pause_flow::open(h);
    pause_flow::leave_for_action(h);
    CHECK(!h.paused);
    CHECK_EQ(h.alive(), 0);
    CHECK(h.before("set_unpaused", "release_sprites"));
    CHECK_EQ(h.music_paused, 0);
}

TEST_CASE(
    pause_flow_a_level_load_releases_whatever_was_left_and_builds_nothing) {
    Host h;
    pause_flow::open(h);  // e.g. unloaded while paused
    pause_flow::reset(h);
    CHECK_EQ(h.alive(), 0);
    CHECK(!h.paused);
}

TEST_CASE(pause_flow_a_second_pause_after_a_resume_leaves_no_old_menu_behind) {
    Host h;
    pause_flow::open(h);
    pause_flow::close(h);
    pause_flow::open(h);
    pause_flow::close(h);
    CHECK_EQ(h.alive(), 0);
}

TEST_CASE(pause_flow_confirming_restart_or_title_screen_tells_the_level) {
    using pause_flow::Action;
    using pause_flow::Option;
    CHECK(pause_flow::action_for(Option::Restart) == Action::RestartRequested);
    CHECK(
        pause_flow::action_for(Option::TitleScreen) ==
        Action::ReturnToTitleRequested);
    CHECK(pause_flow::action_for(Option::Continue) == Action::None);
    CHECK(pause_flow::action_for(Option::Options) == Action::None);
    CHECK(pause_flow::action_for(Option::Controls) == Action::None);
}

TEST_CASE(pause_flow_release_frees_the_title_and_the_rows) {
    std::vector<int> title(6);
    std::vector<int> rows(5);
    pause_flow::release(title, rows);
    CHECK(title.empty());
    CHECK(rows.empty());
}

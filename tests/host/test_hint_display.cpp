#include "test_framework.h"

#include <vector>

#include "hint_logic.h"

// The life of a shown hint (hint::Display) and the scene's frame around the
// level update (hint::step_level).

namespace {

using hint::Sprites;

// Plays frames of a hint whose sprites follow what update() says.
struct Play {
    hint::Display display{3};
    bool sprites = false;
    bool paused = false;
    bool arena = false;
    bool b = false;

    void show(bool waits = false) { sprites = display.show(waits); }
    auto frame() -> Sprites {
        const Sprites what = display.update(paused, arena, b, sprites);
        if (what == Sprites::Draw) {
            sprites = true;
        }
        if (what == Sprites::Release || what == Sprites::Finish) {
            sprites = false;
        }
        return what;
    }
};

struct StepHost {
    std::vector<int> log;  // 1 = hint released, 2 = level update
    bool start = false;
    [[nodiscard]] auto start_pressed() const -> bool { return start; }
    void release_hint_sprites() { log.push_back(1); }
    auto update_level() -> int {
        log.push_back(2);
        return 7;
    }
};

}  // namespace

TEST_CASE(
    hint_display_is_drawn_at_once_on_a_normal_level_and_ends_after_its_frames) {
    Play play;
    play.show();
    CHECK(play.sprites);
    CHECK(play.frame() == Sprites::Keep);
    CHECK(play.frame() == Sprites::Keep);
    CHECK(play.frame() == Sprites::Finish);
    CHECK(!play.sprites);
    CHECK(!play.display.active());
}

TEST_CASE(hint_display_the_countdown_and_b_only_run_while_the_game_does) {
    Play play;
    play.show();
    play.paused = true;
    play.b = true;  // 'back' in the pause menu, not a dismissal
    for (int i = 0; i < 10; ++i) {
        CHECK(play.frame() != Sprites::Finish);
    }
    CHECK(play.display.active());
    play.paused = false;
    CHECK(play.frame() == Sprites::Finish);
}

TEST_CASE(
    hint_display_sprites_are_released_while_paused_and_drawn_again_after) {
    Play play;
    play.show();
    play.paused = true;
    CHECK(play.frame() == Sprites::Release);
    CHECK(!play.sprites);
    CHECK(play.frame() == Sprites::Release);
    play.paused = false;
    CHECK(play.frame() == Sprites::Draw);
    CHECK(play.sprites);
    CHECK(play.frame() == Sprites::Keep);
}

TEST_CASE(hint_display_a_boss_hint_waits_for_the_arena) {
    Play play;
    play.show(true);
    CHECK(!play.sprites);
    for (int i = 0; i < 10; ++i) {
        CHECK(play.frame() == Sprites::Keep);
    }
    CHECK(play.display.active());
    play.arena = true;
    CHECK(play.frame() == Sprites::Draw);
    CHECK(play.sprites);
}

TEST_CASE(hint_display_without_a_hint_does_nothing) {
    hint::Display display(3);
    CHECK(display.update(false, true, true, false) == Sprites::Keep);
}

TEST_CASE(hint_the_sprites_are_freed_before_the_pause_overlay_allocates) {
    // LevelManager::update() opens the pause menu, which allocates ~57
    // sprites in that very call, so the hint has to go first.
    StepHost host;
    host.start = true;
    CHECK_EQ(hint::step_level(host), 7);
    CHECK_EQ(int(host.log.size()), 2);
    CHECK_EQ(host.log[0], 1);
    CHECK_EQ(host.log[1], 2);
}

TEST_CASE(hint_the_sprites_stay_when_start_is_not_pressed) {
    StepHost host;
    CHECK_EQ(hint::step_level(host), 7);
    CHECK_EQ(int(host.log.size()), 1);
    CHECK_EQ(host.log[0], 2);
}

#include "test_framework.h"

#include "camera.h"
#include "camera_rig.h"
#include "locomotion.h"

// What the player sees: the duck moved by the real movement rules, the camera
// aimed by CameraRig and placed by the engine's Camera, and every sprite put
// on the screen the way Butano does it (world - camera, cut to whole pixels).

namespace {

struct Body {
    struct {
        bn::fixed x = 0;
        bn::fixed y = 0;
    } pos;
    bn::fixed vel_x = 0;
    bn::fixed vel_y = 0;
};

struct Floor {
    auto probe_ground() -> bool { return true; }
    auto probe_ceiling() -> bool { return false; }
    auto probe_left() -> bool { return false; }
    auto probe_right() -> bool { return false; }
    auto probe_pogo() -> bool { return false; }
    void die() {}
};

// Screen pixel of a world x, like Sprite::sync() + the sprite hardware.
auto screen_x(bn::fixed world_x) -> int {
    return Camera::instance().to_screen_x(world_x).right_shift_integer();
}

struct View {
    locomotion::Core core;
    Body duck;
    Floor floor;
    CameraRig<bn::fixed> rig;
    // A platform somewhere in the world; it only moves with the camera.
    bn::fixed platform_x = 37;

    View() {
        Camera::instance().init(2000, 160);
        Camera::instance().set_shake(0, 0);
        core.set_horizontal_bound(900);
        rig.set_bounds(camera_rig::max_offset(bn::fixed(2000), 240), 0);
        rig.snap(duck.pos.x, duck.pos.y);
        Camera::instance().follow(rig.x(), rig.y());
    }

    // One frame like LevelManager::update(): move, aim, place the camera.
    void frame(const InputState& input) {
        core.step(duck, input, floor);
        duck.pos.x += duck.vel_x;  // flat floor: nothing blocks
        rig.update(duck.pos.x, duck.pos.y, duck.vel_x, 0);
        Camera::instance().follow(rig.x(), rig.y());
    }
};

auto right() -> InputState {
    InputState input;
    input.right_held = true;
    return input;
}

}  // namespace

// How often the duck's screen position changes direction (right, then left,
// or the other way round) over `frames` frames of `input_at(frame)`.
template <typename Input>
auto screen_reversals(View& view, int frames, Input input_at) -> int {
    int last = screen_x(view.duck.pos.x);
    int last_dir = 0;
    int reversals = 0;
    for (int frame = 0; frame < frames; ++frame) {
        view.frame(input_at(frame));
        const int now = screen_x(view.duck.pos.x);
        const int dir = now > last ? 1 : (now < last ? -1 : 0);
        if (dir != 0) {
            if (last_dir != 0 && dir != last_dir) {
                ++reversals;
            }
            last_dir = dir;
        }
        last = now;
    }
    return reversals;
}

TEST_CASE(camera_view_a_walking_duck_turns_on_screen_only_once) {
    // It walks right inside the dead zone, then the look-ahead lets it drift
    // back left of the middle: one slow turn, no back and forth.
    View view;
    const int reversals = screen_reversals(view, 120, [](int frame) {
        return frame < 60 ? right() : InputState{};
    });
    CHECK(reversals <= 1);
}

TEST_CASE(camera_view_a_tapping_duck_does_not_wobble_on_screen) {
    // Short taps (the "slight movements" that used to jitter): the duck moves,
    // the view follows, nothing slides back.
    View view;
    const int reversals = screen_reversals(view, 130, [](int frame) {
        return frame % 13 < 3 ? right() : InputState{};
    });
    CHECK_EQ(reversals, 0);
}

TEST_CASE(camera_view_the_world_never_scrolls_back_while_walking_right) {
    View view;
    int last = screen_x(view.platform_x);
    int backward_steps = 0;
    for (int frame = 0; frame < 120; ++frame) {
        view.frame(frame < 60 ? right() : InputState{});
        const int now = screen_x(view.platform_x);
        if (now > last) {
            ++backward_steps;
        }
        last = now;
    }
    CHECK_EQ(backward_steps, 0);
}

TEST_CASE(camera_view_duck_and_world_move_in_whole_pixel_steps_together) {
    // The distance between the duck and a platform on screen must follow the
    // duck's own movement, not the camera's: while the duck walks right it
    // only ever grows.
    View view;
    int last = screen_x(view.duck.pos.x) - screen_x(view.platform_x);
    int shrinks = 0;
    for (int frame = 0; frame < 60; ++frame) {
        view.frame(right());
        const int now = screen_x(view.duck.pos.x) - screen_x(view.platform_x);
        if (now < last) {
            ++shrinks;
        }
        last = now;
    }
    CHECK_EQ(shrinks, 0);
}

TEST_CASE(camera_view_comes_to_rest_soon_after_the_duck_stops) {
    View view;
    for (int frame = 0; frame < 60; ++frame) {
        view.frame(right());
    }
    // The duck brakes to a halt within ~10 frames; give the camera 60 more.
    for (int frame = 0; frame < 70; ++frame) {
        view.frame({});
    }
    const int rest = screen_x(view.platform_x);
    int moves = 0;
    for (int frame = 0; frame < 120; ++frame) {
        view.frame({});
        if (screen_x(view.platform_x) != rest) {
            ++moves;
        }
    }
    CHECK_EQ(moves, 0);
}

TEST_CASE(camera_view_a_small_step_does_not_scroll_the_world) {
    View view;
    const int before = screen_x(view.platform_x);
    // A short tap: the duck moves a few pixels.
    for (int frame = 0; frame < 4; ++frame) {
        view.frame(right());
    }
    for (int frame = 0; frame < 60; ++frame) {
        view.frame({});
    }
    CHECK_EQ(screen_x(view.platform_x), before);
}

#include "test_framework.h"

#include <vector>

#include "locomotion.h"

// The adapter's frame (locomotion::run_frame): every button reaches the core,
// every effect of the frame becomes its sound and event, and the events of the
// frame before are over first.

namespace {

using locomotion::Sound;

struct FakeKeypad {
    bool left = false, right = false, a_press = false, a_hold = false,
         b_press = false, r_press = false;
    bool left_held() const { return left; }
    bool right_held() const { return right; }
    bool a_pressed() const { return a_press; }
    bool a_held() const { return a_hold; }
    bool b_pressed() const { return b_press; }
    bool r_pressed() const { return r_press; }
};

struct FakeBody {
    struct {
        bn::fixed x = 0;
        bn::fixed y = 0;
    } pos;
    bn::fixed vel_x = 0;
    bn::fixed vel_y = 0;
};

struct FakeWorld {
    bool probe_ground() { return true; }
    bool probe_ceiling() { return false; }
    bool probe_left() { return false; }
    bool probe_right() { return false; }
    bool probe_pogo() { return false; }
    void die() {}
};

struct Log {
    std::vector<int> sounds;
    int begins = 0, dashes = 0, jumps = 0;
    void begin_frame() { ++begins; }
    void sound(Sound s) { sounds.push_back(int(s)); }
    void note_dash() { ++dashes; }
    void note_jump() { ++jumps; }
};

struct Rig {
    locomotion::Core core;
    FakeBody body;
    FakeWorld world;
    FakeKeypad keypad;
    Log log;
    Rig() { core.set_horizontal_bound(1000); }
    auto frame(void (*hook)(InputState&) = nullptr) -> locomotion::Effects {
        return locomotion::run_frame(core, body, keypad, hook, world, log);
    }
};

}  // namespace

TEST_CASE(locomotion_frame_every_button_reaches_the_core) {
    // Each button alone changes what the duck does; a button that is not read
    // from the keypad would leave it standing.
    Rig right;
    right.keypad.right = true;
    right.frame();
    CHECK(right.body.vel_x > 0);

    Rig left;
    left.keypad.left = true;
    left.frame();
    CHECK(left.body.vel_x < 0);

    Rig jump;
    jump.keypad.a_press = true;
    jump.keypad.a_hold = true;
    CHECK(jump.frame().jumped);
    CHECK(jump.body.vel_y < 0);
}

TEST_CASE(locomotion_frame_b_and_r_reach_the_core_as_dash_and_pogo_buttons) {
    // The raw buttons as the core sees them, through the input hook.
    static InputState seen;
    Rig rig;
    rig.keypad.b_press = true;
    rig.keypad.r_press = true;
    rig.keypad.a_press = true;
    rig.keypad.a_hold = true;
    rig.keypad.left = true;
    rig.keypad.right = true;
    rig.frame([](InputState& in) { seen = in; });
    CHECK(seen.b_pressed);
    CHECK(seen.r_pressed);
    CHECK(seen.a_pressed);
    CHECK(seen.a_held);
    CHECK(seen.left_held);
    CHECK(seen.right_held);
}

TEST_CASE(locomotion_frame_the_hook_sees_the_buttons_before_the_core) {
    Rig rig;  // no button pressed: the hook presses right
    rig.frame([](InputState& in) { in.right_held = true; });
    CHECK(rig.body.vel_x > 0);
}

TEST_CASE(locomotion_frame_starts_by_ending_the_previous_events) {
    Rig rig;
    rig.frame();
    rig.frame();
    CHECK_EQ(rig.log.begins, 2);
}

TEST_CASE(locomotion_frame_a_jump_is_heard_and_noted) {
    Rig rig;
    rig.keypad.a_press = true;
    rig.keypad.a_hold = true;
    rig.frame();
    CHECK_EQ(rig.log.jumps, 1);
    CHECK_EQ(rig.log.dashes, 0);
    bool heard = false;
    for (int s : rig.log.sounds)
        heard |= s == int(Sound::Jump);
    CHECK(heard);
}

TEST_CASE(locomotion_frame_announce_maps_every_effect) {
    {
        Log log;
        locomotion::Effects e;
        e.dash_started = true;
        locomotion::announce(e, log);
        CHECK_EQ(log.dashes, 1);
        CHECK_EQ(log.jumps, 0);
        CHECK_EQ(int(log.sounds.size()), 1);
        CHECK_EQ(log.sounds[0], int(Sound::Dash));
    }
    {
        Log log;
        locomotion::Effects e;
        e.step_sound = true;
        locomotion::announce(e, log);
        CHECK_EQ(int(log.sounds.size()), 1);
        CHECK_EQ(log.sounds[0], int(Sound::Step));
        CHECK_EQ(log.dashes + log.jumps, 0);
    }
    {
        Log log;
        locomotion::Effects e;
        e.pogo_started = true;
        locomotion::announce(e, log);
        CHECK_EQ(int(log.sounds.size()), 1);
        CHECK_EQ(log.sounds[0], int(Sound::Pogo));
        CHECK_EQ(log.dashes + log.jumps, 0);
    }
    {
        // a bounce rings off the trap and is a jump for the triggers
        Log log;
        locomotion::Effects e;
        e.pogo_bounced = true;
        locomotion::announce(e, log);
        CHECK_EQ(int(log.sounds.size()), 1);
        CHECK_EQ(log.sounds[0], int(Sound::TrapHit));
        CHECK_EQ(log.jumps, 1);
    }
    {
        Log log;
        locomotion::Effects e;
        locomotion::announce(e, log);
        CHECK(log.sounds.empty());
    }
}

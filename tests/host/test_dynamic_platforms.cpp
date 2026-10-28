#include "test_framework.h"

#include "dynamic_platforms.h"
#include "position.h"

// Switched and crumbling platforms (DynamicPlatforms) with stand-ins for the
// engine's StaticBody, Sprite and the game's Trigger.

namespace {

struct FakeBody {
    Position pos;
    bool enabled = true;
    [[nodiscard]] auto is_enabled() const -> bool { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
};

struct FakeSprite {
    Position pos;
    bool enabled = true;
    void enable() { enabled = true; }
    void disable() { enabled = false; }
};

struct FakeTrigger {
    bool on = false;
    [[nodiscard]] auto is_triggered() const -> bool { return on; }
};

using Platforms = DynamicPlatforms<FakeBody, FakeSprite, FakeTrigger, 4>;

// A 32x16 platform centred on (x, y).
auto platform(int x, int y) -> FakeBody {
    FakeBody body;
    body.pos = Position{x, y, 32, 16};
    return body;
}

// The duck's 8x8 box.
auto duck_at(int x, int y) -> Position {
    return Position{x, y, 8, 8};
}

const Position FAR_AWAY = duck_at(500, 500);

auto never(const FakeBody&) -> bool {
    return false;
}

}  // namespace

// ---- switched
// -----------------------------------------------------------------

TEST_CASE(dynamic_platforms_switched_on_platform_follows_its_trigger) {
    FakeBody body = platform(0, 0);
    FakeSprite sprite;
    FakeTrigger trigger;
    Platforms platforms;
    platforms.add_switched(trigger, body, sprite, true);

    platforms.sync_switched(FAR_AWAY);
    CHECK(!body.enabled);
    CHECK(!sprite.enabled);

    trigger.on = true;
    platforms.sync_switched(FAR_AWAY);
    CHECK(body.enabled);
    CHECK(sprite.enabled);
}

TEST_CASE(dynamic_platforms_switched_off_platform_is_the_inverse) {
    FakeBody body = platform(0, 0);
    FakeSprite sprite;
    FakeTrigger trigger;
    Platforms platforms;
    platforms.add_switched(trigger, body, sprite, false);

    platforms.sync_switched(FAR_AWAY);
    CHECK(body.enabled);
    trigger.on = true;
    platforms.sync_switched(FAR_AWAY);
    CHECK(!body.enabled);
    CHECK(!sprite.enabled);
}

TEST_CASE(dynamic_platforms_a_platform_never_appears_inside_the_duck) {
    FakeBody body = platform(0, 0);
    FakeSprite sprite;
    FakeTrigger trigger;
    Platforms platforms;
    platforms.add_switched(trigger, body, sprite, true);
    platforms.sync_switched(FAR_AWAY);

    trigger.on = true;
    platforms.sync_switched(duck_at(0, 0));
    CHECK(!body.enabled);  // held back while the duck is inside
    CHECK(!sprite.enabled);
    platforms.sync_switched(duck_at(30, 0));  // stepped clear
    CHECK(body.enabled);
    CHECK(sprite.enabled);
}

TEST_CASE(dynamic_platforms_a_platform_vanishes_even_under_the_duck) {
    FakeBody body = platform(0, 0);
    FakeSprite sprite;
    FakeTrigger trigger;
    Platforms platforms;
    platforms.add_switched(trigger, body, sprite, false);
    trigger.on = true;
    platforms.sync_switched(duck_at(0, -12));  // standing on it
    CHECK(!body.enabled);
}

// ---- crumbling
// ----------------------------------------------------------------

namespace {

struct Crumbler {
    FakeBody body = platform(0, 0);
    FakeSprite sprite;
    Platforms platforms;
    bool standing = false;

    Crumbler(unsigned crumble_frames, unsigned respawn_frames) {
        platforms.add_crumbling(body, sprite, crumble_frames, respawn_frames);
    }

    void frame(const Position& duck = FAR_AWAY) {
        platforms.sync_crumbling(duck, [this](const FakeBody& candidate) {
            return standing && &candidate == &body;
        });
    }

    void frames(int count) {
        for (int i = 0; i < count; ++i) {
            frame();
        }
    }
};

}  // namespace

TEST_CASE(dynamic_platforms_crumbling_holds_until_the_duck_stands_on_it) {
    Crumbler c(10, 0);
    c.frames(100);
    CHECK(c.body.enabled);
    CHECK(c.sprite.enabled);
    CHECK_EQ(c.sprite.pos.offset_x, bn::fixed(0));
}

TEST_CASE(dynamic_platforms_crumbling_shakes_then_gives_way) {
    Crumbler c(10, 0);
    c.standing = true;
    c.frame();
    c.standing = false;
    c.frame();
    CHECK(c.sprite.pos.offset_x != bn::fixed(0));  // shaking
    CHECK(c.body.enabled);
    c.frames(8);
    CHECK(c.body.enabled);  // 9 frames after the first contact
    c.frame();
    CHECK(!c.body.enabled);   // fell 10 frames after the first contact
    CHECK(c.sprite.enabled);  // still visible while falling
    c.frames(3);  // the drop accelerates from 0 px (crumble::fall_offset())
    CHECK(c.sprite.pos.offset_y > bn::fixed(0));
    c.frames(crumble::FALL_FRAMES);
    CHECK(!c.sprite.enabled);
}

TEST_CASE(
    dynamic_platforms_crumbled_platform_comes_back_but_not_into_the_duck) {
    Crumbler c(1, 30);
    c.standing = true;
    c.frame();
    c.standing = false;
    // Falls, drops out of the picture, then waits 30 frames; on the last of
    // them the duck stands where the platform would come back.
    c.frames(1 + crumble::FALL_FRAMES + 29);
    c.frame(duck_at(0, 0));
    CHECK(!c.body.enabled);
    c.frame();
    CHECK(c.body.enabled);
    CHECK(c.sprite.enabled);
    CHECK_EQ(c.sprite.pos.offset_y, bn::fixed(0));
}

TEST_CASE(dynamic_platforms_reset_puts_every_crumbled_platform_back) {
    Crumbler c(1, 0);
    c.standing = true;
    c.frame();
    c.standing = false;
    c.frames(5);
    CHECK(!c.body.enabled);
    c.platforms.reset_crumbling();
    CHECK(c.body.enabled);
    CHECK(c.sprite.enabled);
    CHECK_EQ(c.sprite.pos.offset_x, bn::fixed(0));
    CHECK_EQ(c.sprite.pos.offset_y, bn::fixed(0));
    c.frames(100);
    CHECK(c.body.enabled);  // untouched again
}

TEST_CASE(dynamic_platforms_only_the_platform_under_the_duck_crumbles) {
    FakeBody under = platform(0, 0);
    FakeBody other = platform(100, 0);
    FakeSprite under_sprite;
    FakeSprite other_sprite;
    Platforms platforms;
    platforms.add_crumbling(under, under_sprite, 1, 0);
    platforms.add_crumbling(other, other_sprite, 1, 0);
    for (int i = 0; i < 5; ++i) {
        platforms.sync_crumbling(
            FAR_AWAY, [&](const FakeBody& body) { return &body == &under; });
    }
    CHECK(!under.enabled);
    CHECK(other.enabled);
}

TEST_CASE(dynamic_platforms_clear_forgets_every_platform) {
    FakeBody body = platform(0, 0);
    FakeSprite sprite;
    FakeTrigger trigger;
    Platforms platforms;
    platforms.add_switched(trigger, body, sprite, true);
    platforms.add_crumbling(body, sprite, 1, 0);
    CHECK(platforms.has_crumbling());
    platforms.clear();
    CHECK(!platforms.has_crumbling());
    platforms.sync_switched(FAR_AWAY);
    platforms.sync_crumbling(FAR_AWAY, never);
    CHECK(body.enabled);  // nothing touched it
}

TEST_CASE(dynamic_platforms_more_than_the_capacity_are_ignored) {
    FakeBody bodies[5];
    FakeSprite sprites[5];
    Platforms platforms;  // capacity 4
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(platforms.add_crumbling(bodies[i], sprites[i], 1, 0), i < 4);
    }
}

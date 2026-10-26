#include "test_framework.h"

#include "fleeing_platforms.h"

// Platforms that run away (fleeing_platforms.h): when the duck comes close a
// platform slides to the next stop of its path, like the runaway door, and a
// death puts it back.

namespace {

struct Pos {
    bn::fixed x = 0;
    bn::fixed y = 0;
    void move(bn::fixed nx, bn::fixed ny) {
        x = nx;
        y = ny;
    }
};
struct Body {
    Pos pos;
};
struct Picture {
    Pos pos;
};
struct Point {
    bn::fixed px;
    bn::fixed py;
    [[nodiscard]] auto x() const -> bn::fixed { return px; }
    [[nodiscard]] auto y() const -> bn::fixed { return py; }
};

constexpr Point AWAY[] = {{48, 0}};

using Platforms = FleeingPlatforms<Body, Picture, Point, 4>;

}  // namespace

TEST_CASE(fleeing_platform_stays_while_the_duck_is_far) {
    Platforms platforms;
    Body body;
    Picture picture;
    platforms.add(body, picture, 100, 60, AWAY, 1, 32, 8);
    platforms.update(0, 60);
    CHECK_EQ(body.pos.x, bn::fixed(100));
    CHECK_EQ(picture.pos.x, bn::fixed(100));
}

TEST_CASE(fleeing_platform_slides_away_when_the_duck_comes_close) {
    Platforms platforms;
    Body body;
    Picture picture;
    platforms.add(body, picture, 100, 60, AWAY, 1, 32, 8);
    platforms.update(70, 50);
    for (int i = 0; i < 10; ++i) {
        platforms.update(70, 50);
    }
    CHECK_EQ(body.pos.x, bn::fixed(148));
    CHECK_EQ(body.pos.y, bn::fixed(60));
    CHECK_EQ(picture.pos.x, bn::fixed(148));
}

TEST_CASE(fleeing_platform_reset_brings_it_back) {
    Platforms platforms;
    Body body;
    Picture picture;
    platforms.add(body, picture, 100, 60, AWAY, 1, 32, 8);
    for (int i = 0; i < 10; ++i) {
        platforms.update(100, 60);
    }
    platforms.reset();
    CHECK_EQ(body.pos.x, bn::fixed(100));
    CHECK_EQ(picture.pos.x, bn::fixed(100));
}

TEST_CASE(fleeing_platforms_clear_forgets_them) {
    Platforms platforms;
    Body body;
    Picture picture;
    platforms.add(body, picture, 100, 60, AWAY, 1, 32, 8);
    platforms.clear();
    platforms.update(100, 60);
    CHECK_EQ(body.pos.x, bn::fixed(0));  // untouched: never placed again
    CHECK(platforms.empty());
}

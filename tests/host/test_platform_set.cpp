#include "test_framework.h"

#include <deque>
#include <memory>
#include <vector>

#include "platform_set.h"
#include "position.h"

// PlatformSet: the platforms of a level and what LevelManager does with them
// (load, attempt start, per-frame work), with stand-ins for StaticBody, Sprite,
// Trigger and PlatformData.

namespace {

struct FakeSprite {
    Position pos;
    bool enabled = true;
    void enable() { enabled = true; }
    void disable() { enabled = false; }
};

struct FakeBody {
    FakeBody(int x, int y, int w, int h) : pos{x, y, w, h} {}
    Position pos;
    FakeSprite* sprite = nullptr;
    bool enabled = true;
    [[nodiscard]] auto is_enabled() const -> bool { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
};

struct FakeTrigger {
    bool on = false;
    [[nodiscard]] auto is_triggered() const -> bool { return on; }
};

struct Point {
    bn::fixed px;
    bn::fixed py;
    [[nodiscard]] auto x() const -> bn::fixed { return px; }
    [[nodiscard]] auto y() const -> bn::fixed { return py; }
};

constexpr Point AWAY[] = {{48, 0}};

struct FakePath {
    const Point* ptr = nullptr;
    int count = 0;
    [[nodiscard]] auto data() const -> const Point* { return ptr; }
    [[nodiscard]] auto size() const -> int { return count; }
    [[nodiscard]] auto empty() const -> bool { return count == 0; }
};

struct FakePlatform {
    int x = 0;
    int y = 0;
    unsigned from_death = 0;
    unsigned until_death = 0;
    const char* trigger_name = nullptr;
    bool active_when_on = true;
    unsigned crumble_frames = 0;
    unsigned crumble_respawn_frames = 0;
    FakePath flee_path;
    int flee_range = 0;
    int flee_frames = 0;
};

struct FakeLevel {
    int platform_count = 0;
    FakePlatform platforms[4];
    int trap_count = 0;
};

struct Hit {
    const FakeBody* body;
};
struct Below {
    int count = 0;
    Hit hits[2] = {};
};

using Set = PlatformSet<
    std::deque<FakeBody>,
    std::vector<std::unique_ptr<FakeSprite>>,
    FakeTrigger,
    Point,
    4>;

// Adds a platform of 32x16 centred on (x, y); returns its body.
auto add(Set& set, int x, int y) -> FakeBody& {
    return set.add(std::make_unique<FakeSprite>(), x, y, 32, 16);
}

auto always(unsigned, unsigned) -> bool {
    return true;
}
auto never_there(unsigned, unsigned) -> bool {
    return false;
}

FakeTrigger g_trigger;
auto the_trigger(const FakePlatform&) -> FakeTrigger& {
    return g_trigger;
}

const Position FAR_AWAY{500, 500, 8, 8};

}  // namespace

TEST_CASE(platform_set_add_links_the_body_to_its_picture) {
    Set set;
    FakeBody& body = add(set, 10, 20);
    CHECK(body.sprite != nullptr);
    CHECK_EQ(body.pos.x, bn::fixed(10));
    CHECK_EQ(body.pos.width, 32);
}

TEST_CASE(platform_set_hide_and_show_switch_body_and_picture_together) {
    Set set;
    FakeBody& body = add(set, 0, 0);
    set.hide(0);
    CHECK(!body.enabled);
    CHECK(!body.sprite->enabled);
    set.show(0);
    CHECK(body.enabled);
    CHECK(body.sprite->enabled);
}

TEST_CASE(platform_set_show_and_hide_pick_the_platform_by_index) {
    Set set;
    FakeBody& first = add(set, 0, 0);
    FakeBody& second = add(set, 100, 0);
    set.hide(1);
    CHECK(first.enabled);
    CHECK(!second.enabled);
}

TEST_CASE(platform_set_load_dynamic_routes_each_platform_to_its_helper) {
    g_trigger.on = false;
    Set set;
    FakeBody& plain = add(set, 0, 0);
    FakeBody& switched = add(set, 100, 0);
    FakeBody& crumbling = add(set, 200, 0);
    FakeLevel level;
    level.platform_count = 3;
    level.platforms[1].trigger_name = "t";
    level.platforms[2].crumble_frames = 2;
    set.load_dynamic(level, always, the_trigger);

    // The switched one follows its (off) trigger; the plain one is untouched.
    set.sync_switched(FAR_AWAY);
    CHECK(plain.enabled);
    CHECK(!switched.enabled);
    g_trigger.on = true;
    set.sync_switched(FAR_AWAY);
    CHECK(switched.enabled);

    // The crumbling one gives way after the duck stood on it.
    Below below;
    below.count = 1;
    below.hits[0] = {&crumbling};
    for (int i = 0; i < 4; ++i) {
        set.sync_crumbling(FAR_AWAY, [&] { return below; });
    }
    CHECK(!crumbling.enabled);
    set.reset_crumbling();
    CHECK(crumbling.enabled);
}

TEST_CASE(platform_set_load_dynamic_asks_for_the_trigger_of_switched_only) {
    Set set;
    add(set, 0, 0);
    add(set, 100, 0);
    FakeLevel level;
    level.platform_count = 2;
    level.platforms[1].x = 7;
    level.platforms[1].trigger_name = "t";
    int asked = 0;
    set.load_dynamic(level, always, [&](const FakePlatform& p) -> FakeTrigger& {
        ++asked;
        CHECK_EQ(p.x, 7);
        return g_trigger;
    });
    CHECK_EQ(asked, 1);
}

TEST_CASE(platform_set_load_dynamic_leaves_out_platforms_not_there_yet) {
    g_trigger.on = false;
    Set set;
    FakeBody& body = add(set, 0, 0);
    FakeLevel level;
    level.platform_count = 1;
    level.platforms[0].trigger_name = "t";
    level.platforms[0].from_death = 2;
    set.load_dynamic(level, never_there, the_trigger);
    // Not handed to DynamicPlatforms: nothing disables it behind our back.
    set.sync_switched(FAR_AWAY);
    CHECK(body.enabled);
}

TEST_CASE(platform_set_load_dynamic_starts_over_on_every_call) {
    g_trigger.on = false;
    Set set;
    FakeBody& body = add(set, 0, 0);
    FakeLevel level;
    level.platform_count = 1;
    level.platforms[0].trigger_name = "t";
    set.load_dynamic(level, always, the_trigger);
    set.load_dynamic(level, always, the_trigger);
    set.sync_switched(FAR_AWAY);
    CHECK(!body.enabled);
    // The platform is gone for this attempt: the earlier registration must not
    // bring it back.
    set.load_dynamic(level, never_there, the_trigger);
    body.enabled = true;
    set.sync_switched(FAR_AWAY);
    CHECK(body.enabled);
}

TEST_CASE(platform_set_crumbling_probe_is_only_asked_when_one_crumbles) {
    Set set;
    add(set, 0, 0);
    FakeLevel level;
    level.platform_count = 1;
    int probes = 0;
    set.load_dynamic(level, always, the_trigger);
    set.sync_crumbling(FAR_AWAY, [&] {
        ++probes;
        return Below{};
    });
    CHECK_EQ(probes, 0);

    level.platforms[0].crumble_frames = 3;
    set.load_dynamic(level, always, the_trigger);
    set.sync_crumbling(FAR_AWAY, [&] {
        ++probes;
        return Below{};
    });
    CHECK_EQ(probes, 1);
}

TEST_CASE(platform_set_crumbling_only_counts_the_body_the_duck_stands_on) {
    Set set;
    FakeBody& other = add(set, 0, 0);
    FakeBody& stood_on = add(set, 100, 0);
    FakeLevel level;
    level.platform_count = 2;
    level.platforms[0].crumble_frames = 1;
    level.platforms[1].crumble_frames = 1;
    set.load_dynamic(level, always, the_trigger);
    Below below;
    below.count = 1;
    below.hits[0] = {&stood_on};
    for (int i = 0; i < 4; ++i) {
        set.sync_crumbling(FAR_AWAY, [&] { return below; });
    }
    CHECK(!stood_on.enabled);
    CHECK(other.enabled);
}

TEST_CASE(platform_set_fleeing_platform_runs_from_the_duck_and_comes_back) {
    Set set;
    FakeBody& body = add(set, 100, 60);
    FakeLevel level;
    level.platform_count = 1;
    level.platforms[0].x = 100;
    level.platforms[0].y = 60;
    level.platforms[0].flee_path = {AWAY, 1};
    level.platforms[0].flee_range = 32;
    level.platforms[0].flee_frames = 8;
    set.load_dynamic(level, always, the_trigger);
    for (int i = 0; i < 12; ++i) {
        set.update_fleeing(70, 50);
    }
    CHECK_EQ(body.pos.x, bn::fixed(148));
    set.reset_fleeing();
    CHECK_EQ(body.pos.x, bn::fixed(100));
}

TEST_CASE(platform_set_a_fleeing_platform_not_there_does_not_run) {
    Set set;
    FakeBody& body = add(set, 100, 60);
    FakeLevel level;
    level.platform_count = 1;
    level.platforms[0].x = 100;
    level.platforms[0].y = 60;
    level.platforms[0].flee_path = {AWAY, 1};
    level.platforms[0].flee_range = 32;
    level.platforms[0].flee_frames = 8;
    set.load_dynamic(level, never_there, the_trigger);
    for (int i = 0; i < 12; ++i) {
        set.update_fleeing(70, 50);
    }
    CHECK_EQ(body.pos.x, bn::fixed(100));
}

TEST_CASE(platform_set_clear_steps_leave_the_set_empty) {
    g_trigger.on = false;
    Set set;
    add(set, 0, 0);
    FakeLevel level;
    level.platform_count = 1;
    level.platforms[0].trigger_name = "t";
    set.load_dynamic(level, always, the_trigger);
    set.clear_dynamic();
    set.clear_fleeing();
    set.clear_platforms();
    // A new level starts at index 0 again and the old switch is gone.
    FakeBody& body = add(set, 5, 6);
    set.sync_switched(FAR_AWAY);
    CHECK(body.enabled);
    set.hide(0);
    CHECK(!body.enabled);
}

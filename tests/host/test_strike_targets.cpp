#include "test_framework.h"

#include "strike_targets.h"

// Bodies a pogo strike can hurt (strike_targets.h): the pogo finds a body in
// its box and asks the registry who it belongs to.

namespace {

struct Target : Strikeable {
    int struck = 0;
    void on_struck() override { ++struck; }
};

}  // namespace

TEST_CASE(strike_targets_a_registered_body_is_struck) {
    StrikeTargets targets;
    Target boss;
    int body = 0;
    targets.add(&body, boss);
    CHECK(targets.strike(&body));
    CHECK_EQ(boss.struck, 1);
}

TEST_CASE(strike_targets_other_bodies_are_not_targets) {
    StrikeTargets targets;
    Target boss;
    int body = 0;
    int trap = 0;
    targets.add(&body, boss);
    CHECK(!targets.strike(&trap));
    CHECK_EQ(boss.struck, 0);
}

TEST_CASE(strike_targets_removed_bodies_are_gone) {
    StrikeTargets targets;
    Target boss;
    int body = 0;
    targets.add(&body, boss);
    targets.remove(&body);
    CHECK(!targets.strike(&body));
}

TEST_CASE(strike_targets_hold_several_bodies) {
    StrikeTargets targets;
    Target a;
    Target b;
    int body_a = 0;
    int body_b = 0;
    targets.add(&body_a, a);
    targets.add(&body_b, b);
    targets.strike(&body_b);
    CHECK_EQ(a.struck, 0);
    CHECK_EQ(b.struck, 1);
    targets.remove(&body_a);
    CHECK(targets.strike(&body_b));
}

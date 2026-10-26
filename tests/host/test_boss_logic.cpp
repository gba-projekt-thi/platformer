#include "test_framework.h"

#include "boss_logic.h"
#include "cfg_boss.h"

// A boss fight (boss_logic.h): pogo strikes take its hits one by one, each
// starting the next phase; a short invulnerability keeps one strike from
// counting twice.

namespace {

void wait_out(boss::Fight& fight) {
    for (int i = 0; i < Cfg::Boss::INVULNERABLE_FRAMES; ++i) {
        fight.tick();
    }
}

}  // namespace

TEST_CASE(boss_starts_in_phase_one_with_all_its_hits) {
    const boss::Fight fight(3);
    CHECK_EQ(fight.phase(), 1);
    CHECK_EQ(fight.hits_left(), 3);
    CHECK(!fight.defeated());
    CHECK(!fight.invulnerable());
}

TEST_CASE(boss_a_strike_hurts_and_starts_the_next_phase) {
    boss::Fight fight(3);
    CHECK(fight.strike() == boss::Hit::Hurt);
    CHECK_EQ(fight.hits_left(), 2);
    CHECK_EQ(fight.phase(), 2);
    CHECK(fight.invulnerable());
}

TEST_CASE(boss_strikes_while_invulnerable_do_not_count) {
    boss::Fight fight(3);
    fight.strike();
    CHECK(fight.strike() == boss::Hit::Ignored);
    CHECK_EQ(fight.hits_left(), 2);
    for (int i = 0; i < Cfg::Boss::INVULNERABLE_FRAMES - 1; ++i) {
        fight.tick();
    }
    CHECK(fight.invulnerable());
    fight.tick();
    CHECK(!fight.invulnerable());
    CHECK(fight.strike() == boss::Hit::Hurt);
}

TEST_CASE(boss_the_last_hit_defeats_it) {
    boss::Fight fight(2);
    fight.strike();
    wait_out(fight);
    CHECK(fight.strike() == boss::Hit::Defeated);
    CHECK(fight.defeated());
    CHECK_EQ(fight.hits_left(), 0);
    // Nothing happens to a defeated boss.
    wait_out(fight);
    CHECK(fight.strike() == boss::Hit::Ignored);
}

TEST_CASE(boss_reset_brings_back_the_full_fight) {
    boss::Fight fight(3);
    fight.strike();
    fight.reset();
    CHECK_EQ(fight.hits_left(), 3);
    CHECK_EQ(fight.phase(), 1);
    CHECK(!fight.invulnerable());
}

TEST_CASE(boss_blinks_while_invulnerable) {
    boss::Fight fight(3);
    CHECK(fight.visible());
    fight.strike();
    bool hidden = false;
    for (int i = 0; i < Cfg::Boss::INVULNERABLE_FRAMES; ++i) {
        hidden |= !fight.visible();
        fight.tick();
    }
    CHECK(hidden);
    CHECK(fight.visible());
}

TEST_CASE(boss_each_phase_is_faster) {
    // Frames per path segment shrink phase by phase, never to zero.
    const unsigned base = 60;
    unsigned previous = boss::phase_frames(base, 1);
    CHECK_EQ(previous, base);
    for (int phase = 2; phase <= 5; ++phase) {
        const unsigned frames = boss::phase_frames(base, phase);
        CHECK(frames < previous);
        CHECK(frames > 0);
        previous = frames;
    }
    CHECK_EQ(boss::phase_frames(1, 5), 1u);
}

TEST_CASE(boss_constants_are_sane) {
    CHECK(Cfg::Boss::INVULNERABLE_FRAMES > Cfg::Boss::BLINK_FRAMES);
    CHECK(Cfg::Boss::BLINK_FRAMES > 0);
    CHECK(Cfg::Boss::MAX_PHASES >= 5);
}

#include "test_framework.h"

#include "respawn_logic.h"

using respawn::Cause;
using respawn::When;

TEST_CASE(respawn_a_death_waits_for_the_freeze) {
    // The trap that killed the duck stays where it is while the world is
    // frozen; the next attempt is set up when the freeze ends.
    CHECK(respawn::when(Cause::Death, true) == When::AfterFreeze);
}

TEST_CASE(respawn_a_death_without_a_freeze_is_set_up_at_once) {
    CHECK(respawn::when(Cause::Death, false) == When::Now);
}

TEST_CASE(respawn_restart_level_sets_up_the_next_attempt_at_once) {
    // The duck is already at the spawn: the next attempt's traps and
    // platforms must be there before the world moves again, or a trap of the
    // old layout kills it a second time (two deaths, a skipped layout).
    CHECK(respawn::when(Cause::Restart, false) == When::Now);
    CHECK(respawn::when(Cause::Restart, true) == When::Now);
}

// --- Pending: a respawn that waits for the end of the freeze ----------------

using respawn::Pending;

TEST_CASE(respawn_pending_a_death_in_a_freeze_runs_when_the_freeze_ends) {
    Pending pending;
    CHECK(!pending.request(Cause::Death, true));
    CHECK(pending.freeze_ended());
}

TEST_CASE(respawn_pending_runs_exactly_once) {
    Pending pending;
    pending.request(Cause::Death, true);
    CHECK(pending.freeze_ended());
    CHECK(!pending.freeze_ended());
}

TEST_CASE(respawn_pending_a_freeze_that_ends_alone_resets_nothing) {
    Pending pending;
    CHECK(!pending.freeze_ended());
}

TEST_CASE(respawn_pending_a_death_without_a_freeze_runs_at_once) {
    Pending pending;
    CHECK(pending.request(Cause::Death, false));
    CHECK(!pending.freeze_ended());
}

TEST_CASE(respawn_pending_restart_runs_at_once_even_in_a_freeze) {
    Pending pending;
    CHECK(pending.request(Cause::Restart, true));
    CHECK(!pending.freeze_ended());
}

TEST_CASE(respawn_pending_cancel_drops_a_waiting_respawn) {
    // Level load / unload and Restart Level end the freeze that was running.
    Pending pending;
    pending.request(Cause::Death, true);
    pending.cancel();
    CHECK(!pending.freeze_ended());
}

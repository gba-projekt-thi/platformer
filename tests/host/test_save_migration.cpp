#include "test_framework.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game_state.h"
#include "game_state_migration.h"
#include "save_header.h"
#include "save_migration.h"

using engine::save::restore_state;
using engine::save::RestoreResult;
using engine::save::SAVE_VERSION;

namespace {

// A toy state with three "schema versions": v1 = {a, b}, v2 adds c, v3 adds
// the byte flag that fits into tail padding (same sizeof as v2).
struct Toy {
    int32_t a = 1;
    int32_t b = 2;
    int32_t c = 3;
    uint8_t flag = 7;  // added in v3, lives partly in padding
};

constexpr int TOY_V1 = static_cast<int>(offsetof(Toy, c));
constexpr int TOY_V2 = static_cast<int>(offsetof(Toy, flag));

auto toy_valid(uint16_t version) -> int {
    if (version == 0) {
        return 0;
    }
    if (version < 2) {
        return TOY_V1;
    }
    if (version < 3) {
        return TOY_V2;
    }
    return static_cast<int>(sizeof(Toy));
}

}  // namespace

TEST_CASE(migration_exact_payload_roundtrips) {
    Toy saved;
    saved.a = 10;
    saved.flag = 99;
    uint8_t bytes[sizeof(Toy)];
    std::memcpy(bytes, &saved, sizeof(Toy));

    Toy out;
    CHECK(
        restore_state(bytes, sizeof(Toy), 3, &toy_valid, out) ==
        RestoreResult::Exact);
    CHECK_EQ(out.a, 10);
    CHECK_EQ(out.flag, 99);
}

TEST_CASE(migration_older_payload_keeps_prefix_and_defaults_the_rest) {
    Toy v1;
    v1.a = 40;
    v1.b = 50;
    v1.c = 123456;  // not part of schema 1: must be ignored
    uint8_t bytes[sizeof(Toy)];
    std::memcpy(bytes, &v1, sizeof(Toy));

    Toy out;
    out.a = out.b = out.c = -1;
    CHECK(
        restore_state(bytes, TOY_V1, 1, &toy_valid, out) ==
        RestoreResult::Migrated);
    CHECK_EQ(out.a, 40);
    CHECK_EQ(out.b, 50);
    CHECK_EQ(out.c, 3);     // default
    CHECK_EQ(out.flag, 7);  // default
}

TEST_CASE(migration_never_copies_tail_padding_garbage_into_a_new_field) {
    // v2 payload has the same size as v3, but the byte where `flag` lives is
    // old padding (here: garbage). Version-keyed prefix -> default, not 0xAB.
    uint8_t bytes[sizeof(Toy)];
    std::memset(bytes, 0xAB, sizeof(bytes));
    Toy base;
    std::memcpy(bytes, &base, TOY_V2);  // meaningful part only

    Toy out;
    CHECK_EQ(static_cast<int>(sizeof(Toy)), static_cast<int>(sizeof(bytes)));
    CHECK(
        restore_state(bytes, sizeof(Toy), 2, &toy_valid, out) ==
        RestoreResult::Migrated);
    CHECK_EQ(out.flag, 7);
}

TEST_CASE(migration_unknown_old_version_is_incompatible_and_leaves_out_alone) {
    uint8_t bytes[sizeof(Toy)] = {};
    Toy out;
    out.a = 77;
    CHECK(
        restore_state(bytes, sizeof(Toy), 0, &toy_valid, out) ==
        RestoreResult::Incompatible);
    CHECK_EQ(out.a, 77);
}

TEST_CASE(migration_empty_payload_is_incompatible) {
    uint8_t bytes[1] = {};
    Toy out;
    CHECK(
        restore_state(bytes, 0, 3, &toy_valid, out) ==
        RestoreResult::Incompatible);
}

TEST_CASE(migration_payload_longer_than_the_state_is_clamped) {
    uint8_t bytes[sizeof(Toy) + 16] = {};
    Toy out;
    CHECK(
        restore_state(bytes, sizeof(bytes), 3, &toy_valid, out) ==
        RestoreResult::Migrated);
}

TEST_CASE(migration_without_a_table_requires_the_exact_size) {
    Toy saved;
    saved.a = 5;
    uint8_t bytes[sizeof(Toy)];
    std::memcpy(bytes, &saved, sizeof(Toy));

    Toy out;
    CHECK(
        restore_state(bytes, sizeof(Toy), 3, nullptr, out) ==
        RestoreResult::Exact);
    CHECK_EQ(out.a, 5);
    out.a = 0;
    CHECK(
        restore_state(bytes, TOY_V2, 3, nullptr, out) ==
        RestoreResult::Incompatible);
    CHECK_EQ(out.a, 0);  // untouched
}

// --- the real GameState
// -------------------------------------------------------

TEST_CASE(game_state_current_version_maps_to_the_whole_struct) {
    CHECK_EQ(
        game_state_migration::valid_bytes(SAVE_VERSION),
        static_cast<int>(sizeof(GameState)));
}

TEST_CASE(game_state_versions_below_the_minimum_are_not_migratable) {
    for (uint16_t v = 0; v < game_state_migration::MIN_VERSION; ++v) {
        CHECK_EQ(game_state_migration::valid_bytes(v), 0);
    }
    CHECK(game_state_migration::MIN_VERSION <= SAVE_VERSION);
}

TEST_CASE(game_state_valid_bytes_grow_with_the_version_and_fit_the_struct) {
    int previous = 0;
    for (uint16_t v = 0; v <= SAVE_VERSION; ++v) {
        const int bytes = game_state_migration::valid_bytes(v);
        CHECK(bytes >= previous);
        CHECK(bytes <= static_cast<int>(sizeof(GameState)));
        previous = bytes;
    }
}

TEST_CASE(game_state_layout_lock) {
    // If this fails you changed GameState. Fields may only be APPENDED: bump
    // SAVE_VERSION, add the previous version's line to
    // game_state_migration.h::valid_bytes() (offsetof the new field), then
    // update the numbers below.
    CHECK_EQ(static_cast<int>(sizeof(GameState)), 120);
    CHECK_EQ(static_cast<int>(offsetof(GameState, level)), 0);
    CHECK_EQ(static_cast<int>(offsetof(GameState, furthest_level)), 10);
    CHECK_EQ(static_cast<int>(offsetof(GameState, best_time_frames)), 16);
    CHECK_EQ(static_cast<int>(offsetof(GameState, no_death_clears)), 112);
    CHECK_EQ(static_cast<int>(offsetof(GameState, assist_mode_enabled)), 118);
    CHECK_EQ(SAVE_VERSION, 5);
}

TEST_CASE(game_state_current_payload_roundtrips_exactly) {
    GameState saved{};
    saved.level = 9;
    saved.deaths = 321;
    saved.best_time_frames[4] = 4242;
    saved.hard_mode_unlocked = true;
    saved.assist_mode_enabled = true;
    uint8_t bytes[sizeof(GameState)];
    std::memcpy(bytes, &saved, sizeof(GameState));

    GameState out{};
    CHECK(
        restore_state(
            bytes, sizeof(GameState), SAVE_VERSION,
            &game_state_migration::valid_bytes, out) == RestoreResult::Exact);
    CHECK_EQ(out.level, 9);
    CHECK_EQ(out.deaths, 321u);
    CHECK_EQ(out.best_time_frames[4], 4242u);
    CHECK(out.hard_mode_unlocked);
    CHECK(out.assist_mode_enabled);
}

TEST_CASE(game_state_schema_4_payload_is_still_rejected) {
    uint8_t bytes[sizeof(GameState)] = {};
    GameState out{};
    CHECK(
        restore_state(
            bytes, sizeof(GameState), 4, &game_state_migration::valid_bytes,
            out) == RestoreResult::Incompatible);
}

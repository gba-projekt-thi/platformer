#include "test_framework.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game_state.h"
#include "game_state_migration.h"
#include "save_header.h"
#include "save_migration.h"
#include "world_index.h"

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
    CHECK_EQ(static_cast<int>(sizeof(GameState)), 156);
    CHECK_EQ(static_cast<int>(offsetof(GameState, level)), 0);
    CHECK_EQ(static_cast<int>(offsetof(GameState, furthest_level)), 10);
    CHECK_EQ(static_cast<int>(offsetof(GameState, best_time_frames)), 16);
    CHECK_EQ(static_cast<int>(offsetof(GameState, no_death_clears)), 144);
    CHECK_EQ(static_cast<int>(offsetof(GameState, assist_mode_enabled)), 150);
    CHECK_EQ(static_cast<int>(offsetof(GameState, entry_level)), 152);
    CHECK_EQ(static_cast<int>(offsetof(GameState, entry_deaths)), 154);
    CHECK_EQ(SAVE_VERSION, 10);
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

// ---- levels inserted into the roster
// -----------------------------------------
//
// Schema 6 inserted LEVEL_WORLD5_CLIMB at level index 17 (before World 5's
// boss), schema 7 LEVEL_WORLD4_POGO at 10 (World 4's first level), schema 8
// LEVEL_WORLD3_TRICK at 9 (before World 3's boss), schema 9 one level before
// the bosses of World 1, 2 and 5 (at 3, 7 and 22): everything indexed by
// level from there on moves up by one.

namespace {

auto numbered_save() -> GameState {
    GameState state{};
    for (int i = 0; i < 19; ++i) {
        state.best_time_frames[i] = 1000u + unsigned(i);
    }
    state.no_death_clears = (1u << 3) | (1u << 16) | (1u << 17) | (1u << 18);
    return state;
}

}  // namespace

TEST_CASE(game_state_inserting_a_level_moves_the_records_behind_it) {
    GameState state = numbered_save();
    game_state_migration::insert_level(state, 17);
    CHECK_EQ(state.best_time_frames[16], 1016u);  // before it: stays
    CHECK_EQ(state.best_time_frames[17], 0u);     // the new level: no record
    CHECK_EQ(state.best_time_frames[18], 1017u);
    CHECK_EQ(state.best_time_frames[19], 1018u);
    CHECK_EQ(state.best_time_frames[3], 1003u);
}

TEST_CASE(game_state_inserting_a_level_moves_the_no_death_badges) {
    GameState state = numbered_save();
    game_state_migration::insert_level(state, 17);
    CHECK_EQ(
        state.no_death_clears,
        (1u << 3) | (1u << 16) | (1u << 18) | (1u << 19));
}

TEST_CASE(game_state_inserting_a_level_moves_progress_at_or_after_it) {
    GameState at_boss{};
    at_boss.level = 17;
    at_boss.furthest_level = 18;
    game_state_migration::insert_level(at_boss, 17);
    CHECK_EQ(int(at_boss.level), 18);
    CHECK_EQ(int(at_boss.furthest_level), 19);

    // Before it: the new level simply comes next.
    GameState before{};
    before.level = 16;
    before.furthest_level = 16;
    game_state_migration::insert_level(before, 17);
    CHECK_EQ(int(before.level), 16);
    CHECK_EQ(int(before.furthest_level), 16);
}

TEST_CASE(game_state_v8_saves_get_the_world_1_2_and_5_levels) {
    // Schema 9: index 3 (World 1), then 7 (World 2) and 22 (World 5) of the
    // roster as it grows.
    GameState state = numbered_save();
    state.level = 20;  // World 5's boss in schema 8
    game_state_migration::insert_levels_since(state, 8);
    CHECK_EQ(int(state.level), 23);
    CHECK_EQ(state.best_time_frames[2], 1002u);
    CHECK_EQ(state.best_time_frames[3], 0u);     // World 1's new level
    CHECK_EQ(state.best_time_frames[4], 1003u);  // World 1's boss
    CHECK_EQ(state.best_time_frames[7], 0u);     // World 2's new level
    CHECK_EQ(state.best_time_frames[8], 1006u);  // World 2's boss
    CHECK_EQ(state.best_time_frames[20], 1018u);
    CHECK_EQ(state.best_time_frames[22], 0u);  // World 5's new level
    CHECK_EQ(
        state.no_death_clears,
        (1u << 4) | (1u << 18) | (1u << 19) | (1u << 20));
}

TEST_CASE(game_state_v7_saves_get_the_world_3_level) {
    GameState state = numbered_save();
    state.level = 9;  // World 3's boss in schema 7
    game_state_migration::insert_levels_since(state, 7);
    CHECK_EQ(int(state.level), 12);  // + World 3's, then World 1's and 2's
    CHECK_EQ(state.best_time_frames[10], 1008u);
    CHECK_EQ(state.best_time_frames[11], 0u);
    CHECK_EQ(state.best_time_frames[12], 1009u);
}

TEST_CASE(game_state_v6_saves_get_the_world_4_and_3_levels) {
    GameState state = numbered_save();
    state.level = 12;  // World 4's boss in schema 6
    game_state_migration::insert_levels_since(state, 6);
    CHECK_EQ(int(state.level), 16);
    CHECK_EQ(state.best_time_frames[11], 0u);     // World 3's new level
    CHECK_EQ(state.best_time_frames[12], 1009u);  // World 3's boss
    CHECK_EQ(state.best_time_frames[13], 0u);     // World 4's new level
    CHECK_EQ(state.best_time_frames[14], 1010u);
}

TEST_CASE(game_state_v5_saves_get_every_level_in_order) {
    // Schema 6 (index 17 of the v5 roster), 7 (10), then 8 (9).
    GameState state = numbered_save();
    state.level = 17;  // World 5's boss in schema 5
    game_state_migration::insert_levels_since(state, 5);
    CHECK_EQ(int(state.level), 23);
    CHECK_EQ(state.best_time_frames[11], 0u);  // World 3's new level
    CHECK_EQ(state.best_time_frames[13], 0u);  // World 4's new level
    CHECK_EQ(state.best_time_frames[14], 1010u);
    CHECK_EQ(state.best_time_frames[20], 1016u);  // LEVERS
    CHECK_EQ(state.best_time_frames[21], 0u);     // World 5's CLIMB
    CHECK_EQ(state.best_time_frames[22], 0u);     // World 5's new level
    CHECK_EQ(state.best_time_frames[23], 1017u);  // the boss
}

TEST_CASE(game_state_current_saves_are_not_upgraded) {
    GameState state = numbered_save();
    state.level = 18;
    game_state_migration::insert_levels_since(state, SAVE_VERSION);
    CHECK_EQ(state.best_time_frames[17], 1017u);
    CHECK_EQ(int(state.level), 18);
}

TEST_CASE(game_state_roster_still_fits_the_save) {
    CHECK(WorldIndex::LEVEL_COUNT <= GameState::MAX_LEVELS);
}

// ---- schema 9: room for 32 levels
// ----------------------------------------------
//
// Schema 8 held 24 best times; what came after them (no_death_clears and the
// three mode flags) now sits behind 32. upgrade() moves it there.

namespace {

// A schema 8 payload: 24 best times, then no_death_clears and the flags.
constexpr int V8_SIZE = 120;

auto v8_payload() -> std::array<uint8_t, V8_SIZE> {
    std::array<uint8_t, V8_SIZE> bytes{};
    const int16_t level = 20;
    const uint16_t deaths = 77;
    std::memcpy(&bytes[0], &level, 2);
    std::memcpy(&bytes[2], &deaths, 2);
    for (int i = 0; i < 24; ++i) {
        const uint32_t frames = 1000u + unsigned(i);
        std::memcpy(&bytes[16 + 4 * i], &frames, 4);
    }
    const uint32_t clears = (1u << 3) | (1u << 23);
    std::memcpy(&bytes[112], &clears, 4);
    bytes[116] = 1;  // hard_mode_unlocked
    bytes[117] = 0;  // hard_mode_enabled
    bytes[118] = 1;  // assist_mode_enabled
    return bytes;
}

auto load_v8() -> GameState {
    const auto bytes = v8_payload();
    GameState out{};
    restore_state(
        bytes.data(), V8_SIZE, 8, &game_state_migration::valid_bytes, out);
    game_state_migration::upgrade(out, 8);
    return out;
}

}  // namespace

TEST_CASE(game_state_v8_payload_is_read_in_full) {
    CHECK_EQ(game_state_migration::valid_bytes(8), V8_SIZE);
}

TEST_CASE(game_state_v8_flags_and_badges_move_behind_32_levels) {
    const GameState state = load_v8();
    CHECK(state.hard_mode_unlocked);
    CHECK(!state.hard_mode_enabled);
    CHECK(state.assist_mode_enabled);
    // Badges 3 and 23 of schema 8, after the three new levels (3, 7, 22).
    CHECK_EQ(state.no_death_clears, (1u << 4) | (1u << 26));
}

TEST_CASE(game_state_v8_best_times_and_progress_survive) {
    const GameState state = load_v8();
    CHECK_EQ(state.deaths, 77u);
    CHECK_EQ(int(state.level), 23);
    CHECK_EQ(state.best_time_frames[0], 1000u);
    CHECK_EQ(state.best_time_frames[3], 0u);
    CHECK_EQ(state.best_time_frames[4], 1003u);
    CHECK_EQ(state.best_time_frames[26], 1023u);  // schema 8's last level
    for (int i = 27; i < GameState::MAX_LEVELS; ++i) {
        CHECK_EQ(state.best_time_frames[i], 0u);
    }
}

// ---- schema 10: the level the duck is in and its deaths at entry
// --------------------------------------------------------------

TEST_CASE(game_state_v9_payload_keeps_everything_before_entry_level) {
    CHECK_EQ(
        game_state_migration::valid_bytes(9),
        static_cast<int>(offsetof(GameState, entry_level)));
}

TEST_CASE(game_state_v9_slot_has_no_level_entered) {
    GameState saved{};
    saved.level = 5;
    saved.deaths = 9;
    uint8_t bytes[sizeof(GameState)];
    std::memset(bytes, 0xAB, sizeof(bytes));
    std::memcpy(bytes, &saved, offsetof(GameState, entry_level));

    GameState out{};
    restore_state(
        bytes, sizeof(GameState), 9, &game_state_migration::valid_bytes, out);
    game_state_migration::upgrade(out, 9);
    CHECK_EQ(int(out.level), 5);
    CHECK_EQ(int(out.entry_level), -1);
    CHECK_EQ(int(out.entry_deaths), 0);
}

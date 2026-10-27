#include "test_framework.h"

#include <vector>

#include "game_state.h"
#include "replay.h"
#include "world_index.h"

// Recording and playing back the duck's buttons (replay.h), one InputState
// per movement step, stored as runs of equal buttons.

namespace {

auto buttons(
    bool left,
    bool right,
    bool a_pressed,
    bool a_held,
    bool b,
    bool r = false) -> InputState {
    InputState input;
    input.left_held = left;
    input.right_held = right;
    input.a_pressed = a_pressed;
    input.a_held = a_held;
    input.b_pressed = b;
    input.r_pressed = r;
    return input;
}

auto same(const InputState& a, const InputState& b) -> bool {
    return a.left_held == b.left_held && a.right_held == b.right_held &&
           a.a_pressed == b.a_pressed && a.a_held == b.a_held &&
           a.b_pressed == b.b_pressed && a.r_pressed == b.r_pressed;
}

struct Sink {
    std::vector<replay::Run> runs;
    void operator()(const replay::Run& run) { runs.push_back(run); }
};

}  // namespace

TEST_CASE(replay_every_button_combination_survives_encoding) {
    for (int bits = 0; bits < 64; ++bits) {
        CHECK_EQ(int(replay::encode(replay::decode(uint8_t(bits)))), bits);
    }
    const InputState pogo = buttons(false, false, false, true, false, true);
    CHECK(same(replay::decode(replay::encode(pogo)), pogo));
    const InputState jump = buttons(false, true, true, true, false);
    CHECK(same(replay::decode(replay::encode(jump)), jump));
}

TEST_CASE(replay_recorder_packs_equal_steps_into_runs) {
    Sink sink;
    replay::Recorder recorder;
    const InputState idle;
    const InputState run_right = buttons(false, true, false, false, false);
    for (int i = 0; i < 3; ++i) {
        recorder.step(idle, sink);
    }
    for (int i = 0; i < 5; ++i) {
        recorder.step(run_right, sink);
    }
    recorder.flush(sink);
    CHECK_EQ(int(sink.runs.size()), 2);
    CHECK_EQ(int(sink.runs[0].count), 3);
    CHECK_EQ(int(sink.runs[1].buttons), int(replay::encode(run_right)));
    CHECK_EQ(int(sink.runs[1].count), 5);
}

TEST_CASE(replay_recorder_splits_very_long_runs) {
    Sink sink;
    replay::Recorder recorder;
    for (int i = 0; i < replay::MAX_RUN + 10; ++i) {
        recorder.step(InputState{}, sink);
    }
    recorder.flush(sink);
    CHECK_EQ(int(sink.runs.size()), 2);
    CHECK_EQ(int(sink.runs[0].count), replay::MAX_RUN);
    CHECK_EQ(int(sink.runs[1].count), 10);
}

TEST_CASE(replay_flush_without_steps_writes_nothing) {
    Sink sink;
    replay::Recorder recorder;
    recorder.flush(sink);
    CHECK(sink.runs.empty());
}

TEST_CASE(replay_player_gives_back_every_recorded_step) {
    Sink sink;
    replay::Recorder recorder;
    std::vector<InputState> steps;
    for (int i = 0; i < 40; ++i) {
        steps.push_back(
            buttons(i % 7 == 0, i % 3 == 0, i == 5, i > 4 && i < 9, i == 20));
        recorder.step(steps.back(), sink);
    }
    recorder.flush(sink);

    replay::Player player(sink.runs.data(), int(sink.runs.size()));
    for (const InputState& expected : steps) {
        CHECK(!player.finished());
        CHECK(same(player.next(), expected));
    }
    CHECK(player.finished());
    CHECK_EQ(player.steps_played(), 40);
}

TEST_CASE(replay_player_after_the_end_holds_no_button) {
    const replay::Run runs[] = {
        {replay::encode(buttons(false, true, false, false, false)), 1}};
    replay::Player player(runs, 1);
    player.next();
    CHECK(same(player.next(), InputState{}));
    CHECK(player.finished());
}

TEST_CASE(replay_a_run_packs_into_16_bits) {
    for (int buttons = 0; buttons < 64; ++buttons) {
        for (int count : {1, 2, 100, replay::MAX_RUN}) {
            const replay::Run run{uint8_t(buttons), uint16_t(count)};
            const replay::Run back = replay::unpack(replay::pack(run));
            CHECK_EQ(int(back.buttons), buttons);
            CHECK_EQ(int(back.count), count);
        }
    }
}

TEST_CASE(replay_runs_fit_the_packed_count) {
    CHECK(replay::MAX_RUN <= 0x3FF);  // 10 bits next to 6 button bits
}

TEST_CASE(replay_level_blocks_fill_the_sram_behind_the_saves) {
    // GameState saves start at 0; the recording area sits behind them and
    // ends at the 32 KB SRAM boundary.
    CHECK(replay::SRAM_OFFSET >= 0x2000);
    CHECK_EQ(
        replay::SRAM_OFFSET + replay::LEVEL_BLOCKS * replay::BLOCK_SIZE,
        0x8000);
    CHECK(replay::LEVEL_BLOCKS >= WorldIndex::LEVEL_COUNT);
    // Every level a save can hold has a block, not only today's roster.
    CHECK(replay::LEVEL_BLOCKS >= GameState::MAX_LEVELS);
    CHECK_EQ(replay::block_offset(0), replay::SRAM_OFFSET);
    CHECK_EQ(
        replay::block_offset(2), replay::SRAM_OFFSET + 2 * replay::BLOCK_SIZE);
    CHECK(
        replay::MAX_BLOCK_RUNS * 2 + replay::HEADER_SIZE <= replay::BLOCK_SIZE);
}

TEST_CASE(replay_a_recording_from_the_level_start_begins_at_no_death) {
    replay::StartDeaths start;
    start.level_started(7);  // the run's death total when the level loads
    start.step(7);
    CHECK_EQ(start.deaths(), 0u);
    // Deaths along the way do not move the start.
    start.step(9);
    CHECK_EQ(start.deaths(), 0u);
}

TEST_CASE(replay_a_restart_begins_after_the_deaths_in_the_level) {
    // Two deaths, then Restart Level: it counts as the third, before the
    // first step of the new recording.
    replay::StartDeaths start;
    start.level_started(7);
    start.step(7);
    start.step(9);
    start.restarted();
    start.step(10);
    CHECK_EQ(start.deaths(), 3u);
    start.step(11);
    CHECK_EQ(start.deaths(), 3u);
}

TEST_CASE(replay_a_new_level_forgets_the_last_ones_deaths) {
    replay::StartDeaths start;
    start.level_started(0);
    start.restarted();
    start.step(4);
    start.level_started(4);
    start.step(4);
    CHECK_EQ(start.deaths(), 0u);
}

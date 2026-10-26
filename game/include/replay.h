#pragma once

#include <cstdint>

#include "locomotion.h"

// -----------------------------------------------------------------------------
// replay
//
// Recording and playing back the duck's buttons for the emulator tests
// (`make record` / `make replay`, see tools/replays.py). One InputState per
// movement step - not per frame, so pauses and the death freeze do not shift
// anything - stored as runs of equal buttons. Host-tested in
// tests/host/test_replay.cpp.
// -----------------------------------------------------------------------------
namespace replay {

// The longest run; a longer stretch of equal buttons becomes several runs.
// 11 bits, so a run packs into 16 bits with its 5 button bits (pack()).
inline constexpr int MAX_RUN = 0x7FF;

struct Run {
    uint8_t buttons;  // encode()
    uint16_t count;   // steps
};

inline auto pack(const Run& run) -> uint16_t {
    return uint16_t((run.buttons << 11) | run.count);
}

inline auto unpack(uint16_t packed) -> Run {
    return Run{uint8_t(packed >> 11), uint16_t(packed & MAX_RUN)};
}

// `make record` stores each level's last recording in SRAM behind the save
// slots (which take well under 1 KB from offset 0): one BLOCK_SIZE block per
// level index, a 4 byte header (MAGIC, flags, run count) and packed runs.
// tools/replays.py reads them from the .sav file.
inline constexpr int SRAM_OFFSET = 0x2000;
inline constexpr int BLOCK_SIZE = 0x400;
inline constexpr int LEVEL_BLOCKS = (0x8000 - SRAM_OFFSET) / BLOCK_SIZE;
inline constexpr int MAX_BLOCK_RUNS = (BLOCK_SIZE - 4) / 2;
inline constexpr uint8_t MAGIC = 0x52;           // 'R'
inline constexpr uint8_t FLAG_TRUNCATED = 0x01;  // more runs than fit

inline constexpr auto block_offset(int level_index) -> int {
    return SRAM_OFFSET + level_index * BLOCK_SIZE;
}

inline auto encode(const InputState& input) -> uint8_t {
    return uint8_t(
        (input.left_held ? 1 : 0) | (input.right_held ? 2 : 0) |
        (input.a_pressed ? 4 : 0) | (input.a_held ? 8 : 0) |
        (input.b_pressed ? 16 : 0));
}

inline auto decode(uint8_t buttons) -> InputState {
    InputState input;
    input.left_held = (buttons & 1) != 0;
    input.right_held = (buttons & 2) != 0;
    input.a_pressed = (buttons & 4) != 0;
    input.a_held = (buttons & 8) != 0;
    input.b_pressed = (buttons & 16) != 0;
    return input;
}

// Collects steps; every finished run goes to `sink(const Run&)`.
class Recorder {
   public:
    template <typename Sink>
    void step(const InputState& input, Sink& sink) {
        const uint8_t buttons = encode(input);
        if (_count > 0 && (buttons != _buttons || _count == MAX_RUN)) {
            flush(sink);
        }
        _buttons = buttons;
        ++_count;
    }

    // Hands over the run in progress (level end).
    template <typename Sink>
    void flush(Sink& sink) {
        if (_count > 0) {
            sink(Run{_buttons, uint16_t(_count)});
        }
        _count = 0;
    }

    // Drops the run in progress (level restart: the recording starts over).
    void clear() { _count = 0; }

   private:
    uint8_t _buttons = 0;
    int _count = 0;
};

// Plays recorded runs back step by step; after the last one no button is
// held.
class Player {
   public:
    Player(const Run* runs, int run_count)
        : _runs(runs), _run_count(run_count) {}

    auto next() -> InputState {
        while (_run < _run_count && _used == _runs[_run].count) {
            ++_run;
            _used = 0;
        }
        if (_run >= _run_count) {
            return InputState{};
        }
        ++_used;
        ++_played;
        return decode(_runs[_run].buttons);
    }

    [[nodiscard]] auto finished() const -> bool {
        int run = _run;
        int used = _used;
        while (run < _run_count && used == _runs[run].count) {
            ++run;
            used = 0;
        }
        return run >= _run_count;
    }

    [[nodiscard]] auto steps_played() const -> int { return _played; }

   private:
    const Run* _runs;
    int _run_count;
    int _run = 0;
    int _used = 0;
    int _played = 0;
};

}  // namespace replay

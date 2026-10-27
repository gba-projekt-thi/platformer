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
// 10 bits, so a run packs into 16 bits with its 6 button bits (pack()).
inline constexpr int MAX_RUN = 0x3FF;

struct Run {
    uint8_t buttons;  // encode()
    uint16_t count;   // steps
};

inline auto pack(const Run& run) -> uint16_t {
    return uint16_t((run.buttons << 10) | run.count);
}

inline auto unpack(uint16_t packed) -> Run {
    return Run{uint8_t(packed >> 10), uint16_t(packed & MAX_RUN)};
}

// `make record` stores each level's last recording in SRAM behind the save
// slots (which take well under 1 KB from offset 0): one BLOCK_SIZE block per
// level index, a HEADER_SIZE header (MAGIC, flags, run count, the deaths in
// the level the recording began with - StartDeaths) and packed runs.
// tools/replays.py reads them from the .sav file.
inline constexpr int SRAM_OFFSET = 0x2000;
inline constexpr int BLOCK_SIZE = 0x400;
inline constexpr int HEADER_SIZE = 6;
inline constexpr int LEVEL_BLOCKS = (0x8000 - SRAM_OFFSET) / BLOCK_SIZE;
inline constexpr int MAX_BLOCK_RUNS = (BLOCK_SIZE - HEADER_SIZE) / 2;
inline constexpr uint8_t MAGIC = 0x53;           // 'S' ('R': no deaths yet)
inline constexpr uint8_t FLAG_TRUNCATED = 0x01;  // more runs than fit

inline constexpr auto block_offset(int level_index) -> int {
    return SRAM_OFFSET + level_index * BLOCK_SIZE;
}

inline auto encode(const InputState& input) -> uint8_t {
    return uint8_t(
        (input.left_held ? 1 : 0) | (input.right_held ? 2 : 0) |
        (input.a_pressed ? 4 : 0) | (input.a_held ? 8 : 0) |
        (input.b_pressed ? 16 : 0) | (input.r_pressed ? 32 : 0));
}

inline auto decode(uint8_t buttons) -> InputState {
    InputState input;
    input.left_held = (buttons & 1) != 0;
    input.right_held = (buttons & 2) != 0;
    input.a_pressed = (buttons & 4) != 0;
    input.a_held = (buttons & 8) != 0;
    input.b_pressed = (buttons & 16) != 0;
    input.r_pressed = (buttons & 32) != 0;
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

// The deaths in the level a recording began with: the level changes with
// them (death windows, a door that moves), and Restart Level is one of them.
// Read at the first step after a (re)start, when the restart's death has
// been counted.
class StartDeaths {
   public:
    // `total`: the run's death total as the level loads.
    void level_started(unsigned total) {
        _entry = total;
        _deaths = 0;
        _pending = true;
    }

    void restarted() { _pending = true; }

    void step(unsigned total) {
        if (_pending) {
            _deaths = total >= _entry ? total - _entry : 0u;
            _pending = false;
        }
    }

    [[nodiscard]] auto deaths() const -> unsigned { return _deaths; }

   private:
    unsigned _entry = 0;
    unsigned _deaths = 0;
    bool _pending = false;
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

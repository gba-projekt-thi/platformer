#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// hint
//
// Pure logic of the one-time level hint (LevelData::intro_hint): when it is
// shown and how long it stays. Split out of LevelScene so it has no Butano
// dependency and can be unit-tested on the host (see
// tests/host/test_hint_logic.cpp).
//
// A hint is shown while its level has never been cleared (no personal best
// yet, GameState::best_time_frames[level] == 0), so it needs no save field of
// its own: it disappears for good with the first clear, and clearing is also
// what proves the player has understood it.
// -----------------------------------------------------------------------------

namespace hint {

// Longest hint text in characters. The variable-width 8x16 font is at most
// 8px per glyph, so 28 characters (224px) always fit the 240px screen.
// tools/Check_level_hints.py enforces this on the level data.
inline constexpr int MAX_CHARS = 28;

inline auto should_show(const char* text, uint32_t best_time_frames) -> bool {
    return text != nullptr && text[0] != '\0' && best_time_frames == 0;
}

// Frame countdown for a visible hint. Not started = not visible.
class Timer {
   public:
    explicit Timer(int frames) : _frames(frames) {}

    void start() { _remaining = _frames; }
    void dismiss() { _remaining = 0; }
    void tick() {
        if (_remaining > 0) {
            --_remaining;
        }
    }

    [[nodiscard]] auto visible() const -> bool { return _remaining > 0; }

   private:
    int _frames;
    int _remaining = 0;
};

}  // namespace hint

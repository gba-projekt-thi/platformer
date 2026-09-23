#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// frames_to_time
//
// Converts an absolute frame count into minutes/seconds/centiseconds.
// Only called when building level-select menu text (level best-time
// display), NOT in the per-frame HUD path - that stays on Timer::tick()'s
// lookup tables. Plain division here is fine since this isn't a hot path.
//
// Deliberately zero-dependency (only <cstdint>) so it can be unit-tested
// on the host without the ARM toolchain or Butano - see tests/host/.
// timer.h includes this header rather than redeclaring FrameTime/
// frames_to_time itself, so every existing call site keeps compiling
// unchanged.
// -----------------------------------------------------------------------------
struct FrameTime {
    uint16_t minutes;
    uint16_t seconds;
    uint16_t centis;
};

inline auto frames_to_time(uint32_t frames) -> FrameTime {
    // 60 frames == 1 second, matching Timer::tick()'s cadence.
    const uint32_t total_seconds = frames / 60;
    const uint32_t remainder_frames = frames % 60;

    FrameTime result;
    result.minutes = uint16_t(total_seconds / 60);
    result.seconds = uint16_t(total_seconds % 60);
    result.centis = uint16_t((remainder_frames * 100) / 60);
    return result;
}
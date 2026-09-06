#pragma once

#include <cstdint>

// Persistent save data.
//
// Uses fixed-width integer types to ensure:
// - predictable memory layout
// - smaller save size
// - consistent ABI behavior on ARM
struct GameState {
    int16_t level = 0;

    // unsigned: DeathCounter/Player::get_deaths() are unsigned, and
    // negative death counts are meaningless. int16_t previously allowed
    // silent narrowing into a negative value once a save accumulated more
    // than 32767 deaths.
    uint16_t deaths = 0;

    uint16_t centis = 0;
    uint16_t seconds = 0;
    uint16_t minutes = 0;

    // Future:
    // uint16_t collectables;
};
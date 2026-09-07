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
    uint16_t deaths = 0;  // unsigned: matches Player::set_deaths(unsigned int)
                          // and LevelManager's unsigned _last_death_ct — avoids
                          // silent narrowing to negative values.

    uint16_t centis = 0;
    uint16_t seconds = 0;
    uint16_t minutes = 0;
};
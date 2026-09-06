#pragma once

#include "level_common.h"

constexpr PlatformData level0_platforms[] = {

    {-96, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0},
    {-80, 60, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 1},
    {-48, 60, 16, 10, 0, 0, bn::sprite_items::platformslvl1, 2},
    {-16, 60, 16, 14, 0, 0, bn::sprite_items::platformslvl1, 3},

    {0, 40, 16, 8, 0, 0, bn::sprite_items::platformslvl1, 0}};

constexpr TriggerData level0_triggers[] = {{-50, 30, 30, 30}};

constexpr TrapData level0_traps[] = {
    {TrapType::MOVING, 10, 10, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, -0.1, 0.1, 1, NO_PATH_TRAP, 0},

    {TrapType::BASE, -20, -20, 32, 32, 0, 0, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::PATH, 0, 0, 32, 32, 0, 0, bn::sprite_items::bubbles, 10,
     TRAP_GRAPHICS_INDEXES_3, 0, 0, 0, 0, FIGURE_8_PATH, 15}};

const LevelData LEVEL_0 = {
    level0_platforms,
    sizeof(level0_platforms) / sizeof(level0_platforms[0]),

    level0_triggers,
    sizeof(level0_triggers) / sizeof(level0_triggers[0]),

    level0_traps,
    sizeof(level0_traps) / sizeof(level0_traps[0]),

    bn::regular_bg_items::level1,
    bn::music_items::level1,
    {-96, 0},
    {32, 0}};

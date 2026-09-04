#pragma once

#include "level_common.h"

constexpr PlatformData level1_platforms[] = {
    {-96, 60, bn::sprite_items::platformslvl1, 0},
    {-80, 60, bn::sprite_items::platformslvl1, 1},
    {-48, 60, bn::sprite_items::platformslvl1, 3},
    {-16, 60, bn::sprite_items::platformslvl1, 0},
    {-16, 44, bn::sprite_items::platformslvl1, 2},
    {32, 60, bn::sprite_items::platformslvl1, 1},
    {48, 60, bn::sprite_items::platformslvl1, 1},
    {64, 60, bn::sprite_items::platformslvl1, 0},
    {80, 60, bn::sprite_items::platformslvl1, 1},
    {96, 60, bn::sprite_items::platformslvl1, 0},
    {112, 60, bn::sprite_items::platformslvl1, 2}};

constexpr TriggerData level1_triggers[] = {{80, 30, 32, 30}};

constexpr TrapData level1_traps[] = {
    {TrapType::MOVING, 80, -32, 32, 32, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0.0, 4, 4, NO_PATH_TRAP, 0},

    {TrapType::BASE, -48, 8, 32, 32, bn::sprite_items::bubbles, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -64, 60, 16, 16, bn::sprite_items::connector16x16, 16,
     TRAP_GRAPHICS_INDEXES_8, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, -32, 60, 16, 16, bn::sprite_items::connector16x16, 16,
     TRAP_GRAPHICS_INDEXES_8, -1, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_1 = {
    level1_platforms,
    sizeof(level1_platforms) / sizeof(level1_platforms[0]),

    level1_triggers,
    sizeof(level1_triggers) / sizeof(level1_triggers[0]),

    level1_traps,
    sizeof(level1_traps) / sizeof(level1_traps[0]),

    bn::regular_bg_items::level1,
    bn::music_items::level1,
    {-96, 0},
    {112, 40}};

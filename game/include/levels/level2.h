#pragma once

#include "level_common.h"

constexpr PlatformData level2_platforms[] = {
    {-96, -16, bn::sprite_items::platformslvl2, 1},
    {-72, 40, bn::sprite_items::platformslvl2, 2},
    {-24, 40, bn::sprite_items::platformslvl2, 0},
    {24, 40, bn::sprite_items::platformslvl2, 0},
    {64, 24, bn::sprite_items::platformslvl2, 1},
    {96, 16, bn::sprite_items::platformslvl2, 1},
    {112, 16, bn::sprite_items::platformslvl2, 1}};

constexpr TriggerData level2_triggers[] = {{-48, 16, 32, 40}};

constexpr TrapData level2_traps[] = {
    {TrapType::BASE, -112, -16, 16, 16, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::MOVING, -48, 88, 32, 32, bn::sprite_items::can32x32, 26,
     TRAP_GRAPHICS_INDEXES_3, 0, 0.0, -3, 3, NO_PATH_TRAP, 0},

    {TrapType::BASE, 0, 40, 32, 16, bn::sprite_items::rustynails32x16, 26,
     TRAP_GRAPHICS_INDEXES_3, -1, 0, 0, 0, NO_PATH_TRAP, 0},

    {TrapType::BASE, 80, 24, 16, 16, bn::sprite_items::pipe16x16, 26,
     TRAP_GRAPHICS_INDEXES_0, -1, 0, 0, 0, NO_PATH_TRAP, 0}};

const LevelData LEVEL_2 = {
    level2_platforms,
    sizeof(level2_platforms) / sizeof(level2_platforms[0]),

    level2_triggers,
    sizeof(level2_triggers) / sizeof(level2_triggers[0]),

    level2_traps,
    sizeof(level2_traps) / sizeof(level2_traps[0]),

    bn::regular_bg_items::level2,
    bn::music_items::level2,
    {-96, -32},
    {104, 0}};

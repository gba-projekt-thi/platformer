#pragma once
#include "level_structure.h"

constexpr PlatformData level0_platforms[] =
    {{-96, 60, 0}, {-80, 60, 1}, {-48, 60, 2}, {-16, 60, 3}, {0, 40, 0}};

constexpr TriggerData level0_triggers[] = {{-50, 30, 30, 30}};

constexpr TrapData level0_traps[] = {
    {TrapType::MOVING, 10, 10, 32, 32, bn::sprite_items::bubbles, -0.1, 0.1, 1,
     5, 0},
    {TrapType::BASE, -20, -20, 32, 32, bn::sprite_items::bubbles, 0, 0, 0, 0,
     -1}};

const LevelData LEVEL_0 = {
    level0_platforms,
    5,
    level0_triggers,
    1,
    level0_traps,
    2,
    bn::sprite_items::platformslvl1,
    bn::regular_bg_items::level1,
    bn::music_items::level1,
    {-96, 0},
    {32, 0}};

constexpr PlatformData level1_platforms[] = {
    {-96, 60, 0}, {-80, 60, 1}, {-48, 60, 3}, {-16, 60, 0},
    {-16, 44, 2}, {32, 60, 1},  {48, 60, 1},  {64, 60, 0},
    {80, 60, 1},  {96, 60, 0},  {112, 60, 2}};

constexpr TriggerData level1_triggers[] = {{80, 30, 30, 30}};

constexpr TrapData level1_traps[] = {
    {TrapType::MOVING, 80, -64, 32, 32, bn::sprite_items::bubbles, 0.0, 3, 3, 5,
     0},
    {TrapType::BASE, -48, 8, 32, 32, bn::sprite_items::bubbles, 0, 0, 0, 0, -1},
    {TrapType::BASE, -64, 60, 16, 16, bn::sprite_items::connector16x16, 0, 0, 0,
     0, -1},
    {TrapType::BASE, -32, 60, 16, 16, bn::sprite_items::connector16x16, 0, 0, 0,
     0, -1}};

const LevelData LEVEL_1 = {
    level1_platforms,
    11,
    level1_triggers,
    1,
    level1_traps,
    4,
    bn::sprite_items::platformslvl1,
    bn::regular_bg_items::level1,
    bn::music_items::level1,
    {-96, 0},
    {112, 40}};

constexpr PlatformData level2_platforms[] = {
    {-96, -16, 1}, {-72, 40, 2},
    {-24, 40, 0},  /*{-8, 40, 0}, {8, 40, 0},*/ {24, 40, 0},
    {64, 24, 1},   {96, 16, 1},
    {112, 16, 1}};

constexpr TrapData level2_traps[] = {
    {TrapType::BASE, -112, -16, 16, 16, bn::sprite_items::pipe16x16, 0, 0, 0, 0,
     -1},
    {TrapType::MOVING, -48, 88, 32, 32, bn::sprite_items::can32x32, 0.0, -3, 3,
     0, 0},
    {TrapType::BASE, 0, 40, 32, 16, bn::sprite_items::rustynails32x16, 0, 0, 0,
     0, -1},
    {TrapType::BASE, 80, 24, 16, 16, bn::sprite_items::pipe16x16, 0, 0, 0, 0,
     -1}};

constexpr TriggerData level2_triggers[] = {{-48, 16, 32, 40}};

const LevelData LEVEL_2 = {
    level2_platforms,
    7,

    level2_triggers,
    1,

    level2_traps,
    4,

    bn::sprite_items::platformslvl2,
    bn::regular_bg_items::level2,
    bn::music_items::level2,

    {-96, -32},
    {104, 0}};
/*
constexpr PlatformData level3_platforms[] = {
    // --- Nebel ---
    { -80, -32, &bn::sprite_items_nebel, 0 },
    { -48, -32, &bn::sprite_items_nebel, 0 },
    { -16, -32, &bn::sprite_items_nebel, 0 },
    {  16, -32, &bn::sprite_items_nebel, 0 },

    { -80,  32, &bn::sprite_items_nebel, 0 },
    { -48,  32, &bn::sprite_items_nebel, 0 },
    { -16,  32, &bn::sprite_items_nebel, 0 },
    {  16,  32, &bn::sprite_items_nebel, 0 },
    {  48,  32, &bn::sprite_items_nebel, 0 },

    // --- Baumstämme ---
    { -48,  64, &bn::sprite_items_baumstamm, 0 },
    { -16,  64, &bn::sprite_items_baumstamm, 0 },
    {  32,  64, &bn::sprite_items_baumstamm, 0 },
    {  64,  64, &bn::sprite_items_baumstamm, 2 }
};

const LevelData LEVEL_3 = {
    level3_platforms,
    sizeof(level3_platforms) / sizeof(PlatformData),

    nullptr,
    0,

    nullptr,
    0,

    bn::sprite_items_level3,
    bn::regular_bg_items::level3,
    bn::music_items::level1,

    {-80, -64},
    {64, 32}
};*/
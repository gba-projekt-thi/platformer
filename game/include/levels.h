#pragma once
#include "level.h"

constexpr PlatformData level0_platforms[] =
    {{-96, 60, 0}, {-80, 60, 1}, {-48, 60, 2}, {-16, 60, 3}, {0, 40, 0}};

constexpr TriggerData level0_triggers[] = {{-50, 30, 30, 30}};

constexpr TrapData level0_traps[] = {
    {TrapType::MOVING, 10, 10, 50, 50, bn::sprite_items::bubbles, -0.1, 0.1, 5,
     0},
    {TrapType::BASE, -20, -20, 50, 50, bn::sprite_items::bubbles, 0, 0, 0, -1}};

const LevelData LEVEL_0 = {
    level0_platforms,
    5,
    level0_triggers,
    1,
    level0_traps,
    2,
    bn::sprite_items::platformslvl1,
    bn::regular_bg_items::level1,
    bn::music_items::level1};

constexpr PlatformData level1_platforms[] = {
    {-96, 60, 0}, {-80, 60, 1}, {-48, 60, 3},
    {-16, 60, 0}, {-16, 44, 2}, /*{16, 60, 3},*/ {32, 60, 1},
    {48, 60, 1},  {64, 60, 0},  {80, 60, 1},
    {96, 60, 0},  {112, 60, 2}};

constexpr TriggerData level1_triggers[] = {{80, 30, 30, 30}};

constexpr TrapData level1_traps[] = {
    {TrapType::MOVING, 80, -32, 32, 32, bn::sprite_items::bubbles, 0.0, 1, 5,
     0},
    {TrapType::BASE, -48, 8, 32, 32, bn::sprite_items::bubbles, 0, 0, 0, -1},
    {TrapType::BASE, -64, 60, 16, 16, bn::sprite_items::connector16x16, 0, 0, 0,
     -1},
    {TrapType::BASE, -32, 60, 16, 16, bn::sprite_items::connector16x16, 0, 0, 0,
     -1}};

const LevelData LEVEL_1 = {
    level1_platforms,
    11,
    level1_triggers,
    1,
    level1_traps,
    4,
    bn::sprite_items::platformslvl1,
    bn::regular_bg_items::level1,
    bn::music_items::level1};
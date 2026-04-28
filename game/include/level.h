#pragma once
#include "bn_fixed.h"

enum class TrapType { BASE, MOVING };

struct PlatformData {
    int x;
    int y;
    int sprite_index;
};

struct TriggerData {
    int x;
    int y;
    int width;
    int height;
};

struct TrapData {
    TrapType type;

    int x;
    int y;
    int width;
    int height;

    const bn::sprite_item sprite;

    // moving trap parameters
    bn::fixed velocity_x;
    bn::fixed velocity_y;
    int range;

    int trigger_index;  // which trigger this trap uses (-1 if none)
};

struct LevelData {
    const PlatformData* platforms;
    int platform_count;

    const TriggerData* triggers;
    int trigger_count;

    const TrapData* traps;
    int trap_count;

    const bn::sprite_item sprite_item_platform;
    const bn::regular_bg_item back_ground;
    const bn::dmg_music_item music;
};
#pragma once
#include "bn_fixed.h"
#include "bn_music_items.h"
#include "bn_span.h"

enum class TrapType { BASE, MOVING, PATH };

struct PlatformData {
    int x;
    int y;
    const bn::sprite_item sprite;
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
    int sprite_waits;
    bn::span<const uint16_t> graphic_indexes;

    // moving trap parameters
    bn::fixed velocity_x;
    bn::fixed velocity_y;
    bn::fixed max_vel;
    bn::fixed range;

    int trigger_index;  // which trigger this trap uses (-1 if none)

    // path trap parameters
    bn::span<const bn::fixed_point> path;
    unsigned int path_waits;
};

struct PlayerData {
    bn::fixed x;
    bn::fixed y;
};

struct DoorData {
    bn::fixed x;
    bn::fixed y;
};

struct LevelData {
    const PlatformData* platforms;
    int platform_count;

    const TriggerData* triggers;
    int trigger_count;

    const TrapData* traps;
    int trap_count;

    const bn::regular_bg_item back_ground;
    const bn::music_item music;

    // Player start position.
    const PlayerData player_data;
    // Level exit door position.
    const DoorData door;
};
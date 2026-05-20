#pragma once

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_music_item.h"
#include "bn_regular_bg_item.h"
#include "bn_span.h"
#include "bn_sprite_item.h"

// -----------------------------------------------------------------------------
// TrapType
//
// Defines runtime trap behavior.
//
// BASE   -> Static damaging trap
// MOVING -> Velocity/acceleration based trap
// PATH   -> Path interpolation trap
// -----------------------------------------------------------------------------
enum class TrapType { BASE, MOVING, PATH };

// -----------------------------------------------------------------------------
// PlatformData
//
// Static level collision/render object.
// -----------------------------------------------------------------------------
struct PlatformData {
    int x;
    int y;

    bn::sprite_item sprite;

    // Tile index inside sprite tileset.
    int sprite_index;
};

// -----------------------------------------------------------------------------
// TriggerData
//
// Invisible collision volume used to activate gameplay events.
// -----------------------------------------------------------------------------
struct TriggerData {
    int x;
    int y;

    int width;
    int height;

    // Optional:
    // Allows triggers to start already active.
    bool default_on = false;
};

// -----------------------------------------------------------------------------
// TrapData
//
// Unified level trap definition.
//
// Some fields are only used depending on TrapType.
//
// NOTE:
// This structure intentionally stays POD-like because many instances may exist
// and level data is typically static ROM data on GBA.
// -----------------------------------------------------------------------------
struct TrapData {
    // -------------------------------------------------------------------------
    // Common
    // -------------------------------------------------------------------------

    TrapType type;

    int x;
    int y;

    int width;
    int height;

    bn::sprite_item sprite;

    // Frames to wait before advancing animation.
    // Must be >= 1 when animation frames exist.
    int animation_wait = 1;

    // Empty span = no animation.
    bn::span<const uint16_t> graphic_indexes;

    // Trigger index used by moving/path traps.
    // -1 means fallback trigger.
    int trigger_index = -1;

    // -------------------------------------------------------------------------
    // MovingTrap
    // -------------------------------------------------------------------------

    // Acceleration applied every frame once triggered.
    bn::fixed velocity_x = 0;
    bn::fixed velocity_y = 0;

    // Maximum velocity clamp.
    bn::fixed max_vel = 0;

    // -------------------------------------------------------------------------
    // PathTrap
    // -------------------------------------------------------------------------

    // Relative path points from trap origin.
    bn::span<const bn::fixed_point> path;

    // Frames required to move between path nodes.
    unsigned path_waits = 0;
};

// -----------------------------------------------------------------------------
// PlayerData
//
// Player spawn point.
// -----------------------------------------------------------------------------
struct PlayerData {
    bn::fixed x;
    bn::fixed y;
};

// -----------------------------------------------------------------------------
// DoorData
//
// Level completion location.
// -----------------------------------------------------------------------------
struct DoorData {
    bn::fixed x;
    bn::fixed y;
};

// -----------------------------------------------------------------------------
// LevelData
//
// Immutable level definition.
//
// Intended to live entirely in ROM.
// -----------------------------------------------------------------------------
struct LevelData {
    // -------------------------------------------------------------------------
    // Platforms
    // -------------------------------------------------------------------------

    const PlatformData* platforms = nullptr;

    int platform_count = 0;

    // -------------------------------------------------------------------------
    // Triggers
    // -------------------------------------------------------------------------

    const TriggerData* triggers = nullptr;

    int trigger_count = 0;

    // -------------------------------------------------------------------------
    // Traps
    // -------------------------------------------------------------------------

    const TrapData* traps = nullptr;

    int trap_count = 0;

    // -------------------------------------------------------------------------
    // Visuals / Audio
    // -------------------------------------------------------------------------

    bn::regular_bg_item back_ground;

    bn::music_item music;

    // -------------------------------------------------------------------------
    // Gameplay
    // -------------------------------------------------------------------------

    PlayerData player_data;

    DoorData door;
};
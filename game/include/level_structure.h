#pragma once

#include <cstdint>

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_music_item.h"
#include "bn_regular_bg_item.h"
#include "bn_span.h"
#include "bn_sprite_item.h"

#include "cfg.h"

// -----------------------------------------------------------------------------
// TrapType
//
// Defines runtime trap behavior.
//
// BASE   -> Static damaging trap
// MOVING -> Velocity/acceleration based trap
// PATH   -> Path interpolation trap
// CHASE  -> Trails behind the player, closing in as they move right
// -----------------------------------------------------------------------------
enum class TrapType : uint8_t { BASE, MOVING, PATH, CHASE };

// -----------------------------------------------------------------------------
// PlatformData
//
// Static level collision/render object.
// -----------------------------------------------------------------------------
struct PlatformData {
    int x;
    int y;
    int width;
    int height;
    int offset_x;
    int offset_y;
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

    // Optional stable identifier, immune to reordering this level's trigger
    // array. When set, traps should reference this trigger via
    // TrapData::trigger_name instead of a raw array index (see
    // TrapData::trigger_index for the legacy/fallback path). nullptr =
    // unnamed; the trigger is then only reachable by index.
    const char* name = nullptr;
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
    int offset_x;
    int offset_y;
    bn::sprite_item sprite;

    // Frames to wait before advancing animation.
    // Must be >= 1 when animation frames exist.
    int sprite_waits = 1;

    // Empty span = no animation.
    bn::span<const uint16_t> graphic_indexes;

    // Trigger index used by moving/path traps.
    // -1 means fallback trigger.
    // Only used when trigger_name (below) is nullptr - kept as the legacy
    // path and as the ultimate fallback target. Unused by TrapType::CHASE,
    // which tracks the player directly instead of a trigger.
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

    // -------------------------------------------------------------------------
    // ChaserTrap
    // -------------------------------------------------------------------------

    // How far behind (to the left of) the player the trap eases toward.
    bn::fixed chase_follow_distance = 0;

    // Max horizontal distance the trap closes per frame while chasing.
    bn::fixed chase_speed = 0;

    // -------------------------------------------------------------------------
    // Trigger binding (MovingTrap / PathTrap only)
    // -------------------------------------------------------------------------

    // Optional stable trigger identifier (see TriggerData::name). When set,
    // TrapFactory resolves the trigger via LevelManager::get_trigger_by_name()
    // instead of trigger_index above - reordering the level's trigger array
    // no longer breaks this trap's binding. Must stay the LAST member so
    // existing aggregate-init call sites in levels.h that omit it keep
    // compiling unchanged (defaults to nullptr = use trigger_index instead).
    const char* trigger_name = nullptr;
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

    // -------------------------------------------------------------------------
    // Camera
    // -------------------------------------------------------------------------

    // Full extents of the playable world, centered at (0,0). Values <= the
    // screen size keep the camera effectively static (current behavior for
    // the non-scrolling levels); larger values enable real scrolling.
    bn::fixed world_width = Cfg::Screen::WIDTH;
    bn::fixed world_height = Cfg::Screen::HEIGHT;
};
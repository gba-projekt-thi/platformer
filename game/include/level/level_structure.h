#pragma once

#include <cstdint>

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_music_item.h"
#include "bn_regular_bg_item.h"
#include "bn_span.h"
#include "bn_sprite_item.h"
#include "bn_sprite_items_door32x32.h"

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
// AMBUSH -> Dormant until the player is horizontally close, then lunges
//           once in one direction and returns to its start position
// -----------------------------------------------------------------------------
enum class TrapType : uint8_t { BASE, MOVING, PATH, CHASE, AMBUSH };

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

    // Optional stable trigger identifier. When set, TrapFactory resolves
    // the trigger via LevelManager::get_trigger_by_name() instead of a raw
    // array index, so reordering the level's trigger array never breaks a
    // trap's binding. Every MOVING/PATH trap in every level sets this - see
    // TrapData::trigger_name below. Note this is NOT the struct's last
    // field (AmbushTrap's fields below were appended after it) - it just
    // needs a default so positional literals that stop before it, i.e.
    // every one today, keep compiling unchanged.
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

    // Stable trigger identifier (see TriggerData::name) that every
    // MOVING/PATH trap in every level sets - TrapFactory always resolves
    // it via LevelManager::get_trigger_by_name(), by identity rather than
    // array position, so reordering a level's trigger array can never
    // silently rebind a trap. Note this is NOT the struct's last field
    // (AmbushTrap's fields below were appended after it) - it still needs
    // a default for BASE/CHASE/AMBUSH literals, which never set it.
    const char* trigger_name = nullptr;

    // -------------------------------------------------------------------------
    // AmbushTrap
    // -------------------------------------------------------------------------

    // Horizontal distance from the player at which the ambush triggers.
    bn::fixed ambush_range = 0;

    // Per-frame horizontal step while lunging. Sign gives the lunge
    // direction (positive = right, negative = left); magnitude is also
    // used as the return-to-start step speed.
    bn::fixed ambush_speed = 0;

    // Frames spent lunging before switching to the return phase.
    unsigned ambush_duration = 0;
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
// The default door32x32 asset's open-door animation: every other frame
// across its tile strip. Sprites used as a DoorData override (e.g. a
// narrative "goal" marker like Susanne) should pass their own
// graphics_indexes sequence instead - this list is specific to
// door32x32's frame layout and won't make sense for a different sprite.
constexpr uint16_t DOOR32X32_GRAPHICS_INDEXES[] = {0, 2, 4, 6, 8, 10, 12};

struct DoorData {
    bn::fixed x;
    bn::fixed y;

    // Optional: overrides the default door32x32 sprite/animation/facing.
    // Appended last so every existing level's aggregate-init
    // `.door = {x, y}` keeps compiling unchanged. Used for narrative
    // "goal" markers, e.g. the World 5 finale's reunion with Susanne.
    bn::sprite_item sprite = bn::sprite_items::door32x32;
    // Empty span = static/non-animated (same convention as TrapData's
    // graphics_indexes / BaseTrap).
    bn::span<const uint16_t> sprite_graphics_indexes =
        DOOR32X32_GRAPHICS_INDEXES;
    int sprite_animation_wait = 8;
    bool sprite_flip_horizontal = false;
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

    // -------------------------------------------------------------------------
    // Feel
    // -------------------------------------------------------------------------

    // Multiplier on the duck's ground braking (Cfg::Player::DECELERATION),
    // applied via Player::set_ground_friction() when the level loads.
    // 1 = normal stopping (default, matches every level before this field
    // existed). A value < 1 makes the level's floor slippery - the duck
    // skids further before stopping - for level-to-level feel variation
    // (e.g. a mossy World 3 forest floor) without touching acceleration,
    // gravity, or jump speed, so it can never affect whether a platform or
    // gap is reachable. Appended last so every existing level literal
    // keeps compiling unchanged.
    bn::fixed ground_friction = 1;
};
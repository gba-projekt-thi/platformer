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
#include "trigger_logic.h"

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
// CYCLE  -> Moves out by (cycle_dx, cycle_dy), stays, moves back and waits,
//           again and again on its own clock (spikes out of the floor)
// BOSS   -> The one enemy the duck can beat: follows `path` like PATH (faster
//           every phase), kills on touch, and loses one of its boss_hits to
//           every pogo strike from above (see BossTrap / boss_logic.h)
// -----------------------------------------------------------------------------
enum class TrapType : uint8_t {
    BASE,
    MOVING,
    PATH,
    CHASE,
    AMBUSH,
    CYCLE,
    BOSS
};

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

    // Optional: binds the platform to a trigger (TriggerData::name) so a
    // switch can open or close it. nullptr = always solid and visible, as
    // every platform before this field existed. Appended last so existing
    // aggregate-init literals keep compiling unchanged.
    const char* trigger_name = nullptr;
    // With trigger_name set: true = the platform is solid and visible while
    // the trigger is on (appears when switched on); false = it is solid and
    // visible while the trigger is off (vanishes when switched on). See
    // switch_logic::platform_active().
    bool active_when_on = true;

    // Crumbling platform: > 0 makes it give way that many frames after the
    // duck first stands on it (it shakes meanwhile and stays solid), see
    // crumble_logic.h. 0 = an ordinary platform. Cannot be combined with
    // trigger_name. Appended last so existing literals compile unchanged.
    unsigned crumble_frames = 0;
    // Frames after which a crumbled platform comes back (once the duck is not
    // in its box); 0 = it stays gone until a death or restart.
    unsigned crumble_respawn_frames = 0;

    // Level Devil: a platform that runs away (fleeing_platforms.h), like the
    // door's flee_path: whenever the duck comes within flee_range px it slides
    // to the next point of flee_path (offsets from (x, y)) in flee_frames
    // frames. In the JSON a list of [x, y] pairs. Not combined with
    // trigger_name or crumbling. Empty = it stays put.
    bn::span<const bn::fixed_point> flee_path = {};
    int flee_range = 0;
    int flee_frames = 0;
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

    // How entering/leaving the zone changes the trigger's state - see
    // TriggerMode in trigger_logic.h. Latch (the default) is the original
    // behavior every existing trigger keeps. Appended after `name` so
    // positional literals that stop earlier keep compiling unchanged.
    TriggerMode mode = TriggerMode::Latch;

    // Optional visible switch/plate: a sprite item drawn at the trigger's
    // position, showing tile `sprite_index_off` while the trigger is off and
    // `sprite_index_on` while it is on. nullptr = invisible (the default,
    // like every trigger so far). Points at a constexpr sprite item, e.g.
    // &bn::sprite_items::flower1.
    const bn::sprite_item* sprite = nullptr;
    int sprite_index_off = 0;
    int sprite_index_on = 1;

    // Frames between the player's action and the effect on traps/platforms
    // (every mode except Timer), or the timer length of a Timer trigger: it
    // switches on by itself this many frames after the level (re)starts. 0 =
    // immediate, as every trigger so far. Appended last so positional
    // literals that stop earlier keep compiling unchanged.
    unsigned delay_frames = 0;

    // What counts as "entering" the zone: the duck stepping in (Enter, the
    // default) or jumping / dashing while inside it (Jump, Dash). Appended
    // after delay_frames.
    TriggerAction action = TriggerAction::Enter;
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

    // -------------------------------------------------------------------------
    // CycleTrap
    // -------------------------------------------------------------------------

    // How far the trap moves out from its start position (px). Spikes that
    // rise out of the floor: cycle_dx 0, cycle_dy negative.
    bn::fixed cycle_dx = 0;
    bn::fixed cycle_dy = 0;

    // One round in frames: out, stay out, back, rest (see
    // trap_timing::CycleTiming). The four must not all be 0.
    unsigned cycle_extend_frames = 0;
    unsigned cycle_hold_frames = 0;
    unsigned cycle_retract_frames = 0;
    unsigned cycle_wait_frames = 0;

    // Where in the round the trap starts (frames), so two traps with the same
    // timing can alternate. trigger_name (above) is optional for a CYCLE
    // trap: with one it only cycles while that trigger is on, without it it
    // cycles from the start of the level.
    unsigned cycle_phase_frames = 0;

    // -------------------------------------------------------------------------
    // Any trap
    // -------------------------------------------------------------------------

    // Draw order of the trap's sprite (smaller = in front). 0 = the default of
    // every trap so far, level with the duck. A trap that imitates something
    // that stands behind the duck uses Cfg::ZOrder::DOOR / CHECKPOINT (1): a
    // fake door or fake checkpoint then looks exactly like the real one.
    int sprite_z_order = 0;

    // Draws the sprite this many times larger (up to 2, the GBA's affine
    // double size); width / height are the scaled hitbox. 1 = as drawn.
    bn::fixed sprite_scale = 1;

    // -------------------------------------------------------------------------
    // Boss
    // -------------------------------------------------------------------------

    // Pogo strikes it takes (= phases, at most Cfg::Boss::MAX_PHASES). It
    // moves along `path` with `path_waits` frames per segment in phase 1,
    // faster in every later one (boss::phase_frames()). trigger_name is the
    // arena: the fight starts when it is on (nullptr = at once), and phase n
    // >= 2 switches on the trigger "<trigger_name>_p<n>" so more traps
    // attack; the last hit switches on "<trigger_name>_won" (both optional).
    // While a level's boss lives, its door stays shut.
    unsigned boss_hits = 0;

    // CHASE: how far right of its start the chaser flies at most (it stays
    // out of a boss arena); 0 = no limit (trap_timing::chase_target()).
    bn::fixed chase_limit = 0;

    // AMBUSH: lunge along y instead of x (ambush_speed < 0 = up) - arms or
    // spikes shooting out of the ground the moment the duck is above them.
    // The trigger stays the duck's horizontal distance (ambush_range).
    bool ambush_vertical = false;
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

    // Level Devil: a door that runs away (door_flee.h). Whenever the duck
    // comes within flee_range px of it, it slides on to the next point of
    // flee_path (offsets from (x, y), like TrapData::path) in flee_frames
    // frames; at the last one it stays. A death puts it back. In the JSON
    // the path is a list of [x, y] pairs. Empty = an ordinary door.
    bn::span<const bn::fixed_point> flee_path = {};
    int flee_range = 0;
    int flee_frames = 0;
};

// -----------------------------------------------------------------------------
// CheckpointData
//
// A flag the duck can touch to respawn there (see checkpoint_logic.h).
// (x, y) is the center of the 16x16 flag sprite AND the respawn position:
// place it with its bottom edge on a platform's top (y = platform top - 8) so
// the flag stands on the platform. List a level's checkpoints in the order it
// is played. tools/Check_checkpoints.py verifies the placement.
// -----------------------------------------------------------------------------
struct CheckpointData {
    int x;
    int y;
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

    // -------------------------------------------------------------------------
    // Hint
    // -------------------------------------------------------------------------

    // Optional one-time hint shown at the top of the screen when the level
    // starts, for hint::MAX_CHARS characters at most (checked by
    // tools/Check_level_hints.py). It stays until the level has been cleared
    // once (no personal best yet), is hidden after Cfg::Hint::FRAMES or as
    // soon as B is pressed, and never blocks play. nullptr = no hint, as for
    // every level before this field existed. Appended last so existing
    // literals keep compiling unchanged.
    const char* intro_hint = nullptr;

    // -------------------------------------------------------------------------
    // Checkpoints
    // -------------------------------------------------------------------------

    // Optional checkpoints, in play order, at most
    // Cfg::Level::Limits::CHECKPOINTS. nullptr / 0 = none, as for every
    // level before this field existed. Appended last so existing literals
    // keep compiling unchanged.
    const CheckpointData* checkpoints = nullptr;
    int checkpoint_count = 0;
};
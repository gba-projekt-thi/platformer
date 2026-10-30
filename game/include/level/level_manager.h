#pragma once

#include "bn_core.h"
#include "bn_dmg_music_item.h"
#include "bn_music.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"

#include "attempt_variant.h"
#include "base_trap.h"
#include "camera.h"
#include "camera_rig.h"
#include "cfg.h"
#include "checkpoint.h"
#include "checkpoint_logic.h"
#include "checkpoint_system.h"
#include "crumble_logic.h"
#include "data_manager.h"
#include "death_fx.h"
#include "door.h"
#include "dynamic_platforms.h"
#include "fleeing_platforms.h"
#include "game_modes.h"
#include "game_state.h"
#include "i_resettable.h"
#include "level_flow.h"
#include "level_structure.h"
#include "log.h"
#include "pause_controller.h"
#include "platform_set.h"
#include "player.h"
#include "respawn_logic.h"
#include "save_sync_controller.h"
#include "sprite.h"
#include "trigger.h"

// -----------------------------------------------------------------------------
// LevelManager
//
// Handles:
// - Level loading
// - Runtime updates
// - Pause state
// - Trap reset logic
// - Persistent save updates
//
// Uses fixed-capacity containers to avoid heap allocation and fragmentation.
// -----------------------------------------------------------------------------
class BossTrap;

class LevelManager {
   public:
    // Result of a single update() call.
    //
    // None          -> level keeps running (or is paused)
    // LevelComplete -> the door was reached, advance to the next level
    // ReturnToTitle -> the player chose "Title Screen" in the pause menu
    enum class UpdateResult { None, LevelComplete, ReturnToTitle };

    explicit LevelManager(Player& player, DataManager& data_manager);

    // restores hud (timer, counter, etc.)
    void restoreHUD();

    // Loads a level and initializes all entities.
    void load(const LevelData& level);

    // Unloads the current level and frees all temporary resources.
    // This is used when transitioning to the final kiss scene to ensure the
    // old level's palettes and sprites are released.
    void unload();

    // Advances the simulation by one frame.
    UpdateResult update();

    // Returns the trigger whose TriggerData::name matches (linear scan -
    // trivially cheap given Cfg::Level::Limits::TRIGGERS <= 16 and this is
    // only ever called once per trap at level load, never per-frame).
    // Falls back to trigger[0] with a BN_LOG_ERROR if no match is found
    // (defensive; every MOVING/PATH trap sets trigger_name).
    Trigger& get_trigger_by_name(const char* name);
    // Same, but nullptr when there is none (optional triggers, e.g. a boss's
    // phase triggers).
    Trigger* find_trigger(const char* name);

    auto player() -> Player& { return _player; }

    // Replay ROM: right after load(), the level as it stands after `deaths`
    // deaths in it, set up the way a respawn does.
    void set_deaths_in_level(unsigned deaths);

    // True while the pause menu is open. LevelScene reads it to release the
    // level hint's sprites for the duration (see hint::should_draw()).
    [[nodiscard]] auto paused() const -> bool {
        return _pause_controller.paused();
    }

    // The level has a boss (its hint waits for the arena, see
    // hint::should_start()) / the boss is fighting: the duck reached the arena.
    [[nodiscard]] auto has_boss() const -> bool { return _boss != nullptr; }
    [[nodiscard]] auto boss_fighting() const -> bool;

    // The level-clear moment: the door blinks (lit) or not.
    void flash_door(bool lit);

    // Frames elapsed since this level was loaded, excluding time spent
    // paused. Used by LevelScene to compute this attempt's clear time
    // for the best-time record on reaching the door.
    [[nodiscard]] unsigned level_frame_count() const {
        return _level_frame_count;
    }

    // Forwards SaveSyncController::no_deaths_this_attempt() - see there.
    [[nodiscard]] bool no_deaths_this_attempt() const {
        return _save_sync.no_deaths_this_attempt();
    }

    // True if Assist Mode is on (Hard Mode wins if both flags are ever
    // set - see modes::trap_speed_mode()).
    [[nodiscard]] bool assist_mode_active() const {
        return modes::assist_active(_data_manager.state());
    }

    // False if Assist Mode was on when the level loaded or is on now: neither
    // a best time nor a no-death clear is recorded then (a mid-level toggle
    // must not be able to launder an assisted run).
    [[nodiscard]] bool records_allowed() const {
        return !_loaded_in_assist_mode &&
               modes::records_enabled(_data_manager.state());
    }

    // Cfg::HardMode::SPEED_MULTIPLIER with Hard Mode,
    // Cfg::AssistMode::TRAP_SPEED_MULTIPLIER with Assist Mode, else 1 -
    // used by TrapFactory to scale trap speed values at creation time.
    [[nodiscard]] bn::fixed trap_speed_multiplier() const {
        switch (modes::trap_speed_mode(_data_manager.state())) {
            case modes::TrapSpeedMode::Hard:
                return Cfg::HardMode::SPEED_MULTIPLIER;
            case modes::TrapSpeedMode::Assist:
                return Cfg::AssistMode::TRAP_SPEED_MULTIPLIER;
            case modes::TrapSpeedMode::Normal:
            default:
                return bn::fixed(1);
        }
    }

   private:
    void _load_player_spawn(const LevelData& level);
    void _load_door(const LevelData& level);
    void _load_music(const LevelData& level);
    void _update_boss_music();
    void _load_background(const LevelData& level);
    void _setup_camera(const LevelData& level);
    // Aims the camera at the duck for this frame (smoothing, look-ahead, look
    // up / down - see CameraRig) and hands the result to Camera::follow().
    void _update_camera();

    // The steps of load(), unload(), update() and a respawn, in the order
    // level_flow.h gives them (death feedback: death_fx.h).
    struct FlowOps;
    // Level Devil: the respawn brought a different layout
    // (attempt_objects::layout_changes()) - a sound and a short shake.
    void _cue_layout_change(int shake_frames);

    static void _validate_level(const LevelData& level);
    void _load_platforms(const LevelData& level);
    void _load_triggers(const LevelData& level);
    void _load_traps(const LevelData& level);
    void _load_checkpoints(const LevelData& level);
    // Takes this frame's checkpoint touches (CheckpointSystem) and makes a
    // newly reached flag the respawn point.
    void _sync_checkpoints();
    // Restart Level / level load: no checkpoint, respawn at the level spawn.
    void _clear_checkpoint();
    // Hands the switched, crumbling and running platforms to _platforms (see
    // PlatformSet::load_dynamic()), after the platforms and triggers are
    // loaded. A respawn calls it again after every death.
    void _load_dynamic_platforms(const LevelData& level);

    // Level Devil: deaths of this visit of the level (the run's total since
    // load()), see attempt_variant.h.
    unsigned _deaths_in_level() const;
    // Whether an object with this death window (from_death / until_death) is
    // there on the current attempt.
    bool _present(unsigned from_death, unsigned until_death) const;
    // Shows / hides every platform and trap with a death window for the
    // current attempt (attempt_objects.h). At level load and after every
    // death, the checkpoint respawn and Restart Level included.
    void _apply_attempt(const LevelData& level);
    // One frame of the crumbling platforms, after the collision update.
    void _sync_crumble_platforms();

    // Resets all resettable entities (currently: traps) after player
    // death or a manual level restart. Iterates a generic list rather
    // than _traps directly so future non-trap entity types can opt in
    // without LevelManager needing to know their concrete type.
    void _reset_entities();

    // The platforms: bodies and pictures, the switched, crumbling and running
    // ones (platform_set.h).
    PlatformSet<
        bn::vector<StaticBody, Cfg::Level::Limits::PLATFORM_BODIES>,
        bn::vector<bn::unique_ptr<Sprite>, Cfg::Level::Limits::PLATFORMS>,
        Trigger,
        bn::fixed_point,
        Cfg::Level::Limits::PLATFORMS>
        _platforms;
    bn::vector<Trigger, Cfg::Level::Limits::TRIGGERS> _triggers;
    bn::vector<bn::unique_ptr<BaseTrap>, Cfg::Level::Limits::TOTAL_TRAPS>
        _traps;
    // The flags, the active one with the triggers that were on when it was
    // reached, and the level's own spawn point to fall back to
    // (checkpoint_system.h).
    CheckpointSystem<
        bn::vector<Checkpoint, Cfg::Level::Limits::CHECKPOINTS>,
        bn::fixed>
        _checkpoints;
    // Which trigger follows which (TriggerData::follows) and the hand-over
    // between them, once per frame.
    TriggerChains _trigger_chains;
    static_assert(
        Cfg::Level::Limits::TRIGGERS <= TriggerChains::MAX_TRIGGERS,
        "TriggerChains must cover every trigger of a level");

    // Non-owning view over every currently-loaded IResettable entity:
    // the triggers (registered in _load_triggers()) and the traps
    // (_load_traps()); ownership stays with the owning containers.
    bn::vector<
        IResettable*,
        Cfg::Level::Limits::TOTAL_TRAPS + Cfg::Level::Limits::TRIGGERS>
        _resettables;

    bn::optional<bn::regular_bg_ptr> _background;
    Player& _player;
    // Where the camera aims. Cut to the duck on level load and on respawn.
    CameraRig<bn::fixed> _camera_rig;

    // Death feedback and Level Devil counts (see level_flow::Attempt);
    // _pending_respawn defers the trap reset until the freeze ends, so the
    // trap that killed the duck stays where it is while the world is frozen.
    death_fx::Sequence _death_fx;
    level_flow::Attempt _attempt;
    // The loaded level (ROM data), for the per-attempt work after a death;
    // nullptr after unload().
    const LevelData* _level = nullptr;
    respawn::Pending _pending_respawn;
    bn::optional<Door> _door;
    // The level's boss (TrapType::BOSS, owned by _traps) or nullptr. While
    // it lives the door stays shut (_sync_boss_door()).
    BossTrap* _boss = nullptr;
    bool _door_locked = false;
    void _sync_boss_door();
    // False exits (door_logic.h): the trigger that turns the door into a
    // trap (DoorData::trap_trigger_name, nullptr = none) and the door's spot
    // for this attempt (DoorData::attempt_positions).
    Trigger* _door_trap = nullptr;
    void _place_door();
    // A boss hit freezes the world for a moment and shakes the picture.
    int _boss_hits_seen = 0;
    // The boss section of the level's music plays (see _update_boss_music()).
    bool _boss_music = false;
    void _feel_boss_hits();
    bn::optional<bn::music_item> _music;
    PauseController _pause_controller;
    DataManager& _data_manager;
    SaveSyncController _save_sync;

    // See level_frame_count(). Reset in _load_player_spawn(), advanced
    // once per non-paused frame in update().
    unsigned _level_frame_count = 0;

    // Whether Assist Mode was active when the current level loaded (set in
    // _load_player_spawn()); see records_allowed().
    bool _loaded_in_assist_mode = false;
};
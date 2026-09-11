#pragma once

#include "bn_core.h"
#include "bn_dmg_music_item.h"
#include "bn_log.h"
#include "bn_music.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"

#include "camera.h"
#include "cfg.h"
#include "data_manager.h"
#include "door.h"
#include "game_state.h"
#include "level_structure.h"
#include "moving_trap.h"
#include "path_trap.h"
#include "pause_controller.h"
#include "player.h"
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

    // Returns a valid trigger reference.
    // Falls back to trigger[0] if invalid.
    Trigger& get_trigger(int trigger_index);

    auto player() -> Player& { return _player; }

   private:
    void _load_player_spawn(const LevelData& level);
    void _load_door(const LevelData& level);
    void _load_music(const LevelData& level);
    void _load_background(const LevelData& level);
    void _setup_camera(const LevelData& level);

    void _clear_runtime_state();
    static void _validate_level(const LevelData& level);
    void _load_platforms(const LevelData& level);
    void _load_triggers(const LevelData& level);
    void _load_traps(const LevelData& level);

    // Resets all traps after player death.
    void _reset_traps();

    // Persists deaths/timer into the runtime save state without touching
    // the current level index. Used both by death sync and by "return to
    // title" from the pause menu.
    void _save_progress();

    bn::vector<bn::sprite_ptr, Cfg::Level::Limits::PLATFORMS> _platforms;
    bn::vector<StaticBody, Cfg::Level::Limits::PLATFORM_BODIES>
        _platform_bodies;
    bn::vector<Trigger, Cfg::Level::Limits::TRIGGERS> _triggers;
    bn::vector<bn::unique_ptr<BaseTrap>, Cfg::Level::Limits::TOTAL_TRAPS>
        _traps;
    bn::optional<bn::regular_bg_ptr> _background;
    Player& _player;
    bn::optional<Door> _door;
    bn::optional<bn::music_item> _music;
    unsigned _last_death_ct = 0;
    PauseController _pause_controller;
    DataManager& _data_manager;
};
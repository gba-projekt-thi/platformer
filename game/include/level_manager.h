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

#include "cfg.h"
#include "data_manager.h"
#include "door.h"
#include "game_state.h"
#include "level_structure.h"
#include "moving_trap.h"
#include "path_trap.h"
#include "pause_controller.h"
#include "player.h"
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
    // Returns true when level completed.
    bool update();

    // Returns a valid trigger reference.
    // Falls back to trigger[0] if invalid.
    Trigger& get_trigger(int trigger_index);

   private:
    void _init_pause_menu();

    // Resets all traps after player death.
    void _reset_traps();

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
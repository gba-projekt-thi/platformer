#pragma once

#include "bn_core.h"
#include "bn_dmg_music_item.h"
#include "bn_log.h"
#include "bn_music.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "cfg.h"
#include "data_manager.h"
#include "door.h"
#include "game_state.h"
#include "level_structure.h"
#include "moving_trap.h"
#include "path_trap.h"
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
    explicit LevelManager(Player* player, DataManager& data_manager);

    // Loads a level and initializes all entities.
    void load(const LevelData& level);

    // Advances the simulation by one frame.
    // Returns true when level completed.
    bool update();

   private:
    void _init_pause_menu();

    // Returns a valid trigger reference.
    // Falls back to trigger[0] if invalid.
    Trigger& _get_trigger(int trigger_index);

    // Resets all traps after player death.
    void _reset_traps();

   private:
    bn::vector<bn::sprite_ptr, Cfg::Level::Limits::PLATFORMS> _platforms;

    bn::vector<StaticBody, Cfg::Level::Limits::PLATFORM_BODIES>
        _platform_bodies;

    bn::vector<Trigger, Cfg::Level::Limits::TRIGGERS> _triggers;

    bn::vector<BaseTrap, Cfg::Level::Limits::BASE_TRAPS> _base_traps;

    bn::vector<MovingTrap, Cfg::Level::Limits::MOVING_TRAPS> _moving_traps;

    bn::vector<PathTrap, Cfg::Level::Limits::PATH_TRAPS> _path_traps;

    bn::optional<bn::regular_bg_ptr> _background;

    Player* _player;

    bn::optional<Door> _door;

    bn::optional<bn::music_item> _music;

    bn::vector<bn::sprite_ptr, 64> _pause_sprites;

    bool _pause_menu_initialized = false;

    bool _paused = false;
    bool _prev_paused = false;

    unsigned _last_death_ct = 0;

    DataManager _data_manager;
};
#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "data_manager.h"
#include "game_session.h"
#include "level_manager.h"
#include "player.h"

class LevelScene : public core::Scene {
   public:
    LevelScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager,
        GameSession& session);
    void init() override;
    void update() override;

   private:
    void _save_progress_for_next_level();
    void _advance_to_next_level();
    void _finish_game();
    void _handle_level_transition();
    void _update_completion_state();

    Player& _player;
    bn::span<const LevelData> _levels;
    unsigned int _level_index;
    LevelManager& _level_manager;
    DataManager& _data_manager;
    GameSession& _session;

    // Prevents multiple scene transitions.
    bool _transition_requested{false};
};
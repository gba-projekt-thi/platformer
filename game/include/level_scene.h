#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "data_manager.h"
#include "level_manager.h"
#include "player.h"

// TODO:
// Replace with GameSession or SceneResult system.
// Global mutable state becomes difficult to maintain
// as the project scales.
inline bool game_finished = false;

class LevelScene : public core::Scene {
   public:
    LevelScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager);
    void init() override;
    void update() override;

   private:
    Player& _player;
    bn::span<const LevelData> _levels;
    unsigned int _level_index;
    LevelManager& _level_manager;
    DataManager& _data_manager;

    // Prevents multiple scene transitions.
    bool _transition_requested;
};
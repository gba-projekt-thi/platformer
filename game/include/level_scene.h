#pragma once
#include "core_scene.h"
#include "core_scene_manager.h"
#include "data_manager.h"
#include "level_manager.h"
#include "player.h"

inline bool game_finished = false;

class LevelScene : public core::Scene {
   public:
    LevelScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager);
    void init() override;
    void update() override;

   private:
    Player& _player;
    bn::span<const LevelData> _levels;
    unsigned int _level_index;
    LevelManager _level_manager;
    DataManager& _data_manager;
    bool _transition_requested;
};
#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_span.h"

#include "level_structure.h"

class Player;
class DataManager;
class LevelManager;

class LevelScene : public core::Scene {
   public:
    LevelScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager);
    ~LevelScene() override;

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
#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "data_manager.h"
#include "level_manager.h"
#include "levels.h"
#include "player.h"
#include "start_scene.h"

class KissingScene : public core::Scene {
   public:
    KissingScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager);

    void init() override;
    void update() override;

   private:
    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;

    // Countdown timer for showing the kiss scene before reverting to the
    // start screen.
    int _timer;

    // Prevent duplicate transition requests while the end scene is active.
    bool _transition_requested;

    // Keeps the kiss background alive for the duration of the scene.
    bn::optional<bn::regular_bg_ptr> _bg;
};

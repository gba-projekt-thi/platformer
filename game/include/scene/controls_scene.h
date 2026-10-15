#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"

#include "level_structure.h"

class Player;
class DataManager;
class LevelManager;

// -----------------------------------------------------------------------------
// ControlsScene
//
// Controls overview reachable from the start screen (the "Controls" entry
// below the save slots). A/B/Start returns to StartScene. The pause menu has
// its own in-place Controls view (PauseController) because leaving a running
// level for a scene would destroy it; both read ControlsText.
// -----------------------------------------------------------------------------
class ControlsScene : public core::Scene {
   public:
    ControlsScene(
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

    bool _transition_requested = false;

    bn::optional<bn::sprite_text_generator> _text_gen;
    // Title (8) + five short lines (<= 64 chars, see ControlsText) + hint
    // (18) = ~90 characters counting spaces.
    bn::vector<bn::sprite_ptr, 100> _sprites;
};

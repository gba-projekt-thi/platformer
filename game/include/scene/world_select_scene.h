#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"

#include "level_structure.h"

class Player;
class DataManager;
class LevelManager;

// -----------------------------------------------------------------------------
// WorldSelectScene
//
// Lets the player jump directly to any world they have already reached
// (per the loaded save slot's game_state.level), skipping past worlds
// already completed. Reachable from StartScene (press B instead of A
// after picking a save slot). Selecting a world transitions to
// LevelSelectScene for that world; B returns to StartScene.
//
// Worlds beyond the furthest-reached level are shown but locked - pressing
// A on a locked entry does nothing (a rejection sound plays instead).
// -----------------------------------------------------------------------------
class WorldSelectScene : public core::Scene {
   public:
    WorldSelectScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager);

    void init() override;
    void update() override;

   private:
    void _rebuild_menu();
    [[nodiscard]] bool _is_unlocked(int world_index) const;

    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;

    int _selected_index = 0;
    bool _transition_requested = false;

    bn::optional<bn::sprite_text_generator> _text_gen;
    bn::vector<bn::sprite_ptr, 64> _menu_sprites;
};
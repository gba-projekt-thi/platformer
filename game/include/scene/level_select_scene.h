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
// LevelSelectScene
//
// Lists the individual levels belonging to one world (as chosen in
// WorldSelectScene) and lets the player jump directly to any level they
// have already reached. B returns to WorldSelectScene.
// -----------------------------------------------------------------------------
class LevelSelectScene : public core::Scene {
   public:
    LevelSelectScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager,
        int world_index);

    void init() override;
    void update() override;

   private:
    void _rebuild_menu();
    [[nodiscard]] bool _is_unlocked(int level_absolute_index) const;

    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;
    int _world_index;

    int _selected_index = 0;
    bool _transition_requested = false;

    bn::optional<bn::sprite_text_generator> _text_gen;
    // Sized for the longest world (World 5: 8 levels): only the selected row
    // shows its time ("> Lv 1 01:23.45*+", 14 visible glyphs; spaces draw no
    // sprite), the others "Lv 1*+" (5), plus the 18 of the goals line and the
    // 15 of the Mirror Mode line = 82 at most (checked by
    // tools/Check_sprite_budget.py against the hardware's 128).
    bn::vector<bn::sprite_ptr, 124> _menu_sprites;
};
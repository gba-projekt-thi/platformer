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
    // Sized for the longest world (World 5: 6 levels): each line is up to
    // ~16 visible glyphs ("> Level 6 01:23.45*"; spaces draw no sprite),
    // so 6 * 16 = 96, plus the 19 of the Mirror Mode line, with headroom. The
    // list also fits the screen: the sixth line ends at y = 78.
    bn::vector<bn::sprite_ptr, 160> _menu_sprites;
};
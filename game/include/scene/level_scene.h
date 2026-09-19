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
    void _finalize_advance(unsigned int next_level_index);
    void _finalize_completion();
    void _show_new_best_banner();
    void _hide_new_best_banner();

    Player& _player;
    bn::span<const LevelData> _levels;
    unsigned int _level_index;
    LevelManager& _level_manager;
    DataManager& _data_manager;

    // Prevents multiple scene transitions.
    bool _transition_requested;

    // Set when a level clear beats its stored best time. Holds the
    // scene on this frame's completed level, showing a banner, until
    // A/Start is pressed - only then does _finalize_advance()/
    // _finalize_completion() actually run (save + scene swap).
    bool _new_best_pending = false;
    bool _next_is_game_completion = false;
    unsigned int _pending_next_level_index = 0;

    bn::optional<bn::sprite_text_generator> _banner_text_gen;
    bn::vector<bn::sprite_ptr, 16> _banner_sprites;
};
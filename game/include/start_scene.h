#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "level_structure.h"

class Player;
class DataManager;
class LevelManager;

class StartScene : public core::Scene {
   public:
    StartScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager);
    void init() override;
    void update() override;

    void animation();

   private:
    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;

    int _selected_slot;
    bool _transition_requested;
    bn::vector<bn::sprite_ptr, 2> _title_sprites;

    // Graphics
    bn::optional<bn::regular_bg_ptr> _bg;
    bn::optional<bn::sprite_ptr> _schnabel_sprite;
    bn::optional<bn::sprite_ptr> _tail_sprite;
    bn::optional<bn::sprite_text_generator> _text_gen;
    bn::vector<bn::sprite_ptr, 64> _slot_sprites;

    bn::random _random;
};
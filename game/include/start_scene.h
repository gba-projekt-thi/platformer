#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "data_manager.h"
#include "level_scene.h"
#include "player.h"

#include "bn_regular_bg_items_startscreen.h"
#include "bn_sprite_text_generator.h"
#include "common_variable_8x16_sprite_font.h"

class StartScene : public core::Scene {
   public:
    StartScene(
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager);
    void init() override;
    void update() override;

   private:
    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;

    int _selected_slot;
    bool _transition_requested;

    // Graphics
    bn::optional<bn::regular_bg_ptr> _bg;
    bn::optional<bn::sprite_text_generator> _text_gen;
    bn::vector<bn::sprite_ptr, 64> _slot_sprites;
};

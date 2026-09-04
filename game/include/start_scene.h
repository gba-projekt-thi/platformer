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
        DataManager& data_manager,
        LevelManager& level_manager,
        GameSession& session);
    void init() override;
    void update() override;

   private:
    void _refresh_slot_text();
    void _handle_selection_change();
    void _handle_start_input();
    void _start_selected_save();

    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;
    GameSession& _session;

    int _selected_slot;
    bool _transition_requested;
    bn::vector<bn::sprite_ptr, 2> _title_sprites;

    // Graphics
    bn::optional<bn::regular_bg_ptr> _bg;
    bn::optional<bn::sprite_text_generator> _text_gen;
    bn::vector<bn::sprite_ptr, 64> _slot_sprites;
};

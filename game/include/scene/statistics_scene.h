#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"

#include "game_state.h"
#include "level_structure.h"

class Player;
class DataManager;
class LevelManager;

// -----------------------------------------------------------------------------
// StatisticsScene
//
// Deaths, play time, cleared levels and the deaths per level of one save slot,
// reachable from the start screen ("Statistics", or Select on a slot).
// Up/Down scroll the lines (statistics.h), Left/Right switch the slot, B
// returns to StartScene. Reads the slot with
// DataManager::peek_state(), so no slot gets loaded.
// -----------------------------------------------------------------------------
class StatisticsScene : public core::Scene {
   public:
    StatisticsScene(
        int slot,
        Player& player,
        bn::span<const LevelData> levels,
        DataManager& data_manager,
        LevelManager& level_manager);

    void init() override;
    void update() override;

   private:
    void _peek_slot();
    void _rebuild();

    int _slot;
    Player& _player;
    bn::span<const LevelData> _levels;
    DataManager& _data_manager;
    LevelManager& _level_manager;

    GameState _state;
    int _top = 0;
    bool _transition_requested = false;

    bn::optional<bn::sprite_text_generator> _text_gen;
    // One sprite per glyph: a screenful of lines (checked by
    // tests/host/test_statistics.cpp) plus the title and the hint.
    bn::vector<bn::sprite_ptr, 128> _sprites;
};

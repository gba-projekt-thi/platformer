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

#include "cfg.h"
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
    // Peeks all SAVE_SLOT_COUNT slots once (SRAM reads) into
    // _slot_used/_slot_furthest_world. Called only from init() - never
    // per-frame or per-keypress, since slot contents can't change while
    // this scene is up.
    void _peek_slot_progress();
    // Rebuilds the slot list text from _selected_slot (cursor) and the
    // cached _slot_used/_slot_furthest_world - no SRAM access. Called
    // from init() and whenever the cursor moves.
    void _rebuild_slot_list();

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
    // Bumped from 64: each slot line can now carry a "World N"/"New"
    // suffix on top of "> Slot N", worst case ~51 sprites across 3 slots.
    bn::vector<bn::sprite_ptr, 80> _slot_sprites;

    // Per-slot progress preview, peeked once in init() via
    // DataManager::peek_state() - not the active slot's runtime state.
    bool _slot_used[Cfg::StartScreen::SAVE_SLOT_COUNT] = {};
    int _slot_furthest_world[Cfg::StartScreen::SAVE_SLOT_COUNT] = {};

    bn::random _random;
};
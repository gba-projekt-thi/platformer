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
// SummaryScene
//
// A brief "run complete" readout shown once, right after KissingScene and
// before returning to the title screen: total time across every level's
// personal best, total deaths for this playthrough, and how many levels
// were cleared without dying. Purely an aggregation of data every level
// clear already records (GameState::best_time_frames / no_death_clears /
// deaths) - no new persistent field.
//
// DataManager::reset() - previously called at the end of KissingScene -
// happens here instead, once the player dismisses this screen with
// A/Start, so the numbers below are still the just-finished run's live
// state when this scene reads them, not whatever reset() leaves behind.
// -----------------------------------------------------------------------------
class SummaryScene : public core::Scene {
   public:
    SummaryScene(
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
    // Five short lines ("Game Complete!", total time, deaths, no-death
    // clears, "Press A to continue"), ~77 characters combined at time of
    // writing - sized with the same kind of headroom as LevelScene's New
    // Best banner (36 chars -> a 40-sprite buffer).
    bn::vector<bn::sprite_ptr, 100> _sprites;
};
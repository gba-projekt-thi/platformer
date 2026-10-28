#pragma once

#include "core_scene.h"
#include "core_scene_manager.h"

#include "bn_optional.h"
#include "bn_span.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"
#include "common_variable_8x16_sprite_font.h"

#include "clear_logic.h"
#include "hint_logic.h"
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
    // The level-clear moment (clear_logic.h): the world holds still, the door
    // blinks and "Clear!" stands there, then the banner or the next level.
    void _start_clear_moment();
    void _end_clear_moment();
    void _leave_level();
    // One-time level hint (LevelData::intro_hint, see hint_logic.h).
    void _show_hint_if_needed();
    void _draw_hint();
    void _update_hint();

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
    // The clear moment runs; the clear's follow-up is in the fields below.
    clear_logic::Moment _clear_moment;
    bool _clear_new_best = false;
    bn::vector<bn::sprite_ptr, 8> _clear_sprites;
    bool _next_is_game_completion = false;
    unsigned int _pending_next_level_index = 0;

    bn::optional<bn::sprite_text_generator> _banner_text_gen;
    // "New Best!" (9) + time e.g. "01:23.45" (8) + "Press A to
    // continue" (19) = 36 chars, +headroom.
    bn::vector<bn::sprite_ptr, 40> _banner_sprites;

    // One-time hint at the top of the screen. hint::MAX_CHARS (28) glyphs
    // at most, so 32 sprites leave headroom.
    bn::optional<bn::sprite_text_generator> _hint_text_gen;
    bn::vector<bn::sprite_ptr, 32> _hint_sprites;
    // The hint text while the hint is still pending (nullptr = none or
    // expired). Kept so the sprites can be drawn again after a pause: they
    // are released while the pause overlay is open.
    const char* _hint_text = nullptr;
    // Its countdown and sprite life (hint::Display).
    hint::Display _hint{Cfg::Hint::FRAMES};
};
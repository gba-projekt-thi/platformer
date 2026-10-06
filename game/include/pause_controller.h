#pragma once

#include "bn_optional.h"
#include "bn_sound_items.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

class DataManager;

// ----------------------------------------------------------------------------
// PauseController
//
// Handles:
// - Pause toggling (Start)
// - Pause menu navigation (Up/Down + A), matching the start screen controls
// - Menu actions: Continue, Restart Level, Options, Title Screen
// - An embedded Options sub-menu (Music/SFX volume), so adjusting sound
//   never leaves the paused overlay or triggers a scene transition
// ----------------------------------------------------------------------------
class PauseController {
   public:
    enum class Action { None, DeathRequested, ReturnToTitleRequested };

    explicit PauseController(DataManager& data_manager);

    Action update();

    bool paused() const;
    void reset();

   private:
    enum class MenuOption : int {
        Continue = 0,
        Restart = 1,
        Options = 2,
        Controls = 3,
        TitleScreen = 4,
        Count = 5
    };

    void _init_pause_menu();
    void _set_visible(bool visible);
    void _rebuild_menu();
    void _rebuild_options_menu();
    // Replaces the menu with the controls overview (see ControlsText).
    void _rebuild_controls_view();
    void _change_option_level(int delta);
    // Row index of the Assist Mode toggle: 2 normally, 3 once the Hard
    // Mode row (always row 2) is unlocked and shown.
    [[nodiscard]] int _assist_row() const;
    // Renders one "> Label < On/Off >" toggle row at the given row index.
    void _append_toggle_row(int row, const char* label, bool on);

    bool _paused = false;
    bool _pause_menu_initialized = false;
    bool _prev_paused = false;
    int _selected_index = 0;

    // Options sub-menu state.
    bool _showing_options = false;
    // Controls overview shown in place of the menu (A/B returns).
    bool _showing_controls = false;
    // 0 = Music, 1 = SFX, 2 = Hard Mode (once unlocked), then Assist Mode
    // (see _assist_row()).
    int _option_row = 0;

    bn::optional<bn::sprite_text_generator> _text_gen;
    bn::vector<bn::sprite_ptr, 8> _title_sprites;
    // Bumped from 64, then 80: the Options sub-menu's worst case (Music +
    // SFX + Hard Mode + Assist Mode rows plus one info line while a mode
    // is on) is ~80 characters counting spaces, so 96 leaves headroom.
    bn::vector<bn::sprite_ptr, 96> _menu_sprites;

    DataManager& _data_manager;
};
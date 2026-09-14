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
        TitleScreen = 3,
        Count = 4
    };

    void _init_pause_menu();
    void _set_visible(bool visible);
    void _rebuild_menu();
    void _rebuild_options_menu();
    void _change_option_level(int delta);

    bool _paused = false;
    bool _pause_menu_initialized = false;
    bool _prev_paused = false;
    int _selected_index = 0;

    // Options sub-menu state.
    bool _showing_options = false;
    int _option_row = 0;  // 0 = Music, 1 = SFX

    bn::optional<bn::sprite_text_generator> _text_gen;
    bn::vector<bn::sprite_ptr, 8> _title_sprites;
    bn::vector<bn::sprite_ptr, 64> _menu_sprites;

    DataManager& _data_manager;
};
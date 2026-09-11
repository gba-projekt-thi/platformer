#pragma once

#include "bn_optional.h"
#include "bn_sound_items.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

// ----------------------------------------------------------------------------
// PauseController
//
// Handles:
// - Pause toggling (Start)
// - Pause menu navigation (Up/Down + A), matching the start screen controls
// - Menu actions: Continue, Restart Level, Title Screen
// ----------------------------------------------------------------------------
class PauseController {
   public:
    enum class Action { None, DeathRequested, ReturnToTitleRequested };

    Action update();

    bool paused() const;
    void reset();

   private:
    enum class MenuOption : int {
        Continue = 0,
        Restart = 1,
        TitleScreen = 2,
        Count = 3
    };

    void _init_pause_menu();
    void _set_visible(bool visible);
    void _rebuild_menu();

    bool _paused = false;
    bool _pause_menu_initialized = false;
    bool _prev_paused = false;
    int _selected_index = 0;

    bn::optional<bn::sprite_text_generator> _text_gen;
    bn::vector<bn::sprite_ptr, 8> _title_sprites;
    bn::vector<bn::sprite_ptr, 64> _menu_sprites;
};
#pragma once

#include "bn_sound_items.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

class PauseController {
   public:
    enum class Action { None, DeathRequested };
    auto update() -> Action;

    [[nodiscard]] auto paused() const -> bool;
    void reset();

   private:
    void init_pause_menu();
    void set_visible(bool /*visible*/);

    bool _paused = false;
    bool _pause_menu_initialized = false;
    bool _prev_paused = false;
    bn::vector<bn::sprite_ptr, 64> _pause_sprites;
};
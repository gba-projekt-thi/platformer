// Butano
#include "bn_bg_palettes.h"
#include "bn_blending.h"
#include "bn_colors.h"
#include "bn_core.h"
#include "bn_span.h"
#include "bn_unique_ptr.h"

// Engine team
#include "collision_registry.h"
#include "sprite.h"

// Our team
#include "moving_trap.h"
#include "player.h"
#include "trigger.h"

// Level 1
#include "bn_sprite_items_bubbles.h"
#include "bn_sprite_items_connector16x16.h"
#include "bn_sprite_items_door32x32.h"
#include "bn_sprite_items_kugelfisch.h"
#include "bn_sprite_items_platformslvl1.h"

// Level 2
#include "bn_sprite_items_box32x32.h"
#include "bn_sprite_items_can32x32.h"
#include "bn_sprite_items_ladder.h"
#include "bn_sprite_items_pipe16x16.h"
#include "bn_sprite_items_platformslvl2.h"
#include "bn_sprite_items_rustynails32x16.h"

// Level 3
#include "bn_sprite_items_baumstamm.h"
#include "bn_sprite_items_branch32x16.h"
#include "bn_sprite_items_mushroom32x32.h"
#include "bn_sprite_items_nebel.h"
#include "bn_sprite_items_slithermanarms32x32.h"

// Wallpapers
#include "bn_regular_bg_items_level1.h"
#include "bn_regular_bg_items_level2.h"
#include "bn_regular_bg_items_level3.h"
#include "bn_regular_bg_ptr.h"

// Music
#include "bn_music_items.h"

// Levels
#include "core_scene.h"
#include "core_scene_manager.h"
#include "level_manager.h"
#include "level_scene.h"
#include "levels.h"

extern bool game_finished;

int main() {
    // -------------------------------------------------------------------------
    // Butan Initialization
    // Initialize the Butano engine core before using any rendering or input
    // features.
    // -------------------------------------------------------------------------

    bn::core::init();

    // Configure global fade behavior once.
    bn::blending::set_fade_color(bn::blending::fade_color_type::BLACK);
    bn::blending::set_fade_alpha(0);

    // -------------------------------------------------------------------------
    // Level List
    // -------------------------------------------------------------------------

    const LevelData levels[] = {LEVEL_0, LEVEL_1, LEVEL_2, LEVEL_3};

    // -------------------------------------------------------------------------
    // Create the player entity and apply an offset for the sprite anchor point.
    // -------------------------------------------------------------------------

    Player player(0, 0, 8, 8);

    // Initialize the level manager and begin the game loop.
    // Static lifetime prevents accidental destruction
    // during scene transitions.
    static DataManager data_manager;

#ifdef RESET_SAVED

#pragma message( \
    "make clean; make USERFLAGS=-DRESET_SAVED used to reset corrupted game state. After running game once it should be fine from now on without the flag")
    data_manager.reset();
    BN_LOG("Save data reset enabled via RESET_SAVED");

#endif

    // -------------------------------------------------------------------------
    // IMPORTANT:
    // Restore SRAM save into runtime memory ONCE during boot.
    //
    // After this point:
    // - gameplay uses runtime RAM state only
    // - saves explicitly write back to SRAM
    // -------------------------------------------------------------------------

    data_manager.load_from_save();

    // -------------------------------------------------------------------------
    // Initial Scene
    // -------------------------------------------------------------------------

    auto first_scene = bn::make_unique<LevelScene>(
        player, bn::span<const LevelData>(levels), data_manager);
    core::SceneManager::instance().set_next_scene(bn::move(first_scene));

    // -------------------------------------------------------------------------
    // Main Loop
    // -------------------------------------------------------------------------

    while (!game_finished) {
        core::SceneManager::instance().update();
    }

    // -------------------------------------------------------------------------
    // Ending Delay of 10s
    // -------------------------------------------------------------------------

    for (int i = 0; i < Cfg::Sleep::FINISHED_GAME; ++i) {
        bn::core::update();
    }

    // -------------------------------------------------------------------------
    // Game Finished
    //
    // Reset save after full completion.
    // -------------------------------------------------------------------------

    data_manager.reset();
}
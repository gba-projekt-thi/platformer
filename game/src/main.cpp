// Butano
#include "bn_blending.h"
#include "bn_core.h"
#include "bn_span.h"
#include "bn_unique_ptr.h"

// Engine team
#include "core_scene_manager.h"
#include "log.h"

// Our team
#include "data_manager.h"
#include "game_session.h"
#include "level_manager.h"
#include "levels.h"
#include "player.h"
#include "replay_runner.h"
#include "start_scene.h"
#include "story.h"
#include "story_scene.h"
#include "world_index.h"

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

    // static: the list is shared by every scene for the whole program, so it
    // lives in static storage instead of copying all LevelData structs onto
    // main()'s stack (IWRAM, shared with every call below).
    static const LevelData levels[] = {
        LEVEL_0,
        LEVEL_1,
        LEVEL_WORLD1_SCROLL,
        LEVEL_WORLD1_BOSS,
        LEVEL_2,
        LEVEL_WORLD2_SCROLL,
        LEVEL_WORLD2_BOSS,
        LEVEL_3,
        LEVEL_WORLD3_SCROLL,
        LEVEL_WORLD3_BOSS,
        LEVEL_WORLD4_POGO,
        LEVEL_WORLD4_SCROLL,
        LEVEL_WORLD4_SCROLL_2,
        LEVEL_WORLD4_BOSS,
        LEVEL_WORLD5_SCROLL,
        LEVEL_WORLD5_SCROLL_2,
        LEVEL_WORLD5_DASH,
        LEVEL_WORLD5_LEVERS,
        LEVEL_WORLD5_CLIMB,
        LEVEL_WORLD5_BOSS,
        LEVEL_WORLD5_FINALE};
    static_assert(
        sizeof(levels) / sizeof(levels[0]) == WorldIndex::LEVEL_COUNT,
        "levels[] and WorldIndex::WORLDS must list the same number of levels");

    // -------------------------------------------------------------------------
    // Create the player entity and apply an offset for the sprite anchor point.
    // -------------------------------------------------------------------------

    Player player(0, 0, Cfg::Player::BODY_WIDTH, Cfg::Player::BODY_HEIGHT);

    // Initialize the level manager and begin the game loop.
    // Static lifetime prevents accidental destruction
    // during scene transitions.
    static DataManager data_manager;

#ifdef RESET_SAVED

#pragma message( \
    "make clean; make USERFLAGS=-DRESET_SAVED used to reset corrupted game state. After running game once it should be fine from now on without the flag")
    data_manager.reset();
    BN_LOG_INFO(LogCategory::Save, "Save data reset enabled via RESET_SAVED");

#endif

    // -------------------------------------------------------------------------
    // Initial Scene: show Start / Save Slot selection first
    // -------------------------------------------------------------------------

    static LevelManager level_manager
        __attribute__((section(".ewram"))) (player, data_manager);

#if defined(PLATFORMER_REPLAY)
    // Emulator test build (`make replay`): plays every recorded level instead.
    run_replays(
        player, bn::span<const LevelData>(levels), data_manager, level_manager);
#endif

    auto first_scene = bn::make_unique<StartScene>(
        player, bn::span<const LevelData>(levels), data_manager, level_manager);

    core::SceneManager::instance().set_next_scene(bn::move(first_scene));

    // -------------------------------------------------------------------------
    // Main Loop
    // -------------------------------------------------------------------------

    while (true) {
        while (!GameSession::instance().is_finished()) {
            core::SceneManager::instance().update();
        }

        // -------------------------------------------------------------------------
        // After the final level completes, switch into dedicated endgame
        // scenes instead of directly showing the kiss background from main.
        // This ensures the previous level and its palettes are released first.
        // -------------------------------------------------------------------------

        // The story's ending first, then the reunion (StoryScene goes on to
        // KissingScene).
        auto ending = bn::make_unique<StoryScene>(
            story::ENDING, player, bn::span<const LevelData>(levels),
            data_manager, level_manager, StoryScene::Then::Reunion);
        core::SceneManager::instance().set_next_scene(bn::move(ending));

        // Reset the completion flag so the next scene can run normally.
        GameSession::instance().reset();
    }
}
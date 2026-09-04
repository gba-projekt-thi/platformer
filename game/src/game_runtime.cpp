#include "game_runtime.h"

#include "bn_blending.h"
#include "bn_core.h"
#include "bn_span.h"
#include "bn_unique_ptr.h"

#include "cfg.h"
#include "core_scene_manager.h"
#include "game_app.h"
#include "levels.h"
#include "start_scene.h"

namespace game {

GameRuntime::RuntimeContext::RuntimeContext()
    : data_manager(), player(0, 0, 8, 8), level_manager(player, data_manager) {}

auto GameRuntime::run() -> int {
    initialize_engine();
    configure_scene_settings();
    initialize_save_state();
    create_first_scene();
    finish_game_loop();
    return 0;
}

void GameRuntime::initialize_engine() {
    bn::core::init();
}

void GameRuntime::configure_scene_settings() {
    bn::blending::set_fade_color(bn::blending::fade_color_type::BLACK);
    bn::blending::set_fade_alpha(0);
}

void GameRuntime::initialize_save_state() {
#ifdef RESET_SAVED
#pragma message( \
    "make clean; make USERFLAGS=-DRESET_SAVED used to reset corrupted game state. After running game once it should be fine from now on without the flag")
    _context.data_manager.reset();
    BN_LOG("Save data reset enabled via RESET_SAVED");
#endif
}

void GameRuntime::create_first_scene() {
    const LevelData levels[] = {LEVEL_0, LEVEL_1, LEVEL_2, LEVEL_3};

    auto first_scene = bn::make_unique<StartScene>(
        _context.player, bn::span<const LevelData>(levels),
        _context.data_manager, _context.level_manager, _session);

    core::SceneManager::instance().set_next_scene(bn::move(first_scene));
}

void GameRuntime::finish_game_loop() {
    while (!_session.is_finished()) {
        core::SceneManager::instance().update();
    }

    for (int i = 0; i < Cfg::Sleep::FINISHED_GAME; ++i) {
        bn::core::update();
    }

    _context.data_manager.reset();
}

}  // namespace game

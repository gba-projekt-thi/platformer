#pragma once

#include "data_manager.h"
#include "game_session.h"
#include "level_manager.h"
#include "player.h"

namespace game {

class GameRuntime {
   public:
    auto run() -> int;

   private:
    struct RuntimeContext {
        DataManager data_manager;
        Player player;
        LevelManager level_manager;

        RuntimeContext();
    };

    GameSession _session;
    RuntimeContext _context;

    static void initialize_engine();
    static void configure_scene_settings();
    void initialize_save_state();
    void create_first_scene();
    void finish_game_loop();
};

}  // namespace game

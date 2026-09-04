#include "game_app.h"

#include "game_runtime.h"

namespace game {

auto App::run() -> int {
    GameRuntime runtime;
    return runtime.run();
}

}  // namespace game

#pragma once

#include "bn_span.h"

class DataManager;
class LevelManager;
class Player;
struct LevelData;

// -----------------------------------------------------------------------------
// run_replays
//
// `make replay` builds the game with PLATFORMER_REPLAY: instead of the title
// screen it plays every level that has a recording (replay_data.h) with the
// recorded buttons and logs whether the duck reached the door
// ("REPLAY <index> <level> pass|FAIL|none ...", then "REPLAY DONE ...").
// tools/replays.py runs it headless and reads the log. Never returns.
// -----------------------------------------------------------------------------
[[noreturn]] void run_replays(
    Player& player,
    bn::span<const LevelData> levels,
    DataManager& data_manager,
    LevelManager& level_manager);

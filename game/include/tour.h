#pragma once

// -----------------------------------------------------------------------------
// tour
//
// `make tour` builds the game with PLATFORMER_TOUR: a scripted walk through
// the real scenes (menus, story pages, levels, bosses, the ending) for
// tools/tour.py, which photographs it in mGBA. Butano replays the menu
// buttons (bn::core::init with tour_data::KEYPAD); inside a level with a
// recording (game/replays/) the recording plays. In every other build these
// calls do nothing.
// -----------------------------------------------------------------------------

class DataManager;
class Player;

namespace tour {

#if defined(PLATFORMER_TOUR)

// After bn::core::init: counts bn::core::update() calls (the frames of the
// keypad commands) and logs "TOUR SHOT <n>" at the frame of every shot
// (tour_data::SHOT_FRAMES) for tools/tour.py's camera.
void boot();
// The start screen opens: slot 1 gets the save of the tour's next part
// (tour_data::SLOT_1), slot 2 one with every level unlocked.
void start_screen(DataManager& data_manager);
// A level was loaded: play its recording, if there is one.
void level_started(Player& player, unsigned level_index);

#else

inline void boot() {}
inline void start_screen(DataManager&) {}
inline void level_started(Player&, unsigned) {}

#endif

}  // namespace tour

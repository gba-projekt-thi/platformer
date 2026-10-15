#pragma once

// -----------------------------------------------------------------------------
// GameSession
//
// Tracks whether the current playthrough has reached the end of the level
// list (all levels cleared). Replaces the previous file-scope mutable
// `game_finished` bool that was shared via `extern` between main.cpp and
// LevelScene with no ownership or encapsulation.
//
// Scope is deliberately a single flag today. If more cross-scene signals
// show up later (e.g. "return to title requested"), fold them into this
// class rather than reaching for another global.
// -----------------------------------------------------------------------------
class GameSession {
   public:
    static GameSession& instance() {
        static GameSession session;
        return session;
    }

    [[nodiscard]] bool is_finished() const { return _finished; }

    void set_finished() { _finished = true; }

    void reset() { _finished = false; }

   private:
    GameSession() = default;

    bool _finished = false;
};
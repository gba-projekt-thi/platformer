#pragma once

// -----------------------------------------------------------------------------
// GameSession
//
// Tracks whether the current playthrough has reached the end of the level
// list (all levels cleared), instead of a global shared between main.cpp and
// LevelScene.
//
// Deliberately a single flag. Further cross-scene signals belong in this
// class rather than in another global.
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
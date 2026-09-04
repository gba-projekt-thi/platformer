#pragma once

class GameSession {
   public:
    [[nodiscard]] auto is_finished() const -> bool { return _finished; }

    void finish() { _finished = true; }

   private:
    bool _finished = false;
};

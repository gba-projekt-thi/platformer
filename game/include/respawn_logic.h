#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// respawn_logic
//
// When LevelManager sets up the next attempt (_finish_respawn(): traps and
// triggers re-armed, this attempt's death windows and door spot applied).
// Split out so the rule has no Butano dependency and is tested on the host
// (tests/host/test_respawn_logic.cpp).
//
//   * A death the world caused (trap, trapped door, ...) freezes the world
//     first (death_fx.h): the next attempt comes when the freeze ends.
//   * Restart Level puts the duck at the spawn on purpose, before the world
//     moves in that frame: the next attempt has to be there at once, or the
//     physics step runs the old layout around the duck at the spawn - a trap
//     there would be a second death and skip a layout. Still one death, no
//     freeze, no shake.
// -----------------------------------------------------------------------------

namespace respawn {

enum class Cause : uint8_t { Death, Restart };

enum class When : uint8_t { Now, AfterFreeze };

inline auto when(Cause cause, bool frozen) -> When {
    return cause == Cause::Death && frozen ? When::AfterFreeze : When::Now;
}

// The respawn of a death that waits for its freeze. request() says whether to
// set up the next attempt now; otherwise freeze_ended() says so once, when the
// freeze is over. cancel(): the freeze is gone (level load / unload, Restart
// Level) and so is what it was waiting for.
class Pending {
   public:
    auto request(Cause cause, bool frozen) -> bool {
        if (when(cause, frozen) == When::Now) {
            return true;
        }
        _waiting = true;
        return false;
    }

    [[nodiscard]] auto freeze_ended() -> bool {
        const bool run = _waiting;
        _waiting = false;
        return run;
    }

    void cancel() { _waiting = false; }

   private:
    bool _waiting = false;
};

}  // namespace respawn

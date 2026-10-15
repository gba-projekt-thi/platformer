#pragma once

#include "bn_fixed.h"

// -----------------------------------------------------------------------------
// PlayerStateMachine
//
// Tracks the player's coarse movement state (Idle/Run/Jump/Fall), derived
// purely from ground state and vertical velocity. Extracted out of Player
// as the third step of the Player decomposition (after PlayerHud,
// PlayerAnimator).
// -----------------------------------------------------------------------------
class PlayerStateMachine {
   public:
    enum class State { Idle, Run, Jump, Fall };

    // Recomputes the state for this frame from ground state and vertical
    // velocity, and transitions if it changed. Call once per frame from
    // Player::update(), after ground state and velocity are resolved.
    void update(bool onGround, bn::fixed vel_y);

    State current() const { return _state; }

   private:
    void enter_state(State new_state);

    State _state = State::Idle;
};
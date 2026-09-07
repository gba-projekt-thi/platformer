#include "player_state_machine.h"

#include "bn_keypad.h"

void PlayerStateMachine::update(bool onGround, bn::fixed vel_y) {
    State new_state;

    if (!onGround) {
        new_state = (vel_y < 0) ? State::Jump : State::Fall;
    } else {
        // Hold down to enter the idle pose while grounded.
        if (bn::keypad::down_held()) {
            new_state = State::Idle;
        } else {
            // No input → stay in Run (but no animation update)
            new_state = State::Run;
        }
    }

    if (new_state != _state) {
        enter_state(new_state);
    }
}

// Transition into a new player movement state.
void PlayerStateMachine::enter_state(State new_state) {
    _state = new_state;
}
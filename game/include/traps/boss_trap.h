#pragma once

#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_span.h"

#include "base_trap.h"
#include "boss_logic.h"
#include "strike_targets.h"

class LevelManager;
class Trigger;

// ----------------------------------------------------------------------------
// BossTrap
//
// TrapType::BOSS - the one enemy the duck can beat (boss_logic.h has the
// rules). Moves along its path like a PathTrap, faster every phase; kills on
// touch like any trap; registered with StrikeTargets so a pogo strike from
// above costs it a hit. A hit starts the next phase and switches on that
// phase's trigger; the last hit removes it, which opens the level's door
// (LevelManager asks defeated()) and switches on the trigger "<arena>_won"
// if the level has one. reset() (death, restart) starts the fight over.
// ----------------------------------------------------------------------------
class BossTrap : public BaseTrap, public Strikeable {
   public:
    BossTrap(
        const TrapData& data,
        unsigned path_waits,
        LevelManager& level_manager);
    ~BossTrap() override;

    void update() override;
    void on_enter(uint16_t hit_layers, StaticBody* body) override;
    void reset() override;
    void on_struck() override;

    [[nodiscard]] bool defeated() const { return _fight.defeated(); }
    [[nodiscard]] int hits_left() const { return _fight.hits_left(); }

   private:
    void _place(bn::fixed x, bn::fixed y);
    bool _fighting() const;

    boss::Fight _fight;
    bn::fixed _start_x;
    bn::fixed _start_y;
    // The hitbox relative to the picture (TrapData::offset_x / offset_y).
    bn::fixed _offset_x;
    bn::fixed _offset_y;
    bn::span<const bn::fixed_point> _path;
    unsigned _path_waits;
    Trigger* _arena;
    // Phase n's trigger at index n (index 0 / 1 unused: phase 1 is the arena).
    Trigger* _phase_triggers[Cfg::Boss::MAX_PHASES + 1] = {};
    // "<arena>_won", switched on by the last hit (e.g. a bridge appears).
    Trigger* _won = nullptr;
    unsigned _segment = 0;
    unsigned _frame = 0;
};

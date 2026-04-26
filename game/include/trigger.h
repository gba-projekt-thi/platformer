#pragma once

#include "bn_fixed.h"
#include "bn_vector.h"

#include "physics_body.h"
#include "player.h"

class Trigger : public PhysicsBody {
   public:
    static constexpr uint16_t LAYERS = 0x0008;
    static constexpr u_int16_t MASK = Player::LAYERS;
    static constexpr uint16_t BLOCK = 0;

    Trigger(bn::fixed x, bn::fixed y, bn::fixed width, bn::fixed height);
    bool is_triggered();
    void on_enter(StaticBody& body) override;
    void reset();

   private:
    bool triggered;
};
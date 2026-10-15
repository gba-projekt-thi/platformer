#pragma once

#include "bn_sprite_tiles_ptr.h"

#include "cfg.h"
#include "physics_body.h"
#include "sprite.h"

// ----------------------------------------------------------------------------
// Checkpoint
//
// A flag the duck touches to respawn there instead of at the level's spawn
// point (LevelData::checkpoints, rules in checkpoint_logic.h).
//
// Only the touch is detected here: on_enter() raises a flag that LevelManager
// takes once per frame (take_hit()) and then decides whether this checkpoint
// becomes the active one. LevelManager also switches the flag's two frames:
// lowered grey = not reached, raised green = the active checkpoint.
//
// Deliberately NOT resettable: a death keeps the checkpoint; only Restart
// Level and loading the level clear it.
// ----------------------------------------------------------------------------
class Checkpoint : public PhysicsBody {
   public:
    // (x, y) is the center of the flag sprite, which is also where the duck
    // respawns. The touch zone is taller than the sprite so a jump past the
    // flag counts too.
    Checkpoint(bn::fixed t_x, bn::fixed t_y);

    void on_enter(uint16_t hit_layers, StaticBody* body) override;

    // True once after each touch.
    [[nodiscard]] auto take_hit() -> bool;

    void set_active(bool active);

   private:
    Sprite _flag;
    bn::sprite_tiles_ptr _tiles_off;
    bn::sprite_tiles_ptr _tiles_on;
    bool _hit = false;
};

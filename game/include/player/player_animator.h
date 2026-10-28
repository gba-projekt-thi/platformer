#pragma once

#include "bn_sprite_animate_actions.h"
#include "bn_sprite_item.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_vector.h"

#include "cfg.h"
#include "sprite.h"

// -----------------------------------------------------------------------------
// PlayerAnimator
//
// Owns the player's walk/jump sprite animations and picks the right frame
// each frame based on movement/ground state and keypad input.
// -----------------------------------------------------------------------------
class PlayerAnimator {
   public:
    PlayerAnimator(Sprite& sprite, const bn::sprite_item& tiles_source);

    // Picks and updates the walk/jump/idle/back/slide frame for the current
    // frame. onGround must reflect the already-resolved ground state for this
    // frame; sliding = locomotion::Core::wall_sliding().
    void update(bool onGround, bool sliding = false);

   private:
    Sprite& _sprite;

    // Preloaded tile handles (zero runtime allocation).
    bn::vector<bn::sprite_tiles_ptr, Cfg::Player::PLAYER_TILE_CACHE_SIZE>
        _cached_tiles;

    bn::sprite_animate_action<Cfg::Player::ANIMATION_FRAME_COUNT> _walk_action;
    bn::sprite_animate_action<Cfg::Player::ANIMATION_FRAME_COUNT> _jump_action;

    int _slide_frame = 0;  // counts while sliding, for the dust
    bool _was_moving = false;
    bool _was_jumping = false;
};
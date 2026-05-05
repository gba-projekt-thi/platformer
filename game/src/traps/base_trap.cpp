#include "base_trap.h"

BaseTrap::BaseTrap(
    bn::fixed t_x,
    bn::fixed t_y,
    bn::fixed t_width,
    bn::fixed t_height,
    const bn::sprite_item& t_sprite,
    int t_sprite_waits,
    bn::span<const uint16_t> t_graphics_indexes,
    uint16_t t_block,
    bn::fixed t_max_vel)
    : PhysicsBody(
          t_x,
          t_y,
          t_width,
          t_height,
          Cfg::Layer::UN_HITABLE,
          Cfg::Layer::PLAYER,
          t_block,
          t_max_vel),
      trap_sprite(
          t_sprite.create_sprite(t_x, t_y),
          t_x,
          t_y), 
        graphics_indexes(t_graphics_indexes) {
    this->sprite = &trap_sprite;
    
    if (!t_graphics_indexes.empty())
        action = bn::sprite_animate_action<Cfg::MAX_ANIMATION_FRAMES>::forever(trap_sprite.sprite(), t_sprite_waits, t_sprite.tiles_item(), graphics_indexes);

}

void BaseTrap::update() {
    if (action) {
        action->update();
    }
}

void BaseTrap::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    StaticBody* body) {
    static_cast<Player*>(body)->death();
}
#include "trigger.h"

#include "cfg.h"
#include "player_events.h"

Trigger::Trigger(
    bn::fixed t_x,
    bn::fixed t_y,
    bn::fixed t_width,
    bn::fixed t_height,
    bool t_default_on,
    const char* t_name,
    TriggerMode t_mode,
    const bn::sprite_item* t_sprite,
    int t_sprite_index_off,
    int t_sprite_index_on,
    unsigned t_delay_frames,
    TriggerAction t_action,
    bool t_follower)
    : PhysicsBody(
          t_x,
          t_y,
          t_width,
          t_height,
          Cfg::Layer::TRIGGER,
          Cfg::Layer::PLAYER,
          0),
      _zone(t_default_on, t_mode, t_delay_frames, t_action, t_follower),
      _name(t_name) {
    if (t_sprite != nullptr) {
        const int graphics_count = t_sprite->tiles_item().graphics_count();
        _tiles_off = t_sprite->tiles_item().create_tiles(
            t_sprite_index_off % graphics_count);
        _tiles_on = t_sprite->tiles_item().create_tiles(
            t_sprite_index_on % graphics_count);

        bn::sprite_ptr raw_sprite = t_sprite->create_sprite(t_x, t_y);
        raw_sprite.set_blending_enabled(true);

        // Wrapped in Sprite like every other world object, so it scrolls
        // with the camera via SpriteRegistry.
        _sprite.emplace(bn::move(raw_sprite), t_x, t_y);
        sprite = &*_sprite;
        _refresh_visual();
    }
}

bool Trigger::is_triggered() const {
    return _zone.triggered();
}

void Trigger::update() {
    // Jump / Dash triggers fire on the duck's action inside the zone.
    const PlayerEvents& events = PlayerEvents::instance();
    _zone.update(events.jumped(), events.dashed());
    // A Timer trigger changes its pressed state on its own.
    _refresh_visual();
}

void Trigger::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    _zone.enter();
    _refresh_visual();
}

void Trigger::on_exit(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    _zone.exit();
    _refresh_visual();
}

void Trigger::reset() {
    _zone.reset();
    _refresh_visual();
}

void Trigger::follow(bool source_on) {
    _zone.follow(source_on);
    _refresh_visual();
}

void Trigger::restore(bool triggered) {
    _zone.restore(triggered);
    _refresh_visual();
}

void Trigger::_refresh_visual() {
    if (!_sprite) {
        return;
    }
    // Follows what the player did, not the delayed state traps see.
    _sprite->sprite().set_tiles(_zone.pressed() ? *_tiles_on : *_tiles_off);
}

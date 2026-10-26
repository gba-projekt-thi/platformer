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
    TriggerAction t_action)
    : PhysicsBody(
          t_x,
          t_y,
          t_width,
          t_height,
          Cfg::Layer::TRIGGER,
          Cfg::Layer::PLAYER,
          0),
      _state(t_default_on, t_mode, t_delay_frames),
      _name(t_name),
      _action(t_action) {
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
    return _state.triggered();
}

void Trigger::update() {
    _state.tick();
    // Jump / Dash triggers fire on the duck's action inside the zone.
    const PlayerEvents& events = PlayerEvents::instance();
    if (action_fires(_action, _inside, events.jumped(), events.dashed())) {
        _state.on_enter();
    }
    // A Timer trigger changes its pressed state on its own.
    _refresh_visual();
}

void Trigger::on_enter(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    _inside = true;
    if (_action == TriggerAction::Enter) {
        _state.on_enter();
    }
    _refresh_visual();
}

void Trigger::on_exit(
    [[maybe_unused]] uint16_t hit_layers,
    [[maybe_unused]] StaticBody* body) {
    _inside = false;
    _state.on_exit();
    _refresh_visual();
}

void Trigger::reset() {
    _state.reset();
    _refresh_visual();
}

void Trigger::restore(bool triggered) {
    _state.restore(triggered);
    _refresh_visual();
}

void Trigger::_refresh_visual() {
    if (!_sprite) {
        return;
    }
    // Follows what the player did, not the delayed state traps see.
    _sprite->sprite().set_tiles(_state.pressed() ? *_tiles_on : *_tiles_off);
}

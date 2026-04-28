

#pragma once

#include "bn_sprite_ptr.h"
#include "bn_vector.h"
#include "level.h"
#include "player.h"

class LevelManager {
   public:
    void load(const LevelData& level) {
        level.music.play();

        _back_ground.emplace(level.back_ground.create_bg(0, 0));
        _back_ground.value().set_priority(3);

        // clear old level
        _platforms.clear();
        _platform_bodies.clear();
        _triggers.clear();
        _base_traps.clear();
        _moving_traps.clear();

        // platforms
        for (int i = 0; i < level.platform_count; i++) {
            const PlatformData& p = level.platforms[i];

            auto sprite = level.sprite_item_platform.create_sprite(p.x, p.y);

            sprite.set_tiles(
                level.sprite_item_platform.tiles_item().create_tiles(
                    p.sprite_index));

            _platforms.push_back(bn::move(sprite));

            _platform_bodies.emplace_back(
                p.x, p.y, 16, 16, Player::PLATFORM_LAYER);
        }

        // triggers
        for (int i = 0; i < level.trigger_count; i++) {
            const TriggerData& t = level.triggers[i];

            _triggers.emplace_back(t.x, t.y, t.width, t.height);
        }

        // traps
        for (int i = 0; i < level.trap_count; i++) {
            const TrapData& t = level.traps[i];

            Trigger* trigger = nullptr;

            if (t.trigger_index >= 0 && t.trigger_index < _triggers.size()) {
                trigger = &_triggers[t.trigger_index];
            }

            if (t.type == TrapType::BASE) {
                _base_traps.emplace_back(
                    t.x, t.y, t.width, t.height, *t.sprite, 0);
            } else if (t.type == TrapType::MOVING) {
                _moving_traps.emplace_back(
                    t.x, t.y, t.width, t.height, *t.sprite, 0, t.velocity_x,
                    t.velocity_y, t.max_vel, t.range, trigger);
            }
        }
    }

    void update() {
        for (auto& trap : _base_traps) {
            trap.update();
        }

        for (auto& trap : _moving_traps) {
            trap.update();
        }
    }

   private:
    // TODO: figure out sizes that are reasonable
    bn::vector<bn::sprite_ptr, 32> _platforms;
    bn::vector<StaticBody, 32> _platform_bodies;
    bn::vector<Trigger, 16> _triggers;
    bn::vector<BaseTrap, 16> _base_traps;
    bn::vector<MovingTrap, 16> _moving_traps;
    bn::optional<bn::regular_bg_ptr> _back_ground;
};
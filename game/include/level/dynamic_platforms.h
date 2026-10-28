#pragma once

#include "crumble_logic.h"
#include "switch_logic.h"

// -----------------------------------------------------------------------------
// DynamicPlatforms
//
// The platforms of a level that are not always solid:
//   - switched ones (PlatformData::trigger_name): a trigger turns them on or
//     off (switch_logic::platform_active()),
//   - crumbling ones (PlatformData::crumble_frames): they give way after the
//     duck stood on them and may come back (crumble::State).
// A platform never (re)appears inside the duck - it would be stuck in it - so
// it waits until the duck has stepped clear.
//
// Non-owning: the bodies, sprites and triggers live in LevelManager's
// containers. A template over the game's types, so it is host-tested with
// stand-ins (tests/host/test_dynamic_platforms.cpp):
//   Body:    is_enabled(), enable(), disable(),
//            pos.get_x(), pos.get_y(), pos.width, pos.height
//   Sprite:  enable(), disable(), pos.offset_x, pos.offset_y
//   Trigger: is_triggered()
//   Box (the duck): get_x(), get_y(), width, height
// -----------------------------------------------------------------------------
template <typename Body, typename Sprite, typename Trigger, int Capacity>
class DynamicPlatforms {
   public:
    void clear() {
        _switched_count = 0;
        _crumbling_count = 0;
    }

    // False (and ignored) beyond Capacity.
    auto add_switched(
        Trigger& trigger,
        Body& body,
        Sprite& sprite,
        bool active_when_on) -> bool {
        if (_switched_count >= Capacity) {
            return false;
        }
        _switched[_switched_count++] =
            Switched{&trigger, &body, &sprite, active_when_on};
        return true;
    }

    auto add_crumbling(
        Body& body,
        Sprite& sprite,
        unsigned crumble_frames,
        unsigned respawn_frames) -> bool {
        if (_crumbling_count >= Capacity) {
            return false;
        }
        _crumbling[_crumbling_count++] = Crumbling{
            &body, &sprite, crumble::State(crumble_frames, respawn_frames)};
        return true;
    }

    [[nodiscard]] auto has_crumbling() const -> bool {
        return _crumbling_count > 0;
    }

    // Enables / disables the switched platforms to match their triggers.
    template <typename Box>
    void sync_switched(const Box& duck) {
        for (int i = 0; i < _switched_count; ++i) {
            Switched& switched = _switched[i];
            const bool want = switch_logic::platform_active(
                switched.trigger->is_triggered(), switched.active_when_on);
            if (want == switched.body->is_enabled()) {
                continue;
            }
            if (want) {
                if (_overlaps(duck, *switched.body)) {
                    continue;
                }
                switched.body->enable();
                switched.sprite->enable();
            } else {
                switched.body->disable();
                switched.sprite->disable();
            }
        }
    }

    // One frame of every crumbling platform. stands_on(const Body&) tells
    // whether the duck stands on that body (LevelManager: the bottom probe).
    template <typename Box, typename StandsOn>
    void sync_crumbling(const Box& duck, StandsOn stands_on) {
        for (int i = 0; i < _crumbling_count; ++i) {
            Crumbling& crumbling = _crumbling[i];
            switch (crumbling.state.update(
                stands_on(*crumbling.body), _overlaps(duck, *crumbling.body))) {
                case crumble::Event::Fell:
                    crumbling.body->disable();
                    break;
                case crumble::Event::Vanished:
                    crumbling.sprite->disable();
                    break;
                case crumble::Event::Respawned:
                    crumbling.body->enable();
                    crumbling.sprite->enable();
                    break;
                case crumble::Event::Started:
                case crumble::Event::None:
                default:
                    break;
            }
            crumbling.sprite->pos.offset_x = crumbling.state.shake_offset();
            crumbling.sprite->pos.offset_y = crumbling.state.fall_offset();
        }
    }

    // Death / Restart Level: every crumbling platform untouched again.
    void reset_crumbling() {
        for (int i = 0; i < _crumbling_count; ++i) {
            Crumbling& crumbling = _crumbling[i];
            crumbling.state.reset();
            crumbling.body->enable();
            crumbling.sprite->enable();
            crumbling.sprite->pos.offset_x = 0;
            crumbling.sprite->pos.offset_y = 0;
        }
    }

   private:
    struct Switched {
        Trigger* trigger = nullptr;
        Body* body = nullptr;
        Sprite* sprite = nullptr;
        bool active_when_on = true;
    };

    struct Crumbling {
        Body* body = nullptr;
        Sprite* sprite = nullptr;
        crumble::State state{1, 0};
    };

    template <typename Box>
    static auto _overlaps(const Box& duck, const Body& body) -> bool {
        return switch_logic::boxes_overlap(
            duck.get_x(), duck.get_y(), duck.width, duck.height,
            body.pos.get_x(), body.pos.get_y(), body.pos.width,
            body.pos.height);
    }

    Switched _switched[Capacity];
    Crumbling _crumbling[Capacity];
    int _switched_count = 0;
    int _crumbling_count = 0;
};

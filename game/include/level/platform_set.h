#pragma once

#include <type_traits>
#include <utility>

#include "attempt_objects.h"
#include "dynamic_platforms.h"
#include "fleeing_platforms.h"

// -----------------------------------------------------------------------------
// PlatformSet
//
// The platforms of a level: their bodies and pictures, one of each per
// LevelData::platforms entry in the same order, and the helpers for the ones
// that are not always solid (DynamicPlatforms: switched and crumbling,
// FleeingPlatforms: running away). LevelManager creates the Butano objects and
// feeds this the frames. A template over the game's types, so it is host-tested
// with stand-ins (tests/host/test_platform_set.cpp):
//   Bodies:  container of Body with emplace_back(args...), back(), clear(),
//            operator[]; Body: sprite (pointer), enable(), disable(), see
//            DynamicPlatforms and FleeingPlatforms
//   Sprites: container of owning pointers to Sprite (push_back, clear,
//            operator[]); Sprite: enable(), disable(), see DynamicPlatforms
//   Point:   a stop of a flee path (FleeingPlatforms)
// -----------------------------------------------------------------------------
template <
    typename Bodies,
    typename Sprites,
    typename Trigger,
    typename Point,
    int Capacity>
class PlatformSet {
   public:
    using Body = typename Bodies::value_type;
    using SpritePtr = typename Sprites::value_type;
    using Picture = typename std::remove_reference<
        decltype(*std::declval<SpritePtr&>())>::type;

    // Adds the next platform: its body (made from `body_args`) is linked to
    // the picture, which the set takes over. Returns the body.
    template <typename... BodyArgs>
    auto add(SpritePtr sprite, BodyArgs&&... body_args) -> Body& {
        _bodies.emplace_back(static_cast<BodyArgs&&>(body_args)...);
        Body& body = _bodies.back();
        body.sprite = sprite.get();
        _sprites.push_back(static_cast<SpritePtr&&>(sprite));
        return body;
    }

    // Hands the switched (trigger_name) and crumbling (crumble_frames)
    // platforms to DynamicPlatforms and the running ones (flee_path) to
    // FleeingPlatforms, only those that are there (present(from, until)).
    // trigger_of(platform) gives the trigger of a switched one. Starts both
    // over, so a respawn calls it again after every death.
    template <typename Level, typename Present, typename TriggerOf>
    void
    load_dynamic(const Level& level, Present present, TriggerOf trigger_of) {
        struct Ops {
            PlatformSet& s;
            const Level& level;
            Present& present_fn;
            TriggerOf& trigger_of_fn;
            [[nodiscard]] bool present(unsigned from, unsigned until) const {
                return present_fn(from, until);
            }
            void clear_dynamic() { s._dynamic.clear(); }
            void clear_fleeing() { s._fleeing.clear(); }
            void add_fleeing(int i) {
                const auto& platform = level.platforms[i];
                s._fleeing.add(
                    s._bodies[i], *s._sprites[i], platform.x, platform.y,
                    platform.flee_path.data(), platform.flee_path.size(),
                    platform.flee_range, platform.flee_frames);
            }
            void add_switched(int i) {
                const auto& platform = level.platforms[i];
                s._dynamic.add_switched(
                    trigger_of_fn(platform), s._bodies[i], *s._sprites[i],
                    platform.active_when_on);
            }
            void add_crumbling(int i) {
                const auto& platform = level.platforms[i];
                s._dynamic.add_crumbling(
                    s._bodies[i], *s._sprites[i], platform.crumble_frames,
                    platform.crumble_respawn_frames);
            }
        };
        Ops ops{*this, level, present, trigger_of};
        attempt_objects::load_dynamic(level, ops);
    }

    // Attempt start: platform `i` is there / not there (body and picture).
    void show(int i) {
        _bodies[i].enable();
        _sprites[i]->enable();
    }
    void hide(int i) {
        _bodies[i].disable();
        _sprites[i]->disable();
    }

    // The switched platforms follow their triggers.
    template <typename Box>
    void sync_switched(const Box& duck) {
        _dynamic.sync_switched(duck);
    }

    // One frame of the crumbling platforms, after the collision update.
    // probe_below() says what the duck stands on (an object with count and
    // hits[i].body); it is only asked when there is a crumbling platform.
    template <typename Box, typename ProbeBelow>
    void sync_crumbling(const Box& duck, ProbeBelow probe_below) {
        if (!_dynamic.has_crumbling()) {
            return;
        }
        const auto below = probe_below();
        _dynamic.sync_crumbling(duck, [&below](const Body& body) {
            for (int hit = 0; hit < below.count; ++hit) {
                if (below.hits[hit].body == &body) {
                    return true;
                }
            }
            return false;
        });
    }

    void update_fleeing(bn::fixed duck_x, bn::fixed duck_y) {
        _fleeing.update(duck_x, duck_y);
    }

    // Death / Restart Level: crumbling and running platforms untouched again.
    void reset_crumbling() { _dynamic.reset_crumbling(); }
    void reset_fleeing() { _fleeing.reset(); }

    // Level unload, in the steps level_flow.h gives.
    void clear_dynamic() { _dynamic.clear(); }
    void clear_fleeing() { _fleeing.clear(); }
    void clear_platforms() {
        _sprites.clear();
        _bodies.clear();
    }

   private:
    Bodies _bodies;
    Sprites _sprites;
    DynamicPlatforms<Body, Picture, Trigger, Capacity> _dynamic;
    FleeingPlatforms<Body, Picture, Point, Capacity> _fleeing;
};

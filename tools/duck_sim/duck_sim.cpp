// -----------------------------------------------------------------------------
// duck_sim
//
// The duck's movement for tools/Check_level_reachability.py: the game's own
// locomotion::Core (game/include/player/locomotion.h) and PhysicsBody's move
// (extern/engine/core/include/move_resolution.h), built for the host. The
// Python checker plans the route, this program plays each jump frame by
// frame, so a change to the movement rules or to cfg.h reaches the checks
// without touching Python. Built by `make duck-sim` (build/host-tools/).
//
// Line protocol on stdin / stdout, numbers as decimal text:
//
//   info
//       -> "body_w body_h max_speed edge_margin death_height"
//   scene <door: 0 | 1 x y w h> <n> {x y w h} <n> {x y w h} <n> {x y w h}
//       platforms (solid), pogo targets (traps), forbid boxes; boxes are
//       centre + size. Replaces the previous scene, no answer.
//   spawn <x> <y>
//       -> index of the platform a duck dropped there lands on, or -1
//   run <start> <x0> <vx0> <d> <jump> <hold> <dash_t> <release> <trace>
//       a jump / dash policy from platform <start>: hold direction d (-1 / 1)
//       until frame <release>, A pressed on frame 0 if <jump> and held for
//       <hold> frames, B on frame <dash_t> (-1 = no dash)
//   climb <start> <x0> <vx0> <d> <chimney> <slide> <jump> <trace>
//       the wall-jump policy: slide <slide> frames at each wall, then jump
//       off it; toward the opposite wall if <chimney>, else the same one
//   pogo <start> <x0> <vx0> <d> <r_from> <wall_jump> <trace>
//       jump toward d holding A and strike (R) every other frame from
//       <r_from> on
//       -> "landed door_touched [n x y ...]": the platform index landed on
//          (-1 = none), whether the door was touched on the way, and with
//          <trace> = 1 the duck's centre after every frame
// -----------------------------------------------------------------------------

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bn_fixed.h"

#include "cfg.h"
#include "locomotion.h"
#include "move_resolution.h"

namespace {

using bn::fixed;

struct Box {
    fixed x, y, w, h;
    [[nodiscard]] auto top() const -> fixed { return y - h / 2; }
};

auto overlap(fixed ax, fixed ay, fixed aw, fixed ah, const Box& b) -> bool {
    return ax - aw / 2 < b.x + b.w / 2 && ax + aw / 2 > b.x - b.w / 2 &&
           ay - ah / 2 < b.y + b.h / 2 && ay + ah / 2 > b.y - b.h / 2;
}

// Index of the first box the given one overlaps, or -1.
auto first_hit(
    const std::vector<Box>& boxes,
    fixed x,
    fixed y,
    fixed w,
    fixed h) -> int {
    for (size_t i = 0; i < boxes.size(); ++i) {
        if (overlap(x, y, w, h, boxes[i])) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

constexpr fixed BW = Cfg::Player::BODY_WIDTH;
constexpr fixed BH = Cfg::Player::BODY_HEIGHT;
constexpr int MAX_FRAMES = 160;   // a jump or a dash
constexpr int WALL_FRAMES = 400;  // a climb up a shaft takes longer

struct Scene {
    bool has_door = false;
    Box door{};
    std::vector<Box> platforms, targets, forbid;
};

struct Body {
    struct {
        fixed x = 0;
        fixed y = 0;
    } pos;
    fixed vel_x = 0;
    fixed vel_y = 0;
};

// One duck: the game's Core on a Body, with the collision probes of
// PhysicsBody (1 px outside the box, see physics_body.cpp) over the scene.
class Duck {
   public:
    Duck(const Scene& scene, fixed x, fixed y, fixed vx)
        : _world{scene, body, false, false} {
        body.pos.x = x;
        body.pos.y = y;
        body.vel_x = vx;
        // No world-edge bounce: the checker looks at one jump, not the level
        // border (Check_level_reachability.outside_play_area() does that).
        core.set_horizontal_bound(100000);
    }

    // The ground probe says "yes" for the first frame (a duck that ran off
    // the edge just now still has its coyote time).
    void stand_on_air_first_frame() { _world.ground_first_frame = true; }

    auto step(const InputState& input) -> locomotion::Effects {
        const auto effects = core.step(body, input, _world);
        _world.ground_first_frame = false;
        const Scene& s = _world.scene;
        const AxisMove move_x =
            resolve_axis(body.pos.x, body.vel_x, [&](fixed nx) {
                return first_hit(s.platforms, nx, body.pos.y, BW, BH) >= 0;
            });
        body.pos.x = move_x.position;
        if (move_x.blocked) {
            body.vel_x = 0;
        }
        const AxisMove move_y =
            resolve_axis(body.pos.y, body.vel_y, [&](fixed ny) {
                return first_hit(s.platforms, body.pos.x, ny, BW, BH) >= 0;
            });
        body.pos.y = move_y.position;
        if (move_y.blocked) {
            body.vel_y = 0;
        }
        return effects;
    }

    // -1 / 1 = a wall touches the left / right side, 0 = none.
    [[nodiscard]] auto wall() const -> int {
        if (_world.probe_left()) {
            return -1;
        }
        return _world.probe_right() ? 1 : 0;
    }
    [[nodiscard]] auto ground() const -> int {
        return _world.ground_platform();
    }
    [[nodiscard]] auto dead() const -> bool { return _world.dead; }

    Body body;
    locomotion::Core core;

   private:
    struct World {
        const Scene& scene;
        Body& body;
        bool dead;
        bool ground_first_frame;

        [[nodiscard]] auto ground_platform() const -> int {
            return first_hit(
                scene.platforms, body.pos.x, body.pos.y + BH / 2 + 1, BW - 2,
                1);
        }
        auto probe_ground() -> bool {
            return ground_first_frame || ground_platform() >= 0;
        }
        auto probe_ceiling() -> bool {
            return first_hit(
                       scene.platforms, body.pos.x, body.pos.y - BH / 2 - 1,
                       BW - 2, 1) >= 0;
        }
        [[nodiscard]] auto probe_left() const -> bool {
            return first_hit(
                       scene.platforms, body.pos.x - BW / 2 - 1, body.pos.y, 1,
                       BH - 2) >= 0;
        }
        [[nodiscard]] auto probe_right() const -> bool {
            return first_hit(
                       scene.platforms, body.pos.x + BW / 2 + 1, body.pos.y, 1,
                       BH - 2) >= 0;
        }
        auto probe_pogo() -> bool {
            return first_hit(
                       scene.targets, body.pos.x,
                       body.pos.y + BH / 2 + fixed(Cfg::Pogo::REACH) / 2,
                       Cfg::Pogo::WIDTH, Cfg::Pogo::REACH) >= 0;
        }
        void die() { dead = true; }
    };

    World _world;
};

struct Outcome {
    int landed = -1;
    bool door_touched = false;
    std::vector<std::pair<fixed, fixed>> trace;
};

// What every policy checks after a frame; true = the policy is over.
// `airborne` is set once the duck has left its start platform.
struct Watcher {
    const Scene& scene;
    const Box& start;
    bool want_trace;
    Outcome out;
    bool airborne = false;

    void begin(const Duck& duck) {
        if (want_trace) {
            out.trace.emplace_back(duck.body.pos.x, duck.body.pos.y);
        }
    }

    auto after_frame(const Duck& duck, int frame) -> bool {
        const Body& b = duck.body;
        if (want_trace) {
            out.trace.emplace_back(b.pos.x, b.pos.y);
        }
        if (first_hit(scene.forbid, b.pos.x, b.pos.y, BW, BH) >= 0) {
            return true;
        }
        if (scene.has_door && overlap(b.pos.x, b.pos.y, BW, BH, scene.door)) {
            out.door_touched = true;
        }
        if (duck.dead() || b.pos.y > Cfg::Player::DEATH_HEIGHT) {
            return true;
        }
        const int ground = duck.ground();
        if (ground >= 0 && (&scene.platforms[ground] != &start || airborne) &&
            b.vel_y >= 0 && frame >= 1) {
            out.landed = ground;
            return true;
        }
        if (b.pos.y < start.top() - BH / 2 - 1 || b.vel_y != 0) {
            airborne = true;
        }
        return false;
    }
};

auto run_policy(
    const Scene& scene,
    int start,
    fixed x0,
    fixed vx0,
    int d,
    bool jump,
    int hold,
    int dash_t,
    int release,
    bool trace) -> Outcome {
    const Box& from = scene.platforms[start];
    Duck duck(scene, x0, from.top() - BH / 2, vx0);
    duck.stand_on_air_first_frame();
    duck.core.set_dash_enabled(dash_t >= 0);
    Watcher watch{scene, from, trace, {}};
    watch.begin(duck);
    for (int f = 0; f < MAX_FRAMES; ++f) {
        InputState in;
        in.left_held = d < 0 && f < release;
        in.right_held = d > 0 && f < release;
        in.a_pressed = jump && f == 0;
        in.a_held = jump && f < hold;
        in.b_pressed = f == dash_t;
        duck.step(in);
        if (watch.after_frame(duck, f)) {
            break;
        }
    }
    return watch.out;
}

auto climb_policy(
    const Scene& scene,
    int start,
    fixed x0,
    fixed vx0,
    int d,
    bool chimney,
    int slide,
    bool jump,
    bool trace) -> Outcome {
    const Box& from = scene.platforms[start];
    Duck duck(scene, x0, from.top() - BH / 2, vx0);
    duck.core.set_wall_jump_enabled(true);
    Watcher watch{scene, from, trace, {}};
    watch.begin(duck);
    int touching = 0;
    for (int f = 0; f < WALL_FRAMES; ++f) {
        touching =
            duck.wall() == d && !duck.core.on_ground() ? touching + 1 : 0;
        InputState in;
        in.left_held = d < 0;
        in.right_held = d > 0;
        in.a_held = touching != slide + 1;
        in.a_pressed = (jump && f == 0) || touching == slide + 2;
        const auto effects = duck.step(in);
        if (effects.wall_jumped) {
            touching = 0;
            if (chimney) {
                d = -d;
            }
        }
        if (watch.after_frame(duck, f)) {
            break;
        }
    }
    return watch.out;
}

auto pogo_policy(
    const Scene& scene,
    int start,
    fixed x0,
    fixed vx0,
    int d,
    int r_from,
    bool wall_jump,
    bool trace) -> Outcome {
    const Box& from = scene.platforms[start];
    Duck duck(scene, x0, from.top() - BH / 2, vx0);
    duck.core.set_wall_jump_enabled(wall_jump);
    duck.core.set_pogo_enabled(true);
    Watcher watch{scene, from, trace, {}};
    watch.begin(duck);
    int bounces = 0;
    for (int f = 0; f < WALL_FRAMES; ++f) {
        if (bounces > 4) {  // hopping in place on one trap leads nowhere
            break;
        }
        InputState in;
        in.left_held = d < 0;
        in.right_held = d > 0;
        in.a_pressed = f == 0;
        in.a_held = true;
        in.r_pressed = f >= r_from && (f - r_from) % 2 == 0;
        bounces += duck.step(in).pogo_bounced;
        // Touching a trap with the body is death.
        if (first_hit(
                scene.targets, duck.body.pos.x, duck.body.pos.y, BW, BH) >= 0) {
            break;
        }
        if (watch.after_frame(duck, f)) {
            break;
        }
    }
    return watch.out;
}

// The platform a duck dropped at (x, y) lands on, -1 if it falls to its death.
auto spawn_platform(const Scene& scene, fixed x, fixed y) -> int {
    Duck duck(scene, x, y, 0);
    for (int f = 0; f < 300; ++f) {
        if (duck.ground() >= 0) {
            return duck.ground();
        }
        duck.step({});
        if (duck.dead()) {
            return -1;
        }
    }
    return -1;
}

auto read_boxes(std::istream& in) -> std::vector<Box> {
    int n = 0;
    in >> n;
    std::vector<Box> boxes(n);
    for (Box& b : boxes) {
        double x, y, w, h;
        in >> x >> y >> w >> h;
        b = {fixed(x), fixed(y), fixed(w), fixed(h)};
    }
    return boxes;
}

void print(const Outcome& out, bool trace) {
    std::printf("%d %d", out.landed, out.door_touched ? 1 : 0);
    if (trace) {
        std::printf(" %zu", out.trace.size());
        for (const auto& [x, y] : out.trace) {
            std::printf(
                " %.6f %.6f", static_cast<double>(x), static_cast<double>(y));
        }
    }
    std::printf("\n");
}

}  // namespace

int main() {
    Scene scene;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        if (cmd == "info") {
            std::printf(
                "%d %d %d %d %d\n", Cfg::Player::BODY_WIDTH,
                Cfg::Player::BODY_HEIGHT, Cfg::Player::MAX_SPEED,
                Cfg::Screen::EDGE_MARGIN, Cfg::Player::DEATH_HEIGHT);
        } else if (cmd == "scene") {
            int has_door = 0;
            in >> has_door;
            scene = Scene{};
            scene.has_door = has_door != 0;
            if (scene.has_door) {
                double x, y, w, h;
                in >> x >> y >> w >> h;
                scene.door = {fixed(x), fixed(y), fixed(w), fixed(h)};
            }
            scene.platforms = read_boxes(in);
            scene.targets = read_boxes(in);
            scene.forbid = read_boxes(in);
            continue;  // no answer
        } else if (cmd == "spawn") {
            double x, y;
            in >> x >> y;
            std::printf("%d\n", spawn_platform(scene, fixed(x), fixed(y)));
        } else if (cmd == "run") {
            int start, d, jump, hold, dash_t, release, trace;
            double x0, vx0;
            in >> start >> x0 >> vx0 >> d >> jump >> hold >> dash_t >>
                release >> trace;
            print(
                run_policy(
                    scene, start, fixed(x0), fixed(vx0), d, jump != 0, hold,
                    dash_t, release, trace != 0),
                trace != 0);
        } else if (cmd == "climb") {
            int start, d, chimney, slide, jump, trace;
            double x0, vx0;
            in >> start >> x0 >> vx0 >> d >> chimney >> slide >> jump >> trace;
            print(
                climb_policy(
                    scene, start, fixed(x0), fixed(vx0), d, chimney != 0, slide,
                    jump != 0, trace != 0),
                trace != 0);
        } else if (cmd == "pogo") {
            int start, d, r_from, wall_jump, trace;
            double x0, vx0;
            in >> start >> x0 >> vx0 >> d >> r_from >> wall_jump >> trace;
            print(
                pogo_policy(
                    scene, start, fixed(x0), fixed(vx0), d, r_from,
                    wall_jump != 0, trace != 0),
                trace != 0);
        } else {
            std::printf("error unknown command\n");
        }
        std::fflush(stdout);
    }
    return 0;
}

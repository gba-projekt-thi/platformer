# Glossary

Definitions of project-specific and domain terms used throughout the
documentation. Terms are defined conceptually; see the linked page for the full
treatment.

## A

**Accelerometer** — *not used.* (Listed here only to disambiguate: movement is
input-driven, not motion-sensor-driven.)

**Animator (Player)** — The player component responsible for selecting and
advancing sprite frames based on movement state and facing. See
[Components — Player System](components.md).

## B

**Base trap** — A static, always-dangerous hazard. The simplest trap category;
can still animate its sprite while remaining motionless. See
[Components — Trap System](components.md).

**Blending / fade** — The engine's screen-fade feature used for smooth scene
transitions. See [System Internals](system-internals.md).

**Bounce edge** — The horizontal boundary (per level) at which the duck stops
and cannot travel further, derived from the world's width. See
[Components — Level Management](components.md).

**Butano** — The open-source Game Boy Advance engine this project is built on.
See [Getting Started](getting-started.md).

## C

**Camera** — The window into a world. Static for screen-sized worlds; follows
and clamps to the edges for wider (scrolling) worlds. See
[Components — Level Management](components.md).

**Chase trap** — A hazard that trails the player and closes in as they advance,
but never retreats; punishes standing still. See
[Components — Trap System](components.md).

**Collision layer** — A bitflag category (player, trap, platform, trigger, door)
a body belongs to and reacts to, letting the collision system route interactions
without testing every pair. See [Project Architecture](architecture.md).

**Coyote time** — A few grace frames after leaving a ledge during which a jump
is still allowed. A forgiving-control mechanic. See
[Game Concepts](game-concepts.md).

**Core scene manager** — The shared-engine service that creates, swaps, and
destroys scenes. See [Architecture](architecture.md).

## D

**Data manager** — Owns the runtime copy of the game state, the active save
slot, and load/save/reset operations. See [Architecture](architecture.md).

**Data-driven level** — A stage described by declarative data (platforms,
triggers, traps, spawn, door, world size, background, music) rather than
hard-coded logic. See [Level Design](level-design.md).

**Death height** — The vertical threshold below which the duck is considered
dead (fell off the world). See [Components — Player System](components.md).

**Door** — The exit object that, when the duck reaches it, advances the game to
the next stage. See [Components — Level Management](components.md).

## E

**Envelope (physics)** — The maximum vertical climb and horizontal distance a
fully-held jump can cover; level spacing is authored inside a safe margin of
this envelope. See [Level Design](level-design.md).

**Engine team layer** — The shared engine code (`extern/engine/`) that sits
between the game and Butano, providing scene management, physics, save, etc.
See [Architecture](architecture.md).

## F

**Facing** — The direction the duck is oriented (forward, back, left, right),
feeding animation selection. See [Components — Player System](components.md).

**Fixed-capacity container** — A collection with a compile-time maximum size,
used for per-level objects to avoid heap fragmentation on a memory-constrained
device. See [System Internals](system-internals.md).

**Fixed-point** — Integer-based representation of fractional values, used
because the hardware lacks a fast floating-point unit. See
[System Internals](system-internals.md).

## G

**Game state** — The small, fixed-width persistent record (level, deaths,
timer) stored per save slot. See [Architecture](architecture.md).

## I

**IResettable / resettable** — An interface entities implement to be reset by
the level manager on death/restart, decoupling the manager from concrete types.
See [Architecture](architecture.md).

## J

**Jump buffering** — A few frames before landing during which a jump press is
remembered and executed on contact. A forgiving-control mechanic. See
[Game Concepts](game-concepts.md).

## K

**Kiss scene** — The celebratory victory scene played after the final stage.
See [Game Concepts](game-concepts.md).

## L

**Level manager** — The runtime owner of the current stage: loads it, steps it,
resets entities, unloads it, and reports outcomes to the scene. See
[Components — Level Management](components.md).

**Layer (collision)** — see *Collision layer*.

## M

**Mailmap** — A Git file normalizing commit authorship identities across
contributors. See [Development Workflow](development-workflow.md).

**Mimic** — An animated disguised-hazard enemy (dungeon theme) that stays
motionless while cycling animation frames. See
[Components — Trap System](components.md).

**Moving trap** — A trigger-activated hazard driven by per-frame acceleration up
to a max velocity. See [Components — Trap System](components.md).

## P

**Path trap** — A hazard that interpolates along a defined route of relative
waypoints; used for patrols and figure-8 motion. See
[Components — Trap System](components.md).

**Pause controller** — Handles pause toggling and the pause menu (continue /
restart / title). See [Components](components.md).

**Physics body** — The shared engine concept giving an entity position, motion,
and collision-layer participation. Used by player, traps, triggers, platforms,
and the door. See [Architecture](architecture.md).

**Player entity** — The duck, composed of locomotion, animator, state machine,
HUD, and sprite. See [Components — Player System](components.md).

**Pre-commit hooks** — Automated style (clang-format) and lint (clang-tidy)
checks run before each commit. See
[Development Workflow](development-workflow.md).

## R

**ROM** — The compiled game image that runs on the emulator or hardware.

**Resettable** — see *IResettable*.

**Run timer** — The on-screen minutes:seconds:centiseconds counter for the
session, owned by the player HUD. See
[Components — Player System](components.md).

## S

**Save slot** — One of three independent persistence slots selectable on the
start screen. See [Game Concepts](game-concepts.md).

**Save sync controller** — Owns the *when-to-save* policy; commits deaths/timer
to SRAM only on meaningful changes (or when forced). See
[Architecture](architecture.md).

**SRAM** — Battery-backed save memory; writes are slow and limited, motivating
the save-sync policy. See [System Internals](system-internals.md).

**Sprite item / background item** — Engine-side asset templates; on-screen
instances are created from them at load and released at unload. See
[Asset Management](asset-management.md).

**State machine (Player)** — Tracks coarse movement state (idle / run / jump /
fall). See [Components — Player System](components.md).

## T

**Tracker module** — Compact pattern-based music format played back efficiently
by the handheld audio hardware. See [Asset Management](asset-management.md).

**Trigger** — An invisible rectangular zone that activates linked hazards when
the duck enters it; may start active or inactive. See
[Level Design](level-design.md).

**Trap factory** — The mechanism that constructs the correct trap subtype from a
stage-data entry. See [Components — Trap System](components.md).

**Trap** — Any hazard that kills the duck on contact. See
[Components — Trap System](components.md).

## V

**Variable jump height** — Jumping higher the longer the jump button is held.
A forgiving-control mechanic. See [Game Concepts](game-concepts.md).

## W

**World** — A themed group of stages (ocean, factory, forest, garden, dungeon).
See [Game Concepts](game-concepts.md).

**World size** — The extents of a stage's playable area; larger than the screen
enables scrolling and sets the duck's bounce edges. See
[Components — Level Management](components.md).

## Related
- [Index](index.md)
- [Project Architecture](architecture.md)
- [Game Concepts](game-concepts.md)
- [Components](components.md)
- [Level Design](level-design.md)
- [Asset Management](asset-management.md)
- [Development Workflow](development-workflow.md)
- [System Internals](system-internals.md)
- [Extensibility Guide](extensibility.md)
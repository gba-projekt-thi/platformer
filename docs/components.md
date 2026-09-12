# Components

This page gives a **conceptual deep-dive** into the three largest gameplay
systems: the **Player System**, the **Trap System**, and **Level Management**.
Each section describes responsibilities, states, and interactions — not code.

For the high-level view of how these fit together, see
[Project Architecture](architecture.md).

---

## 5.1 Player System

The player is the duck. It is built by **composition**: a top-level entity that
owns several focused collaborators. Each collaborator has one job, which keeps
the system easy to reason about and extend.

```mermaid
graph TD
    Player[Player entity] --> Sprite[Sprite<br/>visible duck]
    Player --> Loc[Locomotion<br/>physics + input]
    Player --> Anim[Animator<br/>frame selection]
    Player --> SM[State Machine<br/>Idle/Run/Jump/Fall]
    Player --> HUD[HUD<br/>deaths + timer]
    Loc --> Body[Physics Body<br/>collision, position]
    Loc -. death callback .-> Player
```

### Player states & capabilities

The state machine tracks a small set of coarse movement states that the
animator and locomotion react to:

| State | Meaning | Typical animation |
|-------|---------|-------------------|
| **Idle** | Standing still, on the ground. | A static/idle frame (or facing-back when not moving). |
| **Run** | Moving horizontally on the ground. | Alternating left/right walk frames. |
| **Jump** | Rising after a jump. | Ascent frames matched to facing direction. |
| **Fall** | Descending (not jumping). | Descent frames matched to facing direction. |

The locomotion component also tracks a **facing** direction (forward, back,
left, right) that feeds the animator and lets the duck turn to look at the
camera when idle.

### Movement mechanics

Locomotion owns the per-frame physics step, performed in a deliberate order so
that collisions, ground detection, and death checks all happen consistently
within a single frame:

1. **Horizontal input** — accelerate left/right up to the max speed, applying
   the configured acceleration per frame.
2. **Jump handling** — including **coyote time** (a few grace frames after
   leaving a ledge) and **jump buffering** (a few frames before landing).
3. **Gravity** — applied every frame to build downward velocity.
4. **Variable jump** — releasing the jump button early shortens the ascent.
5. **Velocity clamping** — keeps the horizontal and fall speeds within the
   configured caps.
6. **Bounds** — the duck bounces off the world's left/right edges (the boundary
   is set per level by the level manager, based on the world width).
7. **Ground state update** — queries the collision system to determine whether
   the duck is currently resting on a platform.
8. **Death check** — if the duck falls below a configured death height, a death
   is raised.

The ground state and facing direction are then made available to the animator
and state machine so they can pick the correct frames for this tick.

### Jump physics principles

The jump is not a single impulse; it is a small set of cooperating rules that
together give the duck a responsive, controllable leap:

- The duck receives an upward velocity when a jump begins.
- The jump is **height-controllable**: holding the jump button extends the
  ascent, releasing early shortens it.
- **Gravity** constantly pulls the duck down, and a **max fall speed** keeps
  descents predictable.
- **Coyote time** and **jump buffering** forgive small input timing errors,
  which is essential for tight platform layouts to feel fair.

The exact tuning values (acceleration, max speed, jump speed, gravity, fall
speed clamp, coyote/buffer frames, death height) live in a central
configuration so designers can tune the feel in one place and have it propagate
to every stage.

### Death & respawn system

Death is simple and forgiving:

```mermaid
stateDiagram-v2
    [*] --> Alive
    Alive --> Dead: touch a trap or fall past death height
    Dead --> RespawnDelay: brief pause
    RespawnDelay --> Alive: teleport to spawn, reset velocity
```

- A **trap collision** or **falling past the death height** triggers a death.
- The duck briefly pauses (so the player registers what happened), then
  teleports back to the stage's **spawn point** with velocity reset.
- The **death counter** increments, and the **save-sync** system persists the
  updated total.
- The level manager uses a **generic reset interface** to put every trap back to
  its starting state, so a respawn always faces the same hazard layout.

The locomotion component does **not** own respawn logic itself. Instead it
raises a death through a small **callback interface**, and the player entity
handles the rest — keeping locomotion focused purely on movement.

### HUD components

The player owns an HUD consisting of:

- **Death counter** — a numeric readout that increments on each respawn.
- **Run timer** — minutes, seconds, centiseconds, driven by a frame counter
  tuned for the handheld's refresh rate and avoiding expensive division.

The HUD exposes a **visibility toggle** so title screens and the kiss scene can
hide it without destroying it. Timer digits are only updated when their value
actually changes, to avoid redundant work each frame.

---

## 5.2 Trap System

Traps are the hazards the duck must avoid. They share a **base trap** for the
concerns every trap has and specialize in behavior through subclasses chosen by
a **trap factory** at level load.

```mermaid
classDiagram
    PhysicsBody
    IResettable
    BaseTrap --> PhysicsBody
    BaseTrap ..|> IResettable
    MovingTrap --|> BaseTrap
    PathTrap --|> BaseTrap
    ChaserTrap --|> BaseTrap
```

### Base responsibilities

The base trap owns:

- A **collision volume** linked to a sprite, with a position, size, and offset.
- **Collision with the player** — when the duck enters the trap's collision
  area, the base trap kills the duck and plays the appropriate hit sound.
- **Optional sprite animation** — a looping animation action is attached when
  the level data specifies an animation frame sequence.
- **Sprite registration** with the engine's sprite system, so the trap's pixels
  are properly managed for the current scene.

### Trap categories & purposes

| Category | Activation | Movement | Design purpose |
|----------|-----------|----------|----------------|
| **Base** | Always active | None | Static obstacles that punish careless landing. |
| **Moving** | Trigger-linked | Acceleration-driven once triggered | Timed hazards released mid-run. |
| **Path** | Trigger-linked | Interpolates along defined waypoints | Patrols, sweeping hazards, figure-8 enemies. |
| **Chase** | Watches the player | Moves to close the gap when the player advances | Pressure hazard that punishes hesitation. |

### Trap behaviors & characteristics

- **Base traps** stay exactly where they are placed. They can still animate
  their sprite (e.g. a mimic breathing) while remaining motionless as a hazard.
- **Moving traps** start dormant. When their linked **trigger** fires, the trap
  begins accelerating in a configured direction up to a maximum velocity. Once
  the duck respawns, the trap returns to its start position and re-arms.
- **Path traps** follow a route defined by waypoints relative to their origin.
  They interpolate smoothly between nodes, waiting a configured number of
  frames on each step. This creates patrols and other repeating trajectories.
- **Chase traps** bypass normal physics entirely. They watch the player's
  position and drift rightward to keep a configured distance behind them, but
  only up to a maximum speed. Because they only ever move right, they never
  retreat — the duck must keep moving forward to stay ahead.

### Activation mechanisms

Two activation styles exist:

- **Trigger-linked activation.** Moving and path traps reference a trigger by
  index. When the duck crosses into the trigger's invisible rectangle, the
  trigger flips "on" and any linked trap begins its behavior.
- **Direct player tracking.** Chase traps read the player's position every
  frame and update their own position accordingly.

The default trigger index for a trap can point to a fallback (index `-1`),
letting a trap respond to a stage-level default rather than a specific zone.

### Interactions with the player

Traps and the player meet through the **collision layer system**. Traps belong
to a **trap layer** and the player subscribes to it; when the player's body
overlaps a trap, the trap's collision handler kills the player. This same
layered approach is what lets platforms block the player, triggers sense the
player without being touched back, and doors signal completion when entered —
all from the same underlying collision framework.

### Reset model

Every trap implements the shared **resettable** interface. On player death or a
manual restart, the level manager walks its resettable list and calls reset on
each. Traps return to their start positions, re-arm, and (for chase traps)
forget their tracking history. This keeps respawn deterministic: the same stage
layout faces the player each time.

---

## 5.3 Level Management

The **level manager** is the stage's runtime container. It owns everything that
exists *during a stage* and nothing that persists across scenes.

```mermaid
graph TD
    LM[Level Manager]
    LM --> Platforms[Platforms<br/>collision + tileset]
    LM --> Triggers[Triggers<br/>invisible zones]
    LM --> Traps[Traps<br/>hazards]
    LM --> Door[Door<br/>exit]
    LM --> Background[Background<br/>tilemap]
    LM --> Camera[Camera<br/>bounds]
    LM --> Music[Music<br/>track]
    LM --> Pause[Pause controller]
    LM --> Sync[Save sync controller]
```

### Level lifecycle

A stage goes through four phases:

1. **Load.** The manager receives the stage's declarative data and builds the
   runtime world: place the platform sprites and register their collision
   bodies; create the triggers; construct each trap through the factory and add
   it to both the trap list and the resettable list; place the door; set the
   music and background; configure the camera bounds and horizontal bounce
   boundary; set the player's spawn point; establish the save-sync baseline.
2. **Run (per frame).** Each frame the manager updates the pause controller,
   the player, the traps, the door, and the save-sync controller, then reports
   an outcome to the calling scene:
   - **None** — keep running (or paused).
   - **LevelComplete** — the door was reached; advance.
   - **ReturnToTitle** — the player chose "Title" from the pause menu.
3. **Reset (optional).** On player death or a manual restart, the manager walks
   its resettable list, restoring traps to their initial states.
4. **Unload.** When leaving the stage, the manager releases every resource —
   sprites, backgrounds, music — so the next scene starts with a clean pool.

### Scene transitions

The level manager does not directly switch scenes. It reports an outcome to the
level **scene**, and the scene asks the **core scene manager** to move to the
next scene. The scene manager guarantees the previous stage is torn down before
the next one initializes, which is critical on a device with limited shared
resources (palettes, sprite slots, VRAM).

### Level data organization

A stage is defined by immutable, ROM-resident data describing:

- The **platform list** — position, size, sprite, and tile index for each
  solid surface.
- The **trigger list** — invisible rectangles that activate hazards.
- The **trap list** — one entry per hazard with its type, sprite, animation,
  activation, and behavior-specific parameters.
- The **background** tilemap, the **music** track, the player's **spawn point**,
  and the **door** position.
- The **world dimensions** — a wider world enables real camera scrolling;
  screen-sized values keep the camera effectively fixed.

Keeping stage contents as static data means designers can add or tune stages
without touching engine or gameplay code paths — see
[Extensibility Guide](extensibility.md).

### Camera systems

The camera is configured per stage from its world dimensions. When the world
matches the screen size, the camera is effectively static and the duck moves
within the visible area. When the world is wider, the camera follows the duck
and clamps to the world's edges. The same world-width value also sets the
duck's horizontal bounce boundary, so the duck never leaves the intended play
area.

### Resource management

The level manager holds its collections in **fixed-capacity** containers sized
to the maximum the game expects. This choice avoids heap fragmentation on a
device where memory is precious and long play sessions must remain stable. On
unload, every resource is explicitly released before the next stage loads, so
the next stage's assets can claim the same sprite slots and palettes without
contention.

---

## Related
- [Index](index.md)
- [Project Architecture](architecture.md)
- [Level Design](level-design.md)
- [Asset Management](asset-management.md)
- [Extensibility Guide](extensibility.md)
- [Glossary](glossary.md)
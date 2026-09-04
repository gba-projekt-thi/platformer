# Architecture overview

This document describes the current structure of the game project and the design direction behind the refactor.

## Core goals of the refactor

The project has been moving away from a single large, mixed bootstrap and toward a more explicit runtime model:

- explicit runtime ownership instead of hidden global state
- scene-based progression instead of a single large update flow
- save and runtime state separated cleanly
- per-level definitions split into smaller reusable files
- trap logic split by behavior type

## Runtime model

The central runtime entry point is `GameRuntime`.

Responsibilities:
- initialize the Butano engine
- configure the scene manager and blending defaults
- construct the runtime-owned objects
- start the first scene
- keep the main loop running until the session ends

The runtime owns the live game objects directly, including:
- `DataManager`
- `Player`
- `LevelManager`
- `GameSession`

This avoids scene-to-scene hidden state and reduces the “magic singleton” pattern.

## Scene lifecycle

The game flow is organized as scenes:

### StartScene

Used before gameplay begins. It is responsible for:
- preparing the title screen and save slots
- handling a selected save slot
- loading persisted progress if present
- transitioning to the actual gameplay scene

### LevelScene

Handles the active level loop. It is responsible for:
- loading the active level when the scene initializes
- checking level completion
- saving player progress before the next level
- advancing to the next level or finishing the game

The transition logic is intentionally guarded to prevent duplicate scene changes.

## State ownership

### Session state

The game completion state is represented by `GameSession` and passed to scenes by reference.

This replaces the previous pattern of a global mutable flag and makes completion status explicit and easy to reason about.

### Save state

`DataManager` owns runtime save state and performs the persistence boundary.

It is responsible for:
- slot normalization
- loading the selected slot from SRAM
- saving the currently shown runtime state
- clearing state during reset

This keeps save logic separated from game logic and scene logic.

## Level system

Levels are stored in reusable per-level headers under the `game/include/levels/` folder.

A shared schema in `level_structure.h` defines the common level data:
- player spawn point
- door position
- background and music
- platform placement
- trigger volume and position
- trap definitions

This keeps the data model consistent across all levels while allowing each level to be defined separately.

## Gameplay systems

### Player

The player owns movement, collision, input handling, and animation.

It currently manages:
- input-driven horizontal movement
- jump buffering and coyote-time behavior
- gravity and variable jump height
- death and respawn transitions
- timer and death counter HUD updates

### LevelManager

This class owns runtime setup for level content and the active scene environment.

It is responsible for:
- creating platforms and bodies
- loading traps and triggers
- resetting trap state after a death
- synchronizing runtime save data
- updating pause/level state during the frame loop

### Traps

Trap behavior is split by specialization:

- BaseTrap: shared collision and animation logic
- MovingTrap: acceleration-based hazard triggered by event criteria
- PathTrap: motion based on interpolation through a fixed path

This separation keeps logic specific to the trap behavior in the correct place instead of putting all trap logic into one large class.

## Asset layout

The asset layout is organized to match the build system and the current project needs:

- `game/assets/`: canonical asset paths used by the build
- `game/audio/`: music and sound effects
- `game/dmg_audio/`: DMG-friendly audio assets
- `game/graphics/`: sprite and background graphics sources

This gives a clearer relationship between generated asset files and the code that consumes them.

## Why this architecture is healthier

The current structure makes the project easier to maintain because it follows a few clear rules:

1. state is explicit and owned by the right object
2. scenes do not hide global mutable state
3. runtime setup is centralized
4. level data is decomposed into focused definitions
5. gameplay systems are separated by behavior and responsibility

## Next improvement areas

The remaining optional cleanup targets are mostly about refinement rather than game behavior changes:

- further split of large gameplay objects if they keep growing
- clearer utility boundaries for frequently reused update logic
- continued documentation updates as systems grow

The goal is not a rewrite, but a stable and readable architecture that remains easy to extend.

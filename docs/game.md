# Game project documentation

This document describes the current platformer project and the modular structure that has evolved around it.

## Overview

The project is a Game Boy Advance platformer built with Butano. The game logic is organized around a small runtime lifecycle, scene-based flow, and explicit data ownership instead of a single oversized bootstrap.

The current design separates responsibilities across:
- runtime startup and loop control
- scene orchestration
- data persistence
- level definitions and runtime loading
- player behavior
- trap behavior

## Build and run

From the repository root:

```bash
make
mgba-qt platformer.gba
```

For automated verification:

```bash
./test-rom.sh platformer.gba
```

## Project layout

```text
game/
├── assets/                  # Canonical asset directories
├── audio/                   # Music and game sounds
├── dmg_audio/               # DMG audio assets
├── graphics/                # Sprite and background graphics
├── include/                 # Game headers
│   ├── levels/              # Per-level definitions
│   ├── levels.h             # Aggregated level list
│   ├── level_structure.h    # Shared level schema and definitions
│   ├── game_runtime.h       # Runtime bootstrap and lifecycle
│   ├── game_session.h       # Game completion/session state
│   ├── data_manager.h       # Save data management
│   ├── level_manager.h      # Runtime level setup and updates
│   ├── start_scene.h        # Start/save menu scene
│   ├── level_scene.h        # Gameplay scene
│   ├── player.h             # Player movement and animation
│   ├── trap_factory.h       # Trap factory creation logic
│   ├── moving_trap.h        # Trigger-driven moving hazard
│   ├── path_trap.h          # Path-following hazard
│   ├── base_trap.h          # Common trap behavior
│   └── ...
└── src/
    ├── traps/               # Trap implementations
    ├── game_runtime.cpp     # Runtime orchestration
    ├── level_scene.cpp      # Level lifecycle and transitions
    ├── start_scene.cpp      # Save slot start menu
    ├── level_manager.cpp    # Level data loading and updates
    ├── player.cpp           # Input/physics/animation
    ├── data_manager.cpp     # Save load/save/reset logic
    └── main.cpp             # Entry point for the game
```

## Runtime architecture

### GameRuntime

The game is initialized through the runtime abstraction in [game/include/game_runtime.h](../game/include/game_runtime.h) and [game/src/game_runtime.cpp](../game/src/game_runtime.cpp).

Responsibilities:
- initialize the Butano engine
- configure scene settings and blending defaults
- create the first scene
- keep the main game loop running until the session ends
- reset save state after the game ends

### GameSession

The game completion state is no longer a global mutable flag. It is represented by [game/include/game_session.h](../game/include/game_session.h), and scenes receive the shared session by reference.

This reduces hidden state and makes finishing logic explicit.

### DataManager

Save/load behavior is coordinated in [game/include/data_manager.h](../game/include/data_manager.h) and [game/src/data_manager.cpp](../game/src/data_manager.cpp).

Responsibilities:
- manage the active save slot
- keep runtime game state in memory
- load from SRAM when a save is selected
- save the current state when level progress changes
- reset runtime state cleanly

## Scene flow

### StartScene

The start menu is implemented in [game/include/start_scene.h](../game/include/start_scene.h) and [game/src/start_scene.cpp](../game/src/start_scene.cpp).

It handles:
- title rendering
- save-slot selection
- slot-specific state load
- transition into the first playable level scene

### LevelScene

Gameplay progression is managed in [game/include/level_scene.h](../game/include/level_scene.h) and [game/src/level_scene.cpp](../game/src/level_scene.cpp).

Responsibilities:
- load the active level
- poll the level manager for completion
- save progress before moving to the next level
- advance the scene or finish the game
- keep transition requests single-shot and safe

## Level system

Level data is split into reusable chunks under [game/include/levels](../game/include/levels).

Each level defines:
- player spawn coordinate
- door position and exit state
- background image and music
- platforms
- triggers
- traps

The shared schema is defined in [game/include/level_structure.h](../game/include/level_structure.h), while the aggregated level list lives in [game/include/levels.h](../game/include/levels.h).

## Player system

The player is implemented in [game/include/player.h](../game/include/player.h) and [game/src/player.cpp](../game/src/player.cpp).

It currently handles:
- horizontal input and movement
- jump buffering and variable jump height
- gravity and fall speed clamping
- collision boundaries and death checks
- animation state management
- timer and death HUD updates

## Level management

The level manager in [game/include/level_manager.h](../game/include/level_manager.h) and [game/src/level_manager.cpp](../game/src/level_manager.cpp) owns runtime level setup.

Responsibilities:
- load a level into the scene
- create all platforms and traps
- reset trap state on death
- sync save data when player deaths occur
- update pause state and collision system

## Trap system

The trap layer has been separated into three main components:
- [game/include/base_trap.h](../game/include/base_trap.h)
- [game/include/moving_trap.h](../game/include/moving_trap.h)
- [game/include/path_trap.h](../game/include/path_trap.h)

and their implementations in [game/src/traps](../game/src/traps).

These classes handle:
- shared collision and animation behavior
- acceleration-based hazards after trigger activation
- path-following movement synchronized to trigger events
- reset behavior after death or level restart

## Game data and assets

The asset layout has been cleaned up to reflect the current project structure:

- [game/assets](../game/assets) contains canonical generated asset paths used by the build
- [game/audio](../game/audio) contains music and sound effects
- [game/dmg_audio](../game/dmg_audio) contains DMG-compatible audio assets
- [game/graphics](../game/graphics) contains sprite and background data

## Extending the project

### Add a new level

1. Create a new per-level header under [game/include/levels](../game/include/levels)
2. Define the shared level data using the structures from [game/include/level_structure.h](../game/include/level_structure.h)
3. Include the new level in [game/include/levels.h](../game/include/levels.h)
4. Add it to the scene’s level array if needed

### Add a new trap type

1. Define a new trap class near the other trap types
2. Add the creation path in [game/include/trap_factory.h](../game/include/trap_factory.h)
3. Ensure the trap resets correctly when the player dies or the level restarts

### Add a new scene

1. Create a new scene class derived from the core scene abstraction
2. Register it through the runtime or scene manager
3. Keep state flow explicit through constructor injection rather than global state

## Notes

This project is intentionally being refactored toward a clearer structure: smaller modules, explicit state ownership, and less dependence on global mutable state. The runtime and scene architecture are the main improvement areas, while the gameplay systems remain intentionally stable and build-safe.

## Related documentation

- [README.md](../README.md)
- [docs/architecture.md](architecture.md)

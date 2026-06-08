# Game Documentation

This document describes the `game/` project inside this repository, including the folder layout, gameplay systems, and how to build or extend the GBA platformer.

## Overview

This repository contains a small Game Boy Advance platformer built with the Butano engine. The playable game lives in `game/` and uses global engine support from `extern/engine/`.

The game includes:
- A start/save selection scene
- Multiple levels defined in code
- Static and moving traps
- Trigger zones that activate hazards
- A door that advances the player to the next level
- Music and tilemap backgrounds
- A death counter HUD

## Build and Run

Build the game from the repository root inside the devcontainer:

```bash
make
```

This produces the ROM file:

```bash
platformer.gba
```

Run the ROM with mGBA on your host machine:

```bash
mgba-qt platformer.gba
```

For headless or CI testing, use:

```bash
./test-rom.sh platformer.gba
```

## Folder Structure

`game/`
- `audio/` — maxmod audio assets used by the game
- `dmg_audio/` — DMG audio assets for older sound support
- `graphics/` — sprite and background graphics source files
- `include/` — game headers, level definitions, and shared data structures
- `src/` — game logic implementation files

### Important files

- `game/src/main.cpp`
  - Initializes the Butano engine
  - Configures fade and global settings
  - Creates the player entity
  - Starts the initial `StartScene`
  - Runs the main game loop

- `game/include/levels.h`
  - Contains the level definitions, platform placement, trigger zones, and trap layout
  - Defines the tilemap background and music for each level
  - Defines player spawn points and door positions

- `game/include/level_manager.h` / `game/src/level_manager.cpp`
  - Controls level transitions and scene switching
  - Manages game state across levels and save data

- `game/include/start_scene.h` / `game/src/start_scene.cpp`
  - Implements the first scene shown to the player
  - Presents save slot / start selection logic

- `game/include/level_scene.h` / `game/src/level_scene.cpp`
  - Implements actual level gameplay
  - Handles player movement, collision, traps, and level completion

- `game/include/player.h` / `game/src/player.cpp`
  - Implements player movement, jump behavior, gravity, and input handling

- `game/include/trap_factory.h` and `game/src/traps/`
  - Define trap creation and trap behaviors
  - Support base traps, moving traps, and path-following traps

## Game Data and Assets

### Graphics

Graphics are stored and imported from `game/graphics/`.
The build system processes graphics assets through the Butano toolchain.

### Audio

Music and sound assets are stored in `game/audio/`.

### Save Data

`game/src/main.cpp` references `DataManager` and save state handling via `data_manager.reset()` and save slot selection.

## Extending the Game

To add or modify a level:
1. Open `game/include/levels.h`
2. Add or update `PlatformData`, `TriggerData`, and `TrapData`
3. Set the level background and music items
4. Update the `levels[]` array in `game/src/main.cpp` if you add new levels

To add a new gameplay object:
1. Define its interface in `game/include/`
2. Implement it in `game/src/`
3. Register it with the appropriate scene or factory class

## Notes

- `game/` is the game-specific project code. It depends on Butano and shared engine code in `extern/engine/`.
- The root `Makefile` points `TARGET := platformer`, so the generated ROM is `platformer.gba`.

## Related Docs

- See the repository README for devcontainer, build, and run instructions.
- See `docs/game.md` for details about the game project.

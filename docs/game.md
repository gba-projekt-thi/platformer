# Game Documentation

This document describes the `game/` project inside this repository, including the folder layout, gameplay systems, and how to build or extend the GBA platformer.

## Overview

This repository contains a small Game Boy Advance platformer built with the Butano engine. The playable game lives in `game/` and uses global engine support from `extern/engine/`.

The game includes:
- A start/save selection scene with three save slots (level, deaths, and run timer persist to SRAM), each showing "New" or its furthest-reached world before you even pick it
- A world-select scene and a level-select scene for jumping directly to any already-reached world or level (reachable from the start screen by pressing B)
- A world-select stats line ("Deaths: N  Cleared: X/Y") aggregated from the loaded save slot's existing death total and best-time records
- Level-select also shows each unlocked level's personal-best clear time (mm:ss.cc), once one has been set, marked with a trailing `*` if that level has ever been cleared without a single death (a no-death clear)
- A sequence of themed worlds (bathtub, sewerage, forest, village, dungeon), each with at least two regular levels plus one boss level, defined in code (17 levels total). World 5 has an extra trap-free finale level after its boss.
- Five trap categories: static (base), trigger-activated moving, path-following patrol, player-chasing, and proximity-triggered ambush hazards
- Trigger zones that activate hazards
- A door that advances the player to the next level, ending in a celebratory kiss scene (the door itself can be reskinned as a different, non-animated sprite for narrative moments - see World 5's finale)
- A "New Best!" banner that holds the level-complete transition until acknowledged, whenever a level is cleared faster than its previous best time
- A run-complete summary screen after the kiss scene (total time, deaths, no-death clears), acknowledged with A/Start before the save slot resets
- Music and tilemap backgrounds
- A death counter and run timer HUD
- A pause menu with Continue / Restart Level / Options / Title Screen; Options lets the player adjust music and SFX volume (0-4), persisted per save slot, plus - once unlocked by completing the game - toggle Hard Mode, which speeds up Moving/Chase/Ambush trap velocities for a tougher replay
 - A camera that follows the player across wider, scrolling worlds (single-screen worlds stay fixed)

> This page is a quick map of the `game/` folder and its key files. For the
> full conceptual documentation — architecture, gameplay, level design,
> components, assets, workflow, system internals, extensibility, and a glossary —
> see [`docs/index.md`](index.md).

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
- `audio/` — maxmod audio assets used by the game (one track per world,
  e.g. `world1.xm`–`world5.xm`, plus dedicated menu/start/end themes)
- `dmg_audio/` — DMG audio assets for older sound support
- `graphics/` — sprite and background graphics source files (organized as
  a `global/` folder shared across worlds, plus one `world1/`–`world5/`
  folder per themed world — see
  [`docs/asset-management.md`](asset-management.md))
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
  - Exposes `hard_mode_multiplier()`, read by the trap factory when constructing Moving/Chase/Ambush traps

- `game/include/start_scene.h` / `game/src/start_scene.cpp`
  - Implements the first scene shown to the player
  - Presents save slot / start selection logic
  - Pressing B (instead of A) after picking a save slot opens the world-select scene
  - Peeks all three slots once via `DataManager::peek_state()` (no SRAM writes, doesn't touch the active slot) and shows each as "New" or its furthest-reached world (`WorldIndex::world_for_level()`) before the player picks one

- `game/include/world_select_scene.h` / `game/src/world_select_scene.cpp`
  - Lists the worlds and lets the player jump directly to any world they have already reached (based on the loaded save slot's furthest level)
  - Locked worlds are shown but cannot be entered; A opens the level-select scene for an unlocked world, B returns to `StartScene`
  - Also renders the save slot's stats line (total deaths, levels cleared), computed from `GameState` at menu-build time

- `game/include/level_select_scene.h` / `game/src/level_select_scene.cpp`
  - Lists the individual levels belonging to the world chosen in `WorldSelectScene`
  - Lets the player jump straight to any unlocked level (writing the chosen level to the save slot); B returns to `WorldSelectScene`
  - Appends each unlocked level's best time and, if earned, a trailing `*` no-death-clear indicator

- `game/include/world_index.h`
  - Declares how the flat `levels[]` array in `game/src/main.cpp` is grouped into worlds for the world/level-select scenes
  - This table is maintained by hand and must be kept in sync whenever the `levels[]` array changes
  - Also declares `world_for_level()`, mapping a `furthest_level` back to its containing world index - shared by `WorldSelectScene`'s unlock check and `StartScene`'s slot preview

- `game/include/level_scene.h` / `game/src/level_scene.cpp`
  - Implements actual level gameplay
  - Handles player movement, collision, traps, and level completion
  - Records new best-clear-times and no-death clears on reaching the door, and shows the "New Best!" banner (holding the scene transition) when a record is beaten

- `game/include/kissing_scene.h` / `game/src/kissing_scene.cpp`
  - The celebratory scene shown after the final level's door - holds on the kiss background for `Cfg::Sleep::KISSING_SCENE` frames, then hands off to `SummaryScene`
  - No longer calls `DataManager::reset()` itself - that now happens in `SummaryScene`, once the player has seen the run's final numbers

- `game/include/summary_scene.h` / `game/src/summary_scene.cpp`
  - Shown once, between `KissingScene` and `StartScene`: sums every level's `best_time_frames` for a total-time readout, plus the slot's total deaths and a `has_no_death_clear()` count - all read from state that's about to be reset
  - Waits for A/Start, then calls `DataManager::reset()` and transitions to `StartScene` - the same reset `KissingScene` used to perform directly

- `game/include/player.h` / `game/src/player.cpp`
  - Implements player movement, jump behavior, gravity, and input handling

- `game/include/trap_factory.h` and `game/src/traps/`
  - Define trap creation and trap behaviors
  - Support base traps, moving traps, path-following traps, chaser traps, and ambush traps
  - Scales Moving/Chase/Ambush trap velocities by `LevelManager::hard_mode_multiplier()` at construction time; Path traps are left unscaled

- `game/include/level_structure.h`
  - `TriggerData::name` / `TrapData::trigger_name` bind a MovingTrap/PathTrap
    to a trigger by stable identifier, resolved via
    `LevelManager::get_trigger_by_name()`. This is the only binding
    mechanism now - the legacy `trigger_index` (raw array-position) field
    has been removed from `TrapData`, since every level had already
    migrated to named binding and the field was never read
  - Because binding is by name rather than position, one trigger can drive
    several traps at once (a **trigger chain**) - see `level0_traps` in
    `levels_world1.h` for an existing example

- `game/include/save_sync_controller.h` / `game/src/save_sync_controller.cpp`
  - Implements `SaveSyncController`, which owns the runtime persistence policy for a level
  - Watches the player's death counter and writes deaths/timer to SRAM via `DataManager` when it changes, or on demand via `force_save()`
  - Tracks whether the death count has changed since the level's baseline (`no_deaths_this_attempt()`), which `LevelScene` reads to award a no-death clear badge on completion

- `extern/engine/core/include/i_resettable.h`
  - Declares `IResettable`, a minimal interface with a single `reset()` method
  - Used by `LevelManager` to reset subsystems (such as traps) back to their level-start state after the player dies or the level restarts

- `extern/engine/core/include/audio_settings.h`
  - Declares `AudioSettings`, an engine-level singleton holding the player's music/SFX volume levels (0..4)
  - Lives in the engine layer (not `game/include/`) because `core_scene_manager.cpp` reads it during fade transitions and the engine must not depend on game-side types
  - Game code persists the chosen levels into `GameState::music_volume`/`sfx_volume` and restores them via `LevelManager::restoreHUD()`

 - `game/include/pause_controller.h` / `game/src/pause_controller.cpp`
  - Handles pause toggling and the pause menu (Continue / Restart Level / Options / Title Screen)
  - Owns an embedded Options sub-menu (Up/Down selects Music/SFX/Hard Mode, Left/Right adjusts the level or flips the toggle, B returns and commits to SRAM once)
  - The Hard Mode row only appears once `GameState::hard_mode_unlocked` is set for the loaded slot

- `game/include/timer.h` / `game/src/timer.cpp`
  - Also declares `frames_to_time()`, a plain-division frame→mm:ss.cc converter used only when building level-select and new-best-banner text (not in the per-frame HUD path, which keeps using `Timer`'s lookup tables)

- `game/include/game_state.h`
  - Declares `GameState`, the fixed-width per-save-slot record: level, deaths, timer, `furthest_level`, audio levels, per-level `best_time_frames`, the `no_death_clears` bitmask, and `hard_mode_unlocked`/`hard_mode_enabled`
  - Also declares `set_no_death_clear()`/`has_no_death_clear()`, the small bit-flag helpers `LevelScene` and `LevelSelectScene` use to set/read a level's no-death badge

## Game Data and Assets

### Graphics

Graphics are stored and imported from `game/graphics/`.
The build system processes graphics assets through the Butano toolchain.

### Audio

Music and sound assets are stored in `game/audio/`.

### Save Data

`game/src/main.cpp` references `DataManager` and save state handling via `data_manager.reset()` and save slot selection. Best times, no-death-clear badges, and the Hard Mode unlock flag are deliberately preserved across `DataManager::reset()` (the full-game-completion reset), while the current run's level/deaths/timer and the Hard Mode on/off toggle are cleared, matching how audio volume settings reset.

## Extending the Game

To add or modify a level:
1. Open `game/include/levels.h`
2. Add or update `PlatformData`, `TriggerData`, and `TrapData`
3. Set the level background and music items
4. Update the `levels[]` array in `game/src/main.cpp` if you add new levels
5. Update the `WorldIndex::WORLDS` table in `game/include/world_index.h` if you add, remove, or reorder levels, so the world/level-select scenes stay in sync with the flat `levels[]` array

New levels automatically participate in Hard Mode, best-time tracking, and no-death clears without any extra authoring - these are runtime/save-state systems, not per-level data.

To add a new gameplay object:
1. Define its interface in `game/include/`
2. Implement it in `game/src/`
3. Register it with the appropriate scene or factory class

## Notes

- `game/` is the game-specific project code. It depends on Butano and shared engine code in `extern/engine/`.
- The root `Makefile` points `TARGET := platformer`, so the generated ROM is `platformer.gba`.
- `tests/host/` holds host-buildable unit tests (no ARM toolchain needed) for the Butano-free pure-logic files (`save_integrity`, `save_buffer`, `game_state`, `frame_time`); `.github/workflows/ci.yml` runs them plus a full devcontainer build + headless boot on every push/PR - see [`docs/development-workflow.md`](development-workflow.md#testing-methodology).

## Related Docs

- See the repository README for devcontainer, build, and run instructions.
- See `docs/game.md` for details about the game project.
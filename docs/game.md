# Game Documentation

A map of the `game/` project: where things live and how to build, run and
extend it. What the game *is* (controls, abilities, traps, worlds) is in
[Game Concepts](game-concepts.md); how the code fits together is in
[Project Architecture](architecture.md) and [Components](components.md). The
full set starts at [`docs/index.md`](index.md).

## Build and run

```bash
make                              # in the dev container: produces platformer.gba
mgba-qt platformer.gba            # on the host
./test-rom.sh platformer.gba      # headless boot check, as in CI
```

`TARGET := platformer` in the root `Makefile` names the ROM. Tests and checks:
see [Development Workflow](development-workflow.md).

## Folder structure

| Path | Contents |
|------|----------|
| `game/src/`, `game/include/` | game logic; subfolders `player/`, `traps/`, `scene/`, `level/` |
| `game/levels/world1..5.json` | level data: platforms, triggers, traps, doors, checkpoints, hints |
| `game/include/level/levels_world*.h` | **generated** from the JSON by `tools/Generate_levels.py` - never edit by hand |
| `game/graphics/`, `game/audio/`, `game/dmg_audio/` | assets, see [Asset Management](asset-management.md) |
| `game/replays/` | recorded solutions (and `counter/` checks) played by `make replay` |
| `tests/host/` | host unit tests for the Butano-free rule headers |
| `tools/` | level generator, checkers, replay and tour scripts, with their tests |
| `docs/audio/` | WAV previews of the boss music (see `docs/audio/README.md`) |
| `extern/engine/` | the shared engine layer (scene manager, save system, `IResettable`, `AudioSettings`) on top of Butano |

## Where to look

The headers carry their own explanation in a header comment; the files worth
knowing first:

- `game/src/main.cpp` - engine setup, the flat `levels[]` list, the scene loop.
- `game/include/level/level_structure.h` - the data structs (`LevelData`,
  `TrapData`, `TriggerData`, ...) every field of which is commented.
- `game/include/level/level_manager.h` and `game/src/level_manager.cpp` - level
  load, per-frame update, death and respawn; it delegates the decisions to the
  pure headers (`*_logic.h`, `level_flow.h`, `pause_flow.h`, `hint_logic.h`,
  `locomotion.h`).
- `game/include/cfg*.h` - every tuning constant (physics, dash, pogo, wall
  jump, camera, boss, death feedback).
- `game/include/game_state.h` and `game_state_migration.h` - the save record and
  how old saves are upgraded.
- `game/include/world_index.h` - how `levels[]` is grouped into worlds for the
  select scenes; `tools/Check_world_index.py` keeps it in sync.

## Extending the game

To add a level, edit the world's JSON (see [Level Design](level-design.md)),
run `python3 tools/Generate_levels.py`, add the `LEVEL_*` name to `levels[]` in
`main.cpp` and the world's row in `WorldIndex::WORLDS`. Inserting a level
anywhere but at the end shifts the save indices: see
`game_state_migration.h`. New levels take part in Hard Mode, best times and
no-death clears without extra authoring. For new traps or systems see the
[Extensibility Guide](extensibility.md).

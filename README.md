# GBA Development with Butano

Game Boy Advance development environment using [Butano](https://github.com/GValiente/butano), devkitARM, and the mGBA emulator.

## What is a dev container?

A dev container is a pre-configured Docker environment that includes the full toolchain needed for GBA development. It keeps the build environment consistent across machines and prevents local toolchain issues.

Why this project uses it:
- no manual installation of devkitARM or Butano dependencies
- reproducible build conditions
- isolated environment for ARM tooling
- VS Code integration for editing and building inside the container

Important: all compilation happens inside the container. The resulting ROM can then be launched on the host with mGBA.

## Quick start

```bash
# 1. Clone the repository with submodules
git clone --recursive <your-repo>
cd /workspace

# 2. Open in VS Code and reopen in the dev container
# 3. Build inside the container
make

# 4. Run the ROM on the host machine
mgba-qt platformer.gba
```

## Building

Build commands from the repository root:

```bash
make
./build.sh
```

The ROM output is generated as:

```bash
platformer.gba
```

## Running the game

```bash
mgba-qt platformer.gba
./test-rom.sh platformer.gba
./run-gba.sh platformer.gba --headless
```

## Current project structure

```text
/workspace
├── docs/                       # Project and architecture documentation
├── game/                       # Game-specific code and asset sources
│   ├── assets/                 # Canonical asset directories used by the build
│   ├── audio/                  # Music and sound assets
│   ├── dmg_audio/              # DMG sound assets
│   ├── graphics/               # Sprite and background graphics
│   ├── include/                # Public headers and game data structures
│   │   ├── levels/             # Per-level definitions
│   │   ├── level_structure.h   # Shared level schema
│   │   ├── game_runtime.h      # Runtime bootstrap and lifecycle
│   │   ├── game_session.h      # Shared game completion/session state
│   │   ├── level_scene.h       # Level progression scene
│   │   ├── start_scene.h       # Start/save selection scene
│   │   ├── player.h            # Player movement and state
│   │   ├── level_manager.h     # Level loading and object setup
│   │   └── trap_factory.h      # Trap construction
│   └── src/                    # Implementation files
│       ├── traps/              # Trap implementations
│       ├── game_runtime.cpp    # Runtime orchestration
│       ├── level_scene.cpp     # Level flow and transitions
│       ├── start_scene.cpp     # Start menu flow
│       ├── level_manager.cpp   # Level setup and runtime updates
│       ├── player.cpp          # Movement/input/animation logic
│       └── main.cpp            # Entry point
├── extern/engine/              # External engine setup and Butano dependency
├── docker/                     # Container build setup
├── build.sh                    # Wrapper build script
├── Makefile                    # Main build configuration
├── test-rom.sh                 # ROM validation script
├── run-gba.sh                  # Helper script for launching ROMs
├── platformer.gba              # Built game ROM
└── README.md                   # Repository overview
```

See [docs/game.md](docs/game.md) and [docs/architecture.md](docs/architecture.md) for detailed architecture notes.

## Game overview

This repository contains a modular platformer built with Butano. The current design centers on a small but explicit runtime architecture:

- a runtime bootstrap object manages startup and the main loop
- a shared session decides whether the game has finished
- save data is managed through a dedicated data manager
- level scenes handle progression and transitions
- level definitions are split into reusable per-level files
- trap logic is separated into base, moving, and path-based categories

Current gameplay features:
- start/save selection screen
- multiple level definitions with backgrounds and music
- trap and trigger-based hazard systems
- door-based progression between rooms
- death counter and timer HUD
- player physics with acceleration, gravity, jumping, and coyote time
- save-slot state persistence using structured runtime data

## Environment variables

These are set automatically in the dev container:
- DEVKITPRO=/opt/devkitpro
- DEVKITARM=/opt/devkitpro/devkitARM
- BUTANO_HOME=/workspace/extern/engine/extern/butano

## Debugging and testing

### Build inside the dev container

```bash
make
```

### Run in GUI mode

```bash
mgba-qt platformer.gba
```

### Headless testing

```bash
./test-rom.sh platformer.gba
```

## Troubleshooting

Missing submodules:

```bash
git submodule update --init --recursive
```

Container rebuild:

- VS Code: Command Palette → Dev Containers: Rebuild Container

## Links

- [Butano documentation](https://gvaliente.github.io/butano/)
- [mGBA documentation](https://mgba.io/docs/)
- [devkitPro](https://devkitpro.org/)

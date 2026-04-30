# GBA Development with Butano

Game Boy Advance development environment using [Butano](https://github.com/GValiente/butano) engine, devkitARM, and mGBA emulator.

## What is a Dev Container?

A **dev container** (development container) is a pre-configured Docker container that includes all the tools and dependencies needed for development. Think of it as a portable, isolated development environment that runs the same way on any computer.

**Why we use it here:**
- **No manual setup**: You don't need to install devkitARM, build tools, or libraries on your computer
- **Consistency**: Everyone uses the exact same toolchain and versions, avoiding "works on my machine" issues
- **Isolation**: The GBA development tools won't interfere with other projects on your system
- **VS Code integration**: Open the project in VS Code, and it automatically sets up everything inside the container

**Important**: All **building** happens inside the dev container (where the toolchain lives). You can **run** the resulting `.gba` files on your host computer using mGBA.

## Quick Start

```bash
# 1. Clone with submodules
git clone --recursive <your-repo>
cd projectGBA

# 2. Open in VS Code
code .

# 3. When prompted, click "Reopen in Container"
#    VS Code will build the dev container automatically (takes a few minutes first time)

# 4. Once inside the container, build your game
make

# 5. Run the generated .gba file on your host computer
#    (Open a terminal OUTSIDE VS Code on your host)
mgba-qt platformer.gba
```

**Alternative:** If not using VS Code, build the Docker image manually:
```bash
docker build -t projectgba -f docker/Dockerfile .
```

## Building

**⚠️ All building must be done inside the dev container** (where the ARM toolchain is installed).

**How to build:**

1. **Open the project in VS Code** and reopen in the dev container when prompted
2. **Open a terminal inside VS Code** (it will automatically be inside the container)
3. **Run one of the build commands below:**

```bash
# Option 1: Using Make (recommended)
make

# Option 2: Using CMake wrapper
./build.sh

# Option 3: Using Docker directly (if not using VS Code)
docker run --rm -v $(pwd):/workspace -w /workspace projectgba \
    bash -c "make"
```

**Output:** `foo.gba` and `foo.elf`

**Note:** If you try to build on your host system (outside the container), you'll get "command not found" errors because the ARM toolchain isn't installed there.

## Running ROMs

```bash
# Interactive GUI (requires X11 forwarding or host installation)
mgba-qt foo.gba

# Headless testing (for CI, works in Docker)
./test-rom.sh foo.gba

# Helper script with options
./run-gba.sh foo.gba [OPTIONS]
#   -h, --headless  Run in headless mode (no GUI, for CI)
#   -c, --console   Enable console output with debug logging
```

**Note:** For GUI mode outside the devcontainer, install mGBA on your host system:
```bash
sudo apt-get install mgba-qt
```

## Project Structure

```
projectGBA/
├── game/                              # Example project
├── extern/engine/                     # Engine team (submodule)
├── extern/engine/extern/butano/       # Butano engine (submodule)
├── docker/                            # Docker build environment
└── .devcontainer/                     # VS Code devcontainer config
```

## Game Overview

This repository includes a small GBA platformer built with the Butano engine. The game currently features:

- Two playable levels defined in `game/include/levels.h`
- Level progression through a door at the end of each stage
- Platform placement and collision defined by level data
- Trigger zones that activate moving traps when the player enters them
- Moving traps that accelerate after being triggered and reset when the player dies
- Static trap hazards and decorative tiles using Butano sprite items
- Background music per level and tilemap backgrounds
- A death counter HUD that updates every time the player respawns
- Player movement with acceleration, maximum speed, gravity, and fall speed clamping
- Jump mechanics with jump buffering and coyote time for more forgiving platforming
- Variable jump height based on how long the jump button is held
- A static camera (camera does not follow the player yet)

## Game Asset Layout

The `game/` folder contains:

- `audio/` and `dmg_audio/` for music and sound assets
- `graphics/` for global and level-specific sprites and tilemaps
- `include/` for game headers, level definitions, and shared data
- `src/` for gameplay code, including player, traps, door, and level management

## Environment Variables

Set automatically in devcontainer:
- `DEVKITPRO=/opt/devkitpro`
- `DEVKITARM=/opt/devkitpro/devkitARM`
- `BUTANO_HOME=/workspace/extern/engine/extern/butano`

## How to Debug

### Recommended Workflow (Verified)

This workflow splits tasks between the dev container and your host system:

1. **Build in dev container** (has the ARM toolchain):
   ```bash
   # Inside VS Code terminal (in dev container) or press Ctrl+Shift+B
   cd src/sprites && make
   ```

2. **Run on host system** (avoids X11 display issues):
   ```bash
   # Open a terminal on your HOST (not in VS Code)
   # Install mGBA if needed: sudo apt-get install mgba-qt
   mgba-qt foo.gba
   ```

3. **Debug with GDB** (split between host and container):
   ```bash
   # Terminal 1 (HOST): Start mGBA GUI with GDB stub on port 2345
   mgba-qt -g foo.gba

   # Terminal 2 (DEV CONTAINER): Connect ARM debugger
   # Must run from container because host doesn't have arm-none-eabi-gdb
   arm-none-eabi-gdb foo.elf
   (gdb) target remote host.docker.internal:2345
   (gdb) continue
   ```

**Why this split?**
- **Build in container**: ARM cross-compiler only exists there
- **Run on host**: Simpler graphics/display, no X11 forwarding needed
- **Debug from container**: ARM GDB only exists there, connects to host mGBA

### VS Code Integration

The dev container includes pre-configured tasks and debug settings:
- **Build**: Press `Ctrl+Shift+B` (runs inside the container)
- **Debug**: Press `F5` (start mGBA with `-g` on host first)
- **Tasks**: View → Command Palette → "Tasks: Run Task"

All commands run automatically in the correct environment (container or host).

### Headless Testing (CI)

For automated testing without GUI:
```bash
./test-rom.sh foo.gba
```

**Note:** GDB connection verified working. The container connects to host mGBA via `host.docker.internal:2345`.

## Troubleshooting

**Missing submodule:**
```bash
git submodule update --init --recursive
```

**Rebuild container:**
VS Code → `F1` → "Dev Containers: Rebuild Container"

**Root-owned files:**
```bash
sudo chown -R $(whoami):$(whoami) build
```

## Contributing
### Linter
Before every commit a short script runs to check if all files are using a consistent code style using clang-format and clang-tidy with pre-commit.
This is only possible in the devcontainer or if you have [pre-commit](https://pre-commit.com), clang-format and clang-tidy installed on your system. 

## Links

- [Butano Documentation](https://gvaliente.github.io/butano/)
- [mGBA Documentation](https://mgba.io/docs/)
- [devkitPro](https://devkitpro.org/)

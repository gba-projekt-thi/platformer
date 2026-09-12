# Getting Started

Welcome! This page introduces the project, explains what makes it special, and
walks you through getting a working copy on your machine — without drowning you
in command-line details. For the exact shell commands, the repository
`README.md` is the authoritative reference; this page explains the *why* behind
each step and what to expect along the way.

## What is this project?

This is a **Game Boy Advance (GBA) side-scrolling platformer** built on top of
the open-source **Butano** engine. You play as a duck hopping through themed
worlds, dodging hazards, and reaching an exit door in each stage. It is a
complete, playable experience with:

- A start screen with multiple **save slots**
- A sequence of themed levels (ocean, factory, forest, garden, dungeon)
- A variety of **traps** and **trigger zones** that bring levels to life
- Per-level music, tilemap backgrounds, and animated sprites
- A persistent **death counter** and a run **timer**
- A pause menu with continue, restart, and return-to-title options
- A celebratory **kiss scene** when the game is finished

See [Game Concepts](game-concepts.md) for the gameplay itself and
[Project Architecture](architecture.md) for how it is built.

## What makes this project unique?

A few things set it apart from a typical "first GBA game" sample:

- **Containerized toolchain.** All building happens inside a reproducible
  development container. You do not install cross-compilers by hand, and every
  contributor shares the exact same versions of the toolchain and engine. This
  eliminates "works on my machine" problems.
- **Data-driven levels.** Levels are described as declarative data
  (platforms, triggers, traps, spawn points, doors, backgrounds, music) rather
  than hard-coded logic. This lets designers shape a stage by arranging data,
  not rewriting engine code.
- **Physics-derived level design.** The level layouts respect the player's
  measured movement envelope (how far a jump can carry horizontally and
  vertically). Spacing rules are documented so new platforms are placed inside
  proven-safe limits.
- **Modular gameplay systems.** The player is composed of focused parts —
  locomotion, animation, state machine, and HUD — and traps share a common
  base with specialized behaviors. This keeps individual systems small and
  understandable.
- **Generic reset model.** Entities that need to reset on death advertise
  themselves through a shared reset interface, so new entity types can opt in
  without the level manager needing to know their concrete type.
- **Console personality.** The game leans into a lighthearted duck theme, from
  the player sprite to the victory kiss scene.

## System requirements

You need:

| Requirement | Why |
|-------------|-----|
| **Docker** | Builds run inside the dev container that holds the ARM toolchain. |
| **VS Code + Dev Containers extension** | The recommended workflow; opens the container automatically. |
| **Git** (with submodule support) | The Butano engine and shared engine code are included as submodules. |
| **mGBA** (on your host) | Runs the produced `.gba` ROM. GUI mode is simplest on your host machine. |
| **~1–2 GB disk** | Docker image plus engine sources and build output. |

You do **not** need to install the GBA toolchain, devkitARM, or Butano
manually — that is exactly what the container provides.

## Prerequisites before you start

1. **Install Docker.** Make sure the Docker daemon is running before opening the
   project.
2. **Install VS Code and the Dev Containers extension.** These give you the
   "Reopen in Container" prompt.
3. **Install mGBA on your host** if you want to play with a graphical window.
   (Headless testing for CI does not need a display.)
4. **Clone the repository with its submodules.** The engine lives in
   submodules, so a recursive clone is important. If you cloned without them,
   initialize them afterward.

The `README.md` shows the exact clone and submodule commands.

## Development environment setup (conceptual)

Think of setup as three layers coming together:

1. **The container image.** A `Dockerfile` in the repository defines an image
   that includes the devkitARM cross-compiler, build tools, and the engine
   environment variables the build expects.
2. **The dev container config.** `.devcontainer/` tells VS Code how to build
   that image and mount your workspace inside it, plus which tasks and debug
   launchers to expose.
3. **The first build.** The first time you open the project in the container,
   the image is assembled (a few minutes). After that, builds are fast and use
   the cached image.

When you open the repository in VS Code, you are prompted to **reopen in the
container**. Doing so gives you a terminal and editor that already know about
the toolchain. You edit files as normal; compilation happens inside the
container.

> **Key mental model:** *build inside the container, run on the host.* The
> container has the cross-compiler but not necessarily a comfortable display
> setup; your host has the emulator with a window. The split is deliberate and
> documented in [Development Workflow](development-workflow.md).

## Build process (described, not scripted)

The build pipeline works like this at a high level:

1. **Assets are processed.** Graphics and music in `game/graphics/` and
   `game/audio/` are converted by the engine's build tooling into the binary
   forms the GBA expects (tilesets, backgrounds, sound data, palettes).
2. **Sources are compiled and linked.** Your game code, the shared engine code,
   and Butano are compiled with the ARM cross-compiler and linked into an
   executable.
3. **A ROM is produced.** The link step emits a GBA ROM file (and a matching
   debug ELF) at the repository root, named for the configured target.

There are a few equivalent ways to invoke the build (a Make target, a wrapper
script, or a direct container command). They all do the same thing; pick
whichever fits your workflow. The exact commands are in the `README.md`.

## Deployment / running process

A GBA "deployment" for development is simply **running the ROM in an emulator**:

- **Interactive play:** Open the produced ROM in mGBA on your host for normal
  play and manual testing.
- **Headless / CI:** A helper script runs the ROM without a graphical window,
  useful for automated checks.
- **Debugging:** mGBA can expose a remote-debugging stub that you connect to
  from the container's ARM debugger — a host/container split explained in
  [Development Workflow](development-workflow.md).

## A first tour (user-level)

Once the ROM runs, here is the experience to expect:

1. **Start screen:** Choose a save slot and begin. Controls for navigating the
   menu mirror the pause menu.
2. **Gameplay:** Move and jump as the duck. Reach the animated **door** at the
   end of each stage to advance. Touching a **trap** or falling too far kills
   you; you respawn and the **death counter** increments.
3. **Pause:** Open the pause menu to continue, restart the level, or return to
   the title screen. Progress (level, deaths, timer) persists to SRAM.
4. **Victory:** Clearing the final world plays the kiss scene.

## Where to go next

| Goal | Read |
|------|------|
| Understand the big picture | [Project Architecture](architecture.md) |
| Learn the gameplay & controls | [Game Concepts](game-concepts.md) |
| Design or tune a level | [Level Design](level-design.md) |
| Understand a subsystem | [Components](components.md) |
| Add art / music / animation | [Asset Management](asset-management.md) |
| Contribute code or debug | [Development Workflow](development-workflow.md) |
| Understand GBA-specific choices | [System Internals](system-internals.md) |
| Extend the game | [Extensibility Guide](extensibility.md) |

## Related
- [Index](index.md)
- [Project Architecture](architecture.md)
- [Development Workflow](development-workflow.md)
- [Glossary](glossary.md)
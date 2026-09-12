# Project Documentation — GBA Platformer

Welcome to the documentation for this Game Boy Advance platformer, a small but
fully-featured side-scrolling game built on the **Butano** engine. These pages
describe the *concepts*, *architecture*, and *system interactions* of the
project. They deliberately stay conceptual — you will not find raw source code
here, only the ideas, structures, and workflows that explain how the game fits
together and how to extend it.

> The repository `README.md` remains the single source of truth for exact
> shell commands (building, running, debugging). This documentation set
> explains the *why* and the *what* behind those commands.

## How to read this documentation

| If you are… | Start with |
|-------------|-----------|
| New to the project | [Getting Started](getting-started.md) |
| A developer wanting the big picture | [Project Architecture](architecture.md) |
| A designer shaping levels | [Level Design](level-design.md) |
| Implementing or debugging a feature | [Components](components.md) |
| Adding art, music, or animations | [Asset Management](asset-management.md) |
| Contributing code | [Development Workflow](development-workflow.md) |
| Curious about GBA-specific choices | [System Internals](system-internals.md) |
| Extending the game | [Extensibility Guide](extensibility.md) |
| Looking up a term | [Glossary](glossary.md) |

## Documentation map

1. **[Getting Started](getting-started.md)** — What the project is, what makes it
   unique, prerequisites, environment setup, and the build/deploy flow.
2. **[Project Architecture](architecture.md)** — High-level components, the main
   subsystems, data flow, and interaction patterns.
3. **[Game Concepts](game-concepts.md)** — Core gameplay, controls, progression,
   level organization, trap behaviors, audio/visual identity.
4. **[Level Design](level-design.md)** — Level anatomy, platform placement
   principles, trap configuration, trigger zones, theming, and progression.
5. **[Components](components.md)** — Deep conceptual descriptions of the Player,
   Trap, and Level Management systems.
6. **[Asset Management](asset-management.md)** — Graphics, audio, animation,
   loading, and organization by theme.
7. **[Development Workflow](development-workflow.md)** — Building, testing,
   debugging, version control, and contribution guidelines.
8. **[System Internals](system-internals.md)** — Memory, performance, engine
   integration, platform constraints, and optimizations.
9. **[Extensibility Guide](extensibility.md)** — Adding levels, traps, player
   behavior, assets, and extending systems.
10. **[Glossary](glossary.md)** — Definitions of project-specific terminology.

## Documentation style

These pages follow a consistent philosophy:

**What you will find**
- Architectural diagrams and system flow illustrations
- Conceptual explanations and design rationales
- Terminology and definitions
- Best practices and principles
- User workflows and scenarios

**What you will not find**
- Actual code implementations
- Low-level engine internals beyond a conceptual level
- Implementation-specific comments
- Code snippets (kept to an absolute conceptual minimum, if ever)

**Conventions used throughout**
- **Mermaid diagrams** depict systems and flows.
- **Tables** summarize configurations, comparisons, and reference data.
- Terms that have dedicated definitions in the [Glossary](glossary.md) appear
  in **bold** the first time they are used in a page.
- Sections link to each other; follow the "Related" footer on each page.

---

*The game itself is a duck-themed platformer. If you see whimsical references to
ducks, kisses, and beaks, that is the personality of the project — the player
character is a duck, and finishing the game earns a celebratory kiss scene.*

## Related
- [Getting Started](getting-started.md)
- [Project Architecture](architecture.md)
- [Glossary](glossary.md)
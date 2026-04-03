# Clang-Tidy Integration Guide for Butano Project

## What's Been Set Up

Your project is now configured to use clang-tidy with proper Butano library support.

### Key Components

1. **Compilation Database** (`compile_commands.json`)
   - Generated using `bear` to capture all compiler flags and include paths
   - Located at `/workspace/compile_commands.json`
   - Contains all necessary Butano include paths and ARM cross-compilation flags

2. **.clang-tidy Configuration**
   - Specifies which checks to run (modernize-* and readability-*)
   - Filters to only lint your source code (not Butano library code)
   - Located at `/workspace/.clang-tidy`

3. **VS Code Integration** (`.vscode/settings.json`)
   - Configures clangd to use clang-tidy
   - Points to the compilation database
   - Enables real-time linting in the editor

## How It Works

- **clangd** (the language server) reads your source code
- **clang-tidy** runs checks defined in `.clang-tidy`
- **compile_commands.json** tells clang-tidy about:
  - Include paths to Butano libraries
  - Compiler flags (C++20, ARM architecture flags)
  - All dependencies

This allows clang-tidy to:
- ✅ Understand all Butano symbols and functions
- ✅ Resolve includes properly  
- ✅ Analyze your code with correct compilation context

## Linting Your Code

### In VS Code
- Warnings appear as squiggles in the editor
- Problems panel shows all issues
- Hover over warnings for quick fixes

### From Command Line
```bash
cd /workspace
clang-tidy -p=. src/sprites/src/main.cpp
```

## Skipping Checks for Specific Code

If you need to disable a check for a line:
```cpp
// NOLINTNEXTLINE(readability-magic-numbers)
int value = 42;
```

Or for a function:
```cpp
// NOLINT(*)
void someFunction() { ... }
```

## Regenerating compile_commands.json

If you add new files or change the build system, regenerate it:
```bash
cd /workspace/src/sprites
make clean
bear -- make
cp compile_commands.json /workspace/
```

## Configuration File Locations

| File | Purpose |
|------|---------|
| `.clang-tidy` | Clang-tidy checks and filtering rules |
| `.vscode/settings.json` | VS Code clang-tidy integration settings |
| `compile_commands.json` | Compilation database with all compiler flags |
| `.devcontainer/devcontainer.json` | Dev container configuration |

## Troubleshooting

### "Variable not found" errors for Butano code
- Rebuild `compile_commands.json`: `cd src/sprites && bear -- make`
- Restart VS Code
- Check that `compile_commands.json` exists in workspace root

### Linting is very slow
- Disable the C++ extension and use clangd only
- Or reduce checks by commenting checks in `.clang-tidy`

### Too many warnings from Butano library
- Warnings are filtered by `HeaderFilterRegex` in `.clang-tidy`
- Only your `src/` files are checked for warnings

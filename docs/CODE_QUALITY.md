# Code Quality Setup Guide

The repository is now supported by a complete code quality pipeline with automatic formatting, linting, and pre-commit hooks.

## 📋 What's Configured

### 1. **Real-Time Linting (VS Code)**
- **Tool**: clangd + clang-tidy
- **Config**: `.clang-tidy`
- **How it works**: Warnings appear as you type in the editor

### 2. **Code Formatting**
- **Tool**: clang-format
- **Config**: `.clang-format`
- **Trigger**: Pre-commit hook (automatic on commit)

### 3. **Deep Static Analysis**
- **Tool**: clang-tidy
- **Config**: `.clang-tidy`
- **Trigger**: Pre-commit hook + CI/CD

### 4. **Automated Build Database**
- **Tool**: bear
- **File**: `compile_commands.json`
- **Purpose**: Tells linters about Butano includes and compiler flags

### 5. **CI/CD Pipeline**
- **File**: `.github/workflows/lint.yml`
- **Trigger**: On every push and pull request

---

## 🚀 Getting Started

### Initial Setup (Run Once)

A quick first pass can make the environment ready for work:

```bash
cd /workspace
bash setup-hooks.sh
cd src/sprites && bear -- make && cp compile_commands.json /workspace/
```

The `setup-hooks.sh` script typically installs:
- `clang-format` for formatting
- `clang-tidy` for static analysis
- `clangd` as the VS Code language server
- `bear` to generate the compilation database

### Daily Workflow

1. **Edit your C++ code** in VS Code
   - Real-time linting errors appear as squiggles
   - Hover to see suggestions

2. **Format and commit**
   ```bash
   git add .
   git commit -m "Your message"
   ```
   - Pre-commit hook automatically:
     - ✅ Runs clang-format on all C++ files
     - ✅ Runs clang-tidy checks
     - ✅ Prevents commit if critical issues found

---

## 📁 Configuration Files

| File | Purpose | When to Edit |
|------|---------|--------------|
| `.clang-format` | Code formatting rules | Never (unless team standards change) |
| `.clang-tidy` | Linting checks & filtering | When you want different checks |
| `.github/workflows/lint.yml` | CI/CD linting | Team collaboration decisions |
| `compile_commands.json` | Build database | Auto-generated, regenerate if build changes |

---

## 🔄 Regenerating the Compilation Database

If you add new files or the build system changes:

```bash
cd /workspace/src/sprites
make clean
bear -- make
cp compile_commands.json /workspace/
```

---

## 🚫 Skipping Checks (When Necessary)

### Skip a single line
```cpp
// NOLINTNEXTLINE(readability-magic-numbers)
int value = 42;
```

### Skip an entire function
```cpp
// NOLINT(*)
void externalCode() { ... }
```

### Bypass pre-commit hook (last resort)
```bash
git commit --no-verify -m "Your message"
```

---

## 📊 Viewing Issues

### In VS Code
- **Problems Panel**: `Ctrl+Shift+M` (View → Problems)
- **Inline**: Hover over squiggles for details
- **Quick Fix**: Hover and click "Quick Fix"

### From Command Line
```bash
# Check a specific file
clang-tidy -p=/workspace src/sprites/src/main.cpp

# Check all source files
find src -name "*.cpp" -o -name "*.h" | xargs clang-tidy -p=/workspace
```

---

## ✅ Verification Checklist

Run this to verify everything is set up:

```bash
#!/bin/bash
echo "Checking clang-format..."
which clang-format && echo "✓ clang-format installed" || echo "✗ Missing"

echo "Checking clang-tidy..."
which clang-tidy && echo "✓ clang-tidy installed" || echo "✗ Missing"

echo "Checking compilation database..."
test -f /workspace/compile_commands.json && echo "✓ compile_commands.json exists" || echo "✗ Missing"

echo "Checking git hook..."
test -x /workspace/.git/hooks/pre-commit && echo "✓ pre-commit hook installed" || echo "✗ Missing"

echo ""
echo "Testing clang-tidy..."
cd /workspace
clang-tidy -p=. src/sprites/src/main.cpp 2>&1 | head -1
```

---

## 🔧 Troubleshooting

### "clang-format: command not found"
```bash
sudo apt-get install -y clang-format
```

### "compile_commands.json not found"
```bash
cd src/sprites && bear -- make && cp compile_commands.json /workspace/
```

### Pre-commit hook doesn't run
```bash
# Reinstall hooks
bash setup-hooks.sh
```

### Too many warnings
Edit `.clang-tidy` and disable checks:
```yaml
Checks: >
  modernize-*,
  readability-*,
  -readability-magic-numbers     # Add here to disable
```

## 🌐 Tools Documentation

- **clang-tidy**: https://clang.llvm.org/extra/clang-tidy/
- **clang-format**: https://clang.llvm.org/docs/ClangFormat/
- **clangd**: https://clangd.llvm.org/


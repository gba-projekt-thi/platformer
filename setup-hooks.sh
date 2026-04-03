#!/bin/bash
# Setup git hooks for this repository
# Run: ./setup-hooks.sh

# Install required tools if not present
echo "Checking required tools..."
if ! command -v clang-format &> /dev/null; then
    echo "Installing clang-format..."
    sudo apt-get update && sudo apt-get install -y clang-format
fi

if ! command -v clang-tidy &> /dev/null; then
    echo "Installing clang-tidy..."
    sudo apt-get install -y clang-tidy
fi

if ! command -v clangd &> /dev/null; then
    echo "Installing clangd..."
    sudo apt-get install -y clangd-18
    sudo ln -sf /usr/bin/clangd-18 /usr/bin/clangd
fi

if ! command -v bear &> /dev/null; then
    echo "Installing bear..."
    sudo apt-get install -y bear
fi

echo "✓ Required tools installed"

HOOKS_DIR=$(dirname "$0")/.githooks
GIT_HOOKS_DIR=$(git rev-parse --git-dir)/hooks

if [ ! -d "$GIT_HOOKS_DIR" ]; then
    echo "Error: Not a git repository or .git/hooks directory doesn't exist"
    exit 1
fi

# Copy hooks and make executable
for hook in $HOOKS_DIR/*; do
    if [ -f "$hook" ]; then
        hook_name=$(basename "$hook")
        target="$GIT_HOOKS_DIR/$hook_name"
        cp "$hook" "$target"
        chmod +x "$target"
        echo "✓ Installed $hook_name hook"
    fi
done

echo "✓ Git hooks installed successfully"
echo ""
echo "Installed hooks:"
echo "  - pre-commit: Runs clang-format and clang-tidy on staged files"

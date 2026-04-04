#!/bin/bash
set -euo pipefail

# Setup git hooks for this repository
# Run: ./setup-hooks.sh

WORKSPACE=$(pwd)
VENV_DIR="$WORKSPACE/.venv"

install_package() {
    local package="$1"
    echo "Installing ${package}..."
    sudo apt-get update
    sudo apt-get install -y "$package"
}

echo "Checking required tools..."
if ! command -v clang-format &> /dev/null; then
    install_package clang-format
fi

if ! command -v clang-tidy &> /dev/null; then
    install_package clang-tidy
fi

if ! command -v clangd &> /dev/null; then
    install_package clangd-18
    sudo ln -sf /usr/bin/clangd-18 /usr/bin/clangd
fi

if ! command -v bear &> /dev/null; then
    install_package bear
fi

if ! command -v python3 &> /dev/null; then
    echo "Error: python3 is required but not installed"
    exit 1
fi

if ! python3 -m venv "$VENV_DIR" >/dev/null 2>&1; then
    install_package python3-venv
    python3 -m venv "$VENV_DIR"
fi

source "$VENV_DIR/bin/activate"
python -m pip install --upgrade pip
python -m pip install pre-commit

if ! python -m pre_commit --version >/dev/null 2>&1; then
    echo "Error: pre-commit installation failed"
    exit 1
fi

echo "✓ Required tools installed"

GIT_HOOKS_DIR=$(git rev-parse --git-dir)/hooks

if [ ! -d "$GIT_HOOKS_DIR" ]; then
    echo "Error: Not a git repository or .git/hooks directory doesn't exist"
    exit 1
fi

echo "Installing pre-commit hook..."
python -m pre_commit install --install-hooks

echo "Generating compilation database..."
pushd src/sprites >/dev/null
bear -- make
cp compile_commands.json "$WORKSPACE/"
popd >/dev/null

echo "✓ compile_commands.json generated"

echo "✓ Git hooks installed successfully"
echo

echo "Installed hooks:"
echo "  - pre-commit: Runs clang-format and clang-tidy on staged files via .pre-commit-config.yaml"

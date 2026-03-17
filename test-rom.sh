#!/bin/bash
# CI-friendly script to test GBA ROM
# This script runs the ROM in headless mode and captures any output
# Exit code 0 = success, non-zero = failure

set -e

ROM_FILE="${1:-polygons.gba}"

if [ ! -f "$ROM_FILE" ]; then
    echo "Error: ROM file not found: $ROM_FILE"
    exit 1
fi

echo "Testing ROM: $ROM_FILE"
echo "Running mGBA in headless mode..."

# Set up environment for headless execution
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy
export XDG_RUNTIME_DIR=/tmp/runtime-dir
mkdir -p "$XDG_RUNTIME_DIR"

# Run mGBA in headless mode for 5 seconds
# This allows the ROM to initialize and run basic tests
timeout 5s mgba --log-level INFO "$ROM_FILE" 2>&1 || EXIT_CODE=$?

# timeout returns 124 if it times out (expected behavior)
# We consider this a success if the ROM ran
if [ "${EXIT_CODE:-0}" -eq 124 ] || [ "${EXIT_CODE:-0}" -eq 0 ]; then
    echo "✓ ROM executed successfully"
    exit 0
else
    echo "✗ ROM execution failed with exit code: $EXIT_CODE"
    exit $EXIT_CODE
fi

#!/bin/bash
# Script to run GBA ROM with mGBA emulator
# Usage: ./run-gba.sh <path-to-rom.gba> [options]
#
# Options:
#   -h, --headless  Run in headless mode (no GUI, for CI)
#   -c, --console   Enable console output
#   -s, --screenshot  Take screenshot after N frames (e.g., -s 100)

set -e

ROM_FILE="$1"
HEADLESS=false
CONSOLE=false

# Parse arguments
shift || true
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--headless)
            HEADLESS=true
            shift
            ;;
        -c|--console)
            CONSOLE=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Check if ROM file exists
if [ -z "$ROM_FILE" ]; then
    echo "Usage: $0 <path-to-rom.gba> [options]"
    echo ""
    echo "Options:"
    echo "  -h, --headless    Run in headless mode (no GUI, for CI)"
    echo "  -c, --console     Enable console output"
    exit 1
fi

if [ ! -f "$ROM_FILE" ]; then
    echo "Error: ROM file not found: $ROM_FILE"
    exit 1
fi

# Build command
if [ "$HEADLESS" = true ]; then
    # Use mgba for headless mode (good for CI)
    CMD="mgba"

    # Run for a limited time in headless mode
    timeout 10s $CMD "$ROM_FILE" || true
else
    # Use mgba-qt for GUI mode
    CMD="mgba-qt"

    if [ "$CONSOLE" = true ]; then
        # Enable debug console
        CMD="$CMD --log-level DEBUG"
    fi

    $CMD "$ROM_FILE"
fi

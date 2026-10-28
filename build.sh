#!/bin/bash
# Build script for GBA projects using CMake
# This script handles the CMake build process

set -e

BUILD_DIR="${1:-build}"
if [ "$#" -gt 0 ]; then
    shift
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake -B "$BUILD_DIR" -S .

# Build
echo "Building project..."
cmake --build "$BUILD_DIR" "$@"

echo "Build complete!"
echo "Output files: platformer.gba and platformer.elf"
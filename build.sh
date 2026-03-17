#!/bin/bash
# Build script for GBA projects using CMake
# This script handles the CMake build process

set -e

BUILD_DIR="${1:-build}"

# Create build directory
mkdir -p "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake -B "$BUILD_DIR" -S .

# Build
echo "Building project..."
cmake --build "$BUILD_DIR" "$@"

echo "Build complete!"
echo "Output files can be found in src/sprites/"

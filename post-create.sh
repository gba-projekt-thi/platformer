#!/bin/bash
# Install Pre-Commit hook
pre-commit install

# Generate compile_commands.json for clang-tidy
make clean
bear -- make

# Remove -mthumb-interwork line for clang-tidy compatability
sed -i 's/"-mthumb-interwork",//g' compile_commands.json


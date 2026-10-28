#!/bin/bash
# Runs once when the dev container is created (postCreateCommand only: running
# it as updateContentCommand as well built the project twice on creation).

# Install Pre-Commit hook
pre-commit install

# Generate compile_commands.json for clang-tidy.
# bear only records the commands of the files it sees compiled, so this needs a
# full build (the 'make clean' below). That is slow, so it is skipped when the
# file already exists: it lives in the workspace and is git-ignored, so it
# survives rebuilding the container. After adding source files or changing the
# Makefile, regenerate it with: ./post-create.sh --force
if [ "$1" != "--force" ] && [ -s compile_commands.json ]; then
    echo "compile_commands.json exists - skipping the build (use --force to regenerate)"
    exit 0
fi
make clean
bear -- make -j1

# Remove -mthumb-interwork line for clang-tidy compatability
sed -i 's/"-mthumb-interwork",//g' compile_commands.json
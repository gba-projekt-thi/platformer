#!/bin/bash
# Install Pre-Commit hook
pre-commit install

# Generate compile_commands.json for clang-tidy
make clean
# Run a serial build under bear to ensure compile_commands.json captures
# exactly one compiler invocation per source file (clang-tidy requires this).
bash -lc "export PATH=/workspace/extern/clang18/bin:$PATH; export CC=/workspace/extern/clang18/bin/clang; export CXX=/workspace/extern/clang18/bin/clang++ ; bear -- make -j1"

# Remove -mthumb-interwork line for clang-tidy compatability
sed -i 's/"-mthumb-interwork",//g' compile_commands.json


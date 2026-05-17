#!/usr/bin/env bash

find game/include game/src \
    \( -name "*.h" -o -name "*.hpp" -o -name "*.cpp" \) \
    -exec ./scripts/check_naming.py {} +
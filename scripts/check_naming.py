#!/usr/bin/env python3

import re
import sys
from pathlib import Path

# -----------------------------
# Naming convention regex
# -----------------------------

PASCAL_CASE = re.compile(r"^[A-Z][A-Za-z0-9]*$")
SNAKE_CASE = re.compile(r"^[a-z][a-z0-9_]*$")
MEMBER_CASE = re.compile(r"^[a-z][a-z0-9_]*$")
UPPER_CASE = re.compile(r"^[A-Z][A-Z0-9_]*$")
FILE_CASE = re.compile(r"^[a-z0-9_]+\.(h|hpp|cpp)$")    

# -----------------------------
# Patterns
# -----------------------------

CLASS_PATTERN = re.compile(r"^\s*(class|struct|enum class)\s+([A-Za-z_][A-Za-z0-9_]*)")
# ignore lines starting with ':' (constructor initializer lists)
FUNCTION_PATTERN = re.compile(
    r"^\s*(?!:)(?:virtual\s+)?(?:static\s+)?(?:inline\s+)?(?:constexpr\s+)?"
    r"(?:[\w:<>]+[\s*&]+)+([a-zA-Z_][a-zA-Z0-9_]*)\s*\("
)
MEMBER_PATTERN = re.compile(
    r"^\s*(?:bn::\w+|int|bool|float|double|char|unsigned|const|auto|std::\w+)[^;=]*\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(?:=|;)"
)
# allow both = and () initialization for constexpr
CONST_PATTERN = re.compile(
    r"^\s*(?:inline\s+)?constexpr\s+.*\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:=|\()"
)
# Compiler attribute pattern
ATTRIBUTE_PATTERN = re.compile(r"__\w+__\s*\(\(.*\)\)")
# Include check
INCLUDE_PATTERN = re.compile(r'^\s*#include\s*<.*>')

# -----------------------------
# Helper
# -----------------------------

errors = []

def report(path, line_no, message):
    errors.append(f"{path}:{line_no}: {message}")

# -----------------------------
# File checks
# -----------------------------

for file_arg in sys.argv[1:]:
    path = Path(file_arg)

    # File naming
    if not FILE_CASE.match(path.name):
        report(path, 0, f"File name '{path.name}' should be snake_case")

    inside_class = False

    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    for i, line in enumerate(lines, start=1):

        stripped_line = line.strip()

        # -------------------------
        # Include check
        # -------------------------
        if INCLUDE_PATTERN.search(line):
            report(path, i, '#include with <> is not allowed, use "" instead')

        # -------------------------
        # Skip constructor initializer lists
        # -------------------------
        if stripped_line.startswith(":"):
            continue

        # -------------------------
        # Class / Struct / Enum
        # -------------------------
        class_match = CLASS_PATTERN.search(line)
        if class_match:
            name = class_match.group(2)
            if not PASCAL_CASE.match(name):
                report(path, i, f"Type '{name}' should be PascalCase")
            inside_class = True

        # crude class scope detection
        if inside_class and "};" in line:
            inside_class = False

        # -------------------------
        # constexpr constants
        # -------------------------
        const_match = CONST_PATTERN.search(line)
        if const_match:
            name = const_match.group(1)
            if not UPPER_CASE.match(name):
                report(path, i, f"Constant '{name}' should be UPPER_SNAKE_CASE")
            # skip function check for constants
            continue

        # -------------------------
        # functions
        # -------------------------
        # Skip all compiler attributes like __attribute__((...))
        if ATTRIBUTE_PATTERN.search(line):
            continue

        func_match = FUNCTION_PATTERN.search(line)
        if func_match:
            name = func_match.group(1)
            ignore = {"if", "while", "switch", "for", "return"}
            if name not in ignore and not SNAKE_CASE.match(name):
                report(path, i, f"Function '{name}' should be snake_case")

        # -------------------------
        # member variables
        # -------------------------
        if inside_class:
            member_match = MEMBER_PATTERN.search(line)
            if member_match:
                name = member_match.group(1)
                # skip static constexpr and accidentally matched methods
                if UPPER_CASE.match(name) or "(" in line:
                    continue
                if not MEMBER_CASE.match(name):
                    report(path, i, f"Member variable '{name}' should be snake_case")

# -----------------------------
# Result
# -----------------------------

if errors:
    print("\nNaming convention violations:\n")
    for err in errors:
        print(err)
    sys.exit(1)

sys.exit(0)
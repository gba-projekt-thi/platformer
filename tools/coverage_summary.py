#!/usr/bin/env python3
"""
coverage_summary.py

Line coverage of the game code by the host unit tests, from the gcov data that
`make -C tests/host coverage` leaves in build/host-tests-coverage/. A line
counts as covered if any test object ran it. Needs only gcov (part of GCC).

Usage (from the repository root):
    python3 tools/coverage_summary.py [OBJECT_DIR] [SOURCE_ROOT]
"""

import json
import subprocess
import sys
from pathlib import Path


def merge(reports, root):
    """{path relative to root: (covered lines, lines)} over gcov JSON reports."""
    root = Path(root).resolve()
    lines = {}
    for report in reports:
        cwd = Path(report["current_working_directory"])
        for entry in report["files"]:
            path = (cwd / entry["file"]).resolve()
            if root not in path.parents:
                continue
            seen = lines.setdefault(str(path.relative_to(root)), {})
            for line in entry["lines"]:
                n = line["line_number"]
                seen[n] = seen.get(n, False) or line["count"] > 0
    return {name: (sum(seen.values()), len(seen)) for name, seen in lines.items()}


def summary(merged):
    """Report lines, least covered file first, then the total."""
    rows = sorted(merged.items(), key=lambda item: (item[1][0] / item[1][1], item[0]))
    out = [f"{100 * c / n:6.1f}%  {c:4d}/{n:<4d} {name}" for name, (c, n) in rows]
    covered = sum(c for c, _ in merged.values())
    total = sum(n for _, n in merged.values())
    out.append(f"{100 * covered / total if total else 0:6.1f}%  {covered}/{total} lines in total")
    return out


def gcov_reports(object_dir):
    for gcda in sorted(Path(object_dir).glob("*.gcda")):
        result = subprocess.run(
            ["gcov", "--json-format", "--stdout", gcda.with_suffix(".o").name],
            cwd=object_dir, capture_output=True, text=True, check=True,
        )
        for line in result.stdout.splitlines():
            if line.strip():
                yield json.loads(line)


def main(argv):
    object_dir = Path(argv[0]) if argv else Path("build/host-tests-coverage")
    root = Path(argv[1]) if len(argv) > 1 else Path("game")
    if not any(object_dir.glob("*.gcda")):
        print(f"No coverage data in {object_dir}: run make -C tests/host coverage first.")
        return 1
    for line in summary(merge(gcov_reports(object_dir), root)):
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

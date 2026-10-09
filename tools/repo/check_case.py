#!/usr/bin/env python3
"""File-name case checks for git-tracked files.

Usage:
  python tools/repo/check_case.py --report   # list findings, exit 0
  python tools/repo/check_case.py            # enforce, exit 1 on findings

Checks:
  1. case-fold collisions between tracked paths (breaks case-insensitive
     filesystems, i.e. cloning on Windows/macOS);
  2. uppercase characters in tracked paths (target: lowercase with approved
     project/tool-name exceptions),
     minus the ALLOW exemptions;
  3. #include / %include directives whose basename matches a tracked
     file only when case is ignored (breaks case-sensitive filesystems).
"""
import argparse
import re
import subprocess
import sys

ALLOW = {
    "AGENTS.md",
    "INSTALL.md",
    "MANIFEST.json",
    "MANIFEST.sha256.json",
}
ALLOW_PREFIXES = ("docs/", ".git", "external/", "LICENSES/")
# Tool-mandated basenames that cannot be made lowercase.
ALLOW_TOOL_BASENAMES = {"README.md", "README.txt", "BUILD.md",
                        "CMakeLists.txt",
                        "CMakePresets.json", "SKILL.md", "NOTICE.md",
                        "OFL.txt", "LICENSE", "LICENSE.txt"}
# Maintainer-approved asset basenames kept in upstream/package spelling.
ALLOW_ASSET_BASENAMES = {"NotoSansJP-Regular.ttf"}
ALLOW_BASENAMES = ALLOW_TOOL_BASENAMES | ALLOW_ASSET_BASENAMES
SRC_EXT = {".c", ".h", ".cpp", ".hpp", ".cc", ".asm", ".x86", ".rc", ".tbl"}
INC_RE = re.compile(rb'''^\s*[#%]\s*include\s+["'<]([^"'>]+)["'>]''',
                    re.M)


def tracked_files():
    out = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True)
    return [p for p in out.stdout.decode("utf-8").split("\0") if p]


def exempt(path: str) -> bool:
    base = path.rsplit("/", 1)[-1]
    return (path in ALLOW or base in ALLOW_BASENAMES
            or ("/" not in path and base.startswith("CHANGES")
                and base.endswith(".md"))
            or base.startswith("Makefile")
            or any(path.startswith(p) for p in ALLOW_PREFIXES))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", action="store_true")
    args = ap.parse_args()
    files = tracked_files()
    findings = 0

    # 1. case-fold collisions
    fold = {}
    for p in files:
        fold.setdefault(p.casefold(), []).append(p)
    for group in fold.values():
        if len(group) > 1:
            print("COLLISION  " + "  ".join(group))
            findings += 1

    # 2. uppercase in paths
    for p in files:
        if exempt(p):
            continue
        if p != p.lower():
            print(f"UPPERCASE  {p}")
            findings += 1

    # 3. include-directive case mismatches (basename heuristic)
    by_lower = {}
    for p in files:
        by_lower.setdefault(p.rsplit("/", 1)[-1].lower(), set()).add(
            p.rsplit("/", 1)[-1])
    exact = set(by_lower)  # keys are lowercase
    for p in files:
        if exempt(p):
            continue
        dot = p.rfind(".")
        if dot < 0 or p[dot:].lower() not in SRC_EXT:
            continue
        with open(p, "rb") as f:
            data = f.read()
        if b"\0" in data:
            continue
        for m in INC_RE.finditer(data):
            inc = m.group(1).decode("latin-1").replace("\\", "/")
            base = inc.rsplit("/", 1)[-1]
            low = base.lower()
            if low in exact and base not in by_lower[low]:
                print(f"INC-CASE   {p}: {inc} (on disk: "
                      f"{'/'.join(sorted(by_lower[low]))})")
                findings += 1

    print(f"{findings} finding(s)", file=sys.stderr)
    return 0 if (args.report or findings == 0) else 1


if __name__ == "__main__":
    sys.exit(main())

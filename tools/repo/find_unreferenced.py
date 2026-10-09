#!/usr/bin/env python3
"""Report tracked source/header files unreachable from the build roots.

Usage:
  python tools/repo/find_unreferenced.py --report [--root FILE ...]

This is a REPORT generator for the M3 triage, not a deleter, and it is
deliberately conservative: reachability is (a) files named in project /
build files, plus (b) the transitive closure of #include / %include
resolved by basename (case-insensitive). Basename resolution
over-approximates reachability, so a file listed as UNREFERENCED is a
candidate, not a verdict; anything ambiguous must land in the UNSURE
section of the triage document.

Default roots: every tracked *.dsp *.dsw *.sln *.vcproj *.vcxproj
CMakeLists.txt Makefile* in the tree.
"""
import argparse
import re
import subprocess
import sys

SRC_EXT = {".c", ".h", ".cpp", ".hpp", ".cc", ".asm", ".x86", ".rc", ".tbl",
           ".inc"}
PROJ_PAT = re.compile(r"(\.dsp|\.dsw|\.sln|\.vcproj|\.vcxproj)$|"
                      r"(^|/)CMakeLists\.txt$|(^|/)Makefile[^/]*$", re.I)
INC_RE = re.compile(rb'''^\s*[#%]\s*include\s+["'<]([^"'>]+)["'>]''',
                    re.M)
# Path-looking tokens inside project/build files.
REF_RE = re.compile(rb'[\w./\\-]+\.(?:c|h|cpp|hpp|cc|asm|x86|rc|tbl|inc)\b',
                    re.I)


def tracked_files():
    out = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True)
    return [p for p in out.stdout.decode("utf-8").split("\0") if p]


def read(path):
    with open(path, "rb") as f:
        return f.read()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--root", action="append", default=[],
                    help="additional root build file(s)")
    args = ap.parse_args()

    files = tracked_files()
    sources = [p for p in files
               if "." in p and p[p.rfind("."):].lower() in SRC_EXT]
    by_base = {}
    for p in sources:
        by_base.setdefault(p.rsplit("/", 1)[-1].lower(), []).append(p)

    roots = [p for p in files if PROJ_PAT.search(p)] + args.root
    if not roots:
        print("no build roots found", file=sys.stderr)
        return 1

    reached, queue = set(), []

    def mark(base_lower):
        for p in by_base.get(base_lower, []):
            if p not in reached:
                reached.add(p)
                queue.append(p)

    for r in roots:
        data = read(r)
        for m in REF_RE.finditer(data):
            token = m.group(0).decode("latin-1").replace("\\", "/")
            mark(token.rsplit("/", 1)[-1].lower())

    while queue:
        p = queue.pop()
        data = read(p)
        if b"\0" in data:
            continue
        for m in INC_RE.finditer(data):
            inc = m.group(1).decode("latin-1").replace("\\", "/")
            mark(inc.rsplit("/", 1)[-1].lower())

    unref = sorted(set(sources) - reached)
    print(f"# roots: {len(roots)}, sources: {len(sources)}, "
          f"reached: {len(reached)}, unreferenced: {len(unref)}")
    for p in unref:
        print(f"UNREFERENCED {p}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

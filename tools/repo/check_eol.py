#!/usr/bin/env python3
"""Report / enforce line endings for git-tracked text files.

Usage:
  python tools/repo/check_eol.py --report    # census: LF / CRLF / MIXED
  python tools/repo/check_eol.py --enforce   # fail unless policy holds

Policy (mirrors .gitattributes): LF everywhere, CRLF for CRLF_EXT,
MIXED never allowed. Binary files (NUL byte) and paths that .gitattributes
marks binary (no text conversion), such as byte-exact evidence captured from
CP/M, are skipped. Exit 0/1.
"""
import argparse
import subprocess
import sys

CRLF_EXT = {".dsp", ".dsw", ".sln", ".vcproj", ".vcxproj", ".filters", ".bat"}


def tracked_files():
    out = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True)
    return [p for p in out.stdout.decode("utf-8").split("\0") if p]


def binary_attribute_paths(paths):
    """Return the paths whose .gitattributes unset the text attribute."""
    if not paths:
        return set()
    out = subprocess.run(
        ["git", "check-attr", "-z", "--stdin", "text"],
        input="\0".join(paths).encode("utf-8") + b"\0",
        capture_output=True,
        check=True,
    )
    fields = out.stdout.decode("utf-8").split("\0")
    result = set()
    for index in range(0, len(fields) - 2, 3):
        path, _, value = fields[index : index + 3]
        if value == "unset":
            result.add(path)
    return result


def eol_class(data: bytes) -> str:
    crlf = data.count(b"\r\n")
    lf = data.count(b"\n") - crlf
    cr = data.count(b"\r") - crlf
    if cr:
        return "MIXED"  # bare CR is always wrong here
    if crlf and lf:
        return "MIXED"
    if crlf:
        return "CRLF"
    return "LF"  # includes files with no newline at all


def ext_of(path: str) -> str:
    dot = path.rfind(".")
    return path[dot:].lower() if dot >= 0 else ""


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--enforce", action="store_true")
    args = ap.parse_args()

    violations = 0
    paths = tracked_files()
    binary = binary_attribute_paths(paths)
    for path in paths:
        if path in binary:
            continue
        with open(path, "rb") as f:
            data = f.read()
        if b"\0" in data:
            continue
        cls = eol_class(data)
        want = "CRLF" if ext_of(path) in CRLF_EXT else "LF"
        if args.report:
            print(f"{cls:5s} {path}")
        if args.enforce and cls != want:
            print(f"VIOLATION [{cls}, want {want}] {path}", file=sys.stderr)
            violations += 1
    if args.enforce:
        print(f"{violations} violation(s)", file=sys.stderr)
        return 1 if violations else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())

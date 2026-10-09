#!/usr/bin/env python3
"""Classify / enforce text-file encoding for all git-tracked files.

Usage:
  python tools/repo/check_encoding.py --report          # census only
  python tools/repo/check_encoding.py --expect cp932    # fail on non-CP932 text
  python tools/repo/check_encoding.py --expect utf8     # fail on non-UTF-8 or BOM
  python tools/repo/check_encoding.py --expect utf8

Classification per file: BINARY, ASCII, UTF8, UTF8-BOM, CP932, UNKNOWN.
ASCII always satisfies both --expect modes. Exit 0 clean, 1 violations.
"""
import argparse
import subprocess
import sys

BOM = b"\xef\xbb\xbf"


def tracked_files():
    out = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True)
    return [p for p in out.stdout.decode("utf-8").split("\0") if p]


def classify(data: bytes) -> str:
    if b"\0" in data:
        return "BINARY"
    if data.startswith(BOM):
        return "UTF8-BOM"
    try:
        data.decode("ascii")
        return "ASCII"
    except UnicodeDecodeError:
        pass
    is_utf8 = False
    try:
        data.decode("utf-8")
        is_utf8 = True
    except UnicodeDecodeError:
        pass
    is_cp932 = False
    try:
        # Require exact round-trip so lenient decoding does not lie to us.
        if data.decode("cp932").encode("cp932") == data:
            is_cp932 = True
    except UnicodeError:
        pass
    if is_utf8 and not is_cp932:
        return "UTF8"
    if is_cp932 and not is_utf8:
        return "CP932"
    if is_utf8 and is_cp932:
        return "UTF8"  # pure single-byte overlap already caught by ASCII
    return "UNKNOWN"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--expect", choices=["cp932", "utf8"])
    ap.add_argument(
        "--exclude",
        action="append",
        default=[],
        metavar="PREFIX",
        help="skip paths with this prefix in --expect mode; repeatable",
    )
    args = ap.parse_args()

    ok = {"cp932": {"ASCII", "CP932"}, "utf8": {"ASCII", "UTF8"}}
    violations = 0
    for path in tracked_files():
        if args.expect and any(path.startswith(prefix) for prefix in args.exclude):
            continue
        try:
            with open(path, "rb") as f:
                data = f.read()
        except OSError as e:
            print(f"READ-ERROR  {path}  ({e})")
            violations += 1
            continue
        cls = classify(data)
        if args.report:
            print(f"{cls:9s} {path}")
        if args.expect and cls != "BINARY" and cls not in ok[args.expect]:
            print(f"VIOLATION [{cls}] {path}", file=sys.stderr)
            violations += 1
    if args.expect:
        print(f"{violations} violation(s)", file=sys.stderr)
        return 1 if violations else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())

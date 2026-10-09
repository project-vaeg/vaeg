#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Check or apply the canonical M92 C/C++ source format."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys

GIT_ENV = os.environ.copy()
GIT_ENV["GIT_CONFIG_GLOBAL"] = "/dev/null"
GIT_ENV["GIT_CONFIG_SYSTEM"] = "/dev/null"
ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "tools/repo/clang_format_files.txt"
FORMATTER = "clang-format-mp-22"
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"}
CHUNK_SIZE = 64


def fail(message: str) -> int:
    print(f"clang-format check: {message}", file=sys.stderr)
    return 1


def load_manifest() -> list[str]:
    paths = []
    for raw in MANIFEST.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line and not line.startswith("#"):
            paths.append(line)
    return paths


def validate_manifest(paths: list[str]) -> None:
    if not paths:
        raise ValueError("manifest is empty")
    if paths != sorted(paths):
        raise ValueError("manifest paths are not sorted")
    if len(paths) != len(set(paths)):
        raise ValueError("manifest contains duplicate paths")
    for item in paths:
        path = Path(item)
        if path.is_absolute() or ".." in path.parts:
            raise ValueError(f"path is not repository-relative: {item}")
        if path.suffix.lower() not in SOURCE_SUFFIXES:
            raise ValueError(f"unsupported source suffix: {item}")
        if path.parts[0] in {"build", "external"}:
            raise ValueError(f"excluded tree is present: {item}")
        if not (ROOT / path).is_file():
            raise ValueError(f"file does not exist: {item}")
    tracked = subprocess.run(
        ["git", "ls-files", "--error-unmatch", "--", *paths],
        cwd=ROOT,
        text=True,
        capture_output=True,
        env=GIT_ENV,
    )
    if tracked.returncode != 0:
        raise ValueError("manifest contains an untracked path")


def validate_formatter() -> None:
    result = subprocess.run(
        [FORMATTER, "--version"], cwd=ROOT, text=True, capture_output=True
    )
    if result.returncode != 0:
        raise RuntimeError(f"cannot run {FORMATTER}: {result.stderr.strip()}")
    version = result.stdout.strip()
    if not re.search(r"clang-format version 22(?:\.|$)", version):
        raise RuntimeError(f"expected clang-format 22, got: {version}")

    config = subprocess.run(
        [FORMATTER, "--style=file", "--dump-config"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout
    keep_empty = re.search(
        r"KeepEmptyLines:\n(?:[ ]+.*\n)*?[ ]+AtStartOfBlock:[ ]+(\w+)", config
    )
    if keep_empty is None or keep_empty.group(1) != "false":
        raise RuntimeError(".clang-format must disable empty lines at block starts")


def run_formatter(paths: list[str], apply: bool) -> int:
    failed = False
    for start in range(0, len(paths), CHUNK_SIZE):
        chunk = paths[start : start + CHUNK_SIZE]
        if apply:
            command = [FORMATTER, "-i", "--style=file", "--fallback-style=none", *chunk]
        else:
            command = [
                FORMATTER,
                "--dry-run",
                "--Werror",
                "--style=file",
                "--fallback-style=none",
                *chunk,
            ]
        result = subprocess.run(command, cwd=ROOT)
        failed = failed or result.returncode != 0
    return 1 if failed else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply", action="store_true", help="rewrite every manifest file in place"
    )
    args = parser.parse_args()

    try:
        paths = load_manifest()
        validate_manifest(paths)
        validate_formatter()
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as exc:
        return fail(str(exc))

    result = run_formatter(paths, args.apply)
    if result == 0:
        action = "formatted" if args.apply else "properly formatted"
        print(f"clang-format check: {len(paths)} files {action} with {FORMATTER}")
    return result


if __name__ == "__main__":
    raise SystemExit(main())

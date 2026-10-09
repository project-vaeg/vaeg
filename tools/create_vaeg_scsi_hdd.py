#!/usr/bin/env python3
"""Create a complete VAEG VHD1.00 SCSI image through the native creator."""
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
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


def default_executable(root: Path) -> Path:
    candidates = (
        root / "build/linux-ci-clang/sdl2/vaeg",
        root / "build/linux-release/sdl2/vaeg",
        root / "build/mingw-cross/sdl2/vaeg.exe",
        root / "build/macos-release/sdl2/vaeg",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError("no built VAEG executable found; use --executable")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--size-mib", type=int)
    parser.add_argument("--block-count", type=int)
    parser.add_argument("--block-size", type=int, default=256)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--executable", type=Path)
    args = parser.parse_args(argv)
    if args.size_mib is None and args.block_count is None:
        args.size_mib = 40
    if args.size_mib is not None and args.block_count is not None:
        parser.error("--size-mib and --block-count are mutually exclusive")
    root = Path(__file__).resolve().parents[1]
    try:
        executable = args.executable or default_executable(root)
    except FileNotFoundError as exc:
        parser.error(str(exc))
    command = [str(executable), "--create-scsi-hdd", "--output", str(args.output),
               "--block-size", str(args.block_size)]
    if args.size_mib is not None:
        command += ["--size-mib", str(args.size_mib)]
    else:
        command += ["--block-count", str(args.block_count)]
    if args.force:
        command.append("--force")
    completed = subprocess.run(command, check=False)
    if completed.returncode != 0:
        return completed.returncode
    stat = args.output.stat()
    allocated = getattr(stat, "st_blocks", 0) * 512
    print(f"logical_size={stat.st_size}")
    print(f"allocated_size={allocated}")
    print(f"sparse={allocated < stat.st_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

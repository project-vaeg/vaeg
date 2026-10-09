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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Build a local Zundamon-orbit disk with a deterministic FAT directory time."""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path


FIXED_FAT_DATE = ((2026 - 1980) << 9) | (1 << 5) | 1
FIXED_FAT_TIME = 0


def load_pcengine_disk():
    path = Path(__file__).resolve().parents[3] / "tools" / "pc88va" / "pcengine_disk.py"
    spec = importlib.util.spec_from_file_location("zundamon_pcengine_disk", path)
    if spec is None or spec.loader is None:
        raise RuntimeError("ZUNDAMON_DISK_TOOL_IMPORT")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fixed_fat_now() -> tuple[int, int]:
    return FIXED_FAT_TIME, FIXED_FAT_DATE


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--payload", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    pcengine_disk = load_pcengine_disk()
    pcengine_disk.fat_now = fixed_fat_now
    try:
        pcengine_disk.create_vanilla(args.source, args.output)
        pcengine_disk.install_payload(args.output, args.payload)
    except (pcengine_disk.DiskError, OSError) as error:
        parser.exit(1, f"error: {error}\n")
    print("ZUNDAMON_DISK_BUILD_PASS fat_timestamp=2026-01-01T00:00:00")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

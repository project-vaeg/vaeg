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

"""Independently validate the generated M98s NASM orbit include."""

from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path

ENTRY = re.compile(r"^\s*dw\s+(-?\d+),\s*(-?\d+)\s*;\s*phase\s+(\d+)$")


class OrbitError(ValueError):
    pass


def inspect(path: Path, width: int = 11, height: int = 9,
            anchor_x: int = 5, anchor_y: int = 4,
            radius_x: int = 96, radius_y: int = 48):
    try:
        raw = path.read_bytes()
        text = raw.decode("ascii")
    except (OSError, UnicodeError) as error:
        raise OrbitError("M98S_TABLE_READ") from error
    entries = []
    phases = []
    for line in text.splitlines():
        match = ENTRY.match(line)
        if match:
            entries.append((int(match.group(1)), int(match.group(2))))
            phases.append(int(match.group(3)))
    if len(entries) != 64 or phases != list(range(64)):
        raise OrbitError("M98S_TABLE_PHASES")
    if entries[0] != (radius_x, 0) or entries[16] != (0, radius_y):
        raise OrbitError("M98S_TABLE_CARDINAL")
    if entries[32] != (-radius_x, 0) or entries[48] != (0, -radius_y):
        raise OrbitError("M98S_TABLE_CARDINAL")
    for phase, (dx, dy) in enumerate(entries):
        if not -32768 <= dx <= 32767 or not -32768 <= dy <= 32767:
            raise OrbitError("M98S_TABLE_COMPONENT")
        if abs(dx) > radius_x or abs(dy) > radius_y:
            raise OrbitError("M98S_TABLE_RADIUS")
        destination = (160 + dx - anchor_x, 100 + dy - anchor_y)
        x, y = destination
        if x < 0 or y < 0 or x + width > 320 or y + height > 200:
            raise OrbitError("M98S_TABLE_BOUNDS")
        if entries[(phase + 1) & 63] == (dx, dy):
            raise OrbitError("M98S_TABLE_DUPLICATE")
    # First quadrant moves from right toward bottom in screen coordinates.
    if not all(entries[index + 1][0] <= entries[index][0]
               and entries[index + 1][1] >= entries[index][1]
               for index in range(16)):
        raise OrbitError("M98S_TABLE_DIRECTION")
    for phase, (dx, dy) in enumerate(entries):
        if entries[(phase + 32) & 63] != (-dx, -dy):
            raise OrbitError("M98S_TABLE_OPPOSITE")
        if entries[(-phase) & 63] != (dx, -dy):
            raise OrbitError("M98S_TABLE_QUARTER_SYMMETRY")
        if entries[(32 - phase) & 63] != (-dx, dy):
            raise OrbitError("M98S_TABLE_QUARTER_SYMMETRY")
    return raw, tuple(entries)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    args = parser.parse_args()
    try:
        raw, entries = inspect(args.input)
    except OrbitError as error:
        print(error)
        return 1
    print(f"M98S_ORBIT_VALIDATION_PASS phases={len(entries)} "
          f"sha256={hashlib.sha256(raw).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

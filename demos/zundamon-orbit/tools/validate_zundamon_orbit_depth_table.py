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

"""Independently validate the generated M98t depth-table include."""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402

OFFSET = re.compile(
    r"^\s*dw\s+(-?\d+),\s*(-?\d+)\s*;\s*phase\s+(\d+)\s+offset$")
STATE = re.compile(
    r"^\s*db\s+(\d+),\s*(\d+),\s*(-?\d+),\s*0\s*;\s*phase\s+(\d+)\s+state$")
HUD_RECT = (4, 4, 70, 20)
EXPECTED_SCALES = (
    16,16,17,18,19,20,21,22,23,24,25,25,26,27,28,29,
    30,29,28,27,26,25,25,24,23,22,21,20,19,18,17,16,
    15,15,14,13,12,11,10,9,8,7,6,6,5,4,3,2,
    1,2,3,4,5,6,6,7,8,9,10,11,12,13,14,15,
)


class DepthTableError(ValueError):
    pass


@dataclass(frozen=True)
class Entry:
    phase: int
    dx: int
    dy: int
    scale_id: int
    depth_rank: int


def round29(q: int) -> int:
    return (29 * q + 16) // 32


def expected_scale(phase: int) -> int:
    if 16 <= phase <= 48:
        return 30 - round29(phase - 16)
    return 1 + round29((phase - 48) % 64)


def intersects(left, right) -> bool:
    return (left[0] < right[2] and right[0] < left[2]
            and left[1] < right[3] and right[1] < left[3])


def inspect(table_path: Path, atlas_path: Path, radius_x: int = 96,
            radius_y: int = 48):
    if not (1 <= radius_x <= 32767 and 1 <= radius_y <= 32767):
        raise DepthTableError("M98Y_RADIUS_RANGE")
    try:
        raw = table_path.read_bytes()
        text = raw.decode("ascii")
    except (OSError, UnicodeError) as error:
        raise DepthTableError("M98T_TABLE_READ") from error
    offsets = []
    states = []
    for line in text.splitlines():
        if match := OFFSET.match(line):
            offsets.append(tuple(map(int, match.groups())))
        elif match := STATE.match(line):
            states.append(tuple(map(int, match.groups())))
    if len(offsets) != 64 or len(states) != 64:
        raise DepthTableError("M98T_TABLE_ENTRY_COUNT")
    entries = []
    for phase, (offset, state) in enumerate(zip(offsets, states)):
        dx, dy, offset_phase = offset
        state_phase, scale_id, depth_rank, comment_phase = state
        if (offset_phase, state_phase, comment_phase) != (phase, phase, phase):
            raise DepthTableError("M98T_TABLE_PHASE_ID")
        entries.append(Entry(phase, dx, dy, scale_id, depth_rank))
    entries = tuple(entries)
    scales = tuple(entry.scale_id for entry in entries)
    if scales != EXPECTED_SCALES or any(
            entry.scale_id != expected_scale(entry.phase) for entry in entries):
        raise DepthTableError("M98T_TABLE_SCALE_FORMULA")
    if any(entry.depth_rank != 2 * entry.scale_id - 31 for entry in entries):
        raise DepthTableError("M98T_TABLE_DEPTH_FORMULA")
    if ((entries[0].scale_id, entries[0].depth_rank) != (16, 1)
            or (entries[16].scale_id, entries[16].depth_rank) != (30, 29)
            or (entries[32].scale_id, entries[32].depth_rank) != (15, -1)
            or (entries[48].scale_id, entries[48].depth_rank) != (1, -29)):
        raise DepthTableError("M98T_TABLE_LANDMARK")
    expected_histogram = {scale: 2 for scale in range(1, 31)}
    expected_histogram.update({1: 1, 6: 4, 15: 3, 16: 3, 25: 4, 30: 1})
    if dict(Counter(scales)) != expected_histogram:
        raise DepthTableError("M98T_TABLE_HISTOGRAM")
    if sum(scales[index] != scales[(index + 1) & 63]
           for index in range(64)) != 58:
        raise DepthTableError("M98T_TABLE_CHANGE_EDGES")
    if ((entries[0].dx, entries[0].dy) != (radius_x, 0)
            or (entries[16].dx, entries[16].dy) != (0, radius_y)
            or (entries[32].dx, entries[32].dy) != (-radius_x, 0)
            or (entries[48].dx, entries[48].dy) != (0, -radius_y)):
        raise DepthTableError("M98T_TABLE_CARDINAL")
    for entry in entries:
        opposite = entries[(entry.phase + 32) & 63]
        if (opposite.dx, opposite.dy) != (-entry.dx, -entry.dy):
            raise DepthTableError("M98T_TABLE_ORBIT_SYMMETRY")
        following = entries[(entry.phase + 1) & 63]
        if (following.dx, following.dy) == (entry.dx, entry.dy):
            raise DepthTableError("M98T_TABLE_DUPLICATE_POSITION")
    try:
        atlas = atlas_format.read_regular_file(atlas_path)
        header, descriptors = atlas_format.inspect_bytes(atlas)
    except atlas_format.AtlasError as error:
        raise DepthTableError("M98T_TABLE_ATLAS") from error
    if header.required_bank_count != 1 or len(descriptors) != 30:
        raise DepthTableError("M98T_TABLE_ATLAS")
    if any(descriptors[index].width > descriptors[index + 1].width
           or descriptors[index].height > descriptors[index + 1].height
           for index in range(29)):
        raise DepthTableError("M98T_TABLE_SCALE_ORDER")
    rectangles = []
    for entry in entries:
        descriptor = descriptors[entry.scale_id - 1]
        x0 = 160 + entry.dx - descriptor.anchor_x
        y0 = 100 + entry.dy - descriptor.anchor_y
        rect = (x0, y0, x0 + descriptor.width, y0 + descriptor.height)
        if not (0 <= rect[0] < rect[2] <= 320
                and 0 <= rect[1] < rect[3] <= 200):
            raise DepthTableError("M98T_TABLE_BOUNDS")
        if intersects(rect, HUD_RECT):
            raise DepthTableError("M98T_TABLE_HUD_INTERSECTION")
        rectangles.append(rect)
    return raw, entries, tuple(rectangles), descriptors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--radius-x", type=int, default=96)
    parser.add_argument("--radius-y", type=int, default=48)
    args = parser.parse_args()
    try:
        raw, entries, _, _ = inspect(args.input, args.atlas,
                                     args.radius_x, args.radius_y)
    except DepthTableError as error:
        print(error)
        return 1
    print(f"M98T_DEPTH_TABLE_VALIDATION_PASS phases={len(entries)} "
          f"scales={len(set(entry.scale_id for entry in entries))} "
          f"sha256={hashlib.sha256(raw).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

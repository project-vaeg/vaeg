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

"""Generate the deterministic M98t phase, depth, and scale table."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import generate_zundamon_orbit_table as orbit  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402

PHASE_COUNT = 64
SCREEN_WIDTH = 320
SCREEN_HEIGHT = 200
CENTER_X = 160
CENTER_Y = 100
HUD_RECT = (4, 4, 70, 20)
M98S_RADIUS_X = 96
M98S_RADIUS_Y = 48


@dataclass(frozen=True)
class Entry:
    phase: int
    dx: int
    dy: int
    scale_id: int
    depth_rank: int


def round29(q: int) -> int:
    if not 0 <= q <= 32:
        raise ValueError("M98T_DEPTH_Q_RANGE")
    return (29 * q + 16) // 32


def scale_for_phase(phase: int) -> int:
    if not 0 <= phase < PHASE_COUNT:
        raise ValueError("M98T_PHASE_RANGE")
    if 16 <= phase <= 48:
        return 30 - round29(phase - 16)
    return 1 + round29((phase - 48) % PHASE_COUNT)


def depth_for_scale(scale_id: int) -> int:
    if not 1 <= scale_id <= 30:
        raise ValueError("M98T_SCALE_RANGE")
    return 2 * scale_id - 31


def rectangle(entry: Entry, descriptor) -> tuple[int, int, int, int]:
    x0 = CENTER_X + entry.dx - descriptor.anchor_x
    y0 = CENTER_Y + entry.dy - descriptor.anchor_y
    return x0, y0, x0 + descriptor.width, y0 + descriptor.height


def intersects(left: tuple[int, int, int, int],
               right: tuple[int, int, int, int]) -> bool:
    return (left[0] < right[2] and right[0] < left[2]
            and left[1] < right[3] and right[1] < left[3])


def generate_entries(radius_x: int, radius_y: int) -> tuple[Entry, ...]:
    offsets = orbit.generate(radius_x, radius_y)
    return tuple(Entry(phase, dx, dy, scale_for_phase(phase),
                       depth_for_scale(scale_for_phase(phase)))
                 for phase, (dx, dy) in enumerate(offsets))


def radius_pair_is_valid(entries: tuple[Entry, ...], descriptors) -> bool:
    positions = tuple((entry.dx, entry.dy) for entry in entries)
    if any(positions[(phase + 1) & 63] == positions[phase]
           for phase in range(PHASE_COUNT)):
        return False
    for entry in entries:
        descriptor = descriptors[entry.scale_id - 1]
        rect = rectangle(entry, descriptor)
        if not (0 <= rect[0] < rect[2] <= SCREEN_WIDTH
                and 0 <= rect[1] < rect[3] <= SCREEN_HEIGHT):
            return False
        if intersects(rect, HUD_RECT):
            return False
    return True


def select_radii(descriptors) -> tuple[int, int, int]:
    accepted = generate_entries(M98S_RADIUS_X, M98S_RADIUS_Y)
    if radius_pair_is_valid(accepted, descriptors):
        return M98S_RADIUS_X, M98S_RADIUS_Y, 0
    candidates = []
    for radius_x in range(1, M98S_RADIUS_X + 1):
        for radius_y in range(1, M98S_RADIUS_Y + 1):
            entries = generate_entries(radius_x, radius_y)
            if radius_pair_is_valid(entries, descriptors):
                candidates.append((radius_x * radius_y, radius_x, radius_y))
    if not candidates:
        raise ValueError("M98T_NO_SAFE_DEPTH_ELLIPSE")
    _, radius_x, radius_y = max(candidates)
    return radius_x, radius_y, int(radius_x != M98S_RADIUS_X) + int(
        radius_y != M98S_RADIUS_Y)


def encode_include(entries: tuple[Entry, ...], radius_x: int,
                   radius_y: int, radius_adjustments: int) -> bytes:
    lines = [
        "; Copyright (c) 2026 Nakata Maho",
        ";",
        "; Redistribution and use in source and binary forms, with or without",
        "; modification, are permitted provided that the following conditions are met:",
        "; 1. Redistributions of source code must retain the above copyright notice,",
        ";    this list of conditions and the following disclaimer.",
        "; 2. Redistributions in binary form must reproduce the above copyright notice,",
        ";    this list of conditions and the following disclaimer in the documentation",
        ";    and/or other materials provided with the distribution.",
        ";",
        "; THIS SOFTWARE IS PROVIDED BY THE AUTHOR \"AS IS\" AND ANY EXPRESS OR IMPLIED",
        "; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF",
        "; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO",
        "; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,",
        "; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,",
        "; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;",
        "; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,",
        "; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR",
        "; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF",
        "; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.",
        ";",
        "; Generated by generate_zundamon_orbit_depth_table.py.  Do not edit by hand.",
        f"%define DEPTH_TABLE_PHASE_COUNT {len(entries)}",
        f"%define DEPTH_TABLE_ENTRY_BYTES 8",
        f"%define DEPTH_TABLE_RADIUS_X {radius_x}",
        f"%define DEPTH_TABLE_RADIUS_Y {radius_y}",
        f"%define DEPTH_TABLE_RADIUS_ADJUSTMENTS {radius_adjustments}",
        "depth_orbit_entries:",
    ]
    for entry in entries:
        lines.append(
            f"    dw {entry.dx:4d}, {entry.dy:4d} ; phase {entry.phase:02d} offset")
        lines.append(
            f"    db {entry.phase:2d}, {entry.scale_id:2d}, {entry.depth_rank:3d}, 0 ; phase {entry.phase:02d} state")
    return ("\n".join(lines) + "\n").encode("ascii")


def validate_generation(entries: tuple[Entry, ...]) -> None:
    scales = tuple(entry.scale_id for entry in entries)
    expected = (
        16,16,17,18,19,20,21,22,23,24,25,25,26,27,28,29,
        30,29,28,27,26,25,25,24,23,22,21,20,19,18,17,16,
        15,15,14,13,12,11,10,9,8,7,6,6,5,4,3,2,
        1,2,3,4,5,6,6,7,8,9,10,11,12,13,14,15,
    )
    if scales != expected:
        raise ValueError("M98T_SCALE_SEQUENCE")
    histogram = Counter(scales)
    expected_histogram = {scale: 2 for scale in range(1, 31)}
    expected_histogram.update({1: 1, 6: 4, 15: 3, 16: 3, 25: 4, 30: 1})
    if dict(histogram) != expected_histogram:
        raise ValueError("M98T_SCALE_HISTOGRAM")
    changes = sum(scales[index] != scales[(index + 1) & 63]
                  for index in range(PHASE_COUNT))
    if changes != 58:
        raise ValueError("M98T_SCALE_CHANGE_EDGES")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--metadata", type=Path)
    args = parser.parse_args()
    if args.output.exists() or (args.metadata is not None and args.metadata.exists()):
        parser.error("refusing to overwrite generated output")
    contents = atlas_format.read_regular_file(args.atlas)
    header, descriptors = atlas_format.inspect_bytes(contents)
    if header.required_bank_count != 1 or len(descriptors) != 30:
        raise SystemExit("M98T_THIRTY_SCALE_ATLAS_INVALID")
    radius_x, radius_y, adjustments = select_radii(descriptors)
    entries = generate_entries(radius_x, radius_y)
    validate_generation(entries)
    encoded = encode_include(entries, radius_x, radius_y, adjustments)
    args.output.write_bytes(encoded)
    digest = hashlib.sha256(encoded).hexdigest()
    if args.metadata is not None:
        args.metadata.write_text(json.dumps({
            "schema": "zundamon-orbit-m98t-depth-table-v1",
            "phase_count": PHASE_COUNT,
            "radius_x": radius_x,
            "radius_y": radius_y,
            "radius_adjustments": adjustments,
            "sha256": digest,
            "scale_histogram": {str(key): value for key, value in
                                sorted(Counter(e.scale_id for e in entries).items())},
            "scale_change_edges": 58,
            "entries": [entry.__dict__ | {
                "rectangle": list(rectangle(entry, descriptors[entry.scale_id - 1]))
            } for entry in entries],
        }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"M98T_DEPTH_TABLE_GENERATION_PASS phases={PHASE_COUNT} "
          f"radius_x={radius_x} radius_y={radius_y} adjustments={adjustments} "
          f"sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

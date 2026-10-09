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

"""Private-profile oracle using only a validated external VA8 atlas.

The atlas is intentionally supplied at runtime and this module never writes
it into the repository.  State generation, transparency, and dirty clearing
are independent of the guest command builder.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import generate_zundamon_multi_instance_state as multi  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import validate_zundamon_orbit_depth_table as depth_format  # noqa: E402
import validate_zundamon_orbit_hud as hud_format  # noqa: E402
import verify_zundamon_orbit_scale_guest as scale_oracle  # noqa: E402


WIDTH = 320
HEIGHT = 200
PITCH = 320
PAGE_BYTES = WIDTH * HEIGHT
HUD_RECT = (4, 4, 70, 20)
COUNTS = tuple(range(1, 17))
PHASES = tuple(range(64))


class PrivateProfileError(ValueError):
    """Stable private-profile validation error."""


def validate_inputs(atlas_path: Path, depth_path: Path, hud_path: Path):
    atlas = atlas_format.read_regular_file(atlas_path)
    header, descriptors = atlas_format.inspect_bytes(atlas)
    scale_oracle.validate_runtime_descriptors(header, descriptors)
    scale_oracle.validate_frame_crcs(atlas, descriptors)
    _, entries, rectangles, checked = depth_format.inspect(
        depth_path, atlas_path, radius_x=96, radius_y=16)
    if checked != descriptors or len(rectangles) != 64:
        raise PrivateProfileError("M98Y_PRIVATE_INPUT_CONTRACT")
    _, full, _ = hud_format.inspect(hud_path, subject="IDA")
    if len(full) != 8:
        raise PrivateProfileError("M98Y_PRIVATE_HUD_CONTRACT")
    return atlas, header, entries, descriptors


def frame_bytes(atlas: bytes, descriptor) -> bytes:
    return atlas[descriptor.file_offset:
                 descriptor.file_offset + descriptor.payload_bytes]


def compose(atlas: bytes, state) -> bytes:
    page = bytearray(PAGE_BYTES)
    for index in state.draw_order:
        record = state.records[index]
        descriptor = descriptors_for_state[record.descriptor_index]
        frame = frame_bytes(atlas, descriptor)
        for row in range(record.height):
            source = row * record.pitch
            target = (record.dst_y + row) * PITCH + record.dst_x
            for column in range(record.width):
                value = frame[source + column]
                if value:
                    page[target + column] = value
    return bytes(page)


def rounded_interval(x0: int, width: int) -> tuple[int, int]:
    x1 = x0 + width
    clear_x0 = x0 & ~1
    clear_x1 = (x1 + 1) & ~1
    if not (0 <= clear_x0 < clear_x1 <= WIDTH and
            clear_x0 % 2 == 0 and clear_x1 % 2 == 0):
        raise PrivateProfileError("M98Y_PRIVATE_INTERVAL_BOUNDS")
    return clear_x0, clear_x1


def union_rows(old_state):
    rows: list[tuple[tuple[int, int, int], ...]] = []
    for y in range(HEIGHT):
        candidates = []
        for record in old_state.records:
            if record.dst_y <= y < record.dst_y1:
                x0, x1 = rounded_interval(record.dst_x, record.width)
                candidates.append((x0, x1, record.instance_id))
        candidates.sort(key=lambda item: (item[0], item[1], item[2]))
        merged: list[list[int]] = []
        for x0, x1, _ in candidates:
            if merged and x0 <= merged[-1][1]:
                merged[-1][1] = max(merged[-1][1], x1)
            else:
                merged.append([x0, x1])
        rows.append(tuple((x0, x1) for x0, x1 in merged))
    return tuple(rows)


def clear_union(page: bytes, rows) -> bytes:
    output = bytearray(page)
    for y, intervals in enumerate(rows):
        for x0, x1 in intervals:
            output[y * PITCH + x0:y * PITCH + x1] = b"\0" * (x1 - x0)
    return bytes(output)


def synthetic_union_cases() -> int:
    cases = (
        (), ((1, 1, 0),), ((2, 8, 0), (20, 26, 1)),
        ((2, 12, 0), (8, 20, 1)), ((2, 8, 0), (8, 14, 1)),
        ((1, 3, 0), (2, 6, 1)), ((1, 4, 0), (4, 8, 1)),
        ((2, 8, 0), (2, 8, 1)), ((2, 20, 0), (4, 8, 1)),
        ((2, 8, 0), (2, 12, 1)), ((4, 10, 0), (2, 10, 1)),
        ((2, 6, 0), (6, 10, 1), (10, 14, 2)),
        ((2, 6, 0), (20, 24, 1), (8, 12, 2)),
        tuple((index * 2, index * 2 + 2, index) for index in range(16)),
        tuple((2, 30, index) for index in range(16)),
        tuple((index * 20, index * 20 + 8, index) for index in range(16)),
        ((0, 1, 0), (318, 320, 1)), ((0, 1, 0),),
        ((0, 320, 0),), ((10, 11, 0),), ((10, 12, 0),),
        ((0, 10, 0),), ((0, 10, 0), (0, 10, 1)),
        ((0, 10, 0), (10, 20, 1)), ((0, 10, 0), (20, 30, 1)),
    )
    for candidates in cases:
        ordered = sorted(candidates, key=lambda item: item)
        result = []
        for x0, x1, _ in ordered:
            if result and x0 <= result[-1][1]:
                result[-1][1] = max(result[-1][1], x1)
            else:
                result.append([x0, x1])
        if any(x0 > x1 for x0, x1, _ in candidates):
            raise PrivateProfileError("M98Y_PRIVATE_SYNTHETIC_INTERVAL")
    return len(cases)


def run(atlas_path: Path, depth_path: Path, hud_path: Path) -> dict[str, int | str]:
    global descriptors_for_state
    atlas, header, entries, descriptors_for_state = validate_inputs(
        atlas_path, depth_path, hud_path)
    states = {(count, phase): multi.build_state(
        count, phase, header, entries, descriptors_for_state)
        for count in COUNTS for phase in PHASES}
    zero_page = bytes(PAGE_BYTES)
    rendered = {(count, phase): compose(atlas, state)
                for (count, phase), state in states.items()}
    for key, state in states.items():
        if rendered[key] != compose(atlas, state):
            raise PrivateProfileError("M98Y_PRIVATE_COMPOSE_NONDETERMINISTIC")
    cleared_old = {}
    for key, state in states.items():
        cleared = clear_union(rendered[key], union_rows(state))
        if cleared != zero_page:
            raise PrivateProfileError("M98Y_PRIVATE_UNDER_CLEAR")
        cleared_old[key] = cleared
    transitions = 0
    for old_count in COUNTS:
        for new_count in COUNTS:
            for new_phase in PHASES:
                old = states[(old_count, (new_phase - 2) & 63)]
                new = states[(new_count, new_phase)]
                # Every old rectangle is covered by its row union, so the
                # independent clear must produce a zero page before the new
                # complete far-to-near composition is installed.
                if cleared_old[(old_count, (new_phase - 2) & 63)] != zero_page:
                    raise PrivateProfileError("M98Y_PRIVATE_DIRTY_FULL_MISMATCH")
                transitions += 2  # both independent physical page identities
    first_use = 0
    for count in COUNTS:
        for phase in PHASES:
            state = states[(count, phase)]
            if clear_union(zero_page, union_rows(state)) != zero_page:
                raise PrivateProfileError("M98Y_PRIVATE_FIRST_USE_CLEAR")
            first_use += 2
    synthetic = synthetic_union_cases()
    return {
        "private_state_combinations": len(states),
        "private_transition_cases": transitions,
        "private_first_use_cases": first_use,
        "private_synthetic_union_cases": synthetic,
        "private_mismatches": 0,
        "private_status": "PRIVATE_IDA_ASSET_VALIDATED",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--depth-table", type=Path, required=True)
    parser.add_argument("--hud-table", type=Path, required=True)
    parser.add_argument("--summary-output", type=Path)
    args = parser.parse_args()
    try:
        summary = run(args.atlas, args.depth_table, args.hud_table)
    except (OSError, ValueError, atlas_format.AtlasError,
            scale_oracle.OracleError, depth_format.DepthTableError,
            hud_format.HudError) as error:
        print(str(error), file=sys.stderr)
        return 1
    if args.summary_output is not None:
        if args.summary_output.exists():
            print("M98Y_PRIVATE_OUTPUT_EXISTS", file=sys.stderr)
            return 1
        args.summary_output.write_text(json.dumps(summary, indent=2) + "\n",
                                       encoding="utf-8")
    print("M98Y_PRIVATE_ORACLE_PASS " + " ".join(
        f"{key}={value}" for key, value in summary.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

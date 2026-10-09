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

"""Generate the public minimal-font FPS/ZUNDAMON G0 HUD tiles."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

FOREGROUND = 0xFF
BACKGROUND = 0x01
HUD_WIDTH = 66
HUD_HEIGHT = 16
CELL_WIDTH = 6
CELL_HEIGHT = 8
FPS_WIDTH = 18
FPS_HEIGHT = 8
FPS_FIELDS = ("60 ", "30 ", "20 ", "15 ", "12 ", "10 ", "8.6", "7.5")
DEFAULT_COUNT_MAX = 16
COUNT_WIDTH = 12
STATUS_SPEED_FIELDS = tuple(f"SPD:{label} " for label in
                            ("0.25X", "0.50X", "0.75X", "1.00X",
                             "1.25X", "1.50X", "2.00X", "3.00X",
                             "4.00X", "5.00X", "6.00X", "7.00X", "8.00X"))
IDA64_STATUS_SPEED_FIELDS = tuple(
    f"SPD:{index / 4:.2f}X " for index in range(4, 17))
STATUS_DISTANCE_FIELDS = tuple(f"DIST:{bias:+d}" for bias in range(-4, 5))
STATUS_LOOK_FIELDS = tuple(f"LOOK:{level:+d}" for level in range(-4, 5))
STATUS_RADIUS_FIELDS = tuple(f"RAD:{label}" for label in
                            ("0.50X", "0.63X", "0.75X", "0.88X",
                             "1.00X", "1.13X", "1.25X", "1.38X", "1.50X"))

# Public, task-authored 5x7 glyphs.  No ROM or firmware font is consulted.
GLYPHS = {
    " ": (".....",) * 7,
    "+": (".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."),
    "-": (".....", ".....", ".....", "#####", ".....", ".....", "....."),
    ".": (".....", ".....", ".....", ".....", ".....", ".##..", ".##.."),
    ":": (".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."),
    "0": (".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."),
    "1": ("..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."),
    "2": (".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"),
    "3": ("####.", "....#", "....#", ".###.", "....#", "....#", "####."),
    "4": ("...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."),
    "5": ("#####", "#....", "#....", "####.", "....#", "....#", "####."),
    "6": (".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."),
    "7": ("#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."),
    "8": (".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."),
    "9": (".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."),
    "A": (".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"),
    "C": (".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."),
    "D": ("####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."),
    "F": ("#####", "#....", "#....", "####.", "#....", "#....", "#...."),
    "I": (".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."),
    "K": ("#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"),
    "L": ("#....", "#....", "#....", "#....", "#....", "#....", "#####"),
    "M": ("#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"),
    "N": ("#...#", "##..#", "##..#", "#.#.#", "#..##", "#..##", "#...#"),
    "O": (".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."),
    "P": ("####.", "#...#", "#...#", "####.", "#....", "#....", "#...."),
    "R": ("####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"),
    "S": (".####", "#....", "#....", ".###.", "....#", "....#", "####."),
    "T": ("#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."),
    "U": ("#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."),
    "X": ("#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"),
    "Z": ("#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"),
}


def render_text(text: str) -> bytes:
    result = bytearray(len(text) * CELL_WIDTH * CELL_HEIGHT)
    row_width = len(text) * CELL_WIDTH
    for cell, character in enumerate(text):
        glyph = GLYPHS[character]
        for y in range(CELL_HEIGHT):
            pattern = glyph[y] if y < 7 else "....."
            for x in range(CELL_WIDTH):
                lit = x < 5 and pattern[x] == "#"
                result[y * row_width + cell * CELL_WIDTH + x] = (
                    FOREGROUND if lit else BACKGROUND)
    return bytes(result)


def render_full(field: str, subject: str = "ZUNDAMON") -> bytes:
    line1 = render_text(f"FPS: {field}   ")
    if subject == "ZUNDAMON":
        line2_text = "ZUNDAMON: 1"
    elif subject == "IDA":
        line2_text = "IDA CNT:  1"
    else:
        raise ValueError("M98Y_HUD_SUBJECT")
    line2 = render_text(line2_text)
    if len(line1) != HUD_WIDTH * CELL_HEIGHT or len(line2) != len(line1):
        raise ValueError("M98T_HUD_LAYOUT")
    return line1 + line2


def emit_bytes(lines: list[str], label: str, data: bytes, width: int) -> None:
    lines.append(f"{label}:")
    for offset in range(0, len(data), width):
        row = data[offset:offset + width]
        lines.append("    db " + ",".join(f"0x{value:02x}" for value in row))


def encode_include(subject: str = "ZUNDAMON", count_max: int = DEFAULT_COUNT_MAX) -> tuple[bytes, tuple[bytes, ...], tuple[bytes, ...]]:
    if not 1 <= count_max <= 64:
        raise ValueError("M98X_COUNT_MAX")
    count_fields = tuple(f"{count:>2}" for count in range(1, count_max + 1))
    full_tiles = tuple(render_full(field, subject) for field in FPS_FIELDS)
    fps_tiles = tuple(render_text(field) for field in FPS_FIELDS)
    count_tiles = tuple(render_text(field) for field in count_fields)
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
        "; Generated by generate_zundamon_orbit_hud.py.  Do not edit by hand.",
        "%define HUD_TILE_COUNT 8",
        f"%define HUD_FULL_TILE_BYTES {HUD_WIDTH * HUD_HEIGHT}",
        f"%define HUD_FPS_TILE_BYTES {FPS_WIDTH * FPS_HEIGHT}",
        f"%define HUD_COUNT_TILE_COUNT {len(count_fields)}",
        f"%define HUD_COUNT_TILE_BYTES {COUNT_WIDTH * CELL_HEIGHT}",
        f"%define HUD_FOREGROUND 0x{FOREGROUND:02x}",
        f"%define HUD_BACKGROUND 0x{BACKGROUND:02x}",
    ]
    if subject == "IDA":
        lines.append("%define HUD_PROFILE_IDA 1")
    for index, data in enumerate(full_tiles, 1):
        emit_bytes(lines, f"hud_full_tile_v{index}", data, HUD_WIDTH)
    lines.append("hud_full_tile_pointers:")
    lines.append("    dw " + ",".join(f"hud_full_tile_v{index}" for index in range(1, 9)))
    for index, data in enumerate(fps_tiles, 1):
        emit_bytes(lines, f"hud_fps_tile_v{index}", data, FPS_WIDTH)
    lines.append("hud_fps_tile_pointers:")
    lines.append("    dw " + ",".join(f"hud_fps_tile_v{index}" for index in range(1, 9)))
    for field, data in zip(count_fields, count_tiles):
        label = field.strip()
        emit_bytes(lines, f"hud_count_tile_{label}", data, COUNT_WIDTH)
    lines.append("hud_count_tile_pointers:")
    lines.append("    dw " + ",".join(
        f"hud_count_tile_{count}" for count in range(1, count_max + 1)))
    return ("\n".join(lines) + "\n").encode("ascii"), full_tiles, fps_tiles


def encode_status_include(speed_fields=STATUS_SPEED_FIELDS) -> bytes:
    """Encode the generic M98z status tiles separately from the legacy HUD."""
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
        "; Generated by generate_zundamon_orbit_hud.py.  Do not edit by hand.",
        "%define HUD_STATUS_SPEED_COUNT 13",
        "%define HUD_STATUS_SPEED_WIDTH 60",
        "%define HUD_STATUS_SPEED_HEIGHT 8",
        "%define HUD_STATUS_DISTANCE_COUNT 9",
        "%define HUD_STATUS_DISTANCE_WIDTH 42",
        "%define HUD_STATUS_DISTANCE_HEIGHT 8",
        "%define HUD_STATUS_LOOK_COUNT 9",
        "%define HUD_STATUS_LOOK_WIDTH 42",
        "%define HUD_STATUS_LOOK_HEIGHT 8",
        "%define HUD_STATUS_RADIUS_COUNT 9",
        "%define HUD_STATUS_RADIUS_WIDTH 54",
        "%define HUD_STATUS_RADIUS_HEIGHT 8",
    ]
    status_sets = (
        (speed_fields, "hud_status_speed_tile", 60),
        (STATUS_DISTANCE_FIELDS, "hud_status_distance_tile", 42),
        (STATUS_LOOK_FIELDS, "hud_status_look_tile", 42),
        (STATUS_RADIUS_FIELDS, "hud_status_radius_tile", 54),
    )
    for fields, prefix, width in status_sets:
        for index, field in enumerate(fields):
            emit_bytes(lines, f"{prefix}_{index}", render_text(field), width)
        lines.append(f"{prefix}_pointers:")
        lines.append("    dw " + ",".join(
            f"{prefix}_{index}" for index in range(len(fields))))
    return ("\n".join(lines) + "\n").encode("ascii")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--metadata", type=Path)
    parser.add_argument("--status-output", type=Path)
    parser.add_argument("--speed-profile", choices=("public", "ida64"),
                        default="public")
    parser.add_argument("--subject", choices=("ZUNDAMON", "IDA"),
                        default="ZUNDAMON")
    parser.add_argument("--count-max", type=int, default=DEFAULT_COUNT_MAX,
                        help="highest runtime count represented in the field")
    args = parser.parse_args()
    if (args.output.exists() or (args.metadata is not None and args.metadata.exists())
            or (args.status_output is not None and args.status_output.exists())):
        parser.error("refusing to overwrite generated output")
    encoded, full_tiles, fps_tiles = encode_include(args.subject, args.count_max)
    count_fields = tuple(f"{count:>2}" for count in range(1, args.count_max + 1))
    count_tiles = tuple(render_text(field) for field in count_fields)
    args.output.write_bytes(encoded)
    if args.status_output is not None:
        speed_fields = (IDA64_STATUS_SPEED_FIELDS if args.speed_profile == "ida64"
                        else STATUS_SPEED_FIELDS)
        args.status_output.write_bytes(encode_status_include(speed_fields))
    digest = hashlib.sha256(encoded).hexdigest()
    if args.metadata is not None:
        args.metadata.write_text(json.dumps({
            "schema": "zundamon-orbit-m98x-hud-v1",
            "subject": args.subject,
            "font_provenance": "task-authored-public-5x7",
            "glyphs": sorted(GLYPHS),
            "foreground": FOREGROUND,
            "background": BACKGROUND,
            "hud_rect": [4, 4, 70, 20],
            "fps_value_rect": [34, 4, 52, 12],
            "fps_fields": list(FPS_FIELDS),
            "count_fields": list(count_fields),
            "count_value_rect": [58, 12, 70, 20],
            "full_tile_sha256": [hashlib.sha256(tile).hexdigest()
                                 for tile in full_tiles],
            "fps_tile_sha256": [hashlib.sha256(tile).hexdigest()
                                for tile in fps_tiles],
            "count_tile_sha256": [hashlib.sha256(tile).hexdigest()
                                  for tile in count_tiles],
            "include_sha256": digest,
        }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"M98X_HUD_GENERATION_PASS fps_fields={len(FPS_FIELDS)} "
          f"count_fields={len(count_fields)} "
          f"full_bytes={len(full_tiles[0])} fps_bytes={len(fps_tiles[0])} "
          f"sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

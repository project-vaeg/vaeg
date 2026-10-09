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

"""Independently validate generated M98x FPS/count HUD tiles and glyphs."""

from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path

DB = re.compile(r"^\s*db\s+(.+)$")
LABEL = re.compile(r"^([a-z0-9_]+):$")
FIELDS = ("60 ", "30 ", "20 ", "15 ", "12 ", "10 ", "8.6", "7.5")
DEFAULT_COUNT_MAX = 16
FG = 0xFF
BG = 0x01

GLYPHS = {
    " ": (0, 0, 0, 0, 0, 0, 0),
    "+": (0, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0),
    "-": (0, 0, 0, 0b11111, 0, 0, 0),
    ".": (0, 0, 0, 0, 0, 0b01100, 0b01100),
    ":": (0, 0b01100, 0b01100, 0, 0b01100, 0b01100, 0),
    "0": (0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110),
    "1": (0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110),
    "2": (0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111),
    "3": (0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110),
    "4": (0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010),
    "5": (0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110),
    "6": (0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110),
    "7": (0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000),
    "8": (0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110),
    "9": (0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110),
    "A": (0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001),
    "C": (0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110),
    "D": (0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110),
    "F": (0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000),
    "I": (0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110),
    "K": (0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001),
    "L": (0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111),
    "M": (0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001),
    "N": (0b10001, 0b11001, 0b11001, 0b10101, 0b10011, 0b10011, 0b10001),
    "O": (0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110),
    "P": (0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000),
    "R": (0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001),
    "S": (0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110),
    "T": (0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100),
    "U": (0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110),
    "X": (0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001),
    "Z": (0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111),
}


class HudError(ValueError):
    pass


def render(text: str) -> bytes:
    width = len(text) * 6
    output = bytearray(width * 8)
    for index, character in enumerate(text):
        rows = GLYPHS[character]
        for y in range(8):
            bits = rows[y] if y < 7 else 0
            for x in range(6):
                output[y * width + index * 6 + x] = (
                    FG if x < 5 and bits & (1 << (4 - x)) else BG)
    return bytes(output)


def parse_sections(path: Path) -> tuple[bytes, dict[str, bytes]]:
    try:
        raw = path.read_bytes()
        lines = raw.decode("ascii").splitlines()
    except (OSError, UnicodeError) as error:
        raise HudError("M98T_HUD_READ") from error
    sections: dict[str, bytearray] = {}
    current = None
    for line in lines:
        if match := LABEL.match(line):
            current = match.group(1)
            sections[current] = bytearray()
        elif current is not None and (match := DB.match(line)):
            try:
                sections[current].extend(int(value.strip(), 0)
                                         for value in match.group(1).split(","))
            except ValueError as error:
                raise HudError("M98T_HUD_BYTE") from error
    return raw, {key: bytes(value) for key, value in sections.items()}


def inspect(path: Path, subject: str = "ZUNDAMON",
            count_max: int = DEFAULT_COUNT_MAX):
    if subject not in ("ZUNDAMON", "IDA"):
        raise HudError("M98Y_HUD_SUBJECT")
    if not 1 <= count_max <= 64:
        raise HudError("M98X_COUNT_MAX")
    raw, sections = parse_sections(path)
    allowed = {BG, FG}
    full_tiles = []
    fps_tiles = []
    for divisor, field in enumerate(FIELDS, 1):
        full = sections.get(f"hud_full_tile_v{divisor}", b"")
        fps = sections.get(f"hud_fps_tile_v{divisor}", b"")
        expected_fps = render(field)
        label = "ZUNDAMON: 1" if subject == "ZUNDAMON" else "IDA CNT:  1"
        expected_full = render(f"FPS: {field}   ") + render(label)
        if len(full) != 1056 or full != expected_full:
            raise HudError("M98T_HUD_FULL_TILE")
        if len(fps) != 144 or fps != expected_fps:
            raise HudError("M98T_HUD_FPS_TILE")
        if set(full) - allowed or set(fps) - allowed:
            raise HudError("M98T_HUD_COLOR")
        if any(full[row * 66 + 30:row * 66 + 48]
               != fps[row * 18:(row + 1) * 18] for row in range(8)):
            raise HudError("M98T_HUD_VALUE_RECT")
        full_tiles.append(full)
        fps_tiles.append(fps)
    count_tiles = []
    count_fields = tuple(f"{count:>2}" for count in range(1, count_max + 1))
    for field in count_fields:
        count = sections.get(f"hud_count_tile_{field.strip()}", b"")
        if len(count) != 96 or count != render(field):
            raise HudError("M98X_HUD_COUNT_TILE")
        if set(count) - allowed:
            raise HudError("M98X_HUD_COUNT_COLOR")
        count_tiles.append(count)
    return raw, tuple(full_tiles), tuple(fps_tiles)


def inspect_count_tiles(path: Path, count_max: int = DEFAULT_COUNT_MAX) -> tuple[bytes, ...]:
    _, sections = parse_sections(path)
    if not 1 <= count_max <= 64:
        raise HudError("M98X_COUNT_MAX")
    count_fields = tuple(f"{count:>2}" for count in range(1, count_max + 1))
    tiles = tuple(sections.get(f"hud_count_tile_{field.strip()}", b"")
                  for field in count_fields)
    if any(tile != render(field) for tile, field in zip(tiles, count_fields)):
        raise HudError("M98X_HUD_COUNT_TILE")
    return tiles


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--subject", choices=("ZUNDAMON", "IDA"),
                        default="ZUNDAMON")
    parser.add_argument("--count-max", type=int, default=DEFAULT_COUNT_MAX)
    args = parser.parse_args()
    try:
        raw, full, fps = inspect(args.input, args.subject, args.count_max)
    except HudError as error:
        print(error)
        return 1
    count_tiles = inspect_count_tiles(args.input, args.count_max)
    print(f"M98X_HUD_VALIDATION_PASS full_tiles={len(full)} fps_tiles={len(fps)} "
          f"count_tiles={len(count_tiles)} "
          f"sha256={hashlib.sha256(raw).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

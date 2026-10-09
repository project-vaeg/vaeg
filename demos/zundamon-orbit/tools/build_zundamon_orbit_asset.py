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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Build the public M98b synthetic indexed-fixture asset."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path


FORMAT = "vaeg-zundamon-orbit-public-fixture-v1"
WIDTH = 23
HEIGHT = 19
TRANSPARENT_INDEX = 0
NEAR_BLACK_INDEX = 1
RESERVED_INDEX = 15
PALETTE_RGB = (
    (0, 0, 0),
    (8, 8, 8),
    (224, 64, 64),
    (64, 224, 64),
    (64, 64, 224),
    (224, 160, 64),
    (160, 64, 224),
    (64, 192, 224),
    (224, 64, 160),
    (96, 224, 160),
    (160, 224, 64),
    (224, 96, 192),
    (96, 160, 224),
    (224, 224, 160),
    (240, 240, 240),
    (0, 0, 0),
)
PALETTE_NAME = "zundamon-orbit-fixture-palette.bin"
PIXEL_NAME = "zundamon-orbit-fixture-indexed.bin"
MANIFEST_NAME = "zundamon-orbit-fixture.json"
FEATURES = {
    "diagonal_a": ((6, 6, 2), (7, 7, 2), (8, 8, 2), (9, 9, 2)),
    "diagonal_b": ((14, 6, 3), (13, 7, 3), (12, 8, 3)),
    "isolated": ((21, 2, 9), (1, 17, 12)),
    "near_black": ((5, 2, NEAR_BLACK_INDEX),),
    "transparent_holes": ((10, 8, TRANSPARENT_INDEX), (11, 9, TRANSPARENT_INDEX)),
}


class BuildError(Exception):
    """A deterministic public-fixture build failure."""


def pixel_offset(x: int, y: int) -> int:
    if not (0 <= x < WIDTH and 0 <= y < HEIGHT):
        raise BuildError(f"M98B_FIXTURE_COORDINATE: out-of-range coordinate {x},{y}")
    return y * WIDTH + x


def put(pixels: bytearray, x: int, y: int, value: int) -> None:
    if not (0 <= value < RESERVED_INDEX):
        raise BuildError(f"M98B_FIXTURE_INDEX: invalid visible index {value}")
    pixels[pixel_offset(x, y)] = value


def draw_line(pixels: bytearray, x0: int, y0: int, x1: int, y1: int, value: int) -> None:
    dx = abs(x1 - x0)
    sx = 1 if x0 < x1 else -1
    dy = -abs(y1 - y0)
    sy = 1 if y0 < y1 else -1
    error = dx + dy
    while True:
        put(pixels, x0, y0, value)
        if x0 == x1 and y0 == y1:
            return
        twice_error = 2 * error
        if twice_error >= dy:
            error += dy
            x0 += sx
        if twice_error <= dx:
            error += dx
            y0 += sy


def build_pixels() -> bytes:
    """Return an asymmetric public marker with required structural features."""
    pixels = bytearray(WIDTH * HEIGHT)
    outline = ((5, 2), (13, 1), (18, 6), (17, 12), (12, 16), (5, 15), (2, 9))
    for start, end in zip(outline, outline[1:] + outline[:1]):
        draw_line(pixels, start[0], start[1], end[0], end[1], NEAR_BLACK_INDEX)
    draw_line(pixels, 5, 5, 13, 13, 2)
    draw_line(pixels, 15, 5, 7, 13, 3)
    draw_line(pixels, 4, 10, 11, 4, 6)
    draw_line(pixels, 14, 11, 16, 14, 7)
    for feature in ("diagonal_a", "diagonal_b", "near_black"):
        for x, y, value in FEATURES[feature]:
            put(pixels, x, y, value)
    put(pixels, 10, 8, TRANSPARENT_INDEX)
    put(pixels, 11, 9, TRANSPARENT_INDEX)
    put(pixels, 21, 2, 9)
    put(pixels, 1, 17, 12)
    return bytes(pixels)


def build_palette() -> bytes:
    return bytes(channel for color in PALETTE_RGB for channel in color)


def digest(contents: bytes) -> str:
    return hashlib.sha256(contents).hexdigest()


def feature_manifest() -> dict[str, list[list[int]]]:
    return {name: [list(item) for item in points] for name, points in FEATURES.items()}


def build_manifest(palette: bytes, pixels: bytes) -> bytes:
    contents = {
        "format": FORMAT,
        "height": HEIGHT,
        "palette": {
            "bytes": len(palette),
            "encoding": "rgb888",
            "entries": len(PALETTE_RGB),
            "path": PALETTE_NAME,
            "sha256": digest(palette),
        },
        "pixels": {
            "bytes": len(pixels),
            "path": PIXEL_NAME,
            "sha256": digest(pixels),
        },
        "properties": {
            "features": feature_manifest(),
            "near_black_index": NEAR_BLACK_INDEX,
            "reserved_index": RESERVED_INDEX,
            "transparent_index": TRANSPARENT_INDEX,
        },
        "width": WIDTH,
    }
    return (json.dumps(contents, indent=2, sort_keys=True) + "\n").encode("utf-8")


def write_fixture(output: Path) -> tuple[Path, bytes]:
    if output.exists():
        raise BuildError(f"M98B_FIXTURE_OUTPUT_EXISTS: {output}")
    palette = build_palette()
    pixels = build_pixels()
    manifest = build_manifest(palette, pixels)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.mkdir()
    try:
        (output / PALETTE_NAME).write_bytes(palette)
        (output / PIXEL_NAME).write_bytes(pixels)
        (output / MANIFEST_NAME).write_bytes(manifest)
    except OSError as error:
        raise BuildError(f"M98B_FIXTURE_WRITE: {error}") from error
    return output, manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="new public fixture directory; it must not already exist")
    args = parser.parse_args(argv)
    try:
        output, manifest = write_fixture(args.output.resolve())
    except BuildError as error:
        print(error, file=sys.stderr)
        return 1
    print(f"M98B_FIXTURE_BUILD_PASS output={output}")
    print(f"manifest_sha256={digest(manifest)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

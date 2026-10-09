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

"""Build a complete synthetic M98d local-input bundle."""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

import build_zundamon_orbit_asset as asset
import validate_zundamon_orbit_manifest as manifest_validator


IMAGE_NAME = "source.bmp"
PALETTE_NAME = "palette.rgb"
MANIFEST_NAME = "input.json"
BMP_HEADER_SIZE = 54
PIXELS_PER_METER = 2835


class FixtureError(Exception):
    """A deterministic M98d synthetic-bundle build failure."""


def build_bmp32(width: int, height: int, indices: bytes, palette: bytes,
                top_down: bool = False) -> bytes:
    if len(indices) != width * height:
        raise FixtureError("M98D_FIXTURE_INDEX_LENGTH: indexed raster length differs")
    if len(palette) != 16 * 3:
        raise FixtureError("M98D_FIXTURE_PALETTE_LENGTH: palette length differs")
    rows = range(height) if top_down else range(height - 1, -1, -1)
    pixels = bytearray()
    for y in rows:
        for index in indices[y * width:(y + 1) * width]:
            palette_offset = index * 3
            red, green, blue = palette[palette_offset:palette_offset + 3]
            pixels.extend((blue, green, red, 255))
    file_size = BMP_HEADER_SIZE + len(pixels)
    file_header = struct.pack("<2sIHHI", b"BM", file_size, 0, 0, BMP_HEADER_SIZE)
    stored_height = -height if top_down else height
    dib_header = struct.pack(
        "<IiiHHIIiiII",
        40,
        width,
        stored_height,
        1,
        32,
        0,
        len(pixels),
        PIXELS_PER_METER,
        PIXELS_PER_METER,
        0,
        0,
    )
    return file_header + dib_header + bytes(pixels)


def build_manifest() -> bytes:
    value = {
        "anchor": {
            "space": "crop-top-left",
            "x": asset.WIDTH // 2,
            "y": asset.HEIGHT // 2,
        },
        "copyright": manifest_validator.COPYRIGHT,
        "crop": {
            "height": asset.HEIGHT,
            "width": asset.WIDTH,
            "x": 0,
            "y": 0,
        },
        "image": {
            "encoding": "bmp32",
            "path": IMAGE_NAME,
        },
        "license": manifest_validator.LICENSE,
        "palette": {
            "encoding": "rgb888",
            "entries": 16,
            "path": PALETTE_NAME,
            "reserved_index": asset.RESERVED_INDEX,
            "transparent_index": asset.TRANSPARENT_INDEX,
        },
        "schema": manifest_validator.SCHEMA_ID,
        "schema_version": manifest_validator.SCHEMA_VERSION,
        "transparency": {
            "background_rgb": list(asset.PALETTE_RGB[asset.TRANSPARENT_INDEX]),
            "method": "exact-rgb",
        },
    }
    manifest_validator.validate_manifest(value)
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def write_fixture(output: Path, top_down: bool = False) -> None:
    if output.exists():
        raise FixtureError("M98D_FIXTURE_OUTPUT_EXISTS: output directory exists")
    palette = asset.build_palette()
    image = build_bmp32(asset.WIDTH, asset.HEIGHT, asset.build_pixels(), palette,
                        top_down=top_down)
    manifest = build_manifest()
    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.mkdir()
        (output / IMAGE_NAME).write_bytes(image)
        (output / PALETTE_NAME).write_bytes(palette)
        (output / MANIFEST_NAME).write_bytes(manifest)
    except OSError as error:
        raise FixtureError("M98D_FIXTURE_WRITE: output could not be written") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="new synthetic bundle directory")
    parser.add_argument("--top-down", action="store_true",
                        help="write a negative-height top-down BMP")
    arguments = parser.parse_args(argv)
    try:
        write_fixture(arguments.output, top_down=arguments.top_down)
    except FixtureError as error:
        print(error, file=sys.stderr)
        return 1
    print("M98D_FIXTURE_BUILD_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

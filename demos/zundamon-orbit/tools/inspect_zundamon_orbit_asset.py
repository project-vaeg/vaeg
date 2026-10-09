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

"""Inspect the public M98b synthetic indexed-fixture asset."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

from build_zundamon_orbit_asset import (FEATURES, FORMAT, HEIGHT, MANIFEST_NAME,
                                        NEAR_BLACK_INDEX, PALETTE_NAME,
                                        PALETTE_RGB, PIXEL_NAME, RESERVED_INDEX,
                                        TRANSPARENT_INDEX, WIDTH, build_palette,
                                        build_pixels, feature_manifest)


class InspectionError(Exception):
    """A deterministic public-fixture inspection failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def digest(contents: bytes) -> str:
    return hashlib.sha256(contents).hexdigest()


def fail(code: str, detail: str) -> None:
    raise InspectionError(code, detail)


def read_bytes(path: Path, code: str) -> bytes:
    try:
        return path.read_bytes()
    except OSError as error:
        fail(code, str(error))


def load_manifest(directory: Path) -> dict[str, object]:
    try:
        contents = read_bytes(directory / MANIFEST_NAME, "M98B_FIXTURE_MANIFEST_READ")
        value = json.loads(contents.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        fail("M98B_FIXTURE_MANIFEST_JSON", str(error))
    if not isinstance(value, dict):
        fail("M98B_FIXTURE_MANIFEST_TYPE", "top-level value is not an object")
    canonical = (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")
    if contents != canonical:
        fail("M98B_FIXTURE_MANIFEST_CANONICAL", "manifest is not canonical JSON")
    return value


def mapping(value: object, code: str) -> dict[str, object]:
    if not isinstance(value, dict):
        fail(code, "value is not an object")
    return value


def expected_digest(manifest: dict[str, object], name: str, contents: bytes) -> None:
    section = mapping(manifest.get(name), f"M98B_FIXTURE_{name.upper()}_SECTION")
    expected = section.get("sha256")
    if not isinstance(expected, str):
        fail(f"M98B_FIXTURE_{name.upper()}_SHA", "missing sha256")
    if digest(contents) != expected:
        fail(f"M98B_FIXTURE_{name.upper()}_SHA", "digest mismatch")


def expected_integer(section: dict[str, object], field: str, expected: int, code: str) -> None:
    if section.get(field) != expected:
        fail(code, f"{field} is not {expected}")


def pixel_at(pixels: bytes, x: int, y: int) -> int:
    return pixels[y * WIDTH + x]


def verify_features(pixels: bytes) -> None:
    for feature, points in FEATURES.items():
        for x, y, value in points:
            if pixel_at(pixels, x, y) != value:
                fail("M98B_FIXTURE_FEATURE", f"{feature} differs at {x},{y}")
    for x, y, _ in FEATURES["isolated"]:
        for neighbour_y in range(y - 1, y + 2):
            for neighbour_x in range(x - 1, x + 2):
                if neighbour_x == x and neighbour_y == y:
                    continue
                if not (0 <= neighbour_x < WIDTH and 0 <= neighbour_y < HEIGHT):
                    continue
                if pixel_at(pixels, neighbour_x, neighbour_y) != TRANSPARENT_INDEX:
                    fail("M98B_FIXTURE_ISOLATED", f"opaque neighbour at {neighbour_x},{neighbour_y}")


def verify_near_black(palette: bytes) -> None:
    start = NEAR_BLACK_INDEX * 3
    if tuple(palette[start:start + 3]) != (8, 8, 8):
        fail("M98B_FIXTURE_NEAR_BLACK", "index 1 is not the public near-black color")


def inspect(directory: Path) -> None:
    manifest = load_manifest(directory)
    if manifest.get("format") != FORMAT:
        fail("M98B_FIXTURE_FORMAT", "unexpected format")
    if manifest.get("width") != WIDTH or manifest.get("height") != HEIGHT:
        fail("M98B_FIXTURE_DIMENSIONS", "unexpected dimensions")
    palette = read_bytes(directory / PALETTE_NAME, "M98B_FIXTURE_PALETTE_READ")
    pixels = read_bytes(directory / PIXEL_NAME, "M98B_FIXTURE_PIXEL_READ")
    palette_section = mapping(manifest.get("palette"), "M98B_FIXTURE_PALETTE_SECTION")
    pixel_section = mapping(manifest.get("pixels"), "M98B_FIXTURE_PIXEL_SECTION")
    expected_integer(palette_section, "bytes", len(PALETTE_RGB) * 3,
                     "M98B_FIXTURE_PALETTE_LENGTH")
    expected_integer(palette_section, "entries", len(PALETTE_RGB),
                     "M98B_FIXTURE_PALETTE_ENTRIES")
    expected_integer(pixel_section, "bytes", WIDTH * HEIGHT,
                     "M98B_FIXTURE_PIXEL_LENGTH")
    if palette_section.get("path") != PALETTE_NAME or pixel_section.get("path") != PIXEL_NAME:
        fail("M98B_FIXTURE_PATH", "manifest output path differs")
    if palette_section.get("encoding") != "rgb888":
        fail("M98B_FIXTURE_PALETTE_ENCODING", "palette is not RGB888")
    if len(palette) != len(PALETTE_RGB) * 3:
        fail("M98B_FIXTURE_PALETTE_LENGTH", "file length differs")
    if len(pixels) != WIDTH * HEIGHT:
        fail("M98B_FIXTURE_PIXEL_LENGTH", "file length differs")
    expected_digest(manifest, "palette", palette)
    expected_digest(manifest, "pixels", pixels)
    if palette != build_palette():
        fail("M98B_FIXTURE_PALETTE_CONTENT", "palette bytes differ")
    if pixels != build_pixels():
        fail("M98B_FIXTURE_PIXEL_CONTENT", "indexed raster differs")
    if any(value >= RESERVED_INDEX for value in pixels):
        fail("M98B_FIXTURE_PIXEL_RANGE", "indexed pixel exceeds palette range")
    if TRANSPARENT_INDEX not in pixels:
        fail("M98B_FIXTURE_TRANSPARENCY", "fixture has no transparent pixels")
    if NEAR_BLACK_INDEX not in pixels:
        fail("M98B_FIXTURE_NEAR_BLACK", "fixture does not use index 1")
    properties = mapping(manifest.get("properties"), "M98B_FIXTURE_PROPERTIES")
    expected_integer(properties, "transparent_index", TRANSPARENT_INDEX,
                     "M98B_FIXTURE_TRANSPARENCY")
    expected_integer(properties, "near_black_index", NEAR_BLACK_INDEX,
                     "M98B_FIXTURE_NEAR_BLACK")
    expected_integer(properties, "reserved_index", RESERVED_INDEX,
                     "M98B_FIXTURE_RESERVED_INDEX")
    if properties.get("features") != feature_manifest():
        fail("M98B_FIXTURE_FEATURE_MANIFEST", "feature manifest differs")
    verify_near_black(palette)
    verify_features(pixels)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True,
                        help="public fixture directory created by the M98b builder")
    args = parser.parse_args(argv)
    try:
        inspect(args.input.resolve())
    except InspectionError as error:
        print(error, file=sys.stderr)
        return 1
    print(f"M98B_FIXTURE_INSPECT_PASS input={args.input.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

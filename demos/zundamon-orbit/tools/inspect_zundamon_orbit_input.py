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

"""Inspect one M98c local-input bundle and recover crop palette indices."""

from __future__ import annotations

import argparse
import stat
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn, cast

import validate_zundamon_orbit_manifest as manifest_validator


BMP_HEADER_SIZE = 54
PALETTE_BYTES = 16 * 3
VISIBLE_FIRST = 1
VISIBLE_LAST = 14
MAX_BMP_BYTES = BMP_HEADER_SIZE + (
    manifest_validator.MAX_CROP_DIMENSION * manifest_validator.MAX_CROP_DIMENSION * 4
)


class InputError(Exception):
    """A stable, fail-closed M98d input-content validation failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise InputError(code, detail)


@dataclass(frozen=True)
class Bmp32:
    width: int
    height: int
    top_down: bool
    contents: bytes

    def rgb_at(self, x: int, y: int) -> tuple[int, int, int]:
        stored_y = y if self.top_down else self.height - 1 - y
        offset = BMP_HEADER_SIZE + (stored_y * self.width + x) * 4
        blue, green, red = self.contents[offset:offset + 3]
        return red, green, blue


@dataclass(frozen=True)
class InputInspection:
    crop_width: int
    crop_height: int
    indexed_pixels: bytes
    transparent_pixels: int
    opaque_pixels: int


def read_regular_file(input_file: Path, maximum: int, file_kind: str) -> bytes:
    code_prefix = f"M98D_{file_kind}"
    try:
        file_status = input_file.lstat()
    except OSError:
        fail(f"{code_prefix}_READ", "input file could not be read")
    if not stat.S_ISREG(file_status.st_mode) or input_file.is_symlink():
        fail(f"{code_prefix}_FILE_TYPE", "input must be a regular non-symlink file")
    if file_status.st_size > maximum:
        fail(f"{code_prefix}_SIZE", "input file exceeds the size limit")
    try:
        with input_file.open("rb") as stream:
            contents = stream.read(maximum + 1)
    except OSError:
        fail(f"{code_prefix}_READ", "input file could not be read")
    if len(contents) > maximum:
        fail(f"{code_prefix}_SIZE", "input file exceeds the size limit")
    return contents


def parse_palette(contents: bytes,
                  background: tuple[int, int, int]) -> tuple[tuple[int, int, int], ...]:
    if len(contents) != PALETTE_BYTES:
        fail("M98D_PALETTE_LENGTH", "RGB888 palette must contain 48 bytes")
    entries = tuple(tuple(contents[offset:offset + 3])
                    for offset in range(0, PALETTE_BYTES, 3))
    typed_entries = cast(tuple[tuple[int, int, int], ...], entries)
    if typed_entries[0] != background:
        fail("M98D_PALETTE_TRANSPARENT_COLOR",
             "transparent entry must equal the background color")
    if typed_entries[15] != background:
        fail("M98D_PALETTE_RESERVED_COLOR",
             "reserved entry must equal the background color")
    visible = typed_entries[VISIBLE_FIRST:VISIBLE_LAST + 1]
    if len(set(visible)) != len(visible):
        fail("M98D_PALETTE_DUPLICATE_VISIBLE", "visible palette entries must be unique")
    if background in visible:
        fail("M98D_BACKGROUND_PALETTE_COLLISION",
             "background color must not be a visible palette entry")
    return typed_entries


def parse_bmp32(contents: bytes) -> Bmp32:
    if len(contents) < BMP_HEADER_SIZE:
        fail("M98D_BMP_HEADER", "BMP header is incomplete")
    magic, declared_size, reserved_a, reserved_b, pixel_offset = struct.unpack_from(
        "<2sIHHI", contents, 0)
    if magic != b"BM":
        fail("M98D_BMP_MAGIC", "BMP signature differs")
    if declared_size != len(contents):
        fail("M98D_BMP_FILE_SIZE", "declared BMP size differs")
    if reserved_a != 0 or reserved_b != 0:
        fail("M98D_BMP_RESERVED", "reserved BMP fields are nonzero")
    if pixel_offset != BMP_HEADER_SIZE:
        fail("M98D_BMP_PIXEL_OFFSET", "pixel array must start at byte 54")

    (dib_size, width, stored_height, planes, bits_per_pixel, compression,
     image_size, _x_pixels_per_meter, _y_pixels_per_meter, colors_used,
     important_colors) = struct.unpack_from("<IiiHHIIiiII", contents, 14)
    if dib_size != 40:
        fail("M98D_BMP_DIB_SIZE", "BITMAPINFOHEADER size must be 40")
    if not 1 <= width <= manifest_validator.MAX_CROP_DIMENSION:
        fail("M98D_BMP_WIDTH", "BMP width is outside the contract")
    if stored_height == 0 or abs(stored_height) > manifest_validator.MAX_CROP_DIMENSION:
        fail("M98D_BMP_HEIGHT", "BMP height is outside the contract")
    if planes != 1:
        fail("M98D_BMP_PLANES", "BMP plane count must be one")
    if bits_per_pixel != 32:
        fail("M98D_BMP_BPP", "BMP must contain 32 bits per pixel")
    if compression != 0:
        fail("M98D_BMP_COMPRESSION", "BMP compression must be BI_RGB")
    height = abs(stored_height)
    expected_image_size = width * height * 4
    if image_size not in (0, expected_image_size):
        fail("M98D_BMP_IMAGE_SIZE", "declared pixel-array size differs")
    if colors_used != 0:
        fail("M98D_BMP_COLORS_USED", "32-bpp BMP color-table count must be zero")
    if important_colors != 0:
        fail("M98D_BMP_IMPORTANT_COLORS", "important-color count must be zero")
    if len(contents) != BMP_HEADER_SIZE + expected_image_size:
        fail("M98D_BMP_LENGTH", "BMP pixel-array length differs")
    return Bmp32(width=width, height=height, top_down=stored_height < 0,
                 contents=contents)


def recover_indices(image: Bmp32, palette: tuple[tuple[int, int, int], ...],
                    crop: dict[str, object],
                    background: tuple[int, int, int]) -> InputInspection:
    crop_x = cast(int, crop["x"])
    crop_y = cast(int, crop["y"])
    crop_width = cast(int, crop["width"])
    crop_height = cast(int, crop["height"])
    if crop_x + crop_width > image.width or crop_y + crop_height > image.height:
        fail("M98D_CROP_BOUNDS", "crop lies outside the BMP")
    visible = {palette[index]: index for index in range(VISIBLE_FIRST, VISIBLE_LAST + 1)}
    indices = bytearray()
    transparent_pixels = 0
    opaque_pixels = 0
    for y in range(crop_y, crop_y + crop_height):
        for x in range(crop_x, crop_x + crop_width):
            color = image.rgb_at(x, y)
            if color == background:
                indices.append(0)
                transparent_pixels += 1
                continue
            index = visible.get(color)
            if index is None:
                fail("M98D_PIXEL_COLOR", "opaque pixel is not an exact visible color")
            indices.append(index)
            opaque_pixels += 1
    if opaque_pixels == 0:
        fail("M98D_CROP_EMPTY", "crop contains no opaque pixels")
    if transparent_pixels == 0:
        fail("M98D_CROP_NO_TRANSPARENCY", "crop contains no transparent pixels")
    return InputInspection(
        crop_width=crop_width,
        crop_height=crop_height,
        indexed_pixels=bytes(indices),
        transparent_pixels=transparent_pixels,
        opaque_pixels=opaque_pixels,
    )


def inspect_bundle(manifest_file: Path) -> InputInspection:
    manifest_value = manifest_validator.read_manifest(manifest_file)
    manifest_validator.validate_manifest(manifest_value)
    manifest = cast(dict[str, object], manifest_value)
    image_section = cast(dict[str, object], manifest["image"])
    palette_section = cast(dict[str, object], manifest["palette"])
    transparency = cast(dict[str, object], manifest["transparency"])
    crop = cast(dict[str, object], manifest["crop"])
    background = cast(tuple[int, int, int], tuple(transparency["background_rgb"]))
    bundle_directory = manifest_file.parent
    palette_contents = read_regular_file(
        bundle_directory / cast(str, palette_section["path"]), PALETTE_BYTES,
        "PALETTE")
    palette = parse_palette(palette_contents, background)
    image_contents = read_regular_file(
        bundle_directory / cast(str, image_section["path"]), MAX_BMP_BYTES, "BMP")
    image = parse_bmp32(image_contents)
    return recover_indices(image, palette, crop, background)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True,
                        help="M98c local-input manifest")
    arguments = parser.parse_args(argv)
    try:
        inspect_bundle(arguments.manifest)
    except (manifest_validator.ManifestError, InputError) as error:
        print(error, file=sys.stderr)
        return 1
    print("M98D_INPUT_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

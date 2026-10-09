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

"""Build private crop and anchor previews from one validated M98d bundle."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path
from typing import NoReturn, cast

import inspect_zundamon_orbit_input as input_inspector
import validate_zundamon_orbit_manifest as manifest_validator


SOURCE_OVERLAY_NAME = "source-overlay.bmp"
CROP_NAME = "crop.bmp"
ANCHOR_OVERLAY_NAME = "crop-anchor.bmp"
CROP_COLOR = (0, 255, 255)
ANCHOR_COLOR = (255, 0, 255)
MIN_SCALE = 1
MAX_SCALE = 8


class PreviewError(Exception):
    """A stable M98e preview-generation failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise PreviewError(code, detail)


def build_rgb_bmp32(width: int, height: int,
                    pixels: tuple[tuple[int, int, int], ...]) -> bytes:
    if width < 1 or height < 1 or len(pixels) != width * height:
        fail("M98E_PREVIEW_GEOMETRY", "preview geometry differs")
    encoded = bytearray()
    for y in range(height - 1, -1, -1):
        for red, green, blue in pixels[y * width:(y + 1) * width]:
            encoded.extend((blue, green, red, 255))
    file_size = input_inspector.BMP_HEADER_SIZE + len(encoded)
    file_header = struct.pack("<2sIHHI", b"BM", file_size, 0, 0,
                              input_inspector.BMP_HEADER_SIZE)
    dib_header = struct.pack(
        "<IiiHHIIiiII",
        40,
        width,
        height,
        1,
        32,
        0,
        len(encoded),
        2835,
        2835,
        0,
        0,
    )
    return file_header + dib_header + bytes(encoded)


def set_pixel(pixels: list[tuple[int, int, int]], width: int, height: int,
              x: int, y: int, color: tuple[int, int, int]) -> None:
    if 0 <= x < width and 0 <= y < height:
        pixels[y * width + x] = color


def draw_rectangle(pixels: list[tuple[int, int, int]], width: int, height: int,
                   x: int, y: int, rectangle_width: int, rectangle_height: int,
                   color: tuple[int, int, int]) -> None:
    right = x + rectangle_width - 1
    bottom = y + rectangle_height - 1
    for pixel_x in range(x, right + 1):
        set_pixel(pixels, width, height, pixel_x, y, color)
        set_pixel(pixels, width, height, pixel_x, bottom, color)
    for pixel_y in range(y, bottom + 1):
        set_pixel(pixels, width, height, x, pixel_y, color)
        set_pixel(pixels, width, height, right, pixel_y, color)


def draw_cross(pixels: list[tuple[int, int, int]], width: int, height: int,
               x: int, y: int, radius: int,
               color: tuple[int, int, int]) -> None:
    for delta in range(-radius, radius + 1):
        set_pixel(pixels, width, height, x + delta, y, color)
        set_pixel(pixels, width, height, x, y + delta, color)


def nearest_scale(pixels: tuple[tuple[int, int, int], ...], width: int,
                  height: int, scale: int) -> tuple[tuple[int, int, int], ...]:
    result: list[tuple[int, int, int]] = []
    for y in range(height):
        source_row = pixels[y * width:(y + 1) * width]
        expanded_row = tuple(color for color in source_row for _ in range(scale))
        for _ in range(scale):
            result.extend(expanded_row)
    return tuple(result)


def load_preview_inputs(manifest_file: Path) -> tuple[
        input_inspector.Bmp32, dict[str, object], dict[str, object]]:
    input_inspector.inspect_bundle(manifest_file)
    manifest_value = manifest_validator.read_manifest(manifest_file)
    manifest_validator.validate_manifest(manifest_value)
    manifest = cast(dict[str, object], manifest_value)
    image_section = cast(dict[str, object], manifest["image"])
    image_contents = input_inspector.read_regular_file(
        manifest_file.parent / cast(str, image_section["path"]),
        input_inspector.MAX_BMP_BYTES,
        "BMP",
    )
    image = input_inspector.parse_bmp32(image_contents)
    return (
        image,
        cast(dict[str, object], manifest["crop"]),
        cast(dict[str, object], manifest["anchor"]),
    )


def build_previews(manifest_file: Path, output: Path, scale: int = 4) -> None:
    if not MIN_SCALE <= scale <= MAX_SCALE:
        fail("M98E_PREVIEW_SCALE", "preview scale is outside 1-8")
    if output.exists():
        fail("M98E_PREVIEW_OUTPUT_EXISTS", "output directory exists")
    image, crop, anchor = load_preview_inputs(manifest_file)
    crop_x = cast(int, crop["x"])
    crop_y = cast(int, crop["y"])
    crop_width = cast(int, crop["width"])
    crop_height = cast(int, crop["height"])
    anchor_x = cast(int, anchor["x"])
    anchor_y = cast(int, anchor["y"])

    source_pixels = tuple(
        image.rgb_at(x, y)
        for y in range(image.height)
        for x in range(image.width)
    )
    source_overlay = list(source_pixels)
    draw_rectangle(source_overlay, image.width, image.height, crop_x, crop_y,
                   crop_width, crop_height, CROP_COLOR)
    draw_cross(source_overlay, image.width, image.height,
               crop_x + anchor_x, crop_y + anchor_y, 4, ANCHOR_COLOR)

    crop_pixels = tuple(
        image.rgb_at(x, y)
        for y in range(crop_y, crop_y + crop_height)
        for x in range(crop_x, crop_x + crop_width)
    )
    scaled_crop = nearest_scale(crop_pixels, crop_width, crop_height, scale)
    scaled_width = crop_width * scale
    scaled_height = crop_height * scale
    anchor_overlay = list(scaled_crop)
    draw_cross(anchor_overlay, scaled_width, scaled_height,
               anchor_x * scale + scale // 2,
               anchor_y * scale + scale // 2,
               max(4, scale * 2), ANCHOR_COLOR)

    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.mkdir()
        (output / SOURCE_OVERLAY_NAME).write_bytes(
            build_rgb_bmp32(image.width, image.height, tuple(source_overlay)))
        (output / CROP_NAME).write_bytes(
            build_rgb_bmp32(scaled_width, scaled_height, scaled_crop))
        (output / ANCHOR_OVERLAY_NAME).write_bytes(
            build_rgb_bmp32(scaled_width, scaled_height, tuple(anchor_overlay)))
    except OSError as error:
        raise PreviewError("M98E_PREVIEW_WRITE", "preview could not be written") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True,
                        help="validated local-input manifest")
    parser.add_argument("--output", type=Path, required=True,
                        help="new private preview directory")
    parser.add_argument("--scale", type=int, default=4,
                        help="integer crop-preview scale from 1 through 8")
    arguments = parser.parse_args(argv)
    try:
        build_previews(arguments.manifest, arguments.output, arguments.scale)
    except (manifest_validator.ManifestError, input_inspector.InputError,
            PreviewError) as error:
        print(error, file=sys.stderr)
        return 1
    print("M98E_PREVIEW_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

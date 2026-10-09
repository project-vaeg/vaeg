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

"""Convert one validated M98 input crop to VA 8-bpp GGGRRRBB pixels."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn, cast

import inspect_zundamon_orbit_input as input_inspector
import validate_zundamon_orbit_manifest as manifest_validator


PIXEL_NAME = "pixels.va8"
REPORT_NAME = "report.json"
REPORT_SCHEMA = "vaeg-zundamon-orbit-va8-report-v1"
REPORT_VERSION = 1
PIXEL_FORMAT = "GGGRRRBB"
TRANSPARENT_VALUE = 0
VISIBLE_FIRST = 1
VISIBLE_LAST = 14


class ConversionError(Exception):
    """A stable M98f conversion failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise ConversionError(code, detail)


def expand_channel(value: int, maximum: int) -> int:
    return (value * 255 + maximum // 2) // maximum


def decode_va8(value: int) -> tuple[int, int, int]:
    if not 0 <= value <= 255:
        fail("M98F_VA8_RANGE", "VA8 value is outside one byte")
    green3 = (value >> 5) & 7
    red3 = (value >> 2) & 7
    blue2 = value & 3
    return (
        expand_channel(red3, 7),
        expand_channel(green3, 7),
        expand_channel(blue2, 3),
    )


def quantize_rgb(rgb: tuple[int, int, int]) -> int:
    red8, green8, blue8 = rgb
    if any(channel < 0 or channel > 255 for channel in rgb):
        fail("M98F_RGB_RANGE", "RGB channel is outside one byte")
    red3 = (red8 * 7 + 127) // 255
    green3 = (green8 * 7 + 127) // 255
    blue2 = (blue8 * 3 + 127) // 255
    return (green3 << 5) | (red3 << 2) | blue2


def squared_error(left: tuple[int, int, int],
                  right: tuple[int, int, int]) -> int:
    return sum((left[index] - right[index]) ** 2 for index in range(3))


def nearest_nonzero_va8(rgb: tuple[int, int, int]) -> int:
    return min(
        range(1, 256),
        key=lambda value: (squared_error(rgb, decode_va8(value)), value),
    )


def convert_opaque_rgb(rgb: tuple[int, int, int]) -> tuple[int, bool]:
    value = quantize_rgb(rgb)
    if value != TRANSPARENT_VALUE:
        return value, False
    return nearest_nonzero_va8(rgb), True


@dataclass(frozen=True)
class ConversionResult:
    width: int
    height: int
    pixels: bytes
    report: dict[str, object]


def load_bundle(manifest_file: Path) -> tuple[
        input_inspector.InputInspection, tuple[tuple[int, int, int], ...]]:
    manifest_value = manifest_validator.read_manifest(manifest_file)
    manifest_validator.validate_manifest(manifest_value)
    manifest = cast(dict[str, object], manifest_value)
    palette_section = cast(dict[str, object], manifest["palette"])
    image_section = cast(dict[str, object], manifest["image"])
    transparency = cast(dict[str, object], manifest["transparency"])
    crop = cast(dict[str, object], manifest["crop"])
    background = cast(tuple[int, int, int],
                      tuple(transparency["background_rgb"]))
    palette_contents = input_inspector.read_regular_file(
        manifest_file.parent / cast(str, palette_section["path"]),
        input_inspector.PALETTE_BYTES,
        "PALETTE",
    )
    palette = input_inspector.parse_palette(palette_contents, background)
    image_contents = input_inspector.read_regular_file(
        manifest_file.parent / cast(str, image_section["path"]),
        input_inspector.MAX_BMP_BYTES,
        "BMP",
    )
    image = input_inspector.parse_bmp32(image_contents)
    inspection = input_inspector.recover_indices(image, palette, crop, background)
    return inspection, palette


def convert_indexed_pixels(
        indexed_pixels: bytes,
        palette: tuple[tuple[int, int, int], ...],
        width: int,
        height: int) -> ConversionResult:
    if width < 1 or height < 1:
        fail("M98F_GEOMETRY", "crop geometry is invalid")
    if len(indexed_pixels) != width * height:
        fail("M98F_PIXEL_LENGTH", "indexed pixel length differs")
    if len(palette) != 16:
        fail("M98F_PALETTE_LENGTH", "palette entry count differs")

    palette_values: list[int] = [0] * 16
    palette_repaired: list[bool] = [False] * 16
    palette_errors: list[int] = [0] * 16
    for index in range(VISIBLE_FIRST, VISIBLE_LAST + 1):
        value, repaired = convert_opaque_rgb(palette[index])
        palette_values[index] = value
        palette_repaired[index] = repaired
        palette_errors[index] = squared_error(palette[index], decode_va8(value))

    usage = [0] * 16
    converted = bytearray()
    for index in indexed_pixels:
        if index == TRANSPARENT_VALUE:
            usage[index] += 1
            converted.append(TRANSPARENT_VALUE)
        elif VISIBLE_FIRST <= index <= VISIBLE_LAST:
            usage[index] += 1
            converted.append(palette_values[index])
        else:
            fail("M98F_INDEX_RANGE", "indexed pixel uses a reserved value")

    collision_map: dict[int, list[int]] = {}
    for index in range(VISIBLE_FIRST, VISIBLE_LAST + 1):
        if usage[index] != 0:
            collision_map.setdefault(palette_values[index], []).append(index)
    collisions = [
        {"source_indices": indices, "va8": value}
        for value, indices in sorted(collision_map.items())
        if len(indices) > 1
    ]

    palette_entries = []
    for index in range(VISIBLE_FIRST, VISIBLE_LAST + 1):
        value = palette_values[index]
        palette_entries.append({
            "decoded_rgb": list(decode_va8(value)),
            "opaque_zero_repaired": palette_repaired[index],
            "pixel_count": usage[index],
            "source_index": index,
            "source_rgb": list(palette[index]),
            "squared_error": palette_errors[index],
            "va8": value,
        })

    opaque_pixels = sum(usage[VISIBLE_FIRST:VISIBLE_LAST + 1])
    opaque_zero_repairs = sum(
        usage[index]
        for index in range(VISIBLE_FIRST, VISIBLE_LAST + 1)
        if palette_repaired[index]
    )
    weighted_squared_error = sum(
        usage[index] * palette_errors[index]
        for index in range(VISIBLE_FIRST, VISIBLE_LAST + 1)
    )
    used_errors = [
        palette_errors[index]
        for index in range(VISIBLE_FIRST, VISIBLE_LAST + 1)
        if usage[index] != 0
    ]
    report: dict[str, object] = {
        "collisions": collisions,
        "copyright": manifest_validator.COPYRIGHT,
        "format": {
            "height": height,
            "pixel_bytes": len(converted),
            "pixel_format": PIXEL_FORMAT,
            "pixel_order": "top-to-bottom-left-to-right",
            "transparent_value": TRANSPARENT_VALUE,
            "width": width,
        },
        "license": manifest_validator.LICENSE,
        "metrics": {
            "maximum_squared_error": max(used_errors, default=0),
            "opaque_pixels": opaque_pixels,
            "opaque_zero_repairs": opaque_zero_repairs,
            "transparent_pixels": usage[TRANSPARENT_VALUE],
            "weighted_squared_error": weighted_squared_error,
        },
        "palette_entries": palette_entries,
        "quantization": {
            "opaque_zero_distance": "decoded-rgb-squared-error",
            "opaque_zero_tie_break": "lowest-va8-byte",
            "rounding": "nearest-integer-with-127-bias",
        },
        "schema": REPORT_SCHEMA,
        "schema_version": REPORT_VERSION,
    }
    return ConversionResult(width=width, height=height, pixels=bytes(converted),
                            report=report)


def convert_bundle(manifest_file: Path) -> ConversionResult:
    inspection, palette = load_bundle(manifest_file)
    return convert_indexed_pixels(
        inspection.indexed_pixels,
        palette,
        inspection.crop_width,
        inspection.crop_height,
    )


def report_bytes(report: dict[str, object]) -> bytes:
    return (json.dumps(report, indent=2, sort_keys=True) + "\n").encode("utf-8")


def write_conversion(manifest_file: Path, output: Path) -> None:
    if output.exists():
        fail("M98F_OUTPUT_EXISTS", "output directory exists")
    result = convert_bundle(manifest_file)
    encoded_report = report_bytes(result.report)
    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.mkdir()
        (output / PIXEL_NAME).write_bytes(result.pixels)
        (output / REPORT_NAME).write_bytes(encoded_report)
    except OSError as error:
        raise ConversionError("M98F_WRITE", "output could not be written") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True,
                        help="validated local-input manifest")
    parser.add_argument("--output", type=Path, required=True,
                        help="new private VA8 output directory")
    arguments = parser.parse_args(argv)
    try:
        write_conversion(arguments.manifest, arguments.output)
    except (manifest_validator.ManifestError, input_inspector.InputError,
            ConversionError) as error:
        print(error, file=sys.stderr)
        return 1
    print("M98F_CONVERSION_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

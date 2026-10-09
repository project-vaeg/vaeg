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

"""Build the complete source-neutral M98 local host-asset pipeline."""

from __future__ import annotations

import argparse
import json
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn, cast

import build_zundamon_orbit_crop_preview as preview_builder
import build_zundamon_orbit_input_fixture as input_fixture
import convert_zundamon_orbit_va8 as va8_converter
import generate_zundamon_orbit_scales as scaler
import inspect_zundamon_orbit_atlas as format_inspector
import inspect_zundamon_orbit_input as input_inspector
import pack_zundamon_orbit_atlas as packer
import validate_zundamon_orbit_manifest as manifest_validator


ATLAS_NAME = "zundorb.bin"
CONTACT_SHEET_NAME = "contact-sheet.bmp"
REPORT_NAME = "pipeline-report.json"
REPORT_SCHEMA = "vaeg-zundamon-orbit-host-pipeline-report-v1"
REPORT_VERSION = 1
MAX_ATLAS_SOURCE_WIDTH = 98
MAX_ATLAS_SOURCE_HEIGHT = 128
SHEET_COLUMNS = 5
SHEET_ROWS = 6
CELL_WIDTH = 240
CELL_HEIGHT = 196
SHEET_WIDTH = SHEET_COLUMNS * CELL_WIDTH
SHEET_HEIGHT = SHEET_ROWS * CELL_HEIGHT
PREVIEW_MARGIN_X = 8
PREVIEW_TOP = 20
PREVIEW_WIDTH = CELL_WIDTH - 2 * PREVIEW_MARGIN_X
PREVIEW_HEIGHT = CELL_HEIGHT - PREVIEW_TOP - 8
SHEET_BACKGROUND = (16, 18, 24)
CELL_BORDER = (88, 92, 104)
LABEL_COLOR = (232, 236, 240)
TRANSPARENT_DARK = (36, 38, 48)
TRANSPARENT_LIGHT = (58, 62, 72)
ANCHOR_OUTLINE = (16, 16, 16)
ANCHOR_COLOR = (255, 64, 80)
GLYPHS: dict[str, tuple[int, ...]] = {
    " ": (0, 0, 0, 0, 0, 0, 0),
    ",": (0, 0, 0, 0, 0, 0b00100, 0b01000),
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
    "L": (0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111),
    "x": (0, 0, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001),
}


class PipelineError(Exception):
    """A stable M98j pipeline or contact-sheet failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise PipelineError(code, detail)


@dataclass(frozen=True)
class PipelineResult:
    atlas: bytes
    contact_sheet: bytes
    report: dict[str, object]


@dataclass(frozen=True)
class NormalizedSource:
    width: int
    height: int
    anchor_x: int
    anchor_y: int
    pixels: bytes
    report: dict[str, object]


def bounded_dimensions(width: int, height: int) -> tuple[int, int]:
    if width < 1 or height < 1:
        fail("M98J_NORMALIZE_GEOMETRY", "normalization geometry is invalid")
    if width <= MAX_ATLAS_SOURCE_WIDTH and height <= MAX_ATLAS_SOURCE_HEIGHT:
        return width, height
    if width * MAX_ATLAS_SOURCE_HEIGHT >= height * MAX_ATLAS_SOURCE_WIDTH:
        target_width = min(width, MAX_ATLAS_SOURCE_WIDTH)
        target_height = max(1, height * target_width // width)
    else:
        target_height = min(height, MAX_ATLAS_SOURCE_HEIGHT)
        target_width = max(1, width * target_height // height)
    if (target_width > width or target_height > height
            or target_width > MAX_ATLAS_SOURCE_WIDTH
            or target_height > MAX_ATLAS_SOURCE_HEIGHT):
        fail("M98J_NORMALIZE_BOUNDS", "normalization target exceeds its bounds")
    return target_width, target_height


def scale_atlas_occupied_bytes(width: int, height: int) -> int:
    if width < 1 or height < 1:
        fail("M98J_NORMALIZE_GEOMETRY", "normalization geometry is invalid")
    cursor = 0
    for level in range(1, scaler.SCALE_COUNT + 1):
        frame_width = scaler.scale_dimension(width, level)
        frame_height = scaler.scale_dimension(height, level)
        pitch = scaler.align_up(frame_width, scaler.ROW_ALIGNMENT)
        cursor = scaler.align_up(cursor, scaler.FRAME_ALIGNMENT)
        cursor += pitch * frame_height
    return cursor


def normalized_dimensions(width: int, height: int) -> tuple[int, int]:
    target_width, target_height = bounded_dimensions(width, height)
    if scale_atlas_occupied_bytes(target_width, target_height) > packer.BANK_SIZE:
        fail("M98J_NORMALIZE_BANK", "bounded source exceeds one BMS bank")
    return target_width, target_height


def normalize_source(pixels: bytes, width: int, height: int,
                     anchor_x: int, anchor_y: int) -> NormalizedSource:
    if width < 1 or height < 1 or len(pixels) != width * height:
        fail("M98J_NORMALIZE_SOURCE", "normalization source differs")
    if not 0 <= anchor_x < width or not 0 <= anchor_y < height:
        fail("M98J_NORMALIZE_ANCHOR", "normalization anchor is out of bounds")
    target_width, target_height = normalized_dimensions(width, height)
    if (target_width, target_height) == (width, height):
        normalized_pixels = pixels
        target_anchor_x = anchor_x
        target_anchor_y = anchor_y
    else:
        output = bytearray()
        for target_y in range(target_height):
            source_y = min(
                height - 1,
                ((2 * target_y + 1) * height) // (2 * target_height),
            )
            for target_x in range(target_width):
                source_x = min(
                    width - 1,
                    ((2 * target_x + 1) * width) // (2 * target_width),
                )
                output.append(pixels[source_y * width + source_x])
        normalized_pixels = bytes(output)
        target_anchor_x = min(
            target_width - 1,
            ((2 * anchor_x + 1) * target_width) // (2 * width),
        )
        target_anchor_y = min(
            target_height - 1,
            ((2 * anchor_y + 1) * target_height) // (2 * height),
        )
    report: dict[str, object] = {
        "downscaled": (target_width, target_height) != (width, height),
        "input_anchor_x": anchor_x,
        "input_anchor_y": anchor_y,
        "input_height": height,
        "input_width": width,
        "maximum_height": MAX_ATLAS_SOURCE_HEIGHT,
        "maximum_width": MAX_ATLAS_SOURCE_WIDTH,
        "method": "center-sampled-nearest-neighbor",
        "output_anchor_x": target_anchor_x,
        "output_anchor_y": target_anchor_y,
        "output_height": target_height,
        "output_width": target_width,
        "one_bank_occupied_bytes": scale_atlas_occupied_bytes(
            target_width, target_height),
        "upscaling": False,
    }
    return NormalizedSource(
        width=target_width,
        height=target_height,
        anchor_x=target_anchor_x,
        anchor_y=target_anchor_y,
        pixels=normalized_pixels,
        report=report,
    )


def set_pixel(pixels: list[tuple[int, int, int]], width: int, height: int,
              x: int, y: int, color: tuple[int, int, int]) -> None:
    if 0 <= x < width and 0 <= y < height:
        pixels[y * width + x] = color


def draw_line(pixels: list[tuple[int, int, int]], width: int, height: int,
              x0: int, y0: int, x1: int, y1: int,
              color: tuple[int, int, int]) -> None:
    if x0 == x1:
        for y in range(min(y0, y1), max(y0, y1) + 1):
            set_pixel(pixels, width, height, x0, y, color)
    elif y0 == y1:
        for x in range(min(x0, x1), max(x0, x1) + 1):
            set_pixel(pixels, width, height, x, y0, color)
    else:
        fail("M98J_CONTACT_LINE", "contact-sheet line must be orthogonal")


def draw_text(pixels: list[tuple[int, int, int]], width: int, height: int,
              x: int, y: int, text: str,
              color: tuple[int, int, int]) -> None:
    cursor = x
    for character in text:
        glyph = GLYPHS.get(character)
        if glyph is None:
            fail("M98J_CONTACT_GLYPH", "contact-sheet label glyph is unavailable")
        for row, bits in enumerate(glyph):
            for column in range(5):
                if bits & (1 << (4 - column)):
                    set_pixel(pixels, width, height,
                              cursor + column, y + row, color)
        cursor += 6


def preview_dimensions(width: int, height: int) -> tuple[int, int]:
    if width < 1 or height < 1:
        fail("M98J_CONTACT_GEOMETRY", "contact-sheet source geometry is invalid")
    if width * PREVIEW_HEIGHT >= height * PREVIEW_WIDTH:
        preview_width = PREVIEW_WIDTH
        preview_height = max(1, height * PREVIEW_WIDTH // width)
    else:
        preview_height = PREVIEW_HEIGHT
        preview_width = max(1, width * PREVIEW_HEIGHT // height)
    return preview_width, preview_height


def contact_sheet_pixels(scale_set: scaler.ScaleSet) -> tuple[
        tuple[tuple[int, int, int], ...], dict[str, object]]:
    if len(scale_set.frames) != scaler.SCALE_COUNT:
        fail("M98J_CONTACT_FRAME_COUNT", "contact sheet requires 30 frames")
    pixels = [SHEET_BACKGROUND] * (SHEET_WIDTH * SHEET_HEIGHT)
    cells = []
    for index, frame in enumerate(scale_set.frames):
        column = index % SHEET_COLUMNS
        row = index // SHEET_COLUMNS
        cell_x = column * CELL_WIDTH
        cell_y = row * CELL_HEIGHT
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  cell_x, cell_y, cell_x + CELL_WIDTH - 1, cell_y, CELL_BORDER)
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  cell_x, cell_y, cell_x, cell_y + CELL_HEIGHT - 1, CELL_BORDER)
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  cell_x + CELL_WIDTH - 1, cell_y,
                  cell_x + CELL_WIDTH - 1, cell_y + CELL_HEIGHT - 1,
                  CELL_BORDER)
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  cell_x, cell_y + CELL_HEIGHT - 1,
                  cell_x + CELL_WIDTH - 1, cell_y + CELL_HEIGHT - 1,
                  CELL_BORDER)
        label = (f"L{frame.level:02d} {frame.width}x{frame.height} "
                 f"A{frame.anchor_x},{frame.anchor_y}")
        draw_text(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  cell_x + PREVIEW_MARGIN_X, cell_y + 6, label, LABEL_COLOR)

        target_width, target_height = preview_dimensions(
            frame.width, frame.height)
        preview_x = cell_x + PREVIEW_MARGIN_X + (PREVIEW_WIDTH - target_width) // 2
        preview_y = cell_y + PREVIEW_TOP + (PREVIEW_HEIGHT - target_height) // 2
        for target_y in range(target_height):
            source_y = min(
                frame.height - 1,
                ((2 * target_y + 1) * frame.height) // (2 * target_height),
            )
            for target_x in range(target_width):
                source_x = min(
                    frame.width - 1,
                    ((2 * target_x + 1) * frame.width) // (2 * target_width),
                )
                value = frame.payload[source_y * frame.pitch + source_x]
                if value == va8_converter.TRANSPARENT_VALUE:
                    color = (TRANSPARENT_DARK
                             if ((target_x // 8) + (target_y // 8)) % 2 == 0
                             else TRANSPARENT_LIGHT)
                else:
                    color = va8_converter.decode_va8(value)
                set_pixel(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                          preview_x + target_x, preview_y + target_y, color)

        anchor_x = min(
            target_width - 1,
            ((2 * frame.anchor_x + 1) * target_width) // (2 * frame.width),
        )
        anchor_y = min(
            target_height - 1,
            ((2 * frame.anchor_y + 1) * target_height) // (2 * frame.height),
        )
        marker_x = preview_x + anchor_x
        marker_y = preview_y + anchor_y
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  marker_x - 5, marker_y, marker_x + 5, marker_y,
                  ANCHOR_OUTLINE)
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  marker_x, marker_y - 5, marker_x, marker_y + 5,
                  ANCHOR_OUTLINE)
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  marker_x - 4, marker_y, marker_x + 4, marker_y,
                  ANCHOR_COLOR)
        draw_line(pixels, SHEET_WIDTH, SHEET_HEIGHT,
                  marker_x, marker_y - 4, marker_x, marker_y + 4,
                  ANCHOR_COLOR)
        cells.append({
            "anchor_x": frame.anchor_x,
            "anchor_y": frame.anchor_y,
            "cell_x": cell_x,
            "cell_y": cell_y,
            "height": frame.height,
            "level": frame.level,
            "marker_x": marker_x,
            "marker_y": marker_y,
            "preview_height": target_height,
            "preview_width": target_width,
            "preview_x": preview_x,
            "preview_y": preview_y,
            "width": frame.width,
        })

    report: dict[str, object] = {
        "cell_height": CELL_HEIGHT,
        "cell_width": CELL_WIDTH,
        "cells": cells,
        "columns": SHEET_COLUMNS,
        "height": SHEET_HEIGHT,
        "rows": SHEET_ROWS,
        "width": SHEET_WIDTH,
    }
    return tuple(pixels), report


def build_contact_sheet(
        scale_set: scaler.ScaleSet,
) -> tuple[bytes, dict[str, object]]:
    pixels, report = contact_sheet_pixels(scale_set)
    try:
        contents = preview_builder.build_rgb_bmp32(
            SHEET_WIDTH, SHEET_HEIGHT, pixels)
    except preview_builder.PreviewError as error:
        raise PipelineError("M98J_CONTACT_BMP", "contact sheet could not be encoded") from error
    return contents, report


def build_pipeline(manifest_file: Path) -> PipelineResult:
    manifest_value = manifest_validator.read_manifest(manifest_file)
    manifest_validator.validate_manifest(manifest_value)
    manifest = cast(dict[str, object], manifest_value)
    anchor = cast(dict[str, object], manifest["anchor"])

    inspection, palette = va8_converter.load_bundle(manifest_file)
    conversion = va8_converter.convert_indexed_pixels(
        inspection.indexed_pixels,
        palette,
        inspection.crop_width,
        inspection.crop_height,
    )
    normalized = normalize_source(
        conversion.pixels,
        conversion.width,
        conversion.height,
        cast(int, anchor["x"]),
        cast(int, anchor["y"]),
    )
    scale_set = scaler.build_scale_set(
        normalized.pixels,
        normalized.width,
        normalized.height,
        normalized.anchor_x,
        normalized.anchor_y,
    )
    packed = packer.build_atlas(scale_set)
    format_header, format_descriptors = format_inspector.inspect_bytes(
        packed.contents)
    packed_header, packed_descriptors, plan = packer.inspect_packed_bytes(
        packed.contents)
    if (format_header != packed_header
            or format_descriptors != packed_descriptors):
        fail("M98J_INSPECTOR_AGREEMENT", "final atlas inspectors disagree")
    contact_sheet, contact_report = build_contact_sheet(scale_set)

    report: dict[str, object] = {
        "contact_sheet": contact_report,
        "conversion": conversion.report,
        "copyright": manifest_validator.COPYRIGHT,
        "input": {
            "opaque_pixels": inspection.opaque_pixels,
            "transparent_pixels": inspection.transparent_pixels,
        },
        "license": manifest_validator.LICENSE,
        "normalization": normalized.report,
        "packing": packed.report,
        "scale_set": scale_set.report,
        "schema": REPORT_SCHEMA,
        "schema_version": REPORT_VERSION,
        "validation": {
            "descriptor_count": len(packed_descriptors),
            "format": "M98H_ATLAS_PASS",
            "packing": "M98I_PACKING_PASS",
            "required_bank_count": plan.required_bank_count,
        },
    }
    return PipelineResult(
        atlas=packed.contents,
        contact_sheet=contact_sheet,
        report=report,
    )


def report_bytes(report: dict[str, object]) -> bytes:
    return (json.dumps(report, indent=2, sort_keys=True) + "\n").encode("utf-8")


def write_pipeline(manifest_file: Path, output: Path) -> None:
    if output.exists():
        fail("M98J_OUTPUT_EXISTS", "output directory exists")
    result = build_pipeline(manifest_file)
    encoded_report = report_bytes(result.report)
    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.mkdir()
        (output / ATLAS_NAME).write_bytes(result.atlas)
        (output / CONTACT_SHEET_NAME).write_bytes(result.contact_sheet)
        (output / REPORT_NAME).write_bytes(encoded_report)
    except OSError as error:
        raise PipelineError("M98J_WRITE", "pipeline output could not be written") from error


def write_public_fixture(output: Path) -> None:
    if output.exists():
        fail("M98J_OUTPUT_EXISTS", "output directory exists")
    with tempfile.TemporaryDirectory(prefix="vaeg-m98j-fixture-") as temporary:
        bundle = Path(temporary) / "bundle"
        input_fixture.write_fixture(bundle)
        write_pipeline(bundle / input_fixture.MANIFEST_NAME, output)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path,
                        help="validated local-input manifest")
    parser.add_argument("--output", type=Path,
                        help="new local pipeline output directory")
    parser.add_argument("--fixture-output", type=Path,
                        help="new public-fixture pipeline output directory")
    arguments = parser.parse_args(argv)
    try:
        local_mode = arguments.manifest is not None or arguments.output is not None
        fixture_mode = arguments.fixture_output is not None
        if fixture_mode == local_mode:
            fail("M98J_ARGUMENTS", "select exactly one complete pipeline mode")
        if fixture_mode:
            write_public_fixture(arguments.fixture_output)
            print("M98J_FIXTURE_PASS")
        else:
            if arguments.manifest is None or arguments.output is None:
                fail("M98J_ARGUMENTS", "local mode requires manifest and output")
            write_pipeline(arguments.manifest, arguments.output)
            print("M98J_LOCAL_BUILD_READY")
    except (manifest_validator.ManifestError, input_inspector.InputError,
            va8_converter.ConversionError, scaler.ScaleError,
            packer.PackingError, format_inspector.AtlasError,
            input_fixture.FixtureError, PipelineError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

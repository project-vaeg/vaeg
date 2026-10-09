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

"""Generate exactly 30 deterministic VA8 nearest-neighbor scale frames."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn, cast

import convert_zundamon_orbit_va8 as va8_converter
import inspect_zundamon_orbit_input as input_inspector
import validate_zundamon_orbit_manifest as manifest_validator


SCALE_COUNT = 30
SCALE_DENOMINATOR = 31
SCALE_ROUNDING_BIAS = SCALE_DENOMINATOR // 2
ROW_ALIGNMENT = 4
FRAME_ALIGNMENT = 16
STREAM_NAME = "scales.va8"
REPORT_NAME = "report.json"
REPORT_SCHEMA = "vaeg-zundamon-orbit-scale-set-report-v1"
REPORT_VERSION = 1


class ScaleError(Exception):
    """A stable M98g scale-generation failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise ScaleError(code, detail)


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


def scale_dimension(source_size: int, level: int) -> int:
    if source_size < 1:
        fail("M98G_SOURCE_GEOMETRY", "source dimension is invalid")
    if not 1 <= level <= SCALE_COUNT:
        fail("M98G_LEVEL_RANGE", "scale level is outside 1-30")
    numerator = level if level < SCALE_COUNT else SCALE_DENOMINATOR
    return max(
        1,
        (source_size * numerator + SCALE_ROUNDING_BIAS) // SCALE_DENOMINATOR,
    )


def source_sample_coordinate(target_coordinate: int, source_size: int,
                             target_size: int) -> int:
    if source_size < 1 or target_size < 1:
        fail("M98G_TARGET_GEOMETRY", "sampling geometry is invalid")
    if not 0 <= target_coordinate < target_size:
        fail("M98G_TARGET_COORDINATE", "target coordinate is out of bounds")
    return min(
        source_size - 1,
        ((2 * target_coordinate + 1) * source_size) // (2 * target_size),
    )


def scale_anchor_coordinate(anchor: int, source_size: int,
                            target_size: int) -> int:
    if source_size < 1 or target_size < 1:
        fail("M98G_TARGET_GEOMETRY", "anchor geometry is invalid")
    if not 0 <= anchor < source_size:
        fail("M98G_ANCHOR_BOUNDS", "source anchor is out of bounds")
    return min(
        target_size - 1,
        ((2 * anchor + 1) * target_size) // (2 * source_size),
    )


def scale_payload(source_pixels: bytes, source_width: int, source_height: int,
                  target_width: int, target_height: int) -> tuple[int, bytes]:
    if source_width < 1 or source_height < 1:
        fail("M98G_SOURCE_GEOMETRY", "source geometry is invalid")
    if len(source_pixels) != source_width * source_height:
        fail("M98G_SOURCE_LENGTH", "source pixel length differs")
    if target_width < 1 or target_height < 1:
        fail("M98G_TARGET_GEOMETRY", "target geometry is invalid")
    pitch = align_up(target_width, ROW_ALIGNMENT)
    payload = bytearray()
    for target_y in range(target_height):
        source_y = source_sample_coordinate(
            target_y, source_height, target_height)
        for target_x in range(target_width):
            source_x = source_sample_coordinate(
                target_x, source_width, target_width)
            payload.append(source_pixels[source_y * source_width + source_x])
        payload.extend(b"\x00" * (pitch - target_width))
    return pitch, bytes(payload)


@dataclass(frozen=True)
class ScaleFrame:
    level: int
    width: int
    height: int
    pitch: int
    anchor_x: int
    anchor_y: int
    offset: int
    payload: bytes


@dataclass(frozen=True)
class ScaleSet:
    stream: bytes
    frames: tuple[ScaleFrame, ...]
    report: dict[str, object]


def build_scale_set(source_pixels: bytes, source_width: int, source_height: int,
                    anchor_x: int, anchor_y: int) -> ScaleSet:
    if source_width < 1 or source_height < 1:
        fail("M98G_SOURCE_GEOMETRY", "source geometry is invalid")
    if len(source_pixels) != source_width * source_height:
        fail("M98G_SOURCE_LENGTH", "source pixel length differs")
    if not 0 <= anchor_x < source_width or not 0 <= anchor_y < source_height:
        fail("M98G_ANCHOR_BOUNDS", "source anchor is out of bounds")

    stream = bytearray()
    frames: list[ScaleFrame] = []
    frame_alignment_bytes = 0
    row_padding_bytes = 0
    useful_pixel_bytes = 0
    for level in range(1, SCALE_COUNT + 1):
        width = scale_dimension(source_width, level)
        height = scale_dimension(source_height, level)
        pitch, payload = scale_payload(
            source_pixels, source_width, source_height, width, height)
        aligned_offset = align_up(len(stream), FRAME_ALIGNMENT)
        alignment_padding = aligned_offset - len(stream)
        stream.extend(b"\x00" * alignment_padding)
        frame_alignment_bytes += alignment_padding
        offset = len(stream)
        stream.extend(payload)
        row_padding_bytes += (pitch - width) * height
        useful_pixel_bytes += width * height
        frames.append(ScaleFrame(
            level=level,
            width=width,
            height=height,
            pitch=pitch,
            anchor_x=scale_anchor_coordinate(anchor_x, source_width, width),
            anchor_y=scale_anchor_coordinate(anchor_y, source_height, height),
            offset=offset,
            payload=payload,
        ))

    descriptors = [
        {
            "anchor_x": frame.anchor_x,
            "anchor_y": frame.anchor_y,
            "height": frame.height,
            "level": frame.level,
            "offset": frame.offset,
            "payload_bytes": len(frame.payload),
            "pitch": frame.pitch,
            "width": frame.width,
        }
        for frame in frames
    ]
    report: dict[str, object] = {
        "copyright": manifest_validator.COPYRIGHT,
        "descriptors": descriptors,
        "format": {
            "frame_alignment": FRAME_ALIGNMENT,
            "pixel_format": va8_converter.PIXEL_FORMAT,
            "row_alignment": ROW_ALIGNMENT,
            "scale_count": SCALE_COUNT,
            "stream_order": "increasing-level",
            "transparent_value": va8_converter.TRANSPARENT_VALUE,
        },
        "license": manifest_validator.LICENSE,
        "metrics": {
            "frame_alignment_bytes": frame_alignment_bytes,
            "row_padding_bytes": row_padding_bytes,
            "stream_bytes": len(stream),
            "useful_pixel_bytes": useful_pixel_bytes,
        },
        "schema": REPORT_SCHEMA,
        "schema_version": REPORT_VERSION,
        "source": {
            "anchor_x": anchor_x,
            "anchor_y": anchor_y,
            "height": source_height,
            "width": source_width,
        },
    }
    return ScaleSet(stream=bytes(stream), frames=tuple(frames), report=report)


def build_from_bundle(manifest_file: Path) -> ScaleSet:
    conversion = va8_converter.convert_bundle(manifest_file)
    manifest_value = manifest_validator.read_manifest(manifest_file)
    manifest_validator.validate_manifest(manifest_value)
    manifest = cast(dict[str, object], manifest_value)
    anchor = cast(dict[str, object], manifest["anchor"])
    return build_scale_set(
        conversion.pixels,
        conversion.width,
        conversion.height,
        cast(int, anchor["x"]),
        cast(int, anchor["y"]),
    )


def report_bytes(report: dict[str, object]) -> bytes:
    return (json.dumps(report, indent=2, sort_keys=True) + "\n").encode("utf-8")


def write_scale_set(manifest_file: Path, output: Path) -> None:
    if output.exists():
        fail("M98G_OUTPUT_EXISTS", "output directory exists")
    scale_set = build_from_bundle(manifest_file)
    encoded_report = report_bytes(scale_set.report)
    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.mkdir()
        (output / STREAM_NAME).write_bytes(scale_set.stream)
        (output / REPORT_NAME).write_bytes(encoded_report)
    except OSError as error:
        raise ScaleError("M98G_WRITE", "output could not be written") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True,
                        help="validated local-input manifest")
    parser.add_argument("--output", type=Path, required=True,
                        help="new private scale-set output directory")
    arguments = parser.parse_args(argv)
    try:
        write_scale_set(arguments.manifest, arguments.output)
    except (manifest_validator.ManifestError, input_inspector.InputError,
            va8_converter.ConversionError, ScaleError) as error:
        print(error, file=sys.stderr)
        return 1
    print("M98G_SCALE_SET_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

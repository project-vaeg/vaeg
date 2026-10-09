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

"""Pack 30 ordered scale frames into one version-1 BMS atlas bank."""

from __future__ import annotations

import argparse
import json
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn, Sequence

import build_zundamon_orbit_atlas_fixture as format_fixture
import generate_zundamon_orbit_scales as scaler
import inspect_zundamon_orbit_atlas as format_inspector
import validate_zundamon_orbit_manifest as manifest_validator


MAGIC = b"ZUNDORB\x00"
VERSION = 1
HEADER_FORMAT = "<8sHHIHHHHIHHIIIIIIII"
DESCRIPTOR_FORMAT = "<HHHHHHHHIIII"
HEADER_SIZE = 64
DESCRIPTOR_SIZE = 32
POSE_COUNT = 1
SCALE_COUNT = 30
SCALE_DENOMINATOR = 31
BANK_SIZE = 0x00020000
FIRST_BANK_VALUE = 1
DESCRIPTOR_OFFSET = HEADER_SIZE
DESCRIPTOR_BYTES = SCALE_COUNT * DESCRIPTOR_SIZE
PAYLOAD_OFFSET = 1024
FILE_CRC_OFFSET = 56
FRAME_ALIGNMENT = 16
ATLAS_NAME = "zundorb.bin"
REPORT_NAME = "packing-report.json"
REPORT_SCHEMA = "vaeg-zundamon-orbit-bms-packing-report-v1"
REPORT_VERSION = 1


class PackingError(Exception):
    """A stable M98i packing or packed-atlas failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise PackingError(code, detail)


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


@dataclass(frozen=True)
class Placement:
    bank_slot: int
    bank_offset: int
    payload_bytes: int


@dataclass(frozen=True)
class PackingPlan:
    placements: tuple[Placement, ...]
    required_bank_count: int
    frame_alignment_bytes: int
    bank_boundary_padding_bytes: int
    bank_payload_bytes: tuple[int, ...]
    bank_occupied_bytes: tuple[int, ...]


@dataclass(frozen=True)
class PackedAtlas:
    contents: bytes
    report: dict[str, object]


def plan_bank_layout(payload_sizes: Sequence[int]) -> PackingPlan:
    if not 1 <= len(payload_sizes) <= SCALE_COUNT:
        fail("M98I_PLAN_COUNT", "packing plan frame count is outside 1-30")

    placements: list[Placement] = []
    bank_slot = 0
    cursor = 0
    frame_alignment_bytes = 0
    bank_boundary_padding_bytes = 0
    bank_payload_bytes = [0]
    bank_occupied_bytes = [0]

    for payload_bytes in payload_sizes:
        if not isinstance(payload_bytes, int) or payload_bytes < 1:
            fail("M98I_PLAN_SIZE", "frame payload size is invalid")
        if payload_bytes > BANK_SIZE:
            fail("M98I_FRAME_TOO_LARGE", "frame payload exceeds one BMS bank")

        aligned_offset = align_up(cursor, FRAME_ALIGNMENT)
        if aligned_offset + payload_bytes > BANK_SIZE:
            bank_boundary_padding_bytes += BANK_SIZE - cursor
            bank_slot += 1
            if bank_slot >= SCALE_COUNT:
                fail("M98I_BANK_COUNT", "packing requires more than 30 banks")
            cursor = 0
            aligned_offset = 0
            bank_payload_bytes.append(0)
            bank_occupied_bytes.append(0)
        else:
            frame_alignment_bytes += aligned_offset - cursor

        placements.append(Placement(
            bank_slot=bank_slot,
            bank_offset=aligned_offset,
            payload_bytes=payload_bytes,
        ))
        bank_payload_bytes[bank_slot] += payload_bytes
        cursor = aligned_offset + payload_bytes
        bank_occupied_bytes[bank_slot] = cursor

    return PackingPlan(
        placements=tuple(placements),
        required_bank_count=bank_slot + 1,
        frame_alignment_bytes=frame_alignment_bytes,
        bank_boundary_padding_bytes=bank_boundary_padding_bytes,
        bank_payload_bytes=tuple(bank_payload_bytes),
        bank_occupied_bytes=tuple(bank_occupied_bytes),
    )


def validate_scale_set(scale_set: scaler.ScaleSet) -> None:
    if len(scale_set.frames) != SCALE_COUNT:
        fail("M98I_SCALE_COUNT", "scale set must contain exactly 30 frames")

    stream_cursor = 0
    source = scale_set.frames[-1]
    for index, frame in enumerate(scale_set.frames):
        if frame.level != index + 1:
            fail("M98I_LEVEL_ORDER", "scale levels are not ordered 1-30")
        if not (1 <= frame.width <= format_inspector.MAX_DIMENSION
                and 1 <= frame.height <= format_inspector.MAX_DIMENSION):
            fail("M98I_DIMENSIONS", "frame dimensions are invalid")
        if frame.pitch != align_up(frame.width, 4):
            fail("M98I_PITCH", "frame pitch is noncanonical")
        if not (0 <= frame.anchor_x < frame.width
                and 0 <= frame.anchor_y < frame.height):
            fail("M98I_ANCHOR", "frame anchor is out of bounds")
        if len(frame.payload) != frame.pitch * frame.height:
            fail("M98I_FRAME_LENGTH", "frame payload length differs")
        if len(frame.payload) > BANK_SIZE:
            fail("M98I_FRAME_TOO_LARGE", "frame payload exceeds one BMS bank")

        numerator = (
            frame.level if frame.level < SCALE_COUNT else SCALE_DENOMINATOR)
        expected_width = max(
            1,
            (source.width * numerator + SCALE_DENOMINATOR // 2)
            // SCALE_DENOMINATOR,
        )
        expected_height = max(
            1,
            (source.height * numerator + SCALE_DENOMINATOR // 2)
            // SCALE_DENOMINATOR,
        )
        if (frame.width, frame.height) != (expected_width, expected_height):
            fail("M98I_SCALE_GEOMETRY", "scale geometry is noncanonical")
        expected_anchor_x = format_inspector.projected_coordinate(
            source.anchor_x, source.width, frame.width)
        expected_anchor_y = format_inspector.projected_coordinate(
            source.anchor_y, source.height, frame.height)
        if (frame.anchor_x, frame.anchor_y) != (
                expected_anchor_x, expected_anchor_y):
            fail("M98I_ANCHOR_GEOMETRY", "scaled anchor is noncanonical")

        expected_offset = align_up(stream_cursor, FRAME_ALIGNMENT)
        if frame.offset != expected_offset:
            fail("M98I_STREAM_LAYOUT", "scale stream layout is noncanonical")
        if frame.offset + len(frame.payload) > len(scale_set.stream):
            fail("M98I_STREAM_RANGE", "frame range exceeds the scale stream")
        if any(scale_set.stream[stream_cursor:frame.offset]):
            fail("M98I_STREAM_PADDING", "scale stream padding is nonzero")
        if scale_set.stream[
                frame.offset:frame.offset + len(frame.payload)] != frame.payload:
            fail("M98I_STREAM_PAYLOAD", "scale stream frame payload differs")
        for row in range(frame.height):
            row_start = row * frame.pitch
            if any(frame.payload[
                    row_start + frame.width:row_start + frame.pitch]):
                fail("M98I_ROW_PADDING", "frame row padding is nonzero")
        stream_cursor = frame.offset + len(frame.payload)

    if stream_cursor != len(scale_set.stream):
        fail("M98I_STREAM_LAYOUT", "scale stream contains trailing bytes")


def pack_header(required_bank_count: int, payload_bytes: int, file_size: int,
                payload_crc32: int, file_crc32: int) -> bytes:
    return struct.pack(
        HEADER_FORMAT,
        MAGIC,
        VERSION,
        HEADER_SIZE,
        0,
        POSE_COUNT,
        SCALE_COUNT,
        DESCRIPTOR_SIZE,
        0,
        BANK_SIZE,
        required_bank_count,
        FIRST_BANK_VALUE,
        DESCRIPTOR_OFFSET,
        DESCRIPTOR_BYTES,
        PAYLOAD_OFFSET,
        payload_bytes,
        file_size,
        payload_crc32,
        file_crc32,
        0,
    )


def validate_production_packing(
        header: format_inspector.Header,
        descriptors: tuple[format_inspector.Descriptor, ...]) -> PackingPlan:
    plan = plan_bank_layout(tuple(
        descriptor.payload_bytes for descriptor in descriptors))
    if header.required_bank_count != plan.required_bank_count:
        fail("M98I_REQUIRED_BANK_COUNT", "required bank count is not minimal")
    for descriptor, placement in zip(descriptors, plan.placements):
        if (descriptor.bank_slot, descriptor.bank_offset) != (
                placement.bank_slot, placement.bank_offset):
            fail("M98I_NONMINIMAL_LAYOUT", "frame bank placement is noncanonical")
    require_one_bank(plan)
    return plan


def require_one_bank(plan: PackingPlan) -> None:
    if plan.required_bank_count != 1:
        fail("M98I_ATLAS_BANK_COUNT", "atlas must fit one BMS bank")


def inspect_packed_bytes(
        contents: bytes,
) -> tuple[format_inspector.Header,
           tuple[format_inspector.Descriptor, ...], PackingPlan]:
    header, descriptors = format_inspector.inspect_bytes(contents)
    plan = validate_production_packing(header, descriptors)
    return header, descriptors, plan


def inspect_packed_file(
        input_file: Path,
) -> tuple[format_inspector.Header,
           tuple[format_inspector.Descriptor, ...], PackingPlan]:
    return inspect_packed_bytes(format_inspector.read_regular_file(input_file))


def build_atlas(scale_set: scaler.ScaleSet) -> PackedAtlas:
    if (struct.calcsize(HEADER_FORMAT), struct.calcsize(DESCRIPTOR_FORMAT),
            PAYLOAD_OFFSET) != (HEADER_SIZE, DESCRIPTOR_SIZE, 1024):
        fail("M98I_FORMAT_SIZE", "atlas format size differs")
    validate_scale_set(scale_set)
    plan = plan_bank_layout(tuple(
        len(frame.payload) for frame in scale_set.frames))
    require_one_bank(plan)

    payload = bytearray()
    descriptor_values = []
    file_alignment_bytes = 0
    for frame, placement in zip(scale_set.frames, plan.placements):
        absolute_offset = PAYLOAD_OFFSET + len(payload)
        aligned_offset = align_up(absolute_offset, FRAME_ALIGNMENT)
        file_padding = aligned_offset - absolute_offset
        payload.extend(b"\x00" * file_padding)
        file_alignment_bytes += file_padding
        file_offset = PAYLOAD_OFFSET + len(payload)
        descriptor_values.append((
            frame.width,
            frame.height,
            frame.pitch,
            frame.anchor_x,
            frame.anchor_y,
            placement.bank_slot,
            0,
            0,
            placement.bank_offset,
            file_offset,
            len(frame.payload),
            zlib.crc32(frame.payload) & 0xffffffff,
        ))
        payload.extend(frame.payload)

    descriptors = b"".join(
        struct.pack(DESCRIPTOR_FORMAT, *values)
        for values in descriptor_values
    )
    payload_contents = bytes(payload)
    payload_crc32 = zlib.crc32(payload_contents) & 0xffffffff
    file_size = PAYLOAD_OFFSET + len(payload_contents)
    header = pack_header(
        plan.required_bank_count, len(payload_contents), file_size,
        payload_crc32, 0)
    contents = header + descriptors + payload_contents
    if len(contents) != file_size:
        fail("M98I_FILE_SIZE", "packed atlas file size differs")
    file_crc32 = zlib.crc32(contents) & 0xffffffff
    header = pack_header(
        plan.required_bank_count, len(payload_contents), file_size,
        payload_crc32, file_crc32)
    contents = header + descriptors + payload_contents

    inspected_header, inspected_descriptors, inspected_plan = inspect_packed_bytes(
        contents)
    if inspected_plan != plan:
        fail("M98I_PLAN_AGREEMENT", "encoded atlas packing plan differs")

    useful_pixel_bytes = sum(
        frame.width * frame.height for frame in scale_set.frames)
    row_padding_bytes = sum(
        (frame.pitch - frame.width) * frame.height
        for frame in scale_set.frames)
    frame_payload_bytes = sum(
        len(frame.payload) for frame in scale_set.frames)
    bms_span_bytes = (
        (plan.required_bank_count - 1) * BANK_SIZE
        + plan.bank_occupied_bytes[-1]
    )
    report: dict[str, object] = {
        "copyright": manifest_validator.COPYRIGHT,
        "format": {
            "atlas_version": VERSION,
            "bank_size": BANK_SIZE,
            "file_frame_alignment": FRAME_ALIGNMENT,
            "first_bank_value": FIRST_BANK_VALUE,
            "scale_count": SCALE_COUNT,
        },
        "license": manifest_validator.LICENSE,
        "metrics": {
            "atlas_file_bytes": len(contents),
            "bank_boundary_padding_bytes": plan.bank_boundary_padding_bytes,
            "bank_frame_alignment_bytes": plan.frame_alignment_bytes,
            "bank_occupied_bytes": list(plan.bank_occupied_bytes),
            "bank_payload_bytes": list(plan.bank_payload_bytes),
            "bms_span_bytes": bms_span_bytes,
            "compact_file_alignment_bytes": file_alignment_bytes,
            "frame_payload_bytes": frame_payload_bytes,
            "payload_region_bytes": inspected_header.payload_bytes,
            "required_bank_count": inspected_header.required_bank_count,
            "row_padding_bytes": row_padding_bytes,
            "useful_pixel_bytes": useful_pixel_bytes,
        },
        "schema": REPORT_SCHEMA,
        "schema_version": REPORT_VERSION,
    }
    if len(inspected_descriptors) != SCALE_COUNT:
        fail("M98I_DESCRIPTOR_COUNT", "encoded descriptor count differs")
    return PackedAtlas(contents=contents, report=report)


def build_public_fixture() -> PackedAtlas:
    return build_atlas(format_fixture.public_scale_set())


def report_bytes(report: dict[str, object]) -> bytes:
    return (json.dumps(report, indent=2, sort_keys=True) + "\n").encode("utf-8")


def write_public_fixture(output: Path) -> None:
    if output.exists():
        fail("M98I_OUTPUT_EXISTS", "output directory exists")
    packed = build_public_fixture()
    encoded_report = report_bytes(packed.report)
    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.mkdir()
        (output / ATLAS_NAME).write_bytes(packed.contents)
        (output / REPORT_NAME).write_bytes(encoded_report)
    except OSError as error:
        raise PackingError("M98I_WRITE", "output could not be written") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument("--fixture-output", type=Path,
                           help="new public packed-fixture directory")
    operation.add_argument("--inspect", type=Path,
                           help="packed version-1 atlas file")
    arguments = parser.parse_args(argv)
    try:
        if arguments.fixture_output is not None:
            write_public_fixture(arguments.fixture_output)
            print("M98I_PACK_PASS")
        else:
            inspect_packed_file(arguments.inspect)
            print("M98I_PACKING_PASS")
    except (PackingError, format_inspector.AtlasError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

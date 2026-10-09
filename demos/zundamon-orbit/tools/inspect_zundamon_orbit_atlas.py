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

"""Independently inspect one M98 version-1 Zundamon orbit atlas."""

from __future__ import annotations

import argparse
import stat
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn


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
MAX_DIMENSION = 4096
MAX_ATLAS_BYTES = PAYLOAD_OFFSET + SCALE_COUNT * (BANK_SIZE + 15)


class AtlasError(Exception):
    """A stable M98h atlas validation failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise AtlasError(code, detail)


@dataclass(frozen=True)
class Header:
    flags: int
    required_bank_count: int
    first_bank_value: int
    descriptor_offset: int
    descriptor_bytes: int
    payload_offset: int
    payload_bytes: int
    file_size: int
    payload_crc32: int
    file_crc32: int


@dataclass(frozen=True)
class Descriptor:
    width: int
    height: int
    pitch: int
    anchor_x: int
    anchor_y: int
    bank_slot: int
    flags: int
    reserved: int
    bank_offset: int
    file_offset: int
    payload_bytes: int
    frame_crc32: int


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


def read_regular_file(input_file: Path) -> bytes:
    try:
        file_status = input_file.lstat()
    except OSError:
        fail("M98H_FILE_READ", "atlas could not be read")
    if not stat.S_ISREG(file_status.st_mode) or input_file.is_symlink():
        fail("M98H_FILE_TYPE", "atlas must be a regular non-symlink file")
    if file_status.st_size > MAX_ATLAS_BYTES:
        fail("M98H_FILE_LIMIT", "atlas exceeds the size limit")
    try:
        contents = input_file.read_bytes()
    except OSError:
        fail("M98H_FILE_READ", "atlas could not be read")
    if len(contents) > MAX_ATLAS_BYTES:
        fail("M98H_FILE_LIMIT", "atlas exceeds the size limit")
    return contents


def parse_header(contents: bytes) -> Header:
    if struct.calcsize(HEADER_FORMAT) != HEADER_SIZE or len(contents) < HEADER_SIZE:
        fail("M98H_HEADER", "atlas header is incomplete")
    (magic, version, header_size, flags, pose_count, scale_count,
     descriptor_size, reserved0, bank_size, required_bank_count,
     first_bank_value, descriptor_offset, descriptor_bytes, payload_offset,
     payload_bytes, file_size, payload_crc32, file_crc32,
     reserved1) = struct.unpack_from(HEADER_FORMAT, contents, 0)
    if magic != MAGIC:
        fail("M98H_MAGIC", "atlas magic differs")
    if version != VERSION:
        fail("M98H_VERSION", "atlas version differs")
    if header_size != HEADER_SIZE:
        fail("M98H_HEADER_SIZE", "header size differs")
    if flags != 0:
        fail("M98H_HEADER_FLAGS", "header flags are nonzero")
    if pose_count != POSE_COUNT:
        fail("M98H_POSE_COUNT", "pose count must be one")
    if scale_count != SCALE_COUNT:
        fail("M98H_SCALE_COUNT", "scale count must be 30")
    if descriptor_size != DESCRIPTOR_SIZE:
        fail("M98H_DESCRIPTOR_SIZE", "descriptor size differs")
    if reserved0 != 0 or reserved1 != 0:
        fail("M98H_HEADER_RESERVED", "reserved header field is nonzero")
    if bank_size != BANK_SIZE:
        fail("M98H_BANK_SIZE", "BMS bank size differs")
    if not 1 <= required_bank_count <= SCALE_COUNT:
        fail("M98H_BANK_COUNT", "required bank count is outside 1-30")
    if first_bank_value != FIRST_BANK_VALUE:
        fail("M98H_FIRST_BANK", "first BMS selector value differs")
    if first_bank_value + required_bank_count - 1 > 255:
        fail("M98H_BANK_SELECTOR_RANGE", "BMS selector range exceeds one byte")
    if descriptor_offset != DESCRIPTOR_OFFSET:
        fail("M98H_DESCRIPTOR_OFFSET", "descriptor offset differs")
    if descriptor_bytes != DESCRIPTOR_BYTES:
        fail("M98H_DESCRIPTOR_BYTES", "descriptor byte count differs")
    if payload_offset != PAYLOAD_OFFSET:
        fail("M98H_PAYLOAD_OFFSET", "payload offset differs")
    if payload_bytes < 1 or payload_offset + payload_bytes != file_size:
        fail("M98H_PAYLOAD_BOUNDS", "payload bounds differ")
    if file_size != len(contents):
        fail("M98H_FILE_SIZE", "declared file size differs")
    return Header(
        flags=flags,
        required_bank_count=required_bank_count,
        first_bank_value=first_bank_value,
        descriptor_offset=descriptor_offset,
        descriptor_bytes=descriptor_bytes,
        payload_offset=payload_offset,
        payload_bytes=payload_bytes,
        file_size=file_size,
        payload_crc32=payload_crc32,
        file_crc32=file_crc32,
    )


def parse_descriptors(contents: bytes, header: Header) -> tuple[Descriptor, ...]:
    if struct.calcsize(DESCRIPTOR_FORMAT) != DESCRIPTOR_SIZE:
        fail("M98H_DESCRIPTOR_SIZE", "inspector descriptor size differs")
    descriptors = []
    for index in range(SCALE_COUNT):
        offset = header.descriptor_offset + index * DESCRIPTOR_SIZE
        values = struct.unpack_from(DESCRIPTOR_FORMAT, contents, offset)
        descriptor = Descriptor(*values)
        if not (1 <= descriptor.width <= MAX_DIMENSION
                and 1 <= descriptor.height <= MAX_DIMENSION):
            fail("M98H_DIMENSIONS", "frame dimensions are invalid")
        if descriptor.pitch != align_up(descriptor.width, 4):
            fail("M98H_PITCH", "frame pitch is noncanonical")
        if not (0 <= descriptor.anchor_x < descriptor.width
                and 0 <= descriptor.anchor_y < descriptor.height):
            fail("M98H_ANCHOR", "frame anchor is out of bounds")
        if descriptor.flags != 0:
            fail("M98H_DESCRIPTOR_FLAGS", "descriptor flags are nonzero")
        if descriptor.reserved != 0:
            fail("M98H_DESCRIPTOR_RESERVED", "descriptor reserved field is nonzero")
        if descriptor.bank_slot >= header.required_bank_count:
            fail("M98H_BANK_SLOT", "logical bank slot is out of range")
        if descriptor.bank_offset % 16 != 0:
            fail("M98H_BANK_OFFSET_ALIGNMENT", "bank offset is not 16-byte aligned")
        if descriptor.payload_bytes != descriptor.pitch * descriptor.height:
            fail("M98H_PAYLOAD_LENGTH", "frame payload length differs")
        if descriptor.bank_offset + descriptor.payload_bytes > BANK_SIZE:
            fail("M98H_BANK_CROSSING", "frame crosses a BMS bank boundary")
        if descriptor.file_offset % 16 != 0:
            fail("M98H_FILE_OFFSET_ALIGNMENT", "file offset is not 16-byte aligned")
        if (descriptor.file_offset < header.payload_offset
                or descriptor.file_offset + descriptor.payload_bytes > header.file_size):
            fail("M98H_FILE_RANGE", "frame file range is out of bounds")
        descriptors.append(descriptor)
    return tuple(descriptors)


def projected_coordinate(anchor: int, source_size: int, target_size: int) -> int:
    return min(
        target_size - 1,
        ((2 * anchor + 1) * target_size) // (2 * source_size),
    )


def validate_canonical_geometry(descriptors: tuple[Descriptor, ...]) -> None:
    source = descriptors[-1]
    for index, descriptor in enumerate(descriptors):
        level = index + 1
        numerator = level if level < SCALE_COUNT else SCALE_DENOMINATOR
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
        if (descriptor.width, descriptor.height) != (expected_width, expected_height):
            fail("M98H_SCALE_GEOMETRY", "scale geometry is noncanonical")
        expected_anchor_x = projected_coordinate(
            source.anchor_x, source.width, descriptor.width)
        expected_anchor_y = projected_coordinate(
            source.anchor_y, source.height, descriptor.height)
        if (descriptor.anchor_x, descriptor.anchor_y) != (
                expected_anchor_x, expected_anchor_y):
            fail("M98H_ANCHOR_GEOMETRY", "scaled anchor is noncanonical")


def validate_layout(header: Header,
                    descriptors: tuple[Descriptor, ...]) -> None:
    expected_file_offset = header.payload_offset
    previous_bank_slot = 0
    bank_ranges: dict[int, list[tuple[int, int]]] = {}
    used_bank_slots = set()
    for index, descriptor in enumerate(descriptors):
        expected_file_offset = align_up(expected_file_offset, 16)
        if descriptor.file_offset != expected_file_offset:
            fail("M98H_FILE_LAYOUT", "frame file layout is noncanonical")
        expected_file_offset += descriptor.payload_bytes
        if index != 0 and descriptor.bank_slot < previous_bank_slot:
            fail("M98H_BANK_ORDER", "logical bank slots decrease")
        previous_bank_slot = descriptor.bank_slot
        used_bank_slots.add(descriptor.bank_slot)
        bank_ranges.setdefault(descriptor.bank_slot, []).append((
            descriptor.bank_offset,
            descriptor.bank_offset + descriptor.payload_bytes,
        ))
    if expected_file_offset != header.file_size:
        fail("M98H_FILE_LAYOUT", "final frame does not end at file size")
    for ranges in bank_ranges.values():
        ranges.sort()
        for previous, current in zip(ranges, ranges[1:]):
            if current[0] < previous[1]:
                fail("M98H_BANK_OVERLAP", "frame ranges overlap within a bank")
    if used_bank_slots != set(range(header.required_bank_count)):
        fail("M98H_BANK_USAGE", "logical bank slots are not contiguous")


def validate_padding(contents: bytes, header: Header,
                     descriptors: tuple[Descriptor, ...]) -> None:
    cursor = header.payload_offset
    for descriptor in descriptors:
        if any(contents[cursor:descriptor.file_offset]):
            fail("M98H_FILE_PADDING", "inter-frame padding is nonzero")
        payload = contents[
            descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes]
        for row in range(descriptor.height):
            row_start = row * descriptor.pitch
            if any(payload[row_start + descriptor.width:row_start + descriptor.pitch]):
                fail("M98H_ROW_PADDING", "row padding is nonzero")
        cursor = descriptor.file_offset + descriptor.payload_bytes


def validate_frame_crcs(contents: bytes,
                        descriptors: tuple[Descriptor, ...]) -> None:
    for descriptor in descriptors:
        payload = contents[
            descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes]
        if zlib.crc32(payload) & 0xffffffff != descriptor.frame_crc32:
            fail("M98H_FRAME_CRC", "frame CRC32 differs")


def validate_payload_crc(contents: bytes, header: Header) -> None:
    payload = contents[header.payload_offset:header.file_size]
    if zlib.crc32(payload) & 0xffffffff != header.payload_crc32:
        fail("M98H_PAYLOAD_CRC", "payload CRC32 differs")


def validate_file_crc(contents: bytes, header: Header) -> None:
    crc_input = bytearray(contents)
    crc_input[FILE_CRC_OFFSET:FILE_CRC_OFFSET + 4] = b"\x00\x00\x00\x00"
    if zlib.crc32(crc_input) & 0xffffffff != header.file_crc32:
        fail("M98H_FILE_CRC", "file CRC32 differs")


def inspect_bytes(contents: bytes) -> tuple[Header, tuple[Descriptor, ...]]:
    header = parse_header(contents)
    descriptors = parse_descriptors(contents, header)
    validate_canonical_geometry(descriptors)
    validate_layout(header, descriptors)
    validate_padding(contents, header, descriptors)
    validate_frame_crcs(contents, descriptors)
    validate_payload_crc(contents, header)
    validate_file_crc(contents, header)
    return header, descriptors


def inspect_file(input_file: Path) -> tuple[Header, tuple[Descriptor, ...]]:
    return inspect_bytes(read_regular_file(input_file))


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True,
                        help="version-1 atlas file")
    arguments = parser.parse_args(argv)
    try:
        inspect_file(arguments.input)
    except AtlasError as error:
        print(error, file=sys.stderr)
        return 1
    print("M98H_ATLAS_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

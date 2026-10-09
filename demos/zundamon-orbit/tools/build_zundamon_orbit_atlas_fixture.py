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

"""Build the deterministic public M98h version-1 atlas-format fixture."""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path

import build_zundamon_orbit_asset as asset
import convert_zundamon_orbit_va8 as va8_converter
import generate_zundamon_orbit_scales as scaler


MAGIC = b"ZUNDORB\x00"
VERSION = 1
HEADER_FORMAT = "<8sHHIHHHHIHHIIIIIIII"
DESCRIPTOR_FORMAT = "<HHHHHHHHIIII"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
DESCRIPTOR_SIZE = struct.calcsize(DESCRIPTOR_FORMAT)
POSE_COUNT = 1
SCALE_COUNT = 30
BANK_SIZE = 0x00020000
FIRST_BANK_VALUE = 1
DESCRIPTOR_OFFSET = HEADER_SIZE
DESCRIPTOR_BYTES = SCALE_COUNT * DESCRIPTOR_SIZE
PAYLOAD_OFFSET = (DESCRIPTOR_OFFSET + DESCRIPTOR_BYTES + 15) & ~15
FILE_CRC_OFFSET = 56


class FixtureError(Exception):
    """A deterministic M98h format-fixture build failure."""


def public_scale_set() -> scaler.ScaleSet:
    source = bytes(
        0 if index == 0 else va8_converter.convert_opaque_rgb(
            asset.PALETTE_RGB[index])[0]
        for index in asset.build_pixels()
    )
    return scaler.build_scale_set(
        source,
        asset.WIDTH,
        asset.HEIGHT,
        asset.WIDTH // 2,
        asset.HEIGHT // 2,
    )


def pack_header(payload_bytes: int, file_size: int, payload_crc32: int,
                file_crc32: int) -> bytes:
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
        SCALE_COUNT,
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


def build_fixture() -> bytes:
    if HEADER_SIZE != 64 or DESCRIPTOR_SIZE != 32 or PAYLOAD_OFFSET != 1024:
        raise FixtureError("M98H_FIXTURE_FORMAT_SIZE: format size differs")
    scale_set = public_scale_set()
    if len(scale_set.frames) != SCALE_COUNT:
        raise FixtureError("M98H_FIXTURE_SCALE_COUNT: scale count differs")

    payload = bytearray()
    descriptor_values = []
    for bank_slot, frame in enumerate(scale_set.frames):
        if len(frame.payload) > BANK_SIZE:
            raise FixtureError("M98H_FIXTURE_FRAME_SIZE: public frame exceeds one bank")
        absolute_offset = PAYLOAD_OFFSET + len(payload)
        aligned_offset = (absolute_offset + 15) & ~15
        payload.extend(b"\x00" * (aligned_offset - absolute_offset))
        file_offset = PAYLOAD_OFFSET + len(payload)
        descriptor_values.append((
            frame.width,
            frame.height,
            frame.pitch,
            frame.anchor_x,
            frame.anchor_y,
            bank_slot,
            0,
            0,
            0,
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
    header = pack_header(len(payload_contents), file_size, payload_crc32, 0)
    contents = header + descriptors + payload_contents
    if len(contents) != file_size:
        raise FixtureError("M98H_FIXTURE_FILE_SIZE: fixture size differs")
    file_crc32 = zlib.crc32(contents) & 0xffffffff
    header = pack_header(
        len(payload_contents), file_size, payload_crc32, file_crc32)
    contents = header + descriptors + payload_contents
    if contents[FILE_CRC_OFFSET:FILE_CRC_OFFSET + 4] == b"\x00\x00\x00\x00":
        raise FixtureError("M98H_FIXTURE_FILE_CRC: file CRC is unexpectedly zero")
    return contents


def write_fixture(output: Path) -> None:
    if output.exists():
        raise FixtureError("M98H_FIXTURE_OUTPUT_EXISTS: output file exists")
    contents = build_fixture()
    try:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(contents)
    except OSError as error:
        raise FixtureError("M98H_FIXTURE_WRITE: output could not be written") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="new public atlas-format fixture file")
    arguments = parser.parse_args(argv)
    try:
        write_fixture(arguments.output)
    except FixtureError as error:
        print(error, file=sys.stderr)
        return 1
    print("M98H_FIXTURE_BUILD_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

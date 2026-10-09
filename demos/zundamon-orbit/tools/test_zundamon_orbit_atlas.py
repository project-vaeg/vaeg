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

"""Test the M98h version-1 atlas writer and independent inspector."""

from __future__ import annotations

import dataclasses
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Callable

import build_zundamon_orbit_atlas_fixture as builder
import inspect_zundamon_orbit_atlas as inspector


TOOL_DIRECTORY = Path(__file__).resolve().parent
BUILDER = TOOL_DIRECTORY / "build_zundamon_orbit_atlas_fixture.py"
INSPECTOR = TOOL_DIRECTORY / "inspect_zundamon_orbit_atlas.py"
Mutation = Callable[[bytearray], None]


def set_u16(contents: bytearray, offset: int, value: int) -> None:
    contents[offset:offset + 2] = value.to_bytes(2, "little")


def set_u32(contents: bytearray, offset: int, value: int) -> None:
    contents[offset:offset + 4] = value.to_bytes(4, "little")


def descriptor_offset(index: int, field_offset: int = 0) -> int:
    return builder.DESCRIPTOR_OFFSET + index * builder.DESCRIPTOR_SIZE + field_offset


class ZundamonOrbitAtlasTests(unittest.TestCase):
    def valid_contents(self) -> bytes:
        contents = builder.build_fixture()
        inspector.inspect_bytes(contents)
        return contents

    def assert_atlas_error(self, contents: bytes, expected_code: str) -> None:
        with self.assertRaises(inspector.AtlasError) as caught:
            inspector.inspect_bytes(contents)
        self.assertEqual(caught.exception.code, expected_code)

    def test_fixture_is_reproducible_and_inspects(self) -> None:
        first = builder.build_fixture()
        second = builder.build_fixture()
        self.assertEqual(first, second)
        first_header, first_descriptors = inspector.inspect_bytes(first)
        second_header, second_descriptors = inspector.inspect_bytes(second)
        self.assertEqual(first_header, second_header)
        self.assertEqual(first_descriptors, second_descriptors)

    def test_header_and_descriptor_contract(self) -> None:
        contents = self.valid_contents()
        header, descriptors = inspector.inspect_bytes(contents)
        self.assertEqual(builder.HEADER_SIZE, 64)
        self.assertEqual(builder.DESCRIPTOR_SIZE, 32)
        self.assertEqual(builder.PAYLOAD_OFFSET, 1024)
        self.assertEqual(header.required_bank_count, 30)
        self.assertEqual(header.first_bank_value, 1)
        self.assertEqual(len(descriptors), 30)
        self.assertEqual([descriptor.bank_slot for descriptor in descriptors],
                         list(range(30)))
        self.assertTrue(all(descriptor.bank_offset == 0
                            for descriptor in descriptors))
        self.assertEqual(header.payload_offset + header.payload_bytes,
                         header.file_size)
        self.assertEqual(header.file_size, len(contents))

    def test_header_failures_reach_exact_codes(self) -> None:
        cases: tuple[tuple[str, Mutation, str], ...] = (
            ("magic", lambda data: data.__setitem__(0, ord("X")), "M98H_MAGIC"),
            ("version", lambda data: set_u16(data, 8, 2), "M98H_VERSION"),
            ("header size", lambda data: set_u16(data, 10, 63),
             "M98H_HEADER_SIZE"),
            ("flags", lambda data: set_u32(data, 12, 1), "M98H_HEADER_FLAGS"),
            ("pose count", lambda data: set_u16(data, 16, 2),
             "M98H_POSE_COUNT"),
            ("scale count", lambda data: set_u16(data, 18, 29),
             "M98H_SCALE_COUNT"),
            ("descriptor size", lambda data: set_u16(data, 20, 31),
             "M98H_DESCRIPTOR_SIZE"),
            ("reserved zero", lambda data: set_u16(data, 22, 1),
             "M98H_HEADER_RESERVED"),
            ("bank size", lambda data: set_u32(data, 24, 0x10000),
             "M98H_BANK_SIZE"),
            ("bank count", lambda data: set_u16(data, 28, 0),
             "M98H_BANK_COUNT"),
            ("first bank", lambda data: set_u16(data, 30, 0),
             "M98H_FIRST_BANK"),
            ("descriptor offset", lambda data: set_u32(data, 32, 68),
             "M98H_DESCRIPTOR_OFFSET"),
            ("descriptor bytes", lambda data: set_u32(data, 36, 1000),
             "M98H_DESCRIPTOR_BYTES"),
            ("payload offset", lambda data: set_u32(data, 40, 1040),
             "M98H_PAYLOAD_OFFSET"),
            ("payload bounds", lambda data: set_u32(data, 44, 0),
             "M98H_PAYLOAD_BOUNDS"),
            ("reserved one", lambda data: set_u32(data, 60, 1),
             "M98H_HEADER_RESERVED"),
        )
        valid = self.valid_contents()
        for name, mutation, expected_code in cases:
            with self.subTest(name=name):
                changed = bytearray(valid)
                mutation(changed)
                self.assert_atlas_error(bytes(changed), expected_code)
        self.assert_atlas_error(valid[:-1], "M98H_FILE_SIZE")
        self.assert_atlas_error(valid + b"\x00", "M98H_FILE_SIZE")

    def test_descriptor_failures_reach_exact_codes(self) -> None:
        valid = self.valid_contents()
        _, descriptors = inspector.inspect_bytes(valid)
        first = descriptors[0]
        last = descriptors[-1]
        cases: tuple[tuple[str, Mutation, str], ...] = (
            ("dimensions", lambda data: set_u16(
                data, descriptor_offset(0, 0), 0), "M98H_DIMENSIONS"),
            ("pitch", lambda data: set_u16(
                data, descriptor_offset(0, 4), 0), "M98H_PITCH"),
            ("anchor", lambda data: set_u16(
                data, descriptor_offset(0, 6), first.width), "M98H_ANCHOR"),
            ("flags", lambda data: set_u16(
                data, descriptor_offset(0, 12), 1), "M98H_DESCRIPTOR_FLAGS"),
            ("reserved", lambda data: set_u16(
                data, descriptor_offset(0, 14), 1), "M98H_DESCRIPTOR_RESERVED"),
            ("bank slot", lambda data: set_u16(
                data, descriptor_offset(0, 10), 30), "M98H_BANK_SLOT"),
            ("bank alignment", lambda data: set_u32(
                data, descriptor_offset(0, 16), 1),
             "M98H_BANK_OFFSET_ALIGNMENT"),
            ("bank crossing", lambda data: set_u32(
                data, descriptor_offset(0, 16), inspector.BANK_SIZE),
             "M98H_BANK_CROSSING"),
            ("file alignment", lambda data: set_u32(
                data, descriptor_offset(0, 20), first.file_offset + 1),
             "M98H_FILE_OFFSET_ALIGNMENT"),
            ("file range", lambda data: set_u32(
                data, descriptor_offset(29, 20),
                inspector.align_up(last.file_offset + last.payload_bytes, 16)),
             "M98H_FILE_RANGE"),
            ("payload length", lambda data: set_u32(
                data, descriptor_offset(0, 24), first.payload_bytes + 1),
             "M98H_PAYLOAD_LENGTH"),
        )
        for name, mutation, expected_code in cases:
            with self.subTest(name=name):
                changed = bytearray(valid)
                mutation(changed)
                self.assert_atlas_error(bytes(changed), expected_code)

    def test_canonical_geometry_failures_use_isolated_layer(self) -> None:
        contents = self.valid_contents()
        _, descriptors = inspector.inspect_bytes(contents)
        inspector.validate_canonical_geometry(descriptors)
        changed_dimensions = list(descriptors)
        changed_dimensions[0] = dataclasses.replace(
            changed_dimensions[0], width=changed_dimensions[0].width + 1)
        with self.assertRaises(inspector.AtlasError) as caught:
            inspector.validate_canonical_geometry(tuple(changed_dimensions))
        self.assertEqual(caught.exception.code, "M98H_SCALE_GEOMETRY")

        anchor_index = next(
            index for index, descriptor in enumerate(descriptors)
            if descriptor.width > 1)
        changed_anchor = list(descriptors)
        descriptor = changed_anchor[anchor_index]
        replacement = (descriptor.anchor_x + 1) % descriptor.width
        changed_anchor[anchor_index] = dataclasses.replace(
            descriptor, anchor_x=replacement)
        with self.assertRaises(inspector.AtlasError) as caught:
            inspector.validate_canonical_geometry(tuple(changed_anchor))
        self.assertEqual(caught.exception.code, "M98H_ANCHOR_GEOMETRY")

    def test_layout_failures_use_isolated_layer(self) -> None:
        contents = self.valid_contents()
        header, descriptors = inspector.inspect_bytes(contents)
        inspector.validate_layout(header, descriptors)

        changed = list(descriptors)
        changed[1] = dataclasses.replace(
            changed[1], file_offset=changed[1].file_offset + 16)
        with self.assertRaises(inspector.AtlasError) as caught:
            inspector.validate_layout(header, tuple(changed))
        self.assertEqual(caught.exception.code, "M98H_FILE_LAYOUT")

        changed = list(descriptors)
        changed[2] = dataclasses.replace(changed[2], bank_slot=0)
        with self.assertRaises(inspector.AtlasError) as caught:
            inspector.validate_layout(header, tuple(changed))
        self.assertEqual(caught.exception.code, "M98H_BANK_ORDER")

        changed = list(descriptors)
        changed[1] = dataclasses.replace(changed[1], bank_slot=0, bank_offset=0)
        with self.assertRaises(inspector.AtlasError) as caught:
            inspector.validate_layout(header, tuple(changed))
        self.assertEqual(caught.exception.code, "M98H_BANK_OVERLAP")

        changed = list(descriptors)
        changed[1] = dataclasses.replace(
            changed[1],
            bank_slot=0,
            bank_offset=inspector.align_up(descriptors[0].payload_bytes, 16),
        )
        with self.assertRaises(inspector.AtlasError) as caught:
            inspector.validate_layout(header, tuple(changed))
        self.assertEqual(caught.exception.code, "M98H_BANK_USAGE")

    def test_padding_and_crc_failures_reach_exact_codes(self) -> None:
        valid = self.valid_contents()
        header, descriptors = inspector.inspect_bytes(valid)

        gap_index = next(
            index for index in range(1, len(descriptors))
            if descriptors[index].file_offset
            > descriptors[index - 1].file_offset + descriptors[index - 1].payload_bytes)
        changed = bytearray(valid)
        gap_offset = (descriptors[gap_index - 1].file_offset
                      + descriptors[gap_index - 1].payload_bytes)
        changed[gap_offset] = 1
        self.assert_atlas_error(bytes(changed), "M98H_FILE_PADDING")

        padded = next(descriptor for descriptor in descriptors
                      if descriptor.width < descriptor.pitch)
        changed = bytearray(valid)
        changed[padded.file_offset + padded.width] = 1
        self.assert_atlas_error(bytes(changed), "M98H_ROW_PADDING")

        changed = bytearray(valid)
        changed[descriptors[0].file_offset] ^= 1
        self.assert_atlas_error(bytes(changed), "M98H_FRAME_CRC")

        changed = bytearray(valid)
        frame_crc_offset = descriptor_offset(0, 28)
        changed[frame_crc_offset] ^= 1
        self.assert_atlas_error(bytes(changed), "M98H_FRAME_CRC")

        changed = bytearray(valid)
        changed[52] ^= 1
        self.assert_atlas_error(bytes(changed), "M98H_PAYLOAD_CRC")

        changed = bytearray(valid)
        changed[inspector.FILE_CRC_OFFSET] ^= 1
        self.assert_atlas_error(bytes(changed), "M98H_FILE_CRC")
        inspector.validate_payload_crc(valid, header)
        inspector.validate_file_crc(valid, header)

    def test_cli_privacy_file_type_and_overwrite(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98h-") as temporary:
            root = Path(temporary)
            atlas = root / "atlas.bin"
            build = subprocess.run(
                [sys.executable, str(BUILDER), "--output", str(atlas)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(build.returncode, 0, build.stderr)
            self.assertEqual(build.stdout, "M98H_FIXTURE_BUILD_PASS\n")
            inspect = subprocess.run(
                [sys.executable, str(INSPECTOR), "--input", str(atlas)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(inspect.returncode, 0, inspect.stderr)
            self.assertEqual(inspect.stdout, "M98H_ATLAS_PASS\n")
            overwrite = subprocess.run(
                [sys.executable, str(BUILDER), "--output", str(atlas)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(overwrite.returncode, 0)
            self.assertEqual(
                overwrite.stderr,
                "M98H_FIXTURE_OUTPUT_EXISTS: output file exists\n",
            )
            marker = "localidentitymustnotappear"
            missing = subprocess.run(
                [sys.executable, str(INSPECTOR), "--input", str(root / marker)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(missing.returncode, 0)
            self.assertEqual(missing.stdout, "")
            self.assertEqual(missing.stderr,
                             "M98H_FILE_READ: atlas could not be read\n")
            self.assertNotIn(marker, missing.stderr)
            link = root / "atlas-link.bin"
            link.symlink_to(atlas)
            with self.assertRaises(inspector.AtlasError) as caught:
                inspector.inspect_file(link)
            self.assertEqual(caught.exception.code, "M98H_FILE_TYPE")
            directory = root / "atlas-directory"
            directory.mkdir()
            with self.assertRaises(inspector.AtlasError) as caught:
                inspector.inspect_file(directory)
            self.assertEqual(caught.exception.code, "M98H_FILE_TYPE")
            oversized = root / "atlas-oversized.bin"
            with oversized.open("wb") as output:
                output.truncate(inspector.MAX_ATLAS_BYTES + 1)
            with self.assertRaises(inspector.AtlasError) as caught:
                inspector.inspect_file(oversized)
            self.assertEqual(caught.exception.code, "M98H_FILE_LIMIT")


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(
        ZundamonOrbitAtlasTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98H_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

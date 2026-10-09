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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Tests for the tolerant CP/MVA D88 extractor."""

import importlib.util
from pathlib import Path
import struct
import unittest

HERE = Path(__file__).resolve().parent


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


installer = load("installer_for_extract_test", HERE.parent / "install_cpmva.py")
extractor = load("extract_d88", HERE.parent / "zex" / "extract_d88.py")

FILES = {"HELLO.COM": b"\xc9" * 128, "OUT.TXT": b"text\r\n" + b"\x1a" * 122}


def make_image():
    return installer.build_tools_disk(installer.pad_cpm_records(dict(FILES), b"\x00"))


def reorder_like_hxcfe(image):
    """Same sectors, another header name/type and a reversed track order."""
    offsets = struct.unpack_from("<164I", image, 0x20)
    tracks = []
    for index, offset in enumerate(offsets):
        if not offset:
            continue
        end = next((o for o in sorted(offsets) if o > offset), len(image))
        tracks.append(image[offset:end])
    header = bytearray(image[:0x2B0])
    header[0:17] = b"HxCFE".ljust(17, b"\0")
    header[0x1B] = 0x00
    body = bytearray()
    new_offsets = [0] * 164
    for index, track in reversed(list(enumerate(tracks))):
        new_offsets[index] = 0x2B0 + len(body)
        body.extend(track)
    for index, offset in enumerate(new_offsets):
        struct.pack_into("<I", header, 0x20 + index * 4, offset)
    return bytes(header) + bytes(body)


class ExtractTest(unittest.TestCase):
    def test_installer_layout(self):
        self.assertEqual(extractor.extract(make_image()), FILES)

    def test_reordered_foreign_header(self):
        image = reorder_like_hxcfe(make_image())
        with self.assertRaises(installer.InstallerError):
            installer.unwrap_cpm_d88(image)
        self.assertEqual(extractor.extract(image), FILES)

    def test_missing_sector(self):
        image = bytearray(make_image())
        struct.pack_into("<I", image, 0x20 + 79 * 4, 0)
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(bytes(image))
        self.assertEqual(context.exception.code, "D88_MISSING")

    def test_sector_error_status(self):
        image = bytearray(make_image())
        first = struct.unpack_from("<I", image, 0x20)[0]
        image[first + 8] = 0xE0
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(bytes(image))
        self.assertEqual(context.exception.code, "D88_STATUS")

    def mark_sector_error(self, image, cylinder, head, record):
        image = bytearray(image)
        offsets = struct.unpack_from("<164I", image, 0x20)
        for offset in offsets:
            if not offset:
                continue
            cursor = offset
            for _ in range(struct.unpack_from("<H", image, offset + 4)[0]):
                if tuple(image[cursor : cursor + 3]) == (cylinder, head, record):
                    image[cursor + 8] = 0xB0
                    return bytes(image)
                cursor += 16 + struct.unpack_from("<H", image, cursor + 14)[0]
        raise AssertionError("sector not found")

    def test_unused_sector_error_allowed_only_on_request(self):
        image = self.mark_sector_error(make_image(), 36, 0, 6)  # far past the two files
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(image)
        self.assertEqual(context.exception.code, "D88_STATUS")
        self.assertEqual(extractor.extract(image, allow_unused_errors=True), FILES)

    def test_allocated_sector_error_is_rejected(self):
        image = self.mark_sector_error(make_image(), 2, 1, 1)  # block 2 holds HELLO.COM
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(image, allow_unused_errors=True)
        self.assertEqual(context.exception.code, "D88_STATUS")

    def test_directory_sector_error_is_rejected(self):
        image = self.mark_sector_error(make_image(), 2, 0, 1)  # first directory sector
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(image, allow_unused_errors=True)
        self.assertEqual(context.exception.code, "D88_STATUS")

    def patch_directory_entry(self, image, index, entry):
        image = bytearray(image)
        offsets = struct.unpack_from("<164I", image, 0x20)
        for offset in offsets:
            if not offset:
                continue
            cursor = offset
            for _ in range(struct.unpack_from("<H", image, offset + 4)[0]):
                if tuple(image[cursor : cursor + 3]) == (2, 0, 1):  # first directory sector
                    start = cursor + 16 + index * 32
                    image[start : start + 32] = entry
                    return bytes(image)
                cursor += 16 + struct.unpack_from("<H", image, cursor + 14)[0]
        raise AssertionError("sector not found")

    def test_corrupted_entry_dropped_only_on_request(self):
        garbage = bytes([7, 0x10, 0x00, 0x74, 0x07, 0x8B, 0x5F, 0x02, 0x0B, 0xDB, 0x74, 0x12,
                         0x8B, 0, 0x42, 0]) + bytes(16)
        image = self.patch_directory_entry(make_image(), 2, garbage)
        # The extractor loads its own copy of the installer, so match the code.
        with self.assertRaises(Exception) as context:
            extractor.extract(image)
        self.assertEqual(getattr(context.exception, "code", None), "CPM_USER")
        self.assertEqual(extractor.extract(image, drop=[2]), FILES)

    def test_drop_entry_refuses_a_valid_or_unused_entry(self):
        image = make_image()
        for index, code in ((0, "DROP_ENTRY_VALID"), (5, "DROP_ENTRY_UNUSED"),
                            (128, "DROP_ENTRY_RANGE")):
            with self.subTest(index=index):
                with self.assertRaises(extractor.ExtractError) as context:
                    extractor.extract(image, drop=[index])
                self.assertEqual(context.exception.code, code)

    def test_duplicate_sector(self):
        image = bytearray(make_image())
        struct.pack_into("<I", image, 0x20 + 1 * 4, struct.unpack_from("<I", image, 0x20)[0])
        with self.assertRaises(extractor.ExtractError) as context:
            extractor.extract(bytes(image))
        self.assertEqual(context.exception.code, "D88_DUPLICATE")


if __name__ == "__main__":
    unittest.main()

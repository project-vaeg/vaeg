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

"""Test strict M98d BMP, palette, crop, and index recovery."""

from __future__ import annotations

import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Callable

import build_zundamon_orbit_asset as asset
import build_zundamon_orbit_input_fixture as fixture
import inspect_zundamon_orbit_input as inspector
import validate_zundamon_orbit_manifest as manifest_validator


TOOL_DIRECTORY = Path(__file__).resolve().parent
BUILDER = TOOL_DIRECTORY / "build_zundamon_orbit_input_fixture.py"
INSPECTOR = TOOL_DIRECTORY / "inspect_zundamon_orbit_input.py"
Mutation = Callable[[Path], None]


def load_manifest(bundle: Path) -> dict[str, object]:
    value = json.loads((bundle / fixture.MANIFEST_NAME).read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise TypeError("synthetic manifest root differs")
    return value


def write_manifest(bundle: Path, value: dict[str, object]) -> None:
    (bundle / fixture.MANIFEST_NAME).write_text(
        json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def mutate_bmp_field(bundle: Path, offset: int, field_format: str, value: int) -> None:
    image_file = bundle / fixture.IMAGE_NAME
    contents = bytearray(image_file.read_bytes())
    struct.pack_into(field_format, contents, offset, value)
    image_file.write_bytes(contents)


def set_bmp_rgb(contents: bytearray, x: int, y: int,
                color: tuple[int, int, int]) -> None:
    width, stored_height = struct.unpack_from("<ii", contents, 18)
    height = abs(stored_height)
    stored_y = y if stored_height < 0 else height - 1 - y
    offset = inspector.BMP_HEADER_SIZE + (stored_y * width + x) * 4
    red, green, blue = color
    contents[offset:offset + 3] = bytes((blue, green, red))


class ZundamonOrbitInputTests(unittest.TestCase):
    def make_bundle(self, root: Path, top_down: bool = False) -> Path:
        bundle = root / "input"
        fixture.write_fixture(bundle, top_down=top_down)
        return bundle

    def assert_bundle_error(self, bundle: Path, expected_code: str) -> None:
        with self.assertRaises((inspector.InputError,
                                manifest_validator.ManifestError)) as caught:
            inspector.inspect_bundle(bundle / fixture.MANIFEST_NAME)
        self.assertEqual(caught.exception.code, expected_code)

    def test_bottom_up_and_top_down_recover_exact_indices(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98d-") as temporary:
            root = Path(temporary)
            bottom = root / "bottom"
            top = root / "top"
            fixture.write_fixture(bottom)
            fixture.write_fixture(top, top_down=True)
            bottom_result = inspector.inspect_bundle(bottom / fixture.MANIFEST_NAME)
            top_result = inspector.inspect_bundle(top / fixture.MANIFEST_NAME)
            self.assertEqual(bottom_result.indexed_pixels, asset.build_pixels())
            self.assertEqual(top_result.indexed_pixels, asset.build_pixels())
            self.assertEqual(bottom_result, top_result)

    def test_fixture_is_byte_reproducible(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98d-") as temporary:
            root = Path(temporary)
            first = root / "first"
            second = root / "second"
            fixture.write_fixture(first)
            fixture.write_fixture(second)
            for filename in (fixture.IMAGE_NAME, fixture.PALETTE_NAME,
                             fixture.MANIFEST_NAME):
                self.assertEqual((first / filename).read_bytes(),
                                 (second / filename).read_bytes(), filename)

    def test_manifest_and_file_failures_reach_exact_codes(self) -> None:
        def missing_image(bundle: Path) -> None:
            (bundle / fixture.IMAGE_NAME).unlink()

        def short_palette(bundle: Path) -> None:
            palette_file = bundle / fixture.PALETTE_NAME
            palette_file.write_bytes(palette_file.read_bytes()[:-1])

        def oversized_palette(bundle: Path) -> None:
            palette_file = bundle / fixture.PALETTE_NAME
            palette_file.write_bytes(palette_file.read_bytes() + b"\x00")

        def palette_directory(bundle: Path) -> None:
            palette_file = bundle / fixture.PALETTE_NAME
            palette_file.unlink()
            palette_file.mkdir()

        def crop_outside(bundle: Path) -> None:
            value = load_manifest(bundle)
            crop = value["crop"]
            if not isinstance(crop, dict):
                raise TypeError("synthetic crop differs")
            crop["x"] = 1
            write_manifest(bundle, value)

        cases: tuple[tuple[str, Mutation, str], ...] = (
            ("missing image", missing_image, "M98D_BMP_READ"),
            ("short palette", short_palette, "M98D_PALETTE_LENGTH"),
            ("oversized palette", oversized_palette, "M98D_PALETTE_SIZE"),
            ("palette directory", palette_directory, "M98D_PALETTE_FILE_TYPE"),
            ("crop outside image", crop_outside, "M98D_CROP_BOUNDS"),
        )
        for name, mutation, expected_code in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory(
                    prefix="vaeg-m98d-") as temporary:
                bundle = self.make_bundle(Path(temporary))
                mutation(bundle)
                self.assert_bundle_error(bundle, expected_code)

    def test_palette_failures_reach_exact_codes(self) -> None:
        def change_entry(bundle: Path, index: int, color: tuple[int, int, int]) -> None:
            palette_file = bundle / fixture.PALETTE_NAME
            contents = bytearray(palette_file.read_bytes())
            contents[index * 3:index * 3 + 3] = bytes(color)
            palette_file.write_bytes(contents)

        cases: tuple[tuple[str, int, tuple[int, int, int], str], ...] = (
            ("transparent entry", 0, (1, 0, 0),
             "M98D_PALETTE_TRANSPARENT_COLOR"),
            ("reserved entry", 15, (1, 0, 0),
             "M98D_PALETTE_RESERVED_COLOR"),
            ("duplicate visible", 2, asset.PALETTE_RGB[1],
             "M98D_PALETTE_DUPLICATE_VISIBLE"),
            ("background collision", 1, asset.PALETTE_RGB[0],
             "M98D_BACKGROUND_PALETTE_COLLISION"),
        )
        for name, index, color, expected_code in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory(
                    prefix="vaeg-m98d-") as temporary:
                bundle = self.make_bundle(Path(temporary))
                change_entry(bundle, index, color)
                self.assert_bundle_error(bundle, expected_code)

    def test_bmp_header_failures_reach_exact_codes(self) -> None:
        def change_magic(bundle: Path) -> None:
            image_file = bundle / fixture.IMAGE_NAME
            contents = bytearray(image_file.read_bytes())
            contents[0:2] = b"ZZ"
            image_file.write_bytes(contents)

        cases: tuple[tuple[str, Mutation, str], ...] = (
            ("magic", change_magic, "M98D_BMP_MAGIC"),
            ("declared size", lambda bundle: mutate_bmp_field(bundle, 2, "<I", 54),
             "M98D_BMP_FILE_SIZE"),
            ("reserved", lambda bundle: mutate_bmp_field(bundle, 6, "<H", 1),
             "M98D_BMP_RESERVED"),
            ("pixel offset", lambda bundle: mutate_bmp_field(bundle, 10, "<I", 58),
             "M98D_BMP_PIXEL_OFFSET"),
            ("DIB size", lambda bundle: mutate_bmp_field(bundle, 14, "<I", 108),
             "M98D_BMP_DIB_SIZE"),
            ("zero width", lambda bundle: mutate_bmp_field(bundle, 18, "<i", 0),
             "M98D_BMP_WIDTH"),
            ("zero height", lambda bundle: mutate_bmp_field(bundle, 22, "<i", 0),
             "M98D_BMP_HEIGHT"),
            ("planes", lambda bundle: mutate_bmp_field(bundle, 26, "<H", 2),
             "M98D_BMP_PLANES"),
            ("bits per pixel", lambda bundle: mutate_bmp_field(bundle, 28, "<H", 24),
             "M98D_BMP_BPP"),
            ("compression", lambda bundle: mutate_bmp_field(bundle, 30, "<I", 1),
             "M98D_BMP_COMPRESSION"),
            ("image size", lambda bundle: mutate_bmp_field(bundle, 34, "<I", 4),
             "M98D_BMP_IMAGE_SIZE"),
            ("colors used", lambda bundle: mutate_bmp_field(bundle, 46, "<I", 1),
             "M98D_BMP_COLORS_USED"),
            ("important colors", lambda bundle: mutate_bmp_field(bundle, 50, "<I", 1),
             "M98D_BMP_IMPORTANT_COLORS"),
        )
        for name, mutation, expected_code in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory(
                    prefix="vaeg-m98d-") as temporary:
                bundle = self.make_bundle(Path(temporary))
                mutation(bundle)
                self.assert_bundle_error(bundle, expected_code)

    def test_bmp_length_failure_starts_from_valid_zero_image_size(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98d-") as temporary:
            bundle = self.make_bundle(Path(temporary))
            mutate_bmp_field(bundle, 34, "<I", 0)
            inspector.inspect_bundle(bundle / fixture.MANIFEST_NAME)
            mutate_bmp_field(bundle, 18, "<i", 22)
            self.assert_bundle_error(bundle, "M98D_BMP_LENGTH")

    def test_pixel_failures_reach_exact_codes(self) -> None:
        def unexplained_color(bundle: Path) -> None:
            image_file = bundle / fixture.IMAGE_NAME
            contents = bytearray(image_file.read_bytes())
            set_bmp_rgb(contents, 5, 2, (1, 2, 3))
            image_file.write_bytes(contents)

        def all_background(bundle: Path) -> None:
            image_file = bundle / fixture.IMAGE_NAME
            contents = bytearray(image_file.read_bytes())
            for y in range(asset.HEIGHT):
                for x in range(asset.WIDTH):
                    set_bmp_rgb(contents, x, y, asset.PALETTE_RGB[0])
            image_file.write_bytes(contents)

        def no_transparency(bundle: Path) -> None:
            image_file = bundle / fixture.IMAGE_NAME
            contents = bytearray(image_file.read_bytes())
            original = inspector.parse_bmp32(bytes(contents))
            for y in range(asset.HEIGHT):
                for x in range(asset.WIDTH):
                    if original.rgb_at(x, y) == asset.PALETTE_RGB[0]:
                        set_bmp_rgb(contents, x, y, asset.PALETTE_RGB[1])
            image_file.write_bytes(contents)

        cases: tuple[tuple[str, Mutation, str], ...] = (
            ("unexplained color", unexplained_color, "M98D_PIXEL_COLOR"),
            ("all background", all_background, "M98D_CROP_EMPTY"),
            ("no transparency", no_transparency, "M98D_CROP_NO_TRANSPARENCY"),
        )
        for name, mutation, expected_code in cases:
            with self.subTest(name=name), tempfile.TemporaryDirectory(
                    prefix="vaeg-m98d-") as temporary:
                bundle = self.make_bundle(Path(temporary))
                mutation(bundle)
                self.assert_bundle_error(bundle, expected_code)

    def test_cli_success_and_failure_are_path_redacted(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98d-") as temporary:
            bundle = self.make_bundle(Path(temporary))
            manifest_file = bundle / fixture.MANIFEST_NAME
            success = subprocess.run(
                [sys.executable, str(INSPECTOR), "--manifest", str(manifest_file)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(success.returncode, 0, success.stderr)
            self.assertEqual(success.stdout, "M98D_INPUT_PASS\n")
            self.assertEqual(success.stderr, "")

            value = load_manifest(bundle)
            image = value["image"]
            if not isinstance(image, dict):
                raise TypeError("synthetic image section differs")
            marker = "localidentitymustnotappear.bmp"
            image["path"] = marker
            write_manifest(bundle, value)
            failure = subprocess.run(
                [sys.executable, str(INSPECTOR), "--manifest", str(manifest_file)],
                check=False,
                capture_output=True,
                text=True,
            )
        self.assertNotEqual(failure.returncode, 0)
        self.assertEqual(failure.stdout, "")
        self.assertEqual(failure.stderr, "M98D_BMP_READ: input file could not be read\n")
        self.assertNotIn(marker, failure.stderr)

    def test_fixture_cli_refuses_existing_output(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98d-") as temporary:
            output = Path(temporary) / "input"
            first = subprocess.run(
                [sys.executable, str(BUILDER), "--output", str(output)],
                check=False,
                capture_output=True,
                text=True,
            )
            second = subprocess.run(
                [sys.executable, str(BUILDER), "--output", str(output)],
                check=False,
                capture_output=True,
                text=True,
            )
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(first.stdout, "M98D_FIXTURE_BUILD_PASS\n")
        self.assertNotEqual(second.returncode, 0)
        self.assertEqual(second.stderr,
                         "M98D_FIXTURE_OUTPUT_EXISTS: output directory exists\n")


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(ZundamonOrbitInputTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98D_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

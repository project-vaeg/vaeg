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

"""Test deterministic RGB888 to VA 8-bpp GGGRRRBB conversion."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import cast

import build_zundamon_orbit_asset as asset
import build_zundamon_orbit_input_fixture as fixture
import convert_zundamon_orbit_va8 as converter


TOOL_DIRECTORY = Path(__file__).resolve().parent
CONVERTER = TOOL_DIRECTORY / "convert_zundamon_orbit_va8.py"


def digest(input_file: Path) -> str:
    return hashlib.sha256(input_file.read_bytes()).hexdigest()


def oracle_decode(value: int) -> tuple[int, int, int]:
    return (
        (((value >> 2) & 7) * 255 + 3) // 7,
        (((value >> 5) & 7) * 255 + 3) // 7,
        ((value & 3) * 255 + 1) // 3,
    )


def oracle_convert(rgb: tuple[int, int, int]) -> int:
    red, green, blue = rgb
    value = (((green * 7 + 127) // 255) << 5
             | ((red * 7 + 127) // 255) << 2
             | ((blue * 3 + 127) // 255))
    if value != 0:
        return value
    return min(
        range(1, 256),
        key=lambda candidate: (
            sum((rgb[channel] - oracle_decode(candidate)[channel]) ** 2
                for channel in range(3)),
            candidate,
        ),
    )


class ZundamonOrbitVa8Tests(unittest.TestCase):
    def make_bundle(self, root: Path) -> Path:
        bundle = root / "input"
        fixture.write_fixture(bundle)
        return bundle

    def test_channel_rounding_and_bit_layout(self) -> None:
        cases = (
            ((0, 0, 0), 0x00),
            ((255, 0, 0), 0x1c),
            ((0, 255, 0), 0xe0),
            ((0, 0, 255), 0x03),
            ((255, 255, 255), 0xff),
            ((18, 18, 42), 0x00),
            ((19, 0, 0), 0x04),
            ((0, 19, 0), 0x20),
            ((0, 0, 43), 0x01),
        )
        for rgb, expected in cases:
            with self.subTest(rgb=rgb):
                self.assertEqual(converter.quantize_rgb(rgb), expected)

    def test_decode_and_opaque_zero_repair(self) -> None:
        self.assertEqual(converter.decode_va8(0x00), (0, 0, 0))
        self.assertEqual(converter.decode_va8(0x1c), (255, 0, 0))
        self.assertEqual(converter.decode_va8(0xe0), (0, 255, 0))
        self.assertEqual(converter.decode_va8(0x03), (0, 0, 255))
        self.assertEqual(converter.decode_va8(0xff), (255, 255, 255))
        value, repaired = converter.convert_opaque_rgb((1, 1, 1))
        self.assertEqual(value, 0x04)
        self.assertTrue(repaired)
        value, repaired = converter.convert_opaque_rgb((19, 0, 0))
        self.assertEqual(value, 0x04)
        self.assertFalse(repaired)

    def test_fixture_matches_independent_oracle(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98f-") as temporary:
            bundle = self.make_bundle(Path(temporary))
            result = converter.convert_bundle(bundle / fixture.MANIFEST_NAME)
            indices = asset.build_pixels()
            expected = bytes(
                0 if index == 0 else oracle_convert(asset.PALETTE_RGB[index])
                for index in indices
            )
            self.assertEqual(result.pixels, expected)
            for source_index, converted in zip(indices, result.pixels):
                self.assertEqual(converted == 0, source_index == 0)
            self.assertEqual(result.width, asset.WIDTH)
            self.assertEqual(result.height, asset.HEIGHT)
            self.assertGreater(
                cast(dict[str, int], result.report["metrics"])[
                    "opaque_zero_repairs"],
                0,
            )

    def test_collision_and_error_report(self) -> None:
        palette = list(asset.PALETTE_RGB)
        palette[1] = (255, 0, 0)
        palette[2] = (254, 0, 0)
        palette[3] = (1, 1, 1)
        result = converter.convert_indexed_pixels(
            bytes((0, 1, 2, 3)), tuple(palette), 4, 1)
        self.assertEqual(result.pixels, bytes((0, 0x1c, 0x1c, 0x04)))
        self.assertEqual(result.report["collisions"], [
            {"source_indices": [1, 2], "va8": 0x1c},
        ])
        metrics = cast(dict[str, int], result.report["metrics"])
        self.assertEqual(metrics["transparent_pixels"], 1)
        self.assertEqual(metrics["opaque_pixels"], 3)
        self.assertEqual(metrics["opaque_zero_repairs"], 1)
        self.assertGreater(metrics["weighted_squared_error"], 0)

    def test_outputs_are_reproducible_and_inputs_unchanged(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98f-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            inputs = tuple(bundle.iterdir())
            before = {input_file.name: digest(input_file) for input_file in inputs}
            first = root / "first"
            second = root / "second"
            converter.write_conversion(bundle / fixture.MANIFEST_NAME, first)
            converter.write_conversion(bundle / fixture.MANIFEST_NAME, second)
            for filename in (converter.PIXEL_NAME, converter.REPORT_NAME):
                self.assertEqual((first / filename).read_bytes(),
                                 (second / filename).read_bytes(), filename)
            report = json.loads((first / converter.REPORT_NAME).read_text(
                encoding="utf-8"))
            self.assertEqual(report["schema"], converter.REPORT_SCHEMA)
            after = {input_file.name: digest(input_file) for input_file in inputs}
            self.assertEqual(before, after)

    def test_focused_failures_reach_exact_codes(self) -> None:
        palette = asset.PALETTE_RGB
        valid_pixels = asset.build_pixels()
        converter.convert_indexed_pixels(
            valid_pixels, palette, asset.WIDTH, asset.HEIGHT)
        cases = (
            ("geometry", lambda: converter.convert_indexed_pixels(
                valid_pixels, palette, 0, asset.HEIGHT), "M98F_GEOMETRY"),
            ("pixel length", lambda: converter.convert_indexed_pixels(
                valid_pixels[:-1], palette, asset.WIDTH, asset.HEIGHT),
             "M98F_PIXEL_LENGTH"),
            ("palette length", lambda: converter.convert_indexed_pixels(
                valid_pixels, palette[:-1], asset.WIDTH, asset.HEIGHT),
             "M98F_PALETTE_LENGTH"),
            ("reserved index", lambda: converter.convert_indexed_pixels(
                bytes((15,)) + valid_pixels[1:], palette,
                asset.WIDTH, asset.HEIGHT), "M98F_INDEX_RANGE"),
            ("VA8 range", lambda: converter.decode_va8(256),
             "M98F_VA8_RANGE"),
            ("RGB range", lambda: converter.quantize_rgb((256, 0, 0)),
             "M98F_RGB_RANGE"),
        )
        converter.decode_va8(255)
        converter.quantize_rgb((255, 0, 0))
        for name, operation, expected_code in cases:
            with self.subTest(name=name), self.assertRaises(
                    converter.ConversionError) as caught:
                operation()
            self.assertEqual(caught.exception.code, expected_code)

        with tempfile.TemporaryDirectory(prefix="vaeg-m98f-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            output = root / "output"
            converter.write_conversion(bundle / fixture.MANIFEST_NAME, output)
            with self.assertRaises(converter.ConversionError) as caught:
                converter.write_conversion(bundle / fixture.MANIFEST_NAME, output)
            self.assertEqual(caught.exception.code, "M98F_OUTPUT_EXISTS")

    def test_cli_success_and_failure_are_path_redacted(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98f-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            success = subprocess.run(
                [sys.executable, str(CONVERTER), "--manifest",
                 str(bundle / fixture.MANIFEST_NAME), "--output",
                 str(root / "success")],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(success.returncode, 0, success.stderr)
            self.assertEqual(success.stdout, "M98F_CONVERSION_PASS\n")
            self.assertEqual(success.stderr, "")
            marker = "localidentitymustnotappear"
            failure = subprocess.run(
                [sys.executable, str(CONVERTER), "--manifest",
                 str(root / marker), "--output", str(root / "failure")],
                check=False,
                capture_output=True,
                text=True,
            )
        self.assertNotEqual(failure.returncode, 0)
        self.assertEqual(failure.stdout, "")
        self.assertEqual(failure.stderr,
                         "M98C_MANIFEST_READ: manifest could not be read\n")
        self.assertNotIn(marker, failure.stderr)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(
        ZundamonOrbitVa8Tests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98F_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

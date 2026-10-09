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

"""Test the deterministic M98g 30-level nearest-neighbor scale set."""

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
import convert_zundamon_orbit_va8 as va8_converter
import generate_zundamon_orbit_scales as scaler


TOOL_DIRECTORY = Path(__file__).resolve().parent
GENERATOR = TOOL_DIRECTORY / "generate_zundamon_orbit_scales.py"


def digest(input_file: Path) -> str:
    return hashlib.sha256(input_file.read_bytes()).hexdigest()


def oracle_dimension(source_size: int, level: int) -> int:
    numerator = level if level < 30 else 31
    return max(1, (source_size * numerator + 15) // 31)


def oracle_sample(coordinate: int, source_size: int, target_size: int) -> int:
    return min(
        source_size - 1,
        ((2 * coordinate + 1) * source_size) // (2 * target_size),
    )


def oracle_anchor(anchor: int, source_size: int, target_size: int) -> int:
    return min(
        target_size - 1,
        ((2 * anchor + 1) * target_size) // (2 * source_size),
    )


def oracle_payload(source: bytes, source_width: int, source_height: int,
                   target_width: int, target_height: int) -> tuple[int, bytes]:
    pitch = (target_width + 3) & ~3
    payload = bytearray()
    for y in range(target_height):
        source_y = oracle_sample(y, source_height, target_height)
        for x in range(target_width):
            source_x = oracle_sample(x, source_width, target_width)
            payload.append(source[source_y * source_width + source_x])
        payload.extend(b"\x00" * (pitch - target_width))
    return pitch, bytes(payload)


class ZundamonOrbitScaleTests(unittest.TestCase):
    def make_bundle(self, root: Path) -> Path:
        bundle = root / "input"
        fixture.write_fixture(bundle)
        return bundle

    def make_source(self) -> tuple[bytes, int, int, int, int]:
        pixels = bytes(
            0 if index == 0 else va8_converter.convert_opaque_rgb(
                asset.PALETTE_RGB[index])[0]
            for index in asset.build_pixels()
        )
        return pixels, asset.WIDTH, asset.HEIGHT, asset.WIDTH // 2, asset.HEIGHT // 2

    def test_exact_dimensions_and_level_30_source(self) -> None:
        source, width, height, anchor_x, anchor_y = self.make_source()
        result = scaler.build_scale_set(
            source, width, height, anchor_x, anchor_y)
        self.assertEqual(len(result.frames), 30)
        self.assertEqual([frame.level for frame in result.frames],
                         list(range(1, 31)))
        dimensions = [(frame.width, frame.height) for frame in result.frames]
        self.assertEqual(dimensions, [
            (oracle_dimension(width, level), oracle_dimension(height, level))
            for level in range(1, 31)
        ])
        self.assertEqual(dimensions, sorted(dimensions))
        self.assertLess(len(set(dimensions)), 30)
        full = result.frames[-1]
        self.assertEqual((full.width, full.height), (width, height))
        for y in range(height):
            row = full.payload[y * full.pitch:(y + 1) * full.pitch]
            self.assertEqual(row[:width], source[y * width:(y + 1) * width])
            self.assertEqual(row[width:], b"\x00" * (full.pitch - width))

    def test_center_sampling_and_anchor_convention(self) -> None:
        source = bytes(range(12))
        pitch, payload = scaler.scale_payload(source, 4, 3, 2, 2)
        self.assertEqual(pitch, 4)
        self.assertEqual(payload, bytes((1, 3, 0, 0, 9, 11, 0, 0)))
        self.assertEqual(
            [scaler.source_sample_coordinate(x, 4, 3) for x in range(3)],
            [0, 2, 3],
        )
        self.assertEqual(scaler.scale_anchor_coordinate(2, 4, 2), 1)
        self.assertEqual(scaler.scale_anchor_coordinate(2, 4, 4), 2)

    def test_every_frame_matches_independent_oracle(self) -> None:
        source, width, height, anchor_x, anchor_y = self.make_source()
        result = scaler.build_scale_set(
            source, width, height, anchor_x, anchor_y)
        source_values = set(source)
        for frame in result.frames:
            expected_pitch, expected_payload = oracle_payload(
                source, width, height, frame.width, frame.height)
            self.assertEqual(frame.pitch, expected_pitch, frame.level)
            self.assertEqual(frame.payload, expected_payload, frame.level)
            self.assertEqual(frame.anchor_x,
                             oracle_anchor(anchor_x, width, frame.width))
            self.assertEqual(frame.anchor_y,
                             oracle_anchor(anchor_y, height, frame.height))
            self.assertLess(frame.anchor_x, frame.width)
            self.assertLess(frame.anchor_y, frame.height)
            for y in range(frame.height):
                row = frame.payload[y * frame.pitch:(y + 1) * frame.pitch]
                self.assertTrue(set(row[:frame.width]).issubset(source_values))
                self.assertEqual(row[frame.width:],
                                 b"\x00" * (frame.pitch - frame.width))

    def test_frame_alignment_and_report_descriptors(self) -> None:
        source, width, height, anchor_x, anchor_y = self.make_source()
        result = scaler.build_scale_set(
            source, width, height, anchor_x, anchor_y)
        cursor = 0
        for frame in result.frames:
            self.assertEqual(frame.offset % 16, 0)
            self.assertEqual(result.stream[cursor:frame.offset],
                             b"\x00" * (frame.offset - cursor))
            end = frame.offset + len(frame.payload)
            self.assertEqual(result.stream[frame.offset:end], frame.payload)
            cursor = end
        self.assertEqual(cursor, len(result.stream))
        self.assertEqual(result.report["schema"], scaler.REPORT_SCHEMA)
        descriptors = cast(list[dict[str, int]], result.report["descriptors"])
        self.assertEqual(len(descriptors), 30)
        for frame, descriptor in zip(result.frames, descriptors):
            self.assertEqual(descriptor["level"], frame.level)
            self.assertEqual(descriptor["offset"], frame.offset)
            self.assertEqual(descriptor["payload_bytes"], len(frame.payload))

    def test_outputs_are_reproducible_and_inputs_unchanged(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98g-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            inputs = tuple(bundle.iterdir())
            before = {input_file.name: digest(input_file) for input_file in inputs}
            first = root / "first"
            second = root / "second"
            scaler.write_scale_set(bundle / fixture.MANIFEST_NAME, first)
            scaler.write_scale_set(bundle / fixture.MANIFEST_NAME, second)
            for filename in (scaler.STREAM_NAME, scaler.REPORT_NAME):
                self.assertEqual((first / filename).read_bytes(),
                                 (second / filename).read_bytes(), filename)
            report = json.loads((first / scaler.REPORT_NAME).read_text(
                encoding="utf-8"))
            self.assertEqual(report["format"]["scale_count"], 30)
            self.assertEqual(len(report["descriptors"]), 30)
            after = {input_file.name: digest(input_file) for input_file in inputs}
            self.assertEqual(before, after)

    def test_focused_failures_reach_exact_codes(self) -> None:
        source, width, height, anchor_x, anchor_y = self.make_source()
        scaler.build_scale_set(source, width, height, anchor_x, anchor_y)
        scaler.scale_dimension(width, 1)
        scaler.source_sample_coordinate(0, width, 1)
        cases = (
            ("source geometry", lambda: scaler.build_scale_set(
                source, 0, height, anchor_x, anchor_y),
             "M98G_SOURCE_GEOMETRY"),
            ("source length", lambda: scaler.build_scale_set(
                source[:-1], width, height, anchor_x, anchor_y),
             "M98G_SOURCE_LENGTH"),
            ("anchor bounds", lambda: scaler.build_scale_set(
                source, width, height, width, anchor_y),
             "M98G_ANCHOR_BOUNDS"),
            ("level range", lambda: scaler.scale_dimension(width, 0),
             "M98G_LEVEL_RANGE"),
            ("target geometry", lambda: scaler.scale_payload(
                source, width, height, 0, 1), "M98G_TARGET_GEOMETRY"),
            ("target coordinate", lambda: scaler.source_sample_coordinate(
                1, width, 1), "M98G_TARGET_COORDINATE"),
        )
        for name, operation, expected_code in cases:
            with self.subTest(name=name), self.assertRaises(
                    scaler.ScaleError) as caught:
                operation()
            self.assertEqual(caught.exception.code, expected_code)

        with tempfile.TemporaryDirectory(prefix="vaeg-m98g-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            output = root / "output"
            scaler.write_scale_set(bundle / fixture.MANIFEST_NAME, output)
            with self.assertRaises(scaler.ScaleError) as caught:
                scaler.write_scale_set(bundle / fixture.MANIFEST_NAME, output)
            self.assertEqual(caught.exception.code, "M98G_OUTPUT_EXISTS")

    def test_cli_success_and_failure_are_path_redacted(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98g-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            success = subprocess.run(
                [sys.executable, str(GENERATOR), "--manifest",
                 str(bundle / fixture.MANIFEST_NAME), "--output",
                 str(root / "success")],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(success.returncode, 0, success.stderr)
            self.assertEqual(success.stdout, "M98G_SCALE_SET_PASS\n")
            self.assertEqual(success.stderr, "")
            marker = "localidentitymustnotappear"
            failure = subprocess.run(
                [sys.executable, str(GENERATOR), "--manifest",
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
        ZundamonOrbitScaleTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98G_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

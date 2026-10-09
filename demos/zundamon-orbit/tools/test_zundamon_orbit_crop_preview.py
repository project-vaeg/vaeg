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

"""Test deterministic M98e crop and anchor preview generation."""

from __future__ import annotations

import hashlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import build_zundamon_orbit_asset as asset
import build_zundamon_orbit_crop_preview as preview
import build_zundamon_orbit_input_fixture as fixture
import inspect_zundamon_orbit_input as input_inspector


TOOL_DIRECTORY = Path(__file__).resolve().parent
PREVIEW_TOOL = TOOL_DIRECTORY / "build_zundamon_orbit_crop_preview.py"


def digest(input_file: Path) -> str:
    return hashlib.sha256(input_file.read_bytes()).hexdigest()


class ZundamonOrbitCropPreviewTests(unittest.TestCase):
    def make_bundle(self, root: Path) -> Path:
        bundle = root / "input"
        fixture.write_fixture(bundle)
        return bundle

    def test_preview_geometry_pixels_and_markers(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98e-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            output = root / "preview"
            preview.build_previews(bundle / fixture.MANIFEST_NAME, output, scale=2)
            source = input_inspector.parse_bmp32(
                (output / preview.SOURCE_OVERLAY_NAME).read_bytes())
            crop = input_inspector.parse_bmp32((output / preview.CROP_NAME).read_bytes())
            anchor = input_inspector.parse_bmp32(
                (output / preview.ANCHOR_OVERLAY_NAME).read_bytes())
            self.assertEqual((source.width, source.height), (asset.WIDTH, asset.HEIGHT))
            self.assertEqual((crop.width, crop.height), (asset.WIDTH * 2, asset.HEIGHT * 2))
            self.assertEqual((anchor.width, anchor.height),
                             (asset.WIDTH * 2, asset.HEIGHT * 2))
            self.assertEqual(source.rgb_at(0, 0), preview.CROP_COLOR)
            self.assertEqual(source.rgb_at(asset.WIDTH // 2, asset.HEIGHT // 2),
                             preview.ANCHOR_COLOR)
            self.assertEqual(anchor.rgb_at(asset.WIDTH, asset.HEIGHT),
                             preview.ANCHOR_COLOR)
            marker_x = (asset.WIDTH // 2) * 2 + 1
            marker_y = (asset.HEIGHT // 2) * 2 + 1
            for y in range(anchor.height):
                for x in range(anchor.width):
                    is_marker = (
                        (x == marker_x and abs(y - marker_y) <= 4)
                        or (y == marker_y and abs(x - marker_x) <= 4)
                    )
                    expected_color = (preview.ANCHOR_COLOR if is_marker
                                      else crop.rgb_at(x, y))
                    self.assertEqual(anchor.rgb_at(x, y), expected_color)
            palette = asset.PALETTE_RGB
            expected = asset.build_pixels()
            for y in range(asset.HEIGHT):
                for x in range(asset.WIDTH):
                    self.assertEqual(crop.rgb_at(x * 2, y * 2),
                                     palette[expected[y * asset.WIDTH + x]])

    def test_previews_are_reproducible_and_source_is_unchanged(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98e-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            source_files = tuple(bundle.iterdir())
            before = {input_file.name: digest(input_file) for input_file in source_files}
            first = root / "first"
            second = root / "second"
            preview.build_previews(bundle / fixture.MANIFEST_NAME, first)
            preview.build_previews(bundle / fixture.MANIFEST_NAME, second)
            for filename in (preview.SOURCE_OVERLAY_NAME, preview.CROP_NAME,
                             preview.ANCHOR_OVERLAY_NAME):
                self.assertEqual((first / filename).read_bytes(),
                                 (second / filename).read_bytes(), filename)
            after = {input_file.name: digest(input_file) for input_file in source_files}
            self.assertEqual(before, after)

    def test_invalid_scale_and_existing_output_reach_exact_codes(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98e-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            manifest = bundle / fixture.MANIFEST_NAME
            with self.assertRaises(preview.PreviewError) as scale_error:
                preview.build_previews(manifest, root / "bad-scale", scale=0)
            self.assertEqual(scale_error.exception.code, "M98E_PREVIEW_SCALE")
            output = root / "preview"
            preview.build_previews(manifest, output)
            with self.assertRaises(preview.PreviewError) as output_error:
                preview.build_previews(manifest, output)
            self.assertEqual(output_error.exception.code,
                             "M98E_PREVIEW_OUTPUT_EXISTS")

    def test_cli_success_and_failure_are_path_redacted(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98e-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            manifest = bundle / fixture.MANIFEST_NAME
            success = subprocess.run(
                [sys.executable, str(PREVIEW_TOOL), "--manifest", str(manifest),
                 "--output", str(root / "success")],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(success.returncode, 0, success.stderr)
            self.assertEqual(success.stdout, "M98E_PREVIEW_PASS\n")
            self.assertEqual(success.stderr, "")
            marker = "localidentitymustnotappear"
            failure = subprocess.run(
                [sys.executable, str(PREVIEW_TOOL), "--manifest",
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
        ZundamonOrbitCropPreviewTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98E_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

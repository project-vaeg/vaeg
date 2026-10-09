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

"""Test the complete public M98j host-asset pipeline."""

from __future__ import annotations

import dataclasses
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import cast

import build_zundamon_orbit_atlas_fixture as format_fixture
import build_zundamon_orbit_input_fixture as fixture
import build_zundamon_orbit_pipeline as pipeline
import convert_zundamon_orbit_va8 as va8_converter
import generate_zundamon_orbit_scales as scaler
import inspect_zundamon_orbit_atlas as format_inspector
import inspect_zundamon_orbit_input as input_inspector
import pack_zundamon_orbit_atlas as packer
import validate_zundamon_orbit_manifest as manifest_validator


TOOL_DIRECTORY = Path(__file__).resolve().parent
PIPELINE = TOOL_DIRECTORY / "build_zundamon_orbit_pipeline.py"


class ZundamonOrbitPipelineTests(unittest.TestCase):
    def make_bundle(self, root: Path) -> Path:
        bundle = root / "bundle"
        fixture.write_fixture(bundle)
        return bundle

    def assert_pipeline_error(self, operation, expected_code: str) -> None:
        with self.assertRaises(pipeline.PipelineError) as caught:
            operation()
        self.assertEqual(caught.exception.code, expected_code)

    def independent_scale_set(self, manifest_file: Path) -> scaler.ScaleSet:
        conversion = va8_converter.convert_bundle(manifest_file)
        manifest_value = manifest_validator.read_manifest(manifest_file)
        manifest_validator.validate_manifest(manifest_value)
        manifest = cast(dict[str, object], manifest_value)
        anchor = cast(dict[str, object], manifest["anchor"])
        return scaler.build_scale_set(
            conversion.pixels,
            conversion.width,
            conversion.height,
            cast(int, anchor["x"]),
            cast(int, anchor["y"]),
        )

    def test_pipeline_matches_independently_composed_atlas(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98j-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            manifest_file = bundle / fixture.MANIFEST_NAME
            before = {path.name: path.read_bytes() for path in bundle.iterdir()}
            result = pipeline.build_pipeline(manifest_file)
            independent_scales = self.independent_scale_set(manifest_file)
            independent_atlas = packer.build_atlas(independent_scales)
            self.assertEqual(result.atlas, independent_atlas.contents)
            format_inspector.inspect_bytes(result.atlas)
            packer.inspect_packed_bytes(result.atlas)
            after = {path.name: path.read_bytes() for path in bundle.iterdir()}
            self.assertEqual(before, after)

    def test_pipeline_and_contact_sheet_are_byte_reproducible(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98j-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            manifest_file = bundle / fixture.MANIFEST_NAME
            first = pipeline.build_pipeline(manifest_file)
            second = pipeline.build_pipeline(manifest_file)
            self.assertEqual(first.atlas, second.atlas)
            self.assertEqual(first.contact_sheet, second.contact_sheet)
            self.assertEqual(
                pipeline.report_bytes(first.report),
                pipeline.report_bytes(second.report),
            )

    def test_normalization_is_downscale_only_for_replicated_source(self) -> None:
        width = pipeline.MAX_ATLAS_SOURCE_WIDTH
        height = pipeline.MAX_ATLAS_SOURCE_HEIGHT
        replication = 3
        source = bytes(
            ((x * 17 + y * 29) % 255) + 1
            for y in range(height)
            for x in range(width)
        )
        expanded = bytearray()
        for y in range(height):
            row = source[y * width:(y + 1) * width]
            expanded_row = bytes(
                value for value in row for _ in range(replication))
            for _ in range(replication):
                expanded.extend(expanded_row)
        anchor_x = 43
        anchor_y = 61
        normalized = pipeline.normalize_source(
            bytes(expanded), width * replication, height * replication,
            anchor_x * replication + 1, anchor_y * replication + 1)
        self.assertEqual((normalized.width, normalized.height), (width, height))
        self.assertEqual((normalized.anchor_x, normalized.anchor_y),
                         (anchor_x, anchor_y))
        self.assertEqual(normalized.pixels, source)
        self.assertEqual(normalized.report["downscaled"], True)
        self.assertEqual(normalized.report["upscaling"], False)
        self.assertLessEqual(
            normalized.report["one_bank_occupied_bytes"], packer.BANK_SIZE)

        small = pipeline.normalize_source(bytes(range(12)), 4, 3, 2, 1)
        self.assertEqual((small.width, small.height), (4, 3))
        self.assertEqual((small.anchor_x, small.anchor_y), (2, 1))
        self.assertEqual(small.pixels, bytes(range(12)))
        self.assertEqual(small.report["downscaled"], False)
        self.assertEqual(pipeline.normalized_dimensions(98, 128), (98, 128))
        self.assertEqual(
            pipeline.scale_atlas_occupied_bytes(98, 128), 127456)
        self.assertLess(127456, packer.BANK_SIZE)
        self.assertEqual(pipeline.normalized_dimensions(196, 128), (98, 64))
        self.assertEqual(pipeline.normalized_dimensions(98, 256), (49, 128))

    def test_contact_sheet_geometry_labels_and_anchors(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98j-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            manifest_file = bundle / fixture.MANIFEST_NAME
            result = pipeline.build_pipeline(manifest_file)
            image = input_inspector.parse_bmp32(result.contact_sheet)
            self.assertEqual(
                (image.width, image.height),
                (pipeline.SHEET_WIDTH, pipeline.SHEET_HEIGHT),
            )
            contact = cast(dict[str, object], result.report["contact_sheet"])
            cells = cast(list[dict[str, int]], contact["cells"])
            self.assertEqual(len(cells), 30)
            self.assertEqual([cell["level"] for cell in cells], list(range(1, 31)))
            for cell in cells:
                self.assertEqual(
                    image.rgb_at(cell["marker_x"], cell["marker_y"]),
                    pipeline.ANCHOR_COLOR,
                    cell["level"],
                )
                self.assertTrue(
                    cell["preview_x"] <= cell["marker_x"]
                    < cell["preview_x"] + cell["preview_width"])
                self.assertTrue(
                    cell["preview_y"] <= cell["marker_y"]
                    < cell["preview_y"] + cell["preview_height"])
                self.assertEqual(
                    image.rgb_at(cell["cell_x"] + pipeline.PREVIEW_MARGIN_X,
                                 cell["cell_y"] + 6),
                    pipeline.LABEL_COLOR,
                )
            colors = {
                image.rgb_at(x, y)
                for y in range(image.height)
                for x in range(image.width)
            }
            self.assertIn(pipeline.TRANSPARENT_DARK, colors)
            self.assertIn(pipeline.TRANSPARENT_LIGHT, colors)

    def test_combined_report_reconciles_without_input_identity(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98j-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            manifest_file = bundle / fixture.MANIFEST_NAME
            result = pipeline.build_pipeline(manifest_file)
            report = result.report
            self.assertEqual(report["schema"], pipeline.REPORT_SCHEMA)
            validation = cast(dict[str, object], report["validation"])
            self.assertEqual(validation["format"], "M98H_ATLAS_PASS")
            self.assertEqual(validation["packing"], "M98I_PACKING_PASS")
            self.assertEqual(validation["descriptor_count"], 30)
            self.assertEqual(validation["required_bank_count"], 1)
            packing = cast(dict[str, object], report["packing"])
            packing_metrics = cast(dict[str, object], packing["metrics"])
            self.assertEqual(validation["required_bank_count"],
                             packing_metrics["required_bank_count"])
            normalization = cast(dict[str, object], report["normalization"])
            self.assertEqual(normalization["one_bank_occupied_bytes"],
                             packing_metrics["bms_span_bytes"])
            conversion = cast(dict[str, object], report["conversion"])
            conversion_format = cast(dict[str, object], conversion["format"])
            source = cast(dict[str, object], report["input"])
            self.assertEqual(
                cast(int, source["opaque_pixels"])
                + cast(int, source["transparent_pixels"]),
                cast(int, conversion_format["width"])
                * cast(int, conversion_format["height"]),
            )
            encoded = pipeline.report_bytes(report).decode("utf-8")
            self.assertNotIn(str(root), encoded)
            self.assertNotIn(fixture.MANIFEST_NAME, encoded)
            self.assertNotIn("sha256", encoded.lower())
            self.assertNotIn("crc32", encoded.lower())

    def test_writers_emit_only_three_outputs_and_refuse_overwrite(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98j-") as temporary:
            root = Path(temporary)
            bundle = self.make_bundle(root)
            manifest_file = bundle / fixture.MANIFEST_NAME
            first = root / "first"
            second = root / "second"
            pipeline.write_pipeline(manifest_file, first)
            pipeline.write_pipeline(manifest_file, second)
            expected = {
                pipeline.ATLAS_NAME,
                pipeline.CONTACT_SHEET_NAME,
                pipeline.REPORT_NAME,
            }
            self.assertEqual({path.name for path in first.iterdir()}, expected)
            self.assertEqual({path.name for path in second.iterdir()}, expected)
            for filename in expected:
                self.assertEqual((first / filename).read_bytes(),
                                 (second / filename).read_bytes())
            self.assert_pipeline_error(
                lambda: pipeline.write_pipeline(manifest_file, first),
                "M98J_OUTPUT_EXISTS",
            )

    def test_focused_contact_and_argument_failures_reach_exact_codes(self) -> None:
        scale_set = format_fixture.public_scale_set()
        pipeline.build_contact_sheet(scale_set)
        missing = dataclasses.replace(scale_set, frames=scale_set.frames[:-1])
        cases = (
            (lambda: pipeline.build_contact_sheet(missing),
             "M98J_CONTACT_FRAME_COUNT"),
            (lambda: pipeline.preview_dimensions(0, 1),
             "M98J_CONTACT_GEOMETRY"),
            (lambda: pipeline.draw_text([], 0, 0, 0, 0, "?", (0, 0, 0)),
             "M98J_CONTACT_GLYPH"),
            (lambda: pipeline.draw_line([], 0, 0, 0, 0, 1, 1, (0, 0, 0)),
             "M98J_CONTACT_LINE"),
            (lambda: pipeline.normalized_dimensions(0, 1),
             "M98J_NORMALIZE_GEOMETRY"),
            (lambda: pipeline.normalize_source(b"\x00", 2, 1, 0, 0),
             "M98J_NORMALIZE_SOURCE"),
            (lambda: pipeline.normalize_source(b"\x00\x00", 2, 1, 2, 0),
             "M98J_NORMALIZE_ANCHOR"),
        )
        for operation, expected_code in cases:
            with self.subTest(expected_code=expected_code):
                self.assert_pipeline_error(operation, expected_code)

    def test_cli_public_local_and_failure_output_are_path_redacted(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98j-") as temporary:
            root = Path(temporary)
            public_output = root / "public"
            public = subprocess.run(
                [sys.executable, str(PIPELINE),
                 "--fixture-output", str(public_output)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(public.returncode, 0, public.stderr)
            self.assertEqual(public.stdout, "M98J_FIXTURE_PASS\n")
            self.assertEqual(public.stderr, "")
            format_inspector.inspect_file(public_output / pipeline.ATLAS_NAME)
            packer.inspect_packed_file(public_output / pipeline.ATLAS_NAME)

            bundle = self.make_bundle(root)
            local_output = root / "local"
            local = subprocess.run(
                [sys.executable, str(PIPELINE),
                 "--manifest", str(bundle / fixture.MANIFEST_NAME),
                 "--output", str(local_output)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(local.returncode, 0, local.stderr)
            self.assertEqual(local.stdout, "M98J_LOCAL_BUILD_READY\n")
            self.assertEqual(local.stderr, "")

            overwrite = subprocess.run(
                [sys.executable, str(PIPELINE),
                 "--fixture-output", str(public_output)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(overwrite.returncode, 0)
            self.assertEqual(
                overwrite.stderr,
                "M98J_OUTPUT_EXISTS: output directory exists\n",
            )
            marker = "localidentitymustnotappear"
            missing = subprocess.run(
                [sys.executable, str(PIPELINE),
                 "--manifest", str(root / marker),
                 "--output", str(root / "missing-output")],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(missing.returncode, 0)
            self.assertEqual(missing.stdout, "")
            self.assertEqual(
                missing.stderr,
                "M98C_MANIFEST_READ: manifest could not be read\n",
            )
            self.assertNotIn(marker, missing.stderr)
            arguments = subprocess.run(
                [sys.executable, str(PIPELINE), "--output", str(root / marker)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(arguments.returncode, 0)
            self.assertEqual(
                arguments.stderr,
                "M98J_ARGUMENTS: local mode requires manifest and output\n",
            )
            self.assertNotIn(marker, arguments.stderr)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(
        ZundamonOrbitPipelineTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98J_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

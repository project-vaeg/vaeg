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

"""Test the M98c local-input manifest contract and validator."""

from __future__ import annotations

import copy
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Callable

import validate_zundamon_orbit_manifest as validator


TOOL_DIRECTORY = Path(__file__).resolve().parent
DEMO_DIRECTORY = TOOL_DIRECTORY.parent
EXAMPLE = DEMO_DIRECTORY / "examples" / "input-manifest-v1.json"
SCHEMA = DEMO_DIRECTORY / "schema" / "input-manifest-v1.schema.json"
VALIDATOR = TOOL_DIRECTORY / "validate_zundamon_orbit_manifest.py"
Mutation = Callable[[dict[str, object]], None]


def load_json(input_file: Path) -> dict[str, object]:
    value = json.loads(input_file.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise TypeError("fixture root is not an object")
    return value


def nested(value: dict[str, object], name: str) -> dict[str, object]:
    section = value[name]
    if not isinstance(section, dict):
        raise TypeError(f"fixture section {name} is not an object")
    return section


class ZundamonOrbitManifestTests(unittest.TestCase):
    def setUp(self) -> None:
        self.example = load_json(EXAMPLE)

    def assert_manifest_error(self, value: object, expected_code: str) -> None:
        with self.assertRaises(validator.ManifestError) as caught:
            validator.validate_manifest(value)
        self.assertEqual(caught.exception.code, expected_code)

    def test_schema_and_validator_contract_agree(self) -> None:
        schema = load_json(SCHEMA)
        self.assertFalse(schema["additionalProperties"])
        self.assertEqual(frozenset(schema["required"]), validator.ROOT_KEYS)
        properties = nested(schema, "properties")
        self.assertEqual(nested(properties, "schema")["const"], validator.SCHEMA_ID)
        self.assertEqual(nested(properties, "schema_version")["const"],
                         validator.SCHEMA_VERSION)
        self.assertEqual(nested(properties, "copyright")["const"], validator.COPYRIGHT)
        self.assertEqual(nested(properties, "license")["const"], validator.LICENSE)
        image = nested(nested(properties, "image"), "properties")
        palette = nested(nested(properties, "palette"), "properties")
        crop = nested(nested(properties, "crop"), "properties")
        transparency = nested(nested(properties, "transparency"), "properties")
        anchor = nested(nested(properties, "anchor"), "properties")
        self.assertEqual(frozenset(nested(properties, "image")["required"]),
                         validator.IMAGE_KEYS)
        self.assertEqual(frozenset(nested(properties, "palette")["required"]),
                         validator.PALETTE_KEYS)
        self.assertEqual(frozenset(nested(properties, "crop")["required"]),
                         validator.CROP_KEYS)
        self.assertEqual(frozenset(nested(properties, "transparency")["required"]),
                         validator.TRANSPARENCY_KEYS)
        self.assertEqual(frozenset(nested(properties, "anchor")["required"]),
                         validator.ANCHOR_KEYS)
        self.assertEqual(image["encoding"], {"const": "bmp32"})
        self.assertEqual(palette["encoding"], {"const": "rgb888"})
        self.assertEqual(palette["entries"], {"const": 16})
        self.assertEqual(palette["transparent_index"], {"const": 0})
        self.assertEqual(palette["reserved_index"], {"const": 15})
        self.assertEqual(crop["width"]["maximum"], validator.MAX_CROP_DIMENSION)
        self.assertEqual(crop["height"]["maximum"], validator.MAX_CROP_DIMENSION)
        self.assertEqual(crop["x"]["maximum"], validator.MAX_SOURCE_COORDINATE)
        self.assertEqual(crop["y"]["maximum"], validator.MAX_SOURCE_COORDINATE)
        self.assertEqual(transparency["method"], {"const": "exact-rgb"})
        self.assertEqual(anchor["space"], {"const": "crop-top-left"})
        validator.validate_manifest(self.example)

    def test_one_mutation_failures_reach_exact_codes(self) -> None:
        cases: tuple[tuple[str, Mutation, str], ...] = (
            ("missing root member", lambda value: value.pop("license"),
             "M98C_ROOT_KEYS"),
            ("unknown root member", lambda value: value.__setitem__("notes", "x"),
             "M98C_ROOT_KEYS"),
            ("wrong schema", lambda value: value.__setitem__("schema", "other"),
             "M98C_SCHEMA"),
            ("boolean schema version",
             lambda value: value.__setitem__("schema_version", True),
             "M98C_SCHEMA_VERSION"),
            ("unknown image member",
             lambda value: nested(value, "image").__setitem__("source", "x"),
             "M98C_IMAGE_KEYS"),
            ("absolute image path",
             lambda value: nested(value, "image").__setitem__("path", "/tmp/source.bmp"),
             "M98C_IMAGE_PATH"),
            ("parent palette path",
             lambda value: nested(value, "palette").__setitem__("path", "../palette.rgb"),
             "M98C_PALETTE_PATH"),
            ("wrong image encoding",
             lambda value: nested(value, "image").__setitem__("encoding", "png"),
             "M98C_IMAGE_ENCODING"),
            ("wrong palette count",
             lambda value: nested(value, "palette").__setitem__("entries", 15),
             "M98C_PALETTE_ENTRIES"),
            ("wrong palette encoding",
             lambda value: nested(value, "palette").__setitem__("encoding", "rgb565"),
             "M98C_PALETTE_ENCODING"),
            ("wrong transparent index",
             lambda value: nested(value, "palette").__setitem__(
                 "transparent_index", 1),
             "M98C_TRANSPARENT_INDEX"),
            ("wrong reserved index",
             lambda value: nested(value, "palette").__setitem__("reserved_index", 14),
             "M98C_RESERVED_INDEX"),
            ("boolean crop coordinate",
             lambda value: nested(value, "crop").__setitem__("x", False),
             "M98C_CROP_X"),
            ("zero crop width",
             lambda value: nested(value, "crop").__setitem__("width", 0),
             "M98C_CROP_WIDTH"),
            ("crop coordinate overflow",
             lambda value: nested(value, "crop").__setitem__("x", 65535),
             "M98C_CROP_RANGE"),
            ("short background",
             lambda value: nested(value, "transparency").__setitem__(
                 "background_rgb", [0, 0]),
             "M98C_BACKGROUND_RGB"),
            ("wrong transparency method",
             lambda value: nested(value, "transparency").__setitem__(
                 "method", "threshold"),
             "M98C_TRANSPARENCY_METHOD"),
            ("boolean background channel",
             lambda value: nested(value, "transparency").__setitem__(
                 "background_rgb", [0, False, 0]),
             "M98C_BACKGROUND_CHANNEL"),
            ("wrong anchor space",
             lambda value: nested(value, "anchor").__setitem__("space", "source"),
             "M98C_ANCHOR_SPACE"),
            ("anchor outside crop",
             lambda value: nested(value, "anchor").__setitem__("x", 23),
             "M98C_ANCHOR_BOUNDS"),
        )
        for name, mutation, expected_code in cases:
            with self.subTest(name=name):
                value = copy.deepcopy(self.example)
                mutation(value)
                self.assert_manifest_error(value, expected_code)

    def test_non_object_root_reaches_exact_code(self) -> None:
        self.assert_manifest_error([], "M98C_ROOT_TYPE")

    def test_duplicate_member_reaches_parser_code(self) -> None:
        contents = EXAMPLE.read_bytes().replace(
            b"{\n", b'{\n  "schema": "duplicate",\n', 1)
        with self.assertRaises(validator.ManifestError) as caught:
            validator.parse_manifest_bytes(contents)
        self.assertEqual(caught.exception.code, "M98C_JSON_DUPLICATE_KEY")

    def test_encoding_and_size_failures_reach_exact_codes(self) -> None:
        cases = (
            (b"\xef\xbb\xbf{}", "M98C_MANIFEST_BOM"),
            (b"\xff", "M98C_MANIFEST_UTF8"),
            (b"{", "M98C_MANIFEST_JSON"),
            (b" " * (validator.MAX_MANIFEST_BYTES + 1), "M98C_MANIFEST_SIZE"),
        )
        for contents, expected_code in cases:
            with self.subTest(code=expected_code):
                with self.assertRaises(validator.ManifestError) as caught:
                    validator.parse_manifest_bytes(contents)
                self.assertEqual(caught.exception.code, expected_code)

    def test_cli_accepts_example_without_reading_referenced_files(self) -> None:
        result = subprocess.run(
            [sys.executable, str(VALIDATOR), "--input", str(EXAMPLE)],
            check=False,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "M98C_MANIFEST_PASS\n")
        self.assertEqual(result.stderr, "")

    def test_cli_read_failure_does_not_echo_input_path(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98c-") as temporary:
            marker = "local-input-identity-must-not-appear"
            missing = Path(temporary) / marker
            result = subprocess.run(
                [sys.executable, str(VALIDATOR), "--input", str(missing)],
                check=False,
                capture_output=True,
                text=True,
            )
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "")
        self.assertEqual(result.stderr,
                         "M98C_MANIFEST_READ: manifest could not be read\n")
        self.assertNotIn(marker, result.stderr)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(ZundamonOrbitManifestTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98C_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

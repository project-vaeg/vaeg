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

"""Test deterministic generation and fail-closed inspection of the M98b fixture."""

from __future__ import annotations

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


TOOL_DIRECTORY = Path(__file__).resolve().parent
BUILDER = TOOL_DIRECTORY / "build_zundamon_orbit_asset.py"
INSPECTOR = TOOL_DIRECTORY / "inspect_zundamon_orbit_asset.py"
FIXTURE_FILES = (
    "zundamon-orbit-fixture-indexed.bin",
    "zundamon-orbit-fixture-palette.bin",
    "zundamon-orbit-fixture.json",
)


def run_tool(path: Path, *arguments: str | Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(path), *[str(argument) for argument in arguments]],
        check=False,
        capture_output=True,
        text=True,
    )


class ZundamonOrbitFixtureTests(unittest.TestCase):
    def build(self, output: Path) -> subprocess.CompletedProcess[str]:
        return run_tool(BUILDER, "--output", output)

    def inspect(self, output: Path) -> subprocess.CompletedProcess[str]:
        return run_tool(INSPECTOR, "--input", output)

    def test_fixture_is_byte_reproducible(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98b-") as temporary:
            root = Path(temporary)
            first = root / "first"
            second = root / "second"
            self.assertEqual(self.build(first).returncode, 0)
            self.assertEqual(self.build(second).returncode, 0)
            self.assertEqual(self.inspect(first).returncode, 0)
            self.assertEqual(self.inspect(second).returncode, 0)
            self.assertEqual(tuple(sorted(path.name for path in first.iterdir())),
                             tuple(sorted(FIXTURE_FILES)))
            for filename in FIXTURE_FILES:
                self.assertEqual((first / filename).read_bytes(),
                                 (second / filename).read_bytes(), filename)

    def test_changed_indexed_byte_fails_with_digest_code(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98b-") as temporary:
            output = Path(temporary) / "fixture"
            self.assertEqual(self.build(output).returncode, 0)
            self.assertEqual(self.inspect(output).returncode, 0)
            pixels = output / "zundamon-orbit-fixture-indexed.bin"
            contents = bytearray(pixels.read_bytes())
            contents[0] ^= 1
            pixels.write_bytes(contents)
            result = self.inspect(output)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("M98B_FIXTURE_PIXELS_SHA", result.stderr)

    def test_existing_output_is_not_overwritten(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98b-") as temporary:
            output = Path(temporary) / "fixture"
            self.assertEqual(self.build(output).returncode, 0)
            result = self.build(output)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("M98B_FIXTURE_OUTPUT_EXISTS", result.stderr)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(ZundamonOrbitFixtureTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98B_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

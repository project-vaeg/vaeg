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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Public/synthetic checks for the generic M98y profile path."""

from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parent
ROOT = TOOLS.parents[2]
sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import generate_zundamon_orbit_depth_table as depth  # noqa: E402
import generate_zundamon_orbit_hud as hud  # noqa: E402
import validate_zundamon_orbit_hud as hud_validator  # noqa: E402
import verify_m98y_private_profile as oracle  # noqa: E402


class M98yProfileTests(unittest.TestCase):
    def test_ida_hud_subject_is_fixed_width_and_distinct(self) -> None:
        public, _, _ = hud.encode_include()
        private, full, _ = hud.encode_include("IDA")
        self.assertNotEqual(public, private)
        self.assertEqual(len(full), 8)
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "hud.inc"
            path.write_bytes(private)
            hud_validator.inspect(path, subject="IDA")

    def test_private_oracle_runs_against_synthetic_public_atlas(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            pipeline.write_public_fixture(root / "fixture")
            atlas = root / "fixture" / pipeline.ATLAS_NAME
            table = root / "depth.inc"
            private_table = depth.encode_include(
                depth.generate_entries(96, 16), 96, 16, 1)
            table.write_bytes(private_table)
            private_hud = root / "hud.inc"
            private_hud.write_bytes(hud.encode_include("IDA")[0])
            summary = oracle.run(atlas, table, private_hud)
            self.assertEqual(summary["private_transition_cases"], 32768)
            self.assertEqual(summary["private_mismatches"], 0)

    def test_public_profile_guest_identity_is_deterministic(self) -> None:
        build = ROOT / "demos/zundamon-orbit/256/build.sh"
        with tempfile.TemporaryDirectory() as temporary:
            first = Path(temporary) / "one.com"
            second = Path(temporary) / "two.com"
            env = dict(__import__("os").environ, M98X_RUNTIME_MODE="1")
            for output in (first, second):
                result = subprocess.run((str(build), str(output),
                                         str(output.with_suffix(".lst"))),
                                        cwd=ROOT, env=env,
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(hashlib.sha256(first.read_bytes()).hexdigest(),
                             "e0f2111e4da5d0723633f6ac11658fad49f825c2bc6e6346578ac72ff67aa93f")
            self.assertEqual(first.read_bytes(), second.read_bytes())


if __name__ == "__main__":
    unittest.main()

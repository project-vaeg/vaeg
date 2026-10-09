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

"""Independent M98x parser, state, and release-source checks."""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

import build_zundamon_orbit_pipeline as pipeline
import generate_zundamon_multi_instance_state as multi
import validate_zundamon_orbit_hud as hud
import zundamon_runtime_count as runtime

TOOLS = Path(__file__).resolve().parent
ROOT = TOOLS.parents[2]
GUEST = TOOLS.parent / "256" / "zundamon_orbit_256.asm"
BUILD = TOOLS.parent / "256" / "build.sh"
LOCAL_D88_BUILD = TOOLS.parent / "build-local-d88.sh"
HUD_TABLE = TOOLS.parent / "256" / "zundamon_hud_table.inc"
DEPTH_TABLE = TOOLS.parent / "256" / "zundamon_depth_table.inc"


class RuntimeCountParserTests(unittest.TestCase):
    def test_default_and_all_valid_count_values(self) -> None:
        self.assertEqual(runtime.parse_options(""), (4, 1))
        for count in range(1, 17):
            self.assertEqual(runtime.parse_options(f"/N{count}"), (count, 1))
            self.assertEqual(runtime.parse_options(f"/V8 /n{count}"), (count, 8))
            self.assertEqual(runtime.parse_options(f"/n{count} /v1"), (count, 1))

    def test_invalid_and_duplicate_options_have_stable_codes(self) -> None:
        invalid_n = ("/N", "/N0", "/N17", "/N01", "/N001", "/N+1",
                     "/N-1", "/N1x", "/N1/anything", "/N=1", "/N 1")
        for option in invalid_n:
            with self.subTest(option=option):
                with self.assertRaises(runtime.CountOptionError) as raised:
                    runtime.parse_options(option)
                self.assertEqual(raised.exception.code, "M98X_INVALID_N")
        for option in ("/N1 /N1", "/N1 /n2"):
            with self.assertRaises(runtime.CountOptionError) as raised:
                runtime.parse_options(option)
            self.assertEqual(raised.exception.code, "M98X_DUPLICATE_N")
        with self.assertRaises(runtime.CountOptionError) as raised:
            runtime.parse_options("/V1 /V2")
        self.assertEqual(raised.exception.code, "M98X_DUPLICATE_V")


class RuntimeCountStateTests(unittest.TestCase):
    def test_requested_pending_visible_are_distinct(self) -> None:
        state = runtime.RuntimeCountState()
        state.begin_frame()
        self.assertTrue(state.press(1))
        self.assertEqual(state.pending_render_count, 4)
        self.assertEqual(state.requested_count, 5)
        state.publish()
        self.assertEqual(state.visible_published_count, 4)
        state.begin_frame()
        self.assertEqual(state.pending_render_count, 5)
        state.publish()
        self.assertEqual(state.visible_published_count, 5)
        self.assertEqual(state.global_phase_next, 2)

    def test_saturation_never_wraps(self) -> None:
        state = runtime.RuntimeCountState(requested_count=16,
                                          next_render_count=16,
                                          pending_render_count=16,
                                          visible_published_count=16)
        self.assertFalse(state.press(1))
        self.assertEqual(state.requested_count, 16)
        state.requested_count = 1
        self.assertFalse(state.press(-1))
        self.assertEqual(state.requested_count, 1)
        self.assertEqual(state.count_noop_saturations, 2)


class RuntimeCountGeometryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.tmp = tempfile.TemporaryDirectory()
        fixture = Path(cls.tmp.name) / "fixture"
        pipeline.write_public_fixture(fixture)
        cls.header, cls.entries, cls.descriptors = multi.load_inputs(
            DEPTH_TABLE, fixture / pipeline.ATLAS_NAME)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.tmp.cleanup()

    def make_state(self, count: int, phase: int):
        return multi.build_state(count, phase, self.header, self.entries,
                                 self.descriptors)

    def test_all_1024_count_phase_states(self) -> None:
        combinations, records = runtime.validate_state_matrix(self.make_state)
        self.assertEqual(combinations, 1024)
        self.assertEqual(records, 8704)

    def test_all_page_count_transition_cases(self) -> None:
        self.assertEqual(runtime.validate_transition_matrix(self.make_state),
                         32768)

    def test_transition_serialization_is_deterministic(self) -> None:
        first = runtime.transition_matrix_digest(self.make_state)
        second = runtime.transition_matrix_digest(self.make_state)
        self.assertEqual(first, second)
        serialized = runtime.serialize_transition_matrix(self.make_state)
        self.assertEqual(hashlib.sha256(serialized).hexdigest(), first)
        self.assertNotIn(b"/Users/", serialized)

    def test_count_one_matches_m98t_records(self) -> None:
        for phase in range(64):
            state = self.make_state(1, phase)
            record = state.records[0]
            entry = self.entries[phase]
            self.assertEqual(record.phase_id, entry.phase)
            self.assertEqual(record.depth_rank, entry.depth_rank)
            self.assertEqual(record.scale_id, entry.scale_id)
            descriptor = self.descriptors[entry.scale_id - 1]
            self.assertEqual(record.dst_x,
                             160 + entry.dx - descriptor.anchor_x)
            self.assertEqual(record.dst_y,
                             100 + entry.dy - descriptor.anchor_y)


class RuntimeCountBuildTests(unittest.TestCase):
    def test_hud_has_all_sixteen_count_tiles(self) -> None:
        self.assertEqual(len(hud.inspect_count_tiles(HUD_TABLE)), 16)

    def test_runtime_guest_is_single_binary_and_bounded(self) -> None:
        source = GUEST.read_text(encoding="utf-8")
        self.assertIn("M98X_RUNTIME_MODE", source)
        self.assertIn("parse_cadence_option", source)
        self.assertIn("KEY_SCAN_UP", source)
        self.assertIn("KEY_SCAN_DOWN", source)
        self.assertIn("KEY_SCAN_UP_EXTENDED", source)
        self.assertIn("KEY_SCAN_DOWN_EXTENDED", source)
        self.assertIn("%define KEY_SCAN_UP             0x3a", source)
        self.assertIn("%define KEY_SCAN_DOWN           0x3d", source)
        self.assertIn("update_hud_count_field", source)
        self.assertNotIn("M98V_ACTIVE_COUNT]", source)

    def test_runtime_build_is_deterministic(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            hashes = []
            for index in (1, 2):
                output = root / f"ZUNDORB-{index}.COM"
                listing = root / f"ZUNDORB-{index}.LST"
                env = os.environ.copy()
                env["M98X_RUNTIME_MODE"] = "1"
                result = subprocess.run((str(BUILD), str(output), str(listing)),
                                        cwd=ROOT, env=env,
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertLess(output.stat().st_size, 65280)
                hashes.append(hashlib.sha256(output.read_bytes()).hexdigest())
            self.assertEqual(hashes[0], hashes[1])

    def test_local_d88_build_forwards_runtime_mode(self) -> None:
        source = LOCAL_D88_BUILD.read_text(encoding="utf-8")
        self.assertIn("M98X_RUNTIME_MODE=${M98X_RUNTIME_MODE:-0} NASM=", source)
        self.assertIn("zero default preserves the fixed-count", source)


if __name__ == "__main__":
    unittest.main()

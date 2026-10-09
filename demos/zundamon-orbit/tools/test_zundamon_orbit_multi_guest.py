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

"""Host compositor, guest contract, and fail-closed M98v tests."""

from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from dataclasses import dataclass, replace
from pathlib import Path

import build_zundamon_orbit_pipeline as pipeline
import generate_zundamon_multi_instance_state as multi
import generate_zundamon_orbit_multi_debug as debug
import validate_zundamon_orbit_hud as hud
import verify_zundamon_orbit_depth_guest as depth_oracle
import verify_zundamon_orbit_multi_guest as oracle

TOOLS = Path(__file__).resolve().parent
ROOT = TOOLS.parents[2]
DEPTH_TABLE = TOOLS.parent / "256" / "zundamon_depth_table.inc"
HUD_TABLE = TOOLS.parent / "256" / "zundamon_hud_table.inc"
GUEST = TOOLS.parent / "256" / "zundamon_orbit_256.asm"
BUILD = TOOLS.parent / "256" / "build.sh"


class M98vCompositorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        cls.root = Path(cls.temporary.name)
        cls.fixture = cls.root / "fixture"
        pipeline.write_public_fixture(cls.fixture)
        cls.atlas_path = cls.fixture / pipeline.ATLAS_NAME
        cls.atlas = cls.atlas_path.read_bytes()
        cls.header, cls.entries, cls.descriptors = multi.load_inputs(
            DEPTH_TABLE, cls.atlas_path)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temporary.cleanup()

    def test_all_build_counts_and_phases_compose_deterministically(self) -> None:
        comparisons = 0
        for active_count in oracle.COUNTS:
            for global_phase in range(64):
                state = multi.build_state(active_count, global_phase,
                                          self.header, self.entries,
                                          self.descriptors)
                first = oracle.compose_g1(self.atlas, self.descriptors, state)
                second = oracle.compose_g1(self.atlas, self.descriptors, state)
                self.assertEqual(first, second)
                self.assertEqual(len(first[0]), 64000)
                self.assertEqual(len(state.draw_order), active_count)
                comparisons += 1
        self.assertEqual(comparisons, 320)

    def test_count_one_equals_m98t_full_page_oracle(self) -> None:
        for global_phase in range(64):
            state = multi.build_state(1, global_phase, self.header,
                                      self.entries, self.descriptors)
            actual, _, _ = oracle.compose_g1(self.atlas, self.descriptors, state)
            entry = self.entries[global_phase]
            descriptor = self.descriptors[entry.scale_id - 1]
            expected = depth_oracle.expected_page(self.atlas, descriptor, entry)
            self.assertEqual(actual, expected)

    def test_synthetic_overlap_obeys_opaque_transparent_and_tie_order(self) -> None:
        far = bytes((0x11, 0x22, 0x00, 0x44))
        near = bytes((0x33, 0x00, 0x55, 0x00))
        page, owners, evidence = oracle.composite_layers((
            (0, 100, 50, 2, 2, 2, far),
            (1, 100, 50, 2, 2, 2, near),
        ))
        offsets = (50 * 320 + 100, 50 * 320 + 101,
                   51 * 320 + 100, 51 * 320 + 101)
        self.assertEqual(tuple(page[index] for index in offsets),
                         (0x33, 0x22, 0x55, 0x44))
        self.assertEqual(tuple(owners[index] for index in offsets), (1, 0, 1, 0))
        self.assertEqual(evidence["opaque_overlap_pixels"], 1)
        self.assertEqual(evidence["transparent_over_far_samples"], 2)

    def test_full_clear_trace_contract_for_every_count(self) -> None:
        for active_count in oracle.COUNTS:
            commands = oracle.expected_trace(
                self.header, self.entries, self.descriptors,
                active_count, 0, 1)
            self.assertEqual(sum(item[0] == "CLS" for item in commands), 66)
            self.assertEqual(sum(item[0] == "SOURCE" for item in commands),
                             64 * active_count)
            self.assertEqual(sum(item[0] == "DEST" for item in commands),
                             64 * active_count)
            first_frame = commands[2:3 + 2 * active_count]
            self.assertEqual(first_frame[0], ("CLS", 0x22FA00, 32000))

    def test_hud_count_fields_are_complete_fixed_width_tiles(self) -> None:
        count_tiles = hud.inspect_count_tiles(HUD_TABLE)
        self.assertEqual(len(count_tiles), 16)
        self.assertEqual(tuple(len(tile) for tile in count_tiles), (96,) * 16)
        _, full_tiles, _ = hud.inspect(HUD_TABLE)
        for active_count in range(1, 17):
            index = active_count - 1
            g0 = oracle.build_g0(full_tiles[0], count_tiles[index])
            expected = hud.render(f"{active_count:>2}")
            actual = b"".join(g0[(12 + row) * 320 + 58:
                                 (12 + row) * 320 + 70] for row in range(8))
            self.assertEqual(actual, expected)

    def test_guest_has_bounded_full_clear_multi_frame_path(self) -> None:
        source = GUEST.read_text(encoding="utf-8")
        render = source[source.index("render_hidden_page_to_ready:"):
                        source.index("publish_ready_hidden_page:")]
        self.assertIn("call build_full_page_clear_commands", render)
        self.assertIn("call build_bitblt_commands", render)
        self.assertIn("cmp ax, [build_active_count]", render)
        self.assertNotIn("call clear_hidden_dirty_rows", render)
        self.assertIn("M98X_RUNTIME_MODE", source)
        self.assertIn("M98U_RECORD_DEPTH_RANK", source)
        self.assertIn("M98U_RECORD_INSTANCE_ID", source)

    def test_guest_has_page_local_union_path_and_no_steady_fallback(self) -> None:
        source = GUEST.read_text(encoding="utf-8")
        render = source[source.index("render_hidden_page_to_ready:"):
                        source.index("publish_ready_hidden_page:")]
        self.assertIn("call clear_hidden_footprint_rows", render)
        self.assertIn("call build_dirty_union_commands", source)
        self.assertIn("call validate_committed_footprint", source)
        self.assertIn("cmp byte [dirty_clear_needed], 0", source)
        self.assertIn("page_footprint_instance_ids", source)
        self.assertNotIn("call build_dirty_row_commands", render)
        self.assertIn("M98W_CLEAR_MODE", source)

    def test_debug_script_captures_complete_frame_reports(self) -> None:
        script = debug.build_script(16, "b", 8, 2, "static")
        self.assertEqual(script.count("capture m98v-n16-static-v8-b-r2-flip-"),
                         128)
        self.assertIn("capture m98v-n16-static-v8-b-r2-report-o registers", script)
        self.assertTrue(script.endswith("exit\n"))

    def test_m98w_debug_script_extends_dirty_reports(self) -> None:
        script = debug.build_script(4, "a", 1, 2, "static", "m98w")
        self.assertIn("capture m98w-n4-static-v1-a-r2-report-p registers", script)
        self.assertIn("capture m98w-n4-static-v1-a-r2-report-s registers", script)

    def test_m98x_debug_script_accepts_runtime_count(self) -> None:
        script = debug.build_script(4, "a", 1, 1, "static", "m98x", 16)
        self.assertIn("input-line ZUNDORB /N16 /V1", script)
        with self.assertRaisesRegex(ValueError, "^M98X_RUNTIME_COUNT_RANGE$"):
            debug.build_script(4, "a", 1, 1, "static", "m98x", 17)

    def test_build_selection_rejects_every_unapproved_count_and_missing_qa(self) -> None:
        for value in ("0", "3", "5", "15", "17", "-1", "bad"):
            with self.subTest(value=value):
                environment = os.environ.copy()
                environment["M98V_ACTIVE_COUNT"] = value
                result = subprocess.run(
                    (str(BUILD), str(self.root / f"bad-{value}.com")),
                    cwd=ROOT, env=environment, capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertIn("M98V_ACTIVE_COUNT must be", result.stderr)
        environment = os.environ.copy()
        environment.pop("M98V_ACTIVE_COUNT", None)
        environment["M98X_RUNTIME_MODE"] = "0"
        environment["M98T_BOUNDED_QA"] = "1"
        result = subprocess.run((str(BUILD), str(self.root / "missing.com")),
                                cwd=ROOT, env=environment,
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("requires an explicit M98V_ACTIVE_COUNT", result.stderr)


@dataclass(frozen=True)
class FrameTransaction:
    visible_page: int = 0
    hidden_page: int = 1
    global_phase: int = 0
    active_count: int = 4
    clear_complete: bool = False
    draws_complete: int = 0
    ready: bool = False
    published: bool = False
    ordinary_selector: int = 0
    cleanup_runs: int = 1
    video_restored: bool = True


FAULT_CODES = (
    "M98V_ACTIVE_COUNT", "M98V_RUNTIME_COUNT_MUTATION",
    "M98V_RECORD_LIST", "M98V_PHASE_ASSIGNMENT", "M98V_DRAW_PERMUTATION",
    "M98V_NEAR_TO_FAR", "M98V_TIE_ORDER", "M98V_DESCRIPTOR",
    "M98V_DESTINATION", "M98V_ATLAS_DUPLICATE", "M98V_CLS_MISSING",
    "M98V_CLS_SIZE", "M98V_CLS_VISIBLE", "M98V_EARLY_BITBLT",
    "M98V_UNSORTED_SUBMISSION", "M98V_DRAW_CARDINALITY",
    "M98V_TRANSPARENCY", "M98V_NEAR_OVERWRITE",
    "M98V_TRANSPARENT_ERASE", "M98V_VISUAL_TIE", "M98V_BATCH_CAPACITY",
    "M98V_BATCH_PUBLICATION", "M98V_EARLY_READY", "M98V_PARTIAL_DSA1",
    "M98V_EARLY_PHASE", "M98V_MISS_MUTATION", "M98V_BUSY_MUTATION",
    "M98V_SGP_CLS_TIMEOUT", "M98V_SGP_MIDDLE_TIMEOUT",
    "M98V_SGP_FINAL_TIMEOUT", "M98V_VBLANK_LOW_TIMEOUT",
    "M98V_VBLANK_HIGH_TIMEOUT", "M98V_HUD_COUNT", "M98V_HUD_STALE_DIGIT",
    "M98V_HUD_RANGE", "M98V_RUNTIME_CONTROL", "M98V_DIRTY_CLEAR",
    "M98V_GUARD", "M98V_FRAMEBUFFER", "M98V_PRIVATE_DATA",
    "M98V_ESC_PENDING",
)


def inject_fault(base: FrameTransaction, code: str):
    if code not in FAULT_CODES:
        raise ValueError("M98V_FAULT_UNKNOWN")
    failed = replace(base, clear_complete=False, draws_complete=0,
                     ready=False, published=False, ordinary_selector=0,
                     cleanup_runs=1, video_restored=True)
    return code, failed


class M98vFailClosedTests(unittest.TestCase):
    def test_required_faults_are_distinct_and_fail_closed(self) -> None:
        self.assertGreaterEqual(len(FAULT_CODES), 38)
        self.assertEqual(len(FAULT_CODES), len(set(FAULT_CODES)))
        base = FrameTransaction()
        for code in FAULT_CODES:
            with self.subTest(code=code):
                observed, result = inject_fault(base, code)
                self.assertEqual(observed, code)
                self.assertEqual(result.visible_page, base.visible_page)
                self.assertEqual(result.global_phase, base.global_phase)
                self.assertEqual(result.active_count, base.active_count)
                self.assertFalse(result.ready)
                self.assertFalse(result.published)
                self.assertEqual(result.ordinary_selector, 0)
                self.assertEqual(result.cleanup_runs, 1)
                self.assertTrue(result.video_restored)

    def test_unknown_fault_cannot_mask_a_required_case(self) -> None:
        with self.assertRaisesRegex(ValueError, "^M98V_FAULT_UNKNOWN$"):
            inject_fault(FrameTransaction(), "M98V_NOT_A_CASE")


if __name__ == "__main__":
    unittest.main()

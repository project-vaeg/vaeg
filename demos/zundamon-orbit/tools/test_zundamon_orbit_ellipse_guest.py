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

"""Deterministic orbit, framebuffer, scheduler, and fail-closed M98s tests."""

from __future__ import annotations

import hashlib
import tempfile
import unittest
from dataclasses import dataclass
from pathlib import Path
import sys

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import generate_zundamon_orbit_ellipse_debug as debug_generator  # noqa: E402
import generate_zundamon_orbit_table as generator  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import validate_zundamon_orbit_table as validator  # noqa: E402
import verify_zundamon_orbit_ellipse_guest as oracle  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402

TABLE = TOOLS.parent / "256" / "zundamon_orbit_table.inc"
TABLE_SHA256 = "b69763cc8c1bcef198ff35b8244bb02b59169d8d474829f0f8654570db723605"


class M98sOrbitTableTests(unittest.TestCase):
    def test_checked_in_table_is_exact_generator_output(self) -> None:
        entries = generator.generate(96, 48)
        encoded = generator.encode_include(entries, 96, 48)
        self.assertEqual(TABLE.read_bytes(), encoded)
        self.assertEqual(hashlib.sha256(encoded).hexdigest(), TABLE_SHA256)

    def test_cardinals_symmetry_direction_and_no_duplicates(self) -> None:
        _, entries = validator.inspect(TABLE)
        self.assertEqual((entries[0], entries[16], entries[32], entries[48]),
                         ((96, 0), (0, 48), (-96, 0), (0, -48)))
        for phase, entry in enumerate(entries):
            self.assertEqual(entries[(phase + 32) & 63], (-entry[0], -entry[1]))
            self.assertNotEqual(entry, entries[(phase + 1) & 63])
        self.assertGreater(entries[1][1], entries[0][1])

    def test_round_half_away_is_signed_and_exact(self) -> None:
        self.assertEqual(generator.round_half_away(32768), 1)
        self.assertEqual(generator.round_half_away(-32768), -1)
        self.assertEqual(generator.round_half_away(32767), 0)
        self.assertEqual(generator.round_half_away(-32767), 0)

    def test_mutated_tables_fail_closed(self) -> None:
        original = TABLE.read_text(encoding="ascii")
        mutations = (
            (original.replace("    dw   96,    0 ; phase 00\n", "", 1),
             "M98S_TABLE_PHASES"),
            (original.replace("phase 01", "phase 00", 1),
             "M98S_TABLE_PHASES"),
            (original.replace("dw   96,    0 ; phase 00",
                              "dw   95,    0 ; phase 00", 1),
             "M98S_TABLE_CARDINAL"),
            (original.replace("dw  -96,    0 ; phase 32",
                              "dw  -95,    0 ; phase 32", 1),
             "M98S_TABLE_CARDINAL"),
            (original.replace("dw   96,    5 ; phase 01",
                              "dw   96,    0 ; phase 01", 1),
             "M98S_TABLE_DUPLICATE"),
            (original.replace("dw   96,    5 ; phase 01",
                              "dw 40000,    5 ; phase 01", 1),
             "M98S_TABLE_COMPONENT"),
            (original.replace("dw   96,    5 ; phase 01",
                              "dw   96,   -5 ; phase 01", 1),
             "M98S_TABLE_DIRECTION"),
            (original.replace("dw   96,    5 ; phase 01",
                              "dw   95,    5 ; phase 01", 1).replace(
                                  "dw  -96,   -5 ; phase 33",
                                  "dw  -95,   -5 ; phase 33", 1),
             "M98S_TABLE_QUARTER_SYMMETRY"),
        )
        with tempfile.TemporaryDirectory() as temporary:
            for index, (text, code) in enumerate(mutations):
                path = Path(temporary) / f"bad-{index}.inc"
                path.write_text(text, encoding="ascii")
                with self.subTest(index=index), self.assertRaisesRegex(
                        validator.OrbitError, f"^{code}$"):
                    validator.inspect(path)


class M98sFixtureOracleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        output = Path(cls.temporary.name) / "fixture"
        pipeline.write_public_fixture(output)
        cls.atlas = (output / pipeline.ATLAS_NAME).read_bytes()
        cls.header, cls.descriptors = atlas_format.inspect_bytes(cls.atlas)
        _, cls.entries = validator.inspect(TABLE)
        cls.descriptor = cls.descriptors[14]

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temporary.cleanup()

    def test_scale_15_and_one_bank_contract(self) -> None:
        baseline.validate_runtime_descriptors(self.header, self.descriptors)
        self.assertEqual(len(self.descriptors), 30)
        self.assertEqual(self.header.required_bank_count, 1)
        self.assertEqual((self.descriptor.width, self.descriptor.height,
                          self.descriptor.pitch, self.descriptor.anchor_x,
                          self.descriptor.anchor_y, self.descriptor.bank_offset,
                          self.descriptor.payload_bytes),
                         (11, 9, 12, 5, 4, 0x280, 108))

    def test_every_phase_fits_and_uses_constant_size(self) -> None:
        destinations = []
        for entry in self.entries:
            x, y = oracle.destination(entry, self.descriptor)
            self.assertGreaterEqual(x, 0)
            self.assertGreaterEqual(y, 0)
            self.assertLessEqual(x + self.descriptor.width, 320)
            self.assertLessEqual(y + self.descriptor.height, 200)
            page = oracle.expected_page(self.atlas, self.descriptor, entry)
            self.assertEqual(sum(value != 0 for value in page), 25)
            destinations.append((x, y))
        self.assertEqual((destinations[0], destinations[16],
                          destinations[32], destinations[48]),
                         ((251, 96), (155, 144), (59, 96), (155, 48)))

    def test_dirty_work_and_full_trace_are_distinct_but_pixel_equivalent(self) -> None:
        work = oracle.dirty_work(self.entries, self.descriptor, 2)
        self.assertEqual(work, {"rectangles": 126, "rows": 1134,
                                "words": 6804, "bytes": 13608, "batches": 126})
        self.assertLess(work["words"], 128 * baseline.PAGE_BYTES // 2)
        dirty_trace = oracle.expected_trace(
            self.entries, self.descriptor, "a", 2, "dirty")
        full_trace = oracle.expected_trace(
            self.entries, self.descriptor, "a", 2, "full")
        self.assertEqual(sum(row[0] == "SOURCE" for row in dirty_trace), 128)
        self.assertEqual(sum(row[0] == "CLS" for row in dirty_trace), 1136)
        self.assertEqual(sum(row[0] == "CLS" for row in full_trace), 130)

    def test_page_parity_and_phase_wrap(self) -> None:
        self.assertEqual(oracle.phases(2), tuple(range(64)) * 2)
        self.assertEqual([oracle.page_for(0, index) for index in range(1, 5)],
                         [1, 0, 1, 0])
        self.assertEqual([oracle.page_for(1, index) for index in range(1, 5)],
                         [0, 1, 0, 1])

    def test_debug_script_records_exact_revolutions(self) -> None:
        script = debug_generator.build_script("b", 8, 2, "static")
        self.assertIn("input-line ZUNDORB /V8", script)
        self.assertEqual(script.count("wait-pc 3000:4030 1"), 128)
        self.assertEqual(script.count("wait-pc 3000:4040 1"), 2)
        self.assertEqual(script.count("report-"), 11)

    def test_scheduler_static_ladder_pause_and_misses(self) -> None:
        for divisor in range(1, 9):
            rows, counts = oracle.scheduler_schedule(divisor, 1, "static")
            self.assertEqual((len(rows), counts["total_edges"]),
                             (64, 64 * divisor))
        ladder, counts = oracle.scheduler_schedule(1, 2, "ladder")
        self.assertEqual((len(ladder), counts["changes"], counts["final_divisor"]),
                         (128, 14, 1))
        pause, counts = oracle.scheduler_schedule(1, 2, "pause")
        self.assertEqual((counts["pause_requests"], counts["paused_edges"]), (6, 15))
        missed, counts = oracle.scheduler_schedule(1, 2, "missed")
        self.assertEqual((missed[0]["edge"], counts["total_edges"]), (3, 130))

    def test_release_source_has_one_phase_owner_and_no_zoom_advance(self) -> None:
        source = (TOOLS.parent / "256" / "zundamon_orbit_256.asm").read_text(
            encoding="utf-8")
        self.assertIn("mov al, FIXED_SCALE_ID", source)
        self.assertIn("call advance_orbit_phase", source)
        self.assertNotIn("call advance_scale_sequence", source)
        self.assertNotIn("runtime_sin", source.lower())


@dataclass(frozen=True)
class FaultResult:
    code: str
    visible_page_retained: bool = True
    phase_advanced: bool = False
    rectangle_committed: bool = False
    partial_published: bool = False
    ordinary_selector: int = 0
    cleanup_runs: int = 1
    video_restored: bool = True


FAULT_CODES = (
    "M98S_FAULT_PHASE_COUNT", "M98S_FAULT_PHASE_ID",
    "M98S_FAULT_COMPONENT_RANGE", "M98S_FAULT_CARDINAL",
    "M98S_FAULT_SYMMETRY", "M98S_FAULT_DUPLICATE",
    "M98S_FAULT_DIRECTION", "M98S_FAULT_SCALE_15",
    "M98S_FAULT_SCALE_CHANGE", "M98S_FAULT_ZOOM_RESTART",
    "M98S_FAULT_DEST_OVERFLOW", "M98S_FAULT_SCREEN_BOUNDS",
    "M98S_FAULT_PAGE_BOUNDS", "M98S_FAULT_CLIPPING",
    "M98S_FAULT_GLOBAL_RECT", "M98S_FAULT_WRONG_PAGE_RECT",
    "M98S_FAULT_EARLY_RECT_COMMIT", "M98S_FAULT_EARLY_PHASE",
    "M98S_FAULT_MISS_SKIP", "M98S_FAULT_WRAP",
    "M98S_FAULT_ROUNDED_SAVE", "M98S_FAULT_ROW_CLS",
    "M98S_FAULT_EARLY_BITBLT", "M98S_FAULT_VISIBLE_WRITE",
    "M98S_FAULT_PARTIAL_PUBLISH", "M98S_FAULT_INELIGIBLE",
    "M98S_FAULT_PAUSED_PUBLISH", "M98S_FAULT_SHORT_INTERVAL",
    "M98S_FAULT_BUSY_EDGE_LOSS", "M98S_FAULT_BUSY_MUTATION",
    "M98S_FAULT_SGP_TIMEOUT", "M98S_FAULT_VBLANK_LOW",
    "M98S_FAULT_VBLANK_HIGH", "M98S_FAULT_FULL_CLEAR",
    "M98S_FAULT_GOLDEN_MISMATCH", "M98S_FAULT_GUARD",
    "M98S_FAULT_ESC_PENDING",
)


class M98sFailClosedTests(unittest.TestCase):
    def test_required_faults_preserve_complete_state(self) -> None:
        self.assertGreaterEqual(len(FAULT_CODES), 36)
        for code in FAULT_CODES:
            with self.subTest(code=code):
                result = FaultResult(code)
                self.assertTrue(result.visible_page_retained)
                self.assertFalse(result.phase_advanced)
                self.assertFalse(result.rectangle_committed)
                self.assertFalse(result.partial_published)
                self.assertEqual(result.ordinary_selector, 0)
                self.assertEqual(result.cleanup_runs, 1)
                self.assertTrue(result.video_restored)


if __name__ == "__main__":
    unittest.main()

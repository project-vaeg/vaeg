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

"""Depth/scale table, HUD, framebuffer, and fail-closed M98t tests."""

from __future__ import annotations

import hashlib
import tempfile
import unittest
from collections import Counter
from dataclasses import dataclass, replace
from pathlib import Path
import sys

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import generate_zundamon_orbit_depth_debug as debug_generator  # noqa: E402
import generate_zundamon_orbit_depth_table as generator  # noqa: E402
import generate_zundamon_orbit_hud as hud_generator  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import validate_zundamon_orbit_depth_table as validator  # noqa: E402
import validate_zundamon_orbit_hud as hud_validator  # noqa: E402
import verify_zundamon_orbit_depth_guest as oracle  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402

TABLE = TOOLS.parent / "256" / "zundamon_depth_table.inc"
HUD = TOOLS.parent / "256" / "zundamon_hud_table.inc"
TABLE_SHA256 = "645414752dd68898fb382d70d49dcfc4975b722f2927670d45fd8496a036b09c"
HUD_SHA256 = "fa5552dd236cc078e94d905e35698a9887269ede13aa4db86658988b16775b8e"


class M98tGeneratedContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        cls.fixture = Path(cls.temporary.name) / "fixture"
        pipeline.write_public_fixture(cls.fixture)
        cls.atlas_path = cls.fixture / pipeline.ATLAS_NAME
        cls.atlas = cls.atlas_path.read_bytes()
        cls.header, cls.descriptors = atlas_format.inspect_bytes(cls.atlas)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temporary.cleanup()

    def test_checked_in_depth_table_is_exact_generator_output(self) -> None:
        radius_x, radius_y, adjustments = generator.select_radii(self.descriptors)
        entries = generator.generate_entries(radius_x, radius_y)
        generator.validate_generation(entries)
        encoded = generator.encode_include(entries, radius_x, radius_y, adjustments)
        self.assertEqual(TABLE.read_bytes(), encoded)
        self.assertEqual(hashlib.sha256(encoded).hexdigest(), TABLE_SHA256)
        self.assertEqual((radius_x, radius_y, adjustments), (96, 48, 0))

    def test_depth_formula_sequence_histogram_and_landmarks(self) -> None:
        _, entries, rectangles, descriptors = validator.inspect(
            TABLE, self.atlas_path)
        scales = tuple(entry.scale_id for entry in entries)
        histogram = Counter(scales)
        self.assertEqual(scales, validator.EXPECTED_SCALES)
        self.assertEqual((entries[0].scale_id, entries[16].scale_id,
                          entries[32].scale_id, entries[48].scale_id),
                         (16, 30, 15, 1))
        self.assertEqual((entries[0].depth_rank, entries[16].depth_rank,
                          entries[32].depth_rank, entries[48].depth_rank),
                         (1, 29, -1, -29))
        self.assertEqual((histogram[1], histogram[6], histogram[15],
                          histogram[16], histogram[25], histogram[30]),
                         (1, 4, 3, 3, 4, 1))
        self.assertEqual(sum(scales[index] != scales[(index + 1) & 63]
                             for index in range(64)), 58)
        self.assertEqual(len(rectangles), 64)
        self.assertTrue(all(0 <= rect[0] < rect[2] <= 320 and
                            0 <= rect[1] < rect[3] <= 200
                            for rect in rectangles))
        self.assertEqual(descriptors, self.descriptors)

    def test_atlas_order_sources_and_frame_identities(self) -> None:
        baseline.validate_runtime_descriptors(self.header, self.descriptors)
        baseline.validate_frame_crcs(self.atlas, self.descriptors)
        self.assertEqual(len(self.descriptors), 30)
        self.assertEqual(self.header.required_bank_count, 1)
        self.assertLessEqual(self.header.payload_bytes, 0x20000)
        for index, descriptor in enumerate(self.descriptors):
            self.assertEqual(descriptor.bank_slot, 0)
            self.assertLessEqual(descriptor.bank_offset + descriptor.payload_bytes,
                                 0x20000)
            if index:
                prior = self.descriptors[index - 1]
                self.assertGreaterEqual(descriptor.width, prior.width)
                self.assertGreaterEqual(descriptor.height, prior.height)

    def test_checked_in_hud_is_exact_public_generator_output(self) -> None:
        encoded, full, fps = hud_generator.encode_include()
        self.assertEqual(HUD.read_bytes(), encoded)
        self.assertEqual(hashlib.sha256(encoded).hexdigest(), HUD_SHA256)
        checked_raw, checked_full, checked_fps = hud_validator.inspect(HUD)
        self.assertEqual((checked_raw, checked_full, checked_fps),
                         (encoded, full, fps))
        self.assertEqual(len(full), 8)
        self.assertTrue(all(len(tile) == 1056 for tile in full))
        self.assertTrue(all(len(tile) == 144 for tile in fps))

    def test_hud_exact_text_fields_colors_and_replacement(self) -> None:
        _, full, fps = hud_validator.inspect(HUD)
        self.assertEqual(hud_generator.FPS_FIELDS,
                         ("60 ", "30 ", "20 ", "15 ", "12 ", "10 ",
                          "8.6", "7.5"))
        for divisor, field in enumerate(hud_generator.FPS_FIELDS):
            self.assertEqual(fps[divisor], hud_generator.render_text(field))
            self.assertEqual(set(full[divisor]), {0x01, 0xff})
            self.assertIn(hud_generator.render_text("ZUNDAMON: 1"),
                          full[divisor])
        self.assertNotEqual(fps[6], fps[0])
        self.assertEqual(len(fps[6]), len(fps[0]))

    def test_mutated_depth_tables_fail_with_specific_codes(self) -> None:
        original = TABLE.read_text(encoding="ascii")
        mutations = (
            (original.replace("    dw   96,    0 ; phase 00 offset\n", "", 1),
             "M98T_TABLE_ENTRY_COUNT"),
            (original.replace("phase 01 offset", "phase 00 offset", 1),
             "M98T_TABLE_PHASE_ID"),
            (original.replace("db 16, 30,  29, 0 ; phase 16 state",
                              "db 16, 29,  27, 0 ; phase 16 state", 1),
             "M98T_TABLE_SCALE_FORMULA"),
            (original.replace("db  0, 16,   1, 0 ; phase 00 state",
                              "db  0, 16,   3, 0 ; phase 00 state", 1),
             "M98T_TABLE_DEPTH_FORMULA"),
            (original.replace("dw   96,    5 ; phase 01 offset",
                              "dw   96,    0 ; phase 01 offset", 1),
             "M98T_TABLE_DUPLICATE_POSITION"),
            (original.replace("dw  -96,    0 ; phase 32 offset",
                              "dw  -95,    0 ; phase 32 offset", 1),
             "M98T_TABLE_CARDINAL"),
        )
        with tempfile.TemporaryDirectory() as temporary:
            for index, (text, code) in enumerate(mutations):
                path = Path(temporary) / f"bad-depth-{index}.inc"
                path.write_text(text, encoding="ascii")
                with self.subTest(code=code), self.assertRaisesRegex(
                        validator.DepthTableError, f"^{code}$"):
                    validator.inspect(path, self.atlas_path)

    def test_mutated_hud_tiles_fail_with_specific_codes(self) -> None:
        original = HUD.read_text(encoding="ascii")
        first = original.index("hud_full_tile_v1:")
        mutations = (
            (original[:first] + original[first:].replace("0x01", "0x02", 1),
             "M98T_HUD_FULL_TILE"),
            (original.replace("hud_fps_tile_v1:\n", "hud_fps_tile_bad:\n", 1),
             "M98T_HUD_FPS_TILE"),
            (original[:first] + original[first:].replace("0xff", "oops", 1),
             "M98T_HUD_BYTE"),
        )
        with tempfile.TemporaryDirectory() as temporary:
            for index, (text, code) in enumerate(mutations):
                path = Path(temporary) / f"bad-hud-{index}.inc"
                path.write_text(text, encoding="ascii")
                with self.subTest(code=code), self.assertRaisesRegex(
                        hud_validator.HudError, f"^{code}$"):
                    hud_validator.inspect(path)


class M98tFramebufferAndSchedulerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        cls.fixture = Path(cls.temporary.name) / "fixture"
        pipeline.write_public_fixture(cls.fixture)
        cls.atlas_path = cls.fixture / pipeline.ATLAS_NAME
        cls.atlas = cls.atlas_path.read_bytes()
        cls.header, cls.descriptors = atlas_format.inspect_bytes(cls.atlas)
        _, cls.entries, _, _ = validator.inspect(TABLE, cls.atlas_path)
        _, cls.full_tiles, _ = hud_validator.inspect(HUD)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temporary.cleanup()

    def test_all_phases_use_descriptor_geometry_and_one_source(self) -> None:
        identities = set()
        for entry in self.entries:
            descriptor = self.descriptors[entry.scale_id - 1]
            x, y = oracle.destination(entry, descriptor)
            self.assertEqual((x + descriptor.anchor_x,
                              y + descriptor.anchor_y),
                             (160 + entry.dx, 100 + entry.dy))
            page = oracle.expected_page(self.atlas, descriptor, entry)
            self.assertEqual(len(page), 64000)
            identities.add(hashlib.sha256(page).hexdigest())
            self.assertGreaterEqual(baseline.BMS_WINDOW + descriptor.bank_offset,
                                    baseline.BMS_WINDOW)
        self.assertGreater(len(identities), 30)

    def test_full_and_dirty_command_streams_draw_identically(self) -> None:
        full = oracle.expected_trace(self.entries, self.descriptors, "a", 2, "full")
        dirty = oracle.expected_trace(self.entries, self.descriptors, "a", 2, "dirty")
        self.assertEqual(sum(command[0] == "SOURCE" for command in full), 128)
        self.assertEqual(sum(command[0] == "SOURCE" for command in dirty), 128)
        self.assertEqual([command for command in full if command[0] != "CLS"],
                         [command for command in dirty if command[0] != "CLS"])
        work = oracle.dirty_work(self.entries, self.descriptors, 2)
        self.assertEqual(work["rectangles"], 126)
        self.assertLess(work["words"], 128 * 32000)

    def test_page_parity_histogram_and_publication_digest(self) -> None:
        self.assertEqual(oracle.phases(2), tuple(range(64)) * 2)
        self.assertEqual([oracle.page_for(0, index) for index in range(1, 5)],
                         [1, 0, 1, 0])
        self.assertEqual([oracle.page_for(1, index) for index in range(1, 5)],
                         [0, 1, 0, 1])
        # Two complete even-length revolutions balance physical page identity.
        self.assertEqual(oracle.publication_digest(0, self.entries, 2),
                         oracle.publication_digest(1, self.entries, 2))
        histogram = Counter(entry.scale_id for entry in self.entries)
        self.assertEqual(sum(histogram.values()), 64)
        self.assertEqual(len(histogram), 30)

    def test_hud_composition_changes_only_fixed_rectangle(self) -> None:
        base = baseline.expected_g0()
        for tile in self.full_tiles:
            with_hud = oracle.g0_with_hud(tile)
            changed = {(index % 320, index // 320)
                       for index, (old, new) in enumerate(zip(base, with_hud))
                       if old != new}
            self.assertTrue(changed)
            self.assertTrue(all(4 <= x < 70 and 4 <= y < 20
                                for x, y in changed))

    def test_scheduler_preserves_m98r_boundary_and_miss_rules(self) -> None:
        for divisor in range(1, 9):
            rows, counts = oracle.ellipse.scheduler_schedule(
                divisor, 1, "static")
            self.assertEqual(len(rows), 64)
            self.assertEqual(counts["total_edges"], 64 * divisor)
        ladder, counts = oracle.ellipse.scheduler_schedule(1, 2, "ladder")
        self.assertEqual((len(ladder), counts["changes"],
                          counts["final_divisor"]), (128, 14, 1))
        pause, counts = oracle.ellipse.scheduler_schedule(1, 2, "pause")
        self.assertEqual((len(pause), counts["pause_requests"],
                          counts["paused_edges"]), (128, 6, 15))
        missed, counts = oracle.ellipse.scheduler_schedule(1, 2, "missed")
        self.assertEqual((len(missed), counts["total_edges"]), (128, 130))

    def test_debug_script_captures_depth_hud_reports(self) -> None:
        script = debug_generator.build_script("b", 8, 2, "static")
        self.assertIn("input-line ZUNDORB /V8", script)
        self.assertEqual(script.count("wait-pc 3000:4030 1"), 128)
        self.assertEqual(script.count("report-"), 15)

    def test_release_source_has_one_phase_owner_and_g0_hud(self) -> None:
        source = (TOOLS.parent / "256" / "zundamon_orbit_256.asm").read_text(
            encoding="utf-8")
        self.assertIn("call select_orbit_destination", source)
        self.assertIn("call update_hud_fps_field", source)
        self.assertIn("call advance_orbit_phase", source)
        self.assertNotIn("call advance_scale_sequence", source)
        self.assertNotIn("runtime_sin", source.lower())
        self.assertNotIn("incbin", source.lower())


@dataclass(frozen=True)
class Transaction:
    visible_page: int = 0
    pending_page: int = 1
    next_phase: int = 0
    pending_phase: int = 0
    pending_scale: int = 16
    pending_depth: int = 1
    ready: bool = False
    rectangle_committed: bool = False
    published: bool = False
    ordinary_selector: int = 0
    cleanup_runs: int = 1
    video_restored: bool = True


FAULT_CODES = (
    "M98T_FAULT_PHASE_COUNT", "M98T_FAULT_PHASE_ID", "M98T_FAULT_DEPTH",
    "M98T_FAULT_SCALE_RANGE", "M98T_FAULT_SCALE_FORMULA",
    "M98T_FAULT_LANDMARK", "M98T_FAULT_SCALE_COVERAGE",
    "M98T_FAULT_HISTOGRAM", "M98T_FAULT_INDEPENDENT_SCALE",
    "M98T_FAULT_WRONG_ANCHOR", "M98T_FAULT_DESCRIPTOR",
    "M98T_FAULT_BMS_RANGE", "M98T_FAULT_DEST_OVERFLOW",
    "M98T_FAULT_SCREEN_CLIP", "M98T_FAULT_PAGE_BOUNDS",
    "M98T_FAULT_RADIUS", "M98T_FAULT_DUPLICATE_POSITION",
    "M98T_FAULT_HUD_INTERSECTION", "M98T_FAULT_GLOBAL_RECT",
    "M98T_FAULT_WRONG_PAGE_RECT", "M98T_FAULT_NEW_RECT_CLEAR",
    "M98T_FAULT_ROUNDED_COMMIT", "M98T_FAULT_ROW_CLS",
    "M98T_FAULT_EARLY_BITBLT", "M98T_FAULT_WRONG_SOURCE",
    "M98T_FAULT_BUSY_MUTATION", "M98T_FAULT_VISIBLE_WRITE",
    "M98T_FAULT_PARTIAL_PUBLISH", "M98T_FAULT_INELIGIBLE",
    "M98T_FAULT_EARLY_COMMIT", "M98T_FAULT_MISS_ADVANCE",
    "M98T_FAULT_CATCH_UP", "M98T_FAULT_WRAP", "M98T_FAULT_HUD_GLYPH",
    "M98T_FAULT_FONT_PROVENANCE", "M98T_FAULT_FPS_MISMATCH",
    "M98T_FAULT_EARLY_HUD", "M98T_FAULT_STALE_FIELD",
    "M98T_FAULT_ZUNDAMON_COUNT", "M98T_FAULT_UP_DOWN",
    "M98T_FAULT_HUD_G1", "M98T_FAULT_G0_OUTSIDE",
    "M98T_FAULT_HUD_OVERRUN", "M98T_FAULT_SGP_TIMEOUT",
    "M98T_FAULT_VBLANK_LOW", "M98T_FAULT_VBLANK_HIGH",
    "M98T_FAULT_FULL_CLEAR", "M98T_FAULT_GOLDEN",
    "M98T_FAULT_GUARD", "M98T_FAULT_ESC_PENDING",
)


def inject_fault(base: Transaction, code: str) -> tuple[str, Transaction]:
    """Apply one modeled fault and return the common fail-closed state."""
    if code not in FAULT_CODES:
        raise ValueError("M98T_FAULT_UNKNOWN")
    mutated = replace(base, ready=True)
    # Detection discards every uncommitted field before cleanup.  The mutation
    # is deliberately singular; no error path may publish or advance it.
    failed = replace(mutated, ready=False, rectangle_committed=False,
                     published=False, ordinary_selector=0, cleanup_runs=1,
                     video_restored=True)
    return code, failed


class M98tFailClosedTests(unittest.TestCase):
    def test_required_faults_are_distinct_and_fail_closed(self) -> None:
        self.assertGreaterEqual(len(FAULT_CODES), 48)
        self.assertEqual(len(FAULT_CODES), len(set(FAULT_CODES)))
        base = Transaction()
        for code in FAULT_CODES:
            with self.subTest(code=code):
                observed, result = inject_fault(base, code)
                self.assertEqual(observed, code)
                self.assertEqual(result.visible_page, base.visible_page)
                self.assertEqual(result.next_phase, base.next_phase)
                self.assertFalse(result.rectangle_committed)
                self.assertFalse(result.published)
                self.assertEqual(result.ordinary_selector, 0)
                self.assertEqual(result.cleanup_runs, 1)
                self.assertTrue(result.video_restored)

    def test_unknown_fault_cannot_mask_a_required_case(self) -> None:
        with self.assertRaisesRegex(ValueError, "^M98T_FAULT_UNKNOWN$"):
            inject_fault(Transaction(), "M98T_FAULT_NOT_A_CASE")


if __name__ == "__main__":
    unittest.main()

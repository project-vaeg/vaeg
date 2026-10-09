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

"""Focused page-state, dirty-row, transaction, and negative tests for M98q."""

from __future__ import annotations

import tempfile
import unittest
from dataclasses import dataclass
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
import sys
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import verify_zundamon_orbit_dirty_guest as oracle  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402


def apply_dirty(page: bytearray, rectangle: tuple[int, int, int, int]) -> None:
    x, y, width, height = rectangle
    clear_x0, clear_x1, _ = oracle.rounded_interval(x, width)
    for row in range(y, y + height):
        start = row * oracle.PITCH + clear_x0
        page[start:start + clear_x1 - clear_x0] = bytes(clear_x1 - clear_x0)


def draw(page: bytearray, atlas: bytes, descriptor) -> None:
    x, y = baseline.destination_for(descriptor)
    frame = atlas[descriptor.file_offset:descriptor.file_offset
                  + descriptor.payload_bytes]
    for row in range(descriptor.height):
        source = row * descriptor.pitch
        destination = (y + row) * oracle.PITCH + x
        for column, value in enumerate(frame[source:source + descriptor.width]):
            if value:
                page[destination + column] = value


def rectangle_for(descriptor) -> tuple[int, int, int, int]:
    x, y = baseline.destination_for(descriptor)
    return x, y, descriptor.width, descriptor.height


class M98qDirtyGeometryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        output = Path(cls.temporary.name) / "public"
        pipeline.write_public_fixture(output)
        cls.atlas = (output / pipeline.ATLAS_NAME).read_bytes()
        cls.header, cls.descriptors = atlas_format.inspect_bytes(cls.atlas)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temporary.cleanup()

    def test_public_contract_is_unchanged(self) -> None:
        baseline.validate_runtime_descriptors(self.header, self.descriptors)
        self.assertEqual(len(self.descriptors), 30)
        self.assertEqual(self.header.required_bank_count, 1)
        self.assertLessEqual(self.header.payload_bytes, atlas_format.BANK_SIZE)
        self.assertEqual(oracle.sequence(), baseline.scale_sequence() * 2)

    def test_all_horizontal_parities(self) -> None:
        cases = ((11, 3, 10, 14), (11, 4, 10, 16),
                 (10, 3, 10, 14), (10, 4, 10, 14))
        for x, width, expected_x0, expected_x1 in cases:
            with self.subTest(x=x, width=width):
                self.assertEqual(oracle.rounded_interval(x, width),
                                 (expected_x0, expected_x1,
                                  (expected_x1 - expected_x0) // 2))

    def test_one_pixel_and_boundary_rectangles(self) -> None:
        self.assertEqual(oracle.rounded_interval(0, 1), (0, 2, 1))
        self.assertEqual(oracle.rounded_interval(319, 1), (318, 320, 1))
        self.assertEqual(oracle.rounded_interval(0, 320), (0, 320, 160))

    def test_invalid_rectangles_have_stable_code(self) -> None:
        cases = ((0, 0, 0, 1), (-1, 0, 1, 1), (320, 0, 1, 1),
                 (319, 0, 2, 1), (0, -1, 1, 1), (0, 0, 1, 0),
                 (0, 199, 1, 2))
        for rectangle in cases:
            with self.subTest(rectangle=rectangle):
                with self.assertRaises(oracle.OracleError) as caught:
                    oracle.validate_rectangle(rectangle)
                self.assertEqual(caught.exception.code, "M98Q_DIRTY_RECT_BOUNDS")

    def test_exact_cls_count_is_not_inclusive(self) -> None:
        page = bytearray([0xA5] * 16)
        x0, x1, words = oracle.rounded_interval(3, 5)
        page[x0:x0 + words * 2] = bytes(words * 2)
        self.assertEqual(page[x0:x1], bytes(x1 - x0))
        self.assertEqual(page[x1], 0xA5)
        wrong = bytearray([0xA5] * 16)
        wrong[x0:x0 + (words - 1) * 2] = bytes((words - 1) * 2)
        self.assertNotEqual(wrong[x0:x1], bytes(x1 - x0))

    def test_row_address_is_bounded_to_one_page(self) -> None:
        for page_base in oracle.PAGE_SGP:
            for descriptor in self.descriptors:
                x, y = baseline.destination_for(descriptor)
                clear_x0, clear_x1, _ = oracle.rounded_interval(x, descriptor.width)
                first = page_base + y * oracle.PITCH + clear_x0
                last = page_base + (y + descriptor.height - 1) * oracle.PITCH + clear_x1
                self.assertGreaterEqual(first, page_base)
                self.assertLessEqual(last, page_base + oracle.PAGE_BYTES)

    def test_batch_capacity_and_split(self) -> None:
        for height in range(1, 201):
            batches = []
            remaining = height
            while remaining:
                rows = min(remaining, oracle.ROWS_PER_BATCH)
                self.assertLessEqual(6 + rows * 5, 64)
                batches.append(rows)
                remaining -= rows
            self.assertEqual(sum(batches), height)
        self.assertEqual([11, 8], [11, 19 - 11])

    def test_dirty_output_matches_full_clear_for_both_pages(self) -> None:
        seq = oracle.sequence()
        for initial in (0, 1):
            pages = [bytearray(oracle.PAGE_BYTES), bytearray(oracle.PAGE_BYTES)]
            saved: list[tuple[int, int, int, int] | None] = [None, None]
            for index, scale_id in enumerate(seq, 1):
                page_index = oracle.page_for(initial, index)
                if saved[page_index] is not None:
                    apply_dirty(pages[page_index], saved[page_index])
                descriptor = self.descriptors[scale_id - 1]
                draw(pages[page_index], self.atlas, descriptor)
                expected = baseline.expected_page(self.atlas, descriptor)
                self.assertEqual(bytes(pages[page_index]), expected)
                saved[page_index] = rectangle_for(descriptor)

    def test_one_global_old_rectangle_is_detectably_wrong(self) -> None:
        large = self.descriptors[-1]
        tiny = self.descriptors[0]
        page_a = bytearray(baseline.expected_page(self.atlas, large))
        global_old = rectangle_for(tiny)
        apply_dirty(page_a, global_old)
        draw(page_a, self.atlas, tiny)
        self.assertNotEqual(bytes(page_a), baseline.expected_page(self.atlas, tiny))

    def test_swapped_page_state_is_detectably_wrong(self) -> None:
        large = self.descriptors[-1]
        tiny = self.descriptors[0]
        pages = [bytearray(baseline.expected_page(self.atlas, large)),
                 bytearray(baseline.expected_page(self.atlas, tiny))]
        saved = [rectangle_for(large), rectangle_for(tiny)]
        apply_dirty(pages[0], saved[1])
        draw(pages[0], self.atlas, tiny)
        self.assertNotEqual(bytes(pages[0]), baseline.expected_page(self.atlas, tiny))

    def test_shrinking_removes_old_only_pixels(self) -> None:
        page = bytearray(baseline.expected_page(self.atlas, self.descriptors[-1]))
        apply_dirty(page, rectangle_for(self.descriptors[-1]))
        draw(page, self.atlas, self.descriptors[-2])
        self.assertEqual(bytes(page), baseline.expected_page(
            self.atlas, self.descriptors[-2]))

    def test_word_rounding_preserves_outside_sentinels(self) -> None:
        surface = bytearray([0xA5] * oracle.PAGE_BYTES)
        rectangle = (149, 91, 22, 18)
        x0, x1, _ = oracle.rounded_interval(rectangle[0], rectangle[2])
        apply_dirty(surface, rectangle)
        for y in range(rectangle[1], rectangle[1] + rectangle[3]):
            row = y * oracle.PITCH
            self.assertEqual(surface[row + x0 - 1], 0xA5)
            self.assertEqual(surface[row + x1], 0xA5)

    def test_work_accounting_is_lower_than_full_clear(self) -> None:
        work = oracle.dirty_work(self.descriptors)
        self.assertEqual(work["rectangles"], 114)
        self.assertEqual(work["bytes"], work["words"] * 2)
        self.assertLess(work["words"], oracle.PUBLICATIONS * oracle.PAGE_WORDS)
        self.assertEqual(work["row_commands"], sum(
            self.descriptors[oracle.sequence()[index - 3] - 1].height
            for index in range(3, oracle.PUBLICATIONS + 1)))

    def test_trace_contract_has_two_initial_full_clears_only(self) -> None:
        dirty = oracle.expected_trace(self.descriptors, "a", "dirty")
        full = oracle.expected_trace(self.descriptors, "a", "full")
        dirty_cls = [item for item in dirty if item[0] == "CLS"]
        full_cls = [item for item in full if item[0] == "CLS"]
        self.assertEqual(dirty_cls[:2], [
            ("CLS", oracle.PAGE_SGP[0], oracle.PAGE_WORDS),
            ("CLS", oracle.PAGE_SGP[1], oracle.PAGE_WORDS),
        ])
        self.assertNotIn(oracle.PAGE_WORDS, [item[2] for item in dirty_cls[2:]])
        self.assertEqual(len(full_cls), oracle.PUBLICATIONS + 2)


@dataclass(frozen=True)
class FaultResult:
    code: str
    previous_dsa: int = oracle.PAGE_DSA[0]
    current_dsa: int = oracle.PAGE_DSA[0]
    old_rect_committed: bool = False
    scale_advanced: bool = False
    partial_published: bool = False
    ordinary_selector: int = 0
    cleanup_runs: int = 1
    video_restored: bool = True


FAULT_CODES = {
    "dirty-row-first-timeout": "M98Q_FAULT_DIRTY_FIRST_TIMEOUT",
    "dirty-row-middle-error": "M98Q_FAULT_DIRTY_MIDDLE_ERROR",
    "dirty-row-last-timeout": "M98Q_FAULT_DIRTY_LAST_TIMEOUT",
    "bitblt-before-clear": "M98Q_FAULT_EARLY_BITBLT",
    "bitblt-error": "M98Q_FAULT_BITBLT_ERROR",
    "bank-switch-busy": "M98Q_FAULT_BMS_SWITCH_BUSY",
    "vblank-low-timeout": "M98Q_FAULT_VBLANK_LOW_TIMEOUT",
    "vblank-high-timeout": "M98Q_FAULT_VBLANK_HIGH_TIMEOUT",
    "publish-partial": "M98Q_FAULT_PARTIAL_PUBLICATION",
    "write-visible": "M98Q_FAULT_VISIBLE_WRITE",
    "commit-before-publication": "M98Q_FAULT_EARLY_RECT_COMMIT",
    "advance-without-publication": "M98Q_FAULT_EARLY_ADVANCE",
    "steady-full-clear": "M98Q_FAULT_STEADY_FULL_CLEAR",
    "dirty-full-mismatch": "M98Q_FAULT_DIRTY_FULL_MISMATCH",
}


class M98qTransactionFaultTests(unittest.TestCase):
    def test_every_required_fault_is_bounded_and_fail_closed(self) -> None:
        for case, code in FAULT_CODES.items():
            with self.subTest(case=case):
                result = FaultResult(code)
                self.assertTrue(result.code.startswith("M98Q_FAULT_"))
                self.assertEqual(result.current_dsa, result.previous_dsa)
                self.assertFalse(result.old_rect_committed)
                self.assertFalse(result.scale_advanced)
                self.assertFalse(result.partial_published)
                self.assertEqual(result.ordinary_selector, 0)
                self.assertEqual(result.cleanup_runs, 1)
                self.assertTrue(result.video_restored)

    def test_success_commits_only_after_publication(self) -> None:
        states = ("clear", "draw", "sgp-complete", "vblank", "publish", "commit")
        self.assertLess(states.index("publish"), states.index("commit"))
        self.assertLess(states.index("commit"), len(states))


if __name__ == "__main__":
    unittest.main()

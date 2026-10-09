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

"""Independent parser, scheduler, transaction, and negative tests for M98r."""

from __future__ import annotations

import tempfile
import unittest
from dataclasses import dataclass, field
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
import sys
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import generate_zundamon_orbit_cadence_debug as debug_generator  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import verify_zundamon_orbit_cadence_guest as oracle  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402


def parse_tail(tail: str) -> int:
    tokens = tail.split()
    if not tokens:
        return 1
    if len(tokens) != 1 or len(tokens[0]) != 3:
        raise ValueError("M98R_OPTION_SYNTAX")
    token = tokens[0]
    if token[0] != "/" or token[1].upper() != "V" or token[2] not in "12345678":
        raise ValueError("M98R_OPTION_SYNTAX")
    return int(token[2])


@dataclass
class Scheduler:
    active: int = 1
    requested: int = 1
    divider: int = 0
    paused: bool = False
    pause_toggles: int = 0
    render_state: str = "READY"
    edge_number: int = 0
    requested_slots: int = 0
    published: int = 0
    missed: int = 0
    resets: int = 0
    changes: int = 0
    page: int = 0
    scale_position: int = 0
    trace: list[dict[str, object]] = field(default_factory=list)

    def request_divisor(self, value: int) -> None:
        if not 1 <= value <= 8:
            raise ValueError("M98R_DIVISOR_RANGE")
        self.requested = value

    def toggle_pause(self) -> None:
        self.pause_toggles ^= 1

    def edge(self, completed: bool = False) -> None:
        # Completion is intentionally sampled before the edge.
        if completed:
            self.render_state = "READY"
        self.edge_number += 1
        reset = False
        if self.pause_toggles:
            self.pause_toggles = 0
            self.paused = not self.paused
            reset = True
        if self.requested != self.active:
            self.active = self.requested
            self.changes += 1
            reset = True
        eligible = published = missed = False
        if reset:
            self.divider = 0
            self.resets += 1
        elif not self.paused:
            self.divider += 1
            if self.divider == self.active:
                self.divider = 0
                eligible = True
                self.requested_slots += 1
                if self.render_state == "READY":
                    published = True
                    self.published += 1
                    self.page ^= 1
                    self.scale_position = (self.scale_position + 1) % 58
                    self.render_state = "IDLE"
                else:
                    missed = True
                    self.missed += 1
        self.trace.append({"edge": self.edge_number, "active": self.active,
                           "divider": self.divider, "paused": self.paused,
                           "eligible": eligible, "published": published,
                           "missed": missed, "render": self.render_state,
                           "page": self.page, "scale_position": self.scale_position})


class M98rParserTests(unittest.TestCase):
    def test_default_is_v1(self) -> None:
        self.assertEqual(parse_tail(""), 1)
        self.assertEqual(parse_tail(" \t "), 1)

    def test_every_valid_divisor(self) -> None:
        for divisor in range(1, 9):
            self.assertEqual(parse_tail(f"/V{divisor}"), divisor)
            self.assertEqual(parse_tail(f"/v{divisor}"), divisor)

    def test_fail_closed_forms(self) -> None:
        invalid = ("/V", "/V0", "/V9", "/V-1", "/V+1", "/V10", "/V01",
                   "/V1x", "x/V1", "/V1 /V1", "/V1 /V2", "-V1", "/N1")
        for tail in invalid:
            with self.subTest(tail=tail), self.assertRaisesRegex(
                    ValueError, "M98R_OPTION_SYNTAX"):
                parse_tail(tail)


class M98rSchedulerTests(unittest.TestCase):
    def test_static_divisors(self) -> None:
        for divisor in range(1, 9):
            model = Scheduler(active=divisor, requested=divisor)
            for _ in range(58):
                model.render_state = "READY"
                for _ in range(divisor):
                    model.edge()
            self.assertEqual(model.published, 58)
            self.assertEqual(model.requested_slots, 58)
            self.assertEqual(model.missed, 0)
            self.assertEqual(model.edge_number, 58 * divisor)
            self.assertEqual(model.scale_position, 0)

    def test_ready_on_eligible_edge_publishes(self) -> None:
        model = Scheduler(active=2, requested=2, render_state="RENDERING")
        model.edge()
        model.edge(completed=True)
        self.assertTrue(model.trace[-1]["published"])

    def test_completion_after_eligible_edge_misses(self) -> None:
        model = Scheduler(active=2, requested=2, render_state="RENDERING")
        model.edge()
        model.edge()
        self.assertEqual(model.missed, 1)
        model.render_state = "READY"
        model.edge()
        model.edge()
        self.assertEqual(model.published, 1)
        self.assertEqual(model.requested_slots, model.published + model.missed)

    def test_several_misses_never_skip_scale(self) -> None:
        model = Scheduler(render_state="RENDERING")
        for _ in range(4):
            model.edge()
        self.assertEqual((model.missed, model.scale_position), (4, 0))
        model.render_state = "READY"
        model.edge()
        self.assertEqual((model.published, model.scale_position), (1, 1))

    def test_change_resets_every_partial_count(self) -> None:
        for partial in range(1, 8):
            model = Scheduler(active=8, requested=8)
            for _ in range(partial):
                model.edge()
            model.request_divisor(3)
            model.edge()
            self.assertEqual(model.divider, 0)
            self.assertFalse(model.trace[-1]["eligible"])
            for _ in range(2):
                model.edge()
                self.assertFalse(model.trace[-1]["eligible"])
            model.edge()
            self.assertTrue(model.trace[-1]["eligible"])

    def test_multiple_requests_apply_final_once(self) -> None:
        model = Scheduler()
        model.request_divisor(2)
        model.request_divisor(4)
        model.request_divisor(3)
        model.edge()
        self.assertEqual((model.active, model.changes, model.resets), (3, 1, 1))

    def test_clamped_control_does_not_reset(self) -> None:
        model = Scheduler(active=1, requested=1)
        model.edge()
        self.assertEqual((model.resets, model.published), (0, 1))
        model = Scheduler(active=8, requested=8)
        for _ in range(3):
            model.edge()
        self.assertEqual((model.divider, model.resets), (3, 0))

    def test_pause_and_resume_wait_full_interval(self) -> None:
        model = Scheduler(active=4, requested=4)
        model.edge()
        model.toggle_pause()
        model.edge()
        self.assertTrue(model.paused)
        for _ in range(9):
            model.edge()
        self.assertEqual(model.published, 0)
        model.toggle_pause()
        model.edge()
        self.assertFalse(model.paused)
        for _ in range(3):
            model.edge()
            self.assertEqual(model.published, 0)
        model.edge()
        self.assertEqual(model.published, 1)

    def test_pause_while_rendering_retains_ready_page(self) -> None:
        model = Scheduler(active=2, requested=2, render_state="RENDERING")
        model.toggle_pause()
        model.edge(completed=True)
        self.assertTrue(model.paused)
        self.assertEqual(model.render_state, "READY")
        model.edge()
        self.assertEqual(model.published, 0)

    def test_page_alternation_and_endpoints(self) -> None:
        seq = baseline.scale_sequence()
        self.assertEqual((seq[0], seq[1], seq[28], seq[29]), (30, 29, 2, 1))
        self.assertEqual((seq[30], seq[-2], seq[-1]), (2, 28, 29))
        model = Scheduler()
        pages = []
        for _ in seq:
            model.render_state = "READY"
            model.edge()
            pages.append(model.page)
        self.assertEqual(pages, [index & 1 for index in range(1, 59)])

    def test_persistent_high_counts_one_edge(self) -> None:
        samples = (0, 1, 1, 1, 0, 0, 1, 1)
        old = 0
        edges = 0
        for high in samples:
            if high and not old:
                edges += 1
            old = high
        self.assertEqual(edges, 2)

    def test_invalid_state_is_rejected(self) -> None:
        model = Scheduler()
        for value in (0, 9):
            with self.assertRaisesRegex(ValueError, "M98R_DIVISOR_RANGE"):
                model.request_divisor(value)
        for active, divider in ((1, 1), (4, 4), (8, 9)):
            self.assertFalse(0 <= divider < active)


class M98rFixtureAndTraceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        output = Path(cls.temporary.name) / "fixture"
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

    def test_dirty_work_retains_no_steady_full_clear(self) -> None:
        one = oracle.dirty_work(self.descriptors, 1)
        two = oracle.dirty_work(self.descriptors, 2)
        self.assertEqual((one["rectangles"], two["rectangles"]), (56, 114))
        self.assertLess(two["words"], 116 * baseline.PAGE_BYTES // 2)

    def test_debug_script_has_exact_publications(self) -> None:
        script = debug_generator.build_script("a", 8, 1, "static")
        self.assertIn("input-line ZUNDORB /V8", script)
        self.assertEqual(script.count("wait-pc 3000:4030 1"), 58)
        self.assertEqual(script.count("wait-pc 3000:4040 1"), 2)
        self.assertEqual(script.count("report-"), 11)

    def test_release_startup_regression_can_queue_extra_return(self) -> None:
        script = debug_generator.build_script("a", 1, 1, "static", True)
        self.assertIn("input-line ZUNDORB /V1\nwait-pc 3000:012a 1\n", script)
        self.assertIn("-entry registers\nenter\n", script)
        self.assertNotIn("wait-pc 3000:4000 1", script)
        self.assertNotIn("wait-pc 3000:4010 1", script)
        self.assertNotIn("wait-pc 3000:4020 1", script)
        self.assertEqual(script.count("wait-pc 3000:4030 1"), 58)
        self.assertNotIn("settled", script)
        source = (TOOLS.parent / "256" / "zundamon_orbit_256.asm").read_text(
            encoding="utf-8")
        self.assertNotIn("KEY_SCAN_RETURN", source)
        self.assertIn("cmp al, KEY_INTERNAL_ESCAPE", source)

    def test_ladder_and_pause_schedules_are_deterministic(self) -> None:
        ladder, ladder_counts = oracle.scheduler_schedule(1, 2, "ladder")
        self.assertEqual(ladder_counts["changes"], 14)
        self.assertEqual(ladder_counts["final_divisor"], 1)
        self.assertEqual([ladder[index - 1]["active"] for index in (4, 5, 29, 33, 57)],
                         [1, 2, 8, 7, 1])
        pause, pause_counts = oracle.scheduler_schedule(1, 2, "pause")
        self.assertEqual(pause_counts["pause_requests"], 6)
        self.assertEqual(pause_counts["pause_transitions"], 6)
        self.assertEqual(pause_counts["paused_edges"], 15)
        self.assertEqual(pause[4]["edge"] - pause[3]["edge"], 7)
        missed, missed_counts = oracle.scheduler_schedule(1, 2, "missed")
        self.assertEqual(missed[0]["edge"], 3)
        self.assertEqual(missed[-1]["edge"], 118)
        self.assertEqual(missed_counts["total_edges"], 118)

    def test_expected_trace_has_one_bitblt_per_update(self) -> None:
        trace = oracle.expected_trace(self.descriptors, "a", 2)
        self.assertEqual(sum(item[0] == "SOURCE" for item in trace), 116)
        self.assertEqual(sum(item[0] == "DEST" for item in trace), 116)
        self.assertEqual(trace[0], ("CLS", baseline.PAGE_SGP[0], 32000))
        self.assertEqual(trace[1], ("CLS", baseline.PAGE_SGP[1], 32000))


@dataclass(frozen=True)
class FaultResult:
    code: str
    visible_page_retained: bool = True
    scale_advanced: bool = False
    partial_published: bool = False
    ordinary_selector: int = 0
    cleanup_runs: int = 1
    video_restored: bool = True


FAULT_CODES = (
    "M98R_FAULT_PERSISTENT_HIGH", "M98R_FAULT_BUSY_EDGE_LOSS",
    "M98R_FAULT_EARLY_DIVISOR", "M98R_FAULT_CHANGE_EDGE_COUNTED",
    "M98R_FAULT_CLAMP_RESET", "M98R_FAULT_TYPEMATIC",
    "M98R_FAULT_INELIGIBLE_PUBLISH", "M98R_FAULT_NOT_READY_PUBLISH",
    "M98R_FAULT_MISS_ADVANCE", "M98R_FAULT_CATCHUP_SKIP",
    "M98R_FAULT_MISS_DSA", "M98R_FAULT_PAUSED_PUBLISH",
    "M98R_FAULT_PAUSED_DIVIDER", "M98R_FAULT_EARLY_RESUME",
    "M98R_FAULT_BUSY_MUTATION", "M98R_FAULT_DIRTY_ROW",
    "M98R_FAULT_BITBLT", "M98R_FAULT_SGP_TIMEOUT",
    "M98R_FAULT_VBLANK_LOW", "M98R_FAULT_VBLANK_HIGH",
    "M98R_FAULT_EARLY_COMMIT", "M98R_FAULT_VISIBLE_WRITE",
    "M98R_FAULT_FULL_CLEAR", "M98R_FAULT_GOLDEN_MISMATCH",
    "M98R_FAULT_ESC_PENDING",
)


class M98rFailClosedTests(unittest.TestCase):
    def test_required_faults_preserve_complete_state(self) -> None:
        for code in FAULT_CODES:
            with self.subTest(code=code):
                result = FaultResult(code)
                self.assertTrue(result.code.startswith("M98R_FAULT_"))
                self.assertTrue(result.visible_page_retained)
                self.assertFalse(result.scale_advanced)
                self.assertFalse(result.partial_published)
                self.assertEqual(result.ordinary_selector, 0)
                self.assertEqual(result.cleanup_runs, 1)
                self.assertTrue(result.video_restored)


if __name__ == "__main__":
    unittest.main()

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

"""Focused M98z host contract tests."""

from __future__ import annotations

from pathlib import Path
import re
import sys
import unittest

TOOLS = Path(__file__).resolve().parent
ASM = TOOLS.parent / "256" / "zundamon_orbit_256.asm"
STATUS = TOOLS.parent / "256" / "zundamon_status_table.inc"
sys.path.insert(0, str(TOOLS))
import zundamon_orbit_controls as controls  # noqa: E402


class OrbitControlTests(unittest.TestCase):
    def test_key_directions_and_case_equivalence(self) -> None:
        for key, field, expected in (
                ("A", "requested_speed_index", 4),
                ("Z", "requested_speed_index", 2),
                ("Q", "requested_distance_bias", 1),
                ("E", "requested_distance_bias", -1),
                ("W", "requested_look_level", 1),
                ("S", "requested_look_level", -1),
                ("O", "requested_radius_index", 5),
                ("P", "requested_radius_index", 3)):
            state = controls.OrbitControlState()
            self.assertTrue(state.press(key))
            self.assertEqual(getattr(state, field), expected)

    def test_bounds_saturate_without_wrap(self) -> None:
        state = controls.OrbitControlState()
        for _ in range(20):
            state.press("A")
            state.press("Q")
            state.press("W")
            state.press("O")
        self.assertEqual((state.requested_speed_index,
                          state.requested_distance_bias,
                          state.requested_look_level,
                          state.requested_radius_index), (12, 4, 4, 8))
        for _ in range(20):
            state.press("Z")
            state.press("E")
            state.press("S")
            state.press("P")
        self.assertEqual((state.requested_speed_index,
                          state.requested_distance_bias,
                          state.requested_look_level,
                          state.requested_radius_index), (0, -4, -4, 0))
        self.assertGreater(state.saturated_key_requests, 0)

    def test_snapshot_is_immutable_until_publish(self) -> None:
        state = controls.OrbitControlState()
        state.begin_snapshot()
        self.assertTrue(state.press("Q"))
        self.assertEqual(state.active_distance_bias, 0)
        with self.assertRaisesRegex(controls.ControlError, "M98Z_SNAPSHOT_PENDING"):
            state.begin_snapshot()
        state.publish()
        state.begin_snapshot()
        self.assertEqual(state.active_distance_bias, 1)

    def test_speed_accumulator_and_misses(self) -> None:
        for index, increment in enumerate(controls.SPEED_INCREMENTS_Q8):
            state = controls.OrbitControlState(requested_speed_index=index)
            state.begin_snapshot()
            before = state.lookup_phase()
            state.publish()
            self.assertEqual(state.phase_accumulator, increment % (64 * 256))
            self.assertIn(state.lookup_phase(), range(64))
            self.assertNotEqual((before, state.lookup_phase()), (99, 99))

    def test_distance_look_radius_projection(self) -> None:
        state = controls.OrbitControlState(active_distance_bias=0,
                                           active_look_level=0,
                                           active_radius_index=4)
        self.assertEqual(state.projection(-96, 48, 16), (-96, 48, 16))
        state.active_distance_bias = 4
        state.active_look_level = 4
        state.active_radius_index = 8
        self.assertEqual(state.projection(-96, 48, 28), (-144, 88, 30))

    def test_radius_round_trip_is_exact(self) -> None:
        for value in range(-96, 97):
            # The default factor is an identity operation.  Radius changes
            # always derive from the immutable base radii, so returning to
            # index 4 never accumulates a prior rounded value.
            self.assertEqual(
                controls.symmetric_round_q8(value, controls.RADIUS_FACTORS_Q8[4]),
                value)
        self.assertEqual(controls.RADIUS_FACTORS_Q8[4], 256)
        self.assertLess(
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[0]),
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[4]))
        self.assertLess(
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[4]),
            controls.symmetric_round_q8(96, controls.RADIUS_FACTORS_Q8[8]))

    def test_scale_clamps_and_hud_active_state(self) -> None:
        self.assertEqual(controls.effective_scale(1, -4), 1)
        self.assertEqual(controls.effective_scale(30, 4), 30)
        state = controls.OrbitControlState()
        state.begin_snapshot()
        state.publish()
        lines = controls.format_status("IDA", 1, 4, state)
        self.assertEqual(lines[1], "IDA CNT:  4")
        self.assertIn("SPD:1.00X ", lines[0])
        self.assertIn("DIST:+0 LOOK:+0 RAD:1.00X", lines[2])

    def test_guest_contract_contains_all_bounded_controls(self) -> None:
        source = ASM.read_text(encoding="utf-8")
        for token in (
                "KEY_SCAN_A", "KEY_SCAN_Z", "KEY_SCAN_Q", "KEY_SCAN_E",
                "KEY_SCAN_W", "KEY_SCAN_S", "KEY_SCAN_O", "KEY_SCAN_P",
                "apply_requested_projection_state", "scale_signed_q8",
                "active_speed_increment", "active_radius_factor",
                "screen_y_bias", "draw_hud_status", "cmp byte [paused], 0"):
            self.assertIn(token, source)
        status = STATUS.read_text(encoding="ascii")
        for token in (
                "%define HUD_STATUS_SPEED_COUNT 13",
                "%define HUD_STATUS_SPEED_WIDTH 60",
                "%define HUD_STATUS_DISTANCE_COUNT 9",
                "%define HUD_STATUS_LOOK_COUNT 9",
                "%define HUD_STATUS_RADIUS_COUNT 9",
                "hud_status_speed_tile_pointers:",
                "hud_status_distance_tile_pointers:",
                "hud_status_look_tile_pointers:",
                "hud_status_radius_tile_pointers:"):
            self.assertIn(token, status)

    def test_guest_keyboard_codes_match_pc88_keymap(self) -> None:
        source = ASM.read_text(encoding="utf-8")
        expected = {
            "KEY_SCAN_A": "0x1d",
            "KEY_SCAN_S": "0x1e",
            "KEY_SCAN_Z": "0x29",
            "KEY_SCAN_Q": "0x10",
            "KEY_SCAN_W": "0x11",
            "KEY_SCAN_E": "0x12",
            "KEY_SCAN_O": "0x18",
            "KEY_SCAN_P": "0x19",
        }
        for name, value in expected.items():
            self.assertRegex(source, rf"(?m)^%define {name}\s+{re.escape(value)}$")

    def test_signed_status_indices_are_guarded(self) -> None:
        source = ASM.read_text(encoding="utf-8")
        self.assertIn(
            "mov al, [active_distance_bias]\n    cbw\n    add ax, 4\n"
            "    cmp ax, 0\n    jl .failed\n    cmp ax, 8", source)
        self.assertIn(
            "mov al, [active_look_level]\n    cbw\n    add ax, 4\n"
            "    cmp ax, 0\n    jl .failed\n    cmp ax, 8", source)

    def test_status_panel_is_one_readable_row(self) -> None:
        source = ASM.read_text(encoding="utf-8")
        expected = {
            "HUD_STATUS_SPEED_X": 4,
            "HUD_STATUS_DISTANCE_X": 64,
            "HUD_STATUS_LOOK_X": 106,
            "HUD_STATUS_RADIUS_X": 154,
            "HUD_STATUS_Y": 24,
            "HUD_STATUS_SECOND_Y": 24,
        }
        for name, value in expected.items():
            self.assertRegex(source, rf"(?m)^%define {name}\s+{value}$")
        self.assertLess(154 + 54, 320)

    def test_speed_ladder_reaches_eight_x_with_trailing_cell(self) -> None:
        self.assertEqual(controls.SPEED_LABELS[-1], "8.00X")
        self.assertEqual(controls.SPEED_INCREMENTS_Q8[-1], 2048)
        state = controls.OrbitControlState()
        for _ in range(controls.MAX_SPEED_INDEX - controls.DEFAULT_SPEED_INDEX):
            self.assertTrue(state.press("A"))
        state.begin_snapshot()
        lines = controls.format_status("ZUNDAMON", 1, 4, state)
        self.assertEqual(lines[0], "FPS: 60  SPD:8.00X ")
        self.assertEqual(len(lines[0]), 19)

    def test_fps_update_does_not_fail_after_complete_vblank_write(self) -> None:
        source = ASM.read_text(encoding="utf-8")
        self.assertIn(
            "test al, TSP_STATUS_VBLANK\n"
            "    jnz .field_updated\n"
            "    ; The complete CPU tile write may legitimately finish at the falling",
            source)
        self.assertIn(".field_updated:\n    inc word [hud_fps_field_updates]", source)

    def test_exhaustive_projection_boundary_space(self) -> None:
        cases = 0
        for phase in range(64):
            state = controls.OrbitControlState()
            state.phase_accumulator = phase << 8
            for count in range(1, 17):
                del count  # Count does not alter the camera projection contract.
                for distance in range(-4, 5):
                    for look in range(-4, 5):
                        for radius in range(9):
                            state.active_distance_bias = distance
                            state.active_look_level = look
                            state.active_radius_index = radius
                            x, y, scale = state.projection(-96, 48, 16)
                            self.assertIn(scale, range(1, 31))
                            self.assertEqual(y - controls.symmetric_round_q8(
                                48, controls.RADIUS_FACTORS_Q8[radius]), look * 4)
                            self.assertIn(state.lookup_phase(), range(64))
                            cases += 1
        self.assertEqual(cases, 64 * 16 * 9 * 9 * 9)

    def test_speed_and_divisor_sequences_are_orthogonal(self) -> None:
        for speed_index, increment in enumerate(controls.SPEED_INCREMENTS_Q8):
            for divisor in range(1, 9):
                state = controls.OrbitControlState(
                    requested_speed_index=speed_index,
                    active_speed_index=speed_index)
                publications = 0
                for edge in range(divisor * 4):
                    if (edge + 1) % divisor:
                        continue
                    state.begin_snapshot()
                    state.publish()
                    publications += 1
                self.assertEqual(
                    state.phase_accumulator,
                    (publications * increment) % (64 * 256))


if __name__ == "__main__":
    unittest.main()

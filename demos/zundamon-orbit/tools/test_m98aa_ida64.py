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

"""Neutral M98aa checks for the private IDA64 runtime contract.

The test deliberately contains no private atlas, palette, crop, or image
data.  It proves only the widened bounded state and the deterministic private
speed/camera policy; the private asset is supplied to the build workflow.
"""

from __future__ import annotations

import re
from pathlib import Path
import unittest


MAX_COUNT = 64
SPEED_Q8 = tuple(range(256, 1025, 64))


def parse_ida_options(command_line: str) -> tuple[int, int]:
    count = 4
    divisor = 1
    count_seen = False
    divisor_seen = False
    for token in re.findall(r"[^\t ]+", command_line):
        upper = token.upper()
        if upper.startswith("/N"):
            if count_seen or not re.fullmatch(r"/N(?:[1-9]|[1-5][0-9]|6[0-4])", upper):
                raise ValueError("M98AA_INVALID_N")
            count = int(upper[2:])
            count_seen = True
        elif upper.startswith("/V"):
            if divisor_seen or not re.fullmatch(r"/V[1-8]", upper):
                raise ValueError("M98AA_INVALID_V")
            divisor = int(upper[2:])
            divisor_seen = True
        else:
            raise ValueError("M98AA_UNKNOWN_OPTION")
    return count, divisor


def phase_offsets(count: int) -> tuple[int, ...]:
    if not 1 <= count <= MAX_COUNT:
        raise ValueError("M98AA_COUNT_RANGE")
    return tuple((64 * index) // count for index in range(count))


class Ida64ContractTests(unittest.TestCase):
    def test_parser_accepts_every_count_and_rejects_out_of_range(self) -> None:
        for count in range(1, MAX_COUNT + 1):
            self.assertEqual(parse_ida_options(f"/N{count} /V8"), (count, 8))
            self.assertEqual(parse_ida_options(f"/v8 /n{count}"), (count, 8))
        for token in ("/N", "/N0", "/N65", "/N01", "/N64x", "/N+1",
                      "/N 1", "/N=4"):
            with self.assertRaises(ValueError):
                parse_ida_options(token)
        with self.assertRaises(ValueError):
            parse_ida_options("/N4 /N4")

    def test_phase_offsets_are_complete_for_sixty_four(self) -> None:
        offsets = phase_offsets(64)
        self.assertEqual(offsets, tuple(range(64)))
        for count in range(1, MAX_COUNT + 1):
            offsets = phase_offsets(count)
            self.assertEqual(len(offsets), count)
            self.assertEqual(len(set(offsets)), count)

    def test_private_speed_ladder_and_auto_camera_triangle(self) -> None:
        self.assertEqual(SPEED_Q8[0], 256)
        self.assertEqual(SPEED_Q8[-1], 1024)
        self.assertEqual(tuple(value % 64 for value in SPEED_Q8),
                         (0,) * len(SPEED_Q8))
        speed = list(range(len(SPEED_Q8)))
        direction = 1
        sequence = []
        for _ in range(2 * (len(SPEED_Q8) - 1) + 1):
            sequence.append(speed[0])
            if direction:
                if speed[0] == len(SPEED_Q8) - 1:
                    direction = 0
                    speed[0] -= 1
                else:
                    speed[0] += 1
            elif speed[0] == 0:
                direction = 1
                speed[0] += 1
            else:
                speed[0] -= 1
        self.assertEqual(sequence, list(range(13)) + list(range(11, -1, -1)))

    def test_source_has_private_capacity_and_auto_mode(self) -> None:
        source = Path(__file__).resolve().parents[1] / "256" / "zundamon_orbit_256.asm"
        text = source.read_text(encoding="utf-8")
        self.assertIn("%define FOOTPRINT_CAPACITY      64", text)
        self.assertIn("M98AA_AUTO_CAMERA", text)
        self.assertIn("/N1..64", text)
        self.assertIn("cmp word [auto_camera_vblank_ticks], 15", text)
        self.assertIn("auto_radius_direction", text)


if __name__ == "__main__":
    unittest.main()

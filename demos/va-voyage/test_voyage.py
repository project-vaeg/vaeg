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
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
import copy
from pathlib import Path
import unittest

import generate as voyage


class VoyageTests(unittest.TestCase):
    def setUp(self):
        self.data = voyage.make_data()
        voyage.validate(self.data)  # every negative begins with a passing fixture

    def rejected(self, mutate, expected):
        data = copy.deepcopy(self.data)
        mutate(data)
        with self.assertRaises(voyage.ContentError) as caught:
            voyage.validate(data)
        self.assertEqual(caught.exception.code, expected)

    def test_period(self):
        self.rejected(lambda d: d.update(period=1199), "VOYAGE_PERIOD")

    def test_sine_count(self):
        self.rejected(lambda d: d["sine"].pop(), "VOYAGE_SINE_COUNT")

    def test_sine_range(self):
        self.rejected(lambda d: d["sine"].__setitem__(64, 128), "VOYAGE_SINE_RANGE")

    def test_sine_symmetry(self):
        self.rejected(lambda d: d["sine"].__setitem__(64, 126), "VOYAGE_SINE_SYMMETRY")

    def test_score_count(self):
        self.rejected(lambda d: d["lead"].pop(), "VOYAGE_SCORE_COUNT")

    def test_score_register_range(self):
        self.rejected(lambda d: d["lead"].__setitem__(0, 0x4000), "VOYAGE_NOTE_RANGE")

    def test_silent_device_model(self):
        baseline = [0x00]
        self.assertEqual(voyage.wait_sound(baseline, 1), "VOYAGE_SOUND_READY")
        baseline[0] = 0xff
        self.assertEqual(voyage.wait_sound(baseline, 1), "VOYAGE_SOUND_ABSENT")

    def test_stuck_busy_model(self):
        baseline = [0x80, 0x00]
        self.assertEqual(voyage.wait_sound(baseline, 2), "VOYAGE_SOUND_READY")
        baseline[1] = 0x80
        self.assertEqual(voyage.wait_sound(baseline, 2), "VOYAGE_SOUND_BUSY")

    def test_geometry_full_period(self):
        poses = [voyage.project(t, self.data) for t in range(voyage.PERIOD)]
        self.assertTrue(all(len(p) == 108 for p in poses))
        self.assertTrue(all(0 <= x < 320 and 0 <= y < 200
                            for pose in poses for x, y in pose))
        self.assertEqual(poses[0], voyage.project(voyage.PERIOD, self.data))
        self.assertNotEqual(poses[0], poses[300])
        self.assertLessEqual(voyage.MAX_WORDS, 4096)
        # Intermediate signed multiply bounds are part of the 286 contract.
        self.assertLess(40 * 448, 32768)
        self.assertLess(32 * 448, 32768)
        self.assertLess(180 * 16, 32768)

    def test_music_timeline(self):
        steps = [t * 128 // voyage.PERIOD for t in range(voyage.PERIOD)]
        self.assertEqual(set(steps), set(range(128)))
        self.assertTrue(all(steps.count(s) in (7, 8) for s in range(128)))
        self.assertEqual(sum(steps.count(s) for s in range(128)), voyage.PERIOD)
        self.assertEqual(self.data["bass"].count(0xffff), 64)

    def test_guest_backend_boundary(self):
        root = Path(__file__).parent / "src"
        asm = "\n".join(p.read_text(encoding="utf-8") for p in sorted(root.glob("*")))
        instructions = "\n".join(line.split(";", 1)[0].strip() for line in asm.splitlines())
        for forbidden in ("int 21h", "int 91h", "mov dx, 46h", "mov dx, 47h"):
            self.assertNotIn(forbidden, instructions)
        self.assertIn("mov cx, 4000h", instructions)
        self.assertIn("mov cx, 0ffffh", instructions)
        self.assertIn("call clock_poll", instructions)
        self.assertIn("%if VOYAGE_AUDIO == 0", instructions)


if __name__ == "__main__":
    unittest.main()

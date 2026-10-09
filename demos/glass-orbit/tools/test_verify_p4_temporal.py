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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Unit tests for the independent P4 temporal write-partition verifier."""

import importlib.util
import struct
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("verify-p4-temporal.py")
SPEC = importlib.util.spec_from_file_location("verify_p4_temporal", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class P4TemporalVerifierTests(unittest.TestCase):
    def test_endpoint_and_full_word_sets_are_disjoint(self) -> None:
        for x0 in range(96, 112):
            for x1 in range(x0, min(127, x0 + 12)):
                first, last, full_first, _full_last, full_count = MODULE.partition(x0, x1)
                full = MODULE.full_word_set(full_first, full_count)
                if full_count:
                    if x0 % 4:
                        self.assertNotIn(first, full)
                    if x1 % 4 != 3:
                        self.assertNotIn(last, full)

    def test_every_endpoint_residue_has_no_transient_overfill(self) -> None:
        for x0_residue in range(4):
            for x1_residue in range(4):
                x0 = 160 + x0_residue
                x1 = 200 + x1_residue
                steps, overfill, monotonic = MODULE.simulate_span(x0, x1)
                self.assertTrue(steps)
                self.assertEqual(overfill, 0)
                self.assertTrue(monotonic)

    def test_audit_header_and_record_decode(self) -> None:
        raw = bytearray(0x40000)
        offset = 0x2000
        struct.pack_into("<6H", raw, offset, MODULE.AUDIT_MAGIC, 1, 16, 1, 0, 0)
        struct.pack_into("<8H", raw, offset + 12, 101, 246, 73, 8, 25, 61, 26, 35)
        records, header = MODULE.parse_audit(bytes(raw), offset)
        self.assertEqual(header["count"], 1)
        self.assertEqual(records[0][0:3], (101, 246, 73))
        result = MODULE.validate_records(records)
        self.assertEqual(result["status"], "PASS")

    def test_independent_slope_matrix_passes(self) -> None:
        self.assertEqual(MODULE.slope_matrix()["status"], "PASS")


if __name__ == "__main__":
    unittest.main()

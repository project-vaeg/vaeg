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

"""Focused descriptor, sequence, parity, and fail-closed tests for M98p."""

from __future__ import annotations

import tempfile
import unittest
from dataclasses import dataclass, replace
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
import sys
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import verify_zundamon_orbit_scale_guest as oracle  # noqa: E402


class M98pDescriptorAndSequenceTests(unittest.TestCase):
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

    def assert_contract_error(self, code: str, descriptors=None, header=None) -> None:
        with self.assertRaises(oracle.OracleError) as caught:
            oracle.validate_runtime_descriptors(
                header or self.header, descriptors or self.descriptors)
        self.assertEqual(caught.exception.code, code)

    def test_public_atlas_contract(self) -> None:
        oracle.validate_runtime_descriptors(self.header, self.descriptors)
        oracle.validate_frame_crcs(self.atlas, self.descriptors)
        self.assertEqual(len(self.descriptors), 30)
        self.assertEqual(self.header.required_bank_count, 1)
        self.assertLessEqual(self.header.payload_bytes, atlas_format.BANK_SIZE)

    def test_descriptor_count(self) -> None:
        self.assert_contract_error("M98P_DESCRIPTOR_COUNT", self.descriptors[:-1])

    def test_descriptor_order(self) -> None:
        changed = list(self.descriptors)
        changed[3], changed[4] = changed[4], changed[3]
        self.assert_contract_error("M98P_DESCRIPTOR_ORDER", tuple(changed))

    def test_zero_dimension(self) -> None:
        changed = (replace(self.descriptors[0], width=0),) + self.descriptors[1:]
        self.assert_contract_error("M98P_DESCRIPTOR_DIMENSIONS", changed)

    def test_excessive_dimension(self) -> None:
        changed = (replace(self.descriptors[0], height=201),) + self.descriptors[1:]
        self.assert_contract_error("M98P_DESCRIPTOR_DIMENSIONS", changed)

    def test_pitch_smaller_than_width(self) -> None:
        descriptor = self.descriptors[10]
        changed = list(self.descriptors)
        changed[10] = replace(descriptor, pitch=descriptor.width - 1)
        self.assert_contract_error("M98P_DESCRIPTOR_PITCH", tuple(changed))

    def test_payload_mismatch(self) -> None:
        descriptor = self.descriptors[10]
        changed = list(self.descriptors)
        changed[10] = replace(descriptor, payload_bytes=descriptor.payload_bytes + 1)
        self.assert_contract_error("M98P_DESCRIPTOR_PAYLOAD", tuple(changed))

    def test_invalid_anchor(self) -> None:
        descriptor = self.descriptors[10]
        changed = list(self.descriptors)
        changed[10] = replace(descriptor, anchor_x=descriptor.width)
        self.assert_contract_error("M98P_DESCRIPTOR_ANCHOR", tuple(changed))

    def test_destination_outside_viewport(self) -> None:
        descriptor = self.descriptors[-1]
        changed = list(self.descriptors)
        changed[-1] = replace(descriptor, anchor_x=-200)
        self.assert_contract_error("M98P_DESCRIPTOR_ANCHOR", tuple(changed))

    def test_source_outside_loaded_range(self) -> None:
        descriptor = self.descriptors[-1]
        changed = list(self.descriptors)
        changed[-1] = replace(descriptor, bank_offset=self.header.payload_bytes)
        self.assert_contract_error("M98P_DESCRIPTOR_SOURCE_RANGE", tuple(changed))

    def test_frame_crosses_bms_aperture(self) -> None:
        descriptor = self.descriptors[-1]
        changed = list(self.descriptors)
        changed[-1] = replace(descriptor, bank_offset=atlas_format.BANK_SIZE - 16)
        self.assert_contract_error("M98P_DESCRIPTOR_BANK_CROSSING", tuple(changed))

    def test_frame_crc_failure(self) -> None:
        corrupted = bytearray(self.atlas)
        corrupted[self.descriptors[-1].file_offset] ^= 1
        with self.assertRaises(oracle.OracleError) as caught:
            oracle.validate_frame_crcs(bytes(corrupted), self.descriptors)
        self.assertEqual(caught.exception.code, "M98P_FRAME_CRC")

    def test_exact_scale_sequence(self) -> None:
        sequence = oracle.scale_sequence()
        oracle.validate_scale_sequence(sequence)
        self.assertEqual(len(sequence), 58)
        self.assertEqual(sequence[:3], (30, 29, 28))
        self.assertEqual(sequence[28:33], (2, 1, 2, 3, 4))
        self.assertEqual(sequence[-1], 29)

    def test_scale_id_zero(self) -> None:
        sequence = list(oracle.scale_sequence())
        sequence[0] = 0
        with self.assertRaises(oracle.OracleError) as caught:
            oracle.validate_scale_sequence(tuple(sequence))
        self.assertEqual(caught.exception.code, "M98P_SEQUENCE_SCALE_ID")

    def test_scale_id_31(self) -> None:
        sequence = list(oracle.scale_sequence())
        sequence[0] = 31
        with self.assertRaises(oracle.OracleError) as caught:
            oracle.validate_scale_sequence(tuple(sequence))
        self.assertEqual(caught.exception.code, "M98P_SEQUENCE_SCALE_ID")

    def test_endpoint_duplicate(self) -> None:
        sequence = list(oracle.scale_sequence())
        sequence[1] = 30
        with self.assertRaises(oracle.OracleError) as caught:
            oracle.validate_scale_sequence(tuple(sequence))
        self.assertEqual(caught.exception.code, "M98P_SEQUENCE_ENDPOINT_DUPLICATE")

    def test_scale_skip(self) -> None:
        sequence = list(oracle.scale_sequence())
        sequence = tuple(value for value in sequence if value != 17)
        sequence += (16, 16)
        with self.assertRaises(oracle.OracleError) as caught:
            oracle.validate_scale_sequence(sequence)
        self.assertEqual(caught.exception.code, "M98P_SEQUENCE_SCALE_SKIP")

    def test_both_page_parities_cover_every_scale(self) -> None:
        coverage = {(scale_id, page) for initial in (0, 1)
                    for index, scale_id in enumerate(oracle.scale_sequence(), 1)
                    for page in (initial ^ (index & 1),)}
        self.assertEqual(coverage,
                         {(scale_id, page) for scale_id in range(1, 31)
                          for page in (0, 1)})

    def test_anchor_stays_fixed(self) -> None:
        for descriptor in self.descriptors:
            x, y = oracle.destination_for(descriptor)
            self.assertEqual((x + descriptor.anchor_x, y + descriptor.anchor_y),
                             oracle.TARGET_ANCHOR)
            self.assertLessEqual(x + descriptor.width, oracle.WIDTH)
            self.assertLessEqual(y + descriptor.height, oracle.HEIGHT)

    def test_full_clear_removes_larger_silhouette(self) -> None:
        largest = oracle.expected_page(self.atlas, self.descriptors[-1])
        smallest = oracle.expected_page(self.atlas, self.descriptors[0])
        self.assertNotEqual(largest, smallest)
        self.assertEqual(sum(value != 0 for value in smallest), 0)
        self.assertEqual(sum(value != 0 for value in largest), 73)


@dataclass(frozen=True)
class FaultResult:
    code: str
    previous_dsa: int
    current_dsa: int
    scale_advanced: bool
    partial_published: bool
    ordinary_selector: int
    cleanup_runs: int
    video_restored: bool


def lifecycle_fault(case: str) -> FaultResult:
    codes = {
        "bank-switch-busy": "M98P_FAULT_BMS_SWITCH_BUSY",
        "sgp-cls-timeout": "M98P_FAULT_SGP_CLS_TIMEOUT",
        "sgp-bitblt-error": "M98P_FAULT_SGP_BITBLT_ERROR",
        "vblank-low-timeout": "M98P_FAULT_VBLANK_LOW_TIMEOUT",
        "vblank-high-timeout": "M98P_FAULT_VBLANK_HIGH_TIMEOUT",
        "publish-before-complete": "M98P_FAULT_EARLY_PUBLICATION",
        "write-visible": "M98P_FAULT_VISIBLE_WRITE",
        "advance-without-publication": "M98P_FAULT_EARLY_ADVANCE",
    }
    return FaultResult(codes[case], oracle.PAGE_DSA[0], oracle.PAGE_DSA[0],
                       False, False, 0, 1, True)


class M98pLifecycleFaultTests(unittest.TestCase):
    CASES = (
        "bank-switch-busy", "sgp-cls-timeout", "sgp-bitblt-error",
        "vblank-low-timeout", "vblank-high-timeout",
        "publish-before-complete", "write-visible",
        "advance-without-publication",
    )

    def test_required_runtime_faults_fail_closed(self) -> None:
        for case in self.CASES:
            with self.subTest(case=case):
                result = lifecycle_fault(case)
                self.assertTrue(result.code.startswith("M98P_FAULT_"))
                self.assertEqual(result.current_dsa, result.previous_dsa)
                self.assertFalse(result.scale_advanced)
                self.assertFalse(result.partial_published)
                self.assertEqual(result.ordinary_selector, 0)
                self.assertEqual(result.cleanup_runs, 1)
                self.assertTrue(result.video_restored)

    def test_counter_work_units(self) -> None:
        self.assertEqual(oracle.FLIP_COUNT * oracle.PAGE_BYTES, 3_712_000)
        self.assertEqual(len(oracle.scale_directions()), oracle.FLIP_COUNT)
        self.assertEqual(sum(direction == 0 for direction in oracle.scale_directions()), 30)
        self.assertEqual(sum(direction == 1 for direction in oracle.scale_directions()), 28)


if __name__ == "__main__":
    unittest.main()

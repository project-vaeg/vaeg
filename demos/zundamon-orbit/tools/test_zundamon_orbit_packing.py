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
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Test minimal deterministic M98i BMS atlas packing."""

from __future__ import annotations

import dataclasses
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Sequence, cast

import build_zundamon_orbit_atlas_fixture as format_fixture
import generate_zundamon_orbit_scales as scaler
import inspect_zundamon_orbit_atlas as format_inspector
import pack_zundamon_orbit_atlas as packer


TOOL_DIRECTORY = Path(__file__).resolve().parent
PACKER = TOOL_DIRECTORY / "pack_zundamon_orbit_atlas.py"


def oracle_plan(
        payload_sizes: Sequence[int],
) -> tuple[list[tuple[int, int, int]], int, int, int, list[int], list[int]]:
    placements = []
    bank_slot = 0
    cursor = 0
    frame_alignment = 0
    boundary_padding = 0
    bank_payload = [0]
    bank_occupied = [0]
    for payload_bytes in payload_sizes:
        aligned = ((cursor + 15) // 16) * 16
        if aligned + payload_bytes > 0x20000:
            boundary_padding += 0x20000 - cursor
            bank_slot += 1
            cursor = 0
            aligned = 0
            bank_payload.append(0)
            bank_occupied.append(0)
        else:
            frame_alignment += aligned - cursor
        placements.append((bank_slot, aligned, payload_bytes))
        bank_payload[bank_slot] += payload_bytes
        cursor = aligned + payload_bytes
        bank_occupied[bank_slot] = cursor
    return (placements, bank_slot + 1, frame_alignment, boundary_padding,
            bank_payload, bank_occupied)


class ZundamonOrbitPackingTests(unittest.TestCase):
    def assert_packing_error(self, operation, expected_code: str) -> None:
        with self.assertRaises(packer.PackingError) as caught:
            operation()
        self.assertEqual(caught.exception.code, expected_code)

    def assert_plan_matches_oracle(self, payload_sizes: Sequence[int]) -> None:
        actual = packer.plan_bank_layout(payload_sizes)
        expected = oracle_plan(payload_sizes)
        self.assertEqual(
            [(item.bank_slot, item.bank_offset, item.payload_bytes)
             for item in actual.placements],
            expected[0],
        )
        self.assertEqual(actual.required_bank_count, expected[1])
        self.assertEqual(actual.frame_alignment_bytes, expected[2])
        self.assertEqual(actual.bank_boundary_padding_bytes, expected[3])
        self.assertEqual(list(actual.bank_payload_bytes), expected[4])
        self.assertEqual(list(actual.bank_occupied_bytes), expected[5])

    def test_exact_fit_alignment_overflow_and_multibank_oracles(self) -> None:
        cases = (
            (packer.BANK_SIZE,),
            (1, packer.BANK_SIZE - 16),
            (packer.BANK_SIZE - 16, 16),
            (packer.BANK_SIZE - 16, 17),
            (packer.BANK_SIZE, packer.BANK_SIZE, 1),
            (15, 16, 17, packer.BANK_SIZE - 64, 65),
        )
        for payload_sizes in cases:
            with self.subTest(payload_sizes=payload_sizes):
                self.assert_plan_matches_oracle(payload_sizes)

        alignment_fit = packer.plan_bank_layout(
            (1, packer.BANK_SIZE - 16))
        self.assertEqual(alignment_fit.required_bank_count, 1)
        self.assertEqual(alignment_fit.placements[1].bank_offset, 16)
        self.assertEqual(alignment_fit.frame_alignment_bytes, 15)

        one_byte_overflow = packer.plan_bank_layout(
            (packer.BANK_SIZE - 16, 17))
        self.assertEqual(one_byte_overflow.required_bank_count, 2)
        self.assertEqual(one_byte_overflow.placements[1].bank_offset, 0)
        self.assertEqual(one_byte_overflow.bank_boundary_padding_bytes, 16)

    def test_plan_failures_reach_exact_codes(self) -> None:
        cases = (
            (lambda: packer.plan_bank_layout(()), "M98I_PLAN_COUNT"),
            (lambda: packer.plan_bank_layout((0,)), "M98I_PLAN_SIZE"),
            (lambda: packer.plan_bank_layout((packer.BANK_SIZE + 1,)),
             "M98I_FRAME_TOO_LARGE"),
            (lambda: packer.plan_bank_layout(tuple(1 for _ in range(31))),
             "M98I_PLAN_COUNT"),
        )
        for operation, expected_code in cases:
            with self.subTest(expected_code=expected_code):
                self.assert_packing_error(operation, expected_code)

    def test_public_atlas_is_reproducible_minimal_and_format_valid(self) -> None:
        first = packer.build_public_fixture()
        second = packer.build_public_fixture()
        self.assertEqual(first.contents, second.contents)
        self.assertEqual(
            packer.report_bytes(first.report), packer.report_bytes(second.report))

        header, descriptors, plan = packer.inspect_packed_bytes(first.contents)
        self.assertEqual(header.required_bank_count, plan.required_bank_count)
        self.assertEqual(plan.required_bank_count, 1)
        self.assertEqual(len(descriptors), 30)
        self.assertTrue(all(descriptor.bank_slot == 0
                            for descriptor in descriptors))
        self.assert_plan_matches_oracle(tuple(
            descriptor.payload_bytes for descriptor in descriptors))

        format_header, format_descriptors = format_inspector.inspect_bytes(
            format_fixture.build_fixture())
        self.assertEqual(format_header.required_bank_count, 30)
        self.assertEqual(len(format_descriptors), 30)
        self.assert_packing_error(
            lambda: packer.inspect_packed_bytes(format_fixture.build_fixture()),
            "M98I_REQUIRED_BANK_COUNT",
        )

    def test_second_bank_plan_is_rejected(self) -> None:
        valid = packer.plan_bank_layout((packer.BANK_SIZE,))
        packer.require_one_bank(valid)
        second_bank = packer.plan_bank_layout((packer.BANK_SIZE, 1))
        self.assertEqual(second_bank.required_bank_count, 2)
        self.assert_packing_error(
            lambda: packer.require_one_bank(second_bank),
            "M98I_ATLAS_BANK_COUNT",
        )

    def test_scale_set_failures_reach_exact_isolated_codes(self) -> None:
        valid = format_fixture.public_scale_set()
        packer.validate_scale_set(valid)

        missing = dataclasses.replace(valid, frames=valid.frames[:-1])
        self.assert_packing_error(
            lambda: packer.validate_scale_set(missing), "M98I_SCALE_COUNT")

        changed_frames = list(valid.frames)
        changed_frames[0] = dataclasses.replace(changed_frames[0], level=2)
        changed_level = dataclasses.replace(valid, frames=tuple(changed_frames))
        self.assert_packing_error(
            lambda: packer.validate_scale_set(changed_level),
            "M98I_LEVEL_ORDER")

        changed_frames = list(valid.frames)
        changed_frames[0] = dataclasses.replace(
            changed_frames[0], payload=changed_frames[0].payload[:-1])
        changed_length = dataclasses.replace(valid, frames=tuple(changed_frames))
        self.assert_packing_error(
            lambda: packer.validate_scale_set(changed_length),
            "M98I_FRAME_LENGTH")

        gap_index = next(
            index for index in range(1, len(valid.frames))
            if valid.frames[index].offset
            > valid.frames[index - 1].offset + len(valid.frames[index - 1].payload))
        gap_offset = (valid.frames[gap_index - 1].offset
                      + len(valid.frames[gap_index - 1].payload))
        changed_stream = bytearray(valid.stream)
        changed_stream[gap_offset] = 1
        changed_padding = dataclasses.replace(valid, stream=bytes(changed_stream))
        self.assert_packing_error(
            lambda: packer.validate_scale_set(changed_padding),
            "M98I_STREAM_PADDING")

        changed_stream = bytearray(valid.stream)
        changed_stream[valid.frames[0].offset] ^= 1
        changed_payload = dataclasses.replace(valid, stream=bytes(changed_stream))
        self.assert_packing_error(
            lambda: packer.validate_scale_set(changed_payload),
            "M98I_STREAM_PAYLOAD")

        trailing = dataclasses.replace(valid, stream=valid.stream + b"\x00")
        self.assert_packing_error(
            lambda: packer.validate_scale_set(trailing),
            "M98I_STREAM_LAYOUT")

        padded_index = next(
            index for index, frame in enumerate(valid.frames)
            if frame.width < frame.pitch)
        padded = valid.frames[padded_index]
        changed_frame_payload = bytearray(padded.payload)
        changed_frame_payload[padded.width] = 1
        changed_frames = list(valid.frames)
        changed_frames[padded_index] = dataclasses.replace(
            padded, payload=bytes(changed_frame_payload))
        changed_stream = bytearray(valid.stream)
        changed_stream[padded.offset + padded.width] = 1
        changed_row_padding = dataclasses.replace(
            valid, frames=tuple(changed_frames), stream=bytes(changed_stream))
        self.assert_packing_error(
            lambda: packer.validate_scale_set(changed_row_padding),
            "M98I_ROW_PADDING")

    def test_production_packing_validator_rejects_isolated_corruptions(self) -> None:
        packed = packer.build_public_fixture()
        header, descriptors = format_inspector.inspect_bytes(packed.contents)
        packer.validate_production_packing(header, descriptors)

        changed_header = dataclasses.replace(header, required_bank_count=2)
        self.assert_packing_error(
            lambda: packer.validate_production_packing(
                changed_header, descriptors),
            "M98I_REQUIRED_BANK_COUNT")

        changed_descriptors = list(descriptors)
        changed_descriptors[1] = dataclasses.replace(
            changed_descriptors[1], bank_slot=1, bank_offset=0)
        self.assert_packing_error(
            lambda: packer.validate_production_packing(
                header, tuple(changed_descriptors)),
            "M98I_NONMINIMAL_LAYOUT")

    def test_report_metrics_reconcile(self) -> None:
        packed = packer.build_public_fixture()
        _, descriptors, plan = packer.inspect_packed_bytes(packed.contents)
        metrics = cast(dict[str, object], packed.report["metrics"])
        frame_payload = sum(descriptor.payload_bytes for descriptor in descriptors)
        useful = sum(descriptor.width * descriptor.height
                     for descriptor in descriptors)
        row_padding = sum(
            (descriptor.pitch - descriptor.width) * descriptor.height
            for descriptor in descriptors)
        self.assertEqual(metrics["frame_payload_bytes"], frame_payload)
        self.assertEqual(metrics["useful_pixel_bytes"], useful)
        self.assertEqual(metrics["row_padding_bytes"], row_padding)
        self.assertEqual(frame_payload, useful + row_padding)
        self.assertEqual(metrics["bank_frame_alignment_bytes"],
                         plan.frame_alignment_bytes)
        self.assertEqual(metrics["bank_boundary_padding_bytes"],
                         plan.bank_boundary_padding_bytes)
        self.assertEqual(metrics["bank_payload_bytes"],
                         list(plan.bank_payload_bytes))
        self.assertEqual(metrics["bank_occupied_bytes"],
                         list(plan.bank_occupied_bytes))
        self.assertEqual(metrics["required_bank_count"],
                         plan.required_bank_count)
        self.assertEqual(
            metrics["payload_region_bytes"],
            frame_payload + cast(int, metrics["compact_file_alignment_bytes"]),
        )
        self.assertEqual(
            metrics["bms_span_bytes"],
            frame_payload + plan.frame_alignment_bytes
            + plan.bank_boundary_padding_bytes,
        )
        self.assertEqual(
            metrics["atlas_file_bytes"],
            packer.PAYLOAD_OFFSET + cast(int, metrics["payload_region_bytes"]),
        )
        self.assertEqual(packed.report["schema"], packer.REPORT_SCHEMA)

    def test_cli_outputs_are_reproducible_redacted_and_refuse_overwrite(self) -> None:
        with tempfile.TemporaryDirectory(prefix="vaeg-m98i-") as temporary:
            root = Path(temporary)
            first = root / "first"
            second = root / "second"
            for output in (first, second):
                result = subprocess.run(
                    [sys.executable, str(PACKER), "--fixture-output", str(output)],
                    check=False,
                    capture_output=True,
                    text=True,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, "M98I_PACK_PASS\n")
                self.assertEqual(result.stderr, "")
            for filename in (packer.ATLAS_NAME, packer.REPORT_NAME):
                self.assertEqual((first / filename).read_bytes(),
                                 (second / filename).read_bytes())
            report = json.loads((first / packer.REPORT_NAME).read_text(
                encoding="utf-8"))
            self.assertEqual(report["schema"], packer.REPORT_SCHEMA)

            inspect = subprocess.run(
                [sys.executable, str(PACKER), "--inspect",
                 str(first / packer.ATLAS_NAME)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(inspect.returncode, 0, inspect.stderr)
            self.assertEqual(inspect.stdout, "M98I_PACKING_PASS\n")
            self.assertEqual(inspect.stderr, "")

            marker = "localidentitymustnotappear"
            overwrite = subprocess.run(
                [sys.executable, str(PACKER), "--fixture-output",
                 str(first)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(overwrite.returncode, 0)
            self.assertEqual(
                overwrite.stderr,
                "M98I_OUTPUT_EXISTS: output directory exists\n",
            )
            missing = subprocess.run(
                [sys.executable, str(PACKER), "--inspect", str(root / marker)],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(missing.returncode, 0)
            self.assertEqual(missing.stdout, "")
            self.assertEqual(
                missing.stderr, "M98H_FILE_READ: atlas could not be read\n")
            self.assertNotIn(marker, missing.stderr)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(
        ZundamonOrbitPackingTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if result.wasSuccessful():
        print("M98I_TEST_PASS")
    raise SystemExit(0 if result.wasSuccessful() else 1)

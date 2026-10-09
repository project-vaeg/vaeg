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

"""Exhaustive and fail-closed M98u multi-instance state tests."""

from __future__ import annotations

import copy
import hashlib
import tempfile
import unittest
from unittest import mock
from dataclasses import replace
from pathlib import Path
import sys

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import generate_zundamon_multi_instance_state as generator  # noqa: E402
import validate_zundamon_multi_instance_state as validator  # noqa: E402

DEPTH_TABLE = TOOLS.parent / "256" / "zundamon_depth_table.inc"
CONTRACT = TOOLS.parent / "256" / "zundamon_multi_instance_contract.inc"


class M98uReferenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        cls.root = Path(cls.temporary.name)
        cls.fixture = cls.root / "fixture"
        pipeline.write_public_fixture(cls.fixture)
        cls.atlas_path = cls.fixture / pipeline.ATLAS_NAME
        cls.header, cls.entries, cls.descriptors = generator.load_inputs(
            DEPTH_TABLE, cls.atlas_path)
        cls.document, cls.summary = generator.build_reference_document(
            cls.header, cls.entries, cls.descriptors)
        cls.golden = generator.canonical_json(cls.document)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temporary.cleanup()

    def test_checked_in_contract_is_exact_generator_output(self) -> None:
        expected = generator.encode_contract_include()
        self.assertEqual(CONTRACT.read_bytes(), expected)
        checked = validator.validate_contract_include(CONTRACT)
        self.assertEqual(checked, expected)
        self.assertEqual(generator.INSTANCE_RECORD_BYTES, 50)
        self.assertEqual(generator.INSTANCE_RECORD_BYTES * 16, 800)

    def test_exhaustive_matrix_has_exact_totals(self) -> None:
        self.assertEqual(self.summary["max_instances"], 16)
        self.assertEqual(self.summary["counts_tested"], 16)
        self.assertEqual(self.summary["global_phases_tested"], 64)
        self.assertEqual(self.summary["count_phase_combinations"], 1024)
        self.assertEqual(self.summary["instance_records_generated"], 8704)
        self.assertEqual(self.summary["draw_orders_generated"], 1024)
        for name, value in self.summary.items():
            if name.endswith("_failures") or name.endswith("_mismatches"):
                self.assertEqual(value, 0, name)

    def test_independent_validator_accepts_complete_matrix(self) -> None:
        summary = validator.validate_document(
            self.document, self.header, self.entries, self.descriptors)
        self.assertEqual(summary, self.summary)

    def test_canonical_generation_is_byte_deterministic(self) -> None:
        second, second_summary = generator.build_reference_document(
            self.header, self.entries, self.descriptors)
        second_bytes = generator.canonical_json(second)
        self.assertEqual(second_summary, self.summary)
        self.assertEqual(second_bytes, self.golden)
        self.assertEqual(hashlib.sha256(second_bytes).digest(),
                         hashlib.sha256(self.golden).digest())

    def test_exact_representative_phase_offsets(self) -> None:
        expected = {
            1: (0,),
            2: (0, 32),
            4: (0, 16, 32, 48),
            8: (0, 8, 16, 24, 32, 40, 48, 56),
            16: tuple(range(0, 64, 4)),
        }
        for count, offsets in expected.items():
            with self.subTest(count=count):
                self.assertEqual(generator.expected_offsets(count), offsets)

    def test_nondivisor_counts_have_balanced_circular_gaps(self) -> None:
        for count in (3, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15):
            offsets = generator.expected_offsets(count)
            gaps = generator.validate_offsets(count, offsets)
            self.assertEqual(sum(gaps), 64)
            self.assertLessEqual(max(gaps) - min(gaps), 1)

    def test_count_one_equals_every_m98t_phase(self) -> None:
        for global_phase in range(64):
            state = generator.build_state(1, global_phase, self.header,
                                          self.entries, self.descriptors)
            record = state.records[0]
            entry = self.entries[global_phase]
            descriptor = self.descriptors[entry.scale_id - 1]
            self.assertEqual(record.phase_id, global_phase)
            self.assertEqual(record.depth_rank, entry.depth_rank)
            self.assertEqual(record.scale_id, entry.scale_id)
            self.assertEqual(record.descriptor_index, entry.scale_id - 1)
            self.assertEqual(record.sgp_source,
                             generator.BMS_WINDOW + descriptor.bank_offset)
            self.assertEqual(record.source_identity, descriptor.frame_crc32)
            self.assertEqual(state.draw_order, (0,))

    def test_rotation_covariance_and_identity(self) -> None:
        for count in range(1, 17):
            prior = generator.build_state(count, 0, self.header,
                                          self.entries, self.descriptors)
            for global_phase in range(1, 64):
                current = generator.build_state(
                    count, global_phase, self.header, self.entries,
                    self.descriptors)
                for instance_id in range(count):
                    self.assertEqual(
                        current.records[instance_id].phase_id,
                        (prior.records[instance_id].phase_id + 1) & 63)
                    self.assertEqual(current.records[instance_id].instance_id,
                                     instance_id)
                prior = current
            wrapped = generator.build_state(count, 0, self.header,
                                            self.entries, self.descriptors)
            for instance_id in range(count):
                self.assertEqual(
                    wrapped.records[instance_id].phase_id,
                    (prior.records[instance_id].phase_id + 1) & 63)

    def test_all_records_share_one_bank_and_contain_no_payload_copy(self) -> None:
        for count_section in self.document["counts"]:
            for state in count_section["states"]:
                for record in state["records"]:
                    self.assertEqual(record["bms_bank"], 1)
                    self.assertEqual(tuple(record), validator.RECORD_FIELDS)
                    self.assertTrue(all(isinstance(value, int)
                                        for value in record.values()))
                    self.assertNotIn("payload", record)
                    self.assertNotIn("pixels", record)

    def test_draw_order_is_explicit_depth_then_instance_id(self) -> None:
        ties = 0
        for count_section in self.document["counts"]:
            for state in count_section["states"]:
                records = state["records"]
                keys = [(records[index]["depth_rank"],
                         records[index]["instance_id"])
                        for index in state["draw_order"]]
                self.assertEqual(keys, sorted(keys))
                ties += sum(keys[index][0] == keys[index + 1][0]
                            for index in range(len(keys) - 1))
        self.assertGreater(ties, 0)

    def test_generated_reference_round_trips_through_file_validator(self) -> None:
        golden_path = self.root / "golden.json"
        golden_path.write_bytes(self.golden)
        try:
            raw, summary, contract = validator.inspect(
                golden_path, self.atlas_path, DEPTH_TABLE, CONTRACT)
        finally:
            golden_path.unlink()
        self.assertEqual(raw, self.golden)
        self.assertEqual(summary, self.summary)
        self.assertEqual(contract, CONTRACT.read_bytes())


class M98uNegativeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory()
        cls.root = Path(cls.temporary.name)
        cls.fixture = cls.root / "fixture"
        pipeline.write_public_fixture(cls.fixture)
        cls.atlas_path = cls.fixture / pipeline.ATLAS_NAME
        cls.header, cls.entries, cls.descriptors = generator.load_inputs(
            DEPTH_TABLE, cls.atlas_path)
        cls.document, _ = generator.build_reference_document(
            cls.header, cls.entries, cls.descriptors)

    @classmethod
    def tearDownClass(cls) -> None:
        cls.temporary.cleanup()

    def assert_generator_code(self, code: str, callable_object, *args) -> None:
        with self.assertRaises(generator.MultiInstanceError) as raised:
            callable_object(*args)
        self.assertEqual(raised.exception.code, code)

    def assert_validator_code(self, code: str, document) -> None:
        with self.assertRaises(validator.ReferenceError) as raised:
            validator.validate_document(document, self.header, self.entries,
                                        self.descriptors)
        self.assertEqual(raised.exception.code, code)

    def test_active_count_and_global_phase_ranges_fail_closed(self) -> None:
        self.assert_generator_code("M98U_ACTIVE_COUNT_RANGE",
                                   generator.phase_offset, 0, 0)
        self.assert_generator_code("M98U_ACTIVE_COUNT_RANGE",
                                   generator.phase_offset, 17, 0)
        self.assert_generator_code(
            "M98U_GLOBAL_PHASE_RANGE", generator.build_state, 1, -1,
            self.header, self.entries, self.descriptors)
        self.assert_generator_code(
            "M98U_GLOBAL_PHASE_RANGE", generator.build_state, 1, 64,
            self.header, self.entries, self.descriptors)

    def test_instance_id_and_u16_arithmetic_fail_closed(self) -> None:
        self.assert_generator_code("M98U_INSTANCE_ID_RANGE",
                                   generator.phase_offset, 4, 4)
        self.assert_generator_code("M98U_U16_MULTIPLY_INPUT",
                                   generator.checked_u16_product, -1, 64)
        self.assert_generator_code("M98U_U16_MULTIPLY_OVERFLOW",
                                   generator.checked_u16_product, 1024, 64)

    def test_wrong_rounding_incremental_and_duplicate_offsets_fail(self) -> None:
        self.assert_generator_code("M98U_OFFSET_FORMULA",
                                   generator.validate_offsets, 3,
                                   (0, 21, 43))
        self.assert_generator_code("M98U_OFFSET_FORMULA",
                                   generator.validate_offsets, 5,
                                   (0, 12, 24, 36, 48))
        self.assert_generator_code("M98U_OFFSET_FORMULA",
                                   generator.validate_offsets, 4,
                                   (0, 16, 16, 48))
        self.assert_generator_code("M98U_PHASE_UNIQUE",
                                   generator.validate_phase_ids, 4,
                                   (0, 16, 16, 48))

    def test_invalid_circular_gap_sum_and_balance_fail(self) -> None:
        self.assert_generator_code("M98U_GAP_SUM",
                                   generator.validate_circular_gaps, 3,
                                   (20, 20, 20))
        self.assert_generator_code("M98U_GAP_BALANCE",
                                   generator.validate_circular_gaps, 3,
                                   (10, 27, 27))

    def test_missing_or_malformed_phase_input_fails(self) -> None:
        self.assert_generator_code(
            "M98U_PHASE_TABLE", generator.validate_shared_inputs,
            self.header, self.entries[:-1], self.descriptors)
        malformed = list(self.entries)
        malformed[1] = replace(malformed[1], phase=0)
        self.assert_generator_code(
            "M98U_PHASE_TABLE", generator.validate_shared_inputs,
            self.header, tuple(malformed), self.descriptors)

    def test_depth_scale_and_scale_range_fail(self) -> None:
        bad_depth = list(self.entries)
        bad_depth[0] = replace(bad_depth[0], depth_rank=3)
        self.assert_generator_code(
            "M98U_DEPTH_SCALE_MISMATCH", generator.validate_shared_inputs,
            self.header, tuple(bad_depth), self.descriptors)
        bad_scale = list(self.entries)
        bad_scale[0] = replace(bad_scale[0], scale_id=0)
        self.assert_generator_code(
            "M98U_SCALE_RANGE", generator.validate_shared_inputs,
            self.header, tuple(bad_scale), self.descriptors)
        bad_scale[0] = replace(bad_scale[0], scale_id=31)
        self.assert_generator_code(
            "M98U_SCALE_RANGE", generator.validate_shared_inputs,
            self.header, tuple(bad_scale), self.descriptors)

    def test_descriptor_payload_geometry_anchor_and_source_fail(self) -> None:
        cases = (
            ("M98U_DESCRIPTOR_PAYLOAD",
             replace(self.descriptors[15], payload_bytes=1)),
            ("M98U_DESCRIPTOR_GEOMETRY",
             replace(self.descriptors[15], pitch=1,
                     payload_bytes=self.descriptors[15].height)),
            ("M98U_DESCRIPTOR_ANCHOR",
             replace(self.descriptors[15], anchor_x=100)),
            ("M98U_ATLAS_BANK_CONTRACT",
             replace(self.descriptors[15], bank_slot=1)),
            ("M98U_SOURCE_RANGE",
             replace(self.descriptors[15], bank_offset=0x1FFF0,
                     payload_bytes=self.descriptors[15].pitch
                     * self.descriptors[15].height)),
        )
        for code, descriptor in cases:
            descriptors = list(self.descriptors)
            descriptors[15] = descriptor
            with self.subTest(code=code):
                self.assert_generator_code(
                    code, generator.derive_record, 1, 0, 0, self.header,
                    self.entries, tuple(descriptors))

    def test_descriptor_identity_paths_must_agree(self) -> None:
        with mock.patch.object(
                generator.depth_table, "inspect",
                return_value=(None, self.entries, None,
                              self.descriptors[:-1])):
            self.assert_generator_code(
                "M98U_DESCRIPTOR_IDENTITY", generator.load_inputs,
                DEPTH_TABLE, self.atlas_path)

    def test_destination_overflow_and_hud_intersection_fail(self) -> None:
        outside = list(self.entries)
        outside[0] = replace(outside[0], dx=1000)
        self.assert_generator_code(
            "M98U_DESTINATION_BOUNDS", generator.derive_record, 1, 0, 0,
            self.header, tuple(outside), self.descriptors)
        hud = list(self.entries)
        hud[0] = replace(hud[0], dx=-150, dy=-90)
        self.assert_generator_code(
            "M98U_HUD_INTERSECTION", generator.derive_record, 1, 0, 0,
            self.header, tuple(hud), self.descriptors)
        self.assert_generator_code(
            "M98U_G1_PAGE_BOUNDS", generator.validate_g1_destination,
            generator.G1_PAGE_BASES[0], (0, 199, 320, 201))
        self.assert_generator_code(
            "M98U_G1_ADDRESS_OVERFLOW", generator.validate_g1_destination,
            0xFFFFFFF0, (0, 1, 1, 2))

    def test_fixed_sort_capacity_and_record_ids_fail(self) -> None:
        state = generator.build_state(16, 0, self.header, self.entries,
                                      self.descriptors)
        self.assert_generator_code(
            "M98U_SORT_CAPACITY", generator.bounded_insertion_order,
            state.records + (state.records[0],))
        swapped = (state.records[1], state.records[0]) + state.records[2:]
        self.assert_generator_code(
            "M98U_RECORD_IDS", generator.bounded_insertion_order, swapped)

    def test_draw_order_permutation_and_near_to_far_fail(self) -> None:
        state = generator.build_state(16, 0, self.header, self.entries,
                                      self.descriptors)
        bad_permutation = state.draw_order[:-1] + (state.draw_order[-2],)
        self.assert_generator_code(
            "M98U_DRAW_ORDER_PERMUTATION", generator.validate_draw_order,
            state.records, bad_permutation)
        wrong_depth = list(state.draw_order)
        for position in range(len(wrong_depth) - 1):
            first = state.records[wrong_depth[position]]
            second = state.records[wrong_depth[position + 1]]
            if first.depth_rank != second.depth_rank:
                wrong_depth[position], wrong_depth[position + 1] = (
                    wrong_depth[position + 1], wrong_depth[position])
                break
        self.assert_generator_code(
            "M98U_DRAW_ORDER_KEY", generator.validate_draw_order,
            state.records, tuple(wrong_depth))

    def test_equal_depth_tie_requires_ascending_instance_id(self) -> None:
        for count in range(2, 17):
            for phase in range(64):
                state = generator.build_state(count, phase, self.header,
                                              self.entries, self.descriptors)
                order = list(state.draw_order)
                for position in range(len(order) - 1):
                    first = state.records[order[position]]
                    second = state.records[order[position + 1]]
                    if first.depth_rank == second.depth_rank:
                        order[position], order[position + 1] = (
                            order[position + 1], order[position])
                        self.assert_generator_code(
                            "M98U_DRAW_ORDER_TIE",
                            generator.validate_draw_order, state.records,
                            tuple(order))
                        return
        self.fail("no equal-depth tie found")

    def test_serialization_rejects_private_and_nondecimal_metadata(self) -> None:
        for value, code in (
            ({"path": "relative"}, "M98U_SERIALIZATION_PRIVATE"),
            ({"value": "/private/input"}, "M98U_SERIALIZATION_PRIVATE"),
            ({"value": "0x080000"}, "M98U_SERIALIZATION_NUMERIC"),
        ):
            with self.subTest(code=code), self.assertRaises(
                    validator.ReferenceError) as raised:
                validator.find_private_metadata(value)
            self.assertEqual(raised.exception.code, code)

    def test_golden_missing_record_payload_copy_and_count_one_mismatch_fail(self) -> None:
        missing = copy.deepcopy(self.document)
        missing["counts"][0]["states"][0]["records"] = []
        self.assert_validator_code("M98U_RECORD_COUNT", missing)
        payload = copy.deepcopy(self.document)
        payload["counts"][0]["states"][0]["records"][0]["payload_copy"] = [0]
        self.assert_validator_code("M98U_RECORD_FIELDS", payload)
        mismatch = copy.deepcopy(self.document)
        mismatch["counts"][0]["states"][0]["records"][0]["scale_id"] += 1
        self.assert_validator_code("M98U_COUNT_ONE_MISMATCH", mismatch)

    def test_serialized_gap_permutation_order_and_summary_fail(self) -> None:
        gap = copy.deepcopy(self.document)
        gap["counts"][2]["circular_gaps"][0] += 1
        self.assert_validator_code("M98U_GAP_VALUES", gap)
        permutation = copy.deepcopy(self.document)
        permutation["counts"][3]["states"][0]["draw_order"][0] = 99
        self.assert_validator_code("M98U_DRAW_ORDER_PERMUTATION", permutation)
        order = copy.deepcopy(self.document)
        records = order["counts"][15]["states"][0]["records"]
        draw_order = order["counts"][15]["states"][0]["draw_order"]
        for position in range(len(draw_order) - 1):
            if (records[draw_order[position]]["depth_rank"]
                    != records[draw_order[position + 1]]["depth_rank"]):
                draw_order[position], draw_order[position + 1] = (
                    draw_order[position + 1], draw_order[position])
                break
        self.assert_validator_code("M98U_DRAW_ORDER_KEY", order)
        summary = copy.deepcopy(self.document)
        summary["summary"]["instance_records_generated"] = 8703
        self.assert_validator_code("M98U_SUMMARY", summary)

    def test_contract_rejects_expansion_and_layout_changes(self) -> None:
        original = CONTRACT.read_text(encoding="ascii")
        cases = (
            (original.replace("M98U_INSTANCE_RECORD_BYTES 50",
                              "M98U_INSTANCE_RECORD_BYTES 49", 1),
             "M98U_CONTRACT_LAYOUT"),
            (original + "%define M98U_PHASE_COUNT 64\n",
             "M98U_CONTRACT_DUPLICATE"),
            (original + "global_phase_0: db 0\n", "M98U_CONTRACT_EXPANDED"),
        )
        for index, (text, code) in enumerate(cases):
            path = self.root / f"bad-contract-{index}.inc"
            path.write_text(text, encoding="ascii")
            try:
                with self.subTest(code=code), self.assertRaises(
                        validator.ReferenceError) as raised:
                    validator.validate_contract_include(path)
                self.assertEqual(raised.exception.code, code)
            finally:
                path.unlink()

    def test_partial_or_noncanonical_golden_is_rejected(self) -> None:
        path = self.root / "bad-golden.json"
        path.write_text('{"schema":"bad"}\n', encoding="utf-8")
        try:
            with self.assertRaises(validator.ReferenceError) as raised:
                validator.read_json(path)
            self.assertEqual(raised.exception.code, "M98U_GOLDEN_CANONICAL")
        finally:
            path.unlink()


if __name__ == "__main__":
    unittest.main()

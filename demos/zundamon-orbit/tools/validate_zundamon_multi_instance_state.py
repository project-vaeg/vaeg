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

"""Independently validate the exhaustive M98u reference serialization."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import validate_zundamon_orbit_depth_table as depth_table  # noqa: E402

MAX_INSTANCES = 16
PHASE_COUNT = 64
SCALE_COUNT = 30
BMS_WINDOW = 0x080000
BMS_BANK_BYTES = 0x020000
HUD_RECT = (4, 4, 70, 20)
PAGE_BASES = (0x220000, 0x22FA00)
PAGE_BYTES = 64000
PITCH = 320
MAX_GOLDEN_BYTES = 32 * 1024 * 1024
SCHEMA = "zundamon-orbit-m98u-multi-instance-state-v1"
RECORD_FIELDS = (
    "instance_id", "phase_offset", "phase_id", "scale_id", "depth_rank",
    "bms_bank", "descriptor_index", "reserved", "dx", "dy", "width",
    "height", "pitch", "anchor_x", "anchor_y", "target_anchor_x",
    "target_anchor_y", "dst_x", "dst_y", "dst_x1", "dst_y1",
    "bank_offset", "sgp_source", "payload_bytes", "source_identity",
)
EXPECTED_LAYOUT = (
    ("instance_id", 0, 1, False),
    ("phase_offset", 1, 1, False),
    ("phase_id", 2, 1, False),
    ("scale_id", 3, 1, False),
    ("depth_rank", 4, 1, True),
    ("bms_bank", 5, 1, False),
    ("descriptor_index", 6, 1, False),
    ("reserved", 7, 1, False),
    ("dx", 8, 2, True),
    ("dy", 10, 2, True),
    ("width", 12, 2, False),
    ("height", 14, 2, False),
    ("pitch", 16, 2, False),
    ("anchor_x", 18, 2, False),
    ("anchor_y", 20, 2, False),
    ("target_anchor_x", 22, 2, True),
    ("target_anchor_y", 24, 2, True),
    ("dst_x", 26, 2, True),
    ("dst_y", 28, 2, True),
    ("dst_x1", 30, 2, True),
    ("dst_y1", 32, 2, True),
    ("bank_offset", 34, 4, False),
    ("sgp_source", 38, 4, False),
    ("payload_bytes", 42, 4, False),
    ("source_identity", 46, 4, False),
)
DEFINE = re.compile(r"^%define\s+([A-Z0-9_]+)\s+(\d+)(?:\s*;.*)?$")


class ReferenceError(ValueError):
    """One stable M98u reference validation failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> None:
    raise ReferenceError(code, detail)


def canonical_json(document) -> bytes:
    return (json.dumps(document, ensure_ascii=True, indent=2,
                       separators=(",", ": ")) + "\n").encode("utf-8")


def read_json(path: Path):
    try:
        status = path.lstat()
        if path.is_symlink() or not path.is_file() or status.st_size > MAX_GOLDEN_BYTES:
            fail("M98U_GOLDEN_FILE", "golden must be a bounded regular file")
        raw = path.read_bytes()
        document = json.loads(raw.decode("utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise ReferenceError("M98U_GOLDEN_READ", "golden JSON could not be read") from error
    if canonical_json(document) != raw:
        fail("M98U_GOLDEN_CANONICAL", "golden JSON is not canonical")
    return raw, document


def find_private_metadata(value, key: str = "") -> None:
    forbidden_keys = {"timestamp", "hostname", "pointer", "path", "private"}
    if isinstance(value, dict):
        for child_key, child in value.items():
            if child_key.lower() in forbidden_keys:
                fail("M98U_SERIALIZATION_PRIVATE", "forbidden metadata key is present")
            find_private_metadata(child, child_key)
    elif isinstance(value, list):
        for child in value:
            find_private_metadata(child, key)
    elif isinstance(value, str):
        if value.startswith("/") or re.match(r"^[A-Za-z]:[\\/]", value):
            fail("M98U_SERIALIZATION_PRIVATE", "absolute path is present")
        if "0x" in value.lower() or re.search(r"\b[0-9a-f]{8,}\b", value.lower()):
            fail("M98U_SERIALIZATION_NUMERIC", "numeric identity is not a decimal integer")


def expected_offsets(active_count: int) -> tuple[int, ...]:
    return tuple((PHASE_COUNT * instance_id) // active_count
                 for instance_id in range(active_count))


def expected_gaps(offsets: tuple[int, ...]) -> tuple[int, ...]:
    return tuple(offsets[index + 1] - offsets[index]
                 for index in range(len(offsets) - 1)) + (
                     PHASE_COUNT - offsets[-1] + offsets[0],)


def intersects(left, right) -> bool:
    return (left[0] < right[2] and right[0] < left[2]
            and left[1] < right[3] and right[1] < left[3])


def expected_record(active_count: int, global_phase: int, instance_id: int,
                    header, entries, descriptors) -> dict[str, int]:
    offset = (PHASE_COUNT * instance_id) // active_count
    phase_id = (global_phase + offset) & 63
    entry = entries[phase_id]
    descriptor_index = entry.scale_id - 1
    descriptor = descriptors[descriptor_index]
    target_x = 160 + entry.dx
    target_y = 100 + entry.dy
    x0 = target_x - descriptor.anchor_x
    y0 = target_y - descriptor.anchor_y
    x1 = x0 + descriptor.width
    y1 = y0 + descriptor.height
    if not (0 <= x0 < x1 <= 320 and 0 <= y0 < y1 <= 200):
        fail("M98U_DESTINATION_BOUNDS", "expected rectangle is outside 320x200")
    if intersects((x0, y0, x1, y1), HUD_RECT):
        fail("M98U_HUD_INTERSECTION", "expected rectangle intersects HUD")
    if descriptor.bank_slot != 0:
        fail("M98U_ATLAS_BANK_CONTRACT", "descriptor is outside the shared bank")
    if descriptor.bank_offset + descriptor.payload_bytes > BMS_BANK_BYTES:
        fail("M98U_SOURCE_RANGE", "expected source exceeds the BMS bank")
    for page_base in PAGE_BASES:
        first = page_base + y0 * PITCH + x0
        end = page_base + (y1 - 1) * PITCH + x1
        if first < page_base or end > page_base + PAGE_BYTES:
            fail("M98U_G1_PAGE_BOUNDS", "expected destination exceeds a G1 page")
    return {
        "instance_id": instance_id,
        "phase_offset": offset,
        "phase_id": phase_id,
        "scale_id": entry.scale_id,
        "depth_rank": entry.depth_rank,
        "bms_bank": header.first_bank_value + descriptor.bank_slot,
        "descriptor_index": descriptor_index,
        "reserved": 0,
        "dx": entry.dx,
        "dy": entry.dy,
        "width": descriptor.width,
        "height": descriptor.height,
        "pitch": descriptor.pitch,
        "anchor_x": descriptor.anchor_x,
        "anchor_y": descriptor.anchor_y,
        "target_anchor_x": target_x,
        "target_anchor_y": target_y,
        "dst_x": x0,
        "dst_y": y0,
        "dst_x1": x1,
        "dst_y1": y1,
        "bank_offset": descriptor.bank_offset,
        "sgp_source": BMS_WINDOW + descriptor.bank_offset,
        "payload_bytes": descriptor.payload_bytes,
        "source_identity": descriptor.frame_crc32,
    }


def validate_contract_include(path: Path) -> bytes:
    try:
        raw = path.read_bytes()
        text = raw.decode("ascii")
    except (OSError, UnicodeError) as error:
        raise ReferenceError("M98U_CONTRACT_READ", "contract include could not be read") from error
    defines = {}
    for line in text.splitlines():
        match = DEFINE.match(line)
        if match:
            name, value = match.groups()
            if name in defines:
                fail("M98U_CONTRACT_DUPLICATE", "contract define is duplicated")
            defines[name] = int(value)
    expected = {
        "M98U_MAX_ZUNDAMON_INSTANCES": 16,
        "M98U_PHASE_COUNT": 64,
        "M98U_INSTANCE_RECORD_BYTES": 50,
        "M98U_INSTANCE_RECORD_CAPACITY_BYTES": 800,
        "M98U_DRAW_ORDER_INDEX_BYTES": 1,
        "M98U_DRAW_ORDER_CAPACITY_BYTES": 16,
    }
    for name, offset, _, _ in EXPECTED_LAYOUT:
        expected[f"M98U_RECORD_{name.upper()}"] = offset
    if defines != expected:
        fail("M98U_CONTRACT_LAYOUT", "contract include layout differs")
    if re.search(r"^global_phase_", text, re.MULTILINE) or text.count("phase=") != 1:
        fail("M98U_CONTRACT_EXPANDED", "contract contains pre-expanded state")
    return raw


def require_keys(value: dict, expected: tuple[str, ...], code: str) -> None:
    if tuple(value.keys()) != expected:
        fail(code, "object field order or membership differs")


def validate_document(document, header, entries, descriptors):
    require_keys(document, ("schema", "contract", "counts", "summary"),
                 "M98U_GOLDEN_FIELDS")
    if document["schema"] != SCHEMA:
        fail("M98U_GOLDEN_SCHEMA", "schema differs")
    contract = document["contract"]
    require_keys(contract, (
        "max_instances", "phase_count", "scale_count",
        "instance_record_bytes", "draw_order_index_bytes",
        "record_capacity_bytes", "draw_order_capacity_bytes",
        "bms_bank_bytes", "bms_window", "hud_rect"),
        "M98U_CONTRACT_FIELDS")
    expected_contract = {
        "max_instances": 16,
        "phase_count": 64,
        "scale_count": 30,
        "instance_record_bytes": 50,
        "draw_order_index_bytes": 1,
        "record_capacity_bytes": 800,
        "draw_order_capacity_bytes": 16,
        "bms_bank_bytes": BMS_BANK_BYTES,
        "bms_window": BMS_WINDOW,
        "hud_rect": list(HUD_RECT),
    }
    if contract != expected_contract:
        fail("M98U_CONTRACT_VALUES", "serialized contract values differ")
    counts = document["counts"]
    if len(counts) != MAX_INSTANCES:
        fail("M98U_COUNT_MATRIX", "count matrix must contain 16 sections")
    records_generated = 0
    draw_orders_generated = 0
    tie_example = None
    prior_phases: dict[tuple[int, int], int] = {}
    count_one_mismatches = 0
    for active_count, count_section in enumerate(counts, 1):
        require_keys(count_section, (
            "active_count", "phase_offsets", "circular_gaps", "states"),
            "M98U_COUNT_FIELDS")
        if count_section["active_count"] != active_count:
            fail("M98U_COUNT_ORDER", "active count section is reordered")
        offsets = expected_offsets(active_count)
        if tuple(count_section["phase_offsets"]) != offsets:
            fail("M98U_OFFSET_FORMULA", "serialized offsets differ from floor formula")
        if len(set(offsets)) != active_count:
            fail("M98U_PHASE_UNIQUE", "expected offsets are not unique")
        gaps = expected_gaps(offsets)
        if tuple(count_section["circular_gaps"]) != gaps:
            fail("M98U_GAP_VALUES", "serialized circular gaps differ")
        if sum(gaps) != 64:
            fail("M98U_GAP_SUM", "circular gaps do not sum to 64")
        lower = 64 // active_count
        upper = (64 + active_count - 1) // active_count
        if any(gap not in (lower, upper) for gap in gaps):
            fail("M98U_GAP_BALANCE", "circular gap is outside floor/ceiling bounds")
        states = count_section["states"]
        if len(states) != PHASE_COUNT:
            fail("M98U_GLOBAL_PHASE_MATRIX", "count section must contain 64 states")
        starting_phases = {}
        for global_phase, state in enumerate(states):
            require_keys(state, ("global_phase", "records", "draw_order"),
                         "M98U_STATE_FIELDS")
            if state["global_phase"] != global_phase:
                fail("M98U_GLOBAL_PHASE_ORDER", "global phase state is reordered")
            records = state["records"]
            if len(records) != active_count:
                fail("M98U_RECORD_COUNT", "record count differs from active count")
            expected_records = []
            for instance_id, record in enumerate(records):
                if not isinstance(record, dict):
                    fail("M98U_RECORD_TYPE", "record must be an object")
                require_keys(record, RECORD_FIELDS, "M98U_RECORD_FIELDS")
                expected = expected_record(active_count, global_phase,
                                           instance_id, header, entries,
                                           descriptors)
                if record != expected:
                    if active_count == 1:
                        fail("M98U_COUNT_ONE_MISMATCH",
                             "count-one record differs from accepted M98t")
                    fail("M98U_RECORD_IDENTITY", "derived record differs from M98t")
                if any(not isinstance(value, int) or isinstance(value, bool)
                       for value in record.values()):
                    fail("M98U_RECORD_NUMERIC", "record fields must be decimal integers")
                expected_records.append(expected)
                key = (active_count, instance_id)
                if global_phase == 0:
                    starting_phases[key] = record["phase_id"]
                else:
                    prior = prior_phases[key]
                    if record["phase_id"] != ((prior + 1) & 63):
                        fail("M98U_ROTATION_COVARIANCE", "instance phase did not advance by one")
                prior_phases[key] = record["phase_id"]
            if len({record["phase_id"] for record in records}) != active_count:
                fail("M98U_PHASE_UNIQUE", "serialized instance phases are not unique")
            order = state["draw_order"]
            if len(order) != active_count or sorted(order) != list(range(active_count)):
                fail("M98U_DRAW_ORDER_PERMUTATION", "draw order is not a permutation")
            expected_order = sorted(range(active_count), key=lambda index: (
                records[index]["depth_rank"], records[index]["instance_id"]))
            if order != expected_order:
                keys = [(records[index]["depth_rank"],
                         records[index]["instance_id"]) for index in order]
                if any(keys[index][0] == keys[index + 1][0]
                       and keys[index][1] >= keys[index + 1][1]
                       for index in range(len(keys) - 1)):
                    fail("M98U_DRAW_ORDER_TIE", "equal-depth IDs are not ascending")
                fail("M98U_DRAW_ORDER_KEY", "draw order is not signed depth then ID")
            if tie_example is None:
                for first, second in zip(order, order[1:]):
                    if records[first]["depth_rank"] == records[second]["depth_rank"]:
                        tie_example = {
                            "active_count": active_count,
                            "global_phase": global_phase,
                            "depth_rank": records[first]["depth_rank"],
                            "ordered_instance_ids": [
                                records[first]["instance_id"],
                                records[second]["instance_id"],
                            ],
                        }
                        break
            records_generated += active_count
            draw_orders_generated += 1
        for instance_id in range(active_count):
            final_phase = prior_phases[(active_count, instance_id)]
            if ((final_phase + 1) & 63) != starting_phases[(active_count, instance_id)]:
                fail("M98U_ROTATION_WRAP", "instance does not return after 64 phases")
    if tie_example is None:
        fail("M98U_TIE_COVERAGE", "matrix contains no equal-depth tie")
    expected_summary = {
        "max_instances": 16,
        "counts_tested": 16,
        "global_phases_tested": 64,
        "count_phase_combinations": 1024,
        "instance_records_generated": records_generated,
        "draw_orders_generated": draw_orders_generated,
        "unique_phase_failures": 0,
        "gap_balance_failures": 0,
        "descriptor_failures": 0,
        "bounds_failures": 0,
        "hud_overlap_failures": 0,
        "source_range_failures": 0,
        "permutation_failures": 0,
        "depth_order_failures": 0,
        "tie_break_failures": 0,
        "count_one_mismatches": count_one_mismatches,
        "determinism_mismatches": 0,
        "private_data_findings": 0,
        "tie_example": tie_example,
    }
    if records_generated != 8704 or draw_orders_generated != 1024:
        fail("M98U_EXHAUSTIVE_COUNTS", "exhaustive matrix totals differ")
    if document["summary"] != expected_summary:
        fail("M98U_SUMMARY", "serialized summary differs from independent totals")
    return expected_summary


def inspect(golden_path: Path, atlas_path: Path, table_path: Path,
            contract_path: Path):
    raw, document = read_json(golden_path)
    find_private_metadata(document)
    atlas = atlas_format.read_regular_file(atlas_path)
    header, descriptors = atlas_format.inspect_bytes(atlas)
    _, entries, _, checked_descriptors = depth_table.inspect(table_path,
                                                              atlas_path)
    if descriptors != checked_descriptors:
        fail("M98U_DESCRIPTOR_IDENTITY", "accepted descriptor paths disagree")
    if header.required_bank_count != 1 or header.first_bank_value != 1:
        fail("M98U_ATLAS_BANK_CONTRACT", "atlas is not the accepted one-bank contract")
    summary = validate_document(document, header, entries, descriptors)
    contract = validate_contract_include(contract_path)
    return raw, summary, contract


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--golden", type=Path, required=True)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--depth-table", type=Path, required=True)
    parser.add_argument("--contract", type=Path, required=True)
    args = parser.parse_args()
    try:
        raw, summary, contract = inspect(args.golden, args.atlas,
                                         args.depth_table, args.contract)
    except (ReferenceError, atlas_format.AtlasError,
            depth_table.DepthTableError) as error:
        print(error)
        return 1
    print("M98U_MULTI_INSTANCE_VALIDATION_PASS "
          f"combinations={summary['count_phase_combinations']} "
          f"records={summary['instance_records_generated']} "
          f"draw_orders={summary['draw_orders_generated']} "
          f"golden_sha256={hashlib.sha256(raw).hexdigest()} "
          f"contract_sha256={hashlib.sha256(contract).hexdigest()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

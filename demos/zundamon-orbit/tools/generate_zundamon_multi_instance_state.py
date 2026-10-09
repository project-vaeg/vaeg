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

"""Generate the bounded M98u multi-instance state reference."""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import validate_zundamon_orbit_depth_table as depth_table  # noqa: E402

MAX_ZUNDAMON_INSTANCES = 16
PHASE_COUNT = 64
SCALE_COUNT = 30
SCREEN_WIDTH = 320
SCREEN_HEIGHT = 200
SCREEN_CENTER_X = 160
SCREEN_CENTER_Y = 100
SCREEN_PITCH = 320
G1_PAGE_BYTES = 64000
G1_PAGE_BASES = (0x220000, 0x22FA00)
BMS_WINDOW = 0x080000
BMS_BANK_BYTES = 0x020000
HUD_RECT = (4, 4, 70, 20)
UINT16_MAX = 0xFFFF

# Field order and offsets are the future 16-bit guest ABI. The active prefix
# contains exactly active_count records; draw order contains byte indices.
RECORD_LAYOUT = (
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
INSTANCE_RECORD_BYTES = 50
DRAW_ORDER_INDEX_BYTES = 1


class MultiInstanceError(ValueError):
    """One stable M98u input or state failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> None:
    raise MultiInstanceError(code, detail)


@dataclass(frozen=True)
class InstanceRecord:
    instance_id: int
    phase_offset: int
    phase_id: int
    scale_id: int
    depth_rank: int
    bms_bank: int
    descriptor_index: int
    reserved: int
    dx: int
    dy: int
    width: int
    height: int
    pitch: int
    anchor_x: int
    anchor_y: int
    target_anchor_x: int
    target_anchor_y: int
    dst_x: int
    dst_y: int
    dst_x1: int
    dst_y1: int
    bank_offset: int
    sgp_source: int
    payload_bytes: int
    source_identity: int


@dataclass(frozen=True)
class InstanceState:
    active_count: int
    global_phase: int
    records: tuple[InstanceRecord, ...]
    draw_order: tuple[int, ...]
    circular_gaps: tuple[int, ...]


def checked_u16_product(left: int, right: int) -> int:
    if left < 0 or right < 0 or left > UINT16_MAX or right > UINT16_MAX:
        fail("M98U_U16_MULTIPLY_INPUT", "multiplication input is outside unsigned 16-bit range")
    product = left * right
    if product > UINT16_MAX:
        fail("M98U_U16_MULTIPLY_OVERFLOW", "multiplication exceeds unsigned 16-bit range")
    return product


def validate_active_count(active_count: int) -> None:
    if not 1 <= active_count <= MAX_ZUNDAMON_INSTANCES:
        fail("M98U_ACTIVE_COUNT_RANGE", "active count must be 1 through 16")


def validate_global_phase(global_phase: int) -> None:
    if not 0 <= global_phase < PHASE_COUNT:
        fail("M98U_GLOBAL_PHASE_RANGE", "global phase must be 0 through 63")


def phase_offset(active_count: int, instance_id: int) -> int:
    validate_active_count(active_count)
    if not 0 <= instance_id < active_count:
        fail("M98U_INSTANCE_ID_RANGE", "instance ID is outside the active prefix")
    return checked_u16_product(PHASE_COUNT, instance_id) // active_count


def expected_offsets(active_count: int) -> tuple[int, ...]:
    return tuple(phase_offset(active_count, instance_id)
                 for instance_id in range(active_count))


def validate_circular_gaps(active_count: int,
                           gaps: tuple[int, ...]) -> None:
    validate_active_count(active_count)
    if len(gaps) != active_count:
        fail("M98U_GAP_COUNT", "circular gap count differs from active count")
    if sum(gaps) != PHASE_COUNT:
        fail("M98U_GAP_SUM", "circular gaps do not sum to 64")
    lower = PHASE_COUNT // active_count
    upper = (PHASE_COUNT + active_count - 1) // active_count
    if any(gap not in (lower, upper) for gap in gaps):
        fail("M98U_GAP_BALANCE", "circular gap is outside floor/ceiling bounds")


def validate_offsets(active_count: int, offsets: tuple[int, ...]) -> tuple[int, ...]:
    validate_active_count(active_count)
    if len(offsets) != active_count:
        fail("M98U_OFFSET_COUNT", "offset count differs from active count")
    if offsets != expected_offsets(active_count):
        fail("M98U_OFFSET_FORMULA", "phase offsets differ from floor(64*i/n)")
    if len(set(offsets)) != active_count:
        fail("M98U_PHASE_UNIQUE", "phase offsets are not unique")
    gaps = tuple(offsets[index + 1] - offsets[index]
                 for index in range(active_count - 1))
    gaps += (PHASE_COUNT - offsets[-1] + offsets[0],)
    validate_circular_gaps(active_count, gaps)
    return gaps


def rectangles_intersect(left: tuple[int, int, int, int],
                         right: tuple[int, int, int, int]) -> bool:
    return (left[0] < right[2] and right[0] < left[2]
            and left[1] < right[3] and right[1] < left[3])


def validate_phase_ids(active_count: int, phase_ids: tuple[int, ...]) -> None:
    if len(phase_ids) != active_count:
        fail("M98U_PHASE_COUNT", "instance phase count differs from active count")
    if any(not 0 <= phase_id < PHASE_COUNT for phase_id in phase_ids):
        fail("M98U_INSTANCE_PHASE_RANGE", "instance phase is outside 0 through 63")
    if len(set(phase_ids)) != active_count:
        fail("M98U_PHASE_UNIQUE", "instance phases are not unique")


def validate_g1_destination(page_base: int,
                            rect: tuple[int, int, int, int]) -> None:
    x0, y0, x1, y1 = rect
    first_offset = y0 * SCREEN_PITCH + x0
    end_offset = (y1 - 1) * SCREEN_PITCH + x1
    if first_offset < 0 or end_offset > G1_PAGE_BYTES or end_offset <= first_offset:
        fail("M98U_G1_PAGE_BOUNDS", "instance destination is outside a G1 page")
    if page_base < 0 or page_base + end_offset > 0xFFFFFFFF:
        fail("M98U_G1_ADDRESS_OVERFLOW", "instance destination address exceeds 32 bits")


def validate_shared_inputs(header, entries, descriptors) -> None:
    if len(entries) != PHASE_COUNT or tuple(entry.phase for entry in entries) != tuple(range(PHASE_COUNT)):
        fail("M98U_PHASE_TABLE", "accepted phase table is incomplete or reordered")
    if len(descriptors) != SCALE_COUNT:
        fail("M98U_DESCRIPTOR_COUNT", "accepted atlas must contain 30 descriptors")
    if header.required_bank_count != 1 or header.first_bank_value != 1:
        fail("M98U_ATLAS_BANK_CONTRACT", "accepted atlas must use one selector-1 bank")
    for entry in entries:
        if not 1 <= entry.scale_id <= SCALE_COUNT:
            fail("M98U_SCALE_RANGE", "phase scale is outside 1 through 30")
        if entry.depth_rank != 2 * entry.scale_id - 31:
            fail("M98U_DEPTH_SCALE_MISMATCH", "phase depth and scale disagree")


def derive_record(active_count: int, global_phase: int, instance_id: int,
                  header, entries, descriptors) -> InstanceRecord:
    validate_active_count(active_count)
    validate_global_phase(global_phase)
    offset = phase_offset(active_count, instance_id)
    phase_id = (global_phase + offset) & (PHASE_COUNT - 1)
    entry = entries[phase_id]
    if entry.phase != phase_id:
        fail("M98U_PHASE_TABLE", "phase lookup identity differs")
    if not 1 <= entry.scale_id <= SCALE_COUNT:
        fail("M98U_SCALE_RANGE", "phase scale is outside 1 through 30")
    descriptor_index = entry.scale_id - 1
    descriptor = descriptors[descriptor_index]
    if descriptor.payload_bytes != descriptor.pitch * descriptor.height:
        fail("M98U_DESCRIPTOR_PAYLOAD", "descriptor payload length differs")
    if descriptor.pitch < descriptor.width or descriptor.width < 1 or descriptor.height < 1:
        fail("M98U_DESCRIPTOR_GEOMETRY", "descriptor geometry is invalid")
    if not 0 <= descriptor.anchor_x < descriptor.width or not 0 <= descriptor.anchor_y < descriptor.height:
        fail("M98U_DESCRIPTOR_ANCHOR", "descriptor anchor is invalid")
    if descriptor.bank_slot != 0:
        fail("M98U_ATLAS_BANK_CONTRACT", "descriptor does not use the shared bank")
    if descriptor.bank_offset < 0 or descriptor.bank_offset + descriptor.payload_bytes > BMS_BANK_BYTES:
        fail("M98U_SOURCE_RANGE", "descriptor source is outside the loaded BMS bank")
    target_anchor_x = SCREEN_CENTER_X + entry.dx
    target_anchor_y = SCREEN_CENTER_Y + entry.dy
    dst_x = target_anchor_x - descriptor.anchor_x
    dst_y = target_anchor_y - descriptor.anchor_y
    dst_x1 = dst_x + descriptor.width
    dst_y1 = dst_y + descriptor.height
    rect = (dst_x, dst_y, dst_x1, dst_y1)
    if not (0 <= dst_x < dst_x1 <= SCREEN_WIDTH
            and 0 <= dst_y < dst_y1 <= SCREEN_HEIGHT):
        fail("M98U_DESTINATION_BOUNDS", "instance rectangle is outside 320x200")
    if rectangles_intersect(rect, HUD_RECT):
        fail("M98U_HUD_INTERSECTION", "instance rectangle intersects the HUD")
    for page_base in G1_PAGE_BASES:
        validate_g1_destination(page_base, rect)
    bms_bank = header.first_bank_value + descriptor.bank_slot
    sgp_source = BMS_WINDOW + descriptor.bank_offset
    return InstanceRecord(
        instance_id=instance_id,
        phase_offset=offset,
        phase_id=phase_id,
        scale_id=entry.scale_id,
        depth_rank=entry.depth_rank,
        bms_bank=bms_bank,
        descriptor_index=descriptor_index,
        reserved=0,
        dx=entry.dx,
        dy=entry.dy,
        width=descriptor.width,
        height=descriptor.height,
        pitch=descriptor.pitch,
        anchor_x=descriptor.anchor_x,
        anchor_y=descriptor.anchor_y,
        target_anchor_x=target_anchor_x,
        target_anchor_y=target_anchor_y,
        dst_x=dst_x,
        dst_y=dst_y,
        dst_x1=dst_x1,
        dst_y1=dst_y1,
        bank_offset=descriptor.bank_offset,
        sgp_source=sgp_source,
        payload_bytes=descriptor.payload_bytes,
        source_identity=descriptor.frame_crc32,
    )


def bounded_insertion_order(records: tuple[InstanceRecord, ...]) -> tuple[int, ...]:
    if not 1 <= len(records) <= MAX_ZUNDAMON_INSTANCES:
        fail("M98U_SORT_CAPACITY", "record list exceeds fixed sort capacity")
    if tuple(record.instance_id for record in records) != tuple(range(len(records))):
        fail("M98U_RECORD_IDS", "records are not in ascending instance-ID order")
    scratch = [0] * MAX_ZUNDAMON_INSTANCES
    for position in range(len(records)):
        scratch[position] = position
    for position in range(1, len(records)):
        candidate = scratch[position]
        candidate_key = (records[candidate].depth_rank,
                         records[candidate].instance_id)
        scan = position
        while scan > 0:
            prior = scratch[scan - 1]
            prior_key = (records[prior].depth_rank, records[prior].instance_id)
            if prior_key <= candidate_key:
                break
            scratch[scan] = prior
            scan -= 1
        scratch[scan] = candidate
    return tuple(scratch[:len(records)])


def validate_draw_order(records: tuple[InstanceRecord, ...],
                        order: tuple[int, ...]) -> None:
    count = len(records)
    if len(order) != count or sorted(order) != list(range(count)):
        fail("M98U_DRAW_ORDER_PERMUTATION", "draw order is not a complete permutation")
    keys = tuple((records[index].depth_rank, records[index].instance_id)
                 for index in order)
    for index in range(count - 1):
        if (keys[index][0] == keys[index + 1][0]
                and keys[index][1] >= keys[index + 1][1]):
            fail("M98U_DRAW_ORDER_TIE", "equal-depth IDs are not ascending")
    if any(keys[index] > keys[index + 1] for index in range(count - 1)):
        fail("M98U_DRAW_ORDER_KEY", "draw order is not far-to-near with ID ties")


def build_state(active_count: int, global_phase: int, header, entries,
                descriptors) -> InstanceState:
    validate_shared_inputs(header, entries, descriptors)
    validate_global_phase(global_phase)
    offsets = expected_offsets(active_count)
    gaps = validate_offsets(active_count, offsets)
    records = tuple(derive_record(active_count, global_phase, instance_id,
                                  header, entries, descriptors)
                    for instance_id in range(active_count))
    validate_phase_ids(active_count,
                       tuple(record.phase_id for record in records))
    order = bounded_insertion_order(records)
    validate_draw_order(records, order)
    return InstanceState(active_count, global_phase, records, order, gaps)


def encode_contract_include() -> bytes:
    lines = [
        "; Copyright (c) 2026 Nakata Maho",
        ";",
        "; Redistribution and use in source and binary forms, with or without",
        "; modification, are permitted provided that the following conditions are met:",
        "; 1. Redistributions of source code must retain the above copyright notice,",
        ";    this list of conditions and the following disclaimer.",
        "; 2. Redistributions in binary form must reproduce the above copyright notice,",
        ";    this list of conditions and the following disclaimer in the documentation",
        ";    and/or other materials provided with the distribution.",
        ";",
        "; THIS SOFTWARE IS PROVIDED BY THE AUTHOR \"AS IS\" AND ANY EXPRESS OR IMPLIED",
        "; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF",
        "; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO",
        "; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,",
        "; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,",
        "; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;",
        "; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,",
        "; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR",
        "; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF",
        "; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.",
        ";",
        "; Generated by generate_zundamon_multi_instance_state.py. Do not edit.",
        "; Reference-only M98u ABI; the accepted one-instance guest does not include it.",
        "; phase=(global_phase+floor(64*instance_id/active_count))&63",
        "; draw key=(signed depth_rank ascending, instance_id ascending)",
        f"%define M98U_MAX_ZUNDAMON_INSTANCES {MAX_ZUNDAMON_INSTANCES}",
        f"%define M98U_PHASE_COUNT {PHASE_COUNT}",
        f"%define M98U_INSTANCE_RECORD_BYTES {INSTANCE_RECORD_BYTES}",
        f"%define M98U_INSTANCE_RECORD_CAPACITY_BYTES {INSTANCE_RECORD_BYTES * MAX_ZUNDAMON_INSTANCES}",
        f"%define M98U_DRAW_ORDER_INDEX_BYTES {DRAW_ORDER_INDEX_BYTES}",
        f"%define M98U_DRAW_ORDER_CAPACITY_BYTES {MAX_ZUNDAMON_INSTANCES}",
    ]
    for name, offset, size, signed in RECORD_LAYOUT:
        kind = "signed" if signed else "unsigned"
        lines.append(f"%define M98U_RECORD_{name.upper()} {offset} ; {kind} {size}-byte")
    return ("\n".join(lines) + "\n").encode("ascii")


def canonical_json(document) -> bytes:
    return (json.dumps(document, ensure_ascii=True, indent=2,
                       separators=(",", ": ")) + "\n").encode("utf-8")


def build_reference_document(header, entries, descriptors):
    validate_shared_inputs(header, entries, descriptors)
    count_sections = []
    record_count = 0
    draw_order_count = 0
    tie_example = None
    for active_count in range(1, MAX_ZUNDAMON_INSTANCES + 1):
        offsets = expected_offsets(active_count)
        gaps = validate_offsets(active_count, offsets)
        states = []
        for global_phase in range(PHASE_COUNT):
            state = build_state(active_count, global_phase, header, entries,
                                descriptors)
            records = [asdict(record) for record in state.records]
            states.append({
                "global_phase": global_phase,
                "records": records,
                "draw_order": list(state.draw_order),
            })
            record_count += len(records)
            draw_order_count += 1
            if tie_example is None:
                for first, second in zip(state.draw_order, state.draw_order[1:]):
                    if state.records[first].depth_rank == state.records[second].depth_rank:
                        tie_example = {
                            "active_count": active_count,
                            "global_phase": global_phase,
                            "depth_rank": state.records[first].depth_rank,
                            "ordered_instance_ids": [
                                state.records[first].instance_id,
                                state.records[second].instance_id,
                            ],
                        }
                        break
        count_sections.append({
            "active_count": active_count,
            "phase_offsets": list(offsets),
            "circular_gaps": list(gaps),
            "states": states,
        })
    if tie_example is None:
        fail("M98U_TIE_COVERAGE", "exhaustive reference did not exercise a depth tie")
    summary = {
        "max_instances": MAX_ZUNDAMON_INSTANCES,
        "counts_tested": MAX_ZUNDAMON_INSTANCES,
        "global_phases_tested": PHASE_COUNT,
        "count_phase_combinations": MAX_ZUNDAMON_INSTANCES * PHASE_COUNT,
        "instance_records_generated": record_count,
        "draw_orders_generated": draw_order_count,
        "unique_phase_failures": 0,
        "gap_balance_failures": 0,
        "descriptor_failures": 0,
        "bounds_failures": 0,
        "hud_overlap_failures": 0,
        "source_range_failures": 0,
        "permutation_failures": 0,
        "depth_order_failures": 0,
        "tie_break_failures": 0,
        "count_one_mismatches": 0,
        "determinism_mismatches": 0,
        "private_data_findings": 0,
        "tie_example": tie_example,
    }
    if record_count != 8704 or draw_order_count != 1024:
        fail("M98U_EXHAUSTIVE_COUNTS", "exhaustive reference totals differ")
    document = {
        "schema": "zundamon-orbit-m98u-multi-instance-state-v1",
        "contract": {
            "max_instances": MAX_ZUNDAMON_INSTANCES,
            "phase_count": PHASE_COUNT,
            "scale_count": SCALE_COUNT,
            "instance_record_bytes": INSTANCE_RECORD_BYTES,
            "draw_order_index_bytes": DRAW_ORDER_INDEX_BYTES,
            "record_capacity_bytes": INSTANCE_RECORD_BYTES * MAX_ZUNDAMON_INSTANCES,
            "draw_order_capacity_bytes": MAX_ZUNDAMON_INSTANCES,
            "bms_bank_bytes": BMS_BANK_BYTES,
            "bms_window": BMS_WINDOW,
            "hud_rect": list(HUD_RECT),
        },
        "counts": count_sections,
        "summary": summary,
    }
    return document, summary


def load_inputs(table_path: Path, atlas_path: Path):
    atlas = atlas_format.read_regular_file(atlas_path)
    header, descriptors = atlas_format.inspect_bytes(atlas)
    _, entries, _, checked_descriptors = depth_table.inspect(table_path, atlas_path)
    if descriptors != checked_descriptors:
        fail("M98U_DESCRIPTOR_IDENTITY", "depth validator and atlas descriptors differ")
    return header, entries, descriptors


def refuse_existing(*paths: Path) -> None:
    if any(path.exists() for path in paths):
        fail("M98U_OUTPUT_EXISTS", "refusing to overwrite generated output")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--depth-table", type=Path, required=True)
    parser.add_argument("--golden-output", type=Path, required=True)
    parser.add_argument("--summary-output", type=Path, required=True)
    parser.add_argument("--contract-output", type=Path, required=True)
    args = parser.parse_args()
    try:
        refuse_existing(args.golden_output, args.summary_output,
                        args.contract_output)
        header, entries, descriptors = load_inputs(args.depth_table, args.atlas)
        document, summary = build_reference_document(header, entries, descriptors)
        golden = canonical_json(document)
        digest = hashlib.sha256(golden).hexdigest()
        qa_summary = dict(summary)
        qa_summary["canonical_golden_sha256"] = digest
        args.golden_output.write_bytes(golden)
        args.summary_output.write_bytes(canonical_json(qa_summary))
        args.contract_output.write_bytes(encode_contract_include())
    except (MultiInstanceError, atlas_format.AtlasError,
            depth_table.DepthTableError) as error:
        print(error)
        return 1
    print("M98U_MULTI_INSTANCE_GENERATION_PASS "
          f"combinations={summary['count_phase_combinations']} "
          f"records={summary['instance_records_generated']} "
          f"draw_orders={summary['draw_orders_generated']} "
          f"sha256={digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

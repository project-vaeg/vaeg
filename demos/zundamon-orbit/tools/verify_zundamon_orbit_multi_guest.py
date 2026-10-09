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

"""Independent M98w full/dirty multi-instance guest oracle."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import generate_zundamon_multi_instance_state as multi  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import validate_zundamon_orbit_depth_table as depth_format  # noqa: E402
import validate_zundamon_orbit_hud as hud_format  # noqa: E402
import verify_zundamon_orbit_depth_guest as depth_oracle  # noqa: E402
import verify_zundamon_orbit_ellipse_guest as ellipse  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402
import zundamon_dirty_union as dirty_union  # noqa: E402

PHASE_COUNT = 64
COUNTS = (1, 2, 4, 8, 16)
NOMINAL_FIELDS = ("60 ", "30 ", "20 ", "15 ", "12 ", "10 ", "8.6", "7.5")
TRACE_CLS = re.compile(r"^SGP_SCAN: CLS addr=([0-9a-fA-F]+) words=(\d+)$")
HUD_RECT = (4, 4, 70, 20)


class OracleError(ValueError):
    pass


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def prefixes(active_count: int, divisor: int, initial_page: str,
             revolutions: int, scenario: str, milestone: str = "m98v"):
    root = (f"{milestone}-n{active_count}-{scenario}-v{divisor}-"
            f"{initial_page}-r{revolutions}")
    publications = PHASE_COUNT * revolutions
    flips = tuple(f"{root}-flip-{index:03d}"
                  for index in range(1, publications + 1))
    settled = (f"{root}-settled-a", f"{root}-settled-b")
    report_letters = ("abcdefghijklmnopqrs" if milestone == "m98w"
                      else "abcdefghijklmno")
    reports = tuple(f"{root}-report-{letter}" for letter in report_letters)
    return root, flips, settled, reports


def page_for(initial_page: int, publication: int) -> int:
    return initial_page ^ (publication & 1)


def build_g0(full_tile: bytes, count_tile: bytes) -> bytes:
    g0 = bytearray(baseline.expected_g0())
    for row in range(16):
        start = (4 + row) * baseline.PITCH + 4
        g0[start:start + 66] = full_tile[row * 66:(row + 1) * 66]
    for row in range(8):
        start = (12 + row) * baseline.PITCH + 58
        g0[start:start + 12] = count_tile[row * 12:(row + 1) * 12]
    return bytes(g0)


def composite_layers(layers):
    page = bytearray(baseline.PAGE_BYTES)
    owner = [-1] * baseline.PAGE_BYTES
    opaque_overlaps = transparent_over_far = 0
    for instance_id, dst_x, dst_y, width, height, pitch, frame in layers:
        for row in range(height):
            source = row * pitch
            target = (dst_y + row) * baseline.PITCH + dst_x
            for x in range(width):
                value = frame[source + x]
                offset = target + x
                if value:
                    if page[offset]:
                        opaque_overlaps += 1
                    page[offset] = value
                    owner[offset] = instance_id
                elif page[offset]:
                    transparent_over_far += 1
    return bytes(page), owner, {
        "opaque_overlap_pixels": opaque_overlaps,
        "transparent_over_far_samples": transparent_over_far,
    }


def compose_g1(atlas: bytes, descriptors, state: multi.InstanceState):
    layers = []
    for index in state.draw_order:
        record = state.records[index]
        descriptor = descriptors[record.descriptor_index]
        frame = atlas[descriptor.file_offset:
                      descriptor.file_offset + descriptor.payload_bytes]
        layers.append((record.instance_id, record.dst_x, record.dst_y,
                       record.width, record.height, record.pitch, frame))
    return composite_layers(layers)


def expected_trace(header, entries, descriptors, active_count: int,
                   initial_page: int, revolutions: int,
                   clear_mode: str = "full"):
    commands: list[tuple[object, ...]] = [
        ("CLS", baseline.PAGE_SGP[0], baseline.PAGE_BYTES // 2),
        ("CLS", baseline.PAGE_SGP[1], baseline.PAGE_BYTES // 2),
    ]
    for publication, global_phase in enumerate(
            tuple(range(PHASE_COUNT)) * revolutions, 1):
        page = page_for(initial_page, publication)
        if clear_mode == "full":
            commands.append(("CLS", baseline.PAGE_SGP[page], baseline.PAGE_BYTES // 2))
        elif clear_mode == "dirty" and publication > 2:
            old_state = multi.build_state(active_count,
                                          (publication - 3) % PHASE_COUNT,
                                          header, entries, descriptors)
            old_rectangles = [(record.dst_x, record.dst_y,
                               record.width, record.height)
                              for record in old_state.records]
            commands.extend(("CLS", address, words)
                            for address, words in dirty_union.all_rows(
                                old_rectangles, baseline.PAGE_SGP[page]))
        state = multi.build_state(active_count, global_phase, header,
                                  entries, descriptors)
        for index in state.draw_order:
            record = state.records[index]
            commands.append(("SOURCE", record.sgp_source, 0, 2, record.width,
                             record.height, record.pitch))
            commands.append(("DEST", baseline.PAGE_SGP[page]
                             + record.dst_y * baseline.PITCH
                             + (record.dst_x & ~1), record.dst_x & 1, 2,
                             record.width, record.height, baseline.PITCH))
    return commands


def check_trace(path: Path, header, entries, descriptors, active_count: int,
                initial_page: int, revolutions: int, errors: list[str],
                clear_mode: str = "full"):
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98V_TRACE_MISSING")
        return {}
    actual = []
    for line in lines:
        if match := TRACE_CLS.match(line):
            actual.append(("CLS", int(match.group(1), 16), int(match.group(2))))
        elif match := baseline.TRACE_SOURCE.match(line):
            actual.append(("SOURCE",) + tuple(
                int(match.group(index), 16 if index == 1 else 10)
                for index in range(1, 7)))
        elif match := baseline.TRACE_DESTINATION.match(line):
            actual.append(("DEST",) + tuple(
                int(match.group(index), 16 if index == 1 else 10)
                for index in range(1, 7)))
    expected = expected_trace(header, entries, descriptors, active_count,
                              initial_page, revolutions, clear_mode)
    if actual != expected:
        add_error(errors, "M98V_TRACE_COMMAND_SEQUENCE")
    return {
        "cls_count": sum(command[0] == "CLS" for command in actual),
        "bitblt_count": sum(command[0] == "SOURCE" for command in actual),
        "command_identity": sha256(json.dumps(actual).encode("ascii")),
    }


def read_registers(path: Path) -> dict[str, str]:
    return ellipse.read_registers(path)


def require_registers(directory: Path, prefix: str,
                      expected: dict[str, int | str], errors: list[str]) -> None:
    try:
        values = read_registers(directory / f"{prefix}.registers.tsv")
    except ellipse.OracleError:
        add_error(errors, "M98V_REGISTERS_SCHEMA")
        return
    contract = {"schema": "vaeg-registers-v1", "cs": "3000", "ds": "3000"}
    contract.update({key: f"{value & 0xffff:04x}" if isinstance(value, int) else value
                     for key, value in expected.items()})
    if any(values.get(key) != value for key, value in contract.items()):
        add_error(errors, "M98V_REGISTER_SIGNATURE")


def source_bytes_for(header, entries, descriptors, active_count: int,
                     revolutions: int) -> int:
    return sum(record.payload_bytes
               for global_phase in tuple(range(PHASE_COUNT)) * revolutions
               for record in multi.build_state(active_count, global_phase,
                                               header, entries,
                                               descriptors).records)


def publication_digest(initial_page: int, active_count: int,
                       revolutions: int) -> int:
    low = high = 0
    for publication, global_phase in enumerate(
            tuple(range(PHASE_COUNT)) * revolutions, 1):
        total = low + global_phase + (active_count << 8)
        low = total & 0xffff
        high = (high + (total >> 16)
                + page_for(initial_page, publication) + 1) & 0xffff
    return low | (high << 16)


def verify(directory: Path, atlas_path: Path, table_path: Path, hud_path: Path,
           trace_path: Path, active_count: int, divisor: int,
           initial_page_name: str, revolutions: int, scenario: str,
           clear_mode: str = "full", milestone: str = "m98v"):
    errors: list[str] = []
    schema_name = ("zundamon-orbit-m98w-oracle-v1" if milestone == "m98w"
                   else "zundamon-orbit-m98v-oracle-v1")
    if active_count not in COUNTS:
        return {"schema": schema_name, "status": "FAIL",
                "errors": ["M98V_ACTIVE_COUNT"]}
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
        baseline.validate_runtime_descriptors(header, descriptors)
        baseline.validate_frame_crcs(atlas, descriptors)
        _, entries, _, _ = depth_format.inspect(table_path, atlas_path)
        _, full_tiles, _ = hud_format.inspect(hud_path)
        count_tiles = hud_format.inspect_count_tiles(hud_path)
    except (OSError, atlas_format.AtlasError, baseline.OracleError,
            depth_format.DepthTableError, hud_format.HudError):
        return {"schema": schema_name, "status": "FAIL",
                "errors": ["M98V_INPUT_CONTRACT"]}
    initial_page = 0 if initial_page_name == "a" else 1
    root, flips, settled, reports = prefixes(active_count, divisor,
                                             initial_page_name, revolutions,
                                             scenario, milestone)
    count = PHASE_COUNT * revolutions
    edge_rows, scheduler = ellipse.scheduler_schedule(divisor, revolutions, scenario)
    ready_wait_edges = (sum(row["active"] - 1 for row in edge_rows)
                        + scheduler["changes"]
                        + scheduler["pause_transitions"] // 2)
    pages = [bytes(baseline.PAGE_BYTES), bytes(baseline.PAGE_BYTES)]
    base_g0 = baseline.expected_g0()
    frame_records = []
    overlap_total = transparent_total = 0
    final_raw = b""
    for publication, (prefix, global_phase, edge_row) in enumerate(
            zip(flips, tuple(range(PHASE_COUNT)) * revolutions, edge_rows), 1):
        state = multi.build_state(active_count, global_phase, header,
                                  entries, descriptors)
        expected_page, _, overlap = compose_g1(atlas, descriptors, state)
        overlap_total += overlap["opaque_overlap_pixels"]
        transparent_total += overlap["transparent_over_far_samples"]
        page = page_for(initial_page, publication)
        pages[page] = expected_page
        # M98x retains the five-count M98v/M98w fixtures but the generated
        # HUD now has a tile for every runtime count 1 through 16.
        g0 = build_g0(full_tiles[edge_row["active"] - 1],
                      count_tiles[active_count - 1])
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98V_GVRAM_MISSING")
            continue
        if len(raw) != baseline.GVRAM_BYTES:
            add_error(errors, "M98V_GVRAM_SIZE")
            continue
        actual_g0 = raw[:baseline.G0_BYTES]
        actual_pages = (
            raw[baseline.G1_OFFSET:baseline.G1_OFFSET + baseline.PAGE_BYTES],
            raw[baseline.G1_OFFSET + baseline.PAGE_BYTES:
                baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES],
        )
        if actual_g0 != g0:
            add_error(errors, "M98V_G0_HUD_CONTENT")
        outside_actual = bytearray(actual_g0)
        outside_base = bytearray(base_g0)
        for row in range(HUD_RECT[1], HUD_RECT[3]):
            start = row * baseline.PITCH + HUD_RECT[0]
            outside_actual[start:start + 66] = b"\0" * 66
            outside_base[start:start + 66] = b"\0" * 66
        if outside_actual != outside_base:
            add_error(errors, "M98V_G0_OUTSIDE_HUD")
        if actual_pages != tuple(pages):
            add_error(errors, "M98V_G1_PAGE_CONTENT")
        if any(raw[baseline.G0_BYTES:baseline.G1_OFFSET]) or any(
                raw[baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES:]):
            add_error(errors, "M98V_GVRAM_GUARD")
        frame_records.append({
            "publication": publication,
            "global_phase": global_phase,
            "active_count": active_count,
            "page": "A" if page == 0 else "B",
            "draw_order": list(state.draw_order),
            "instance_ids": [state.records[index].instance_id
                             for index in state.draw_order],
            "depths": [state.records[index].depth_rank for index in state.draw_order],
            "phases": [record.phase_id for record in state.records],
            "scales": [record.scale_id for record in state.records],
            "g1_sha256": sha256(expected_page),
            "composite_sha256": sha256(baseline.composite(g0, expected_page)),
            **overlap,
        })
        final_raw = raw
    for prefix in settled:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
            if raw != final_raw or not baseline.bmp_nonblack(
                    directory / f"{prefix}.screen.bmp"):
                add_error(errors, "M98V_SETTLED_CAPTURE")
        except (OSError, baseline.OracleError):
            add_error(errors, "M98V_SETTLED_CAPTURE")

    chunks = (header.payload_bytes + baseline.STAGING_BYTES - 1) // baseline.STAGING_BYTES
    phase_zero = multi.build_state(active_count, 0, header, entries, descriptors)
    required: dict[str, dict[str, int | str]] = {
        f"{root}-probe": {"ax": 0x98A1, "bx": 0x01D0, "cx": 0x0080,
                          "dx": 2, "si": 0xA55A, "di": 0x0081, "bp": 0,
                          "ip": 0x4000},
        f"{root}-load": {"ax": 0x98B1, "bx": chunks,
                         "cx": header.payload_bytes & 0xffff,
                         "dx": header.payload_bytes >> 16,
                         "si": baseline.STAGING_BYTES,
                         "di": header.file_size & 0xffff,
                         "bp": header.file_size >> 16, "ip": 0x4010},
        f"{root}-initialize": {
            "ax": 0x98C1, "bx": baseline.PAGE_BYTES, "cx": baseline.PITCH,
            "dx": 0x0105, "si": phase_zero.records[0].sgp_source & 0xffff,
            "di": phase_zero.records[0].sgp_source >> 16, "bp": 0x0201,
            "ip": 0x4020},
    }
    for publication, (prefix, global_phase, edge_row) in enumerate(
            zip(flips, tuple(range(PHASE_COUNT)) * revolutions, edge_rows), 1):
        required[prefix] = {
            "ax": 0x98D1, "bx": publication, "cx": global_phase,
            "dx": active_count | (edge_row["active"] << 8),
            "si": publication * active_count, "ip": 0x4030,
        }
    final_dsa = baseline.PAGE_DSA[initial_page]
    for prefix in settled:
        required[prefix] = {"ax": 0x98D2, "bx": count, "cx": revolutions,
                            "dx": count, "si": count, "di": initial_page,
                            "bp": final_dsa & 0xffff, "ip": 0x4040}
    page_counts = ((count // 2 + 1, count // 2) if initial_page == 0
                   else (count // 2, count // 2 + 1))
    source_bytes = source_bytes_for(header, entries, descriptors,
                                    active_count, revolutions)
    digest = publication_digest(initial_page, active_count, revolutions)
    instances = count * active_count
    hud_bytes = 1056 + 96 + 144 * scheduler["changes"]
    dirty_candidates = dirty_rows = dirty_batches = dirty_commands = 0
    dirty_words = dirty_bytes = 0
    dirty_nonempty = dirty_overlap = dirty_adjacency = dirty_containment = 0
    if clear_mode == "dirty":
        for publication in range(3, count + 1):
            old_state = multi.build_state(active_count, (publication - 3) % PHASE_COUNT,
                                          header, entries, descriptors)
            rectangles = [(record.dst_x, record.dst_y,
                           record.width, record.height)
                          for record in old_state.records]
            row_results = [dirty_union.row_union(rectangles, row)
                           for row in range(dirty_union.HEIGHT)]
            dirty_candidates += sum(len(result["candidates"])
                                    for result in row_results)
            row_counts = [len(result["merged"]) for result in row_results]
            dirty_rows += sum(row_counts)
            dirty_nonempty += sum(value != 0 for value in row_counts)
            dirty_overlap += sum(result["overlap_merges"] for result in row_results)
            dirty_adjacency += sum(result["adjacency_merges"] for result in row_results)
            dirty_containment += sum(result["containment_merges"] for result in row_results)
            dirty_batches += sum((value + 10) // 11 for value in row_counts
                                 if value)
            for result in row_results:
                for interval in result["merged"]:
                    dirty_bytes += interval.x1 - interval.x0
                    dirty_words += (interval.x1 - interval.x0) // 2
        dirty_commands = dirty_rows + 3 * dirty_batches
    expected_lists = (1 + count * (1 + active_count)
                      if clear_mode == "full" else
                      1 + dirty_batches + instances)
    expected_commands = (5 + count * (3 + 4 * active_count)
                         if clear_mode == "full" else
                         5 + dirty_commands + 4 * instances)
    try:
        report_h_actual = read_registers(directory / f"{reports[7]}.registers.tsv")
        report_j_actual = read_registers(directory / f"{reports[9]}.registers.tsv")
        requested_actual = int(report_h_actual["cx"], 16)
        missed_actual = int(report_h_actual["si"], 16)
        ready_wait_actual = int(report_h_actual["di"], 16)
        total_edges_actual = int(report_j_actual["bx"], 16)
        unpaused_edges_actual = int(report_j_actual["cx"], 16)
        paused_edges_actual = int(report_j_actual["dx"], 16)
        if requested_actual != count + missed_actual:
            add_error(errors, "M98V_REQUESTED_SLOT_INVARIANT")
        if total_edges_actual != unpaused_edges_actual + paused_edges_actual:
            add_error(errors, "M98V_VBLANK_COUNTER_INVARIANT")
    except (OSError, KeyError, ValueError, ellipse.OracleError):
        add_error(errors, "M98V_REPORT_TELEMETRY")
        requested_actual = count + (2 if scenario == "missed" else 0)
        missed_actual = 2 if scenario == "missed" else 0
        ready_wait_actual = ready_wait_edges
        total_edges_actual = scheduler["total_edges"]
        unpaused_edges_actual = scheduler["unpaused_edges"]
        paused_edges_actual = scheduler["paused_edges"]
    report_expected = (
        {"ax": 0x98E1, "bx": active_count, "cx": count, "dx": count,
         "si": 2, "di": count if clear_mode == "full" else 0,
         "bp": instances, "ip": 0x4050},
        {"ax": 0x98E2, "bx": 3, "cx": page_counts[0], "dx": page_counts[1],
         "si": 0, "di": 0, "bp": 0, "ip": 0x4060},
        {"ax": 0x98E3, "bx": count * 2, "cx": 1, "dx": initial_page,
         "si": revolutions, "di": count, "bp": 0, "ip": 0x4070},
        {"ax": 0x98E4, "bx": source_bytes & 0xffff,
         "cx": source_bytes >> 16,
         "dx": ((count * 64000) if clear_mode == "full" else 0) & 0xffff,
         "si": ((count * 64000) if clear_mode == "full" else 0) >> 16,
         "di": count,
         "bp": final_dsa & 0xffff, "ip": 0x4080},
        {"ax": 0x98E5, "bx": instances, "cx": instances, "dx": instances,
         "si": instances, "di": 0, "bp": 1, "ip": 0x4090},
        {"ax": 0x98E6, "bx": 0, "cx": 0, "dx": 0, "si": 0, "di": 0,
         "bp": 0, "ip": 0x40A0},
        {"ax": 0x98E7, "bx": ((count * 32000) if clear_mode == "full" else 0) & 0xffff,
         "cx": ((count * 32000) if clear_mode == "full" else 0) >> 16,
         "dx": expected_lists,
         "si": expected_commands,
         "di": (count * (1 + active_count) if clear_mode == "full"
                else dirty_batches + instances), "bp": count, "ip": 0x40B0},
        {"ax": 0x98E8, "bx": count, "cx": requested_actual,
         "dx": count, "si": missed_actual,
         "di": ready_wait_actual, "bp": 0, "ip": 0x40C0},
        {"ax": 0x98E9,
         "bx": scheduler["final_divisor"] | (scheduler["final_divisor"] << 8),
         "cx": scheduler["changes"], "dx": scheduler["changes"],
         "si": scheduler["resets"], "di": scheduler["pause_requests"],
         "bp": scheduler["pause_transitions"], "ip": 0x40D0},
        {"ax": 0x98EA, "bx": total_edges_actual,
         "cx": unpaused_edges_actual, "dx": paused_edges_actual,
         "si": 0, "di": count, "bp": instances, "ip": 0x40E0},
        {"ax": 0x98EB, "bx": digest & 0xffff, "cx": digest >> 16,
         "dx": revolutions, "si": count, "di": 0, "bp": 0, "ip": 0x40F0},
        {"ax": 0x98EC, "bx": 58, "cx": instances // 2,
         "dx": instances // 2, "si": 96, "di": 48, "bp": 0,
         "ip": 0x4100},
        {"ax": 0x98ED, "bx": 1, "cx": 1 + scheduler["changes"], "dx": 1,
         "si": 0, "di": 0, "bp": 0, "ip": 0x4110},
        {"ax": 0x98EE, "bx": hud_bytes & 0xffff, "cx": hud_bytes >> 16,
         "dx": 0, "si": 0, "di": 0, "bp": 1, "ip": 0x4120},
        {"ax": 0x98EF, "bx": 63, "cx": 63, "dx": count, "si": count,
         "di": count, "bp": 1, "ip": 0x4130},
        {"ax": 0x98F0, "bx": (dirty_rows if clear_mode == "dirty" else 0) & 0xffff,
         "cx": (dirty_rows if clear_mode == "dirty" else 0) >> 16,
         "dx": (dirty_rows if clear_mode == "dirty" else 0) & 0xffff,
         "si": (dirty_rows if clear_mode == "dirty" else 0) >> 16,
         "di": (dirty_rows if clear_mode == "dirty" else 0) & 0xffff,
         "bp": 2 if clear_mode == "dirty" else 0, "ip": 0x4140},
        {"ax": 0x98F1, "bx": (dirty_words if clear_mode == "dirty" else 0) & 0xffff,
         "cx": (dirty_words if clear_mode == "dirty" else 0) >> 16,
         "dx": (dirty_bytes if clear_mode == "dirty" else 0) & 0xffff,
         "si": (dirty_bytes if clear_mode == "dirty" else 0) >> 16,
         "di": count, "bp": 0, "ip": 0x4150},
        {"ax": 0x98F2,
         "bx": (200 * (count - 2) if clear_mode == "dirty" else 0),
         "cx": dirty_nonempty if clear_mode == "dirty" else 0,
         "dx": dirty_nonempty if clear_mode == "dirty" else 0,
         "si": dirty_overlap if clear_mode == "dirty" else 0,
         "di": dirty_adjacency if clear_mode == "dirty" else 0,
         "bp": dirty_containment if clear_mode == "dirty" else 0,
         "ip": 0x4160},
        {"ax": 0x98F3,
         "bx": (count - 2 if clear_mode == "dirty" else 0),
         "cx": dirty_batches if clear_mode == "dirty" else 0,
         "dx": instances,
         "si": 0, "di": (count - 2 if clear_mode == "dirty" else 0),
         "bp": (count - 2 if clear_mode == "dirty" else 0),
         "ip": 0x4170},
    )
    for prefix, expected in zip(reports, report_expected):
        required[prefix] = expected
    ordered = ((f"{root}-probe", f"{root}-load", f"{root}-initialize")
               + flips + settled + reports)
    for prefix in ordered:
        require_registers(directory, prefix, required[prefix], errors)
    ellipse.check_events(directory, ordered, errors)
    errors[:] = [code.replace("M98S_", "M98V_", 1)
                 if code.startswith("M98S_") else code for code in errors]
    if milestone == "m98w":
        errors[:] = [code.replace("M98V_", "M98W_", 1)
                     if code.startswith("M98V_") else code for code in errors]
    trace = check_trace(trace_path, header, entries, descriptors, active_count,
                        initial_page, revolutions, errors, clear_mode)
    if len(frame_records) != count:
        add_error(errors, "M98V_FRAME_COUNT")
    histogram = Counter(scale for record in frame_records
                        for scale in record.get("scales", ()))
    expected_histogram = Counter(entry.scale_id for entry in entries)
    expected_histogram = Counter({key: value * active_count * revolutions
                                  for key, value in expected_histogram.items()})
    if histogram != expected_histogram:
        add_error(errors, "M98V_SCALE_HISTOGRAM")
    if clear_mode == "dirty":
        measured_lists = 1 + dirty_batches + instances
        measured_commands = 5 + dirty_commands + 4 * instances
    else:
        measured_lists = 1 + count * (1 + active_count)
        measured_commands = 5 + count * (3 + 4 * active_count)
    return {
        "schema": schema_name,
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
        "active_count": active_count,
        "initial_page": initial_page_name,
        "divisor": divisor,
        "revolutions": revolutions,
        "scenario": scenario,
        "nominal_fps_field": NOMINAL_FIELDS[divisor - 1],
        "vaeg_display_vblank_hz": depth_oracle.VAEG_DISPLAY_HZ,
        "requested_actual_hz": depth_oracle.VAEG_DISPLAY_HZ / divisor,
        "requested_revolution_period_seconds": (
            PHASE_COUNT * divisor / depth_oracle.VAEG_DISPLAY_HZ),
        "publication_rate_hz_from_edge_ratio": (
            depth_oracle.VAEG_DISPLAY_HZ * count / total_edges_actual),
        "measured_revolution_period_seconds_from_edges": (
            total_edges_actual / depth_oracle.VAEG_DISPLAY_HZ / revolutions),
        "vblank_edges_total": total_edges_actual,
        "vblank_edges_unpaused": unpaused_edges_actual,
        "vblank_edges_paused": paused_edges_actual,
        "requested_slots": requested_actual,
        "ready_wait_edges": ready_wait_actual,
        "publications": len(frame_records),
        "instances_published": len(frame_records) * active_count,
        "missed_slots": missed_actual,
        "source_bytes": source_bytes,
        "baseline_full_clear_bytes": count * 64000,
        "full_page_clear_bytes": count * 64000 if clear_mode == "full" else 0,
        "steady_full_page_clears": count if clear_mode == "full" else 0,
        "dirty_rect_clears": count - 2 if clear_mode == "dirty" else 0,
        "dirty_first_use_skips": 2 if clear_mode == "dirty" else 0,
        "dirty_candidate_intervals": dirty_candidates,
        "dirty_merged_intervals": dirty_rows,
        "dirty_row_cls_commands": dirty_rows,
        "dirty_words_cleared": dirty_words,
        "dirty_bytes_cleared": dirty_bytes,
        "dirty_clear_batches": dirty_batches,
        "sgp_command_lists": measured_lists,
        "sgp_commands": measured_commands,
        "overlap_pixels": overlap_total,
        "transparent_over_far_samples": transparent_total,
        "frame_identity": sha256(json.dumps(frame_records,
                                             sort_keys=True).encode("utf-8")),
        "trace": trace,
        "frames": frame_records,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--table", type=Path, required=True)
    parser.add_argument("--hud", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--active-count", choices=COUNTS, type=int, required=True)
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--divisor", choices=range(1, 9), type=int, required=True)
    parser.add_argument("--revolutions", choices=(1, 2), type=int, required=True)
    parser.add_argument("--scenario", choices=("static", "ladder", "pause", "missed"),
                        default="static")
    parser.add_argument("--clear-mode", choices=("full", "dirty"), default="full")
    parser.add_argument("--milestone", choices=("m98v", "m98w"), default="m98v")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result = verify(args.directory, args.atlas, args.table, args.hud, args.trace,
                    args.active_count, args.divisor, args.initial_page,
                    args.revolutions, args.scenario, args.clear_mode,
                    args.milestone)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(json.dumps({key: value for key, value in result.items()
                      if key != "frames"}, sort_keys=True))
    if args.report is not None:
        args.report.write_text(encoded, encoding="utf-8")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

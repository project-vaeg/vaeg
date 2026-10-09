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

"""Independent indexed-frame, depth/scale, HUD, dirty-row, and cadence oracle."""

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
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import validate_zundamon_orbit_depth_table as depth_format  # noqa: E402
import validate_zundamon_orbit_hud as hud_format  # noqa: E402
import verify_zundamon_orbit_dirty_guest as dirty  # noqa: E402
import verify_zundamon_orbit_ellipse_guest as ellipse  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402

PHASE_COUNT = 64
ROWS_PER_BATCH = 11
VAEG_DISPLAY_HZ = 59.95
NOMINAL_FIELDS = ("60 ", "30 ", "20 ", "15 ", "12 ", "10 ", "8.6", "7.5")
TRACE_CLS = re.compile(r"^SGP_SCAN: CLS addr=([0-9a-fA-F]+) words=(\d+)$")
HUD_RECT = (4, 4, 70, 20)


class OracleError(Exception):
    pass


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def phases(revolutions: int) -> tuple[int, ...]:
    return tuple(range(PHASE_COUNT)) * revolutions


def page_for(initial: int, publication: int) -> int:
    return initial ^ (publication & 1)


def destination(entry, descriptor) -> tuple[int, int]:
    return (160 + entry.dx - descriptor.anchor_x,
            100 + entry.dy - descriptor.anchor_y)


def expected_page(atlas: bytes, descriptor, entry) -> bytes:
    x, y = destination(entry, descriptor)
    page = bytearray(baseline.PAGE_BYTES)
    frame = atlas[descriptor.file_offset:
                  descriptor.file_offset + descriptor.payload_bytes]
    for row in range(descriptor.height):
        source = row * descriptor.pitch
        target = (y + row) * baseline.PITCH + x
        page[target:target + descriptor.width] = frame[source:source + descriptor.width]
    return bytes(page)


def g0_with_hud(full_tile: bytes) -> bytes:
    result = bytearray(baseline.expected_g0())
    for row in range(16):
        target = (4 + row) * baseline.PITCH + 4
        result[target:target + 66] = full_tile[row * 66:(row + 1) * 66]
    return bytes(result)


def prefixes(divisor: int, initial_page: str, revolutions: int, scenario: str):
    root = f"m98t-{scenario}-v{divisor}-{initial_page}-r{revolutions}"
    count = PHASE_COUNT * revolutions
    flips = tuple(f"{root}-flip-{index:03d}" for index in range(1, count + 1))
    settled = (f"{root}-settled-a", f"{root}-settled-b")
    reports = tuple(f"{root}-report-{letter}" for letter in "abcdefghijklmno")
    return root, flips, settled, reports


def dirty_work(entries, descriptors, revolutions: int) -> dict[str, int]:
    rows = words = batches = 0
    sequence = phases(revolutions)
    for publication in range(3, len(sequence) + 1):
        old_phase = sequence[publication - 3]
        entry = entries[old_phase]
        descriptor = descriptors[entry.scale_id - 1]
        x, _ = destination(entry, descriptor)
        _, _, row_words = dirty.rounded_interval(x, descriptor.width)
        rows += descriptor.height
        words += descriptor.height * row_words
        batches += (descriptor.height + ROWS_PER_BATCH - 1) // ROWS_PER_BATCH
    return {"rectangles": len(sequence) - 2, "rows": rows, "words": words,
            "bytes": words * 2, "batches": batches}


def publication_digest(initial: int, entries, revolutions: int) -> int:
    low = high = 0
    for publication, phase in enumerate(phases(revolutions), 1):
        total = low + phase + (entries[phase].scale_id << 8)
        low = total & 0xffff
        high = (high + (total >> 16) + page_for(initial, publication) + 1) & 0xffff
    return low | (high << 16)


def expected_trace(entries, descriptors, initial_page: str, revolutions: int,
                   clear_mode: str):
    initial = 0 if initial_page == "a" else 1
    commands: list[tuple[object, ...]] = [
        ("CLS", baseline.PAGE_SGP[0], baseline.PAGE_BYTES // 2),
        ("CLS", baseline.PAGE_SGP[1], baseline.PAGE_BYTES // 2),
    ]
    sequence = phases(revolutions)
    for publication, phase in enumerate(sequence, 1):
        page = page_for(initial, publication)
        if clear_mode == "full":
            commands.append(("CLS", baseline.PAGE_SGP[page], baseline.PAGE_BYTES // 2))
        elif publication > 2:
            old_phase = sequence[publication - 3]
            old_entry = entries[old_phase]
            old_descriptor = descriptors[old_entry.scale_id - 1]
            old_x, old_y = destination(old_entry, old_descriptor)
            clear_x0, _, words = dirty.rounded_interval(
                old_x, old_descriptor.width)
            commands.extend(("CLS", baseline.PAGE_SGP[page] + y * baseline.PITCH
                             + clear_x0, words)
                            for y in range(old_y, old_y + old_descriptor.height))
        entry = entries[phase]
        descriptor = descriptors[entry.scale_id - 1]
        x, y = destination(entry, descriptor)
        commands.append(("SOURCE", baseline.BMS_WINDOW + descriptor.bank_offset,
                         0, 2, descriptor.width, descriptor.height, descriptor.pitch))
        commands.append(("DEST", baseline.PAGE_SGP[page] + y * baseline.PITCH
                         + (x & ~1), x & 1, 2, descriptor.width,
                         descriptor.height, baseline.PITCH))
    return commands


def check_trace(path: Path, entries, descriptors, initial_page: str,
                revolutions: int, clear_mode: str, errors: list[str]):
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98T_TRACE_MISSING")
        return {}
    commands = []
    for line in lines:
        if match := TRACE_CLS.match(line):
            commands.append(("CLS", int(match.group(1), 16), int(match.group(2))))
        elif match := baseline.TRACE_SOURCE.match(line):
            commands.append(("SOURCE",) + tuple(
                int(match.group(index), 16 if index == 1 else 10)
                for index in range(1, 7)))
        elif match := baseline.TRACE_DESTINATION.match(line):
            commands.append(("DEST",) + tuple(
                int(match.group(index), 16 if index == 1 else 10)
                for index in range(1, 7)))
    expected = expected_trace(entries, descriptors, initial_page, revolutions, clear_mode)
    if commands != expected:
        add_error(errors, "M98T_TRACE_COMMAND_SEQUENCE")
    return {"bitblt_count": sum(command[0] == "SOURCE" for command in commands),
            "cls_count": sum(command[0] == "CLS" for command in commands),
            "command_identity": sha256(json.dumps(commands).encode("ascii"))}


def check_frames(directory: Path, atlas: bytes, entries, descriptors, full_tiles,
                 initial_page: str, revolutions: int, edge_rows, flips, settled,
                 errors: list[str]):
    initial = 0 if initial_page == "a" else 1
    pages = [bytes(baseline.PAGE_BYTES), bytes(baseline.PAGE_BYTES)]
    base_g0 = baseline.expected_g0()
    records = []
    final_raw = b""
    for publication, (prefix, phase, edge_row) in enumerate(
            zip(flips, phases(revolutions), edge_rows), 1):
        entry = entries[phase]
        descriptor = descriptors[entry.scale_id - 1]
        page = page_for(initial, publication)
        expected = expected_page(atlas, descriptor, entry)
        pages[page] = expected
        g0 = g0_with_hud(full_tiles[edge_row["active"] - 1])
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98T_GVRAM_MISSING")
            continue
        if len(raw) != baseline.GVRAM_BYTES:
            add_error(errors, "M98T_GVRAM_SIZE")
            continue
        actual_g0 = raw[:baseline.G0_BYTES]
        actual_pages = (
            raw[baseline.G1_OFFSET:baseline.G1_OFFSET + baseline.PAGE_BYTES],
            raw[baseline.G1_OFFSET + baseline.PAGE_BYTES:
                baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES],
        )
        if actual_g0 != g0:
            add_error(errors, "M98T_G0_HUD_CONTENT")
        outside_actual = bytearray(actual_g0)
        outside_base = bytearray(base_g0)
        for row in range(HUD_RECT[1], HUD_RECT[3]):
            start = row * baseline.PITCH + HUD_RECT[0]
            outside_actual[start:start + 66] = b"\0" * 66
            outside_base[start:start + 66] = b"\0" * 66
        if outside_actual != outside_base:
            add_error(errors, "M98T_G0_OUTSIDE_HUD")
        if actual_pages != tuple(pages):
            add_error(errors, "M98T_G1_PAGE_CONTENT")
        if any(raw[baseline.G0_BYTES:baseline.G1_OFFSET]) or any(
                raw[baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES:]):
            add_error(errors, "M98T_GVRAM_GUARD")
        x, y = destination(entry, descriptor)
        records.append({
            "publication": publication, "phase": phase,
            "depth_rank": entry.depth_rank, "scale_id": entry.scale_id,
            "target_anchor": [160 + entry.dx, 100 + entry.dy],
            "destination": [x, y, descriptor.width, descriptor.height],
            "bms_source": baseline.BMS_WINDOW + descriptor.bank_offset,
            "page": "A" if page == 0 else "B",
            "nominal_field": NOMINAL_FIELDS[edge_row["active"] - 1],
            "g1_sha256": sha256(expected),
            "g0_sha256": sha256(g0),
            "composite_sha256": sha256(baseline.composite(g0, expected)),
            "g1_nonzero": sum(value != 0 for value in expected),
            "g1_bbox": baseline.nonzero_bbox(expected),
        })
        final_raw = raw
    for prefix in settled:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
            if raw != final_raw or not baseline.bmp_nonblack(
                    directory / f"{prefix}.screen.bmp"):
                add_error(errors, "M98T_SETTLED_CAPTURE")
        except (OSError, baseline.OracleError):
            add_error(errors, "M98T_SETTLED_CAPTURE")
    try:
        if ((directory / f"{settled[0]}.screen.bmp").read_bytes()
                != (directory / f"{settled[1]}.screen.bmp").read_bytes()):
            add_error(errors, "M98T_SCREEN_UNSTABLE")
    except OSError:
        add_error(errors, "M98T_SETTLED_CAPTURE")
    return records


def read_registers(path: Path) -> dict[str, str]:
    return ellipse.read_registers(path)


def require_registers(directory: Path, prefix: str,
                      expected: dict[str, int | str], errors: list[str]) -> None:
    try:
        values = read_registers(directory / f"{prefix}.registers.tsv")
    except ellipse.OracleError:
        add_error(errors, "M98T_REGISTERS_SCHEMA")
        return
    required = {"schema": "vaeg-registers-v1", "cs": "3000", "ds": "3000"}
    required.update({key: f"{value:04x}" if isinstance(value, int) else value
                     for key, value in expected.items()})
    if any(values.get(key) != value for key, value in required.items()):
        add_error(errors, "M98T_REGISTER_SIGNATURE")


def verify(directory: Path, atlas_path: Path, table_path: Path, hud_path: Path,
           trace_path: Path, source: Path, divisor: int, initial_page: str,
           revolutions: int, scenario: str, clear_mode: str):
    errors: list[str] = []
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
        baseline.validate_runtime_descriptors(header, descriptors)
        baseline.validate_frame_crcs(atlas, descriptors)
        table_raw, entries, rectangles, _ = depth_format.inspect(table_path, atlas_path)
        hud_raw, full_tiles, fps_tiles = hud_format.inspect(hud_path)
    except (OSError, atlas_format.AtlasError, baseline.OracleError,
            depth_format.DepthTableError, hud_format.HudError):
        return {"schema": "zundamon-orbit-m98t-oracle-v1", "status": "FAIL",
                "errors": ["M98T_INPUT_CONTRACT"]}
    root, flips, settled, reports = prefixes(divisor, initial_page, revolutions, scenario)
    count = PHASE_COUNT * revolutions
    initial = 0 if initial_page == "a" else 1
    edge_rows, scheduler = ellipse.scheduler_schedule(divisor, revolutions, scenario)
    # A READY page waits on each non-eligible divisor edge.  A divisor-change
    # boundary and a resume boundary reset the counter while retaining READY,
    # so each contributes one additional wait edge.  Paused edges do not.
    ready_wait_edges = (sum(row["active"] - 1 for row in edge_rows)
                        + scheduler["changes"]
                        + scheduler["pause_transitions"] // 2)
    chunks = (header.payload_bytes + baseline.STAGING_BYTES - 1) // baseline.STAGING_BYTES
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
    }
    phase_zero_descriptor = descriptors[entries[0].scale_id - 1]
    phase_zero_source = baseline.BMS_WINDOW + phase_zero_descriptor.bank_offset
    required[f"{root}-initialize"] = {
        "ax": 0x98C1, "bx": baseline.PAGE_BYTES, "cx": baseline.PITCH,
        "dx": 0x0105, "si": phase_zero_source & 0xffff,
        "di": phase_zero_source >> 16, "bp": 0x0201, "ip": 0x4020,
    }
    for publication, (prefix, phase, edge_row) in enumerate(
            zip(flips, phases(revolutions), edge_rows), 1):
        entry = entries[phase]
        required[prefix] = {
            "ax": 0x98D1, "bx": publication, "cx": phase,
            "dx": entry.scale_id | (edge_row["active"] << 8),
            "si": entry.depth_rank & 0xffff, "di": edge_row["edge"],
            "bp": edge_row["requested_slots"], "ip": 0x4030,
        }
    final_dsa = baseline.PAGE_DSA[initial]
    for prefix in settled:
        required[prefix] = {"ax": 0x98D2, "bx": count, "cx": revolutions,
                            "dx": count, "si": count, "di": initial,
                            "bp": final_dsa & 0xffff, "ip": 0x4040}
    work = dirty_work(entries, descriptors, revolutions)
    measured = work if clear_mode == "dirty" else {
        "rectangles": 0, "rows": 0, "words": 0, "bytes": 0, "batches": 0}
    page_counts = ((count // 2 + 1, count // 2) if initial == 0
                   else (count // 2, count // 2 + 1))
    final_phases = 0x3E3F if initial == 0 else 0x3F3E
    source_bytes = sum(descriptors[entries[phase].scale_id - 1].payload_bytes
                       for phase in phases(revolutions))
    digest = publication_digest(initial, entries, revolutions)
    hud_bytes = 1056 + 144 * scheduler["changes"]
    report_expected = (
        {"ax": 0x98E1, "bx": 2, "cx": count, "dx": count, "si": 2,
         "di": count if clear_mode == "full" else 0, "bp": count, "ip": 0x4050},
        {"ax": 0x98E2, "bx": 3, "cx": page_counts[0], "dx": page_counts[1],
         "si": 0, "di": 0, "bp": 0, "ip": 0x4060},
        {"ax": 0x98E3, "bx": count * 2, "cx": 1, "dx": initial,
         "si": revolutions, "di": count, "bp": 0, "ip": 0x4070},
        {"ax": 0x98E4, "bx": source_bytes & 0xffff, "cx": source_bytes >> 16,
         "dx": measured["rows"] & 0xffff, "si": measured["rows"] >> 16,
         "di": count, "bp": final_dsa & 0xffff, "ip": 0x4080},
        {"ax": 0x98E5, "bx": digest & 0xffff, "cx": digest >> 16,
         "dx": count, "si": 15, "di": 58 * revolutions - 1,
         "bp": 1, "ip": 0x4090},
        {"ax": 0x98E6, "bx": measured["rectangles"],
         "cx": measured["words"] & 0xffff, "dx": measured["words"] >> 16,
         "si": measured["bytes"] & 0xffff, "di": measured["bytes"] >> 16,
         "bp": 0, "ip": 0x40A0},
        {"ax": 0x98E7, "bx": (count * 32000) & 0xffff,
         "cx": (count * 32000) >> 16, "dx": (count * 64000) & 0xffff,
         "si": (count * 64000) >> 16,
         "di": 1 + count + measured["batches"],
         "bp": (5 + 7 * count if clear_mode == "full" else
                5 + 5 * count + work["rows"] + 3 * work["batches"]),
         "ip": 0x40B0},
        {"ax": 0x98E8, "bx": 0, "cx": count, "dx": 1, "si": 1,
         "di": final_phases, "bp": 1, "ip": 0x40C0},
        {"ax": 0x98E9, "bx": scheduler["total_edges"],
         "cx": scheduler["unpaused_edges"], "dx": scheduler["paused_edges"],
         "si": count + (2 if scenario == "missed" else 0), "di": count,
         "bp": 2 if scenario == "missed" else 0, "ip": 0x40D0},
        {"ax": 0x98EA,
         "bx": scheduler["final_divisor"] | (scheduler["final_divisor"] << 8),
         "cx": scheduler["changes"], "dx": scheduler["changes"],
         "si": scheduler["resets"], "di": ready_wait_edges,
         "bp": 58 * revolutions - 1,
         "ip": 0x40E0},
        {"ax": 0x98EB, "bx": scheduler["pause_requests"],
         "cx": scheduler["pause_transitions"], "dx": 0, "si": 0,
         "di": 0, "bp": 0, "ip": 0x40F0},
        {"ax": 0x98EC, "bx": 58, "cx": count // 2, "dx": count // 2,
         "si": 96, "di": 48, "bp": 0, "ip": 0x4100},
        {"ax": 0x98ED, "bx": 1, "cx": 1 + scheduler["changes"], "dx": 1,
         "si": 0, "di": 0, "bp": 0, "ip": 0x4110},
        {"ax": 0x98EE, "bx": hud_bytes & 0xffff, "cx": hud_bytes >> 16,
         "dx": 0xffff, "si": 15, "di": 0xffff, "bp": 15, "ip": 0x4120},
        {"ax": 0x98EF, "bx": 58 * revolutions - 1, "cx": count,
         "dx": 0, "si": 0, "di": 0, "bp": 1, "ip": 0x4130},
    )
    for prefix, expected in zip(reports, report_expected):
        required[prefix] = expected
    ordered = ((f"{root}-probe", f"{root}-load", f"{root}-initialize")
               + flips + settled + reports)
    for prefix in ordered:
        require_registers(directory, prefix, required[prefix], errors)
    pc_frames = ellipse.check_events(directory, ordered, errors)
    errors[:] = [code.replace("M98S_", "M98T_", 1)
                 if code.startswith("M98S_") else code for code in errors]
    records = check_frames(directory, atlas, entries, descriptors, full_tiles,
                           initial_page, revolutions, edge_rows, flips, settled, errors)
    trace = check_trace(trace_path, entries, descriptors, initial_page,
                        revolutions, clear_mode, errors)
    try:
        source_text = source.read_text(encoding="utf-8")
        listing = (directory / "ZUNDORB.LST").read_text(
            encoding="utf-8", errors="replace")
        required_source = (
            "select_orbit_destination:", "validate_orbit_table:",
            "update_hud_fps_field:", "prepare_dirty_clear_state:",
            "process_scheduler_edge:", "publish_ready_hidden_page:",
        )
        if ("incbin" in source_text.lower()
                or any(item not in source_text for item in required_source)
                or "call advance_scale_sequence" in source_text
                or re.search(r"\b0F8[0-9A-Fa-f]", listing)):
            add_error(errors, "M98T_SOURCE_CONTRACT")
    except (OSError, UnicodeError):
        add_error(errors, "M98T_SOURCE_READ")
    histogram = Counter(entry.scale_id for entry in entries)
    measured_edges = scheduler["total_edges"]
    return {
        "schema": "zundamon-orbit-m98t-oracle-v1",
        "status": "PASS" if not errors else "FAIL", "errors": errors,
        "divisor": divisor, "scenario": scenario, "clear_mode": clear_mode,
        "nominal_fps_field": NOMINAL_FIELDS[divisor - 1],
        "vaeg_display_vblank_hz": VAEG_DISPLAY_HZ,
        "requested_actual_hz": VAEG_DISPLAY_HZ / divisor,
        "requested_revolution_period_seconds": PHASE_COUNT * divisor / VAEG_DISPLAY_HZ,
        "publication_rate_hz_from_edge_ratio": VAEG_DISPLAY_HZ * count / measured_edges,
        "measured_revolution_period_seconds_from_edges": (
            measured_edges / VAEG_DISPLAY_HZ / revolutions),
        "missed_slots": 2 if scenario == "missed" else 0,
        "initial_visible_page": initial_page.upper(), "revolutions": revolutions,
        "publication_count": len(records), "publication_records": records,
        "phase_publications": [revolutions] * PHASE_COUNT,
        "scale_publications": [histogram[scale] * revolutions
                               for scale in range(1, 31)],
        "near_publications": count // 2, "far_publications": count // 2,
        "table_scale_change_edges": 58,
        "pc_frames": pc_frames, "dirty_work": measured,
        "dirty_equivalent_work": work, "sgp_trace": trace,
        "orbit": {"center": [160, 100], "radius": [96, 48],
                  "radius_adjustments": 0, "sha256": sha256(table_raw),
                  "rectangles": [list(rectangle) for rectangle in rectangles]},
        "hud": {"rect": list(HUD_RECT), "fps_rect": [34, 4, 52, 12],
                "foreground": 255, "background": 1,
                "full_initializations": 1,
                "fps_field_updates": 1 + scheduler["changes"],
                "zundamon_field_updates": 1, "bytes_written": hud_bytes,
                "include_sha256": sha256(hud_raw),
                "full_tile_sha256": [sha256(tile) for tile in full_tiles],
                "fps_tile_sha256": [sha256(tile) for tile in fps_tiles]},
        "atlas": {"file_size": len(atlas), "descriptor_count": len(descriptors),
                  "required_bank_count": header.required_bank_count,
                  "payload_bytes": header.payload_bytes, "sha256": sha256(atlas)},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--table", type=Path, required=True)
    parser.add_argument("--hud", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--source", type=Path,
                        default=TOOLS.parent / "256" / "zundamon_orbit_256.asm")
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--divisor", choices=range(1, 9), type=int, required=True)
    parser.add_argument("--revolutions", choices=(1, 2), type=int, required=True)
    parser.add_argument("--scenario", choices=("static", "ladder", "pause", "missed"),
                        default="static")
    parser.add_argument("--clear-mode", choices=("full", "dirty"), required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result = verify(args.directory, args.atlas, args.table, args.hud, args.trace,
                    args.source, args.divisor, args.initial_page,
                    args.revolutions, args.scenario, args.clear_mode)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    print(json.dumps(result, sort_keys=True))
    if args.report is not None:
        args.report.write_text(encoded, encoding="utf-8")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

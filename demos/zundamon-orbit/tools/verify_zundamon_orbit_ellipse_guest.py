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

"""Independent indexed-frame, orbit, dirty-row, and cadence oracle for M98s."""

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
import validate_zundamon_orbit_table as orbit_format  # noqa: E402
import verify_zundamon_orbit_dirty_guest as dirty  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402

PHASE_COUNT = 64
FIXED_SCALE_ID = 15
ROWS_PER_BATCH = 11
NOMINAL_LABELS = ("60", "30", "20", "15", "12", "10", "8.6", "7.5")
VAEG_DISPLAY_HZ = 59.95
TRACE_CLS = re.compile(r"^SGP_SCAN: CLS addr=([0-9a-fA-F]+) words=(\d+)$")


class OracleError(Exception):
    def __init__(self, code: str):
        super().__init__(code)
        self.code = code


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def phases(revolutions: int) -> tuple[int, ...]:
    return tuple(range(PHASE_COUNT)) * revolutions


def page_for(initial: int, publication: int) -> int:
    return initial ^ (publication & 1)


def destination(entry: tuple[int, int], descriptor) -> tuple[int, int]:
    return (160 + entry[0] - descriptor.anchor_x,
            100 + entry[1] - descriptor.anchor_y)


def expected_page(atlas: bytes, descriptor, entry: tuple[int, int]) -> bytes:
    x, y = destination(entry, descriptor)
    page = bytearray(baseline.PAGE_BYTES)
    frame = atlas[descriptor.file_offset:
                  descriptor.file_offset + descriptor.payload_bytes]
    for row in range(descriptor.height):
        source = row * descriptor.pitch
        target = (y + row) * baseline.PITCH + x
        page[target:target + descriptor.width] = frame[source:source + descriptor.width]
    return bytes(page)


def prefixes(divisor: int, initial_page: str, revolutions: int, scenario: str):
    root = f"m98s-{scenario}-v{divisor}-{initial_page}-r{revolutions}"
    count = PHASE_COUNT * revolutions
    flips = tuple(f"{root}-flip-{index:03d}" for index in range(1, count + 1))
    settled = (f"{root}-settled-a", f"{root}-settled-b")
    reports = tuple(f"{root}-report-{letter}" for letter in "abcdefghijk")
    return root, flips, settled, reports


def scheduler_schedule(divisor: int, revolutions: int, scenario: str):
    count = PHASE_COUNT * revolutions
    active = divisor
    edge = 0
    rows = []
    changes = resets = pause_requests = pause_transitions = paused_edges = 0
    ladder = {4: 2, 8: 3, 12: 4, 16: 5, 20: 6, 24: 7, 28: 8,
              32: 7, 36: 6, 40: 5, 44: 4, 48: 3, 52: 2, 56: 1}
    pause_points = {4, 20, 40}
    for publication in range(1, count + 1):
        if scenario == "missed" and publication == 1:
            edge += 2
        if publication > 1:
            previous = publication - 1
            if scenario == "ladder" and previous in ladder:
                active = ladder[previous]
                edge += 1
                changes += 1
                resets += 1
            if scenario == "pause" and previous in pause_points:
                edge += 6
                pause_requests += 2
                pause_transitions += 2
                paused_edges += 5
                resets += 2
        edge += active
        rows.append({"publication": publication, "active": active, "edge": edge,
                     "requested_slots": publication
                     + (2 if scenario == "missed" else 0)})
    return rows, {
        "changes": changes, "resets": resets,
        "pause_requests": pause_requests, "pause_transitions": pause_transitions,
        "paused_edges": paused_edges, "unpaused_edges": edge - paused_edges,
        "total_edges": edge, "final_divisor": active,
    }


def read_registers(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise OracleError("M98S_REGISTERS_SCHEMA") from error
    for line in lines:
        key, separator, value = line.partition("\t")
        if not separator or not key or key in values:
            raise OracleError("M98S_REGISTERS_SCHEMA")
        values[key] = value
    return values


def require_registers(directory: Path, prefix: str,
                      expected: dict[str, int | str], errors: list[str]) -> None:
    try:
        values = read_registers(directory / f"{prefix}.registers.tsv")
    except OracleError as error:
        add_error(errors, error.code)
        return
    required = {"schema": "vaeg-registers-v1", "cs": "3000", "ds": "3000"}
    required.update({key: f"{value:04x}" if isinstance(value, int) else value
                     for key, value in expected.items()})
    if any(values.get(key) != value for key, value in required.items()):
        add_error(errors, "M98S_REGISTER_SIGNATURE")


def dirty_work(entries, descriptor, revolutions: int) -> dict[str, int]:
    rows = words = batches = 0
    sequence = phases(revolutions)
    for publication in range(3, len(sequence) + 1):
        x, _ = destination(entries[sequence[publication - 3]], descriptor)
        _, _, row_words = dirty.rounded_interval(x, descriptor.width)
        rows += descriptor.height
        words += descriptor.height * row_words
        batches += (descriptor.height + ROWS_PER_BATCH - 1) // ROWS_PER_BATCH
    return {"rectangles": len(sequence) - 2, "rows": rows, "words": words,
            "bytes": words * 2, "batches": batches}


def publication_digest(initial: int, revolutions: int) -> int:
    low = high = 0
    for publication, phase in enumerate(phases(revolutions), 1):
        total = low + phase + (FIXED_SCALE_ID << 8)
        low = total & 0xffff
        high = (high + (total >> 16) + page_for(initial, publication) + 1) & 0xffff
    return low | (high << 16)


def expected_trace(entries, descriptor, initial_page: str, revolutions: int,
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
            commands.append(("CLS", baseline.PAGE_SGP[page],
                             baseline.PAGE_BYTES // 2))
        elif publication > 2:
            old_phase = sequence[publication - 3]
            old_x, old_y = destination(entries[old_phase], descriptor)
            clear_x0, _, words = dirty.rounded_interval(old_x, descriptor.width)
            commands.extend(("CLS", baseline.PAGE_SGP[page] + y * baseline.PITCH
                             + clear_x0, words)
                            for y in range(old_y, old_y + descriptor.height))
        x, y = destination(entries[phase], descriptor)
        commands.append(("SOURCE", baseline.BMS_WINDOW + descriptor.bank_offset,
                         0, 2, descriptor.width, descriptor.height, descriptor.pitch))
        commands.append(("DEST", baseline.PAGE_SGP[page] + y * baseline.PITCH
                         + (x & ~1), x & 1, 2, descriptor.width,
                         descriptor.height, baseline.PITCH))
    return commands


def check_trace(path: Path, entries, descriptor, initial_page: str,
                revolutions: int, clear_mode: str, errors: list[str]):
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98S_TRACE_MISSING")
        return {}
    commands: list[tuple[object, ...]] = []
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
    expected = expected_trace(
        entries, descriptor, initial_page, revolutions, clear_mode)
    if commands != expected:
        add_error(errors, "M98S_TRACE_COMMAND_SEQUENCE")
    return {"bitblt_count": sum(command[0] == "SOURCE" for command in commands),
            "cls_count": sum(command[0] == "CLS" for command in commands),
            "command_identity": sha256(json.dumps(commands).encode("ascii"))}


def check_frames(directory: Path, atlas: bytes, entries, descriptor,
                 initial_page: str, revolutions: int, flips, settled,
                 errors: list[str]):
    initial = 0 if initial_page == "a" else 1
    pages = [bytes(baseline.PAGE_BYTES), bytes(baseline.PAGE_BYTES)]
    g0 = baseline.expected_g0()
    records = []
    final_raw = b""
    for publication, (prefix, phase) in enumerate(zip(flips, phases(revolutions)), 1):
        page = page_for(initial, publication)
        expected = expected_page(atlas, descriptor, entries[phase])
        pages[page] = expected
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98S_GVRAM_MISSING")
            continue
        if len(raw) != baseline.GVRAM_BYTES:
            add_error(errors, "M98S_GVRAM_SIZE")
            continue
        actual_pages = (
            raw[baseline.G1_OFFSET:baseline.G1_OFFSET + baseline.PAGE_BYTES],
            raw[baseline.G1_OFFSET + baseline.PAGE_BYTES:
                baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES],
        )
        if raw[:baseline.G0_BYTES] != g0:
            add_error(errors, "M98S_G0_CONTENT")
        if actual_pages != tuple(pages):
            add_error(errors, "M98S_G1_PAGE_CONTENT")
        if any(raw[baseline.G0_BYTES:baseline.G1_OFFSET]) or any(
                raw[baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES:]):
            add_error(errors, "M98S_GVRAM_GUARD")
        x, y = destination(entries[phase], descriptor)
        records.append({
            "publication": publication, "phase": phase,
            "target_anchor": [160 + entries[phase][0], 100 + entries[phase][1]],
            "destination": [x, y, descriptor.width, descriptor.height],
            "scale_id": FIXED_SCALE_ID, "page": "A" if page == 0 else "B",
            "g1_sha256": sha256(expected),
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
                add_error(errors, "M98S_SETTLED_CAPTURE")
        except (OSError, baseline.OracleError):
            add_error(errors, "M98S_SETTLED_CAPTURE")
    try:
        if ((directory / f"{settled[0]}.screen.bmp").read_bytes()
                != (directory / f"{settled[1]}.screen.bmp").read_bytes()):
            add_error(errors, "M98S_SCREEN_UNSTABLE")
    except OSError:
        add_error(errors, "M98S_SETTLED_CAPTURE")
    return records


def check_events(directory: Path, ordered: tuple[str, ...], errors: list[str]):
    try:
        lines = (directory / "events.tsv").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        add_error(errors, "M98S_EVENTS_SCHEMA")
        return []
    if not lines or lines[0] != "event\tframe\tid\tvalue":
        add_error(errors, "M98S_EVENTS_SCHEMA")
        return []
    rows = [line.split("\t") for line in lines[1:]]
    if any(len(row) != 4 for row in rows) or any(row[0] == "frame-limit" for row in rows):
        add_error(errors, "M98S_EVENTS_SCHEMA")
        return []
    pc_frames = [int(row[1]) for row in rows if row[0] == "pc" and row[1].isdigit()]
    captures = [(row[1], row[2]) for row in rows if row[0] == "capture"]
    if captures != list(zip(map(str, pc_frames), ordered)):
        add_error(errors, "M98S_EVENTS_SEQUENCE")
    return pc_frames


def verify(directory: Path, atlas_path: Path, table_path: Path, trace_path: Path,
           source: Path, divisor: int, initial_page: str,
           revolutions: int, scenario: str, clear_mode: str):
    errors: list[str] = []
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
        baseline.validate_runtime_descriptors(header, descriptors)
        baseline.validate_frame_crcs(atlas, descriptors)
        table_raw, entries = orbit_format.inspect(table_path)
    except (OSError, atlas_format.AtlasError, baseline.OracleError,
            orbit_format.OrbitError):
        return {"schema": "zundamon-orbit-m98s-oracle-v1", "status": "FAIL",
                "errors": ["M98S_INPUT_CONTRACT"]}
    descriptor = descriptors[FIXED_SCALE_ID - 1]
    if (descriptor.width, descriptor.height, descriptor.pitch,
            descriptor.anchor_x, descriptor.anchor_y) != (11, 9, 12, 5, 4):
        add_error(errors, "M98S_FIXED_SCALE_15")
    root, flips, settled, reports = prefixes(
        divisor, initial_page, revolutions, scenario)
    count = PHASE_COUNT * revolutions
    initial = 0 if initial_page == "a" else 1
    edge_rows, scheduler = scheduler_schedule(divisor, revolutions, scenario)
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
        f"{root}-initialize": {"ax": 0x98C1, "bx": baseline.PAGE_BYTES,
                               "cx": baseline.PITCH, "dx": 0x0105,
                               "si": (baseline.BMS_WINDOW + descriptor.bank_offset) & 0xffff,
                               "di": (baseline.BMS_WINDOW + descriptor.bank_offset) >> 16,
                               "bp": 0x0201, "ip": 0x4020},
    }
    for publication, (prefix, phase, edge_row) in enumerate(
            zip(flips, phases(revolutions), edge_rows), 1):
        if clear_mode == "full":
            try:
                actual = read_registers(directory / f"{prefix}.registers.tsv")
                actual_edge = int(actual["di"], 16)
                actual_slots = int(actual["bp"], 16)
                if (actual_edge % divisor or actual_slots < publication
                        or (publication > 1
                            and actual_slots < int(read_registers(
                                directory / f"{flips[publication - 2]}.registers.tsv"
                            )["bp"], 16))):
                    add_error(errors, "M98S_FULL_CADENCE_ORDER")
                edge_row = dict(edge_row, edge=actual_edge,
                                requested_slots=actual_slots)
            except (OracleError, KeyError, ValueError):
                add_error(errors, "M98S_REGISTERS_SCHEMA")
        required[prefix] = {
            "ax": 0x98D1, "bx": publication, "cx": phase,
            "dx": FIXED_SCALE_ID | (edge_row["active"] << 8),
            "si": page_for(initial, publication), "di": edge_row["edge"],
            "bp": edge_row["requested_slots"], "ip": 0x4030,
        }
    final_dsa = baseline.PAGE_DSA[initial]
    for prefix in settled:
        required[prefix] = {"ax": 0x98D2, "bx": count, "cx": revolutions,
                            "dx": count, "si": count, "di": initial,
                            "bp": final_dsa & 0xffff, "ip": 0x4040}
    work = dirty_work(entries, descriptor, revolutions)
    measured_work = (work if clear_mode == "dirty"
                     else {"rectangles": 0, "rows": 0, "words": 0,
                           "bytes": 0, "batches": 0})
    page_counts = ((count // 2 + 1, count // 2) if initial == 0
                   else (count // 2, count // 2 + 1))
    final_phases = 0x3E3F if initial == 0 else 0x3F3E
    source_bytes = descriptor.payload_bytes * count
    digest = publication_digest(initial, revolutions)
    report_expected = (
        {"ax": 0x98E1, "bx": 2, "cx": count, "dx": count,
         "si": 2, "di": count if clear_mode == "full" else 0,
         "bp": count, "ip": 0x4050},
        {"ax": 0x98E2, "bx": 3, "cx": page_counts[0], "dx": page_counts[1],
         "si": 0, "di": 0, "bp": 0, "ip": 0x4060},
        {"ax": 0x98E3, "bx": count * 2, "cx": 1, "dx": initial,
         "si": revolutions, "di": count, "bp": 0, "ip": 0x4070},
        {"ax": 0x98E4, "bx": source_bytes & 0xffff,
         "cx": source_bytes >> 16, "dx": measured_work["rows"] & 0xffff,
         "si": measured_work["rows"] >> 16, "di": count,
         "bp": final_dsa & 0xffff, "ip": 0x4080},
        {"ax": 0x98E5, "bx": digest & 0xffff, "cx": digest >> 16,
         "dx": count, "si": FIXED_SCALE_ID, "di": 0, "bp": 1, "ip": 0x4090},
        {"ax": 0x98E6, "bx": measured_work["rectangles"],
         "cx": measured_work["words"] & 0xffff,
         "dx": measured_work["words"] >> 16,
         "si": measured_work["bytes"] & 0xffff,
         "di": measured_work["bytes"] >> 16,
         "bp": 0, "ip": 0x40A0},
        {"ax": 0x98E7,
         "bx": (count * (baseline.PAGE_BYTES // 2)) & 0xffff,
         "cx": (count * (baseline.PAGE_BYTES // 2)) >> 16,
         "dx": (count * baseline.PAGE_BYTES) & 0xffff,
         "si": (count * baseline.PAGE_BYTES) >> 16,
         "di": 1 + count + measured_work["batches"],
         "bp": (5 + 7 * count if clear_mode == "full" else
                5 + 5 * count + work["rows"] + 3 * work["batches"]),
         "ip": 0x40B0},
        {"ax": 0x98E8, "bx": 0, "cx": count, "dx": 1, "si": 1,
         "di": final_phases, "bp": 1, "ip": 0x40C0},
    )
    for prefix, expected in zip(reports[:8], report_expected):
        required[prefix] = expected
    cadence_values = []
    total = missed = 0
    for prefix in reports[8:]:
        try:
            cadence_values.append(read_registers(directory / f"{prefix}.registers.tsv"))
        except OracleError as error:
            add_error(errors, error.code)
            cadence_values.append({})
    if len(cadence_values) == 3:
        i, j, _ = cadence_values
        try:
            total = int(i["bx"], 16)
            unpaused = int(i["cx"], 16)
            paused = int(i["dx"], 16)
            slots = int(i["si"], 16)
            published = int(i["di"], 16)
            missed = int(i["bp"], 16)
        except (KeyError, ValueError):
            add_error(errors, "M98S_CADENCE_REPORT")
            total = unpaused = paused = slots = published = missed = 0
        required[reports[8]] = {"ax": 0x98E9, "bx": total, "cx": unpaused,
                                "dx": paused, "si": slots, "di": published,
                                "bp": missed, "ip": 0x40D0}
        final_divisor = scheduler["final_divisor"]
        required[reports[9]] = {
            "ax": 0x98EA, "bx": final_divisor | (final_divisor << 8),
            "cx": scheduler["changes"], "dx": scheduler["changes"],
            "si": scheduler["resets"], "di": int(j.get("di", "0"), 16),
            "bp": 0, "ip": 0x40E0,
        }
        required[reports[10]] = {
            "ax": 0x98EB, "bx": scheduler["pause_requests"],
            "cx": scheduler["pause_transitions"], "dx": 0,
            "si": 0, "di": 0, "bp": 0, "ip": 0x40F0,
        }
        cadence_expected = (total >= count * divisor and unpaused == total
                            and paused == 0) if clear_mode == "full" else (
            total == scheduler["total_edges"]
            and unpaused == scheduler["unpaused_edges"]
            and paused == scheduler["paused_edges"])
        if not (cadence_expected and slots == published + missed
                and published == count):
            add_error(errors, "M98S_CADENCE_INVARIANT")
    ordered = ((f"{root}-probe", f"{root}-load", f"{root}-initialize")
               + flips + settled + reports)
    for prefix in ordered:
        if prefix in required:
            require_registers(directory, prefix, required[prefix], errors)
    pc_frames = check_events(directory, ordered, errors)
    records = check_frames(directory, atlas, entries, descriptor,
                           initial_page, revolutions, flips, settled, errors)
    trace = check_trace(trace_path, entries, descriptor,
                        initial_page, revolutions, clear_mode, errors)
    try:
        source_text = source.read_text(encoding="utf-8")
        listing = (directory / "ZUNDORB.LST").read_text(
            encoding="utf-8", errors="replace")
        required_source = (
            "select_orbit_destination:", "validate_orbit_table:",
            "advance_orbit_phase:", "prepare_dirty_clear_state:",
            "process_scheduler_edge:", "publish_ready_hidden_page:",
        )
        if ("incbin" in source_text.lower()
                or any(item not in source_text for item in required_source)
                or "call advance_scale_sequence" in source_text
                or re.search(r"\b0F8[0-9A-Fa-f]", listing)):
            add_error(errors, "M98S_SOURCE_CONTRACT")
    except (OSError, UnicodeError):
        add_error(errors, "M98S_SOURCE_READ")
    return {
        "schema": "zundamon-orbit-m98s-oracle-v1",
        "status": "PASS" if not errors else "FAIL", "errors": errors,
        "divisor": divisor, "scenario": scenario, "clear_mode": clear_mode,
        "nominal_label_fps": NOMINAL_LABELS[divisor - 1],
        "vaeg_display_vblank_hz": VAEG_DISPLAY_HZ,
        "requested_actual_hz": VAEG_DISPLAY_HZ / divisor,
        "requested_revolution_period_seconds": PHASE_COUNT * divisor / VAEG_DISPLAY_HZ,
        "publication_rate_hz_from_edge_ratio": VAEG_DISPLAY_HZ * count / max(1, total),
        "measured_revolution_period_seconds_from_edges": total / VAEG_DISPLAY_HZ / revolutions,
        "missed_slots": missed, "initial_visible_page": initial_page.upper(),
        "revolutions": revolutions, "publication_count": len(records),
        "phase_publications": [revolutions] * PHASE_COUNT,
        "publication_records": records, "pc_frames": pc_frames,
        "dirty_work": measured_work, "dirty_equivalent_work": work,
        "sgp_trace": trace,
        "orbit": {"phase_count": len(entries), "center": [160, 100],
                  "radius": [96, 48], "sha256": sha256(table_raw),
                  "cardinals": {"0": entries[0], "16": entries[16],
                                "32": entries[32], "48": entries[48]}},
        "fixed_scale": {"id": FIXED_SCALE_ID, "width": descriptor.width,
                        "height": descriptor.height, "pitch": descriptor.pitch,
                        "anchor": [descriptor.anchor_x, descriptor.anchor_y],
                        "bank_offset": descriptor.bank_offset,
                        "sgp_source": baseline.BMS_WINDOW + descriptor.bank_offset,
                        "payload_bytes": descriptor.payload_bytes,
                        "frame_crc32": f"{descriptor.frame_crc32:08x}"},
        "atlas": {"file_size": len(atlas), "descriptor_count": len(descriptors),
                  "required_bank_count": header.required_bank_count,
                  "payload_bytes": header.payload_bytes, "sha256": sha256(atlas)},
        "cadence_reports": cadence_values,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--table", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--divisor", choices=range(1, 9), type=int, required=True)
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--revolutions", choices=(1, 2), type=int, default=1)
    parser.add_argument("--scenario", choices=("static", "ladder", "pause", "missed"),
                        default="static")
    parser.add_argument("--clear-mode", choices=("full", "dirty"), default="dirty")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    source = args.source or TOOLS.parent / "256" / "zundamon_orbit_256.asm"
    result = verify(args.directory, args.atlas, args.table, args.trace, source,
                    args.divisor, args.initial_page, args.revolutions,
                    args.scenario, args.clear_mode)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        if args.report.parent != args.directory or args.report.exists():
            raise SystemExit("M98S_REPORT_PATH")
        args.report.write_text(encoded, encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

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

"""Fail-closed indexed-frame and scheduler oracle for static M98r VA2 runs."""

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
import verify_zundamon_orbit_dirty_guest as dirty  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402

PUBLICATIONS_PER_CYCLE = 58
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


def sequence(cycles: int) -> tuple[int, ...]:
    return baseline.scale_sequence() * cycles


def directions(cycles: int) -> tuple[int, ...]:
    return baseline.scale_directions() * cycles


def page_for(initial: int, publication: int) -> int:
    return initial ^ (publication & 1)


def prefixes(divisor: int, initial_page: str, cycles: int,
             scenario: str = "static"):
    root = f"m98r-{scenario}-v{divisor}-{initial_page}-c{cycles}"
    publications = PUBLICATIONS_PER_CYCLE * cycles
    flips = tuple(f"{root}-flip-{index:03d}" for index in range(1, publications + 1))
    settled = (f"{root}-settled-a", f"{root}-settled-b")
    reports = tuple(f"{root}-report-{letter}" for letter in "abcdefghijk")
    return root, flips, settled, reports


def scheduler_schedule(divisor: int, cycles: int, scenario: str):
    publications = PUBLICATIONS_PER_CYCLE * cycles
    active = divisor
    edge = 0
    rows = []
    changes = resets = pause_requests = pause_transitions = 0
    paused_edges = 0
    ladder = {4: 2, 8: 3, 12: 4, 16: 5, 20: 6, 24: 7, 28: 8,
              32: 7, 36: 6, 40: 5, 44: 4, 48: 3, 52: 2, 56: 1}
    pause_points = {4, 20, 40}
    for publication in range(1, publications + 1):
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
                # Pause boundary, four fully paused edges, and resume boundary.
                edge += 6
                pause_requests += 2
                pause_transitions += 2
                paused_edges += 5
                resets += 2
        edge += active
        requested_slots = publication + (2 if scenario == "missed" else 0)
        rows.append({"publication": publication, "active": active,
                     "edge": edge, "requested_slots": requested_slots})
    total_edges = edge
    return rows, {"changes": changes, "resets": resets,
                  "pause_requests": pause_requests,
                  "pause_transitions": pause_transitions,
                  "paused_edges": paused_edges,
                  "unpaused_edges": total_edges - paused_edges,
                  "total_edges": total_edges, "final_divisor": active}


def read_registers(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise OracleError("M98R_REGISTERS_SCHEMA") from error
    for line in lines:
        key, separator, value = line.partition("\t")
        if not separator or not key or key in values:
            raise OracleError("M98R_REGISTERS_SCHEMA")
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
        add_error(errors, "M98R_REGISTER_SIGNATURE")
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        add_error(errors, "M98R_REGISTER_FLAGS")
    else:
        if (flags & 0x0400) or not (flags & 0x0200):
            add_error(errors, "M98R_REGISTER_FLAGS")


def dirty_work(descriptors, cycles: int) -> dict[str, int]:
    seq = sequence(cycles)
    rows = words = batches = 0
    for publication in range(3, len(seq) + 1):
        descriptor = descriptors[seq[publication - 3] - 1]
        x, _ = baseline.destination_for(descriptor)
        _, _, row_words = dirty.rounded_interval(x, descriptor.width)
        rows += descriptor.height
        words += descriptor.height * row_words
        batches += (descriptor.height + ROWS_PER_BATCH - 1) // ROWS_PER_BATCH
    return {"rectangles": len(seq) - 2, "rows": rows, "words": words,
            "bytes": words * 2, "batches": batches}


def publication_digest(initial: int, cycles: int) -> int:
    low = high = 0
    for index, (scale_id, direction) in enumerate(
            zip(sequence(cycles), directions(cycles)), 1):
        total = low + scale_id + (direction << 8)
        low = total & 0xffff
        high = (high + (total >> 16) + page_for(initial, index) + 1) & 0xffff
    return low | (high << 16)


def check_events(directory: Path, ordered: tuple[str, ...], flip_count: int,
                 expected_edge_rows, errors: list[str]) -> tuple[list[int], list[int]]:
    try:
        lines = (directory / "events.tsv").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        add_error(errors, "M98R_EVENTS_SCHEMA")
        return [], []
    if not lines or lines[0] != "event\tframe\tid\tvalue":
        add_error(errors, "M98R_EVENTS_SCHEMA")
        return [], []
    rows = [line.split("\t") for line in lines[1:]]
    if any(len(row) != 4 for row in rows):
        add_error(errors, "M98R_EVENTS_SCHEMA")
        return [], []
    if any(row[0] == "frame-limit" for row in rows):
        add_error(errors, "M98R_EVENTS_TIMEOUT")
    pc_frames = [int(row[1]) for row in rows if row[0] == "pc" and row[1].isdigit()]
    captures = [(row[1], row[2]) for row in rows if row[0] == "capture"]
    if captures != list(zip(map(str, pc_frames), ordered)):
        add_error(errors, "M98R_EVENTS_SEQUENCE")
    flip_frames = pc_frames[3:3 + flip_count]
    intervals = [right - left for left, right in zip(flip_frames, flip_frames[1:])]
    # The guest checkpoint may be reached in the same VAEG debug frame as the
    # publication or one frame later.  Guest DI records the authoritative
    # VBLANK edge and is checked above against expected_edge_rows.  Host frame
    # numbers prove checkpoint order, but are not a substitute VBLANK clock.
    if len(expected_edge_rows) != flip_count or any(interval <= 0 for interval in intervals):
        add_error(errors, "M98R_PUBLICATION_CADENCE")
    settled_start = 3 + flip_count
    if (len(pc_frames) > settled_start + 1
            and pc_frames[settled_start + 1] != pc_frames[settled_start] + 1):
        add_error(errors, "M98R_SETTLED_FRAME_SEQUENCE")
    return pc_frames, intervals


def check_frames(directory: Path, atlas: bytes, descriptors, initial_page: str,
                 cycles: int, flips: tuple[str, ...], settled: tuple[str, ...],
                 errors: list[str]):
    initial = 0 if initial_page == "a" else 1
    pages = [bytes(baseline.PAGE_BYTES), bytes(baseline.PAGE_BYTES)]
    g0 = baseline.expected_g0()
    records = []
    final_raw = b""
    for publication, (prefix, scale_id, direction) in enumerate(
            zip(flips, sequence(cycles), directions(cycles)), 1):
        page = page_for(initial, publication)
        descriptor = descriptors[scale_id - 1]
        expected = baseline.expected_page(atlas, descriptor)
        pages[page] = expected
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98R_GVRAM_MISSING")
            continue
        if len(raw) != baseline.GVRAM_BYTES:
            add_error(errors, "M98R_GVRAM_SIZE")
            continue
        actual_pages = (
            raw[baseline.G1_OFFSET:baseline.G1_OFFSET + baseline.PAGE_BYTES],
            raw[baseline.G1_OFFSET + baseline.PAGE_BYTES:
                baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES],
        )
        if raw[:baseline.G0_BYTES] != g0:
            add_error(errors, "M98R_G0_CONTENT")
        if actual_pages != tuple(pages):
            add_error(errors, "M98R_G1_PAGE_CONTENT")
        if any(raw[baseline.G0_BYTES:baseline.G1_OFFSET]) or any(
                raw[baseline.G1_OFFSET + 2 * baseline.PAGE_BYTES:]):
            add_error(errors, "M98R_GVRAM_GUARD")
        records.append({
            "publication": publication, "scale_id": scale_id,
            "direction": "shrink" if direction == 0 else "grow",
            "page": "A" if page == 0 else "B",
            "g1_sha256": sha256(expected),
            "composite_sha256": sha256(baseline.composite(g0, expected)),
            "g1_nonzero": sum(value != 0 for value in expected),
            "g1_bbox": baseline.nonzero_bbox(expected),
        })
        final_raw = raw
    for prefix in settled:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
            screen = directory / f"{prefix}.screen.bmp"
            if raw != final_raw:
                add_error(errors, "M98R_GVRAM_UNSTABLE")
            if not baseline.bmp_nonblack(screen):
                add_error(errors, "M98R_SCREEN_BLACK")
        except (OSError, baseline.OracleError):
            add_error(errors, "M98R_SETTLED_CAPTURE")
    try:
        if ((directory / f"{settled[0]}.screen.bmp").read_bytes()
                != (directory / f"{settled[1]}.screen.bmp").read_bytes()):
            add_error(errors, "M98R_SCREEN_UNSTABLE")
    except OSError:
        add_error(errors, "M98R_SETTLED_CAPTURE")
    return records


def expected_trace(descriptors, initial_page: str, cycles: int):
    initial = 0 if initial_page == "a" else 1
    commands: list[tuple[object, ...]] = [
        ("CLS", baseline.PAGE_SGP[0], baseline.PAGE_BYTES // 2),
        ("CLS", baseline.PAGE_SGP[1], baseline.PAGE_BYTES // 2),
    ]
    seq = sequence(cycles)
    for publication, scale_id in enumerate(seq, 1):
        page = page_for(initial, publication)
        if publication > 2:
            old = descriptors[seq[publication - 3] - 1]
            old_x, old_y = baseline.destination_for(old)
            clear_x0, _, words = dirty.rounded_interval(old_x, old.width)
            commands.extend(("CLS", baseline.PAGE_SGP[page] + y * baseline.PITCH
                             + clear_x0, words)
                            for y in range(old_y, old_y + old.height))
        descriptor = descriptors[scale_id - 1]
        x, y = baseline.destination_for(descriptor)
        commands.append(("SOURCE", baseline.BMS_WINDOW + descriptor.bank_offset,
                         0, 2, descriptor.width, descriptor.height, descriptor.pitch))
        commands.append(("DEST", baseline.PAGE_SGP[page] + y * baseline.PITCH
                         + (x & ~1), x & 1, 2, descriptor.width,
                         descriptor.height, baseline.PITCH))
    return commands


def check_trace(path: Path, descriptors, initial_page: str, cycles: int,
                errors: list[str]):
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98R_TRACE_MISSING")
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
    expected = expected_trace(descriptors, initial_page, cycles)
    if commands != expected:
        add_error(errors, "M98R_TRACE_COMMAND_SEQUENCE")
    return {"bitblt_count": sum(command[0] == "SOURCE" for command in commands),
            "cls_count": sum(command[0] == "CLS" for command in commands),
            "command_identity": sha256(json.dumps(commands).encode("ascii"))}


def verify(directory: Path, atlas_path: Path, trace_path: Path, source: Path,
           divisor: int, initial_page: str, cycles: int, scenario: str):
    errors: list[str] = []
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
        baseline.validate_runtime_descriptors(header, descriptors)
        baseline.validate_frame_crcs(atlas, descriptors)
    except (OSError, atlas_format.AtlasError, baseline.OracleError):
        return {"schema": "zundamon-orbit-m98r-oracle-v1", "status": "FAIL",
                "errors": ["M98R_ATLAS_CONTRACT"]}
    root, flips, settled, reports = prefixes(
        divisor, initial_page, cycles, scenario)
    publications = PUBLICATIONS_PER_CYCLE * cycles
    initial = 0 if initial_page == "a" else 1
    edge_rows, scheduler_expected = scheduler_schedule(divisor, cycles, scenario)
    chunks = (header.payload_bytes + baseline.STAGING_BYTES - 1) // baseline.STAGING_BYTES
    maximum = descriptors[-1]
    required = {
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
                               "si": (baseline.BMS_WINDOW + maximum.bank_offset) & 0xffff,
                               "di": (baseline.BMS_WINDOW + maximum.bank_offset) >> 16,
                               "bp": 0x0201, "ip": 0x4020},
    }
    flip_registers = []
    for publication, (prefix, scale_id, direction, edge_row) in enumerate(
            zip(flips, sequence(cycles), directions(cycles), edge_rows), 1):
        try:
            values = read_registers(directory / f"{prefix}.registers.tsv")
            edge = int(values.get("di", ""), 16)
            slots = int(values.get("bp", ""), 16)
        except (OracleError, ValueError):
            add_error(errors, "M98R_REGISTERS_SCHEMA")
            edge = slots = 0
        expected = {"ax": 0x98D1, "bx": publication, "cx": scale_id,
                    "dx": direction | (edge_row["active"] << 8),
                    "si": page_for(initial, publication),
                    "di": edge_row["edge"],
                    "bp": edge_row["requested_slots"], "ip": 0x4030}
        required[prefix] = expected
        flip_registers.append({"edge": edge, "requested_slots": slots})
    final_dsa = baseline.PAGE_DSA[initial]
    for prefix in settled:
        required[prefix] = {"ax": 0x98D2, "bx": publications, "cx": cycles,
                            "dx": cycles * 2, "si": publications,
                            "di": initial, "bp": final_dsa & 0xffff,
                            "ip": 0x4040}
    work = dirty_work(descriptors, cycles)
    source_bytes = sum(descriptors[scale_id - 1].payload_bytes
                       for scale_id in sequence(cycles))
    page_counts = ((publications // 2 + 1, publications // 2)
                   if initial == 0 else
                   (publications // 2, publications // 2 + 1))
    batches = work["batches"]
    packed_scales = 0x1C1D if initial == 0 else 0x1D1C
    report_expected = (
        {"ax": 0x98E1, "bx": 2, "cx": publications, "dx": publications,
         "si": 2, "di": 0, "bp": publications, "ip": 0x4050},
        {"ax": 0x98E2, "bx": 3, "cx": page_counts[0], "dx": page_counts[1],
         "si": 0, "di": 0, "bp": 0, "ip": 0x4060},
        {"ax": 0x98E3, "bx": publications * 2, "cx": 1, "dx": initial,
         "si": cycles, "di": cycles * 2, "bp": 0, "ip": 0x4070},
        {"ax": 0x98E4, "bx": source_bytes & 0xffff,
         "cx": source_bytes >> 16, "dx": work["rows"] & 0xffff,
         "si": work["rows"] >> 16, "di": publications,
         "bp": final_dsa & 0xffff, "ip": 0x4080},
        {"ax": 0x98E5, "bx": publication_digest(initial, cycles) & 0xffff,
         "cx": publication_digest(initial, cycles) >> 16,
         "dx": publications, "si": 30 * cycles, "di": 28 * cycles,
         "bp": 1, "ip": 0x4090},
        {"ax": 0x98E6, "bx": publications - 2,
         "cx": work["words"] & 0xffff, "dx": work["words"] >> 16,
         "si": work["bytes"] & 0xffff, "di": work["bytes"] >> 16,
         "bp": 0, "ip": 0x40A0},
        {"ax": 0x98E7,
         "bx": (publications * (baseline.PAGE_BYTES // 2)) & 0xffff,
         "cx": (publications * (baseline.PAGE_BYTES // 2)) >> 16,
         "dx": (publications * baseline.PAGE_BYTES) & 0xffff,
         "si": (publications * baseline.PAGE_BYTES) >> 16,
         "di": 1 + publications + batches,
         "bp": 5 + 5 * publications + work["rows"] + 3 * batches,
         "ip": 0x40B0},
        {"ax": 0x98E8, "bx": 0, "cx": publications, "dx": 1, "si": 1,
         "di": packed_scales, "bp": 1, "ip": 0x40C0},
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
        i, j, k = cadence_values
        try:
            total = int(i["bx"], 16)
            unpaused = int(i["cx"], 16)
            paused_edges = int(i["dx"], 16)
            slots = int(i["si"], 16)
            published = int(i["di"], 16)
            missed = int(i["bp"], 16)
        except (KeyError, ValueError):
            add_error(errors, "M98R_CADENCE_REPORT")
            total = unpaused = paused_edges = slots = published = missed = 0
        required[reports[8]] = {"ax": 0x98E9, "bx": total, "cx": unpaused,
                                "dx": paused_edges, "si": slots,
                                "di": published, "bp": missed, "ip": 0x40D0}
        final_divisor = scheduler_expected["final_divisor"]
        required[reports[9]] = {"ax": 0x98EA,
                                "bx": final_divisor | (final_divisor << 8),
                                "cx": scheduler_expected["changes"],
                                "dx": scheduler_expected["changes"],
                                "si": scheduler_expected["resets"],
                                "di": int(j.get("di", "0"), 16),
                                "bp": publications, "ip": 0x40E0}
        required[reports[10]] = {"ax": 0x98EB,
                                 "bx": scheduler_expected["pause_requests"],
                                 "cx": scheduler_expected["pause_transitions"],
                                 "dx": 0,
                                 "si": 0, "di": 0, "bp": 0, "ip": 0x40F0}
        if not (total == scheduler_expected["total_edges"]
                and unpaused == scheduler_expected["unpaused_edges"]
                and paused_edges == scheduler_expected["paused_edges"]
                and slots == published + missed
                and published == publications):
            add_error(errors, "M98R_CADENCE_INVARIANT")
    ordered = ((f"{root}-probe", f"{root}-load", f"{root}-initialize")
               + flips + settled + reports)
    for prefix in ordered:
        if prefix in required:
            require_registers(directory, prefix, required[prefix], errors)
    pc_frames, intervals = check_events(
        directory, ordered, publications, edge_rows, errors)
    records = check_frames(
        directory, atlas, descriptors, initial_page, cycles, flips, settled, errors)
    trace = check_trace(trace_path, descriptors, initial_page, cycles, errors)
    try:
        source_text = source.read_text(encoding="utf-8")
        listing = (directory / "ZUNDORB.LST").read_text(encoding="utf-8", errors="replace")
        required_source = ("parse_cadence_option:", "observe_vblank_sample:",
                           "process_scheduler_edge:", "wait_scheduler_edge:",
                           "render_hidden_page_to_ready:",
                           "publish_ready_hidden_page:",
                           "prepare_dirty_clear_state:")
        if ("incbin" in source_text.lower() or "%include" in source_text.lower()
                or any(item not in source_text for item in required_source)
                or re.search(r"\b0F8[0-9A-Fa-f]", listing)):
            add_error(errors, "M98R_SOURCE_CONTRACT")
    except (OSError, UnicodeError):
        add_error(errors, "M98R_SOURCE_READ")
    return {
        "schema": "zundamon-orbit-m98r-oracle-v1",
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
        "divisor": divisor,
        "scenario": scenario,
        "nominal_label_fps": NOMINAL_LABELS[divisor - 1],
        "vaeg_display_vblank_hz": VAEG_DISPLAY_HZ,
        "requested_actual_hz": VAEG_DISPLAY_HZ / divisor,
        "publication_rate_hz_from_edge_ratio": (
            VAEG_DISPLAY_HZ * publications / max(1, total)),
        "missed_slots": missed,
        "initial_visible_page": initial_page.upper(),
        "cycles": cycles,
        "publication_count": len(records),
        "publication_records": records,
        "publication_frame_intervals": intervals,
        "pc_frames": pc_frames,
        "sgp_trace": trace,
        "atlas": {"file_size": len(atlas), "descriptor_count": len(descriptors),
                  "required_bank_count": header.required_bank_count,
                  "payload_bytes": header.payload_bytes, "sha256": sha256(atlas)},
        "guest_artifacts": {path.name: {"size": path.stat().st_size,
                                        "sha256": sha256(path.read_bytes())}
                            for path in directory.iterdir()
                            if path.is_file() and path.suffix.lower()
                            in (".com", ".lst", ".bin", ".d88", ".debug")},
        "cadence_reports": cadence_values,
        "flip_scheduler_trace": flip_registers,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--divisor", choices=range(1, 9), type=int, required=True)
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--cycles", choices=(1, 2), type=int, default=1)
    parser.add_argument("--scenario", choices=("static", "ladder", "pause", "missed"),
                        default="static")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    source = args.source or TOOLS.parent / "256" / "zundamon_orbit_256.asm"
    result = verify(args.directory, args.atlas, args.trace, source,
                    args.divisor, args.initial_page, args.cycles, args.scenario)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        if args.report.parent != args.directory or args.report.exists():
            raise SystemExit("M98R_REPORT_PATH")
        args.report.write_text(encoded, encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

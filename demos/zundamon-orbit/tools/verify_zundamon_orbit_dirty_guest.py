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

"""Fail-closed VA2 oracle for M98q full/dirty two-cycle runs."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from dataclasses import asdict
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import verify_zundamon_orbit_scale_guest as baseline  # noqa: E402

WIDTH = baseline.WIDTH
HEIGHT = baseline.HEIGHT
PITCH = baseline.PITCH
PAGE_BYTES = baseline.PAGE_BYTES
PAGE_WORDS = PAGE_BYTES // 2
G0_BYTES = baseline.G0_BYTES
G1_OFFSET = baseline.G1_OFFSET
GVRAM_BYTES = baseline.GVRAM_BYTES
PAGE_SGP = baseline.PAGE_SGP
PAGE_DSA = baseline.PAGE_DSA
BMS_WINDOW = baseline.BMS_WINDOW
STAGING_BYTES = baseline.STAGING_BYTES
CYCLES = 2
PUBLICATIONS_PER_CYCLE = 58
PUBLICATIONS = CYCLES * PUBLICATIONS_PER_CYCLE
ROWS_PER_BATCH = 11

TRACE_CLS = re.compile(r"^SGP_SCAN: CLS addr=([0-9a-fA-F]+) words=(\d+)$")
TRACE_SOURCE = baseline.TRACE_SOURCE
TRACE_DESTINATION = baseline.TRACE_DESTINATION


class OracleError(Exception):
    """One stable M98q oracle failure."""

    def __init__(self, code: str):
        super().__init__(code)
        self.code = code


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def sequence() -> tuple[int, ...]:
    return baseline.scale_sequence() * CYCLES


def directions() -> tuple[int, ...]:
    return baseline.scale_directions() * CYCLES


def prefixes(initial_page: str, clear_mode: str):
    root = f"m98q-{initial_page}-{clear_mode}"
    flips = tuple(f"{root}-flip-{index:03d}"
                  for index in range(1, PUBLICATIONS + 1))
    settled = (f"{root}-settled-a", f"{root}-settled-b")
    reports = tuple(f"{root}-report-{letter}" for letter in "abcdefgh")
    return root, flips, settled, reports


def page_for(initial: int, publication: int) -> int:
    return initial ^ (publication & 1)


def rounded_interval(x: int, width: int) -> tuple[int, int, int]:
    if width <= 0 or x < 0 or x + width > WIDTH:
        raise OracleError("M98Q_DIRTY_RECT_BOUNDS")
    clear_x0 = x & ~1
    clear_x1 = (x + width + 1) & ~1
    if not (0 <= clear_x0 < clear_x1 <= WIDTH):
        raise OracleError("M98Q_DIRTY_ROUND_BOUNDS")
    if (clear_x0 | clear_x1) & 1:
        raise OracleError("M98Q_DIRTY_ROUND_ALIGNMENT")
    return clear_x0, clear_x1, (clear_x1 - clear_x0) // 2


def validate_rectangle(rectangle: tuple[int, int, int, int]) -> None:
    x, y, width, height = rectangle
    rounded_interval(x, width)
    if height <= 0 or y < 0 or y + height > HEIGHT:
        raise OracleError("M98Q_DIRTY_RECT_BOUNDS")


def dirty_work(descriptors) -> dict[str, int]:
    row_commands = words = batches = 0
    seq = sequence()
    for publication in range(3, PUBLICATIONS + 1):
        old_descriptor = descriptors[seq[publication - 3] - 1]
        x, y = baseline.destination_for(old_descriptor)
        validate_rectangle((x, y, old_descriptor.width, old_descriptor.height))
        _, _, row_words = rounded_interval(x, old_descriptor.width)
        row_commands += old_descriptor.height
        words += old_descriptor.height * row_words
        batches += (old_descriptor.height + ROWS_PER_BATCH - 1) // ROWS_PER_BATCH
    return {
        "rectangles": PUBLICATIONS - 2,
        "row_commands": row_commands,
        "words": words,
        "bytes": words * 2,
        "batches": batches,
    }


def publication_digest(initial: int) -> int:
    low = high = 0
    for index, (scale_id, direction) in enumerate(zip(sequence(), directions()), 1):
        total = low + scale_id + (direction << 8)
        low = total & 0xffff
        high = (high + (total >> 16) + page_for(initial, index) + 1) & 0xffff
    return low | (high << 16)


def read_registers(path: Path) -> dict[str, str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise OracleError("M98Q_REGISTERS_SCHEMA") from error
    values: dict[str, str] = {}
    for line in lines:
        key, separator, value = line.partition("\t")
        if not separator or not key or key in values:
            raise OracleError("M98Q_REGISTERS_SCHEMA")
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
        add_error(errors, "M98Q_REGISTER_SIGNATURE")
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        add_error(errors, "M98Q_REGISTER_FLAGS")
    else:
        if (flags & 0x0400) or not (flags & 0x0200):
            add_error(errors, "M98Q_REGISTER_FLAGS")


def expected_register_map(header, descriptors, initial_page: str, clear_mode: str):
    root, flips, settled, reports = prefixes(initial_page, clear_mode)
    initial = 0 if initial_page == "a" else 1
    chunks = (header.payload_bytes + STAGING_BYTES - 1) // STAGING_BYTES
    maximum = descriptors[-1]
    mapping: dict[str, dict[str, int | str]] = {
        f"{root}-probe": {"ax": 0x98A1, "bx": 0x01D0, "cx": 0x0080,
                          "dx": 0x0002, "si": 0xA55A, "di": 0x0081,
                          "bp": 0, "ip": 0x4000},
        f"{root}-load": {"ax": 0x98B1, "bx": chunks,
                         "cx": header.payload_bytes & 0xffff,
                         "dx": header.payload_bytes >> 16, "si": STAGING_BYTES,
                         "di": header.file_size & 0xffff,
                         "bp": header.file_size >> 16, "ip": 0x4010},
        f"{root}-initialize": {
            "ax": 0x98C1, "bx": PAGE_BYTES, "cx": PITCH, "dx": 0x0105,
            "si": (BMS_WINDOW + maximum.bank_offset) & 0xffff,
            "di": (BMS_WINDOW + maximum.bank_offset) >> 16,
            "bp": 0x0201, "ip": 0x4020,
        },
    }
    for index, (prefix, scale_id, direction) in enumerate(
            zip(flips, sequence(), directions()), 1):
        descriptor = descriptors[scale_id - 1]
        x, y = baseline.destination_for(descriptor)
        mapping[prefix] = {
            "ax": 0x98D1, "bx": index, "cx": scale_id, "dx": direction,
            "si": page_for(initial, index), "di": x, "bp": y, "ip": 0x4030,
        }
    final_dsa = PAGE_DSA[initial]
    for prefix in settled:
        mapping[prefix] = {
            "ax": 0x98D2, "bx": PUBLICATIONS, "cx": CYCLES,
            "dx": CYCLES * 2, "si": PUBLICATIONS, "di": initial,
            "bp": final_dsa & 0xffff, "ip": 0x4040,
        }
    work = dirty_work(descriptors)
    source_bytes = sum(descriptors[scale_id - 1].payload_bytes
                       for scale_id in sequence())
    digest = publication_digest(initial)
    page_counts = (59, 58) if initial == 0 else (58, 59)
    steady_full = PUBLICATIONS if clear_mode == "full" else 0
    dirty_rects = 0 if clear_mode == "full" else work["rectangles"]
    dirty_rows = 0 if clear_mode == "full" else work["row_commands"]
    dirty_words = 0 if clear_mode == "full" else work["words"]
    dirty_bytes = dirty_words * 2
    clear_batches = 0 if clear_mode == "full" else work["batches"]
    command_lists = 1 + PUBLICATIONS + clear_batches
    commands = 5 + (7 * PUBLICATIONS if clear_mode == "full" else
                    5 * PUBLICATIONS + dirty_rows + 3 * clear_batches)
    packed_scales = 0x1C1D if initial == 0 else 0x1D1C
    expected_reports = (
        {"ax": 0x98E1, "bx": 2, "cx": PUBLICATIONS, "dx": PUBLICATIONS,
         "si": 2, "di": steady_full, "bp": PUBLICATIONS, "ip": 0x4050},
        {"ax": 0x98E2, "bx": PUBLICATIONS + 3, "cx": page_counts[0],
         "dx": page_counts[1], "si": 0, "di": 0, "bp": 0, "ip": 0x4060},
        {"ax": 0x98E3, "bx": PUBLICATIONS * 2, "cx": 1, "dx": initial,
         "si": CYCLES, "di": CYCLES * 2, "bp": 0, "ip": 0x4070},
        {"ax": 0x98E4, "bx": source_bytes & 0xffff,
         "cx": source_bytes >> 16, "dx": dirty_rows & 0xffff,
         "si": dirty_rows >> 16, "di": PUBLICATIONS,
         "bp": final_dsa & 0xffff, "ip": 0x4080},
        {"ax": 0x98E5, "bx": digest & 0xffff, "cx": digest >> 16,
         "dx": PUBLICATIONS, "si": 30 * CYCLES, "di": 28 * CYCLES,
         "bp": 1, "ip": 0x4090},
        {"ax": 0x98E6, "bx": dirty_rects, "cx": dirty_words & 0xffff,
         "dx": dirty_words >> 16, "si": dirty_bytes & 0xffff,
         "di": dirty_bytes >> 16, "bp": 0, "ip": 0x40A0},
        {"ax": 0x98E7, "bx": (PUBLICATIONS * PAGE_WORDS) & 0xffff,
         "cx": (PUBLICATIONS * PAGE_WORDS) >> 16,
         "dx": (PUBLICATIONS * PAGE_BYTES) & 0xffff,
         "si": (PUBLICATIONS * PAGE_BYTES) >> 16,
         "di": command_lists, "bp": commands, "ip": 0x40B0},
        {"ax": 0x98E8, "bx": 0, "cx": PUBLICATIONS, "dx": 1, "si": 1,
         "di": packed_scales, "bp": 1, "ip": 0x40C0},
    )
    mapping.update(zip(reports, expected_reports))
    return mapping


def check_events(directory: Path, expected_prefixes: tuple[str, ...],
                 errors: list[str]) -> list[int]:
    try:
        lines = (directory / "events.tsv").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        add_error(errors, "M98Q_EVENTS_SCHEMA")
        return []
    if not lines or lines[0] != "event\tframe\tid\tvalue":
        add_error(errors, "M98Q_EVENTS_SCHEMA")
        return []
    rows = [line.split("\t") for line in lines[1:]]
    if any(len(row) != 4 for row in rows):
        add_error(errors, "M98Q_EVENTS_SCHEMA")
        return []
    if any(row[0] == "frame-limit" for row in rows):
        add_error(errors, "M98Q_EVENTS_TIMEOUT")
    pc_frames = [int(row[1]) for row in rows if row[0] == "pc" and row[1].isdigit()]
    captures = [(row[1], row[2]) for row in rows if row[0] == "capture"]
    expected_captures = list(zip((str(frame) for frame in pc_frames), expected_prefixes))
    if (len(pc_frames) != len(expected_prefixes) or captures != expected_captures
            or any(right < left for left, right in zip(pc_frames, pc_frames[1:]))):
        add_error(errors, "M98Q_EVENTS_SEQUENCE")
    settled_index = 3 + PUBLICATIONS
    if (len(pc_frames) == len(expected_prefixes)
            and pc_frames[settled_index + 1] != pc_frames[settled_index] + 1):
        add_error(errors, "M98Q_SETTLED_FRAME_SEQUENCE")
    return pc_frames


def check_gvram(directory: Path, atlas: bytes, descriptors, initial_page: str,
                clear_mode: str, errors: list[str]):
    _, flips, settled, _ = prefixes(initial_page, clear_mode)
    initial = 0 if initial_page == "a" else 1
    pages = [bytes(PAGE_BYTES), bytes(PAGE_BYTES)]
    g0 = baseline.expected_g0()
    records = []
    final_raw = b""
    for index, (prefix, scale_id, direction) in enumerate(
            zip(flips, sequence(), directions()), 1):
        page = page_for(initial, index)
        descriptor = descriptors[scale_id - 1]
        expected = baseline.expected_page(atlas, descriptor)
        pages[page] = expected
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98Q_GVRAM_MISSING")
            continue
        if len(raw) != GVRAM_BYTES:
            add_error(errors, "M98Q_GVRAM_SIZE")
            continue
        if raw[:G0_BYTES] != g0:
            add_error(errors, "M98Q_G0_CONTENT")
        actual_pages = (raw[G1_OFFSET:G1_OFFSET + PAGE_BYTES],
                        raw[G1_OFFSET + PAGE_BYTES:G1_OFFSET + PAGE_BYTES * 2])
        if actual_pages != tuple(pages):
            add_error(errors, "M98Q_G1_PAGE_CONTENT")
        if any(raw[G0_BYTES:G1_OFFSET]) or any(raw[G1_OFFSET + PAGE_BYTES * 2:]):
            add_error(errors, "M98Q_GVRAM_GUARD")
        composed = baseline.composite(g0, expected)
        records.append({
            "cycle": (index - 1) // PUBLICATIONS_PER_CYCLE + 1,
            "direction": "shrink" if direction == 0 else "grow",
            "destination": list(baseline.destination_for(descriptor)),
            "g1_bbox": baseline.nonzero_bbox(expected),
            "g1_nonzero": sum(value != 0 for value in expected),
            "g1_sha256": sha256(expected),
            "composite_sha256": sha256(composed),
            "page": "A" if page == 0 else "B",
            "publication": index,
            "scale_id": scale_id,
        })
        final_raw = raw
    settled_raw = []
    for prefix in settled:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98Q_GVRAM_MISSING")
            continue
        if raw != final_raw:
            add_error(errors, "M98Q_GVRAM_UNSTABLE")
        settled_raw.append(raw)
    return records, final_raw, settled_raw


def check_screens(directory: Path, settled: tuple[str, ...], errors: list[str]) -> bool:
    screens = []
    for prefix in settled:
        try:
            path = directory / f"{prefix}.screen.bmp"
            screens.append(path.read_bytes())
            if not baseline.bmp_nonblack(path):
                add_error(errors, "M98Q_SCREEN_BLACK")
        except (OSError, baseline.OracleError):
            add_error(errors, "M98Q_SCREEN_FORMAT")
    if len(screens) == 2 and screens[0] != screens[1]:
        add_error(errors, "M98Q_SCREEN_UNSTABLE")
    return len(screens) == 2 and screens[0] == screens[1]


def expected_trace(descriptors, initial_page: str, clear_mode: str):
    initial = 0 if initial_page == "a" else 1
    commands: list[tuple[object, ...]] = [
        ("CLS", PAGE_SGP[0], PAGE_WORDS),
        ("CLS", PAGE_SGP[1], PAGE_WORDS),
    ]
    seq = sequence()
    for index, scale_id in enumerate(seq, 1):
        page = page_for(initial, index)
        if clear_mode == "full":
            commands.append(("CLS", PAGE_SGP[page], PAGE_WORDS))
        elif index > 2:
            old = descriptors[seq[index - 3] - 1]
            old_x, old_y = baseline.destination_for(old)
            clear_x0, _, words = rounded_interval(old_x, old.width)
            commands.extend(("CLS", PAGE_SGP[page] + y * PITCH + clear_x0, words)
                            for y in range(old_y, old_y + old.height))
        descriptor = descriptors[scale_id - 1]
        x, y = baseline.destination_for(descriptor)
        commands.append(("SOURCE", BMS_WINDOW + descriptor.bank_offset, 0, 2,
                         descriptor.width, descriptor.height, descriptor.pitch))
        commands.append(("DEST", PAGE_SGP[page] + y * PITCH + (x & ~1),
                         x & 1, 2, descriptor.width, descriptor.height, PITCH))
    return commands


def check_trace(path: Path, descriptors, initial_page: str, clear_mode: str,
                errors: list[str]) -> dict[str, object]:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98Q_TRACE_MISSING")
        return {}
    commands: list[tuple[object, ...]] = []
    for line in lines:
        if match := TRACE_CLS.match(line):
            commands.append(("CLS", int(match.group(1), 16), int(match.group(2))))
        elif match := TRACE_SOURCE.match(line):
            commands.append(("SOURCE",) + tuple(
                int(match.group(index), 16 if index == 1 else 10)
                for index in range(1, 7)))
        elif match := TRACE_DESTINATION.match(line):
            commands.append(("DEST",) + tuple(
                int(match.group(index), 16 if index == 1 else 10)
                for index in range(1, 7)))
    expected = expected_trace(descriptors, initial_page, clear_mode)
    if commands != expected:
        add_error(errors, "M98Q_TRACE_COMMAND_SEQUENCE")
    cls = [command for command in commands if command[0] == "CLS"]
    return {
        "bitblt_count": sum(command[0] == "SOURCE" for command in commands),
        "cls_count": len(cls),
        "command_identity": sha256(json.dumps(commands).encode("ascii")),
        "first_cls": list(cls[0][1:]) if cls else None,
        "last_cls": list(cls[-1][1:]) if cls else None,
    }


def check_source(source: Path, listing: Path, errors: list[str]) -> None:
    try:
        text = source.read_text(encoding="utf-8")
        listing_text = listing.read_text(encoding="utf-8", errors="replace")
    except (OSError, UnicodeError):
        add_error(errors, "M98Q_SOURCE_READ")
        return
    if "incbin" in text.lower() or "%include" in text.lower():
        add_error(errors, "M98Q_SOURCE_EMBEDDED_ATLAS")
    required = (
        "%define DIRTY_ROWS_PER_BATCH", "prepare_dirty_clear_state:",
        "build_dirty_row_commands:", "commit_pending_rectangle:",
        "page_old_valid:", "page_old_x:", "call wait_vblank_edge",
        "call publish_page", "call select_render_ordinary",
    )
    if any(item not in text for item in required):
        add_error(errors, "M98Q_SOURCE_CONTRACT")
    if text.count("mov ax, SGP_COMMAND_BITBLT") != 1:
        add_error(errors, "M98Q_SOURCE_BITBLT_TEMPLATE")
    if re.search(r"\b0F8[0-9A-Fa-f]", listing_text):
        add_error(errors, "M98Q_VA2_INSTRUCTION_SET")


def check_golden(directory: Path, golden: Path, initial_page: str,
                 errors: list[str]) -> int:
    _, dirty_flips, dirty_settled, _ = prefixes(initial_page, "dirty")
    _, full_flips, full_settled, _ = prefixes(initial_page, "full")
    mismatches = 0
    for actual_prefix, golden_prefix in zip(
            dirty_flips + dirty_settled, full_flips + full_settled):
        for suffix in ("gvram.bin",):
            try:
                actual = (directory / f"{actual_prefix}.{suffix}").read_bytes()
                expected = (golden / f"{golden_prefix}.{suffix}").read_bytes()
            except OSError:
                add_error(errors, "M98Q_GOLDEN_MISSING")
                return mismatches + 1
            if actual != expected:
                mismatches += 1
    for actual_prefix, golden_prefix in zip(dirty_settled, full_settled):
        try:
            actual = (directory / f"{actual_prefix}.screen.bmp").read_bytes()
            expected = (golden / f"{golden_prefix}.screen.bmp").read_bytes()
        except OSError:
            add_error(errors, "M98Q_GOLDEN_MISSING")
            return mismatches + 1
        if actual != expected:
            mismatches += 1
    if mismatches:
        add_error(errors, "M98Q_DIRTY_FULL_MISMATCH")
    return mismatches


def artifact_records(directory: Path) -> dict[str, dict[str, object]]:
    records = {}
    for path in sorted(candidate for candidate in directory.iterdir()
                       if candidate.is_file() and candidate.suffix.lower()
                       in (".com", ".lst", ".bin", ".d88", ".log", ".debug")):
        data = path.read_bytes()
        records[path.name] = {"size": len(data), "sha256": sha256(data)}
    return records


def verify(directory: Path, source: Path, atlas_path: Path, trace: Path,
           initial_page: str, clear_mode: str, golden: Path | None):
    errors: list[str] = []
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
        baseline.validate_runtime_descriptors(header, descriptors)
        baseline.validate_frame_crcs(atlas, descriptors)
    except (OSError, atlas_format.AtlasError, baseline.OracleError):
        return {"schema": "zundamon-orbit-m98q-oracle-v1", "status": "FAIL",
                "errors": ["M98Q_ATLAS_CONTRACT"]}
    root, flips, settled, reports = prefixes(initial_page, clear_mode)
    register_map = expected_register_map(header, descriptors, initial_page, clear_mode)
    ordered = ((f"{root}-probe", f"{root}-load", f"{root}-initialize")
               + flips + settled + reports)
    for prefix in ordered:
        require_registers(directory, prefix, register_map[prefix], errors)
    pc_frames = check_events(directory, ordered, errors)
    records, final_raw, settled_raw = check_gvram(
        directory, atlas, descriptors, initial_page, clear_mode, errors)
    screen_stable = check_screens(directory, settled, errors)
    trace_report = check_trace(trace, descriptors, initial_page, clear_mode, errors)
    check_source(source, directory / "ZUNDORB.LST", errors)
    mismatch_count = 0
    if clear_mode == "dirty":
        if golden is None:
            add_error(errors, "M98Q_GOLDEN_REQUIRED")
        else:
            mismatch_count = check_golden(directory, golden, initial_page, errors)
    descriptor_rows = []
    for scale_id, descriptor in enumerate(descriptors, 1):
        row = asdict(descriptor)
        x, _ = baseline.destination_for(descriptor)
        clear_x0, clear_x1, words = rounded_interval(x, descriptor.width)
        row.update({"scale_id": scale_id,
                    "destination": list(baseline.destination_for(descriptor)),
                    "rounded_clear": [clear_x0, clear_x1],
                    "rounded_words": words,
                    "sgp_source": BMS_WINDOW + descriptor.bank_offset})
        descriptor_rows.append(row)
    work = dirty_work(descriptors)
    return {
        "artifacts": artifact_records(directory),
        "atlas": {"descriptor_count": len(descriptors),
                  "descriptors": descriptor_rows, "file_size": len(atlas),
                  "payload_bytes": header.payload_bytes,
                  "required_bank_count": header.required_bank_count,
                  "sha256": sha256(atlas)},
        "clear_mode": clear_mode,
        "dirty_work": work,
        "errors": errors,
        "golden_mismatch_count": mismatch_count,
        "initial_visible_page": initial_page.upper(),
        "pc_frames": pc_frames,
        "publication_records": records,
        "sequence": list(sequence()),
        "settled_gvram_stable": bool(final_raw)
                                  and settled_raw == [final_raw, final_raw],
        "settled_screen_stable": screen_stable,
        "sgp_trace": trace_report,
        "status": "PASS" if not errors else "FAIL",
        "work_ratio": work["words"] / (PUBLICATIONS * PAGE_WORDS),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--clear-mode", choices=("full", "dirty"), required=True)
    parser.add_argument("--golden-directory", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    source = args.source or Path(__file__).resolve().parents[1] / "256" / "zundamon_orbit_256.asm"
    result = verify(args.directory, source, args.atlas, args.trace,
                    args.initial_page, args.clear_mode, args.golden_directory)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        if args.report.parent != args.directory or args.report.exists():
            raise SystemExit("M98Q_REPORT_PATH")
        args.report.write_text(encoded, encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

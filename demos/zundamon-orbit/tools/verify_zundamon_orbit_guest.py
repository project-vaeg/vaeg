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

"""Fail-closed oracle for the M98o transparent G1 double-buffer proof."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from dataclasses import asdict
from pathlib import Path

TOOLS_DIRECTORY = Path(__file__).resolve().parent
if str(TOOLS_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIRECTORY))
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402

PHASE_PREFIXES = ("m98o-probe", "m98o-load", "m98o-initialize")
FLIP_PREFIXES = tuple(f"m98o-flip-{index}" for index in range(1, 5))
SETTLED_PREFIXES = ("m98o-settled-a", "m98o-settled-b")
REPORT_PREFIXES = ("m98o-report-a", "m98o-report-b", "m98o-report-c")
ALL_PREFIXES = PHASE_PREFIXES + FLIP_PREFIXES + SETTLED_PREFIXES + REPORT_PREFIXES
GVRAM_PREFIXES = FLIP_PREFIXES + SETTLED_PREFIXES
GVRAM_SIZE = 0x40000
G0_SIZE = 320 * 200
G1_OFFSET = 0x20000
G1_WIDTH = 320
G1_VISIBLE_HEIGHT = 200
G1_BACKING_HEIGHT = 400
G1_PAGE_BYTES = G1_WIDTH * G1_VISIBLE_HEIGHT
G1_BACKING_BYTES = G1_WIDTH * G1_BACKING_HEIGHT
G1_PAGE_A_SGP = 0x220000
G1_PAGE_B_SGP = G1_PAGE_A_SGP + G1_PAGE_BYTES
G1_PAGE_A_DSA = 0x020000
G1_PAGE_B_DSA = G1_PAGE_A_DSA + G1_PAGE_BYTES
POSITION_P0 = (48, 40)
POSITION_P1 = (248, 140)
STAGING_BYTES = 4096
BMS_WINDOW_BASE = 0x080000
ATLAS_SELECTED_CELL = "level-30"
TRACE_SOURCE = re.compile(
    r"^SGP_SCAN: SET_SOURCE addr=([0-9a-fA-F]+) dot=(\d+) mode=(\d+) "
    r"width=(\d+) height=(\d+) fbw=(\d+)$")
TRACE_DESTINATION = re.compile(
    r"^SGP_SCAN: SET_DEST addr=([0-9a-fA-F]+) dot=(\d+) mode=(\d+) "
    r"width=(\d+) height=(\d+) fbw=(\d+)$")


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_tsv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or not key or key in values:
            raise ValueError("M98O_REGISTERS_SCHEMA")
        values[key] = value
    return values


def expected_registers(header, descriptor) -> dict[str, dict[str, str]]:
    source = BMS_WINDOW_BASE + descriptor.bank_offset
    chunks = (header.payload_bytes + STAGING_BYTES - 1) // STAGING_BYTES
    expected = {
        "m98o-probe": {
            "ax": "98a1", "bx": "01d0", "cx": "0080", "dx": "0002",
            "si": "a55a", "di": "0081", "bp": "0000", "ip": "3000"},
        "m98o-load": {
            "ax": "98b1", "bx": f"{chunks:04x}",
            "cx": f"{header.payload_bytes & 0xffff:04x}",
            "dx": f"{header.payload_bytes >> 16:04x}", "si": "1000",
            "di": f"{header.file_size & 0xffff:04x}",
            "bp": f"{header.file_size >> 16:04x}", "ip": "3010"},
        "m98o-initialize": {
            "ax": "98c1", "bx": f"{G1_PAGE_BYTES:04x}", "cx": "0140",
            "dx": "0105", "si": f"{source & 0xffff:04x}",
            "di": f"{source >> 16:04x}", "bp": "0201", "ip": "3020"},
    }
    flip_rows = (
        (1, 1, 0, 0x0405, 0x0001, G1_PAGE_B_DSA & 0xffff),
        (2, 0, 1, 0x0504, 0x0100, G1_PAGE_A_DSA & 0xffff),
        (3, 1, 0, 0x0405, 0x0001, G1_PAGE_B_DSA & 0xffff),
        (4, 0, 1, 0x0504, 0x0100, G1_PAGE_A_DSA & 0xffff),
    )
    for prefix, row in zip(FLIP_PREFIXES, flip_rows):
        flip, visible, position, states, roles, dsa = row
        expected[prefix] = {
            "ax": "98d1", "bx": f"{flip:04x}", "cx": f"{visible:04x}",
            "dx": f"{position:04x}", "si": f"{states:04x}",
            "di": f"{roles:04x}", "bp": f"{dsa:04x}", "ip": "3030"}
    for prefix in SETTLED_PREFIXES:
        expected[prefix] = {
            "ax": "98d2", "bx": f"{descriptor.width:04x}",
            "cx": f"{descriptor.height:04x}", "dx": f"{descriptor.pitch:04x}",
            "si": "0004", "di": "0100", "bp": "0000", "ip": "3040"}
    expected.update({
        "m98o-report-a": {
            "ax": "98e1", "bx": "0002", "cx": "0004", "dx": "0004",
            "si": "0004", "di": "0004", "bp": "0004", "ip": "3050"},
        "m98o-report-b": {
            "ax": "98e2", "bx": "0007", "cx": "0003", "dx": "0002",
            "si": "0000", "di": "0000", "bp": "0000", "ip": "3060"},
        "m98o-report-c": {
            "ax": "98e3", "bx": "0008", "cx": "0001", "dx": "0000",
            "si": "0504", "di": "0100", "bp": "0000", "ip": "3070"},
    })
    return expected


def check_registers(directory: Path, expected, errors: list[str]) -> int:
    captures = 0
    for prefix in ALL_PREFIXES:
        try:
            values = read_tsv(directory / f"{prefix}.registers.tsv")
        except (OSError, UnicodeError, ValueError):
            add_error(errors, "M98O_REGISTERS_SCHEMA")
            continue
        captures += 1
        required = {"schema": "vaeg-registers-v1", "cs": "3000", "ds": "3000"}
        required.update(expected[prefix])
        if any(values.get(key) != value for key, value in required.items()):
            suffix = prefix.removeprefix("m98o-").upper().replace("-", "_")
            add_error(errors, f"M98O_{suffix}_SIGNATURE")
        try:
            flags = int(values.get("flags", ""), 16)
        except ValueError:
            add_error(errors, "M98O_FLAGS")
        else:
            if (flags & 0x0400) or not (flags & 0x0200):
                add_error(errors, "M98O_FLAGS")
    return captures


def parse_events(directory: Path, errors: list[str]) -> list[int]:
    try:
        lines = (directory / "events.tsv").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        add_error(errors, "M98O_EVENTS_SCHEMA")
        return []
    if not lines or lines[0] != "event\tframe\tid\tvalue":
        add_error(errors, "M98O_EVENTS_SCHEMA")
        return []
    rows = [line.split("\t") for line in lines[1:]]
    if any(len(row) != 4 for row in rows):
        add_error(errors, "M98O_EVENTS_SCHEMA")
        return []
    if any(row[0] == "frame-limit" for row in rows):
        add_error(errors, "M98O_EVENTS_TIMEOUT")
    pc_frames = [int(row[1]) for row in rows if row[0] == "pc" and row[1].isdigit()]
    captures = [(row[1], row[2]) for row in rows if row[0] == "capture"]
    expected_captures = list(zip((str(frame) for frame in pc_frames), ALL_PREFIXES))
    settled_a = len(PHASE_PREFIXES) + len(FLIP_PREFIXES)
    if (len(pc_frames) != len(ALL_PREFIXES) or captures != expected_captures
            or any(right < left for left, right in zip(pc_frames, pc_frames[1:]))
            or (len(pc_frames) == len(ALL_PREFIXES)
                and pc_frames[settled_a + 1] != pc_frames[settled_a] + 1)):
        add_error(errors, "M98O_EVENTS_SEQUENCE")
    return pc_frames


def expected_g0() -> bytes:
    row_a = bytes((0x24,) * 8 + (0x49,) * 8)
    row_b = bytes((0x49,) * 8 + (0x24,) * 8)
    output = bytearray()
    for y in range(G1_VISIBLE_HEIGHT):
        for tile in range(20):
            output.extend(row_a if ((tile + y // 16) & 1) == 0 else row_b)
    return bytes(output)


def expected_page(atlas: bytes, descriptor, position: tuple[int, int]) -> bytes:
    frame = atlas[descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes]
    page = bytearray(G1_PAGE_BYTES)
    x, y = position
    for row_index in range(descriptor.height):
        source = row_index * descriptor.pitch
        destination = (y + row_index) * G1_WIDTH + x
        page[destination:destination + descriptor.width] = frame[
            source:source + descriptor.width]
    return bytes(page)


def nonzero_bbox(surface: bytes) -> list[int] | None:
    positions = [(index % G1_WIDTH, index // G1_WIDTH)
                 for index, value in enumerate(surface) if value]
    if not positions:
        return None
    return [min(x for x, _ in positions), min(y for _, y in positions),
            max(x for x, _ in positions), max(y for _, y in positions)]


def check_gvram(directory: Path, page_a: bytes, page_b: bytes,
                errors: list[str]) -> tuple[list[bytes], dict[str, object]]:
    captures: list[bytes] = []
    for prefix in GVRAM_PREFIXES:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98O_GVRAM_MISSING")
            continue
        if len(raw) != GVRAM_SIZE:
            add_error(errors, "M98O_GVRAM_SIZE")
            continue
        captures.append(raw)
    if len(captures) != len(GVRAM_PREFIXES):
        return captures, {}
    zero_page = bytes(G1_PAGE_BYTES)
    expected_pairs = ((zero_page, page_b), (page_a, page_b),
                      (page_a, page_b), (page_a, page_b),
                      (page_a, page_b), (page_a, page_b))
    for raw, (expected_a, expected_b) in zip(captures, expected_pairs):
        if raw[:G0_SIZE] != expected_g0():
            add_error(errors, "M98O_G0_CONTENT")
        actual_a = raw[G1_OFFSET:G1_OFFSET + G1_PAGE_BYTES]
        actual_b = raw[G1_OFFSET + G1_PAGE_BYTES:G1_OFFSET + G1_BACKING_BYTES]
        if actual_a != expected_a or actual_b != expected_b:
            add_error(errors, "M98O_G1_PAGE_CONTENT")
    # The page that is visible at the start of each next batch must not change.
    if (captures[0][G1_OFFSET:G1_OFFSET + G1_PAGE_BYTES] != zero_page
            or captures[1][G1_OFFSET + G1_PAGE_BYTES:G1_OFFSET + G1_BACKING_BYTES]
            != captures[0][G1_OFFSET + G1_PAGE_BYTES:G1_OFFSET + G1_BACKING_BYTES]
            or captures[2][G1_OFFSET:G1_OFFSET + G1_PAGE_BYTES]
            != captures[1][G1_OFFSET:G1_OFFSET + G1_PAGE_BYTES]
            or captures[3][G1_OFFSET + G1_PAGE_BYTES:G1_OFFSET + G1_BACKING_BYTES]
            != captures[2][G1_OFFSET + G1_PAGE_BYTES:G1_OFFSET + G1_BACKING_BYTES]):
        add_error(errors, "M98O_VISIBLE_PAGE_MODIFIED")
    if captures[-1] != captures[-2]:
        add_error(errors, "M98O_GVRAM_UNSTABLE")
    return captures, {
        "page_a_bbox": nonzero_bbox(page_a),
        "page_a_nonzero": sum(value != 0 for value in page_a),
        "page_a_sha256": sha256(page_a),
        "page_b_bbox": nonzero_bbox(page_b),
        "page_b_nonzero": sum(value != 0 for value in page_b),
        "page_b_sha256": sha256(page_b),
    }


def bmp_rgb_nonblack(path: Path) -> bool:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("M98O_SCREEN_FORMAT")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = struct.unpack_from("<i", data, 22)[0]
    planes, bits = struct.unpack_from("<HH", data, 26)
    if width < 320 or abs(height) < 200 or planes != 1 or bits not in (24, 32):
        raise ValueError("M98O_SCREEN_FORMAT")
    pixel_size = bits // 8
    row_size = ((width * bits + 31) // 32) * 4
    if pixel_offset + row_size * abs(height) > len(data):
        raise ValueError("M98O_SCREEN_FORMAT")
    return any(data[pixel_offset + row * row_size + column * pixel_size + channel]
               for row in range(abs(height)) for column in range(width)
               for channel in range(3))


def check_screens(directory: Path, errors: list[str]) -> list[bytes]:
    screens: list[bytes] = []
    for prefix in GVRAM_PREFIXES:
        path = directory / f"{prefix}.screen.bmp"
        try:
            screens.append(path.read_bytes())
            if not bmp_rgb_nonblack(path):
                add_error(errors, "M98O_SCREEN_BLACK")
        except OSError:
            add_error(errors, "M98O_SCREEN_MISSING")
        except ValueError:
            add_error(errors, "M98O_SCREEN_FORMAT")
    if len(screens) == len(GVRAM_PREFIXES):
        if screens[-1] != screens[-2]:
            add_error(errors, "M98O_SCREEN_UNSTABLE")
        if screens[0] != screens[2] or screens[1] != screens[3]:
            add_error(errors, "M98O_SCREEN_PARITY")
    return screens


def check_trace(trace: Path, descriptor, errors: list[str]) -> dict[str, object]:
    source_address = BMS_WINDOW_BASE + descriptor.bank_offset
    destinations = (
        G1_PAGE_B_SGP + POSITION_P0[1] * G1_WIDTH + POSITION_P0[0],
        G1_PAGE_A_SGP + POSITION_P1[1] * G1_WIDTH + POSITION_P1[0],
        G1_PAGE_B_SGP + POSITION_P0[1] * G1_WIDTH + POSITION_P0[0],
        G1_PAGE_A_SGP + POSITION_P1[1] * G1_WIDTH + POSITION_P1[0])
    try:
        lines = trace.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98O_TRACE_MISSING")
        return {}
    sources = [match for line in lines if (match := TRACE_SOURCE.match(line))]
    destinations_found = [match for line in lines
                          if (match := TRACE_DESTINATION.match(line))]
    decode = lambda match: tuple(  # noqa: E731
        int(match.group(index), 16 if index == 1 else 10) for index in range(1, 7))
    source_values = [decode(match) for match in sources]
    destination_values = [decode(match) for match in destinations_found]
    expected_source = (source_address, 0, 2, descriptor.width,
                       descriptor.height, descriptor.pitch)
    expected_destinations = [(address, 0, 2, descriptor.width,
                              descriptor.height, G1_WIDTH)
                             for address in destinations]
    if source_values != [expected_source] * 4:
        add_error(errors, "M98O_TRACE_BMS_SOURCE_SEQUENCE")
    if destination_values != expected_destinations:
        add_error(errors, "M98O_TRACE_HIDDEN_DESTINATION_SEQUENCE")
    return {"bitblt_count": len(source_values),
            "destination_addresses": list(destinations),
            "source_address": source_address}


def listing_symbol_offset(path: Path, symbol: str) -> int | None:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return None
    label = re.compile(rf"(?:^|\s){re.escape(symbol)}:\s*$")
    for index, line in enumerate(lines):
        if label.search(line):
            for following in lines[index + 1:index + 4]:
                match = re.search(r"\s([0-9A-Fa-f]{8})\s", following)
                if match:
                    return int(match.group(1), 16) + 0x100
    return None


def check_source(source_path: Path, listing: Path, errors: list[str]) -> int | None:
    try:
        source = source_path.read_text(encoding="utf-8")
    except (OSError, UnicodeError):
        add_error(errors, "M98O_SOURCE_ASSEMBLY")
        return None
    lowered = source.lower()
    if "incbin" in lowered or "%include" in lowered:
        add_error(errors, "M98O_SOURCE_EMBEDDED_ATLAS")
    required = (
        "%define G1_PAGE_BYTES           0xfa00",
        "%define G1_PAGE_WORD_COUNT      0x7d00",
        "%define G1_PAGE_B_SGP_BASE      0x22fa00",
        "%define G1_PAGE_B_DSA           0x02fa00",
        "%define POSITION_P0_X           48",
        "%define POSITION_P1_X           248",
        "%define PAGE_UNINITIALIZED      0",
        "%define PAGE_HIDDEN_RENDERING   2",
        "%define PAGE_HIDDEN_COMPLETE    3",
        "%define PAGE_VISIBLE            4",
        "call select_render_bms", "call wait_vblank_edge",
        "call publish_page", "call select_render_ordinary")
    if any(text not in source for text in required):
        add_error(errors, "M98O_SOURCE_CONTRACT")
    if source.count("mov ax, SGP_COMMAND_BITBLT") != 1:
        add_error(errors, "M98O_SOURCE_BITBLT_TEMPLATE")
    if source.count("call run_sgp_command_list") != 2:
        add_error(errors, "M98O_SOURCE_SUBMISSION_PATHS")
    try:
        listing_text = listing.read_text(encoding="utf-8", errors="replace")
    except OSError:
        listing_text = ""
    if re.search(r"\b0F8[0-9A-Fa-f]", listing_text):
        add_error(errors, "M98O_VA2_INSTRUCTION_SET")
    offset = listing_symbol_offset(listing, "staging_buffer")
    if offset is None:
        add_error(errors, "M98O_LISTING_STAGING")
    return offset


def artifact_records(directory: Path) -> dict[str, dict[str, object]]:
    names = ["ZUNDORB.COM", "ZUNDORB.LST", "ZUNDORB.BIN",
             "zundamon-orbit-m98o-pristine.d88", "zundamon-orbit-m98o.d88",
             "events.tsv", "sgp-trace.log", "vaeg.stdout.log"]
    names.extend(f"{prefix}.registers.tsv" for prefix in ALL_PREFIXES)
    for prefix in GVRAM_PREFIXES:
        names.extend((f"{prefix}.gvram.bin", f"{prefix}.screen.bmp"))
    records = {}
    for name in names:
        path = directory / name
        if path.is_file():
            data = path.read_bytes()
            records[name] = {"sha256": sha256(data), "size": len(data)}
    return records


def verify(directory: Path, source: Path, atlas_path: Path, trace: Path) -> dict[str, object]:
    errors: list[str] = []
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
    except (OSError, atlas_format.AtlasError):
        return {"errors": ["M98O_ATLAS_FORMAT"],
                "schema": "zundamon-orbit-m98o-oracle-v1", "status": "FAIL"}
    descriptor = descriptors[-1]
    if (descriptor.width, descriptor.height, descriptor.pitch) != (23, 19, 24):
        add_error(errors, "M98O_SELECTED_CELL_GEOMETRY")
    page_a = expected_page(atlas, descriptor, POSITION_P1)
    page_b = expected_page(atlas, descriptor, POSITION_P0)
    register_count = check_registers(directory, expected_registers(header, descriptor), errors)
    pc_frames = parse_events(directory, errors)
    gvram, page_report = check_gvram(directory, page_a, page_b, errors)
    screens = check_screens(directory, errors)
    trace_report = check_trace(trace, descriptor, errors)
    staging_offset = check_source(source, directory / "ZUNDORB.LST", errors)
    selected_frame = atlas[
        descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes]
    result = {
        "artifacts": artifact_records(directory),
        "atlas": {"descriptor": asdict(descriptor),
                  "file_class": "generated-public-fixture", "file_size": len(atlas),
                  "selected_cell": ATLAS_SELECTED_CELL,
                  "selected_cell_sha256": sha256(selected_frame),
                  "sha256": sha256(atlas), "transparent_index": 0},
        "bms": {"bank_size": 0x20000, "base_port": "01d0h",
                "ordinary_selector": 0, "selected_selector": 1,
                "window": "80000h-9ffffh"},
        "counters": {"bms_bank_switches": 8, "cleanup_runs": 1,
                     "full_page_clears": 4, "page_a_publications": 3,
                     "page_b_publications": 2, "page_flips": 4,
                     "pages_initialized": 2, "render_batches_completed": 4,
                     "render_batches_started": 4, "sgp_errors": 0,
                     "sgp_timeouts": 0, "transparent_bitblts": 4,
                     "vblank_edges_seen": 7, "vblank_timeouts": 0},
        "errors": errors,
        "mode": {"height": 200, "layers": ["G0", "G1"],
                 "pixel_bits": 8, "width": 320},
        "page_geometry": {"backing_height": G1_BACKING_HEIGHT,
                          "page_a_dsa": G1_PAGE_A_DSA,
                          "page_a_sgp": G1_PAGE_A_SGP,
                          "page_b_dsa": G1_PAGE_B_DSA,
                          "page_b_sgp": G1_PAGE_B_SGP,
                          "page_bytes": G1_PAGE_BYTES, "pitch": G1_WIDTH,
                          "position_p0": list(POSITION_P0),
                          "position_p1": list(POSITION_P1)},
        "page_identities": page_report, "pc_frames": pc_frames,
        "register_captures": register_count,
        "schema": "zundamon-orbit-m98o-oracle-v1",
        "settled_gvram_stable": len(gvram) == len(GVRAM_PREFIXES)
                                and gvram[-1] == gvram[-2],
        "settled_screen_stable": len(screens) == len(GVRAM_PREFIXES)
                                and screens[-1] == screens[-2],
        "sgp": {"bitblt_mode": "0105h", "trace": trace_report},
        "staging": {"address": (f"3000:{staging_offset:04x}"
                                  if staging_offset is not None else None),
                    "maximum_bytes": STAGING_BYTES, "poison": "a5h"},
        "status": "PASS" if not errors else "FAIL",
    }
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    source = args.source or Path(__file__).resolve().parents[1] / "256" / "zundamon_orbit_256.asm"
    result = verify(args.directory, source, args.atlas, args.trace)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        if args.report.parent != args.directory or args.report.exists():
            raise SystemExit("M98O_REPORT_PATH")
        args.report.write_text(encoded, encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

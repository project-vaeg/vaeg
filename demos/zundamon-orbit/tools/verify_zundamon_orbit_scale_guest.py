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

"""Fail-closed oracle for the M98p 30-scale full-page-CLS baseline."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
import zlib
from dataclasses import asdict
from pathlib import Path

TOOLS_DIRECTORY = Path(__file__).resolve().parent
if str(TOOLS_DIRECTORY) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIRECTORY))
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402

WIDTH = 320
HEIGHT = 200
PITCH = 320
G0_BYTES = WIDTH * HEIGHT
G1_OFFSET = 0x20000
PAGE_BYTES = WIDTH * HEIGHT
GVRAM_BYTES = 0x40000
PAGE_SGP = (0x220000, 0x22FA00)
PAGE_DSA = (0x020000, 0x02FA00)
BMS_WINDOW = 0x080000
TARGET_ANCHOR = (160, 100)
STAGING_BYTES = 4096
FLIP_COUNT = 58
TRACE_SOURCE = re.compile(
    r"^SGP_SCAN: SET_SOURCE addr=([0-9a-fA-F]+) dot=(\d+) mode=(\d+) "
    r"width=(\d+) height=(\d+) fbw=(\d+)$")
TRACE_DESTINATION = re.compile(
    r"^SGP_SCAN: SET_DEST addr=([0-9a-fA-F]+) dot=(\d+) mode=(\d+) "
    r"width=(\d+) height=(\d+) fbw=(\d+)$")


class OracleError(Exception):
    """One stable M98p host-oracle failure."""

    def __init__(self, code: str):
        super().__init__(code)
        self.code = code


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def add_error(errors: list[str], code: str) -> None:
    if code not in errors:
        errors.append(code)


def scale_sequence() -> tuple[int, ...]:
    return tuple(range(30, 0, -1)) + tuple(range(2, 30))


def scale_directions() -> tuple[int, ...]:
    return (0,) * 30 + (1,) * 28


def validate_scale_sequence(sequence: tuple[int, ...]) -> None:
    if len(sequence) != FLIP_COUNT:
        raise OracleError("M98P_SEQUENCE_LENGTH")
    if any(scale_id < 1 or scale_id > 30 for scale_id in sequence):
        raise OracleError("M98P_SEQUENCE_SCALE_ID")
    for left, right in zip(sequence, sequence[1:]):
        if left == right and left in (1, 30):
            raise OracleError("M98P_SEQUENCE_ENDPOINT_DUPLICATE")
    if set(sequence) != set(range(1, 31)):
        raise OracleError("M98P_SEQUENCE_SCALE_SKIP")
    if sequence != scale_sequence():
        raise OracleError("M98P_SEQUENCE_ORDER")


def validate_runtime_descriptors(header, descriptors) -> None:
    if len(descriptors) != 30:
        raise OracleError("M98P_DESCRIPTOR_COUNT")
    if header.required_bank_count != 1 or header.payload_bytes > atlas_format.BANK_SIZE:
        raise OracleError("M98P_ATLAS_EXCEEDS_ONE_BANK")
    expected_offset = 0
    for scale_id, descriptor in enumerate(descriptors, 1):
        if not (1 <= descriptor.width <= WIDTH
                and 1 <= descriptor.height <= HEIGHT):
            raise OracleError("M98P_DESCRIPTOR_DIMENSIONS")
        if descriptor.bank_slot != 0:
            raise OracleError("M98P_DESCRIPTOR_BANK")
        if descriptor.pitch < descriptor.width:
            raise OracleError("M98P_DESCRIPTOR_PITCH")
        if descriptor.payload_bytes != descriptor.pitch * descriptor.height:
            raise OracleError("M98P_DESCRIPTOR_PAYLOAD")
        if not (0 <= descriptor.anchor_x < descriptor.width
                and 0 <= descriptor.anchor_y < descriptor.height):
            raise OracleError("M98P_DESCRIPTOR_ANCHOR")
        if descriptor.bank_offset + descriptor.payload_bytes > atlas_format.BANK_SIZE:
            raise OracleError("M98P_DESCRIPTOR_BANK_CROSSING")
        if descriptor.bank_offset + descriptor.payload_bytes > header.payload_bytes:
            raise OracleError("M98P_DESCRIPTOR_SOURCE_RANGE")
        destination = destination_for(descriptor)
        if (destination[0] < 0 or destination[1] < 0
                or destination[0] + descriptor.width > WIDTH
                or destination[1] + descriptor.height > HEIGHT):
            raise OracleError("M98P_DESCRIPTOR_DESTINATION")
        if not 1 <= scale_id <= 30:
            raise OracleError("M98P_DESCRIPTOR_ID")
        if descriptor.bank_offset != expected_offset:
            raise OracleError("M98P_DESCRIPTOR_ORDER")
        expected_offset = atlas_format.align_up(
            descriptor.bank_offset + descriptor.payload_bytes, 16)


def validate_frame_crcs(atlas: bytes, descriptors) -> None:
    for descriptor in descriptors:
        payload = atlas[
            descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes]
        if zlib.crc32(payload) & 0xffffffff != descriptor.frame_crc32:
            raise OracleError("M98P_FRAME_CRC")


def destination_for(descriptor) -> tuple[int, int]:
    return (TARGET_ANCHOR[0] - descriptor.anchor_x,
            TARGET_ANCHOR[1] - descriptor.anchor_y)


def expected_g0() -> bytes:
    row_a = bytes((0x24,) * 8 + (0x49,) * 8)
    row_b = bytes((0x49,) * 8 + (0x24,) * 8)
    output = bytearray()
    for y in range(HEIGHT):
        for tile in range(20):
            output.extend(row_a if ((tile + y // 16) & 1) == 0 else row_b)
    return bytes(output)


def expected_page(atlas: bytes, descriptor) -> bytes:
    frame = atlas[
        descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes]
    page = bytearray(PAGE_BYTES)
    x, y = destination_for(descriptor)
    for row in range(descriptor.height):
        source = row * descriptor.pitch
        destination = (y + row) * PITCH + x
        page[destination:destination + descriptor.width] = frame[
            source:source + descriptor.width]
    return bytes(page)


def composite(g0: bytes, g1: bytes) -> bytes:
    return bytes(upper if upper else lower for lower, upper in zip(g0, g1))


def nonzero_bbox(surface: bytes) -> list[int] | None:
    points = [(offset % WIDTH, offset // WIDTH)
              for offset, value in enumerate(surface) if value]
    if not points:
        return None
    return [min(x for x, _ in points), min(y for _, y in points),
            max(x for x, _ in points), max(y for _, y in points)]


def read_registers(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or not key or key in values:
            raise OracleError("M98P_REGISTERS_SCHEMA")
        values[key] = value
    return values


def require_registers(directory: Path, prefix: str,
                      expected: dict[str, int | str], errors: list[str]) -> None:
    try:
        values = read_registers(directory / f"{prefix}.registers.tsv")
    except (OSError, UnicodeError, OracleError):
        add_error(errors, "M98P_REGISTERS_SCHEMA")
        return
    required = {"schema": "vaeg-registers-v1", "cs": "3000", "ds": "3000"}
    required.update({key: f"{value:04x}" if isinstance(value, int) else value
                     for key, value in expected.items()})
    if any(values.get(key) != value for key, value in required.items()):
        add_error(errors, "M98P_REGISTER_SIGNATURE")
    try:
        flags = int(values.get("flags", ""), 16)
    except ValueError:
        add_error(errors, "M98P_REGISTER_FLAGS")
    else:
        if (flags & 0x0400) or not (flags & 0x0200):
            add_error(errors, "M98P_REGISTER_FLAGS")


def prefixes(initial_page: str) -> tuple[str, tuple[str, ...], tuple[str, ...]]:
    root = f"m98p-{initial_page}"
    flips = tuple(f"{root}-flip-{index:02d}" for index in range(1, 59))
    settled = (f"{root}-settled-a", f"{root}-settled-b")
    return root, flips, settled


def publication_digest(sequence: tuple[int, ...], directions: tuple[int, ...],
                       initial_page_index: int) -> int:
    low = 0
    high = 0
    for index, (scale_id, direction) in enumerate(zip(sequence, directions), 1):
        page = initial_page_index ^ (index & 1)
        total = low + scale_id + (direction << 8)
        low = total & 0xffff
        high = (high + (total >> 16) + page + 1) & 0xffff
    return low | (high << 16)


def expected_register_map(header, descriptors, initial_page: str):
    root, flips, settled = prefixes(initial_page)
    initial = 0 if initial_page == "a" else 1
    sequence = scale_sequence()
    directions = scale_directions()
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
        f"{root}-initialize": {"ax": 0x98C1, "bx": PAGE_BYTES, "cx": PITCH,
                               "dx": 0x0105,
                               "si": (BMS_WINDOW + maximum.bank_offset) & 0xffff,
                               "di": (BMS_WINDOW + maximum.bank_offset) >> 16,
                               "bp": 0x0201, "ip": 0x4020},
    }
    for index, (prefix, scale_id, direction) in enumerate(
            zip(flips, sequence, directions), 1):
        page = initial ^ (index & 1)
        descriptor = descriptors[scale_id - 1]
        x, y = destination_for(descriptor)
        mapping[prefix] = {
            "ax": 0x98D1, "bx": index, "cx": scale_id, "dx": direction,
            "si": page, "di": x, "bp": y, "ip": 0x4030,
        }
    final_dsa = PAGE_DSA[initial]
    for prefix in settled:
        mapping[prefix] = {
            "ax": 0x98D2, "bx": FLIP_COUNT, "cx": 1, "dx": 2,
            "si": FLIP_COUNT, "di": initial,
            "bp": final_dsa & 0xffff, "ip": 0x4040,
        }
    source_bytes = sum(descriptors[scale_id - 1].payload_bytes
                       for scale_id in sequence)
    cleared_bytes = FLIP_COUNT * PAGE_BYTES
    digest = publication_digest(sequence, directions, initial)
    page_counts = (30, 29) if initial == 0 else (29, 30)
    mapping.update({
        f"{root}-report-a": {"ax": 0x98E1, "bx": 2, "cx": FLIP_COUNT,
                              "dx": FLIP_COUNT, "si": FLIP_COUNT,
                              "di": FLIP_COUNT, "bp": FLIP_COUNT, "ip": 0x4050},
        f"{root}-report-b": {"ax": 0x98E2, "bx": 61, "cx": page_counts[0],
                              "dx": page_counts[1], "si": 0, "di": 0,
                              "bp": 0, "ip": 0x4060},
        f"{root}-report-c": {"ax": 0x98E3, "bx": 116, "cx": 1,
                              "dx": initial, "si": 1, "di": 2,
                              "bp": 0, "ip": 0x4070},
        f"{root}-report-d": {"ax": 0x98E4, "bx": source_bytes & 0xffff,
                              "cx": source_bytes >> 16,
                              "dx": cleared_bytes & 0xffff,
                              "si": cleared_bytes >> 16, "di": FLIP_COUNT,
                              "bp": final_dsa & 0xffff, "ip": 0x4080},
        f"{root}-report-e": {"ax": 0x98E5, "bx": digest & 0xffff,
                              "cx": digest >> 16, "dx": FLIP_COUNT,
                              "si": 30, "di": 28, "bp": 1, "ip": 0x4090},
    })
    return mapping


def check_events(directory: Path, expected_prefixes: tuple[str, ...],
                 errors: list[str]) -> list[int]:
    try:
        lines = (directory / "events.tsv").read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        add_error(errors, "M98P_EVENTS_SCHEMA")
        return []
    if not lines or lines[0] != "event\tframe\tid\tvalue":
        add_error(errors, "M98P_EVENTS_SCHEMA")
        return []
    rows = [line.split("\t") for line in lines[1:]]
    if any(len(row) != 4 for row in rows):
        add_error(errors, "M98P_EVENTS_SCHEMA")
        return []
    if any(row[0] == "frame-limit" for row in rows):
        add_error(errors, "M98P_EVENTS_TIMEOUT")
    pc_frames = [int(row[1]) for row in rows if row[0] == "pc" and row[1].isdigit()]
    captures = [(row[1], row[2]) for row in rows if row[0] == "capture"]
    expected_captures = list(zip((str(frame) for frame in pc_frames), expected_prefixes))
    if (len(pc_frames) != len(expected_prefixes) or captures != expected_captures
            or any(right < left for left, right in zip(pc_frames, pc_frames[1:]))):
        add_error(errors, "M98P_EVENTS_SEQUENCE")
    settled_index = 3 + FLIP_COUNT
    if (len(pc_frames) == len(expected_prefixes)
            and pc_frames[settled_index + 1] != pc_frames[settled_index] + 1):
        add_error(errors, "M98P_SETTLED_FRAME_SEQUENCE")
    return pc_frames


def check_gvram(directory: Path, atlas: bytes, descriptors, initial_page: str,
                errors: list[str]) -> tuple[list[dict[str, object]], bytes, bytes]:
    root, flips, settled = prefixes(initial_page)
    initial = 0 if initial_page == "a" else 1
    pages = [bytes(PAGE_BYTES), bytes(PAGE_BYTES)]
    g0 = expected_g0()
    records = []
    final_raw = b""
    for index, (prefix, scale_id, direction) in enumerate(
            zip(flips, scale_sequence(), scale_directions()), 1):
        page = initial ^ (index & 1)
        descriptor = descriptors[scale_id - 1]
        expected = expected_page(atlas, descriptor)
        pages[page] = expected
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98P_GVRAM_MISSING")
            continue
        if len(raw) != GVRAM_BYTES:
            add_error(errors, "M98P_GVRAM_SIZE")
            continue
        if raw[:G0_BYTES] != g0:
            add_error(errors, "M98P_G0_CONTENT")
        actual_pages = (raw[G1_OFFSET:G1_OFFSET + PAGE_BYTES],
                        raw[G1_OFFSET + PAGE_BYTES:G1_OFFSET + PAGE_BYTES * 2])
        if actual_pages != tuple(pages):
            add_error(errors, "M98P_G1_PAGE_CONTENT")
        frame = atlas[
            descriptor.file_offset:descriptor.file_offset + descriptor.payload_bytes]
        if not any(value == 0 for row in range(descriptor.height)
                   for value in frame[row * descriptor.pitch:
                                      row * descriptor.pitch + descriptor.width]):
            add_error(errors, "M98P_TRANSPARENT_HOLE")
        composed = composite(g0, expected)
        records.append({
            "direction": "shrink" if direction == 0 else "grow",
            "destination": list(destination_for(descriptor)),
            "g1_bbox": nonzero_bbox(expected),
            "g1_nonzero": sum(value != 0 for value in expected),
            "g1_sha256": sha256(expected),
            "composite_sha256": sha256(composed),
            "page": "A" if page == 0 else "B",
            "publication": index,
            "scale_id": scale_id,
        })
        final_raw = raw
    settled_raw: list[bytes] = []
    for prefix in settled:
        try:
            raw = (directory / f"{prefix}.gvram.bin").read_bytes()
        except OSError:
            add_error(errors, "M98P_GVRAM_MISSING")
            continue
        if raw != final_raw:
            add_error(errors, "M98P_GVRAM_UNSTABLE")
        settled_raw.append(raw)
    return records, final_raw, b"".join(settled_raw)


def bmp_nonblack(path: Path) -> bool:
    data = path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise OracleError("M98P_SCREEN_FORMAT")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height = abs(struct.unpack_from("<i", data, 22)[0])
    planes, bits = struct.unpack_from("<HH", data, 26)
    if width < WIDTH or height < HEIGHT or planes != 1 or bits not in (24, 32):
        raise OracleError("M98P_SCREEN_FORMAT")
    pixel_bytes = bits // 8
    row_bytes = ((width * bits + 31) // 32) * 4
    if pixel_offset + row_bytes * height > len(data):
        raise OracleError("M98P_SCREEN_FORMAT")
    return any(data[pixel_offset + row * row_bytes + column * pixel_bytes + channel]
               for row in range(height) for column in range(width) for channel in range(3))


def check_screens(directory: Path, settled: tuple[str, ...], errors: list[str]) -> bool:
    screens = []
    for prefix in settled:
        try:
            path = directory / f"{prefix}.screen.bmp"
            screens.append(path.read_bytes())
            if not bmp_nonblack(path):
                add_error(errors, "M98P_SCREEN_BLACK")
        except OSError:
            add_error(errors, "M98P_SCREEN_MISSING")
        except OracleError as error:
            add_error(errors, error.code)
    if len(screens) == 2 and screens[0] != screens[1]:
        add_error(errors, "M98P_SCREEN_UNSTABLE")
    return len(screens) == 2 and screens[0] == screens[1]


def check_trace(path: Path, descriptors, initial_page: str,
                errors: list[str]) -> dict[str, object]:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        add_error(errors, "M98P_TRACE_MISSING")
        return {}
    decode = lambda match: tuple(  # noqa: E731
        int(match.group(index), 16 if index == 1 else 10) for index in range(1, 7))
    sources = [decode(match) for line in lines
               if (match := TRACE_SOURCE.match(line))]
    destinations = [decode(match) for line in lines
                    if (match := TRACE_DESTINATION.match(line))]
    initial = 0 if initial_page == "a" else 1
    expected_sources = []
    expected_destinations = []
    for index, scale_id in enumerate(scale_sequence(), 1):
        descriptor = descriptors[scale_id - 1]
        x, y = destination_for(descriptor)
        page = initial ^ (index & 1)
        expected_sources.append((BMS_WINDOW + descriptor.bank_offset, 0, 2,
                                 descriptor.width, descriptor.height,
                                 descriptor.pitch))
        expected_destinations.append((PAGE_SGP[page] + y * PITCH + (x & ~1),
                                      x & 1, 2,
                                      descriptor.width, descriptor.height, PITCH))
    if sources != expected_sources:
        add_error(errors, "M98P_TRACE_BMS_SOURCE_SEQUENCE")
    if destinations != expected_destinations:
        add_error(errors, "M98P_TRACE_HIDDEN_DESTINATION_SEQUENCE")
    return {"bitblt_count": len(sources),
            "first_source": sources[0][0] if sources else None,
            "last_source": sources[-1][0] if sources else None}


def check_source(source: Path, listing: Path, errors: list[str]) -> None:
    try:
        text = source.read_text(encoding="utf-8")
        listing_text = listing.read_text(encoding="utf-8", errors="replace")
    except (OSError, UnicodeError):
        add_error(errors, "M98P_SOURCE_READ")
        return
    lowered = text.lower()
    if "incbin" in lowered or "%include" in lowered:
        add_error(errors, "M98P_SOURCE_EMBEDDED_ATLAS")
    required = (
        "%define SCALE_PUBLICATIONS_PER_CYCLE 58",
        "%define TARGET_ANCHOR_X         160",
        "call select_scale_descriptor", "call record_scale_publication",
        "call advance_scale_sequence", "call wait_vblank_edge",
        "call publish_page", "call select_render_ordinary")
    if any(item not in text for item in required):
        add_error(errors, "M98P_SOURCE_CONTRACT")
    if text.count("mov ax, SGP_COMMAND_BITBLT") != 1:
        add_error(errors, "M98P_SOURCE_BITBLT_TEMPLATE")
    if text.count("mov ax, SGP_COMMAND_CLS") != 3:
        add_error(errors, "M98P_SOURCE_CLS_TEMPLATES")
    if re.search(r"\b0F8[0-9A-Fa-f]", listing_text):
        add_error(errors, "M98P_VA2_INSTRUCTION_SET")


def artifact_records(directory: Path) -> dict[str, dict[str, object]]:
    records = {}
    for path in sorted(candidate for candidate in directory.iterdir()
                       if candidate.is_file()):
        if path.suffix.lower() in (".com", ".lst", ".bin", ".d88", ".tsv",
                                   ".log", ".bmp", ".json", ".debug"):
            data = path.read_bytes()
            records[path.name] = {"size": len(data), "sha256": sha256(data)}
    return records


def verify(directory: Path, source: Path, atlas_path: Path, trace: Path,
           initial_page: str) -> dict[str, object]:
    errors: list[str] = []
    try:
        atlas = atlas_path.read_bytes()
        header, descriptors = atlas_format.inspect_bytes(atlas)
        validate_runtime_descriptors(header, descriptors)
        validate_frame_crcs(atlas, descriptors)
        validate_scale_sequence(scale_sequence())
    except OSError:
        return {"schema": "zundamon-orbit-m98p-oracle-v1", "status": "FAIL",
                "errors": ["M98P_ATLAS_READ"]}
    except atlas_format.AtlasError as error:
        return {"schema": "zundamon-orbit-m98p-oracle-v1", "status": "FAIL",
                "errors": [f"M98P_ATLAS_{error.code}"]}
    except OracleError as error:
        return {"schema": "zundamon-orbit-m98p-oracle-v1", "status": "FAIL",
                "errors": [error.code]}
    root, flips, settled = prefixes(initial_page)
    register_map = expected_register_map(header, descriptors, initial_page)
    ordered_prefixes = ((f"{root}-probe", f"{root}-load", f"{root}-initialize")
                        + flips + settled
                        + tuple(f"{root}-report-{letter}" for letter in "abcde"))
    for prefix in ordered_prefixes:
        require_registers(directory, prefix, register_map[prefix], errors)
    pc_frames = check_events(directory, ordered_prefixes, errors)
    frame_records, final_raw, settled_raw = check_gvram(
        directory, atlas, descriptors, initial_page, errors)
    screen_stable = check_screens(directory, settled, errors)
    trace_report = check_trace(trace, descriptors, initial_page, errors)
    check_source(source, directory / "ZUNDORB.LST", errors)
    sequence = scale_sequence()
    descriptor_rows = []
    for scale_id, descriptor in enumerate(descriptors, 1):
        row = asdict(descriptor)
        row.update({"scale_id": scale_id,
                    "destination": list(destination_for(descriptor)),
                    "sgp_source": BMS_WINDOW + descriptor.bank_offset})
        descriptor_rows.append(row)
    return {
        "artifacts": artifact_records(directory),
        "atlas": {"descriptor_count": len(descriptors),
                  "descriptors": descriptor_rows, "file_size": len(atlas),
                  "payload_bytes": header.payload_bytes,
                  "required_bank_count": header.required_bank_count,
                  "sha256": sha256(atlas), "transparent_index": 0,
                  "version": 1},
        "bms": {"bank_size": atlas_format.BANK_SIZE, "base_port": "01d0h",
                "ordinary_selector": 0, "selected_selector": 1,
                "window": "80000h-9ffffh"},
        "errors": errors,
        "initial_visible_page": initial_page.upper(),
        "mode": {"width": WIDTH, "height": HEIGHT, "pixel_bits": 8,
                 "layers": ["G0", "G1"]},
        "page_geometry": {"backing_height": 400, "page_bytes": PAGE_BYTES,
                          "page_dsa": list(PAGE_DSA), "page_sgp": list(PAGE_SGP),
                          "pitch": PITCH, "target_anchor": list(TARGET_ANCHOR)},
        "pc_frames": pc_frames,
        "publication_records": frame_records,
        "sequence": list(sequence),
        "settled_gvram_stable": bool(final_raw) and settled_raw == final_raw * 2,
        "settled_screen_stable": screen_stable,
        "sgp": {"bitblt_mode": "0105h", "full_page_cls_bytes": PAGE_BYTES,
                "trace": trace_report},
        "status": "PASS" if not errors else "FAIL",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    source = args.source or Path(__file__).resolve().parents[1] / "256" / "zundamon_orbit_256.asm"
    result = verify(args.directory, source, args.atlas, args.trace, args.initial_page)
    encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        if args.report.parent != args.directory or args.report.exists():
            raise SystemExit("M98P_REPORT_PATH")
        args.report.write_text(encoded, encoding="utf-8")
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

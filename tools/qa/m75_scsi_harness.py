#!/usr/bin/env python3
"""Run one M75 guest scenario and retain a same-run screen/trace pair."""

# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.

import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path


HEADER = b"VAEGSCN1"
TEXTMEM_SIZE = 0x40000


def u32(data, offset):
    return int.from_bytes(data[offset:offset + 4], "little")


def jis_char(word):
    row = (word & 0xff) + 0x20
    cell = word >> 8
    if not (0x21 <= row <= 0x7e and 0x21 <= cell <= 0x7e):
        return "?"
    payload = b"\x1b$B" + bytes((row, cell)) + b"\x1b(B"
    return payload.decode("iso2022_jp")


def decode_cell(raw, offset):
    first = raw[offset]
    second = raw[offset + 1]
    if second == 0 and 0x20 <= first < 0x7f:
        return chr(first), 1
    word = first | (second << 8)
    return jis_char(word), 1


def decode_screen(path):
    data = Path(path).read_bytes()
    if data[:8] != HEADER:
        raise ValueError("invalid screen capture magic")
    version = u32(data, 8)
    run_id_length = u32(data, 12)
    position = 16
    run_id = data[position:position + run_id_length].decode("utf-8")
    position += run_id_length
    texttable = u32(data, position)
    attroffset = u32(data, position + 4)
    lineheight = u32(data, position + 8)
    curn = u32(data, position + 12)
    sprtable = u32(data, position + 16)
    be = u32(data, position + 20)
    textmem_size = u32(data, position + 24)
    position += 28
    textmem = data[position:position + textmem_size]
    if len(textmem) != TEXTMEM_SIZE:
        raise ValueError("unexpected text memory size")

    frame = texttable
    width = (int.from_bytes(textmem[frame + 0x16:frame + 0x18], "little")
             & 0x3ff) // 8
    raster_start = int.from_bytes(textmem[frame + 0x10:frame + 0x14], "little")
    raster_height = int.from_bytes(textmem[frame + 0x14:frame + 0x16], "little") & 0x1fe
    raster_width = int.from_bytes(textmem[frame + 0x16:frame + 0x18], "little") & 0x3ff
    chars_per_row = raster_width // 8
    rows = max(1, raster_height // max(1, lineheight))
    lines = []
    for row in range(rows):
        base = raster_start + row * chars_per_row * 2
        chars = []
        previous_word = None
        column = 0
        while column < width and base + column * 2 + 1 < len(textmem):
            word = int.from_bytes(textmem[base + column * 2:base + column * 2 + 2],
                                  "little")
            if (previous_word is not None) and (word == (previous_word | 0x8000)):
                column += 1
                continue
            char, _ = decode_cell(textmem, base + column * 2)
            chars.append(char)
            previous_word = word
            column += 1
        lines.append("".join(chars).rstrip())
    return {
        "version": version,
        "run_id": run_id,
        "texttable": texttable,
        "attroffset": attroffset,
        "lineheight": lineheight,
        "curn": curn,
        "sprtable": sprtable,
        "be": be,
        "lines": lines,
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--worker", required=True)
    parser.add_argument("--screen-out", required=True,
                        help="TVRAM dump output path")
    parser.add_argument("--rendered-screen-out",
                        help="SDL rendered BMP output path")
    parser.add_argument("--trace-out", required=True)
    parser.add_argument("--timeout", type=float, default=900.0)
    parser.add_argument("worker_args", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    worker_args = list(args.worker_args)
    if worker_args[:1] == ["--"]:
        worker_args = worker_args[1:]

    run_id = f"{time.time_ns():x}"
    screen_path = Path(args.screen_out)
    rendered_screen_path = (Path(args.rendered_screen_out)
                            if args.rendered_screen_out else None)
    trace_path = Path(args.trace_out)
    screen_path.unlink(missing_ok=True)
    if rendered_screen_path is not None:
        rendered_screen_path.unlink(missing_ok=True)
    trace_path.unlink(missing_ok=True)
    environment = os.environ.copy()
    environment.update({
        "SDL_VIDEODRIVER": "dummy",
        "SDL_AUDIODRIVER": "dummy",
        "VAEG_SCREEN_TVRAM_DUMP": str(screen_path),
        "VAEG_SCREEN_RUN_ID": run_id,
    })
    if rendered_screen_path is not None:
        environment["VAEG_SCREEN_DUMP"] = str(rendered_screen_path)
    with trace_path.open("wb") as trace_file:
        try:
            completed = subprocess.run(
                [args.worker, *worker_args],
                env=environment,
                stdout=subprocess.DEVNULL,
                stderr=trace_file,
                timeout=args.timeout,
                check=False,
            )
            termination = "process-exit"
            return_code = completed.returncode
        except subprocess.TimeoutExpired:
            termination = "wall-clock-timeout"
            return_code = None
    if not screen_path.exists():
        raise SystemExit("worker did not produce a TVRAM screen capture")
    if ((rendered_screen_path is not None) and
            (not rendered_screen_path.exists())):
        raise SystemExit("worker did not produce a rendered screen capture")
    screen = decode_screen(screen_path)
    trace_bytes = trace_path.read_bytes()
    trace_text = trace_bytes.decode("utf-8", errors="replace")
    if run_id not in trace_text:
        raise SystemExit("screen and trace do not carry the same run id")
    result = {
        "termination": termination,
        "return_code": return_code,
        "run_id": run_id,
        "screen_path": str(screen_path),
        "rendered_screen_path": (str(rendered_screen_path)
                                 if rendered_screen_path is not None else None),
        "trace_path": str(trace_path),
        "screen_sha256": screen["sha256"],
        "rendered_screen_sha256": (
            hashlib.sha256(rendered_screen_path.read_bytes()).hexdigest()
            if rendered_screen_path is not None else None),
        "trace_sha256": hashlib.sha256(trace_bytes).hexdigest(),
        "screen": screen,
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if termination == "process-exit" else 124


if __name__ == "__main__":
    sys.exit(main())

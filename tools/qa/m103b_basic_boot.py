#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

"""V2 disk BASIC acceptance (M103b, extended in M103c and M103d).

The run mode boots maintainer-local VA2 ROMs and V2 BASIC media without any
mode override, answers the first prompt, types `PRINT 1+1`, and checks the
88-mode text area in the TVRAM dump written at exit: the answer ` 2` follows
the typed line and an `Ok` prompt is present. Private ROMs and media are
never copied into the tree; the media is copied to a temporary directory.
M103b's original check classified a trace of the interrupt-free key-input
wait loop; M103c delivers interrupts, so the visible screen content is the
criterion instead.

M103d adds a red `CIRCLE` filled green by `PAINT`, checked by counting red
and green pixels in the rendered frame, and a disk round trip: a one-line
program is saved, cleared with `NEW`, loaded and run, and its output must
follow `RUN`. The D88 write-protect flag of the V2 media is cleared in the
temporary copy only.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import struct
import subprocess
import sys
import tempfile
from typing import List, Optional, Tuple

SKIP_RETURN_CODE = 77

# 88-mode text: TVRAM byte 6000h is compatible F000h (BNN manual 8.2.1).
# N88-BASIC programs the text DMA start F3C8h with 120-byte rows (80
# characters and 20 attribute pairs).
TEXT_AREA = 0x6000
TEXT_START = 0x63C8
ROW_PITCH = 120
ROW_CHARS = 80
MAX_ROWS = 25

ERR_NO_DUMP = "M103B_NO_TVRAM_DUMP"
ERR_NO_PROMPT = "M103B_NO_BASIC_PROMPT"
ERR_NO_ANSWER = "M103B_NO_ANSWER"
ERR_WORKER = "M103B_WORKER_FAILED"
ERR_NO_ROUNDTRIP = "M103D_NO_DISK_ROUNDTRIP"
ERR_NO_FRAME = "M103D_NO_SCREENSHOT"
ERR_NO_GRAPHICS = "M103D_NO_GRAPHICS"

PROGRAM_OUTPUT = "M103D"
INPUT_SCRIPT = (
    "@wait 3000\n3\n@wait 1200\nPRINT 1+1\n@wait 900\n"
    "CIRCLE (320,100),60,2:PAINT (320,100),4,2\n@wait 1500\n"
    f'10 PRINT "{PROGRAM_OUTPUT}"\n@wait 300\nSAVE "M103D"\n@wait 1500\n'
    'NEW\n@wait 300\nLOAD "M103D"\n@wait 1500\nRUN\n@wait 900\n'
)

# D88 header byte 1Ah: 10h marks the image write-protected.
D88_WRITE_PROTECT = 0x1A

# The red outline and green fill measured about 600 and 11000 output pixels.
MIN_RED_PIXELS = 200
MIN_GREEN_PIXELS = 5000


def textmem_from_dump(dump: bytes) -> Optional[bytes]:
    if len(dump) < 12 or dump[:8] != b"VAEGSCN1":
        return None
    run_id_length = struct.unpack_from("<I", dump, 12)[0]
    pos = 16 + run_id_length + 6 * 4
    if len(dump) < pos + 4:
        return None
    size = struct.unpack_from("<I", dump, pos)[0]
    data = dump[pos + 4 : pos + 4 + size]
    if len(data) != size or size < TEXT_AREA + 0x1000:
        return None
    return data


def screen_rows(textmem: bytes) -> List[str]:
    rows = []
    for row in range(MAX_ROWS):
        start = TEXT_START + row * ROW_PITCH
        if start + ROW_CHARS > TEXT_AREA + 0x1000:
            break
        chars = textmem[start : start + ROW_CHARS]
        rows.append("".join(chr(c) if 0x20 <= c < 0x7F else " " for c in chars).rstrip())
    return rows


def classify(dump: bytes) -> Optional[str]:
    """Return None when BASIC answered, otherwise one stable error code."""
    textmem = textmem_from_dump(dump)
    if textmem is None:
        return ERR_NO_DUMP
    rows = screen_rows(textmem)
    if not any(row.startswith("Ok") for row in rows):
        return ERR_NO_PROMPT
    if not any(
        row.startswith("PRINT 1+1") and rows[index + 1].strip() == "2"
        for index, row in enumerate(rows[:-1])
    ):
        return ERR_NO_ANSWER
    if not any(
        row.startswith("RUN") and rows[index + 1].strip() == PROGRAM_OUTPUT
        for index, row in enumerate(rows[:-1])
    ):
        return ERR_NO_ROUNDTRIP
    return None


def frame_colour_counts(bmp: bytes) -> Optional[Tuple[int, int]]:
    """Count red and green pixels of an uncompressed 32-bit BMP from SDL."""
    if len(bmp) < 54 or bmp[:2] != b"BM":
        return None
    offset = struct.unpack_from("<I", bmp, 10)[0]
    header_size, width, height, _planes, bpp, compression = struct.unpack_from("<IiiHHI", bmp, 14)
    if bpp != 32 or compression not in (0, 3) or width <= 0 or height == 0:
        return None
    if compression == 3 and header_size >= 52:
        masks = struct.unpack_from("<III", bmp, 14 + 40)
    else:
        masks = (0xFF0000, 0x00FF00, 0x0000FF)
    if len(bmp) < offset + width * abs(height) * 4:
        return None

    def channel(value: int, mask: int) -> int:
        shift = (mask & -mask).bit_length() - 1
        return ((value & mask) >> shift) * 255 // (mask >> shift)

    red = green = 0
    for (value,) in struct.iter_unpack("<I", bmp[offset : offset + width * abs(height) * 4]):
        r, g, b = (channel(value, mask) for mask in masks)
        if r >= 200 and g < 64 and b < 64:
            red += 1
        elif g >= 200 and r < 64 and b < 64:
            green += 1
    return red, green


def classify_frame(bmp: bytes) -> Optional[str]:
    """Return None when the circle and its fill are visible, otherwise an error code."""
    counts = frame_colour_counts(bmp)
    if counts is None:
        return ERR_NO_FRAME
    red, green = counts
    if red < MIN_RED_PIXELS or green < MIN_GREEN_PIXELS:
        return ERR_NO_GRAPHICS
    return None


def _fixture(rows: List[str], magic: bytes = b"VAEGSCN1") -> bytes:
    textmem = bytearray(0x40000)
    for index, text in enumerate(rows):
        start = TEXT_START + index * ROW_PITCH
        textmem[start : start + len(text)] = text.encode("ascii")
    header = magic + struct.pack("<II", 1, 0) + struct.pack("<6I", 0, 0, 20, 0, 0, 0)
    return header + struct.pack("<I", len(textmem)) + bytes(textmem)


PASSING_ROWS = ["", "Ok", "PRINT 1+1", " 2", "Ok", "RUN", PROGRAM_OUTPUT, "Ok"]


def _bmp_fixture(red: int, green: int, magic: bytes = b"BM") -> bytes:
    """A 100x60 32-bit BMP with the given numbers of red and green pixels."""
    width, height = 100, 60
    pixels = [0xFF0000] * red + [0x00FF00] * green
    pixels += [0] * (width * height - len(pixels))
    data = struct.pack(f"<{len(pixels)}I", *pixels)
    info = struct.pack("<IiiHHIIiiII", 40, width, height, 1, 32, 0, len(data), 0, 0, 0, 0)
    header = magic + struct.pack("<IHHI", 14 + len(info) + len(data), 0, 0, 14 + len(info))
    return header + info + data


def selftest() -> int:
    failures = []
    if classify(_fixture(PASSING_ROWS)) is not None:
        failures.append("passing fixture rejected")
    # Each case applies exactly one controlled mutation to the passing fixture.
    cases = [
        ("bad magic", _fixture(PASSING_ROWS, b"VAEGSCN0"), ERR_NO_DUMP),
        ("truncated", _fixture(PASSING_ROWS)[:-0x3A000], ERR_NO_DUMP),
        ("no prompt", _fixture([r.replace("Ok", "OK") for r in PASSING_ROWS]), ERR_NO_PROMPT),
        ("wrong answer", _fixture([r.replace(" 2", " 3") for r in PASSING_ROWS]), ERR_NO_ANSWER),
        ("answer not after the line", _fixture(["Ok", " 2", "PRINT 1+1", "Ok"]), ERR_NO_ANSWER),
        (
            "program output missing",
            _fixture([r.replace(PROGRAM_OUTPUT, "File not found") for r in PASSING_ROWS]),
            ERR_NO_ROUNDTRIP,
        ),
        (
            "output not after RUN",
            _fixture([r.replace("RUN", "LIST") for r in PASSING_ROWS]),
            ERR_NO_ROUNDTRIP,
        ),
    ]
    for name, dump, expected in cases:
        actual = classify(dump)
        if actual != expected:
            failures.append(f"{name}: expected {expected}, got {actual}")
    if classify_frame(_bmp_fixture(MIN_RED_PIXELS, MIN_GREEN_PIXELS)) is not None:
        failures.append("passing frame rejected")
    frame_cases = [
        ("bad frame magic", _bmp_fixture(MIN_RED_PIXELS, MIN_GREEN_PIXELS, b"XX"), ERR_NO_FRAME),
        ("truncated frame", _bmp_fixture(MIN_RED_PIXELS, MIN_GREEN_PIXELS)[:-4], ERR_NO_FRAME),
        ("no outline", _bmp_fixture(MIN_RED_PIXELS - 1, MIN_GREEN_PIXELS), ERR_NO_GRAPHICS),
        ("no fill", _bmp_fixture(MIN_RED_PIXELS, MIN_GREEN_PIXELS - 1), ERR_NO_GRAPHICS),
    ]
    for name, bmp, expected in frame_cases:
        actual = classify_frame(bmp)
        if actual != expected:
            failures.append(f"{name}: expected {expected}, got {actual}")
    for failure in failures:
        print(f"m103b-basic-boot selftest: {failure}", file=sys.stderr)
    if failures:
        return 1
    print("m103b-basic-boot selftest: all classifications passed")
    return 0


def run(worker: str, roms: str, media: str, frames: int, timeout: int) -> int:
    # The worker runs in a temporary directory, so resolve every input first.
    worker = os.path.abspath(worker)
    roms = os.path.abspath(roms)
    media = os.path.abspath(media)
    with tempfile.TemporaryDirectory(prefix="vaeg-m103b-") as work:
        workdir = pathlib.Path(work)
        disk = workdir / "boot.d88"
        shutil.copyfile(media, disk)
        image = bytearray(disk.read_bytes())
        if len(image) > D88_WRITE_PROTECT:
            image[D88_WRITE_PROTECT] = 0
            disk.write_bytes(bytes(image))
        script = workdir / "input.txt"
        script.write_text(INPUT_SCRIPT, encoding="ascii")
        dump_path = workdir / "tvram.bin"
        env = dict(os.environ)
        env.update({"SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"})
        command = [
            worker,
            "--nowait",
            "--no-cfg",
            "--no-bkupmem",
            "--roms",
            roms,
            "--model",
            "VA2",
            "--fdd1",
            str(disk),
            "--headless-input-script",
            str(script),
            "--screen-tvram-dump",
            str(dump_path),
            "--screenshot",
            f"{frames}:{workdir / 'final.bmp'}",
        ]
        completed = subprocess.run(
            command,
            cwd=work,
            env=env,
            capture_output=True,
            text=True,
            errors="replace",
            timeout=timeout,
            check=False,
        )
        if completed.returncode != 0:
            print(f"{ERR_WORKER}: worker exit status {completed.returncode}", file=sys.stderr)
            return 1
        dump = dump_path.read_bytes() if dump_path.exists() else b""
        frame_path = workdir / "final.bmp"
        frame = frame_path.read_bytes() if frame_path.exists() else b""
    error = classify(dump)
    if error is not None:
        print(f"{error}: 88-mode text area does not show the expected answer", file=sys.stderr)
        return 1
    error = classify_frame(frame)
    if error is not None:
        print(f"{error}: rendered frame does not show the painted circle", file=sys.stderr)
        return 1
    print(
        "m103b-basic-boot: disk BASIC answered PRINT 1+1, drew a painted circle, "
        "and ran a program after SAVE, NEW and LOAD"
    )
    return 0


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    sub.add_parser("selftest")
    skip_parser = sub.add_parser("skip")
    skip_parser.add_argument("--reason", required=True)
    run_parser = sub.add_parser("run")
    run_parser.add_argument("--worker", required=True)
    run_parser.add_argument("--roms", required=True)
    run_parser.add_argument("--media", required=True)
    run_parser.add_argument("--frames", type=int, default=14000)
    run_parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args(argv)
    if args.mode == "selftest":
        return selftest()
    if args.mode == "skip":
        print(f"m103b-basic-boot: skipped: {args.reason}")
        return SKIP_RETURN_CODE
    return run(args.worker, args.roms, args.media, args.frames, args.timeout)


if __name__ == "__main__":
    sys.exit(main())

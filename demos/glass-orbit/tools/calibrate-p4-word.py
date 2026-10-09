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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Report the independent packed-4bpp word/nibble calibration fixture."""

import argparse
import json
import struct
from pathlib import Path

WIDTH = 640
PITCH = WIDTH // 2
CALIBRATION_Y = 20


def raw_word(data, word):
    offset = CALIBRATION_Y * PITCH + word * 2
    return int.from_bytes(data[offset:offset + 2], "little")


def bmp_rows(path):
    data = path.read_bytes()
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bpp = struct.unpack_from("<H", data, 28)[0]
    if width != WIDTH or bpp != 32 or height <= 0:
        raise ValueError("calibration screenshot must be a positive 640x422 32bpp BMP")
    pitch = ((width * 4 + 3) // 4) * 4
    rows = []
    for y in range(height):
        source = offset + (height - 1 - y) * pitch
        rows.append([tuple(data[source + x * 4 + channel] for channel in (2, 1, 0))
                     for x in range(width)])
    return rows


def displayed_sequence(screen):
    """Find the row containing the 1..15,1 calibration colour sequence.

    This is intentionally based on the host screenshot's RGB sequence, not
    on the raw decoder used by the GLASS verifier.
    """
    expected_length = 16
    for y, row in enumerate(screen):
        for start in range(0, WIDTH - expected_length + 1):
            sequence = row[start:start + expected_length]
            if all(pixel != (0, 0, 0) for pixel in sequence):
                before = row[start - 1] if start else (0, 0, 0)
                after = row[start + expected_length]
                if before == (0, 0, 0) and after == (0, 0, 0):
                    return y, start, sequence
    raise ValueError("calibration colour sequence was not found in screenshot")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", type=Path, required=True)
    parser.add_argument("--screen", type=Path, required=True)
    args = parser.parse_args()
    data = args.raw.read_bytes()
    screen = bmp_rows(args.screen)
    screen_y, screen_x, sequence = displayed_sequence(screen)
    rows = []
    for x in range(16):
        # The diagnostic fixture uses colour x+1 for x<15 and colour 1 at x=15.
        colour = x + 1 if x < 15 else 1
        word = x // 4
        value = raw_word(data, word)
        shifts = [shift for shift in range(0, 16, 4)
                  if ((value >> shift) & 0x0f) == colour]
        if len(shifts) != 1:
            raise ValueError(f"raw calibration word is ambiguous at x={x}: {value:04x}")
        mask = 0x0f << shifts[0]
        displayed_x = screen_x + x
        rows.append({"x": x, "xmod4": x % 4, "word": word,
                     "raw_word": f"0x{value:04x}",
                     "changed_mask": f"0x{mask:04x}",
                     "displayed_x": displayed_x,
                     "pass": displayed_x == x})
    result = {"schema": "glass-p4-word-calibration-v1",
              "pixels_per_word": 4,
              "screen_y": screen_y,
              "screen_start_x": screen_x,
              "rows": rows,
              "status": "PASS" if screen_x == 0 and all(row["pass"] for row in rows)
              else "FAIL"}
    print(json.dumps(result, indent=2, sort_keys=True))
    print("xmod4 | measured raw mask | displayed logical x | PASS/FAIL")
    for xmod in range(4):
        item = next(row for row in rows if row["xmod4"] == xmod)
        print(f"{xmod:5d} | {item['changed_mask']:18s} | "
              f"{item['displayed_x']:19d} | {'PASS' if item['pass'] else 'FAIL'}")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

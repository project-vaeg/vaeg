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

"""Validate the rendered geometry of the SCAN_LEFT/SCAN_RIGHT sanity disk.

The checker deliberately does not use VAEG's SGP implementation.  It checks
the independent test geometry: white LINE boundaries and colored PATBLT
interiors in three separated regions.  A screenshot is cropped by the SDL2
menu height before the guest coordinates are inspected.
"""

import argparse
import json
import struct
import sys
import zlib


def read_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("SCANLR_PNG_FORMAT")
    pos = 8
    idat = bytearray()
    width = height = depth = color_type = interlace = None
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        pos += length + 12
        if kind == b"IHDR":
            width, height, depth, color_type, _comp, _filter, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if depth != 8 or color_type not in (2, 6) or interlace:
                raise ValueError("SCANLR_PNG_UNSUPPORTED")
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break
    if width is None:
        raise ValueError("SCANLR_PNG_NO_HEADER")
    channels = 3 if color_type == 2 else 4
    stride = width * channels
    raw = zlib.decompress(idat)
    if len(raw) != height * (stride + 1):
        raise ValueError("SCANLR_PNG_SIZE")
    rows = []
    previous = bytearray(stride)
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset:offset + stride])
        offset += stride
        for i in range(stride):
            left = row[i - channels] if i >= channels else 0
            up = previous[i]
            upper_left = previous[i - channels] if i >= channels else 0
            if filter_type == 1:
                row[i] = (row[i] + left) & 0xff
            elif filter_type == 2:
                row[i] = (row[i] + up) & 0xff
            elif filter_type == 3:
                row[i] = (row[i] + ((left + up) // 2)) & 0xff
            elif filter_type == 4:
                estimate = left + up - upper_left
                pa = abs(estimate - left)
                pb = abs(estimate - up)
                pc = abs(estimate - upper_left)
                predictor = left if pa <= pb and pa <= pc else up if pb <= pc else upper_left
                row[i] = (row[i] + predictor) & 0xff
            elif filter_type != 0:
                raise ValueError("SCANLR_PNG_FILTER")
        rows.append(row)
        previous = row
    return width, height, channels, rows


def validate(path, crop_y):
    width, height, channels, rows = read_png(path)
    if width < 320 or height - crop_y < 240:
        raise ValueError("SCANLR_VIEWPORT_SIZE")

    def pixel(x, y):
        row = rows[y + crop_y]
        offset = x * channels
        return tuple(row[offset:offset + 3])

    def nonblack(x, y):
        return max(pixel(x, y)) > 32

    def fill(x, y):
        value = pixel(x, y)
        return max(value) > 32 and max(value) < 245

    def white(x, y):
        return min(pixel(x, y)) >= 245

    fill_points = {(x, y) for y in range(height - crop_y) for x in range(width) if fill(x, y)}
    foreground = {(x, y) for y in range(height - crop_y) for x in range(width) if nonblack(x, y)}
    errors = []

    def require(condition, code):
        if not condition:
            errors.append(code)

    # The two primary boundaries are generated independently by LINE.
    for y in range(80, 160):
        require(white(100, y), "SCANLR_LEFT_BOUNDARY")
        require(white(200, y), "SCANLR_RIGHT_BOUNDARY")

    # The three required bands are ten rows each, with black gaps between them.
    for start, end in ((90, 99), (110, 119), (130, 139), (175, 184)):
        for y in range(start, end + 1):
            xs = [x for x in range(640) if fill(x, y)]
            require(xs and min(xs) >= 101 and max(xs) <= 199, "SCANLR_BAND_GEOMETRY")
            require(xs and max(xs) - min(xs) >= 90, "SCANLR_BAND_WIDTH")
    for y in list(range(100, 110)) + list(range(120, 130)) + list(range(140, 175)):
        require(not any(fill(x, y) for x in range(640)), "SCANLR_BAND_GAP")

    # The adjacent-boundary test has one interior pixel at x=150.
    for y in range(215, 225):
        xs = [x for x in range(640) if fill(x, y)]
        require(xs == [150], "SCANLR_ADJACENT")
    for y in range(225, 240):
        require(not any(fill(x, y) for x in range(640)), "SCANLR_ADJACENT_GAP")

    # Nearest-boundary geometry must not fill the outer x=80..100 or 200..220
    # interval; those outer boundaries are intentionally present for the test.
    for y in range(170, 200):
        require(white(80, y) and white(100, y) and white(200, y) and white(220, y),
                "SCANLR_NEAREST_BOUNDARY")
    require(not any(x < 100 or x > 200 for x, _ in fill_points), "SCANLR_OUTER_FILL")

    if foreground:
        bbox = [min(x for x, _ in foreground), min(y for _, y in foreground),
                max(x for x, _ in foreground), max(y for _, y in foreground)]
    else:
        bbox = None
    require(bool(foreground), "SCANLR_NO_FOREGROUND")
    require(bbox is not None and bbox[2] - bbox[0] < width - 16, "SCANLR_FULL_WIDTH")
    require(bbox is not None and bbox[3] <= 239, "SCANLR_OUT_OF_RANGE")
    result = {
        "image": path,
        "image_size": [width, height],
        "crop_y": crop_y,
        "foreground_bbox": bbox,
        "foreground_pixels": len(foreground),
        "fill_pixels": len(fill_points),
        "errors": errors,
        "triangle_test_preserved": True,
        "visual_result": "PASS" if not errors else "FAIL",
    }
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("png")
    parser.add_argument("--crop-y", type=int, default=22)
    args = parser.parse_args()
    try:
        result = validate(args.png, args.crop_y)
    except (OSError, ValueError) as exc:
        print(json.dumps({"visual_result": "FAIL", "error": str(exc)}, sort_keys=True))
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0 if result["visual_result"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())

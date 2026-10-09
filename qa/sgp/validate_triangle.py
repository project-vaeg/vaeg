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

"""Validate the geometry of a captured line-only triangle.

The emulator screen dump includes a 22-line host menu in the current SDL2
frontend.  The validator crops that strip before measuring the guest graphics
viewport.  It deliberately checks connected geometry and expected edge samples;
foreground-pixel count alone is not an acceptance criterion.
"""

import argparse
import json
import struct
import sys
import zlib


def read_png(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos = 8
    idat = bytearray()
    width = height = depth = color_type = interlace = None
    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, depth, color_type, comp, filt, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if depth != 8 or color_type not in (2, 6) or interlace:
                raise ValueError("only non-interlaced 8-bit RGB/RGBA PNG is supported")
        elif kind == b"IDAT":
            idat.extend(payload)
        elif kind == b"IEND":
            break
    if width is None:
        raise ValueError("PNG has no IHDR")
    channels = 3 if color_type == 2 else 4
    raw = zlib.decompress(idat)
    stride = width * channels
    if len(raw) != height * (stride + 1):
        raise ValueError("unexpected PNG scanline size")
    rows = []
    previous = bytearray(stride)
    p = 0
    for _ in range(height):
        mode = raw[p]
        p += 1
        scan = bytearray(raw[p:p + stride])
        p += stride
        for i in range(stride):
            left = scan[i - channels] if i >= channels else 0
            up = previous[i]
            upper_left = previous[i - channels] if i >= channels else 0
            if mode == 1:
                scan[i] = (scan[i] + left) & 0xff
            elif mode == 2:
                scan[i] = (scan[i] + up) & 0xff
            elif mode == 3:
                scan[i] = (scan[i] + ((left + up) // 2)) & 0xff
            elif mode == 4:
                estimate = left + up - upper_left
                pa = abs(estimate - left)
                pb = abs(estimate - up)
                pc = abs(estimate - upper_left)
                predictor = left if pa <= pb and pa <= pc else up if pb <= pc else upper_left
                scan[i] = (scan[i] + predictor) & 0xff
            elif mode != 0:
                raise ValueError("unsupported PNG filter")
        rows.append(scan)
        previous = scan
    return width, height, channels, rows


def validate(path, crop_y):
    width, height, channels, rows = read_png(path)
    if crop_y < 0 or crop_y >= height:
        raise ValueError("crop-y is outside image")
    view_height = height - crop_y
    foreground = set()
    for y in range(crop_y, height):
        row = rows[y]
        for x in range(width):
            off = x * channels
            if max(row[off:off + 3]) > 32:
                foreground.add((x, y - crop_y))
    if not foreground:
        raise ValueError("TRIANGLE_NO_FOREGROUND")
    box = [min(x for x, _ in foreground), min(y for _, y in foreground),
           max(x for x, _ in foreground), max(y for _, y in foreground)]
    components = []
    remaining = set(foreground)
    while remaining:
        start = remaining.pop()
        todo = [start]
        count = 0
        component_box = [start[0], start[1], start[0], start[1]]
        while todo:
            x, y = todo.pop()
            count += 1
            component_box = [min(component_box[0], x), min(component_box[1], y),
                             max(component_box[2], x), max(component_box[3], y)]
            for nx in (x - 1, x, x + 1):
                for ny in (y - 1, y, y + 1):
                    if (nx, ny) in remaining:
                        remaining.remove((nx, ny))
                        todo.append((nx, ny))
        components.append((count, component_box))
    components.sort(reverse=True)

    def near(x, y, radius=2):
        return any((nx, ny) in foreground for nx in range(x - radius, x + radius + 1)
                   for ny in range(y - radius, y + radius + 1))

    def edge_hits(a, b):
        hits = 0
        for step in range(0, 101, 5):
            t = step / 100.0
            x = round(a[0] + (b[0] - a[0]) * t)
            y = round(a[1] + (b[1] - a[1]) * t)
            hits += near(x, y)
        return hits

    a, b, c = (160, 80), (80, 240), (240, 240)
    endpoints = [near(*point) for point in (a, b, c)]
    edge_hits_result = [edge_hits(*edge) for edge in ((a, b), (b, c), (c, a))]
    bbox_ok = box[0] >= 76 and box[1] >= 76 and box[2] <= 244 and box[3] <= 244
    no_full_width = box[2] - box[0] < width - 16
    result = {
        "image": path,
        "image_size": [width, height],
        "viewport_size": [width, view_height],
        "foreground_bbox": box,
        "foreground_pixels": len(foreground),
        "connected_components": components,
        "endpoints": endpoints,
        "edge_hits": edge_hits_result,
        "triangle_detected": bool(endpoints == [True, True, True]
                                   and all(value >= 15 for value in edge_hits_result)
                                   and len(components) == 1 and bbox_ok and no_full_width),
    }
    result["visual_result"] = "PASS" if result["triangle_detected"] else "FAIL"
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
    return 0 if result["triangle_detected"] else 1


if __name__ == "__main__":
    sys.exit(main())

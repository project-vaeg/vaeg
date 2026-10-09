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

"""Generate deterministic 24x24 ray-traced HSV spheres for the 8-bpp demo."""

from __future__ import annotations

import argparse
import colorsys
import math
from pathlib import Path


WIDTH = 24
HEIGHT = 24
SUPERSAMPLE = 6
SPHERE_RADIUS = 0.92

# The light is above-left in image space and in front of the sphere.
LIGHT = (-0.45, 0.58, 0.68)
VIEW = (0.0, 0.0, 1.0)


def normalize(vector: tuple[float, float, float]) -> tuple[float, float, float]:
    length = math.sqrt(sum(component * component for component in vector))
    return tuple(component / length for component in vector)


def dot(left: tuple[float, float, float], right: tuple[float, float, float]) -> float:
    return sum(a * b for a, b in zip(left, right))


def clamp(value: float, lower: float = 0.0, upper: float = 1.0) -> float:
    return max(lower, min(upper, value))


def rgb332_byte(rgb: tuple[float, float, float]) -> int:
    """Round linear RGB directly into the VA GGGRRRBB byte layout."""

    red = int(clamp(rgb[0]) * 7.0 + 0.5)
    green = int(clamp(rgb[1]) * 7.0 + 0.5)
    blue = int(clamp(rgb[2]) * 3.0 + 0.5)
    value = (green << 5) | (red << 2) | blue
    if value == 0 and any(channel > 0.0 for channel in rgb):
        # Keep a lit edge distinguishable from transparent color zero.
        return 0x24
    return value


def trace_sphere(hue: float) -> list[list[int]]:
    light = normalize(LIGHT)
    halfway = normalize(tuple(light[i] + VIEW[i] for i in range(3)))
    base = colorsys.hsv_to_rgb(hue, 0.78, 0.86)
    bitmap: list[list[int]] = []

    for row in range(HEIGHT):
        output_row: list[int] = []
        for column in range(WIDTH):
            red = green = blue = 0.0
            hits = 0
            for sy in range(SUPERSAMPLE):
                for sx in range(SUPERSAMPLE):
                    x = ((column + (sx + 0.5) / SUPERSAMPLE) / WIDTH) * 2.0 - 1.0
                    y = 1.0 - ((row + (sy + 0.5) / SUPERSAMPLE) / HEIGHT) * 2.0
                    radius_squared = x * x + y * y
                    if radius_squared > SPHERE_RADIUS * SPHERE_RADIUS:
                        continue

                    z = math.sqrt(SPHERE_RADIUS * SPHERE_RADIUS - radius_squared)
                    normal = normalize((x, y, z))
                    diffuse = max(0.0, dot(normal, light))
                    specular = max(0.0, dot(normal, halfway)) ** 42
                    lighting = 0.16 + 0.78 * diffuse
                    highlight = 0.42 * specular
                    red += base[0] * lighting + highlight
                    green += base[1] * lighting + highlight
                    blue += base[2] * lighting + highlight
                    hits += 1

            if hits == 0:
                output_row.append(0)
                continue

            coverage = hits / float(SUPERSAMPLE * SUPERSAMPLE)
            edge_factor = 0.45 + 0.55 * coverage
            output_row.append(
                rgb332_byte(
                    (
                        red / (SUPERSAMPLE * SUPERSAMPLE) * edge_factor,
                        green / (SUPERSAMPLE * SUPERSAMPLE) * edge_factor,
                        blue / (SUPERSAMPLE * SUPERSAMPLE) * edge_factor,
                    )
                )
            )
        bitmap.append(output_row)
    return bitmap


def emit_bitmap(name: str, bitmap: list[list[int]]) -> str:
    lines = [f"{name}:"]
    for row in bitmap:
        values = ", ".join(f"0x{value:02x}" for value in row)
        lines.append(f"    db {values}")
    return "\n".join(lines)


def generate() -> str:
    header = """; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
; 16 deterministic 24x24 GGGRRRBB ray-traced spheres.  Each sample traces an
; orthographic ray against a sphere and combines ambient, diffuse, and
; specular lighting before direct 3:3:2 quantization.  Zero bytes are
; transparent for SGP BITBLT mode 0105h.

"""
    bitmaps = [emit_bitmap(f"orb_hsv_{index:02d}", trace_sphere(index / 16.0)) for index in range(16)]
    return header + "\n\n".join(bitmaps) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(generate(), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

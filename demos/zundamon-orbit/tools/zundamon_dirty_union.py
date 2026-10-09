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

"""Independent M98w row-interval union reference.

The guest stores logical rectangles and rounds each span before sorting.  This
module intentionally uses Python tuples and a separate implementation so the
host oracle does not consume the guest command list as its expected value.
"""

from __future__ import annotations

from dataclasses import dataclass

WIDTH = 320
HEIGHT = 200
PITCH = 320
PAGE_BYTES = WIDTH * HEIGHT
MAX_INTERVALS = 16


class UnionError(ValueError):
    """Stable host-side dirty-union validation failure."""

    def __init__(self, code: str):
        super().__init__(code)
        self.code = code


@dataclass(frozen=True)
class Candidate:
    x0: int
    x1: int
    instance_id: int


def validate_rectangle(rectangle: tuple[int, int, int, int]) -> None:
    x0, y0, width, height = rectangle
    if not (0 <= x0 < 320 and 0 <= y0 < 200 and width > 0 and height > 0):
        raise UnionError("M98W_RECT_BOUNDS")
    if x0 + width > WIDTH or y0 + height > HEIGHT:
        raise UnionError("M98W_RECT_BOUNDS")


def rounded_interval(x0: int, width: int) -> tuple[int, int]:
    if width <= 0 or x0 < 0 or x0 + width > WIDTH:
        raise UnionError("M98W_ROUND_BOUNDS")
    clear_x0 = x0 & ~1
    clear_x1 = (x0 + width + 1) & ~1
    if not (0 <= clear_x0 < clear_x1 <= WIDTH) or (clear_x0 | clear_x1) & 1:
        raise UnionError("M98W_ROUND_ALIGNMENT")
    return clear_x0, clear_x1


def row_union(rectangles: list[tuple[int, int, int, int]], row: int) -> dict[str, object]:
    if len(rectangles) > MAX_INTERVALS:
        raise UnionError("M98W_INTERVAL_CAPACITY")
    candidates: list[Candidate] = []
    for instance_id, rectangle in enumerate(rectangles):
        validate_rectangle(rectangle)
        x0, y0, width, height = rectangle
        if y0 <= row < y0 + height:
            clear_x0, clear_x1 = rounded_interval(x0, width)
            candidates.append(Candidate(clear_x0, clear_x1, instance_id))
    ordered = sorted(candidates, key=lambda item: (item.x0, item.x1,
                                                     item.instance_id))
    merged: list[Candidate] = []
    overlap_merges = adjacency_merges = containment_merges = 0
    for candidate in ordered:
        if not merged or candidate.x0 > merged[-1].x1:
            merged.append(candidate)
            continue
        current = merged[-1]
        if candidate.x0 < current.x1:
            if candidate.x1 < current.x1:
                containment_merges += 1
            else:
                overlap_merges += 1
        else:
            adjacency_merges += 1
        if candidate.x1 > current.x1:
            merged[-1] = Candidate(current.x0, candidate.x1,
                                    current.instance_id)
    return {
        "candidates": candidates,
        "sorted": ordered,
        "merged": merged,
        "overlap_merges": overlap_merges,
        "adjacency_merges": adjacency_merges,
        "containment_merges": containment_merges,
    }


def row_commands(rectangles: list[tuple[int, int, int, int]], row: int,
                 page_base: int) -> list[tuple[int, int]]:
    if not 0 <= row < HEIGHT:
        raise UnionError("M98W_ROW_BOUNDS")
    if not 0 <= page_base <= 0xffffffff - PAGE_BYTES:
        raise UnionError("M98W_PAGE_BOUNDS")
    result = row_union(rectangles, row)["merged"]
    commands = []
    for item in result:
        address = page_base + row * PITCH + item.x0
        words = (item.x1 - item.x0) // 2
        if address < page_base or address + words * 2 > page_base + PAGE_BYTES:
            raise UnionError("M98W_PAGE_BOUNDS")
        commands.append((address, words))
    return commands


def all_rows(rectangles: list[tuple[int, int, int, int]], page_base: int) -> list[tuple[int, int]]:
    commands: list[tuple[int, int]] = []
    for row in range(HEIGHT):
        commands.extend(row_commands(rectangles, row, page_base))
    return commands

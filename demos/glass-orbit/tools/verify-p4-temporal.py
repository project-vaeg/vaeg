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
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Validate the P4 exact span write partition and its temporal invariant.

The guest audit records the logical span and the word ownership partition.  A
record is not a visual screenshot: it is a write-audit witness.  This checker
reconstructs each intermediate write from the recorded partition and rejects
any SGP full-word write that would cover a partial endpoint.
"""

import argparse
import json
import struct
from fractions import Fraction
from pathlib import Path


AUDIT_MAGIC = 0x5034
AUDIT_VERSION = 1
AUDIT_RECORD_SIZE = 16
WIDTH = 640
HEIGHT = 200


def partition(x0, x1):
    """Return the exact inclusive span's disjoint word partition."""
    first_word = x0 // 4
    last_word = x1 // 4
    full_first = first_word + (x0 % 4 != 0)
    full_last = last_word - (x1 % 4 != 3)
    full_count = max(0, full_last - full_first + 1)
    return first_word, last_word, full_first, full_last, full_count


def full_word_set(first, count):
    return set(range(first, first + count))


def simulate_span(x0, x1):
    """Simulate the intended operation order and return temporal errors."""
    expected = set(range(x0, x1 + 1))
    first, last, full_first, _full_last, count = partition(x0, x1)
    full = full_word_set(full_first, count)
    state = set()
    max_overfill = 0
    monotonic = True
    previous = set()
    steps = []

    def record(label):
        nonlocal max_overfill, monotonic, previous
        overfill = state - expected
        max_overfill = max(max_overfill, len(overfill))
        if not previous.issubset(state):
            monotonic = False
        previous = set(state)
        steps.append({
            "operation": label,
            "underfill": len(expected - state),
            "overfill": len(overfill),
        })

    left_partial = first not in full
    right_partial = last not in full
    if left_partial and right_partial and first == last:
        state.update(range(x0, x1 + 1))
        record("same-word-RMW")
        return steps, max_overfill, monotonic

    if left_partial:
        state.update(range(x0, min(x1, (first + 1) * 4 - 1) + 1))
        record("left-RMW")

    if count:
        state.update(
            x
            for word in full
            for x in range(word * 4, word * 4 + 4)
        )
        record("SGP-full")

    if right_partial:
        right_start = max(x0, last * 4)
        state.update(range(right_start, x1 + 1))
        record("right-RMW")

    if not steps:
        record("empty")
    return steps, max_overfill, monotonic


def parse_audit(data, offset):
    if offset < 0 or offset + 12 > len(data):
        raise ValueError("audit header is outside the captured GVRAM")
    magic, version, record_size, count, reserved0, reserved1 = struct.unpack_from(
        "<6H", data, offset
    )
    if magic != AUDIT_MAGIC:
        raise ValueError(f"unexpected audit magic: 0x{magic:04x}")
    if version != AUDIT_VERSION:
        raise ValueError(f"unexpected audit version: {version}")
    if record_size != AUDIT_RECORD_SIZE:
        raise ValueError(f"unexpected audit record size: {record_size}")
    records_offset = offset + 12
    end = records_offset + count * record_size
    if end > len(data):
        raise ValueError("audit records exceed captured GVRAM")
    records = [
        struct.unpack_from("<8H", data, records_offset + index * record_size)
        for index in range(count)
    ]
    return records, {
        "magic": f"0x{magic:04x}",
        "version": version,
        "record_size": record_size,
        "count": count,
        "reserved": [reserved0, reserved1],
        "offset": offset,
    }


def validate_records(records):
    errors = []
    max_overfill = 0
    max_underfill = 0
    monotonic = True
    for index, record in enumerate(records):
        x0, x1, y, colour_face, first, last, full_first, full_count = record
        if x0 > x1 or x1 >= WIDTH or y >= HEIGHT:
            errors.append({"index": index, "code": "P4_AUDIT_INVALID_SPAN", "record": record})
            continue
        expected_partition = partition(x0, x1)
        observed_partition = (first, last, full_first, full_count)
        wanted = (
            expected_partition[0],
            expected_partition[1],
            expected_partition[2],
            expected_partition[4],
        )
        if observed_partition != wanted:
            errors.append({
                "index": index,
                "code": "P4_AUDIT_PARTITION_MISMATCH",
                "expected": wanted,
                "observed": observed_partition,
            })
        full = full_word_set(full_first, full_count)
        if first in full and x0 % 4 != 0:
            errors.append({"index": index, "code": "P4_AUDIT_LEFT_OVERLAP"})
        if last in full and x1 % 4 != 3:
            errors.append({"index": index, "code": "P4_AUDIT_RIGHT_OVERLAP"})
        _steps, overfill, record_monotonic = simulate_span(x0, x1)
        max_overfill = max(max_overfill, overfill)
        if _steps:
            max_underfill = max(max_underfill, max(step["underfill"] for step in _steps))
        monotonic = monotonic and record_monotonic
    return {
        "records": len(records),
        "max_transient_overfill": max_overfill,
        "max_transient_underfill": max_underfill,
        "monotonic_fill": monotonic,
        "errors": errors,
        "status": "PASS" if not errors and max_overfill == 0 and monotonic else "FAIL",
    }


def alignment_matrix():
    failures = []
    cases = 0
    for low_x in range(4):
        for width in (1, 2, 3, 4, 5, 8, 9, 13, 16, 17):
            x0 = 120 + low_x
            x1 = x0 + width - 1
            cases += 1
            _steps, overfill, monotonic = simulate_span(x0, x1)
            if overfill or not monotonic:
                failures.append((x0, x1, overfill, monotonic))
    return {"cases": cases, "failures": failures, "status": "PASS" if not failures else "FAIL"}


def polygon_spans(vertices):
    """Independent Fraction-based pixel-center spans for convex test shapes."""
    min_y = max(0, min(y for _, y in vertices))
    max_y = min(HEIGHT - 1, max(y for _, y in vertices))
    spans = []
    for y in range(min_y, max_y + 1):
        sample = Fraction(2 * y + 1, 2)
        intersections = []
        for (x0, y0), (x1, y1) in zip(vertices, vertices[1:] + vertices[:1]):
            if y0 == y1:
                continue
            low, high = sorted((y0, y1))
            if not (Fraction(low) <= sample < Fraction(high)):
                continue
            x = Fraction(x0) + Fraction(x1 - x0) * (sample - y0) / Fraction(y1 - y0)
            intersections.append(x)
        if len(intersections) < 2:
            continue
        left = (min(intersections) + 1) // 1
        right = max(intersections) // 1
        x0, x1 = int(left), int(right)
        if x0 <= x1:
            spans.append((x0, x1))
    return spans


def slope_matrix():
    cases = {
        "shallow_positive": ((40, 20), (180, 38), (150, 80), (62, 64)),
        "steep_positive": ((100, 12), (132, 180), (220, 132), (188, 24)),
        "shallow_negative": ((180, 20), (40, 38), (62, 64), (150, 80)),
        "steep_negative": ((220, 12), (188, 180), (100, 132), (132, 24)),
        "diamond": ((320, 16), (380, 80), (320, 144), (260, 80)),
        "trapezoid": ((430, 24), (570, 24), (540, 150), (460, 150)),
    }
    reports = {}
    failures = []
    for name, vertices in cases.items():
        spans = polygon_spans(vertices)
        max_overfill = 0
        monotonic = True
        for x0, x1 in spans:
            _steps, overfill, current_monotonic = simulate_span(x0, x1)
            max_overfill = max(max_overfill, overfill)
            monotonic = monotonic and current_monotonic
        reports[name] = {
            "span_count": len(spans),
            "max_transient_overfill": max_overfill,
            "monotonic_fill": monotonic,
            "status": "PASS" if spans and not max_overfill and monotonic else "FAIL",
        }
        if reports[name]["status"] != "PASS":
            failures.append(name)
    return {"cases": reports, "failures": failures, "status": "PASS" if not failures else "FAIL"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("raw_gvram", type=Path)
    parser.add_argument("--audit-offset", type=lambda value: int(value, 0), default=0x2000)
    args = parser.parse_args()
    data = args.raw_gvram.read_bytes()
    records, header = parse_audit(data, args.audit_offset)
    report = {
        "schema": "glass-p4-temporal-v1",
        "audit": header,
        "partition": validate_records(records),
        "alignment_matrix": alignment_matrix(),
        "slope_matrix": slope_matrix(),
    }
    report["status"] = "PASS" if all(
        section["status"] == "PASS"
        for section in (report["partition"], report["alignment_matrix"], report["slope_matrix"])
    ) else "FAIL"
    print(json.dumps(report, sort_keys=True))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

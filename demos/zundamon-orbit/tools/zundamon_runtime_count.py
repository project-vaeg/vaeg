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

"""Independent M98x count parser and bounded publication model.

This module deliberately does not call guest control code.  It reuses the
accepted M98u state generator only for geometry/source identity and models
requested, pending, and visible count state separately.
"""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import re

MAX_COUNT = 16
DEFAULT_COUNT = 4
COUNT_FIELDS = tuple(f"{count:>2}" for count in range(1, MAX_COUNT + 1))
_TOKEN = re.compile(r"[^\t ]+")


class CountOptionError(ValueError):
    """Stable parser failure with a machine-readable M98X code."""

    def __init__(self, code: str):
        super().__init__(code)
        self.code = code


def parse_options(command_line: str) -> tuple[int, int]:
    """Parse one optional /N and /V pair, preserving token boundaries."""
    count = DEFAULT_COUNT
    divisor = 1
    count_seen = False
    divisor_seen = False
    tokens = _TOKEN.findall(command_line)
    for token in tokens:
        upper = token.upper()
        if upper.startswith("/N"):
            if count_seen:
                raise CountOptionError("M98X_DUPLICATE_N")
            if not re.fullmatch(r"/N(?:[1-9]|1[0-6])", upper):
                raise CountOptionError("M98X_INVALID_N")
            count = int(upper[2:])
            count_seen = True
        elif upper.startswith("/V"):
            if divisor_seen:
                raise CountOptionError("M98X_DUPLICATE_V")
            if not re.fullmatch(r"/V[1-8]", upper):
                raise CountOptionError("M98X_INVALID_V")
            divisor = int(upper[2:])
            divisor_seen = True
        else:
            raise CountOptionError("M98X_UNKNOWN_OPTION")
    return count, divisor


def count_field(count: int) -> str:
    if not 1 <= count <= MAX_COUNT:
        raise CountOptionError("M98X_COUNT_RANGE")
    return f"{count:>2}"


@dataclass
class RuntimeCountState:
    """Small reference state machine for request/publication boundaries."""

    requested_count: int = DEFAULT_COUNT
    next_render_count: int = DEFAULT_COUNT
    pending_render_count: int = DEFAULT_COUNT
    visible_published_count: int = DEFAULT_COUNT
    request_generation: int = 0
    pending_generation: int = 0
    published_generation: int = 0
    global_phase_next: int = 0
    frame_in_flight: bool = False
    count_requests_coalesced: int = 0
    count_noop_saturations: int = 0

    def _check(self) -> None:
        values = (self.requested_count, self.next_render_count,
                  self.pending_render_count, self.visible_published_count)
        if any(not 1 <= value <= MAX_COUNT for value in values):
            raise CountOptionError("M98X_COUNT_STATE")
        if not 0 <= self.global_phase_next < 64:
            raise CountOptionError("M98X_PHASE_STATE")

    def press(self, direction: int) -> bool:
        if direction not in (-1, 1):
            raise CountOptionError("M98X_DIRECTION")
        candidate = self.requested_count + direction
        if not 1 <= candidate <= MAX_COUNT:
            self.count_noop_saturations += 1
            return False
        self.requested_count = candidate
        self.request_generation += 1
        if self.frame_in_flight:
            self.count_requests_coalesced += 1
        self._check()
        return True

    def begin_frame(self) -> None:
        if self.frame_in_flight:
            raise CountOptionError("M98X_FRAME_ALREADY_PENDING")
        self.next_render_count = self.requested_count
        self.pending_render_count = self.next_render_count
        self.pending_generation = self.request_generation
        self.frame_in_flight = True
        self._check()

    def publish(self) -> None:
        if not self.frame_in_flight:
            raise CountOptionError("M98X_NO_PENDING_FRAME")
        self.visible_published_count = self.pending_render_count
        self.published_generation = self.pending_generation
        self.global_phase_next = (self.global_phase_next + 1) & 63
        self.frame_in_flight = False
        self._check()


def validate_state_matrix(build_state, counts=range(1, MAX_COUNT + 1),
                          phases=range(64)) -> tuple[int, int]:
    """Validate all count/phase lists through the accepted M98u path."""
    combinations = 0
    records = 0
    for count in counts:
        for phase in phases:
            state = build_state(count, phase)
            if len(state.records) != count or len(state.draw_order) != count:
                raise CountOptionError("M98X_STATE_CARDINALITY")
            if tuple(record.instance_id for record in state.records) != tuple(
                    range(count)):
                raise CountOptionError("M98X_INSTANCE_IDS")
            if sorted(state.draw_order) != list(range(count)):
                raise CountOptionError("M98X_DRAW_PERMUTATION")
            depths = [state.records[index].depth_rank for index in state.draw_order]
            if depths != sorted(depths):
                raise CountOptionError("M98X_DEPTH_ORDER")
            combinations += 1
            records += count
    return combinations, records


def _record_fingerprint(record) -> tuple[int, ...]:
    """Return stable geometry/source fields without pointer or host data."""
    return (record.instance_id, record.phase_offset, record.phase_id,
            record.depth_rank, record.scale_id, record.descriptor_index,
            record.dst_x, record.dst_y, record.dst_x1, record.dst_y1,
            record.bms_bank, record.bank_offset, record.sgp_source,
            record.payload_bytes, record.source_identity)


def transition_rows(build_state):
    """Yield the complete old/new-count transition matrix in fixed order.

    This is a host/reference contract only.  It deliberately regenerates both
    complete states from ``build_state`` and never consumes guest telemetry.
    The old page phase follows the accepted alternating-page convention.
    """
    for old_count in range(1, MAX_COUNT + 1):
        for new_count in range(1, MAX_COUNT + 1):
            for global_phase in range(64):
                old_phase = (global_phase - 2) & 63
                old = build_state(old_count, old_phase)
                new = build_state(new_count, global_phase)
                old_rectangles = tuple(
                    (record.dst_x, record.dst_y, record.width, record.height)
                    for record in old.records)
                new_rectangles = tuple(
                    (record.dst_x, record.dst_y, record.width, record.height)
                    for record in new.records)
                for page in (0, 1):
                    yield {
                        "page": page,
                        "old_count": old_count,
                        "old_phase": old_phase,
                        "new_count": new_count,
                        "new_phase": global_phase,
                        "old_rectangles": old_rectangles,
                        "new_rectangles": new_rectangles,
                        "old_records": tuple(_record_fingerprint(record)
                                             for record in old.records),
                        "new_records": tuple(_record_fingerprint(record)
                                             for record in new.records),
                        "draw_order": new.draw_order,
                    }


def validate_transition_matrix(build_state) -> int:
    """Validate and count all 16*16*64*2 transition cases."""
    total = 0
    for row in transition_rows(build_state):
        if row["page"] not in (0, 1):
            raise CountOptionError("M98X_PAGE_ID")
        if len(row["old_rectangles"]) != row["old_count"]:
            raise CountOptionError("M98X_OLD_FOOTPRINT_COUNT")
        if len(row["new_rectangles"]) != row["new_count"]:
            raise CountOptionError("M98X_NEW_FOOTPRINT_COUNT")
        if len(row["draw_order"]) != row["new_count"]:
            raise CountOptionError("M98X_DRAW_ORDER_COUNT")
        total += 1
    if total != 32768:
        raise CountOptionError("M98X_TRANSITION_MATRIX_COUNT")
    return total


def transition_matrix_digest(build_state) -> str:
    """Hash canonical UTF-8 rows for reproducible QA evidence."""
    digest = hashlib.sha256()
    for row in transition_rows(build_state):
        # Explicit insertion order and compact separators form the golden
        # serialization.  No absolute path, timestamp, or pointer is present.
        encoded = json.dumps(row, separators=(",", ":"), ensure_ascii=True)
        digest.update((encoded + "\n").encode("utf-8"))
    return digest.hexdigest()


def serialize_transition_matrix(build_state) -> bytes:
    """Return the generated-only canonical transition serialization."""
    lines = []
    for row in transition_rows(build_state):
        lines.append(json.dumps(row, separators=(",", ":"),
                                ensure_ascii=True))
    return ("\n".join(lines) + "\n").encode("utf-8")

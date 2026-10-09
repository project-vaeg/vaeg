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

"""Independent bounded M98z orbit/camera control model.

The guest consumes the same immutable snapshot contract, but this module does
not import or execute assembly.  It is intentionally small enough to exercise
all bounds and publication rules in host tests.
"""

from __future__ import annotations

from dataclasses import dataclass

MAX_SPEED_INDEX = 12
DEFAULT_SPEED_INDEX = 3
SPEED_INCREMENTS_Q8 = (64, 128, 192, 256, 320, 384, 512, 768,
                       1024, 1280, 1536, 1792, 2048)
SPEED_LABELS = ("0.25X", "0.50X", "0.75X", "1.00X", "1.25X",
                "1.50X", "2.00X", "3.00X", "4.00X", "5.00X", "6.00X",
                "7.00X", "8.00X")
MIN_DISTANCE = -4
MAX_DISTANCE = 4
MIN_LOOK = -4
MAX_LOOK = 4
MIN_RADIUS_INDEX = 0
MAX_RADIUS_INDEX = 8
RADIUS_FACTORS_Q8 = (128, 160, 192, 224, 256, 288, 320, 352, 384)
RADIUS_LABELS = ("0.50X", "0.63X", "0.75X", "0.88X", "1.00X",
                 "1.13X", "1.25X", "1.38X", "1.50X")
SPEED_KEYS = {"a": 1, "z": -1}
DISTANCE_KEYS = {"q": 1, "e": -1}
LOOK_KEYS = {"w": 1, "s": -1}
RADIUS_KEYS = {"o": 1, "p": -1}


class ControlError(ValueError):
    """Stable host failure code for an invalid M98z state or event."""

    def __init__(self, code: str):
        super().__init__(code)
        self.code = code


def symmetric_round_q8(value: int, factor: int) -> int:
    """Round a signed integer times a Q8.8 factor away from zero symmetrically."""
    if not isinstance(value, int) or not isinstance(factor, int):
        raise ControlError("M98Z_RADIUS_TYPE")
    if factor < 0 or factor > 0xFFFF:
        raise ControlError("M98Z_RADIUS_FACTOR")
    magnitude = abs(value)
    result = (magnitude * factor + 128) // 256
    return -result if value < 0 else result


def clamp(value: int, lower: int, upper: int, code: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool):
        raise ControlError(code)
    return max(lower, min(upper, value))


def effective_scale(base_scale: int, distance_bias: int) -> int:
    if not 1 <= base_scale <= 30:
        raise ControlError("M98Z_SCALE_RANGE")
    if not MIN_DISTANCE <= distance_bias <= MAX_DISTANCE:
        raise ControlError("M98Z_DISTANCE_RANGE")
    return clamp(base_scale + distance_bias, 1, 30, "M98Z_SCALE_RANGE")


@dataclass
class OrbitControlState:
    """Requested and active state separated at a complete-frame boundary."""

    requested_speed_index: int = DEFAULT_SPEED_INDEX
    active_speed_index: int = DEFAULT_SPEED_INDEX
    requested_distance_bias: int = 0
    active_distance_bias: int = 0
    requested_look_level: int = 0
    active_look_level: int = 0
    requested_radius_index: int = 4
    active_radius_index: int = 4
    phase_accumulator: int = 0
    paused: bool = False
    pending: bool = False
    speed_change_requests: int = 0
    speed_changes_applied: int = 0
    distance_change_requests: int = 0
    distance_changes_applied: int = 0
    look_change_requests: int = 0
    look_changes_applied: int = 0
    radius_change_requests: int = 0
    radius_changes_applied: int = 0
    saturated_key_requests: int = 0
    paused_geometry_redraws: int = 0

    def check(self) -> None:
        for value in (self.requested_speed_index, self.active_speed_index):
            if not 0 <= value <= MAX_SPEED_INDEX:
                raise ControlError("M98Z_SPEED_RANGE")
        for value in (self.requested_distance_bias, self.active_distance_bias):
            if not MIN_DISTANCE <= value <= MAX_DISTANCE:
                raise ControlError("M98Z_DISTANCE_RANGE")
        for value in (self.requested_look_level, self.active_look_level):
            if not MIN_LOOK <= value <= MAX_LOOK:
                raise ControlError("M98Z_LOOK_RANGE")
        for value in (self.requested_radius_index, self.active_radius_index):
            if not MIN_RADIUS_INDEX <= value <= MAX_RADIUS_INDEX:
                raise ControlError("M98Z_RADIUS_RANGE")
        if not 0 <= self.phase_accumulator < 64 * 256:
            raise ControlError("M98Z_PHASE_ACCUMULATOR")

    def press(self, key: str) -> bool:
        """Apply one debounced make event to requested state only."""
        if not isinstance(key, str) or len(key) != 1:
            raise ControlError("M98Z_KEY")
        key = key.lower()
        if key in SPEED_KEYS:
            field = "requested_speed_index"
            delta = SPEED_KEYS[key]
            lower, upper = 0, MAX_SPEED_INDEX
            counter = "speed_change_requests"
        elif key in DISTANCE_KEYS:
            field = "requested_distance_bias"
            delta = DISTANCE_KEYS[key]
            lower, upper = MIN_DISTANCE, MAX_DISTANCE
            counter = "distance_change_requests"
        elif key in LOOK_KEYS:
            field = "requested_look_level"
            delta = LOOK_KEYS[key]
            lower, upper = MIN_LOOK, MAX_LOOK
            counter = "look_change_requests"
        elif key in RADIUS_KEYS:
            field = "requested_radius_index"
            delta = RADIUS_KEYS[key]
            lower, upper = MIN_RADIUS_INDEX, MAX_RADIUS_INDEX
            counter = "radius_change_requests"
        else:
            return False
        current = getattr(self, field)
        candidate = current + delta
        if candidate < lower or candidate > upper:
            self.saturated_key_requests += 1
            self.check()
            return False
        setattr(self, field, candidate)
        setattr(self, counter, getattr(self, counter) + 1)
        self.check()
        return True

    def begin_snapshot(self) -> None:
        """Latch all requested controls before generating a new frame."""
        if self.pending:
            raise ControlError("M98Z_SNAPSHOT_PENDING")
        changed_geometry = (
            self.requested_distance_bias != self.active_distance_bias
            or self.requested_look_level != self.active_look_level
            or self.requested_radius_index != self.active_radius_index)
        if self.requested_speed_index != self.active_speed_index:
            self.speed_changes_applied += 1
        if self.requested_distance_bias != self.active_distance_bias:
            self.distance_changes_applied += 1
        if self.requested_look_level != self.active_look_level:
            self.look_changes_applied += 1
        if self.requested_radius_index != self.active_radius_index:
            self.radius_changes_applied += 1
        self.active_speed_index = self.requested_speed_index
        self.active_distance_bias = self.requested_distance_bias
        self.active_look_level = self.requested_look_level
        self.active_radius_index = self.requested_radius_index
        self.pending = True
        if self.paused and changed_geometry:
            self.paused_geometry_redraws += 1
        self.check()

    def publish(self) -> int:
        """Complete a frame and advance phase only when not paused."""
        if not self.pending:
            raise ControlError("M98Z_NO_SNAPSHOT")
        self.pending = False
        if not self.paused:
            self.phase_accumulator = (
                self.phase_accumulator
                + SPEED_INCREMENTS_Q8[self.active_speed_index]) % (64 * 256)
        self.check()
        return self.lookup_phase()

    def lookup_phase(self) -> int:
        self.check()
        return (self.phase_accumulator >> 8) & 63

    def projection(self, dx: int, dy: int, base_scale: int) -> tuple[int, int, int]:
        """Return radius-scaled displacement, camera y bias, and scale ID."""
        self.check()
        factor = RADIUS_FACTORS_Q8[self.active_radius_index]
        return (symmetric_round_q8(dx, factor),
                symmetric_round_q8(dy, factor) + self.active_look_level * 4,
                effective_scale(base_scale, self.active_distance_bias))


def format_status(subject: str, divisor: int, count: int,
                  state: OrbitControlState) -> tuple[str, str, str]:
    """Return deterministic fixed-width status lines for the G0 panel."""
    if subject not in ("ZUNDAMON", "IDA"):
        raise ControlError("M98Z_SUBJECT")
    if not 1 <= divisor <= 8 or not 1 <= count <= 16:
        raise ControlError("M98Z_STATUS_RANGE")
    state.check()
    fps = ("60 ", "30 ", "20 ", "15 ", "12 ", "10 ", "8.6", "7.5")[divisor - 1]
    name = "ZUNDAMON" if subject == "ZUNDAMON" else "IDA CNT"
    count_line = f"{name}: {count:>2}" if subject == "ZUNDAMON" else f"{name}: {count:>2}"
    return (f"FPS: {fps} SPD:{SPEED_LABELS[state.active_speed_index]} ",
            count_line,
            f"DIST:{state.active_distance_bias:+d} LOOK:{state.active_look_level:+d} RAD:{RADIUS_LABELS[state.active_radius_index]}")

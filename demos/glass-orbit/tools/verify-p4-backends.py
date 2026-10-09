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

"""Compare GLASS ORBIT P4-1 CPU and P4-2 SGP fixed-frame captures.

The comparator has no baseline-update option.  It requires equal complete
VAEG GVRAM snapshots and equal composed screens from independently launched
CPU and SGP guest payloads.  This validates VAEG functional equivalence only;
it is not a real-PC-88VA timing or conformance oracle.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


PAYLOAD_SEGMENT = "3000"
RAW_CHECKSUM = "7ace"
GVRAM_SIZE = 0x40000
CPU_PREFIX = "glass-p4-cpu"
SGP_PREFIX = "glass-p4-sgp"


def read_tsv(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("\t")
        if not separator or key in values:
            raise ValueError("P4_BACKEND_REGISTERS_SCHEMA")
        values[key] = value
    return values


def check_registers(directory: Path, prefix: str, marker: str, ip: str,
                    errors: list[str]) -> tuple[dict[str, str], bytes | None, bytes | None]:
    registers: dict[str, str] = {}
    raw: bytes | None = None
    screen: bytes | None = None
    try:
        registers = read_tsv(directory / f"{prefix}.registers.tsv")
    except (OSError, UnicodeError, ValueError):
        errors.append(marker + "_REGISTERS")
        return registers, raw, screen
    required = {
        "schema": "vaeg-registers-v1",
        "ax": "4750" if prefix == CPU_PREFIX else "4753",
        "bx": RAW_CHECKSUM,
        "cs": PAYLOAD_SEGMENT,
        "ds": PAYLOAD_SEGMENT,
        "es": PAYLOAD_SEGMENT,
        "ss": PAYLOAD_SEGMENT,
        "sp": "f000",
        "ip": ip,
    }
    for key, expected in required.items():
        if registers.get(key) != expected:
            errors.append(marker + "_" + key.upper())
    try:
        flags = int(registers.get("flags", ""), 16)
    except ValueError:
        errors.append(marker + "_FLAGS")
    else:
        if flags & 0x0400:
            errors.append(marker + "_DIRECTION_FLAG")
        if not flags & 0x0200:
            errors.append(marker + "_INTERRUPTS")
    try:
        events = (directory / "events.tsv").read_text(encoding="utf-8")
    except (OSError, UnicodeError):
        errors.append(marker + "_EVENTS")
    else:
        if "pc\t" not in events:
            errors.append(marker + "_IDLE")
    try:
        raw = (directory / f"{prefix}.gvram.bin").read_bytes()
    except OSError:
        errors.append(marker + "_GVRAM")
    else:
        if len(raw) != GVRAM_SIZE:
            errors.append(marker + "_GVRAM_SIZE")
    try:
        screen = (directory / f"{prefix}.screen.bmp").read_bytes()
    except OSError:
        errors.append(marker + "_SCREEN")
    return registers, raw, screen


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("cpu", type=Path)
    parser.add_argument("sgp", type=Path)
    parser.add_argument("sgp_repeat", type=Path)
    args = parser.parse_args()
    errors: list[str] = []
    cpu_registers, cpu_raw, cpu_screen = check_registers(
        args.cpu, CPU_PREFIX, "P4_CPU", "0200", errors)
    sgp_registers, sgp_raw, sgp_screen = check_registers(
        args.sgp, SGP_PREFIX, "P4_SGP", "0280", errors)
    repeat_registers, repeat_raw, repeat_screen = check_registers(
        args.sgp_repeat, SGP_PREFIX, "P4_SGP_REPEAT", "0280", errors)
    if cpu_raw is None or sgp_raw is None or cpu_raw != sgp_raw:
        errors.append("P4_CPU_SGP_GVRAM_MISMATCH")
    if cpu_screen is None or sgp_screen is None or cpu_screen != sgp_screen:
        errors.append("P4_CPU_SGP_SCREEN_MISMATCH")
    if sgp_raw is None or repeat_raw is None or sgp_raw != repeat_raw:
        errors.append("P4_SGP_GVRAM_REPEAT_MISMATCH")
    if sgp_screen is None or repeat_screen is None or sgp_screen != repeat_screen:
        errors.append("P4_SGP_SCREEN_REPEAT_MISMATCH")
    if sgp_registers.get("bx") != repeat_registers.get("bx"):
        errors.append("P4_SGP_CHECKSUM_REPEAT_MISMATCH")
    result = {
        "cpu_checksum": cpu_registers.get("bx"),
        "errors": errors,
        "gvrams_equal": cpu_raw is not None and cpu_raw == sgp_raw,
        "schema": "glass-orbit-p4-backends-v1",
        "screens_equal": cpu_screen is not None and cpu_screen == sgp_screen,
        "sgp_checksum": sgp_registers.get("bx"),
        "status": "PASS" if not errors else "FAIL",
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

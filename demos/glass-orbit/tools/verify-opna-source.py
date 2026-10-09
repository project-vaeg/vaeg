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

"""Validate the GLASS ORBIT VA OPNA-only source contract.

This is a source/data check.  It proves that the payload contains the three
original GLASS score channels and the required lifecycle calls; it does not
prove audible output on physical hardware.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


FORBIDDEN = ("opl", "ymf262", "ym3812", "opl2", "opl3")
CHANNELS = 3
STEPS = 256


def parse_db_values(text: str) -> list[int]:
    values: list[int] = []
    for line in text.splitlines():
        source = line.split(";", 1)[0]
        if "db" not in source.lower():
            continue
        source = source.split("db", 1)[1]
        for token in source.split(","):
            token = token.strip()
            if not token:
                continue
            match = re.fullmatch(r"0?([0-9A-Fa-f]{1,2})h", token)
            if match is None:
                raise ValueError(f"invalid score byte: {token}")
            values.append(int(match.group(1), 16))
    return values


def check(root: Path) -> dict[str, object]:
    source_dir = root / "src"
    opna = (source_dir / "glass_opna.inc").read_text(encoding="utf-8")
    data = (source_dir / "glass_opna_data.inc").read_text(encoding="utf-8")
    wrapper = (source_dir / "glass_orbit.asm").read_text(encoding="utf-8")
    scene = (source_dir / "glass_scene.inc").read_text(encoding="utf-8")
    errors: list[str] = []

    for path, text in ((source_dir / "glass_opna.inc", opna),
                       (source_dir / "glass_opna_data.inc", data),
                       (source_dir / "glass_orbit.asm", wrapper)):
        lower = text.lower()
        if any(token in lower for token in FORBIDDEN):
            errors.append(f"forbidden non-OPNA token in {path.name}")

    required = (
        "glass_opna_detect",
        "glass_opna_init",
        "glass_opna_tick",
        "glass_opna_shutdown",
        "glass_opna_ssg_note_on",
        "glass_opna_ssg_key_off",
        "GLASS_OPNA_PORT_LOW_ADDR",
        "GLASS_OPNA_PORT_HIGH_ADDR",
    )
    for token in required:
        if token not in opna:
            errors.append(f"missing OPNA contract token: {token}")
    if "call    glass_opna_init" not in scene:
        errors.append("P5 does not initialize OPNA")
    if "call    glass_opna_tick" not in scene:
        errors.append("P5 does not service OPNA music")
    if scene.count("call    glass_opna_shutdown") < 2:
        errors.append("P5 must silence OPNA on exit and failure")

    channels: list[list[int]] = []
    for channel in range(CHANNELS):
        marker = f"glass_opna_music_ch{channel}:"
        start = data.find(marker)
        if start < 0:
            errors.append(f"missing score channel {channel}")
            continue
        end = len(data)
        if channel + 1 < CHANNELS:
            end = data.find(f"glass_opna_music_ch{channel + 1}:", start)
        channels.append(parse_db_values(data[start:end]))
    if len(channels) == CHANNELS:
        for channel, values in enumerate(channels):
            if len(values) != STEPS:
                errors.append(f"channel {channel} has {len(values)} steps, expected {STEPS}")
            if any(value not in range(0x00, 0x100) for value in values):
                errors.append(f"channel {channel} contains an invalid byte")
    report = {
        "status": "PASS" if not errors else "FAIL",
        "backend": "OPNA/YM2608 only",
        "ports": {"low": "0044h/0045h", "high": "0046h/0047h"},
        "channels": len(channels),
        "steps_per_channel": [len(channel) for channel in channels],
        "errors": errors,
    }
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path, nargs="?", default=Path(__file__).parents[1])
    args = parser.parse_args()
    report = check(args.root)
    print(json.dumps(report, sort_keys=True))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

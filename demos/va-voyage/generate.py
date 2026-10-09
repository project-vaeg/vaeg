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
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Original Q7 waves/score; independently callable content validation.

The integer projection model mirrors the payload for focused arithmetic checks,
not an independent proof of SGP, BIOS, hardware sound or guest execution.
"""
import argparse
import json
import math
from pathlib import Path

# Prototype calibration for the default VA2/NP2 worker, not a universal clock.
PERIOD = 900
ROWS, COLS = 12, 9
# Prefix: SET WORK, SET COLOR, CLS; worst colour changes, all LINEs, END.
MAX_LINES = ROWS * (COLS - 1) + (ROWS - 1) * COLS + 14 + 9
MAX_WORDS = 3 + 2 + 5 + 2 * (ROWS + 2) + 8 * MAX_LINES + 1
FNUM = (309, 327, 347, 367, 389, 412, 437, 463, 490, 519, 550, 583)


class ContentError(ValueError):
    def __init__(self, code):
        self.code = code
        super().__init__(code)


def note(semitone, octave):
    octave += semitone // 12
    return (octave << 11) | FNUM[semitone % 12]


def make_data():
    sine = [round(127 * math.sin(2 * math.pi * i / 256)) for i in range(256)]
    bass, lead, pad = [], [], []
    roots = (0, 8, 5, 7, 0, 8, 5, 7)
    pattern = (0, 1, 2, 3, 2, 1, 0, 2, 3, 2, 1, 0, 1, 2, 3, 2)
    for step in range(128):
        root = roots[step // 16]
        chord = (0, 4, 7, 10) if root in (8, 7) else (0, 3, 7, 10)
        bass.append(note(root, 2) if step % 2 == 0 else 0xffff)
        lead.append(note(root + chord[pattern[step % 16]], 4))
        pad.append(note(root + 7, 3))
    return {"version": 1, "period": PERIOD, "sine": sine,
            "bass": bass, "lead": lead, "pad": pad}


def validate(data):
    """Content only; no repository, protected-history or commit dependencies."""
    if data.get("version") != 1 or data.get("period") != PERIOD:
        raise ContentError("VOYAGE_PERIOD")
    sine = data.get("sine", [])
    if len(sine) != 256:
        raise ContentError("VOYAGE_SINE_COUNT")
    if any(type(n) is not int or not -127 <= n <= 127 for n in sine):
        raise ContentError("VOYAGE_SINE_RANGE")
    if any(sine[(i + 128) % 256] != -sine[i] for i in range(256)):
        raise ContentError("VOYAGE_SINE_SYMMETRY")
    for key in ("bass", "lead", "pad"):
        score = data.get(key, [])
        if len(score) != 128:
            raise ContentError("VOYAGE_SCORE_COUNT")
        for value in score:
            if key == "bass" and value == 0xffff:
                continue
            if (type(value) is not int or not 0 <= value <= 0x3fff or
                    (value & 0x7ff) not in FNUM):
                raise ContentError("VOYAGE_NOTE_RANGE")
    if MAX_WORDS > 4096:
        raise ContentError("VOYAGE_COMMAND_CAPACITY")


def project(tick, data):
    phase = (tick % PERIOD) * 256 // PERIOD
    sine = data["sine"]
    bank, heave = sine[phase] // 8, sine[(phase * 2) % 256] // 16
    forward = phase * 8
    points = []
    for row in range(ROWS):
        depth = (50 - row * 4) * 256 - (forward & 255) * 4
        scale = min(448, 819200 // depth)
        for col in range(COLS):
            x = (col * 10 - 40) * scale // 128
            wave_phase = ((row - (forward >> 8)) * 32 + phase * 5 + col * 13) % 256
            y = 58 + scale // 4 + heave + (sine[wave_phase] // 4) * scale // 1024
            final_y = max(0, min(199, y + x * bank // 128))
            final_x = max(0, min(319, 160 + x + (y - 58) * bank // 64))
            points.append((final_x, final_y))
    return points


def wait_sound(statuses, limit=0x4000):
    """Pure model of the payload's closed low-bank status-polling contract."""
    for count, status in enumerate(statuses):
        if count >= limit:
            break
        if status == 0xff:
            return "VOYAGE_SOUND_ABSENT"
        if not status & 0x80:
            return "VOYAGE_SOUND_READY"
    return "VOYAGE_SOUND_BUSY"


def write_include(path, data):
    # Generated only out of tree; retain the source's full BSD notice.
    source = Path(__file__).read_text(encoding="utf-8").splitlines()
    notice = [line.replace("#", ";", 1) for line in source[1:21]]
    lines = notice + ["", "; Source-built original data; do not hand edit."]
    for label, key in (("sine_table", "sine"), ("bass_score", "bass"),
                       ("lead_score", "lead"), ("pad_score", "pad")):
        lines.append(f"{label}:")
        values = data[key]
        for start in range(0, len(values), 16):
            lines.append("    dw " + ",".join(str(n) for n in values[start:start + 16]))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    repository = Path(__file__).resolve().parents[2]
    if output == repository or repository in output.parents:
        raise ContentError("VOYAGE_OUTPUT_IN_TREE")
    data = make_data()
    validate(data)  # validate before output creation/placement
    for tick in range(PERIOD):
        points = project(tick, data)
        if len(points) != ROWS * COLS or any(not (0 <= x < 320 and 0 <= y < 200)
                                            for x, y in points):
            raise ContentError("VOYAGE_COORDINATES")
    output.mkdir(parents=True, exist_ok=True)
    write_include(output / "voyage_tables.inc", data)
    (output / "voyage_data.json").write_text(
        json.dumps(data, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(f"VOYAGE_DATA_OK period={PERIOD} poses={PERIOD} "
          f"line_max={MAX_LINES} words_max={MAX_WORDS}/4096")


if __name__ == "__main__":
    main()

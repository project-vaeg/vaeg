#!/usr/bin/env python3
"""Validate the visible VA text plane without depending on a host renderer.

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

from __future__ import annotations

import argparse
from pathlib import Path


MAIN_BYTES = 80 * 25 * 2
SYSTEM_BYTES = 80 * 2 * 2
FORBIDDEN = (b"DIR A:", b"DIR B:", b"COPY", b"TIME")


def byte_lanes(text: bytes) -> tuple[bytes, bytes]:
    """Return both possible byte lanes used by captured TVRAM words."""

    return (
        bytes(text[index] for index in range(0, len(text), 2)),
        bytes(text[index] for index in range(1, len(text), 2)),
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("tvram", type=Path)
    args = parser.parse_args()

    raw = args.tvram.read_bytes()
    if len(raw) < MAIN_BYTES + SYSTEM_BYTES:
        raise SystemExit(f"FAIL: TVRAM dump is too short: {len(raw)} bytes")

    visible = raw[:MAIN_BYTES]
    lanes = byte_lanes(visible)
    forbidden = [
        token.decode("ascii")
        for token in FORBIDDEN
        if any(token.lower() in lane.lower() for lane in lanes)
    ]
    nonblank = sum(byte not in (0, 0x20) for lane in lanes for byte in lane)

    print(f"VISIBLE_MAIN_BYTES={MAIN_BYTES}")
    print(f"VISIBLE_NONBLANK_BYTES={nonblank}")
    print(f"FORBIDDEN_VISIBLE={' '.join(forbidden) if forbidden else 'NONE'}")
    print("TEXT_VISIBLE=" + ("PASS" if nonblank else "FAIL"))
    print("BOTTOM_GUIDE=" + ("FAIL" if forbidden else "PASS"))

    if forbidden or not nonblank:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def fail(code: str, detail: str) -> None:
    raise SystemExit(f"{code}: {detail}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a generated SQEMM98.SYS")
    parser.add_argument("driver", type=Path)
    args = parser.parse_args()
    try:
        data = args.driver.read_bytes()
    except OSError as error:
        fail("SQEMM98_CHECK_READ", str(error))

    if not 8192 <= len(data) <= 65535:
        fail("SQEMM98_CHECK_SIZE", f"unexpected size {len(data)}")
    if data[10:18] != b"EMMXXXX0":
        fail("SQEMM98_CHECK_DEVICE_NAME", "character-device name is missing")
    if data[6:10] != b"\x14\x00\x1f\x00":
        fail("SQEMM98_CHECK_DRIVER_ENTRY", "unexpected strategy/interrupt entry")
    if data[0x1F:0x22] != b"\x9c\x53\x1e":
        fail("SQEMM98_CHECK_CALLER_STATE", "interrupt entry does not save FLAGS/BX/DS")
    init_return = (
        b"\x1f\x5b\x1e\x06\x50\x51\x52\x53\x56\x57\x55"
        b"\xe8\x08\x2f\x5d\x5f\x5e\x5b\x5a\x59\x58\x07\x1f\x9d\xcb"
    )
    if init_return not in data[:0x80]:
        fail("SQEMM98_CHECK_CALLER_STATE", "init return does not restore registers")
    if b"\x8e\x1e\x57\x02\x33\xff\x33\xdb\x8b\xd3\x33\xc0" not in data:
        fail("SQEMM98_CHECK_MEMORY_TEST_OFFSET", "memory test does not clear DI")
    if data.count(b"\xcd\x83") != 1:
        fail("SQEMM98_CHECK_PCENGINE_OUTPUT", "expected one INT 83h instruction")
    if b"\xcd\x10" in data:
        fail("SQEMM98_CHECK_IBM_VIDEO", "INT 10h instruction remains")
    if b"\xb4\x09\xcd\x21" in data:
        fail("SQEMM98_CHECK_DOS_OUTPUT", "DOS AH=09h output remains")
    if data.count(b"\xcd\x21") != 2:
        fail("SQEMM98_CHECK_DOS_VECTOR", "unexpected INT 21h instruction count")
    for marker in (
        b"SQEMM98 MAX v0.8 for PC-88VA\x0d\x0a\x00",
        b"PC-88VA EMS board was not detected.",
        b"SQEMM98 successfully initialized.",
    ):
        if marker not in data:
            fail("SQEMM98_CHECK_MESSAGE", f"missing marker {marker!r}")
    if b"\xba\xe9\x08" not in data:
        fail("SQEMM98_CHECK_TARGET_PORT", "08E9h target-port load is missing")
    if b"\x81\xc2\xe1\x08" not in data:
        fail("SQEMM98_CHECK_PAGE_PORT", "08E1h page-port base is missing")

    print(f"SQEMM98_CHECK_OK bytes={len(data)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

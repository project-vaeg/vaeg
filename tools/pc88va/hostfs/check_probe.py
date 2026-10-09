#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import hashlib
from pathlib import Path


def fail(message):
    raise SystemExit(f"R2FPROBE.COM check failed: {message}")


def main():
    parser = argparse.ArgumentParser(
        description="Validate the non-resident PC-Engine redirector probe"
    )
    parser.add_argument("--input", required=True)
    args = parser.parse_args()

    data = Path(args.input).read_bytes()
    if not 512 <= len(data) <= 4096:
        fail(f"unexpected flat-binary size {len(data)}")
    for required in (
        b"R2FPROBE 1",
        b"INT21/5F02",
        b"INT21/5F03",
        b"RESULT=NO_DOS_REDIRECTOR_BRIDGE",
        b"RESULT=DOS_REDIRECTOR_BRIDGE_PRESENT",
        b"RESULT=PROBE_HOOK_FAILED",
        b"F:\x00",
        b"\\\\HOSTFS\\ROOT\x00\x00",
    ):
        if data.count(required) != 1:
            fail(f"required marker count is not one: {required!r}")
    if data.count(b"\xcd\x2f") != 2:
        fail("pre-hook and post-hook INT 2F instruction count is not two")
    if data.count(b"\xcd\x83") != 2:
        fail("PC-Engine INT 83 console-output call count is not two")
    if data.count(b"\xcd\x21") < 8:
        fail("expected DOS calls are missing")
    if b"\xb8\x02\x5f\xcd\x21" not in data:
        fail("INT 21/AX=5F02 probe call is missing")
    if b"\xb8\x03\x5f\xcd\x21" not in data:
        fail("INT 21/AX=5F03 probe call is missing")
    digest = hashlib.sha256(data).hexdigest()
    print(f"R2FPROBE.COM ok: {len(data)} bytes sha256={digest}")


if __name__ == "__main__":
    main()

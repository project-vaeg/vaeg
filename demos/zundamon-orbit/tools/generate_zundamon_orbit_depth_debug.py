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

"""Generate one deterministic bounded M98t VAEG debug script."""

from __future__ import annotations

import argparse
from pathlib import Path


def build_script(initial_page: str, divisor: int, revolutions: int,
                 scenario: str = "static") -> str:
    prefix = f"m98t-{scenario}-v{divisor}-{initial_page}-r{revolutions}"
    publications = 64 * revolutions
    lines = [
        "debug-script 1", "limit-frame 30000", "wait-frame 1200",
        f"input-line ZUNDORB /V{divisor}",
        "wait-pc 3000:4000 1", f"capture {prefix}-probe registers",
        "wait-pc 3000:4010 1", f"capture {prefix}-load registers",
        "wait-pc 3000:4020 1", f"capture {prefix}-initialize registers gvram",
    ]
    for index in range(1, publications + 1):
        lines.extend(("wait-pc 3000:4030 1",
                      f"capture {prefix}-flip-{index:03d} registers gvram"))
    lines.extend((
        "wait-pc 3000:4040 1",
        f"capture {prefix}-settled-a registers gvram screen",
        "wait-pc 3000:4040 1",
        f"capture {prefix}-settled-b registers gvram screen",
    ))
    for letter, address in zip("abcdefghijklmno", range(0x4050, 0x4140, 0x10)):
        lines.extend((f"wait-pc 3000:{address:04x} 1",
                      f"capture {prefix}-report-{letter} registers"))
    lines.append("exit")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--divisor", choices=range(1, 9), type=int, required=True)
    parser.add_argument("--revolutions", choices=(1, 2), type=int, default=1)
    parser.add_argument("--scenario", choices=("static", "ladder", "pause", "missed"),
                        default="static")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("refusing to overwrite the output debug script")
    args.output.write_text(build_script(args.initial_page, args.divisor,
                                        args.revolutions, args.scenario),
                           encoding="utf-8")
    print(f"M98T_DEBUG_SCRIPT_PASS divisor={args.divisor} "
          f"initial_page={args.initial_page} revolutions={args.revolutions} "
          f"scenario={args.scenario} output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

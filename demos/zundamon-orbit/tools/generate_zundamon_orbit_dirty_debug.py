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

"""Generate the deterministic two-cycle M98q VAEG debug script."""

from __future__ import annotations

import argparse
from pathlib import Path

CYCLES = 2
PUBLICATIONS_PER_CYCLE = 58


def build_script(initial_page: str, clear_mode: str, cycles: int = CYCLES,
                 interactive_exit: bool = False) -> str:
    prefix = f"m98q-{initial_page}-{clear_mode}"
    lines = [
        "debug-script 1",
        "limit-frame 18000",
        "wait-frame 1200",
        "input-line ZUNDORB",
        "wait-pc 3000:4000 1",
        f"capture {prefix}-probe registers",
        "wait-pc 3000:4010 1",
        f"capture {prefix}-load registers",
        "wait-pc 3000:4020 1",
        f"capture {prefix}-initialize registers",
    ]
    for index in range(1, cycles * PUBLICATIONS_PER_CYCLE + 1):
        lines.extend((
            "wait-pc 3000:4030 1",
            f"capture {prefix}-flip-{index:03d} registers gvram",
        ))
    if interactive_exit:
        lines.append("enter")
    else:
        lines.extend((
            "wait-pc 3000:4040 1",
            f"capture {prefix}-settled-a registers gvram screen",
            "wait-pc 3000:4040 1",
            f"capture {prefix}-settled-b registers gvram screen",
        ))
    for letter, address in zip("abcdefgh", range(0x4050, 0x40D0, 0x10)):
        lines.extend((
            f"wait-pc 3000:{address:04x} 1",
            f"capture {prefix}-report-{letter} registers",
        ))
    lines.append("exit")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--clear-mode", choices=("full", "dirty"), required=True)
    parser.add_argument("--cycles", choices=(2, 3), default=CYCLES, type=int)
    parser.add_argument("--interactive-exit", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("refusing to overwrite the output debug script")
    args.output.write_text(
        build_script(args.initial_page, args.clear_mode, args.cycles,
                     args.interactive_exit), encoding="utf-8")
    print(f"M98Q_DEBUG_SCRIPT_PASS initial_page={args.initial_page} "
          f"clear_mode={args.clear_mode} cycles={args.cycles} "
          f"interactive_exit={int(args.interactive_exit)} output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

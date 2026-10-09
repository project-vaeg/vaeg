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

"""Generate the deterministic M98p one-cycle VAEG debug script."""

from __future__ import annotations

import argparse
from pathlib import Path


def build_script(initial_page: str, cycles: int = 1,
                 interactive_exit: bool = False) -> str:
    prefix = f"m98p-{initial_page}"
    lines = [
        "debug-script 1",
        "limit-frame 12000",
        "wait-frame 1200",
        "input-line ZUNDORB",
        "wait-pc 3000:4000 1",
        f"capture {prefix}-probe registers",
        "wait-pc 3000:4010 1",
        f"capture {prefix}-load registers",
        "wait-pc 3000:4020 1",
        f"capture {prefix}-initialize registers",
    ]
    for index in range(1, cycles * 58 + 1):
        lines.extend((
            "wait-pc 3000:4030 1",
            f"capture {prefix}-flip-{index:02d} registers gvram",
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
    lines.extend((
        "wait-pc 3000:4050 1",
        f"capture {prefix}-report-a registers",
        "wait-pc 3000:4060 1",
        f"capture {prefix}-report-b registers",
        "wait-pc 3000:4070 1",
        f"capture {prefix}-report-c registers",
        "wait-pc 3000:4080 1",
        f"capture {prefix}-report-d registers",
        "wait-pc 3000:4090 1",
        f"capture {prefix}-report-e registers",
        "exit",
    ))
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--cycles", choices=(1, 2), default=1, type=int)
    parser.add_argument("--interactive-exit", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("refusing to overwrite the output debug script")
    args.output.write_text(build_script(
        args.initial_page, args.cycles, args.interactive_exit), encoding="utf-8")
    print(f"M98P_DEBUG_SCRIPT_PASS initial_page={args.initial_page} "
          f"cycles={args.cycles} interactive_exit={int(args.interactive_exit)} "
          f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

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

"""Generate one bounded M98v/M98w VAEG capture script."""

from __future__ import annotations

import argparse
from pathlib import Path


def build_script(active_count: int, initial_page: str, divisor: int,
                 revolutions: int, scenario: str = "static",
                 milestone: str = "m98v", runtime_count: int | None = None) -> str:
    if runtime_count is not None and not 1 <= runtime_count <= 16:
        raise ValueError("M98X_RUNTIME_COUNT_RANGE")
    prefix = (f"{milestone}-n{active_count}-{scenario}-v{divisor}-"
              f"{initial_page}-r{revolutions}")
    publications = 64 * revolutions
    launch_options = f"/V{divisor}"
    if runtime_count is not None:
        launch_options = f"/N{runtime_count} {launch_options}"
    lines = [
        "debug-script 1", "limit-frame 60000", "wait-frame 1200",
        f"input-line ZUNDORB {launch_options}",
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
    report_letters = ("abcdefghijklmnopqrs" if milestone == "m98w"
                      else "abcdefghijklmno")
    report_end = 0x4180 if milestone == "m98w" else 0x4140
    for letter, address in zip(report_letters, range(0x4050, report_end, 0x10)):
        lines.extend((f"wait-pc 3000:{address:04x} 1",
                      f"capture {prefix}-report-{letter} registers"))
    lines.append("exit")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--active-count", choices=(1, 2, 4, 8, 16),
                        type=int, required=True)
    parser.add_argument("--initial-page", choices=("a", "b"), required=True)
    parser.add_argument("--divisor", choices=range(1, 9), type=int, required=True)
    parser.add_argument("--revolutions", choices=(1, 2), type=int, default=1)
    parser.add_argument("--scenario", choices=("static", "ladder", "pause", "missed"),
                        default="static")
    parser.add_argument("--milestone", choices=("m98v", "m98w", "m98x"), default="m98v")
    parser.add_argument("--runtime-count", type=int,
                        help="M98x initial /N count (1 through 16)")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("refusing to overwrite the output debug script")
    args.output.write_text(build_script(args.active_count, args.initial_page,
                                        args.divisor, args.revolutions,
                                        args.scenario, args.milestone,
                                        args.runtime_count), encoding="utf-8")
    print(f"{args.milestone.upper()}_DEBUG_SCRIPT_PASS active_count={args.active_count} "
          f"divisor={args.divisor} initial_page={args.initial_page} "
          f"revolutions={args.revolutions} scenario={args.scenario} "
          f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

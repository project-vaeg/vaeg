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
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Reject geometry-specific P4 post-fill repair stages.

This is a structural guard, not a general-purpose linter.  Exact endpoint
RMW and the intended outline redraw remain allowed; named bridge/patch stages
and post-fill writes through those stages are not.
"""

import argparse
import json
import re
from pathlib import Path


FORBIDDEN_SYMBOLS = (
    "glass_p4_sgp_bridge",
    "glass_p4_sgp_patch",
    "glass_p4_sgp_repair",
    "glass_p4_sgp_fixup",
)


def validate_source(source: str):
    errors = []
    for symbol in FORBIDDEN_SYMBOLS:
        if symbol in source:
            errors.append({"code": "P4_GEOMETRY_REPAIR_SYMBOL", "symbol": symbol})

    ready = re.search(
        r"glass_p4_sgp_build_ready:(.*?)(?=^glass_p4_sgp_failed:)",
        source,
        flags=re.MULTILINE | re.DOTALL,
    )
    if ready is None:
        errors.append({"code": "P4_BUILD_READY_BLOCK_MISSING"})
    else:
        block = ready.group(1)
        if block.count("call    glass_p4_sgp_apply_endpoint_spans") != 1:
            errors.append({"code": "P4_ENDPOINT_STAGE_COUNT"})
        if "glass_p4_sgp_bridge" in block or "glass_p4_sgp_patch" in block:
            errors.append({"code": "P4_POST_FILL_REPAIR_STAGE"})

    return errors


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    args = parser.parse_args()
    errors = validate_source(args.source.read_text(encoding="utf-8"))
    result = {
        "schema": "glass-p4-no-repair-v1",
        "source": str(args.source),
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())

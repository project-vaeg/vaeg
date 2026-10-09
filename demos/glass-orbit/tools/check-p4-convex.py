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

"""Guard that P4 face rendering submits one convex polygon per face."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


def validate_source(source: str, prefix: str) -> list[dict[str, str]]:
    errors: list[dict[str, str]] = []
    start = f"glass_p4_{prefix}_draw_faces:"
    end = f"glass_p4_{prefix}_draw_edges:"
    match = re.search(
        rf"^{re.escape(start)}(.*?)(?=^{re.escape(end)})",
        source,
        flags=re.MULTILINE | re.DOTALL,
    )
    if match is None:
        return [{"code": "P4_CONVEX_FACE_BLOCK_MISSING", "prefix": prefix}]
    body = match.group(1)
    if body.count("call    glass_p4_convex_fill_polygon") != 1:
        errors.append({"code": "P4_CONVEX_FACE_CALL_COUNT", "prefix": prefix})
    if "fill_triangle" in body:
        errors.append({"code": "P4_CONVEX_TRIANGLE_DECOMPOSITION", "prefix": prefix})
    if "mov     cx, 4" not in body:
        errors.append({"code": "P4_CONVEX_QUAD_VERTEX_COUNT", "prefix": prefix})
    if 'include "glass_convex.inc"' not in source:
        errors.append({"code": "P4_CONVEX_IMPLEMENTATION_MISSING", "prefix": prefix})
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("prefix", choices=("sgp", "cpu"))
    args = parser.parse_args()
    source = args.source.read_text(encoding="utf-8")
    errors = validate_source(source, args.prefix)
    result = {
        "schema": "glass-p4-convex-face-v1",
        "source": str(args.source),
        "prefix": args.prefix,
        "status": "PASS" if not errors else "FAIL",
        "errors": errors,
    }
    print(json.dumps(result, sort_keys=True))
    return 0 if not errors else 1


if __name__ == "__main__":
    raise SystemExit(main())

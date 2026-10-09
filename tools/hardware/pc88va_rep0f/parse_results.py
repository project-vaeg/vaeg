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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Validate one raw probe log and emit a source-labelled JSON result."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys
from typing import Iterable


PREFIX = "VAEG_REP0F,"
HEX4 = re.compile(r"[0-9a-f]{4}")
HEX24 = re.compile(r"[0-9a-f]{24}")


class ResultError(RuntimeError):
    """The probe log does not satisfy the fail-closed result schema."""


def parse_line(line: str) -> dict[str, str]:
    if not line.startswith(PREFIX):
        raise ResultError("not a probe line")
    fields: dict[str, str] = {}
    for item in line[len(PREFIX):].strip().split(","):
        if "=" not in item:
            raise ResultError("malformed item {!r}".format(item))
        key, value = item.split("=", 1)
        if not key or key in fields:
            raise ResultError("duplicate or empty key {!r}".format(key))
        fields[key] = value
    return fields


def parse(path: pathlib.Path, source: str) -> dict[str, object]:
    raw = path.read_bytes()
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError as error:
        raise ResultError("raw log is not ASCII") from error
    records = [parse_line(line) for line in text.splitlines()
               if line.startswith(PREFIX)]
    if len(records) != 2:
        raise ResultError("one before and one after record are required")
    before, after = records
    before_keys = {
        "v", "event", "case", "bytes", "ax", "bx", "cx", "dx", "si",
        "di", "bp", "sp", "ip", "flags", "guard",
    }
    after_keys = {
        "v", "event", "case", "completion", "trap", "ip", "cs", "flags",
        "ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "ds", "es",
        "ss", "guard",
    }
    if set(before) != before_keys or set(after) != after_keys:
        raise ResultError("result fields changed")
    if (before["v"] != "1" or after["v"] != "1"
            or before["event"] != "before" or after["event"] != "after"
            or before["case"] != after["case"]
            or not re.fullmatch(r"r00[1-8]", before["case"])
            or after["completion"] != "1"
            or after["trap"] not in {"01", "06"}):
        raise ResultError("event identity or trap changed")
    for record, keys in ((before, before_keys - {"v", "event", "case", "bytes",
                                                 "completion", "trap", "guard"}),
                         (after, after_keys - {"v", "event", "case", "bytes",
                                              "completion", "trap", "guard"})):
        if any(not HEX4.fullmatch(record[key]) for key in keys):
            raise ResultError("register/flag value is not four hex digits")
    if (not re.fullmatch(r"[0-9a-f]{6,12}", before["bytes"])
            or not HEX24.fullmatch(before["guard"])
            or not HEX24.fullmatch(after["guard"])):
        raise ResultError("bytes or guard value is malformed")
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-pc88va-rep0f-result-m47-v1",
        "source": source,
        "raw_log_sha256": hashlib.sha256(raw).hexdigest(),
        "case_id": before["case"],
        "records": records,
    }


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, choices=("emulator", "hardware"))
    parser.add_argument("log", type=pathlib.Path)
    arguments = parser.parse_args(sys.argv[1:] if argv is None else argv)
    try:
        value = parse(arguments.log, arguments.source)
    except (OSError, ResultError) as error:
        print("pc88va-rep0f-result: FAIL: {}".format(error), file=sys.stderr)
        return 1
    print(json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

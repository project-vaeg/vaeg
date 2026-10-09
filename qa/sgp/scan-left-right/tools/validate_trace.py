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

"""Check the gated VAEG SCAN trace against the independent test layout."""

import argparse
import json
import re
import sys


RESULT = re.compile(
    r"SGP_SCAN: (SCAN_(?:LEFT|RIGHT)) result found=(\d+) width=(\d+) "
    r"addr=([0-9a-f]+) dot=(\d+)"
)
START = re.compile(
    r"SGP_SCAN: (SCAN_(?:LEFT|RIGHT)) start addr=([0-9a-f]+) dot=(\d+) "
    r"width=(\d+) mode=(\d+) color=([0-9a-f]+)"
)


def validate(path):
    starts = []
    results = []
    for line in open(path, encoding="utf-8"):
        match = START.search(line)
        if match:
            starts.append(match.groups())
        match = RESULT.search(line)
        if match:
            results.append(match.groups())
    errors = []
    if len(starts) != 100:
        errors.append("SCANLR_TRACE_START_COUNT")
    if len(results) != 100:
        errors.append("SCANLR_TRACE_RESULT_COUNT")
    if any(item[1] != "1" for item in results):
        errors.append("SCANLR_SCAN_NOT_FOUND")
    if any(item[3] != "51" or item[4] != "1" or item[5] != "ffff" for item in starts):
        errors.append("SCANLR_SCAN_SETUP")
    widths = [int(item[2]) for item in results]
    # There are 40 ten-row scans for the 100..200 pair and 10 adjacent scans.
    if widths.count(50) != 80 or widths.count(1) != 20:
        errors.append("SCANLR_SCAN_WIDTH")
    result = {
        "trace": path,
        "scan_start_records": len(starts),
        "scan_result_records": len(results),
        "widths": {"50": widths.count(50), "1": widths.count(1)},
        "errors": errors,
        "trace_result": "PASS" if not errors else "FAIL",
    }
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("trace")
    args = parser.parse_args()
    try:
        result = validate(args.trace)
    except OSError as exc:
        print(json.dumps({"trace_result": "FAIL", "error": str(exc)}, sort_keys=True))
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0 if result["trace_result"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())

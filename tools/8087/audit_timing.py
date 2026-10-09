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

"""Fail-closed content audit for the documented 8087 timing table."""

from __future__ import annotations

import csv
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "docs" / "8087" / "v7" / "records" / "opcode-inventory.tsv"
TIMING = ROOT / "docs" / "8087" / "v7" / "records" / "timing-table.tsv"


class TimingError(Exception):
    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def read_rows(path: Path, required: set[str]) -> list[dict[str, str]]:
    if not path.is_file():
        raise TimingError("P19_TIMING_TABLE_MISSING", str(path))
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader((line for line in stream if not line.startswith("#")),
                                delimiter="\t")
        if set(reader.fieldnames or ()) != required:
            raise TimingError("P19_TIMING_TABLE_HEADER", str(reader.fieldnames))
        return list(reader)


def expand_modrm(value: str) -> set[int]:
    first, sep, last = value.partition("-")
    start = int(first, 16)
    end = int(last if sep else first, 16)
    if start > end or end > 0xff:
        raise TimingError("P19_TIMING_TABLE_RANGE", value)
    return set(range(start, end + 1))


def audit() -> tuple[int, int]:
    inventory = read_rows(INVENTORY,
                          {"form_id", "opcode", "modrm", "form", "class", "source"})
    timing = read_rows(TIMING,
                       {"form_id", "opcode", "modrm", "nominal_ndp_clocks", "ea",
                        "typical_min", "typical_max", "source"})
    if len(inventory) != 111:
        raise TimingError("P19_TIMING_INVENTORY_ROWS", str(len(inventory)))
    if len(timing) != len(inventory):
        raise TimingError("P19_TIMING_ROW_COUNT", f"{len(timing)} != {len(inventory)}")

    inventory_by_id = {row["form_id"]: row for row in inventory}
    if len(inventory_by_id) != len(inventory):
        raise TimingError("P19_TIMING_INVENTORY_DUPLICATE", "form_id")
    timing_by_id: dict[str, dict[str, str]] = {}
    for row in timing:
        form_id = row["form_id"]
        if form_id in timing_by_id:
            raise TimingError("P19_TIMING_DUPLICATE", form_id)
        if form_id not in inventory_by_id:
            raise TimingError("P19_TIMING_UNKNOWN_FORM", form_id)
        source = inventory_by_id[form_id]
        if (row["opcode"], row["modrm"]) != (source["opcode"], source["modrm"]):
            raise TimingError("P19_TIMING_ENCODING_MISMATCH", form_id)
        try:
            nominal = int(row["nominal_ndp_clocks"])
            minimum = int(row["typical_min"])
            maximum = int(row["typical_max"])
            ea = int(row["ea"])
        except ValueError as exc:
            raise TimingError("P19_TIMING_NUMBER", form_id) from exc
        if nominal <= 0 or minimum <= 0 or maximum < minimum:
            raise TimingError("P19_TIMING_VALUE", form_id)
        if not 0 <= ea <= 1:
            raise TimingError("P19_TIMING_EA_FLAG", form_id)
        if not (minimum <= nominal <= maximum):
            raise TimingError("P19_TIMING_NOMINAL_RANGE", form_id)
        timing_by_id[form_id] = row
    if set(timing_by_id) != set(inventory_by_id):
        raise TimingError("P19_TIMING_FORM_SET", "inventory and timing sets differ")

    slots = 0
    for row in timing:
        count = len(expand_modrm(row["modrm"]))
        if row["modrm"] == "00-7F":
            count = 24
        slots += count
    if slots != 1597:
        raise TimingError("P19_TIMING_SLOT_COUNT", str(slots))
    return len(timing), slots


def main() -> int:
    try:
        rows, slots = audit()
    except TimingError as exc:
        print(str(exc))
        return 1
    print(f"P19_TIMING rows={rows} slots={slots}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

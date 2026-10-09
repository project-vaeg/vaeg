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
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Independently expand the hand-authored Intel 8087 opcode inventory."""

from __future__ import annotations

import csv
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "docs" / "8087" / "v7" / "records" / "opcode-inventory.tsv"


class InventoryError(Exception):
    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def expand_range(value: str) -> list[int]:
    parts = value.split("-", 1)
    if len(parts) == 1:
        return [int(parts[0], 16)]
    first, last = (int(part, 16) for part in parts)
    if first > last:
        raise InventoryError("P01_INVENTORY_RANGE_REVERSED", value)
    return list(range(first, last + 1))


def expand() -> tuple[set[tuple[int, int]], int, int]:
    if not INVENTORY.is_file():
        raise InventoryError("P01_INVENTORY_MISSING", str(INVENTORY))
    slots: set[tuple[int, int]] = set()
    memory = 0
    register = 0
    rows = 0
    with INVENTORY.open(encoding="utf-8", newline="") as stream:
        filtered = (line for line in stream if not line.startswith("#"))
        reader = csv.DictReader(filtered, delimiter="\t")
        required = {"form_id", "opcode", "modrm", "form", "class", "source"}
        if set(reader.fieldnames or ()) != required:
            raise InventoryError("P01_INVENTORY_HEADER", str(reader.fieldnames))
        for row in reader:
            rows += 1
            try:
                opcode = int(row["opcode"], 16)
                values = expand_range(row["modrm"])
                if row["modrm"] == "00-7F":
                    values = [0]
            except (TypeError, ValueError) as exc:
                raise InventoryError("P01_INVENTORY_ENCODING", str(row)) from exc
            if not 0xD8 <= opcode <= 0xDF:
                raise InventoryError("P01_INVENTORY_OPCODE", row["form_id"])
            for modrm in values:
                if not 0 <= modrm <= 0xFF:
                    raise InventoryError("P01_INVENTORY_MODRM", row["form_id"])
                if row["modrm"] == "00-7F":
                    match = re.search(r"M([0-7])$", row["form_id"])
                    if match is None:
                        raise InventoryError("P01_MEMORY_FORM_ID", row["form_id"])
                    reg = int(match.group(1))
                    for mod in (0, 1, 2):
                        for rm in range(8):
                            expanded = (mod << 6) | (reg << 3) | rm
                            if (opcode, expanded) in slots:
                                raise InventoryError("P01_DUPLICATE_SLOT", f"{opcode:02X}:{expanded:02X}")
                            slots.add((opcode, expanded))
                            memory += 1
                else:
                    expanded = 0xC0 | (modrm & 0x3F)
                    if (opcode, expanded) in slots:
                        raise InventoryError("P01_DUPLICATE_SLOT", f"{opcode:02X}:{expanded:02X}")
                    slots.add((opcode, expanded))
                    register += 1
    if rows != 111:
        raise InventoryError("P01_INVENTORY_ROW_COUNT", str(rows))
    if memory != 1368 or register != 229 or len(slots) != 1597:
        raise InventoryError("P01_INVENTORY_COUNT", f"memory={memory} register={register} total={len(slots)}")
    return slots, memory, register


def main() -> int:
    try:
        slots, memory, register = expand()
    except InventoryError as exc:
        print(str(exc))
        return 1
    print(f"P01_INVENTORY rows=111 memory={memory} register={register} total={len(slots)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

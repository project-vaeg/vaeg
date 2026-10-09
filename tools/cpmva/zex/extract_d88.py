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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Extract the files of a CP/MVA disk image written by vaeg or a real machine.

usage: extract_d88.py IMAGE OUTPUT_DIR [--lowercase] [--skip-com] [--allow-unused-errors]
                      [--drop-entry N ...]

Unlike the installer's strict reader, this reads any D88 container (for
example one read with a KryoFlux and converted by the HxC Floppy Emulator
software, with another header name,
media type and track order). Each sector is placed by its C/H/R address. The
CP/MVA geometry must be complete: 40 cylinders x 2 heads x 16 sectors of 256
bytes, no duplicates and no sector errors. With --allow-unused-errors, sectors
with an error status are accepted only if they lie outside the directory and
outside every block that a directory entry allocates. Files are written with
their 128-byte record length, as CP/M stores them. --drop-entry N treats the
directory entry with index N (0-127) as unused; it is refused unless the
entry's name is not a printable 8.3 name, so that only a corrupted entry (for
example one written while a probe crashed) can be dropped.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
from pathlib import Path
import struct
import sys

CYLINDERS = 40
HEADS = 2
SECTORS = 16
SECTOR_SIZE = 256
TRACK_SLOTS = 164
HEADER_SIZE = 0x2B0


class ExtractError(Exception):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(f"{code}: {message}")
        self.code = code


def load_installer():
    path = Path(__file__).resolve().parent.parent / "install_cpmva.py"
    spec = importlib.util.spec_from_file_location("cpmva_installer_extract", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


DIRECTORY_OFFSET = 0x4000
DIRECTORY_SIZE = 0x1000
BLOCK_SIZE = 2048


def raw_from_d88(image: bytes, errors: list | None = None) -> bytes:
    """Rebuild the linear CP/M image from sector C/H/R addresses.

    Sectors with an error status raise D88_STATUS unless ``errors`` is a
    list, in which case their raw offsets are appended to it.
    """
    if len(image) < HEADER_SIZE:
        raise ExtractError("D88_SIZE", "image is shorter than a D88 header")
    raw = bytearray(CYLINDERS * HEADS * SECTORS * SECTOR_SIZE)
    seen = set()
    for slot in range(TRACK_SLOTS):
        offset = struct.unpack_from("<I", image, 0x20 + slot * 4)[0]
        if offset == 0:
            continue
        if offset < HEADER_SIZE or offset + 16 > len(image):
            raise ExtractError("D88_TRACK", f"track {slot} offset {offset:#x} is out of range")
        count = struct.unpack_from("<H", image, offset + 4)[0]
        cursor = offset
        for _ in range(count):
            if cursor + 16 > len(image):
                raise ExtractError("D88_SECTOR", f"sector header beyond the image in track {slot}")
            c, h, r, _n = struct.unpack_from("<BBBB", image, cursor)
            status = image[cursor + 8]
            size = struct.unpack_from("<H", image, cursor + 14)[0]
            data = image[cursor + 16 : cursor + 16 + size]
            if status != 0 and errors is None:
                raise ExtractError("D88_STATUS", f"sector C{c} H{h} R{r} has status {status:#x}")
            if size != SECTOR_SIZE or len(data) != SECTOR_SIZE:
                raise ExtractError("D88_GEOMETRY", f"sector C{c} H{h} R{r} is {size} bytes")
            if c >= CYLINDERS or h >= HEADS or not 1 <= r <= SECTORS:
                raise ExtractError("D88_GEOMETRY", f"sector C{c} H{h} R{r} is outside CP/MVA geometry")
            if (c, h, r) in seen:
                raise ExtractError("D88_DUPLICATE", f"sector C{c} H{h} R{r} appears twice")
            seen.add((c, h, r))
            start = ((c * HEADS + h) * SECTORS + (r - 1)) * SECTOR_SIZE
            raw[start : start + SECTOR_SIZE] = data
            if status != 0:
                errors.append(start)
            cursor += 16 + size
    missing = CYLINDERS * HEADS * SECTORS - len(seen)
    if missing:
        raise ExtractError("D88_MISSING", f"{missing} sectors are missing")
    return bytes(raw)


def allocated_blocks(raw: bytes) -> set[int]:
    blocks = set()
    for index in range(DIRECTORY_SIZE // 32):
        entry = raw[DIRECTORY_OFFSET + index * 32 : DIRECTORY_OFFSET + index * 32 + 32]
        if entry[0] != 0xE5:
            blocks.update(b for b in entry[16:32] if b)
    return blocks


def printable_name(entry: bytes) -> bool:
    return all(0x20 <= (byte & 0x7F) < 0x7F for byte in entry[1:12])


def drop_entries(raw: bytes, indices: list[int]) -> bytes:
    """Mark the given corrupted directory entries as unused."""
    data = bytearray(raw)
    for index in indices:
        if not 0 <= index < DIRECTORY_SIZE // 32:
            raise ExtractError("DROP_ENTRY_RANGE", f"directory entry {index} does not exist")
        offset = DIRECTORY_OFFSET + index * 32
        entry = bytes(data[offset : offset + 32])
        if entry[0] == 0xE5:
            raise ExtractError("DROP_ENTRY_UNUSED", f"directory entry {index} is already unused")
        if printable_name(entry):
            raise ExtractError("DROP_ENTRY_VALID", f"directory entry {index} has a valid name")
        print(f"note: dropped directory entry {index}: {entry.hex()}", file=sys.stderr)
        data[offset] = 0xE5
    return bytes(data)


def extract(image: bytes, allow_unused_errors: bool = False,
            drop: list[int] | None = None) -> dict[str, bytes]:
    installer = load_installer()
    errors: list[int] | None = [] if allow_unused_errors else None
    raw = drop_entries(raw_from_d88(image, errors), drop or [])
    if errors:
        used = allocated_blocks(raw)
        for start in errors:
            if DIRECTORY_OFFSET <= start < DIRECTORY_OFFSET + DIRECTORY_SIZE:
                raise ExtractError("D88_STATUS", f"sector error in the directory at {start:#x}")
            block = (start - DIRECTORY_OFFSET) // BLOCK_SIZE
            if start < DIRECTORY_OFFSET or block in used:
                raise ExtractError("D88_STATUS", f"sector error in allocated block {block}")
            print(f"note: sector error in unallocated block {block} ignored", file=sys.stderr)
    return installer.parse_cpm_raw(raw)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--lowercase", action="store_true", help="write lower-case file names")
    parser.add_argument("--skip-com", action="store_true", help="do not write .COM programs")
    parser.add_argument("--allow-unused-errors", action="store_true",
                        help="accept sector errors outside the directory and allocated blocks")
    parser.add_argument("--drop-entry", type=int, action="append", default=[], metavar="N",
                        help="treat the corrupted directory entry N as unused")
    args = parser.parse_args()
    try:
        files = extract(args.image.read_bytes(), args.allow_unused_errors, args.drop_entry)
    except ExtractError as error:
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    args.output.mkdir(parents=True, exist_ok=True)
    for name, data in sorted(files.items()):
        if args.skip_com and name.endswith(".COM"):
            continue
        target = name.lower() if args.lowercase else name
        (args.output / target).write_bytes(data)
        print(f"{hashlib.sha256(data).hexdigest()}  {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

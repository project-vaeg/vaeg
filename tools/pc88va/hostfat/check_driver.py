#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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

import argparse
import hashlib
import struct
from pathlib import Path


EXPECTED_BPB = bytes.fromhex("000410000002800052fff0070090")


def fail(message: str) -> None:
    raise SystemExit(f"HOSTFAT.SYS check failed: {message}")


def require_count(data: bytes, marker: bytes, expected: int, description: str) -> None:
    actual = data.count(marker)
    if actual != expected:
        fail(f"{description} was found {actual} times, expected {expected}")


def dispatch_target(data: bytes, command: int) -> tuple[int, int]:
    marker = bytes((0x3C, command))  # CMP AL, imm8
    require_count(data, marker, 1, f"command {command:02X}H comparison")
    branch = data.index(marker) + len(marker)
    if data[branch] != 0x75:  # JNE rel8 over JMP rel16
        fail(f"command {command:02X}H comparison is not followed by short JNE")
    next_path = branch + 2 + struct.unpack_from("<b", data, branch + 1)[0]
    jump = branch + 2
    if data[jump] == 0xEB:  # JMP rel8
        jump_end = jump + 2
        target = jump_end + struct.unpack_from("<b", data, jump + 1)[0]
    elif data[jump] == 0xE9:  # JMP rel16
        jump_end = jump + 3
        target = jump_end + struct.unpack_from("<h", data, jump + 1)[0]
    else:
        fail(f"command {command:02X}H dispatch does not use an 8086 JMP")
    if next_path != jump_end:
        fail(f"command {command:02X}H short JNE does not skip its JMP")
    if not (0 <= target < len(data)):
        fail(f"command {command:02X}H target is outside the driver")
    return target, next_path


def main() -> None:
    parser = argparse.ArgumentParser(description="Validate the M54 HOSTFAT.SYS")
    parser.add_argument("--input", type=Path, required=True)
    args = parser.parse_args()
    data = args.input.read_bytes()
    if len(data) < 128 or len(data) > 4096:
        fail(f"unexpected size {len(data)}")
    for opcode in range(0x80, 0x90):
        if bytes((0x0F, opcode)) in data:
            fail("386 near conditional jump is not valid in the V30 driver")
    next_driver, attributes, strategy, interrupt = struct.unpack_from("<IHHH", data)
    if next_driver != 0xFFFFFFFF:
        fail("next-driver pointer is not FFFFFFFF")
    if attributes != 0x2000:
        fail(f"attributes are {attributes:04x}, expected 2000")
    if not (15 <= strategy < len(data)) or not (15 <= interrupt < len(data)):
        fail("strategy or interrupt offset is outside the driver")
    if strategy == interrupt:
        fail("strategy and interrupt entry points are identical")
    if data[10] != 1:
        fail(f"unit count is {data[10]}, expected 1")
    name_offset, name_segment = struct.unpack_from("<HH", data, 11)
    if name_segment != 0 or name_offset + 8 > len(data):
        fail("device-name far pointer is invalid")
    if data[name_offset : name_offset + 8] != b"HOSTFAT\0":
        fail("device name is not HOSTFAT")
    require_count(data, EXPECTED_BPB, 1, "PC-Engine HOSTFAT BPB")
    bpb_offset = data.find(EXPECTED_BPB)
    (bytes_per_sector, sectors_per_cluster, reserved_sectors, fat_copies,
     root_entries, total_sectors, _media_id, sectors_per_fat) = (
        struct.unpack_from("<HBHBHHBH", data, bpb_offset)
    )
    root_sectors = (
        root_entries * 32 + bytes_per_sector - 1
    ) // bytes_per_sector
    data_clusters = (
        total_sectors - reserved_sectors - fat_copies * sectors_per_fat
        - root_sectors
    ) // sectors_per_cluster
    if data_clusters >= 4085:
        fail(
            f"DOS-visible geometry has {data_clusters} clusters and would "
            "be interpreted as FAT16"
        )
    if data_clusters != 4084:
        fail(
            f"DOS-visible geometry has {data_clusters} data clusters, "
            "expected the FAT12 maximum of 4084"
        )
    if bytes_per_sector != 1024 or sectors_per_cluster != 16:
        fail(
            "PC-Engine geometry does not use 1024-byte sectors and "
            "16 KiB clusters"
        )
    bpb_pointer_list_offset = bpb_offset - 2
    if bpb_pointer_list_offset < 15:
        fail("BPB pointer list is outside the driver data area")
    if struct.unpack_from("<H", data, bpb_pointer_list_offset)[0] != bpb_offset:
        fail("BPB pointer list does not point to the BPB")

    # PC-Engine's non-IBM block request packet has eight reserved bytes at
    # offsets 5..12. Validate the emitted field displacements themselves so
    # an IBM-sized 18-byte packet layout cannot pass this structural check.
    request_layout_markers = (
        (bytes.fromhex("26c6470e01"), 1, "media-check result at 0EH"),
        (bytes.fromhex("26c6470df0"), 1, "Build-BPB media byte at 0DH"),
        (bytes.fromhex("26c6470d01"), 1, "initialize media byte at 0DH"),
        (bytes.fromhex("26c6470d00"), 1, "unavailable media byte at 0DH"),
        (b"\x26\xc7\x47\x12" + struct.pack("<H", bpb_offset), 1,
         "Build-BPB pointer offset at 12H"),
        (b"\x26\xc7\x47\x12" + struct.pack("<H", bpb_pointer_list_offset), 1,
         "initialize BPB-list pointer offset at 12H"),
        (bytes.fromhex("8cc826894714"), 2, "BPB pointer segment at 14H"),
        (bytes.fromhex("26c747120000"), 2, "zero sector-count/BPB offset at 12H"),
        (bytes.fromhex("26c747140000"), 1, "zero BPB segment at 14H"),
        (bytes.fromhex("26c7470e0000"), 1, "resident-end offset at 0EH"),
        (bytes.fromhex("26894710"), 1, "resident-end segment at 10H"),
        (bytes.fromhex("26894703"), 1, "completion status at 03H"),
        (bytes.fromhex("b80881"), 1, "read-error status 8108H"),
    )
    for marker, expected, description in request_layout_markers:
        require_count(data, marker, expected, description)

    commands = (0x00, 0x01, 0x02, 0x04, 0x08, 0x09, 0x0D, 0x0E, 0x0F)
    dispatch = {command: dispatch_target(data, command) for command in commands}
    positions = [data.index(bytes((0x3C, command))) for command in commands]
    if positions != sorted(positions):
        fail("command comparisons are not in deterministic dispatch order")

    if dispatch[0x08][0] != dispatch[0x09][0]:
        fail("write and write-with-verify do not share the protected path")
    if not data.startswith(bytes.fromhex("b80081"), dispatch[0x08][0]):
        fail("write path does not return write-protect status 8100H")
    if dispatch[0x0D][0] != dispatch[0x0E][0]:
        fail("open and close do not share the successful no-op path")
    if not data.startswith(bytes.fromhex("b80001"), dispatch[0x0D][0]):
        fail("open/close path does not return successful status 0100H")
    if not data.startswith(bytes.fromhex("b80002"), dispatch[0x0F][0]):
        fail("removable-query path does not return status 0200H")
    if not data.startswith(bytes.fromhex("b80381"), dispatch[0x0F][1]):
        fail("unknown-command path does not return status 8103H")
    if not data.startswith(bytes.fromhex("26c6470e01"), dispatch[0x01][0]):
        fail("media-check dispatch does not write its result at 0EH")
    if not data.startswith(bytes.fromhex("26c6470df0"), dispatch[0x02][0]):
        fail("Build-BPB dispatch does not write media ID F0H at 0DH")

    if len(data) % 16 != 0:
        fail("resident image is not paragraph aligned")
    resident_paragraphs = len(data) // 16
    if resident_paragraphs > 0x7F:
        fail("resident image exceeds the clean-room imm8 paragraph contract")
    resident_marker = (
        bytes.fromhex("26c7470e00008cc883c0") + bytes((resident_paragraphs,))
        + bytes.fromhex("26894710")
    )
    require_count(data, resident_marker, 1, "paragraph-rounded resident end")
    require_count(data, bytes.fromhex("80fc48"), 1, "protocol signature H comparison")
    require_count(data, bytes.fromhex("3c31"), 1, "protocol signature 1 comparison")

    for marker in (
        b"check_hostfat",
        b"read_hostfat1",
        b"\r\nHOSTFAT read-only drive ready\r\n\0",
        b"\r\nHOSTFAT unavailable (start vaeg with --hostfat-dir)\r\n\0",
    ):
        require_count(data, marker, 1, f"protocol marker {marker!r}")
    digest = hashlib.sha256(data).hexdigest()
    print(f"HOSTFAT.SYS ok: {len(data)} bytes sha256={digest}")


if __name__ == "__main__":
    main()

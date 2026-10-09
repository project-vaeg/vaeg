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
"""Generate and verify the G60b ROM-authority and target-policy epoch."""

from __future__ import annotations

import argparse
import contextlib
import copy
import csv
import gzip
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from collections.abc import Iterable, Iterator
from typing import Any

import upd9002_m60a_evidence as m60a
import upd9002_semantics_evidence as m59
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M60b"
CANDIDATE_GATE = "G60b"
APPROVED_PREDECESSOR_GATE = "G60a"
# Evidence keeps the original identity; Git topology follows rewritten history.
APPROVED_PREDECESSOR_SHA = "ba2b7d3f5c76646b30d63fd8951f4a1964817b15"
APPROVED_PREDECESSOR_GIT_SHA = "55db5075f2a942e6d377f74ce1ab908cc1982962"
G60A_EVALUATED_SHA = "3d66d41f750048eb29d13c4b7b53ea757d1d1921"
G60A_FULL_TRANSITION_SHA256 = (
    "86b05dba8b958eb731c89c016cd9898b18ac5ff91a53229c0ef3a3aa797e8c13"
)
G60A_ARTIFACT_TREE_SHA256 = (
    "2f03e42095da58d521ac5b491a571faf6d078db2b12794f2e0249354345c2901"
)

ROM_SHA256 = "0460b58d4c5fa19cc8f9a2120bb7e65bbe7c78613552c3057a718444b76e8fee"
ROM_SHA1 = "bcaea28c58816602ca1e8290f534360f1ca03fe8"
ROM_CRC32 = "98c9959a"
ROM_SIZE = 0x80000
ROM_BANK_FILE_BASE = 0x60000

DISPATCH_0F_NOMINAL_OFFSET = 0x66A8A
DISPATCH_0F_START = 0x66A8B
DISPATCH_0F_END = 0x66AB5
DISPATCH_0F_STRING_START = DISPATCH_0F_END
DISPATCH_0F_RECORD_COUNT = 14
DISPATCH_MAIN_START = 0x66350
DISPATCH_MAIN_END = 0x664F4
DISPATCH_MAIN_RECORD_COUNT = 140
DISPATCH_MAIN_MNEMONIC_START = 0x66515
DISPATCH_MAIN_MNEMONIC_END = 0x666FA
DISPATCH_GROUP_START = 0x668FD
DISPATCH_GROUP_END = 0x66921
DISPATCH_GROUP_RECORD_COUNT = 12
STRING_POOL_START = 0x66600
STRING_POOL_END = 0x66F7A

OLD_SUPPORT_MAP = pathlib.Path("tools/qa/golden/upd9002_support_map_m48.csv")
MANIFEST_PATH = pathlib.Path("tests/ssts/v20_dataset_manifest.json")
ARCH_CONTRACT_PATH = pathlib.Path(
    "tests/ssts/contracts/upd9002_architectural_v1.json"
)
FINGERPRINT_CONTRACT_PATH = pathlib.Path(
    "tests/ssts/contracts/upd9002_fingerprint_v1.json"
)
G43_MANIFEST_PATH = pathlib.Path("tests/ssts/epochs/g43/manifest.json")
HARDWARE_PENDING_PATH = pathlib.Path("tests/ssts/hardware_pending.json")
APPROVED_DIVERGENCES_PATH = pathlib.Path(
    "tests/ssts/approved_target_divergences.json"
)
OLD_KNOWN_GAPS_PATH = pathlib.Path(
    "tests/ssts/baseline/upd9002_v20_known_gaps.json"
)
OLD_TAXONOMY_PATH = pathlib.Path("tests/ssts/gap_taxonomy.json")
AUTHORITY_ROOT = pathlib.Path("tests/ssts/authority/g60b")
TARGET_POLICY_PATH = pathlib.Path("tests/ssts/target_policy/g60b.json")
TARGET_POLICY_KNOWN_GAPS_PATH = pathlib.Path(
    "tests/ssts/target_policy/g60b_known_gaps.json"
)
TARGET_POLICY_TAXONOMY_PATH = pathlib.Path(
    "tests/ssts/target_policy/g60b_gap_taxonomy.json"
)

AUTHORIZED_PRIMARY_OPCODES = {0x6C, 0x6D, 0x6E, 0x6F}
TARGET_SELECTED_COUNTS = {"ci": 2000, "full": 7000}
AUTHORIZED_GAP_KIND_FORMS = {"0F31", "0F33", "0F39", "0F3B"}
ROM_PRESENT_0F_FORMS = {
    "0F10",
    "0F11",
    "0F12",
    "0F13",
    "0F14",
    "0F15",
    "0F16",
    "0F17",
    "0F18",
    "0F19",
    "0F1A",
    "0F1B",
    "0F1C",
    "0F1D",
    "0F1E",
    "0F1F",
    "0F20",
    "0F22",
    "0F26",
    "0F28",
    "0F2A",
    "0FFE",
    "0FFF",
}

EXPECTED_DEBUGGER_SNAPSHOTS = {
    557: "c01b045bbd8773076e0d258a6c292123ed407b2afb23a9dc4e399ab627972254",
    563: "d8f62bb6c7bd7dc3ddbca2d88d282efdf065e44dc7cd976a0209f6f48d6fd51c",
    566: "ae522c162dde4374d44a849d184bdcb525961378796c19bed386415262c95c7b",
}


class M60bError(ValueError):
    """A G60b authority or target-policy invariant failed closed."""


def canonical_bytes(value: Any) -> bytes:
    return ratchet.canonical_bytes(value)


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    return ratchet.sha256_file(path)


def write_json(path: pathlib.Path, value: Any) -> None:
    ratchet.write_json(path, value)


def read_json(path: pathlib.Path) -> Any:
    return ratchet.read_json(path)


def require_keys(value: Any, expected: set[str], field: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != expected:
        raise M60bError(f"{field}: unknown schema")
    return value


def require_sha256(value: Any, field: str) -> str:
    try:
        return ratchet.require_sha256(value, field)
    except ratchet.RatchetError as error:
        raise M60bError(str(error)) from error


def require_sha(value: Any, field: str) -> str:
    try:
        return ratchet.require_sha(value, field)
    except ratchet.RatchetError as error:
        raise M60bError(str(error)) from error


def hex_offset(value: int) -> str:
    return f"0x{value:05x}"


def identity_digest(value: Any) -> str:
    return sha256_bytes(canonical_bytes(value))


def validate_rom_identity(data: bytes) -> None:
    validate_rom_claim(len(data), sha256_bytes(data))


def validate_rom_claim(byte_count: int, digest: str) -> None:
    if byte_count != ROM_SIZE:
        raise M60bError(
            f"ROM size differs: expected={ROM_SIZE} actual={byte_count}"
        )
    if digest != ROM_SHA256:
        raise M60bError(
            f"ROM SHA-256 differs: expected={ROM_SHA256} actual={digest}"
        )


def parse_records(
    data: bytes,
    start: int,
    end: int,
    expected_count: int,
    field: str,
) -> list[dict[str, Any]]:
    if start < 0 or end > len(data) or end <= start:
        raise M60bError(f"{field}: truncated table")
    raw = data[start:end]
    if len(raw) % 3:
        raise M60bError(f"{field}: malformed three-byte record")
    if len(raw) // 3 != expected_count:
        raise M60bError(
            f"{field}: unexpected raw-record count "
            f"{len(raw) // 3}, expected {expected_count}"
        )
    records = []
    for index in range(0, len(raw), 3):
        mask, value, group = raw[index:index + 3]
        if value & mask != value:
            raise M60bError(f"{field}: value has bits outside mask")
        records.append(
            {
                "group": f"0x{group:02x}",
                "index": index // 3,
                "mask": f"0x{mask:02x}",
                "offset": hex_offset(start + index),
                "raw": raw[index:index + 3].hex(),
                "value": f"0x{value:02x}",
            }
        )
    return records


def decode_high_bit_strings(
    data: bytes,
    start: int,
    count: int,
    field: str,
    allow_empty: bool = False,
) -> tuple[list[dict[str, Any]], int]:
    result = []
    position = start
    for index in range(count):
        item_start = position
        decoded = bytearray()
        while True:
            if position >= len(data):
                raise M60bError(f"{field}: missing string terminator")
            byte = data[position]
            position += 1
            decoded.append(byte & 0x7F)
            if byte & 0x80:
                break
        if allow_empty and decoded == b"\0":
            text = ""
        else:
            try:
                text = decoded.decode("ascii")
            except UnicodeDecodeError as error:
                raise M60bError(f"{field}: non-ASCII mnemonic") from error
        if (
            (not text and not allow_empty)
            or any(ord(character) < 0x20 for character in text)
        ):
            raise M60bError(f"{field}: malformed mnemonic")
        result.append(
            {
                "index": index,
                "offset": hex_offset(item_start),
                "raw": data[item_start:position].hex(),
                "text": text,
            }
        )
    return result, position


def expand_record(record: dict[str, Any]) -> list[str]:
    mask = int(record["mask"], 16)
    value = int(record["value"], 16)
    return [f"0x{opcode:02x}" for opcode in range(256) if opcode & mask == value]


def validate_expansion(
    records: list[dict[str, Any]],
    mnemonics: list[dict[str, Any]] | None,
    field: str,
    allow_exact_duplicates: bool = False,
    allow_mnemonic_aliases: bool = False,
) -> list[dict[str, Any]]:
    if mnemonics is not None and len(records) != len(mnemonics):
        raise M60bError(f"{field}: incorrect group-to-mnemonic mapping")
    group_owners: dict[tuple[str, str], list[int]] = defaultdict(list)
    mnemonic_owners: dict[str, set[str]] = defaultdict(set)
    expanded = []
    for index, record in enumerate(records):
        opcodes = expand_record(record)
        mnemonic = None if mnemonics is None else mnemonics[index]["text"]
        for opcode in opcodes:
            group_owners[(opcode, record["group"])].append(index)
            if mnemonic is not None:
                mnemonic_owners[opcode].add(mnemonic)
        expanded.append(
            {
                "expanded_opcodes": opcodes,
                "group": record["group"],
                "index": record["index"],
                "mnemonic": mnemonic,
            }
        )
    duplicate = {
        owner: indexes
        for owner, indexes in group_owners.items()
        if len(indexes) != 1
    }
    if duplicate and not allow_exact_duplicates:
        first = sorted(duplicate)[0]
        raise M60bError(
            f"{field}: duplicate expanded opcode/group at "
            f"{first[0]}/{first[1]}"
        )
    ambiguous = {
        opcode: sorted(values)
        for opcode, values in mnemonic_owners.items()
        if len(values) > 1
    }
    if ambiguous and not allow_mnemonic_aliases:
        first = sorted(ambiguous)[0]
        raise M60bError(
            f"{field}: overlapping or ambiguous mask expansion at {first}"
        )
    return expanded


def encode_monitor_string(text: str) -> bytes:
    if not text or not text.isascii():
        raise M60bError("monitor search pattern must be nonempty ASCII")
    result = bytearray(text.encode("ascii"))
    result[-1] |= 0x80
    return bytes(result)


def all_offsets(haystack: bytes, needle: bytes, base: int) -> list[str]:
    result = []
    position = 0
    while True:
        found = haystack.find(needle, position)
        if found < 0:
            break
        result.append(hex_offset(base + found))
        position = found + 1
    return result


def string_pool_audit(data: bytes) -> dict[str, Any]:
    if STRING_POOL_END > len(data) or STRING_POOL_START >= STRING_POOL_END:
        raise M60bError("incomplete string-pool search range")
    region = data[STRING_POOL_START:STRING_POOL_END]
    names = (
        "BRKEM",
        "BRKFEM",
        "REPC",
        "REPNC",
        "PREPARE",
        "DISPOSE",
        "INM",
        "OUTM",
        "INS",
        "OUTS",
        "IN",
        "OUT",
        "FPO1",
        "FPO2",
        "ESC",
        "FADD",
        "FMUL",
    )
    patterns = []
    for name in names:
        encoded = encode_monitor_string(name)
        patterns.append(
            {
                "encoded_hex": encoded.hex(),
                "matches": all_offsets(region, encoded, STRING_POOL_START),
                "text": name,
            }
        )
    by_name = {item["text"]: item["matches"] for item in patterns}
    for required in ("REPC", "REPNC", "PREPARE", "DISPOSE"):
        if not by_name[required]:
            raise M60bError(f"string-pool audit: missing {required}")
    if by_name["INS"] or by_name["OUTS"] or by_name["OUTM"]:
        raise M60bError("string-pool audit contradicts 6c-6f mnemonic absence")
    if not by_name["IN"] or not by_name["OUT"]:
        raise M60bError("string-pool audit lost ordinary IN/OUT controls")
    if not by_name["FADD"] or not by_name["FMUL"]:
        raise M60bError("string-pool audit lost individual 8087 mnemonics")
    return {
        "algorithm": "exact-high-bit-terminated-ascii-v1",
        "patterns": patterns,
        "schema": "vaeg-upd9002-rom-string-pool-audit-v1",
        "schema_version": 1,
        "search_end_exclusive": hex_offset(STRING_POOL_END),
        "search_range_sha256": sha256_bytes(region),
        "search_start": hex_offset(STRING_POOL_START),
    }


def validate_debugger_source(value: Any) -> dict[str, Any]:
    source = require_keys(
        value,
        {
            "authorization",
            "observations",
            "provenance",
            "schema",
            "schema_version",
        },
        "debugger evidence",
    )
    if (
        source["schema"] != "vaeg-upd9002-debugger-evidence-source-v1"
        or source["schema_version"] != 1
    ):
        raise M60bError("debugger evidence: unsupported schema")
    observations = source["observations"]
    if not isinstance(observations, list) or len(observations) != 3:
        raise M60bError("missing BRKFEM debugger corroboration")
    posts = {}
    for item in observations:
        require_keys(
            item,
            {"claim", "local_snapshot_sha256", "post", "url"},
            "debugger observation",
        )
        post = item["post"]
        if post not in EXPECTED_DEBUGGER_SNAPSHOTS or post in posts:
            raise M60bError("debugger evidence: unexpected or duplicate post")
        require_sha256(
            item["local_snapshot_sha256"],
            f"debugger observation {post}",
        )
        if item["local_snapshot_sha256"] != EXPECTED_DEBUGGER_SNAPSHOTS[post]:
            raise M60bError("debugger evidence: snapshot digest differs")
        expected_suffix = f"/{post}"
        if (
            not isinstance(item["url"], str)
            or not item["url"].startswith("https://yomi.tokyo/")
            or not item["url"].endswith(expected_suffix)
        ):
            raise M60bError("debugger evidence: provenance URL differs")
        if not isinstance(item["claim"], str) or not item["claim"]:
            raise M60bError("debugger evidence: empty observation")
        posts[post] = item
    if set(posts) != set(EXPECTED_DEBUGGER_SNAPSHOTS):
        raise M60bError("missing BRKFEM debugger corroboration")
    claims = " ".join(item["claim"].lower() for item in observations)
    if "0f ff imm8" not in claims or "0f fe imm8" not in claims:
        raise M60bError("missing BRKFEM debugger encoding corroboration")
    return source


def table_document(
    records: list[dict[str, Any]],
    start: int,
    end: int,
    name: str,
) -> dict[str, Any]:
    return {
        "end_exclusive": hex_offset(end),
        "name": name,
        "raw_record_count": len(records),
        "raw_records": records,
        "schema": "vaeg-upd9002-rom-dispatch-raw-v1",
        "schema_version": 1,
        "start": hex_offset(start),
    }


def validate_forbidden_0f_inventory(inventory: Any) -> None:
    if not isinstance(inventory, list):
        raise M60bError("authority pack inventory is malformed")
    forbidden = {"0x31", "0x33", "0x39", "0x3b"}
    if forbidden & {item.get("second_opcode") for item in inventory}:
        raise M60bError("0f31/33/39/3b presence contradiction")


def extract_authority_documents(
    rom_data: bytes, debugger_source: dict[str, Any]
) -> dict[str, dict[str, Any]]:
    validate_rom_identity(rom_data)
    debugger_source = validate_debugger_source(debugger_source)

    if not rom_data[DISPATCH_0F_NOMINAL_OFFSET] & 0x80:
        raise M60bError("0f dispatch: nominal boundary is not a string terminator")
    records_0f = parse_records(
        rom_data,
        DISPATCH_0F_START,
        DISPATCH_0F_END,
        DISPATCH_0F_RECORD_COUNT,
        "0f dispatch",
    )
    mnemonics_0f, mnemonic_end = decode_high_bit_strings(
        rom_data,
        DISPATCH_0F_STRING_START,
        DISPATCH_0F_RECORD_COUNT,
        "0f mnemonic map",
    )
    expanded_0f = validate_expansion(
        records_0f, mnemonics_0f, "0f dispatch"
    )
    inventory: dict[tuple[str, str], set[str]] = defaultdict(set)
    for item in expanded_0f:
        for opcode in item["expanded_opcodes"]:
            inventory[(opcode, item["mnemonic"])].add(item["group"])
    inventory_rows = [
        {
            "groups": sorted(groups),
            "mnemonic": mnemonic,
            "second_opcode": opcode,
        }
        for (opcode, mnemonic), groups in sorted(inventory.items())
    ]
    expected_inventory = {
        ("0x10", "TEST1"),
        ("0x11", "TEST1"),
        ("0x18", "TEST1"),
        ("0x19", "TEST1"),
        ("0x12", "CLR1"),
        ("0x13", "CLR1"),
        ("0x1a", "CLR1"),
        ("0x1b", "CLR1"),
        ("0x14", "SET1"),
        ("0x15", "SET1"),
        ("0x1c", "SET1"),
        ("0x1d", "SET1"),
        ("0x16", "NOT1"),
        ("0x17", "NOT1"),
        ("0x1e", "NOT1"),
        ("0x1f", "NOT1"),
        ("0x20", "ADD4S"),
        ("0x22", "SUB4S"),
        ("0x26", "CMP4S"),
        ("0x28", "ROL4"),
        ("0x2a", "ROR4"),
        ("0xfe", "BRKFEM"),
        ("0xff", "BRKEM"),
    }
    actual_inventory = {
        (item["second_opcode"], item["mnemonic"]) for item in inventory_rows
    }
    if actual_inventory != expected_inventory:
        raise M60bError("0f dispatch inventory differs from ROM evidence")
    forbidden = {"0x31", "0x33", "0x39", "0x3b"}
    if forbidden & {item["second_opcode"] for item in inventory_rows}:
        raise M60bError("0f31/33/39/3b presence contradiction")
    brk = {
        item["second_opcode"]: item["mnemonic"]
        for item in inventory_rows
        if item["second_opcode"] in {"0xfe", "0xff"}
    }
    if brk != {"0xfe": "BRKFEM", "0xff": "BRKEM"}:
        raise M60bError("BRKFEM/BRKEM encoding differs")

    records_main = parse_records(
        rom_data,
        DISPATCH_MAIN_START,
        DISPATCH_MAIN_END,
        DISPATCH_MAIN_RECORD_COUNT,
        "main dispatch",
    )
    mnemonics_main, main_mnemonic_end = decode_high_bit_strings(
        rom_data,
        DISPATCH_MAIN_MNEMONIC_START,
        DISPATCH_MAIN_RECORD_COUNT,
        "main dispatch mnemonic map",
        allow_empty=True,
    )
    if main_mnemonic_end != DISPATCH_MAIN_MNEMONIC_END:
        raise M60bError("main dispatch mnemonic boundary differs")
    expanded_main = validate_expansion(
        records_main,
        mnemonics_main,
        "main dispatch",
        allow_exact_duplicates=True,
        allow_mnemonic_aliases=True,
    )
    primary_union = {
        opcode
        for item in expanded_main
        for opcode in item["expanded_opcodes"]
    }
    absent = {f"0x{opcode:02x}" for opcode in AUTHORIZED_PRIMARY_OPCODES}
    if primary_union & absent:
        raise M60bError("main dispatch contains 6c-6f")
    main_inventory: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record, mnemonic, expanded in zip(
        records_main, mnemonics_main, expanded_main, strict=True
    ):
        for opcode in expanded["expanded_opcodes"]:
            main_inventory[opcode].append(
                {
                    "group": record["group"],
                    "mask": record["mask"],
                    "mnemonic": mnemonic["text"],
                    "record_index": record["index"],
                    "value": record["value"],
                }
            )
    main_inventory_rows = [
        {"entries": entries, "primary_opcode": opcode}
        for opcode, entries in sorted(main_inventory.items())
    ]
    control_mnemonics = {
        opcode: {
            entry["mnemonic"] for entry in main_inventory.get(opcode, [])
        }
        for opcode in ("0x64", "0x65", "0x68", "0x69", "0x6a", "0x6b")
    }
    if (
        "REPNC" not in control_mnemonics["0x64"]
        or "REPC" not in control_mnemonics["0x65"]
        or any(
            "PUSH" not in control_mnemonics[opcode]
            for opcode in ("0x68", "0x6a")
        )
        or any(
            "IMUL" not in control_mnemonics[opcode]
            for opcode in ("0x69", "0x6b")
        )
    ):
        raise M60bError("main dispatch control mappings differ")

    records_group = parse_records(
        rom_data,
        DISPATCH_GROUP_START,
        DISPATCH_GROUP_END,
        DISPATCH_GROUP_RECORD_COUNT,
        "primary group dispatch",
    )
    expanded_group = validate_expansion(
        records_group,
        None,
        "primary group dispatch",
        allow_exact_duplicates=True,
    )

    main_decoder = rom_data[0x6592C:0x6598D]
    group_decoder = rom_data[0x6598D:0x659D3]
    extension_decoder = rom_data[0x659D3:0x659E9]
    if (
        b"\xbf\x50\x63" not in main_decoder
        or b"\x8b\x36\x47\x08" not in main_decoder
        or b"\x8b\x1e\x49\x08" not in main_decoder
    ):
        raise M60bError("main dispatch boundary proof differs")
    if b"\xbf\xfc\x68\x47" not in group_decoder:
        raise M60bError("primary group dispatch start proof differs")
    if b"\xbf\x8b\x6a" not in extension_decoder:
        raise M60bError("0f dispatch start proof differs")
    pointer_metadata = rom_data[0x66F76:0x66F84]
    if pointer_metadata != bytes.fromhex(
        "1465f46420692169cc69b46ab56a"
    ):
        raise M60bError("dispatch table boundary metadata differs")

    raw_0f = table_document(
        records_0f,
        DISPATCH_0F_START,
        DISPATCH_0F_END,
        "v30-side-0f",
    )
    raw_0f["nominal_address_byte"] = (
        f"0x{rom_data[DISPATCH_0F_NOMINAL_OFFSET]:02x}"
    )
    raw_0f["nominal_address_note"] = (
        "The specification's 0x66a8a address is the high-bit terminator "
        "of the preceding DS0 string; the first record begins at 0x66a8b."
    )
    raw_0f["pointer_boundary_evidence"] = {
        "file_offset": "0x66f80",
        "mnemonic_start_minus_one": "0x6ab4",
        "record_end_exclusive": "0x6ab5",
    }
    raw_0f["raw_slice_sha256"] = sha256_bytes(
        rom_data[DISPATCH_0F_START:DISPATCH_0F_END]
    )

    expanded_document = {
        "absent_second_opcodes": sorted(forbidden),
        "expanded_records": expanded_0f,
        "inventory": inventory_rows,
        "inventory_count": len(inventory_rows),
        "schema": "vaeg-upd9002-rom-dispatch-expanded-v1",
        "schema_version": 1,
    }
    mnemonic_document = {
        "decoded_end_exclusive": hex_offset(mnemonic_end),
        "encoding": "ASCII with bit 7 set on the final byte",
        "mnemonics": mnemonics_0f,
        "schema": "vaeg-upd9002-rom-mnemonic-map-v1",
        "schema_version": 1,
    }
    primary_document = {
        "absent_primary_opcodes": sorted(absent),
        "decoder_code": {
            "code_file_range": ["0x6592c", "0x6598d"],
            "code_sha256": sha256_bytes(main_decoder),
            "internal_record_start": "0x6350",
            "mnemonic_pointer_variable": "0x0847",
            "record_end_pointer_variable": "0x0849",
        },
        "end_exclusive": hex_offset(DISPATCH_MAIN_END),
        "expanded_records": expanded_main,
        "independent_present_controls": {
            opcode: sorted(values)
            for opcode, values in sorted(control_mnemonics.items())
        },
        "inventory": main_inventory_rows,
        "mnemonic_end_exclusive": hex_offset(main_mnemonic_end),
        "mnemonic_start": hex_offset(DISPATCH_MAIN_MNEMONIC_START),
        "mnemonics": mnemonics_main,
        "pointer_boundary_evidence": {
            "file_offset": "0x66f76",
            "mnemonic_start_minus_one": "0x6514",
            "record_end_exclusive": "0x64f4",
        },
        "raw_record_count": len(records_main),
        "raw_records": records_main,
        "raw_slice_sha256": sha256_bytes(
            rom_data[DISPATCH_MAIN_START:DISPATCH_MAIN_END]
        ),
        "schema": "vaeg-upd9002-rom-primary-dispatch-v1",
        "schema_version": 1,
        "start": hex_offset(DISPATCH_MAIN_START),
        "unresolved_fpo2_note": (
            "Primary 0x66 and 0x67 have no ordinary main-table entry, but "
            "M60b does not infer FPO2 support or absence and does not change "
            "their target policy. M60c owns the dispatch-path audit."
        ),
        "verified_complete_record_count": DISPATCH_MAIN_RECORD_COUNT,
    }
    primary_document["group_subdispatch"] = {
        "decoder_code_range": ["0x6598d", "0x659d3"],
        "decoder_code_sha256": sha256_bytes(group_decoder),
        "end_exclusive": hex_offset(DISPATCH_GROUP_END),
        "expanded_records": expanded_group,
        "pointer_boundary_evidence": {
            "file_offset": "0x66f7a",
            "record_end_exclusive": "0x6921",
            "record_start_minus_one": "0x6920",
        },
        "raw_record_count": len(records_group),
        "raw_records": records_group,
        "raw_slice_sha256": sha256_bytes(
            rom_data[DISPATCH_GROUP_START:DISPATCH_GROUP_END]
        ),
        "start": hex_offset(DISPATCH_GROUP_START),
    }
    source_provenance = {
        "authorization": (
            "The complete copyrighted ROM remains out of tree. M60b stores "
            "only minimal decoded table evidence, hashes, and provenance."
        ),
        "known_reference": {
            "crc32": ROM_CRC32,
            "sha1": ROM_SHA1,
            "source": "repository M18 MAME-compatible VA2 ROM identity",
        },
        "rom_role": "PC-88VA2 main varom00 monitor ROM",
        "rom_sha256": ROM_SHA256,
        "rom_size": ROM_SIZE,
        "schema": "vaeg-upd9002-rom-source-provenance-v1",
        "schema_version": 1,
    }
    rom_map = {
        "address_convention": (
            "Canonical addresses are ROM file offsets. In the monitor bank "
            "containing these tables, the internal 16-bit address equals "
            "file_offset - 0x60000."
        ),
        "bank_file_base": hex_offset(ROM_BANK_FILE_BASE),
        "main_dispatch_internal_range": ["0x6350", "0x64f4"],
        "main_dispatch_mnemonic_internal_range": ["0x6515", "0x66fa"],
        "primary_group_dispatch_internal_range": ["0x68fd", "0x6921"],
        "rom_file_size": ROM_SIZE,
        "schema": "vaeg-upd9002-rom-map-v1",
        "schema_version": 1,
        "v30_0f_dispatch_internal_range": ["0x6a8b", "0x6ab5"],
    }
    debugger_document = {
        "authorization": debugger_source["authorization"],
        "conclusion": (
            "Independent monitor/runtime reporting corroborates the encodings "
            "0f fe imm8 BRKFEM and 0f ff imm8 BRKEM. It does not establish "
            "BRKFEM destination, vector, return, or mode semantics."
        ),
        "observations": debugger_source["observations"],
        "provenance": debugger_source["provenance"],
        "schema": "vaeg-upd9002-debugger-evidence-v1",
        "schema_version": 1,
        "source_identity_sha256": identity_digest(debugger_source),
    }
    conclusions = {
        "claims": [
            {
                "claim": (
                    "The complete V30-side 0f monitor table has fourteen "
                    "raw records."
                ),
                "status": "proven",
            },
            {
                "claim": (
                    "The complete table has no entries for 0f31, 0f33, "
                    "0f39, or 0f3b."
                ),
                "status": "proven",
            },
            {
                "claim": (
                    "The complete primary monitor table has no entries "
                    "for 6c through 6f."
                ),
                "status": "proven",
            },
            {
                "claim": "BRKFEM is encoded as 0f fe imm8 and BRKEM as 0f ff imm8.",
                "status": "proven",
            },
            {
                "claim": "BRKFEM/BRKEM execution semantics are resolved.",
                "status": "underdetermined",
            },
            {
                "claim": (
                    "Generic FPO1/FPO2/ESC string presence or absence "
                    "establishes FPU opcode support."
                ),
                "status": "rejected_non_evidence",
            },
        ],
        "fpo_non_evidence": (
            "Generic FPO1, FPO2, or ESC strings are not target authority. "
            "The monitor stores individual 8087 mnemonics including FADD and "
            "FMUL and has D8-DF records; 66/67 remain for M60c."
        ),
        "schema": "vaeg-upd9002-rom-authority-conclusions-v1",
        "schema_version": 1,
    }
    return {
        "conclusions.json": conclusions,
        "debugger_evidence.json": debugger_document,
        "dispatch_0f_expanded.json": expanded_document,
        "dispatch_0f_raw.json": raw_0f,
        "dispatch_primary_6c_6f.json": primary_document,
        "mnemonic_map.json": mnemonic_document,
        "rom_map.json": rom_map,
        "source_provenance.json": source_provenance,
        "string_pool_audit.json": string_pool_audit(rom_data),
    }


def artifact_entry(path: pathlib.Path, relative: pathlib.Path) -> dict[str, Any]:
    value = read_json(path)
    row_count = 0
    for key in (
        "claims",
        "expanded_records",
        "inventory",
        "mnemonics",
        "observations",
        "patterns",
        "raw_records",
    ):
        item = value.get(key) if isinstance(value, dict) else None
        if isinstance(item, list):
            row_count += len(item)
    group = value.get("group_subdispatch") if isinstance(value, dict) else None
    if isinstance(group, dict):
        row_count += len(group.get("raw_records", []))
        row_count += len(group.get("expanded_records", []))
    return {
        "bytes": path.stat().st_size,
        "path": relative.as_posix(),
        "row_count": row_count,
        "sha256": sha256_file(path),
    }


def output_path(output_root: pathlib.Path, relative: pathlib.Path) -> pathlib.Path:
    return output_root / relative


def generate_authority_pack(
    root: pathlib.Path,
    output_root: pathlib.Path,
    rom_path: pathlib.Path,
    debugger_source_path: pathlib.Path,
) -> dict[str, Any]:
    rom_data = rom_path.read_bytes()
    debugger_source = read_json(debugger_source_path)
    documents = extract_authority_documents(rom_data, debugger_source)
    destination = output_path(output_root, AUTHORITY_ROOT)
    if destination.exists() and any(destination.iterdir()):
        raise M60bError(f"authority output is not empty: {destination}")
    destination.mkdir(parents=True, exist_ok=True)
    for name, value in sorted(documents.items()):
        write_json(destination / name, value)
    artifacts = [
        artifact_entry(destination / name, AUTHORITY_ROOT / name)
        for name in sorted(documents)
    ]
    raw_0f = documents["dispatch_0f_raw.json"]
    expanded = documents["dispatch_0f_expanded.json"]
    mnemonic = documents["mnemonic_map.json"]
    debugger = documents["debugger_evidence.json"]
    manifest = {
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifacts": artifacts,
        "candidate_gate": CANDIDATE_GATE,
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "debugger_evidence_sha256": identity_digest(debugger),
        "extraction_tool": {
            "path": "tools/qa/upd9002_m60b_authority.py",
            "version": 1,
        },
        "license": "BSD-2-Clause",
        "milestone": MILESTONE,
        "mnemonic_map_sha256": identity_digest(mnemonic),
        "raw_record_counts": {
            "primary_group": DISPATCH_GROUP_RECORD_COUNT,
            "primary_main": DISPATCH_MAIN_RECORD_COUNT,
            "v30_0f": DISPATCH_0F_RECORD_COUNT,
        },
        "rom_mapping": {
            "address_identity": "rom-file-offset",
            "bank_file_base": hex_offset(ROM_BANK_FILE_BASE),
        },
        "rom_sha256": ROM_SHA256,
        "rom_size": ROM_SIZE,
        "schema": "vaeg-upd9002-rom-authority-manifest-v1",
        "schema_version": 1,
        "string_pool": {
            "algorithm": "exact-high-bit-terminated-ascii-v1",
            "end_exclusive": hex_offset(STRING_POOL_END),
            "start": hex_offset(STRING_POOL_START),
        },
        "table_boundaries": {
            "primary_group_end_exclusive": hex_offset(DISPATCH_GROUP_END),
            "primary_group_start": hex_offset(DISPATCH_GROUP_START),
            "primary_main_end_exclusive": hex_offset(DISPATCH_MAIN_END),
            "primary_main_mnemonic_end_exclusive": hex_offset(
                DISPATCH_MAIN_MNEMONIC_END
            ),
            "primary_main_mnemonic_start": hex_offset(
                DISPATCH_MAIN_MNEMONIC_START
            ),
            "primary_main_start": hex_offset(DISPATCH_MAIN_START),
            "v30_0f_end_exclusive": raw_0f["end_exclusive"],
            "v30_0f_mnemonic_end_exclusive": mnemonic[
                "decoded_end_exclusive"
            ],
            "v30_0f_start": raw_0f["start"],
        },
        "v30_0f_expanded_opcode_count": expanded["inventory_count"],
    }
    validate_authority_pack_documents(documents, manifest)
    write_json(destination / "manifest.json", manifest)
    print(
        "m60b-authority: "
        f"rom={ROM_SHA256} records_0f={DISPATCH_0F_RECORD_COUNT} "
        f"records_primary={DISPATCH_MAIN_RECORD_COUNT} "
        f"records_primary_group={DISPATCH_GROUP_RECORD_COUNT} "
        f"manifest_sha256={sha256_file(destination / 'manifest.json')}"
    )
    return manifest


def validate_authority_pack_documents(
    documents: dict[str, dict[str, Any]], manifest: dict[str, Any]
) -> None:
    required_documents = {
        "conclusions.json",
        "debugger_evidence.json",
        "dispatch_0f_expanded.json",
        "dispatch_0f_raw.json",
        "dispatch_primary_6c_6f.json",
        "mnemonic_map.json",
        "rom_map.json",
        "source_provenance.json",
        "string_pool_audit.json",
    }
    if set(documents) != required_documents:
        raise M60bError("authority pack document family is incomplete")
    require_keys(
        manifest,
        {
            "approved_predecessor_gate",
            "approved_predecessor_sha",
            "artifacts",
            "candidate_gate",
            "copyright",
            "debugger_evidence_sha256",
            "extraction_tool",
            "license",
            "milestone",
            "mnemonic_map_sha256",
            "raw_record_counts",
            "rom_mapping",
            "rom_sha256",
            "rom_size",
            "schema",
            "schema_version",
            "string_pool",
            "table_boundaries",
            "v30_0f_expanded_opcode_count",
        },
        "authority manifest",
    )
    if (
        manifest["schema"] != "vaeg-upd9002-rom-authority-manifest-v1"
        or manifest["schema_version"] != 1
        or manifest["milestone"] != MILESTONE
        or manifest["candidate_gate"] != CANDIDATE_GATE
        or manifest["approved_predecessor_gate"]
        != APPROVED_PREDECESSOR_GATE
        or manifest["approved_predecessor_sha"] != APPROVED_PREDECESSOR_SHA
        or manifest["rom_sha256"] != ROM_SHA256
        or manifest["rom_size"] != ROM_SIZE
        or manifest["raw_record_counts"]
        != {
            "primary_group": DISPATCH_GROUP_RECORD_COUNT,
            "primary_main": DISPATCH_MAIN_RECORD_COUNT,
            "v30_0f": DISPATCH_0F_RECORD_COUNT,
        }
    ):
        raise M60bError("authority manifest identity differs")
    raw = documents["dispatch_0f_raw.json"]
    if (
        raw.get("raw_record_count") != DISPATCH_0F_RECORD_COUNT
        or raw.get("start") != hex_offset(DISPATCH_0F_START)
        or raw.get("end_exclusive") != hex_offset(DISPATCH_0F_END)
    ):
        raise M60bError("authority pack 0f table boundaries differ")
    expanded = documents["dispatch_0f_expanded.json"]
    inventory = expanded.get("inventory")
    validate_forbidden_0f_inventory(inventory)
    primary = documents["dispatch_primary_6c_6f.json"]
    if set(primary.get("absent_primary_opcodes", [])) != {
        "0x6c",
        "0x6d",
        "0x6e",
        "0x6f",
    }:
        raise M60bError("authority pack does not prove all 6c-6f absent")
    primary_inventory = primary.get("inventory")
    if (
        primary.get("raw_record_count") != DISPATCH_MAIN_RECORD_COUNT
        or primary.get("verified_complete_record_count")
        != DISPATCH_MAIN_RECORD_COUNT
        or primary.get("start") != hex_offset(DISPATCH_MAIN_START)
        or primary.get("end_exclusive") != hex_offset(DISPATCH_MAIN_END)
        or primary.get("mnemonic_start")
        != hex_offset(DISPATCH_MAIN_MNEMONIC_START)
        or primary.get("mnemonic_end_exclusive")
        != hex_offset(DISPATCH_MAIN_MNEMONIC_END)
        or len(primary.get("raw_records", []))
        != DISPATCH_MAIN_RECORD_COUNT
        or len(primary.get("mnemonics", []))
        != DISPATCH_MAIN_RECORD_COUNT
        or not isinstance(primary_inventory, list)
    ):
        raise M60bError("authority pack main dispatch boundaries differ")
    primary_opcodes = {
        item.get("primary_opcode") for item in primary_inventory
    }
    if {
        "0x6c",
        "0x6d",
        "0x6e",
        "0x6f",
    } & primary_opcodes:
        raise M60bError("authority pack main inventory contains 6c-6f")
    controls = primary.get("independent_present_controls", {})
    if (
        "REPNC" not in controls.get("0x64", [])
        or "REPC" not in controls.get("0x65", [])
        or any("PUSH" not in controls.get(opcode, []) for opcode in ("0x68", "0x6a"))
        or any("IMUL" not in controls.get(opcode, []) for opcode in ("0x69", "0x6b"))
    ):
        raise M60bError("authority pack main dispatch controls differ")
    group = primary.get("group_subdispatch")
    if (
        not isinstance(group, dict)
        or group.get("raw_record_count") != DISPATCH_GROUP_RECORD_COUNT
        or group.get("start") != hex_offset(DISPATCH_GROUP_START)
        or group.get("end_exclusive") != hex_offset(DISPATCH_GROUP_END)
    ):
        raise M60bError("authority pack group dispatch boundaries differ")
    debugger = documents["debugger_evidence.json"]
    if "0f fe imm8" not in debugger.get("conclusion", "").lower():
        raise M60bError("missing BRKFEM debugger corroboration")
    if "0f ff imm8" not in debugger.get("conclusion", "").lower():
        raise M60bError("missing BRKEM debugger corroboration")
    if manifest["debugger_evidence_sha256"] != identity_digest(debugger):
        raise M60bError("authority debugger evidence digest differs")
    if manifest["mnemonic_map_sha256"] != identity_digest(
        documents["mnemonic_map.json"]
    ):
        raise M60bError("authority mnemonic-map digest differs")


def load_and_validate_authority_pack(
    output_root: pathlib.Path,
) -> tuple[dict[str, Any], str]:
    destination = output_path(output_root, AUTHORITY_ROOT)
    manifest_path = destination / "manifest.json"
    manifest = read_json(manifest_path)
    documents = {}
    artifacts = manifest.get("artifacts", [])
    if (
        not isinstance(artifacts, list)
        or not all(isinstance(item, dict) for item in artifacts)
        or [item.get("path") for item in artifacts]
        != sorted(item.get("path") for item in artifacts)
        or len({item.get("path") for item in artifacts}) != len(artifacts)
    ):
        raise M60bError("authority artifact manifest is malformed")
    for item in artifacts:
        relative = pathlib.Path(item.get("path", ""))
        if (
            relative.is_absolute()
            or ".." in relative.parts
            or relative.parent != AUTHORITY_ROOT
        ):
            raise M60bError("authority artifact path is unsafe")
        path = output_path(output_root, relative)
        if (
            not path.is_file()
            or path.stat().st_size != item.get("bytes")
            or sha256_file(path) != item.get("sha256")
        ):
            raise M60bError(f"authority artifact differs: {relative}")
        value = read_json(path)
        if path.read_bytes() != canonical_bytes(value) + b"\n":
            raise M60bError(f"authority artifact is not canonical: {relative}")
        if artifact_entry(path, relative)["row_count"] != item.get(
            "row_count"
        ):
            raise M60bError(f"authority artifact row count differs: {relative}")
        documents[path.name] = value
    validate_authority_pack_documents(documents, manifest)
    if manifest_path.read_bytes() != canonical_bytes(manifest) + b"\n":
        raise M60bError("authority manifest is not canonical")
    return manifest, sha256_file(manifest_path)


def modify_support_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    result = copy.deepcopy(rows)
    changed = []
    for row in result:
        opcode = int(row["opcode"], 16)
        if (
            opcode in AUTHORIZED_PRIMARY_OPCODES
            and row["mode"] in {"v30op", "v30op_repe", "v30op_repne"}
        ):
            if row["classification"] != "implemented":
                raise M60bError("candidate overlay expected an applicable row")
            row["classification"] = "known_target_gap"
            row["target"] = "v30_reserved"
            row["basis"] = "g60b-rom-authority"
            changed.append((row["mode"], opcode, row["subopcode"]))
    expected = {
        (mode, opcode, "-")
        for mode in ("v30op", "v30op_repe", "v30op_repne")
        for opcode in AUTHORIZED_PRIMARY_OPCODES
    }
    if set(changed) != expected:
        raise M60bError("candidate overlay did not change exactly twelve rows")
    return result


def read_support_rows(path: pathlib.Path) -> tuple[list[str], list[dict[str, str]]]:
    with path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        rows = list(reader)
        fields = list(reader.fieldnames or [])
    if fields != [
        "mode",
        "opcode",
        "subopcode",
        "target",
        "classification",
        "basis",
    ]:
        raise M60bError("support map schema differs")
    return fields, rows


@contextlib.contextmanager
def candidate_support_map(root: pathlib.Path) -> Iterator[pathlib.Path]:
    old_path = root / OLD_SUPPORT_MAP
    fields, rows = read_support_rows(old_path)
    candidate = modify_support_rows(rows)
    with tempfile.TemporaryDirectory(
        prefix="vaeg-m60b-support-map-"
    ) as temporary:
        path = pathlib.Path(temporary) / "upd9002_support_map_g60b.csv"
        with path.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(
                stream, fieldnames=fields, lineterminator="\n"
            )
            writer.writeheader()
            writer.writerows(candidate)
        ssts.load_support_map(path)
        yield path


def decode_primary_opcode(instruction: list[int]) -> tuple[int, list[int]]:
    position = 0
    prefixes = set(ssts.SEGMENT_PREFIXES) | set(ssts.REPEAT_PREFIXES) | set(
        ssts.IGNORED_PREFIXES
    )
    observed = []
    while position < len(instruction) and instruction[position] in prefixes:
        observed.append(instruction[position])
        position += 1
    if position >= len(instruction):
        raise M60bError("instruction contains prefixes only")
    return instruction[position], observed


def run_profile(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    scope: str,
    profile: str,
    policy: str,
    output: pathlib.Path,
    failure_directory: pathlib.Path,
) -> dict[str, Any]:
    manifest = ssts.load_manifest(root / MANIFEST_PATH)
    flags = "defined" if profile == "architectural" else "all16"
    if profile == "fingerprint" and scope != "full":
        raise M60bError("fingerprint profile must use full scope")
    with candidate_support_map(root) as candidate:
        support_map = root / OLD_SUPPORT_MAP if policy == "g60a" else candidate
        result = ssts.run_profile(
            dataset_root,
            manifest,
            support_map,
            worker,
            scope,
            300.0,
            flags,
        )
    ssts.externalize_failure_signatures(result, failure_directory)
    write_json(output, result)
    print(
        "m60b-profile: "
        f"policy={policy} profile={profile} scope={scope} "
        f"selected={result['selected_records']} "
        f"executed={result['executed_records']} "
        f"pass={result['result_counts'].get('pass', 0)} "
        f"fail={result['failure_signature_count']}"
    )
    return result


def selected_target_records(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    scope: str,
    candidate_map: pathlib.Path,
) -> tuple[
    dict[str, dict[str, Any]],
    dict[str, dict[str, Any]],
    dict[str, Any],
]:
    manifest = ssts.load_manifest(root / MANIFEST_PATH)
    ssts.verify_fast(dataset_root, manifest)
    metadata = json.loads(
        (dataset_root / ssts.SUITE_PATH / "metadata.json").read_text(
            encoding="utf-8"
        )
    )
    ssts.validate_metadata(metadata)
    old_support = ssts.load_support_map(root / OLD_SUPPORT_MAP)
    new_support = ssts.load_support_map(candidate_map)
    by_record: dict[str, dict[str, Any]] = {}
    by_upstream: dict[str, dict[str, Any]] = {}
    prefix_classes: Counter[str] = Counter()
    target_paths = [
        path
        for path in ssts.corpus_files(dataset_root)
        if path.name.removesuffix(".json.gz").upper()
        in {"6C", "6D", "6E", "6F"}
    ]
    if {path.name.upper() for path in target_paths} != {
        "6C.JSON.GZ",
        "6D.JSON.GZ",
        "6E.JSON.GZ",
        "6F.JSON.GZ",
    }:
        raise M60bError("6c-6f structural corpus files are incomplete")
    for path in target_paths:
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            records = json.load(stream)
        form = path.name.removesuffix(".json.gz").upper()
        for raw_record in ssts.profile_records(records, scope):
            record = ssts.validate_record(
                raw_record, f"{path.name}:{raw_record.get('idx')}"
            )
            opcode, prefixes = decode_primary_opcode(record["bytes"])
            if opcode not in AUTHORIZED_PRIMARY_OPCODES:
                continue
            before = ssts.classify_record(
                form, record, metadata, old_support
            )
            after = ssts.classify_record(
                form, record, metadata, new_support
            )
            if before["dispatch"] is None or after["dispatch"] is None:
                raise M60bError("6c-6f selector lacks dispatch metadata")
            record_hash = sha256_bytes(canonical_bytes(record))
            repeat = after["dispatch"]["repeat_prefix"]
            prefix_class = (
                "unprefixed"
                if not prefixes
                else "+".join(f"{byte:02x}" for byte in prefixes)
            )
            item = {
                "after_classification": after["classification"],
                "after_gap_kind": "documented_silicon_absent",
                "before_classification": before["classification"],
                "before_gap_kind": (
                    "implementation_missing"
                    if before["classification"] == "known_target_gap"
                    else None
                ),
                "form": form,
                "instruction_bytes": "".join(
                    f"{byte:02x}" for byte in record["bytes"]
                ),
                "prefix_class": prefix_class,
                "primary_opcode": f"0x{opcode:02x}",
                "record_hash": record_hash,
                "repeat_prefix": repeat,
                "upstream_test_hash": record["hash"],
            }
            if record_hash in by_record or record["hash"] in by_upstream:
                raise M60bError("duplicate selected 6c-6f identity")
            by_record[record_hash] = item
            by_upstream[record["hash"]] = item
            prefix_classes[prefix_class] += 1
    expected = TARGET_SELECTED_COUNTS[scope]
    if len(by_record) != expected:
        raise M60bError(
            f"{scope}: incomplete 6c-6f selector coverage "
            f"{len(by_record)} != {expected}"
        )
    if any(
        item["after_classification"] != "known_target_gap"
        for item in by_record.values()
    ):
        raise M60bError(f"{scope}: selected 6c-6f record remains applicable")
    metadata_summary = {
        "prefix_class_counts": dict(sorted(prefix_classes.items())),
        "record_count": len(by_record),
        "record_hash_set_sha256": ratchet.hash_set_digest(by_record),
        "upstream_hash_set_sha256": ratchet.upstream_hash_set_digest(
            by_upstream
        ),
    }
    return by_record, by_upstream, metadata_summary


def canonicalize_known_gaps(
    known_gaps: dict[str, Any],
) -> dict[str, Any]:
    value = copy.deepcopy(known_gaps)
    for rule in value["rules"]:
        evidence = rule.get("evidence")
        if not isinstance(evidence, dict):
            raise M60bError("candidate known-gap evidence is malformed")
        evidence["support_map"] = (
            "derived:tools/qa/golden/upd9002_support_map_m48.csv"
            "+g60b-rom-authority-overlay"
        )
    return value


def old_gap_kind_by_selector(
    root: pathlib.Path,
) -> tuple[dict[str, str], dict[str, dict[str, Any]]]:
    old_gaps = read_json(root / OLD_KNOWN_GAPS_PATH)
    old_taxonomy = read_json(root / OLD_TAXONOMY_PATH)
    annotations = {
        item["selector_sha256"]: item["gap_kind"]
        for item in old_taxonomy["annotations"]
    }
    rules = {}
    for rule in old_gaps["rules"]:
        digest = ratchet.selector_digest(rule["selector"])
        rules[digest] = rule
        if digest not in annotations:
            raise M60bError("old taxonomy is incomplete")
    return annotations, rules


def protected_gap_state(
    known_gaps: dict[str, Any],
    taxonomy: dict[str, Any],
    forms: set[str],
) -> list[dict[str, Any]]:
    kinds = {
        item["selector_sha256"]: item["gap_kind"]
        for item in taxonomy["annotations"]
    }
    result = []
    for rule in known_gaps["rules"]:
        selector = rule["selector"]
        if selector["metadata_form"] not in forms:
            continue
        selector_sha256 = ratchet.selector_digest(selector)
        result.append(
            {
                "form": selector["metadata_form"],
                "gap_kind": kinds[selector_sha256],
                "resolved_count": rule["resolved_count"],
                "resolved_test_hashes_sha256": rule[
                    "resolved_test_hashes_sha256"
                ],
                "selector_sha256": selector_sha256,
            }
        )
    result.sort(key=lambda item: item["selector_sha256"])
    return result


def create_candidate_taxonomy(
    root: pathlib.Path,
    candidate_known_gaps: dict[str, Any],
    source_digest: str,
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    old_kinds, old_rules = old_gap_kind_by_selector(root)
    annotations = []
    changes = []
    for rule in candidate_known_gaps["rules"]:
        selector = rule["selector"]
        digest = ratchet.selector_digest(selector)
        form = selector["metadata_form"]
        opcode = int(selector["opcode"], 16)
        old_kind = old_kinds.get(digest)
        if opcode in AUTHORIZED_PRIMARY_OPCODES:
            kind = "documented_silicon_absent"
        elif form in AUTHORIZED_GAP_KIND_FORMS:
            kind = "documented_silicon_absent"
        elif old_kind is not None:
            kind = old_kind
        else:
            raise M60bError(
                f"unauthorized new candidate gap selector: {form}"
            )
        if form == "0F28" and kind != "implementation_missing":
            raise M60bError("0f28 gap kind changed")
        if opcode in {0x66, 0x67} and old_kind != kind:
            raise M60bError("66/67 gap kind changed")
        if (
            form.startswith("0F")
            and form in ROM_PRESENT_0F_FORMS
            and kind == "documented_silicon_absent"
        ):
            raise M60bError(
                f"ROM-present form {form} labelled silicon absent"
            )
        annotation = {
            "gap_kind": kind,
            "resolved_count": rule["resolved_count"],
            "resolved_test_hashes_sha256": rule[
                "resolved_test_hashes_sha256"
            ],
            "selector_sha256": digest,
        }
        annotations.append(annotation)
        if old_kind != kind:
            changes.append(
                {
                    "after_gap_kind": kind,
                    "before_gap_kind": old_kind,
                    "form": form,
                    "resolved_count": rule["resolved_count"],
                    "resolved_test_hashes_sha256": rule[
                        "resolved_test_hashes_sha256"
                    ],
                    "selector_sha256": digest,
                    "transition": (
                        "new_known_gap"
                        if digest not in old_rules
                        else "gap_kind_correction"
                    ),
                }
            )
    annotations.sort(key=lambda item: item["selector_sha256"])
    changes.sort(key=lambda item: item["selector_sha256"])
    taxonomy = {
        "annotations": annotations,
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": candidate_known_gaps["dataset_id"],
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-ssts-gap-taxonomy-v1",
        "schema_version": 1,
        "source_known_gaps_path": TARGET_POLICY_KNOWN_GAPS_PATH.as_posix(),
        "source_known_gaps_sha256": source_digest,
    }
    return taxonomy, changes


def classification_rule_summaries(
    candidate_known_gaps: dict[str, Any],
    old_kinds: dict[str, str],
) -> list[dict[str, Any]]:
    result = []
    for rule in candidate_known_gaps["rules"]:
        selector = rule["selector"]
        opcode = int(selector["opcode"], 16)
        if opcode not in AUTHORIZED_PRIMARY_OPCODES:
            continue
        digest = ratchet.selector_digest(selector)
        before_classification = (
            "known_target_gap" if digest in old_kinds else "applicable"
        )
        result.append(
            {
                "after_classification": "known_target_gap",
                "after_gap_kind": "documented_silicon_absent",
                "before_classification": before_classification,
                "before_gap_kind": old_kinds.get(digest),
                "form": selector["metadata_form"],
                "prefix_metadata": {
                    "lock_prefix_constraint": selector[
                        "lock_prefix_constraint"
                    ],
                    "repeat_prefix": selector["repeat_prefix"],
                    "segment_prefix_constraint": selector[
                        "segment_prefix_constraint"
                    ],
                },
                "resolved_count": rule["resolved_count"],
                "resolved_test_hashes": rule["resolved_test_hashes"],
                "resolved_test_hashes_sha256": rule[
                    "resolved_test_hashes_sha256"
                ],
                "selector": selector,
                "selector_sha256": digest,
            }
        )
    result.sort(key=lambda item: item["selector_sha256"])
    owned = set()
    for item in result:
        hashes = set(item["resolved_test_hashes"])
        if owned & hashes:
            raise M60bError("6c-6f selector overlap")
        owned |= hashes
    if len(owned) != TARGET_SELECTED_COUNTS["full"]:
        raise M60bError("6c-6f selector union is incomplete")
    return result


def normalized_retired_entry(
    item: dict[str, Any],
    result: str,
    failure: dict[str, Any] | None,
) -> dict[str, Any]:
    return {
        "form": item["form"],
        "prefix_class": item["prefix_class"],
        "previous_result": result,
        "previous_signature_sha256": (
            None if failure is None else failure["signature_sha256"]
        ),
        "primary_opcode": item["primary_opcode"],
        "record_hash": item["record_hash"],
        "repeat_prefix": item["repeat_prefix"],
        "upstream_test_hash": item["upstream_test_hash"],
    }


def validate_retired_entry(value: Any, expected_result: str) -> None:
    required = {
        "form",
        "prefix_class",
        "previous_result",
        "previous_signature_sha256",
        "primary_opcode",
        "record_hash",
        "repeat_prefix",
        "upstream_test_hash",
    }
    require_keys(value, required, "retired applicable entry")
    if expected_result not in {"pass", "failure"}:
        raise M60bError("retired expected result is malformed")
    if value["previous_result"] != expected_result:
        raise M60bError("retired applicable previous result differs")
    if value["primary_opcode"] not in {
        "0x6c",
        "0x6d",
        "0x6e",
        "0x6f",
    }:
        raise M60bError("retired applicable opcode is outside 6c-6f")
    require_sha256(value["record_hash"], "retired record hash")
    try:
        ratchet.require_sha1(
            value["upstream_test_hash"], "retired upstream hash"
        )
    except ratchet.RatchetError as error:
        raise M60bError(str(error)) from error
    signature = value["previous_signature_sha256"]
    if expected_result == "pass":
        if signature is not None:
            raise M60bError("retired pass has a failure signature")
    else:
        require_sha256(signature, "retired failure signature")


def validate_retirement_result_sets(
    retired_passes: set[str],
    retired_failures: set[str],
    candidate_passes: set[str],
    newly_passing: list[str],
    newly_failing: list[str],
) -> None:
    if retired_passes & retired_failures:
        raise M60bError("retired pass/failure sets overlap")
    if retired_passes & candidate_passes:
        raise M60bError("retired pass was counted as a candidate pass")
    if newly_passing or newly_failing:
        raise M60bError("denominator retirement was counted as progress")
    if retired_failures & set(newly_passing):
        raise M60bError("retired failure was counted as newly passing")


def write_partition_shards(
    entries: list[dict[str, Any]],
    schema: str,
    count_field: str,
    rows_field: str,
    dataset_id: str,
    scope: str,
    output_directory: pathlib.Path,
    canonical_directory: pathlib.Path,
) -> list[dict[str, Any]]:
    if output_directory.exists() and any(output_directory.iterdir()):
        raise M60bError(f"shard directory is not empty: {output_directory}")
    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for entry in entries:
        opcode = entry["primary_opcode"].removeprefix("0x")
        groups[opcode].append(entry)
    output_directory.mkdir(parents=True, exist_ok=True)
    shards = []
    for opcode, rows in sorted(groups.items()):
        rows.sort(key=lambda item: item["record_hash"])
        payload = {
            count_field: len(rows),
            "dataset_id": dataset_id,
            "primary_opcode": f"0x{opcode}",
            "schema": schema,
            "schema_version": 1,
            "scope": scope,
            rows_field: rows,
        }
        path = output_directory / f"{opcode}.json.gz"
        raw_digest, canonical_digest = ratchet.write_deterministic_gzip(
            path, payload
        )
        shards.append(
            {
                "canonical_sha256": canonical_digest,
                count_field: len(rows),
                "path": (canonical_directory / path.name).as_posix(),
                "sha256": raw_digest,
            }
        )
    return shards


def load_approved_scoreboard(
    root: pathlib.Path, profile: str, scope: str
) -> tuple[pathlib.Path, dict[str, Any], dict[str, dict[str, Any]]]:
    path = (
        root
        / f"tests/ssts/scoreboard/g60a_{profile}_{scope}.json"
    )
    value = read_json(path)
    failures = ratchet.load_scoreboard_failures(path, value)
    if (
        value["epoch_gate"] != "G60a"
        or value["evaluated_sha"] != G60A_EVALUATED_SHA
        or value["approved_predecessor_sha"]
        != "e7f2325bc81310532091a8ca82914030fdb8b6ba"
    ):
        raise M60bError("approved G60a scoreboard identity differs")
    return path, value, failures


def create_partition_artifacts(
    root: pathlib.Path,
    output_root: pathlib.Path,
    scope: str,
    target_records: dict[str, dict[str, Any]],
    enumeration: dict[str, Any],
) -> dict[str, Any]:
    _, before, before_failures = load_approved_scoreboard(
        root, "architectural", scope
    )
    retired = set(enumeration["before_sets"]["applicable"]) - set(
        enumeration["after_sets"]["applicable"]
    )
    changed = {
        item["record_hash"]
        for item in enumeration["classification_changes"]
    }
    if retired != changed:
        raise M60bError(f"{scope}: retired/classification-change sets differ")
    if not retired <= set(target_records):
        raise M60bError(f"{scope}: retired record outside 6c-6f selector")
    failure_set = set(before_failures)
    retired_failures = sorted(retired & failure_set)
    retired_passes = sorted(retired - failure_set)
    pass_entries = [
        normalized_retired_entry(
            target_records[item], "pass", None
        )
        for item in retired_passes
    ]
    failure_entries = [
        normalized_retired_entry(
            target_records[item], "failure", before_failures[item]
        )
        for item in retired_failures
    ]
    change_entries = [
        {
            "after_classification": "known_target_gap",
            "before_classification": "applicable",
            "form": target_records[item]["form"],
            "primary_opcode": target_records[item]["primary_opcode"],
            "record_hash": item,
            "repeat_prefix": target_records[item]["repeat_prefix"],
            "transition_kind": "target_authority_correction",
            "upstream_test_hash": target_records[item][
                "upstream_test_hash"
            ],
        }
        for item in sorted(retired)
    ]
    pass_dir_rel = pathlib.Path(
        f"tests/ssts/target_policy/g60b_retired_applicable_pass_{scope}"
    )
    failure_dir_rel = pathlib.Path(
        f"tests/ssts/target_policy/g60b_retired_applicable_failure_{scope}"
    )
    change_dir_rel = pathlib.Path(
        f"tests/ssts/target_policy/g60b_classification_changes_{scope}"
    )
    pass_shards = write_partition_shards(
        pass_entries,
        "vaeg-upd9002-retired-applicable-v1",
        "retired_count",
        "retired",
        before["dataset_id"],
        scope,
        output_path(output_root, pass_dir_rel),
        pass_dir_rel,
    )
    failure_shards = write_partition_shards(
        failure_entries,
        "vaeg-upd9002-retired-applicable-v1",
        "retired_count",
        "retired",
        before["dataset_id"],
        scope,
        output_path(output_root, failure_dir_rel),
        failure_dir_rel,
    )
    change_shards = write_partition_shards(
        change_entries,
        "vaeg-upd9002-classification-changes-v1",
        "classification_change_count",
        "classification_changes",
        before["dataset_id"],
        scope,
        output_path(output_root, change_dir_rel),
        change_dir_rel,
    )
    if len(retired_passes) + len(retired_failures) != len(retired):
        raise M60bError(f"{scope}: retired accounting is incomplete")
    if set(retired_passes) & set(retired_failures):
        raise M60bError(f"{scope}: retired pass/failure sets overlap")
    return {
        "classification_change_count": len(change_entries),
        "classification_change_hash_set_sha256": ratchet.hash_set_digest(
            retired
        ),
        "classification_change_shards": change_shards,
        "retired_applicable_failure_count": len(retired_failures),
        "retired_applicable_failure_hash_set_sha256": (
            ratchet.hash_set_digest(retired_failures)
        ),
        "retired_applicable_failure_shards": failure_shards,
        "retired_applicable_pass_count": len(retired_passes),
        "retired_applicable_pass_hash_set_sha256": ratchet.hash_set_digest(
            retired_passes
        ),
        "retired_applicable_pass_shards": pass_shards,
    }


def load_partition_rows(
    output_root: pathlib.Path,
    shards: list[dict[str, Any]],
    count_field: str,
    rows_field: str,
) -> list[dict[str, Any]]:
    result = []
    seen = set()
    for shard in shards:
        relative = pathlib.Path(shard["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise M60bError("partition shard path is unsafe")
        path = output_path(output_root, relative)
        if not path.is_file() or sha256_file(path) != shard["sha256"]:
            raise M60bError(f"partition shard differs: {relative}")
        payload = ratchet.read_deterministic_gzip(path)
        rows = payload.get(rows_field)
        if (
            not isinstance(rows, list)
            or payload.get(count_field) != len(rows)
            or shard.get(count_field) != len(rows)
        ):
            raise M60bError(f"partition shard count differs: {relative}")
        if sha256_bytes(canonical_bytes(payload) + b"\n") != shard[
            "canonical_sha256"
        ]:
            raise M60bError(f"partition canonical digest differs: {relative}")
        hashes = [item.get("record_hash") for item in rows]
        if (
            hashes != sorted(hashes)
            or len(hashes) != len(set(hashes))
            or seen & set(hashes)
        ):
            raise M60bError(f"partition shard ordering/ownership differs: {relative}")
        for record_hash in hashes:
            require_sha256(record_hash, "partition record hash")
        seen.update(hashes)
        result.extend(rows)
    return result


def historical_g43_reconciliation(
    root: pathlib.Path,
    output_root: pathlib.Path,
    full_partitions: dict[str, Any],
) -> dict[str, Any]:
    transition = read_json(
        root / "tests/ssts/baseline/v20_native_g43_transition.json"
    )
    profiles = {
        item["profile"]: item for item in transition.get("profiles", [])
    }
    full = profiles.get("full")
    if not isinstance(full, dict):
        raise M60bError("G43 full transition is missing")
    removed = sorted(
        item["record_hash"]
        for item in full["removed_from_semantic_failure"]
        if item["reason"] == "segment_override_outs_fixture_translation"
    )
    if len(removed) != 1204:
        raise M60bError("G43 fixture-fix pass population differs")
    pass_rows = load_partition_rows(
        output_root,
        full_partitions["retired_applicable_pass_shards"],
        "retired_count",
        "retired",
    )
    failure_rows = load_partition_rows(
        output_root,
        full_partitions["retired_applicable_failure_shards"],
        "retired_count",
        "retired",
    )
    retired_passes = {item["record_hash"] for item in pass_rows}
    intersection = sorted(set(removed) & retired_passes)
    changed_outs = sorted(
        item["record_hash"]
        for item in full["changed_failure_signatures"]
        if item["opcode_form"] == "6F"
    )
    if len(changed_outs) != 224:
        raise M60bError("G43 changed-signature OUTS population differs")
    failures_by_form: dict[str, list[str]] = defaultdict(list)
    for item in failure_rows:
        failures_by_form[item["form"]].append(item["record_hash"])
    for hashes in failures_by_form.values():
        hashes.sort()
    current_outs_failures = sorted(
        failures_by_form.get("6E", [])
        + failures_by_form.get("6F", [])
    )
    changed_intersection = sorted(
        set(changed_outs) & set(current_outs_failures)
    )
    unchanged_intersection = sorted(
        set(current_outs_failures) - set(changed_outs)
    )
    if (
        len(current_outs_failures) != 641
        or len(changed_intersection) != 224
        or len(unchanged_intersection) != 417
    ):
        raise M60bError("G43/G60a remaining OUTS reconciliation differs")
    return {
        "g43_fixture_fix_pass_count": len(removed),
        "g43_fixture_fix_pass_hash_set_sha256": ratchet.hash_set_digest(
            removed
        ),
        "g43_intersection_with_retired_pass_count": len(intersection),
        "g43_intersection_with_retired_pass_hash_set_sha256": (
            ratchet.hash_set_digest(intersection)
        ),
        "g43_remaining_outs_changed_signature_count": len(
            changed_intersection
        ),
        "g43_remaining_outs_changed_signature_hash_set_sha256": (
            ratchet.hash_set_digest(changed_intersection)
        ),
        "g43_remaining_outs_unchanged_signature_count": len(
            unchanged_intersection
        ),
        "g43_remaining_outs_unchanged_signature_hash_set_sha256": (
            ratchet.hash_set_digest(unchanged_intersection)
        ),
        "g60a_6e_retired_failure_count": len(
            failures_by_form.get("6E", [])
        ),
        "g60a_6e_retired_failure_hash_set_sha256": ratchet.hash_set_digest(
            failures_by_form.get("6E", [])
        ),
        "g60a_6f_retired_failure_count": len(
            failures_by_form.get("6F", [])
        ),
        "g60a_6f_retired_failure_hash_set_sha256": ratchet.hash_set_digest(
            failures_by_form.get("6F", [])
        ),
        "interpretation": (
            "The G43 OUTS fixture correction remains immutable V20 "
            "differential evidence. Its overlap with retired target-policy "
            "records is not uPD9002 semantic progress. The historical "
            "417/224 split describes unchanged versus changed failure "
            "signatures among 641 remaining OUTS failures; the exact G60a "
            "opcode-form accounting is recorded separately."
        ),
    }


def policy_identity_body(policy: dict[str, Any]) -> dict[str, Any]:
    excluded = {"target_policy_id", "target_policy_sha256"}
    return {key: value for key, value in policy.items() if key not in excluded}


def validate_selector_rules(
    selectors: Any, expected_resolved_count: int
) -> None:
    if not isinstance(selectors, list):
        raise M60bError("target policy selector rules are malformed")
    selector_ids = [item.get("selector_sha256") for item in selectors]
    if selector_ids != sorted(set(selector_ids)):
        raise M60bError("target policy selectors are nondeterministic")
    owned = set()
    for selector in selectors:
        if selector.get("after_gap_kind") != "documented_silicon_absent":
            raise M60bError("target policy selector has wrong gap kind")
        try:
            opcode = int(selector["selector"]["opcode"], 16)
        except (KeyError, TypeError, ValueError) as error:
            raise M60bError("target policy selector opcode is malformed") from error
        if opcode not in AUTHORIZED_PRIMARY_OPCODES:
            raise M60bError("opcode outside 6c-6f transitioned")
        hashes = selector.get("resolved_test_hashes")
        if (
            not isinstance(hashes, list)
            or hashes != sorted(set(hashes))
            or owned & set(hashes)
            or selector.get("resolved_count") != len(hashes)
            or selector.get("resolved_test_hashes_sha256")
            != ratchet.upstream_hash_set_digest(hashes)
        ):
            raise M60bError("target policy selector ownership differs")
        owned.update(hashes)
    if len(owned) != expected_resolved_count:
        raise M60bError("target policy selector coverage is incomplete")


def validate_target_policy(value: Any) -> None:
    required = {
        "applicable_hash_sets",
        "approved_predecessor_gate",
        "approved_predecessor_sha",
        "authority_manifest_sha256",
        "candidate_gate",
        "classification_selector_rules",
        "comparison_contracts",
        "copyright",
        "dataset_id",
        "epoch_gate",
        "evaluated_sha",
        "gap_kind_changes",
        "historical_g43_reconciliation",
        "license",
        "milestone",
        "predecessor_policy",
        "retired_applicable",
        "schema",
        "schema_version",
        "selected_hash_sets",
        "support_map_overlay",
        "target_policy_id",
        "target_policy_sha256",
        "taxonomy_counts",
        "transition_kind",
    }
    require_keys(value, required, "target policy")
    if (
        value["schema"] != "vaeg-upd9002-target-policy-v1"
        or value["schema_version"] != 1
        or value["milestone"] != MILESTONE
        or value["candidate_gate"] != CANDIDATE_GATE
        or value["epoch_gate"] != CANDIDATE_GATE
        or value["approved_predecessor_gate"]
        != APPROVED_PREDECESSOR_GATE
        or value["approved_predecessor_sha"] != APPROVED_PREDECESSOR_SHA
        or value["transition_kind"] != "target_authority_correction"
    ):
        raise M60bError("target policy identity differs")
    require_sha(value["evaluated_sha"], "target policy evaluated_sha")
    require_sha256(
        value["authority_manifest_sha256"], "authority manifest digest"
    )
    digest = identity_digest(policy_identity_body(value))
    if value["target_policy_sha256"] != digest:
        raise M60bError("target policy identity digest differs")
    if value["target_policy_id"] != f"upd9002-g60b-{digest}":
        raise M60bError("target policy ID differs")
    selectors = value["classification_selector_rules"]
    validate_selector_rules(selectors, TARGET_SELECTED_COUNTS["full"])
    gap_changes = value["gap_kind_changes"]
    if not isinstance(gap_changes, list):
        raise M60bError("target policy gap-kind changes are malformed")
    for item in gap_changes:
        form = item.get("form")
        if (
            form not in AUTHORIZED_GAP_KIND_FORMS
            and not (
                isinstance(form, str)
                and len(form) == 2
                and int(form, 16) in AUTHORIZED_PRIMARY_OPCODES
            )
        ):
            raise M60bError("unauthorized gap-kind change")
        if item.get("after_gap_kind") != "documented_silicon_absent":
            raise M60bError("target policy gap-kind correction differs")
    if any(
        item.get("form") == "0F28"
        for item in gap_changes
    ):
        raise M60bError("0f28 gap kind changed")
    for scope in ("ci", "full"):
        retired = value["retired_applicable"][scope]
        if (
            retired["retired_applicable_pass_count"]
            + retired["retired_applicable_failure_count"]
            != retired["classification_change_count"]
        ):
            raise M60bError(f"{scope}: retired accounting is incomplete")
    if value["support_map_overlay"]["changed_row_count"] != 12:
        raise M60bError("target policy overlay row count differs")


def generate_target_policy(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    evaluated_sha: str,
) -> dict[str, Any]:
    require_sha(evaluated_sha, "evaluated_sha")
    if evaluated_sha == APPROVED_PREDECESSOR_SHA:
        raise M60bError("current-worktree self-comparison is forbidden")
    _, authority_digest = load_and_validate_authority_pack(output_root)
    manifest = ssts.load_manifest(root / MANIFEST_PATH)
    architectural, architectural_digest = ratchet.load_contract(
        root / ARCH_CONTRACT_PATH
    )
    fingerprint, fingerprint_digest = ratchet.load_contract(
        root / FINGERPRINT_CONTRACT_PATH
    )
    with candidate_support_map(root) as candidate_map:
        _, candidate_known_gaps = ssts.classify_profile(
            dataset_root, manifest, candidate_map, "full"
        )
        candidate_known_gaps = canonicalize_known_gaps(
            candidate_known_gaps
        )
        known_gaps_path = output_path(
            output_root, TARGET_POLICY_KNOWN_GAPS_PATH
        )
        write_json(known_gaps_path, candidate_known_gaps)
        taxonomy, gap_kind_changes = create_candidate_taxonomy(
            root,
            candidate_known_gaps,
            sha256_file(known_gaps_path),
        )
        taxonomy_path = output_path(
            output_root, TARGET_POLICY_TAXONOMY_PATH
        )
        write_json(taxonomy_path, taxonomy)
        ratchet.validate_taxonomy(
            taxonomy,
            candidate_known_gaps,
            read_json(root / HARDWARE_PENDING_PATH),
            sha256_file(known_gaps_path),
        )
        old_kinds, _ = old_gap_kind_by_selector(root)
        selector_rules = classification_rule_summaries(
            candidate_known_gaps, old_kinds
        )
        enumerations = {}
        partitions = {}
        target_metadata = {}
        for scope in ("ci", "full"):
            enumeration = ratchet.enumerate_profiles(
                dataset_root,
                manifest,
                root / OLD_SUPPORT_MAP,
                candidate_map,
                scope,
            )
            target_records, _, metadata_summary = selected_target_records(
                root, dataset_root, scope, candidate_map
            )
            enumerations[scope] = enumeration
            target_metadata[scope] = metadata_summary
            partitions[scope] = create_partition_artifacts(
                root,
                output_root,
                scope,
                target_records,
                enumeration,
            )
        candidate_map_digest = sha256_file(candidate_map)

    old_taxonomy = read_json(root / OLD_TAXONOMY_PATH)
    old_gaps = read_json(root / OLD_KNOWN_GAPS_PATH)
    _, _, old_hash_counts = ratchet.validate_taxonomy(
        old_taxonomy,
        old_gaps,
        read_json(root / HARDWARE_PENDING_PATH),
        sha256_file(root / OLD_KNOWN_GAPS_PATH),
    )
    _, _, new_hash_counts = ratchet.validate_taxonomy(
        taxonomy,
        candidate_known_gaps,
        read_json(root / HARDWARE_PENDING_PATH),
        sha256_file(
            output_path(output_root, TARGET_POLICY_KNOWN_GAPS_PATH)
        ),
    )
    predecessor_body = {
        "approved_gate": "G60a",
        "approved_sha": APPROVED_PREDECESSOR_SHA,
        "approved_target_divergences_sha256": sha256_file(
            root / APPROVED_DIVERGENCES_PATH
        ),
        "gap_taxonomy_sha256": sha256_file(root / OLD_TAXONOMY_PATH),
        "hardware_pending_sha256": sha256_file(
            root / HARDWARE_PENDING_PATH
        ),
        "known_gaps_sha256": sha256_file(root / OLD_KNOWN_GAPS_PATH),
        "support_map_sha256": sha256_file(root / OLD_SUPPORT_MAP),
    }
    predecessor_digest = identity_digest(predecessor_body)
    reconciliation = historical_g43_reconciliation(
        root, output_root, partitions["full"]
    )
    full_changes = load_partition_rows(
        output_root,
        partitions["full"]["classification_change_shards"],
        "classification_change_count",
        "classification_changes",
    )
    full_passes = load_partition_rows(
        output_root,
        partitions["full"]["retired_applicable_pass_shards"],
        "retired_count",
        "retired",
    )
    full_failures = load_partition_rows(
        output_root,
        partitions["full"]["retired_applicable_failure_shards"],
        "retired_count",
        "retired",
    )
    old_rows = read_support_rows(root / OLD_SUPPORT_MAP)[1]
    candidate_rows = modify_support_rows(old_rows)
    old_66_67 = [
        row for row in old_rows if int(row["opcode"], 16) in {0x66, 0x67}
    ]
    candidate_66_67 = [
        row
        for row in candidate_rows
        if int(row["opcode"], 16) in {0x66, 0x67}
    ]
    enforce_authority_correction_contract(
        {
            "authority_manifest_sha256": authority_digest,
            "classification_changes": full_changes,
            "comparison_contract_after": (
                architectural["comparison_contract_id"],
                architectural_digest,
            ),
            "comparison_contract_before": (
                architectural["comparison_contract_id"],
                architectural_digest,
            ),
            "dataset_after": manifest["dataset_id"],
            "dataset_before": manifest["dataset_id"],
            "gap_kind": "documented_silicon_absent",
            "protected_0f28_unchanged": protected_gap_state(
                old_gaps, old_taxonomy, {"0F28"}
            )
            == protected_gap_state(
                candidate_known_gaps, taxonomy, {"0F28"}
            ),
            "protected_66_67_unchanged": (
                old_66_67 == candidate_66_67
                and protected_gap_state(
                    old_gaps, old_taxonomy, {"66", "67"}
                )
                == protected_gap_state(
                    candidate_known_gaps, taxonomy, {"66", "67"}
                )
            ),
            "retired_failure_hashes": [
                item["record_hash"] for item in full_failures
            ],
            "retired_pass_hashes": [
                item["record_hash"] for item in full_passes
            ],
            "selected_after": enumerations["full"][
                "selected_hash_set_sha256"
            ],
            "selected_before": enumerations["full"][
                "selected_hash_set_sha256"
            ],
            "transition_kind": "target_authority_correction",
        }
    )
    policy = {
        "applicable_hash_sets": {
            scope: {
                "after_sha256": enumerations[scope][
                    "after_set_digests"
                ]["applicable"],
                "before_sha256": enumerations[scope][
                    "before_set_digests"
                ]["applicable"],
            }
            for scope in ("ci", "full")
        },
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "authority_manifest_sha256": authority_digest,
        "candidate_gate": CANDIDATE_GATE,
        "classification_selector_rules": selector_rules,
        "comparison_contracts": {
            "architectural": {
                "id": architectural["comparison_contract_id"],
                "sha256": architectural_digest,
            },
            "fingerprint": {
                "id": fingerprint["comparison_contract_id"],
                "sha256": fingerprint_digest,
            },
        },
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": manifest["dataset_id"],
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": gap_kind_changes,
        "historical_g43_reconciliation": reconciliation,
        "license": "BSD-2-Clause",
        "milestone": MILESTONE,
        "predecessor_policy": {
            "derivation": predecessor_body,
            "target_policy_id": (
                f"upd9002-g60a-derived-{predecessor_digest}"
            ),
            "target_policy_sha256": predecessor_digest,
        },
        "retired_applicable": partitions,
        "schema": "vaeg-upd9002-target-policy-v1",
        "schema_version": 1,
        "selected_hash_sets": {
            scope: enumerations[scope]["selected_hash_set_sha256"]
            for scope in ("ci", "full")
        },
        "support_map_overlay": {
            "after_sha256": candidate_map_digest,
            "authorized_primary_opcodes": [
                f"0x{opcode:02x}"
                for opcode in sorted(AUTHORIZED_PRIMARY_OPCODES)
            ],
            "before_path": OLD_SUPPORT_MAP.as_posix(),
            "before_sha256": sha256_file(root / OLD_SUPPORT_MAP),
            "changed_row_count": 12,
            "selector_coverage": target_metadata,
        },
        "target_policy_id": "",
        "target_policy_sha256": "",
        "taxonomy_counts": {
            "after": dict(sorted(new_hash_counts.items())),
            "before": dict(sorted(old_hash_counts.items())),
        },
        "transition_kind": "target_authority_correction",
    }
    digest = identity_digest(policy_identity_body(policy))
    policy["target_policy_id"] = f"upd9002-g60b-{digest}"
    policy["target_policy_sha256"] = digest
    validate_target_policy(policy)
    write_json(output_path(output_root, TARGET_POLICY_PATH), policy)
    print(
        "m60b-target-policy: "
        f"id={policy['target_policy_id']} "
        f"retired_full="
        f"{partitions['full']['classification_change_count']} "
        f"retired_ci={partitions['ci']['classification_change_count']}"
    )
    return policy


@contextlib.contextmanager
def configured_ratchet_identity() -> Iterator[None]:
    before = (
        ratchet.APPROVED_PREDECESSOR_GATE,
        ratchet.APPROVED_PREDECESSOR_SHA,
        ratchet.EPOCH_GATE,
    )
    ratchet.APPROVED_PREDECESSOR_GATE = APPROVED_PREDECESSOR_GATE
    ratchet.APPROVED_PREDECESSOR_SHA = APPROVED_PREDECESSOR_SHA
    ratchet.EPOCH_GATE = CANDIDATE_GATE
    try:
        yield
    finally:
        (
            ratchet.APPROVED_PREDECESSOR_GATE,
            ratchet.APPROVED_PREDECESSOR_SHA,
            ratchet.EPOCH_GATE,
        ) = before


def validate_scoreboard_v2(value: Any) -> None:
    if not isinstance(value, dict):
        raise M60bError("G60b scoreboard must be an object")
    required_extra = {"target_policy_id", "target_policy_sha256"}
    if not required_extra <= set(value):
        raise M60bError("G60b scoreboard lacks target-policy identity")
    require_sha256(
        value["target_policy_sha256"], "scoreboard target-policy digest"
    )
    if not isinstance(value["target_policy_id"], str) or not value[
        "target_policy_id"
    ].startswith("upd9002-g60b-"):
        raise M60bError("scoreboard target-policy ID differs")
    base = copy.deepcopy(value)
    del base["target_policy_id"]
    del base["target_policy_sha256"]
    if base.get("schema") != "vaeg-upd9002-ssts-scoreboard-v2":
        raise M60bError("G60b scoreboard schema differs")
    if base.get("schema_version") != 2:
        raise M60bError("G60b scoreboard version differs")
    base["schema"] = "vaeg-upd9002-ssts-scoreboard-v1"
    base["schema_version"] = 1
    with configured_ratchet_identity():
        try:
            ratchet.validate_scoreboard(base)
        except ratchet.RatchetError as error:
            raise M60bError(str(error)) from error


def scoreboard_v1_view(value: dict[str, Any]) -> dict[str, Any]:
    base = copy.deepcopy(value)
    base.pop("target_policy_id", None)
    base.pop("target_policy_sha256", None)
    if base.get("schema") == "vaeg-upd9002-ssts-scoreboard-v2":
        base["schema"] = "vaeg-upd9002-ssts-scoreboard-v1"
        base["schema_version"] = 1
    return base


def generate_candidate_scoreboard(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    raw_summary_path: pathlib.Path,
    profile: str,
    scope: str,
    evaluated_sha: str,
    output_relative: pathlib.Path,
    failure_directory_relative: pathlib.Path,
) -> dict[str, Any]:
    require_sha(evaluated_sha, "evaluated_sha")
    if evaluated_sha == APPROVED_PREDECESSOR_SHA:
        raise M60bError("current-worktree self-comparison is forbidden")
    if profile not in {"architectural", "fingerprint"}:
        raise M60bError("unknown profile")
    if scope not in {"ci", "full"} or (
        profile == "fingerprint" and scope != "full"
    ):
        raise M60bError("invalid profile/scope")
    immutable = ratchet.verify_immutable_m43(
        root, root / G43_MANIFEST_PATH
    )
    contract_path = (
        root / ARCH_CONTRACT_PATH
        if profile == "architectural"
        else root / FINGERPRINT_CONTRACT_PATH
    )
    contract, contract_digest = ratchet.load_contract(contract_path)
    manifest = ssts.load_manifest(root / MANIFEST_PATH)
    policy = read_json(output_path(output_root, TARGET_POLICY_PATH))
    validate_target_policy(policy)
    if (
        policy["evaluated_sha"] != evaluated_sha
        or policy["dataset_id"] != manifest["dataset_id"]
    ):
        raise M60bError("scoreboard and target-policy identity differ")
    candidate_known_gaps_path = output_path(
        output_root, TARGET_POLICY_KNOWN_GAPS_PATH
    )
    candidate_taxonomy_path = output_path(
        output_root, TARGET_POLICY_TAXONOMY_PATH
    )
    candidate_known_gaps = read_json(candidate_known_gaps_path)
    candidate_taxonomy = read_json(candidate_taxonomy_path)
    ratchet.validate_taxonomy(
        candidate_taxonomy,
        candidate_known_gaps,
        read_json(root / HARDWARE_PENDING_PATH),
        sha256_file(candidate_known_gaps_path),
    )
    ratchet.validate_content_registry(
        read_json(root / APPROVED_DIVERGENCES_PATH),
        "approved-target-divergences",
    )
    with candidate_support_map(root) as candidate_map:
        enumeration = ratchet.enumerate_profiles(
            dataset_root,
            manifest,
            root / OLD_SUPPORT_MAP,
            candidate_map,
            scope,
        )
    raw_summary = read_json(raw_summary_path)
    if (
        raw_summary.get("schema") != "vaeg-upd9002-ssts-result-v1"
        or raw_summary.get("dataset_id") != manifest["dataset_id"]
        or raw_summary.get("profile") != scope
    ):
        raise M60bError("raw candidate summary identity differs")
    if profile == "architectural":
        if "flags_comparison" in raw_summary:
            raise M60bError("architectural raw summary uses fingerprint FLAGS")
    elif raw_summary.get("flags_comparison") != "all16":
        raise M60bError("fingerprint raw summary lacks all16 FLAGS")
    failures_raw = ratchet.load_failure_records(raw_summary_path)
    failures = {
        record_hash: ratchet.failure_entry(failure)
        for record_hash, failure in failures_raw.items()
    }
    applicable_hashes = enumeration["after_sets"]["applicable"]
    applicable_set = set(applicable_hashes)
    if not set(failures) <= applicable_set:
        raise M60bError("candidate failure lies outside applicable set")
    pass_hashes = sorted(applicable_set - set(failures))
    rows = ratchet.build_scoreboard_rows(
        raw_summary, enumeration["after_form_counts"], failures_raw
    )
    failure_directory = output_path(
        output_root, failure_directory_relative
    )
    (
        failure_shards,
        failure_index,
        canonical_sidecars,
        raw_sidecars,
    ) = ratchet.write_failure_shards(
        failures_raw,
        profile,
        scope,
        manifest["dataset_id"],
        failure_directory,
    )
    if failure_index != raw_summary["failure_signature_index_sha256"]:
        raise M60bError("candidate failure index differs from raw result")
    classification_counts = {
        key: len(enumeration["after_sets"][key])
        for key in ratchet.TOP_LEVEL_CLASSIFICATIONS
    }
    raw_classifications = {
        key: value
        for key, value in raw_summary["classification_counts"].items()
        if value
    }
    if raw_classifications != {
        key: value
        for key, value in classification_counts.items()
        if value
    }:
        raise M60bError("candidate raw classification population differs")
    result_counts = raw_summary["result_counts"]
    passed = ratchet.require_count(result_counts.get("pass", 0), "pass")
    failed = sum(
        ratchet.require_count(result_counts.get(kind, 0), kind)
        for kind in ("semantic_failure", "timeout", "crash")
    )
    if passed != len(pass_hashes) or failed != len(failures):
        raise M60bError("candidate raw result arithmetic differs")
    mismatch_classes: Counter[str] = Counter()
    for failure in failures.values():
        mismatch_classes.update(failure["mismatch_classes"])
    m43_ci = immutable["profiles"]["ci"]
    m43_full = immutable["profiles"]["full"]
    summary = {
        "applicable": len(applicable_hashes),
        "applicable_hash_set_sha256": ratchet.hash_set_digest(
            applicable_hashes
        ),
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "blocking": profile == "architectural",
        "classification_counts": classification_counts,
        "classification_hash_sets": enumeration["after_set_digests"],
        "comparison_contract_id": contract["comparison_contract_id"],
        "comparison_contract_sha256": contract_digest,
        "crashes": ratchet.require_count(
            result_counts.get("crash", 0), "crashes"
        ),
        "dataset_id": manifest["dataset_id"],
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "executed": raw_summary["executed_records"],
        "fail": failed,
        "failure_hash_set_sha256": ratchet.hash_set_digest(failures),
        "failure_shards": failure_shards,
        "failure_sidecar_canonical_set_sha256": canonical_sidecars,
        "failure_sidecar_raw_set_sha256": raw_sidecars,
        "failure_signature_index_sha256": failure_index,
        "immutable_m43_ci_failure_index_sha256": m43_ci[
            "failure_index_sha256"
        ],
        "immutable_m43_ci_summary_sha256": m43_ci["summary_sha256"],
        "immutable_m43_full_failure_index_sha256": m43_full[
            "failure_index_sha256"
        ],
        "immutable_m43_full_summary_sha256": m43_full[
            "summary_sha256"
        ],
        "mismatch_classes": dict(sorted(mismatch_classes.items())),
        "pass": passed,
        "pass_hash_set_sha256": ratchet.hash_set_digest(pass_hashes),
        "profile": profile,
        "raw_result_summary_sha256": sha256_file(raw_summary_path),
        "records": rows,
        "schema": "vaeg-upd9002-ssts-scoreboard-v2",
        "schema_version": 2,
        "scope": scope,
        "scoreboard_digest": sha256_bytes(canonical_bytes(rows)),
        "selected": enumeration["selected_count"],
        "selected_hash_set_sha256": enumeration[
            "selected_hash_set_sha256"
        ],
        "target_policy_id": policy["target_policy_id"],
        "target_policy_sha256": policy["target_policy_sha256"],
        "termination_classes": raw_summary["termination_counts"],
        "timeouts": ratchet.require_count(
            result_counts.get("timeout", 0), "timeouts"
        ),
    }
    validate_scoreboard_v2(summary)
    expected_policy_applicable = policy["applicable_hash_sets"][scope][
        "after_sha256"
    ]
    if summary["applicable_hash_set_sha256"] != expected_policy_applicable:
        raise M60bError("scoreboard applicable set differs from target policy")
    if summary["selected_hash_set_sha256"] != policy[
        "selected_hash_sets"
    ][scope]:
        raise M60bError("scoreboard selected set differs from target policy")
    if summary["selected_hash_set_sha256"] != immutable["profiles"][scope][
        "selected_hash_set_sha256"
    ]:
        raise M60bError("scoreboard selected set differs from immutable M43")
    output = output_path(output_root, output_relative)
    write_json(output, summary)
    print(
        "m60b-scoreboard: "
        f"profile={profile} scope={scope} selected={summary['selected']} "
        f"applicable={summary['applicable']} pass={summary['pass']} "
        f"fail={summary['fail']}"
    )
    return summary


def verify_old_policy_raw(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    raw_summary_path: pathlib.Path,
    profile: str,
    scope: str,
) -> dict[str, Any]:
    manifest = ssts.load_manifest(root / MANIFEST_PATH)
    _, approved, approved_failures = load_approved_scoreboard(
        root, profile, scope
    )
    raw = read_json(raw_summary_path)
    if (
        raw.get("schema") != "vaeg-upd9002-ssts-result-v1"
        or raw.get("dataset_id") != manifest["dataset_id"]
        or raw.get("profile") != scope
    ):
        raise M60bError("old-policy raw summary identity differs")
    if profile == "architectural" and "flags_comparison" in raw:
        raise M60bError("old architectural run used fingerprint FLAGS")
    if profile == "fingerprint" and raw.get("flags_comparison") != "all16":
        raise M60bError("old fingerprint run did not compare all FLAGS")
    enumeration = ratchet.enumerate_profiles(
        dataset_root,
        manifest,
        root / OLD_SUPPORT_MAP,
        root / OLD_SUPPORT_MAP,
        scope,
    )
    if enumeration["classification_changes"]:
        raise M60bError("old-policy self-enumeration changed classification")
    raw_failures_source = ratchet.load_failure_records(raw_summary_path)
    raw_failures = {
        key: ratchet.failure_entry(value)
        for key, value in raw_failures_source.items()
    }
    if raw_failures != approved_failures:
        raise M60bError("old-policy failure hashes/signatures differ from G60a")
    rows = ratchet.build_scoreboard_rows(
        raw, enumeration["after_form_counts"], raw_failures_source
    )
    if rows != approved["records"]:
        raise M60bError("old-policy per-form results differ from G60a")
    applicable = set(enumeration["after_sets"]["applicable"])
    passes = sorted(applicable - set(raw_failures))
    result_counts = raw["result_counts"]
    checks = {
        "applicable": len(applicable),
        "applicable_hash_set_sha256": ratchet.hash_set_digest(applicable),
        "classification_hash_sets": enumeration["after_set_digests"],
        "crashes": result_counts.get("crash", 0),
        "executed": raw["executed_records"],
        "fail": len(raw_failures),
        "failure_hash_set_sha256": ratchet.hash_set_digest(raw_failures),
        "failure_signature_index_sha256": raw[
            "failure_signature_index_sha256"
        ],
        "pass": result_counts.get("pass", 0),
        "pass_hash_set_sha256": ratchet.hash_set_digest(passes),
        "selected": raw["selected_records"],
        "selected_hash_set_sha256": enumeration[
            "selected_hash_set_sha256"
        ],
        "termination_classes": raw["termination_counts"],
        "timeouts": result_counts.get("timeout", 0),
    }
    for field, observed in checks.items():
        if observed != approved[field]:
            raise M60bError(f"old-policy G60a reproduction differs: {field}")
    return {
        "failure_hash_set_sha256": checks["failure_hash_set_sha256"],
        "failure_signature_index_sha256": checks[
            "failure_signature_index_sha256"
        ],
        "pass": checks["pass"],
        "pass_hash_set_sha256": checks["pass_hash_set_sha256"],
        "fail": checks["fail"],
        "profile": profile,
        "scope": scope,
        "status": "exact",
    }


def compact_selector_changes(
    policy: dict[str, Any],
    changes: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    ownership: dict[str, dict[str, Any]] = {}
    for rule in policy["classification_selector_rules"]:
        if rule["before_classification"] != "applicable":
            continue
        for upstream_hash in rule["resolved_test_hashes"]:
            if upstream_hash in ownership:
                raise M60bError(
                    "applicable selector ownership overlaps"
                )
            ownership[upstream_hash] = rule

    grouped: dict[str, list[str]] = defaultdict(list)
    seen: set[str] = set()
    for change in changes:
        upstream_hash = change["upstream_test_hash"]
        if upstream_hash in seen:
            raise M60bError("duplicate scoped classification change")
        seen.add(upstream_hash)
        rule = ownership.get(upstream_hash)
        if (
            rule is None
            or rule["form"] != change["form"]
            or rule["selector"]["repeat_prefix"]
            != change["repeat_prefix"]
        ):
            raise M60bError(
                "scoped classification change has no selector owner"
            )
        grouped[rule["selector_sha256"]].append(upstream_hash)

    result = []
    for rule in policy["classification_selector_rules"]:
        selector_sha256 = rule["selector_sha256"]
        hashes = sorted(grouped.get(selector_sha256, []))
        if not hashes:
            continue
        result.append(
            {
                "after": "known_target_gap",
                "before": "applicable",
                "form": rule["form"],
                "resolved_count": len(hashes),
                "resolved_test_hashes_sha256": (
                    ratchet.upstream_hash_set_digest(hashes)
                ),
                "selector_sha256": selector_sha256,
            }
        )
    result.sort(key=lambda item: item["selector_sha256"])
    if sum(item["resolved_count"] for item in result) != len(changes):
        raise M60bError("scoped selector summary is incomplete")
    return result


def validate_target_authority_transition(
    root: pathlib.Path,
    output_root: pathlib.Path,
    value: Any,
) -> None:
    required = {
        "applicable_hash_set_after_sha256",
        "applicable_hash_set_before_sha256",
        "authority_manifest_sha256",
        "before_gate",
        "before_sha",
        "candidate_applicable_count",
        "candidate_failure_count",
        "candidate_pass_count",
        "changed_failure_count",
        "changed_failure_shards",
        "classification_change_count",
        "classification_change_shards",
        "classification_changes",
        "comparison_contract_id",
        "comparison_contract_sha256",
        "dataset_id",
        "epoch_gate",
        "evaluated_sha",
        "gap_kind_changes",
        "newly_failing",
        "newly_passing",
        "predecessor_applicable_count",
        "predecessor_failure_count",
        "predecessor_pass_count",
        "profile",
        "retired_applicable_failure_count",
        "retired_applicable_failure_hash_set_sha256",
        "retired_applicable_failure_shards",
        "retired_applicable_pass_count",
        "retired_applicable_pass_hash_set_sha256",
        "retired_applicable_pass_shards",
        "schema",
        "schema_version",
        "scope",
        "scoreboard_after_digest",
        "scoreboard_before_digest",
        "selected_hash_set_sha256",
        "target_policy_after_id",
        "target_policy_after_sha256",
        "target_policy_before_id",
        "target_policy_before_sha256",
        "taxonomy_counts_after",
        "taxonomy_counts_before",
        "transition_kind",
        "unaffected_applicable_failure_hash_set_sha256",
        "unaffected_applicable_hash_set_sha256",
        "unaffected_applicable_pass_hash_set_sha256",
        "unaffected_applicable_result_sha256",
    }
    require_keys(value, required, "target-authority transition")
    if (
        value["schema"]
        != "vaeg-upd9002-target-authority-transition-v1"
        or value["schema_version"] != 1
        or value["before_gate"] != APPROVED_PREDECESSOR_GATE
        or value["before_sha"] != APPROVED_PREDECESSOR_SHA
        or value["epoch_gate"] != CANDIDATE_GATE
        or value["transition_kind"] != "target_authority_correction"
        or value["profile"] != "architectural"
        or value["scope"] not in {"ci", "full"}
    ):
        raise M60bError("target-authority transition identity differs")
    require_sha(value["evaluated_sha"], "transition evaluated_sha")
    for field in (
        "applicable_hash_set_after_sha256",
        "applicable_hash_set_before_sha256",
        "authority_manifest_sha256",
        "comparison_contract_sha256",
        "retired_applicable_failure_hash_set_sha256",
        "retired_applicable_pass_hash_set_sha256",
        "scoreboard_after_digest",
        "scoreboard_before_digest",
        "selected_hash_set_sha256",
        "target_policy_after_sha256",
        "target_policy_before_sha256",
        "unaffected_applicable_failure_hash_set_sha256",
        "unaffected_applicable_hash_set_sha256",
        "unaffected_applicable_pass_hash_set_sha256",
        "unaffected_applicable_result_sha256",
    ):
        require_sha256(value[field], f"transition {field}")
    if value["newly_failing"] or value["newly_passing"]:
        raise M60bError("retired records were counted as semantic progress")
    if value["changed_failure_count"] or value["changed_failure_shards"]:
        raise M60bError("unaffected failure signature changed")
    if (
        value["retired_applicable_pass_count"]
        + value["retired_applicable_failure_count"]
        != value["classification_change_count"]
    ):
        raise M60bError("transition retired accounting is incomplete")
    if (
        value["candidate_applicable_count"]
        != value["predecessor_applicable_count"]
        - value["classification_change_count"]
        or value["candidate_pass_count"]
        != value["predecessor_pass_count"]
        - value["retired_applicable_pass_count"]
        or value["candidate_failure_count"]
        != value["predecessor_failure_count"]
        - value["retired_applicable_failure_count"]
    ):
        raise M60bError("transition denominator arithmetic differs")
    pass_rows = load_partition_rows(
        output_root,
        value["retired_applicable_pass_shards"],
        "retired_count",
        "retired",
    )
    failure_rows = load_partition_rows(
        output_root,
        value["retired_applicable_failure_shards"],
        "retired_count",
        "retired",
    )
    changes = load_partition_rows(
        output_root,
        value["classification_change_shards"],
        "classification_change_count",
        "classification_changes",
    )
    pass_hashes = [item["record_hash"] for item in pass_rows]
    failure_hashes = [item["record_hash"] for item in failure_rows]
    change_hashes = [item["record_hash"] for item in changes]
    for item in pass_rows:
        validate_retired_entry(item, "pass")
    for item in failure_rows:
        validate_retired_entry(item, "failure")
    for item in changes:
        validate_change_entry(item)
    if set(pass_hashes) & set(failure_hashes):
        raise M60bError("retired pass/failure sets overlap")
    if set(pass_hashes) | set(failure_hashes) != set(change_hashes):
        raise M60bError("retired sets do not cover classification changes")
    if (
        len(pass_hashes) != value["retired_applicable_pass_count"]
        or len(failure_hashes)
        != value["retired_applicable_failure_count"]
        or len(change_hashes) != value["classification_change_count"]
        or ratchet.hash_set_digest(pass_hashes)
        != value["retired_applicable_pass_hash_set_sha256"]
        or ratchet.hash_set_digest(failure_hashes)
        != value["retired_applicable_failure_hash_set_sha256"]
    ):
        raise M60bError("transition retired count/digest differs")
    if any(
        item["primary_opcode"]
        not in {"0x6c", "0x6d", "0x6e", "0x6f"}
        for item in changes
    ):
        raise M60bError("opcode outside 6c-6f left applicable")
    selectors = value["classification_changes"]
    policy = read_json(output_root / TARGET_POLICY_PATH)
    expected_selectors = compact_selector_changes(policy, changes)
    if (
        not isinstance(selectors, list)
        or [item.get("selector_sha256") for item in selectors]
        != sorted(item.get("selector_sha256") for item in selectors)
        or sum(item["resolved_count"] for item in selectors)
        != value["classification_change_count"]
        or selectors != expected_selectors
    ):
        raise M60bError("transition selector summary differs")


def generate_transition(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    candidate_relative: pathlib.Path,
    output_relative: pathlib.Path,
    scope: str,
) -> dict[str, Any]:
    policy = read_json(output_path(output_root, TARGET_POLICY_PATH))
    validate_target_policy(policy)
    candidate_path = output_path(output_root, candidate_relative)
    candidate = read_json(candidate_path)
    validate_scoreboard_v2(candidate)
    if (
        candidate["profile"] != "architectural"
        or candidate["scope"] != scope
        or candidate["target_policy_id"] != policy["target_policy_id"]
        or candidate["evaluated_sha"] != policy["evaluated_sha"]
    ):
        raise M60bError("transition candidate identity differs")
    _, before, before_failures = load_approved_scoreboard(
        root, "architectural", scope
    )
    after_failures = ratchet.load_scoreboard_failures(
        candidate_path, scoreboard_v1_view(candidate)
    )
    manifest = ssts.load_manifest(root / MANIFEST_PATH)
    with candidate_support_map(root) as candidate_map:
        enumeration = ratchet.enumerate_profiles(
            dataset_root,
            manifest,
            root / OLD_SUPPORT_MAP,
            candidate_map,
            scope,
        )
    before_applicable = set(enumeration["before_sets"]["applicable"])
    after_applicable = set(enumeration["after_sets"]["applicable"])
    unaffected = before_applicable & after_applicable
    before_failure_set = set(before_failures)
    after_failure_set = set(after_failures)
    before_pass_set = before_applicable - before_failure_set
    after_pass_set = after_applicable - after_failure_set
    if after_failure_set != before_failure_set & unaffected:
        raise M60bError("unaffected applicable failure set changed")
    if after_pass_set != before_pass_set & unaffected:
        raise M60bError("unaffected applicable pass set changed")
    for record_hash in sorted(after_failure_set):
        if before_failures[record_hash] != after_failures[record_hash]:
            raise M60bError("unaffected failure signature changed")
    if candidate["timeouts"] or candidate["crashes"]:
        raise M60bError("candidate timeout/crash count is nonzero")

    partition = policy["retired_applicable"][scope]
    pass_rows = load_partition_rows(
        output_root,
        partition["retired_applicable_pass_shards"],
        "retired_count",
        "retired",
    )
    failure_rows = load_partition_rows(
        output_root,
        partition["retired_applicable_failure_shards"],
        "retired_count",
        "retired",
    )
    classification_rows = load_partition_rows(
        output_root,
        partition["classification_change_shards"],
        "classification_change_count",
        "classification_changes",
    )
    retired_passes = {item["record_hash"] for item in pass_rows}
    retired_failures = {item["record_hash"] for item in failure_rows}
    validate_retirement_result_sets(
        retired_passes,
        retired_failures,
        after_pass_set,
        [],
        [],
    )
    if retired_passes != before_pass_set - after_pass_set:
        raise M60bError("retired pass accounting differs from execution")
    if retired_failures != before_failure_set - after_failure_set:
        raise M60bError("retired failure accounting differs from execution")
    before_rows = {
        item["form"]: item
        for item in before["records"]
        if item["classification"] == "applicable"
    }
    after_rows = {
        item["form"]: item
        for item in candidate["records"]
        if item["classification"] == "applicable"
    }
    retired_pass_by_form: Counter[str] = Counter(
        item["form"] for item in pass_rows
    )
    for form, row in before_rows.items():
        expected = row["pass"] - retired_pass_by_form[form]
        actual = after_rows.get(form, {}).get("pass", 0)
        if actual != expected:
            raise M60bError(f"{form}: unaffected per-form pass count differs")

    result_rows = []
    for record_hash in sorted(unaffected):
        failure = after_failures.get(record_hash)
        result_rows.append(
            {
                "record_hash": record_hash,
                "result": "failure" if failure else "pass",
                "signature_sha256": (
                    None if failure is None else failure["signature_sha256"]
                ),
            }
        )
    contract = policy["comparison_contracts"]["architectural"]
    transition = {
        "applicable_hash_set_after_sha256": candidate[
            "applicable_hash_set_sha256"
        ],
        "applicable_hash_set_before_sha256": before[
            "applicable_hash_set_sha256"
        ],
        "authority_manifest_sha256": policy[
            "authority_manifest_sha256"
        ],
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "candidate_applicable_count": candidate["applicable"],
        "candidate_failure_count": candidate["fail"],
        "candidate_pass_count": candidate["pass"],
        "changed_failure_count": 0,
        "changed_failure_shards": [],
        "classification_change_count": partition[
            "classification_change_count"
        ],
        "classification_change_shards": partition[
            "classification_change_shards"
        ],
        "classification_changes": compact_selector_changes(
            policy, classification_rows
        ),
        "comparison_contract_id": contract["id"],
        "comparison_contract_sha256": contract["sha256"],
        "dataset_id": policy["dataset_id"],
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": policy["evaluated_sha"],
        "gap_kind_changes": policy["gap_kind_changes"],
        "newly_failing": [],
        "newly_passing": [],
        "predecessor_applicable_count": before["applicable"],
        "predecessor_failure_count": before["fail"],
        "predecessor_pass_count": before["pass"],
        "profile": "architectural",
        "retired_applicable_failure_count": partition[
            "retired_applicable_failure_count"
        ],
        "retired_applicable_failure_hash_set_sha256": partition[
            "retired_applicable_failure_hash_set_sha256"
        ],
        "retired_applicable_failure_shards": partition[
            "retired_applicable_failure_shards"
        ],
        "retired_applicable_pass_count": partition[
            "retired_applicable_pass_count"
        ],
        "retired_applicable_pass_hash_set_sha256": partition[
            "retired_applicable_pass_hash_set_sha256"
        ],
        "retired_applicable_pass_shards": partition[
            "retired_applicable_pass_shards"
        ],
        "schema": "vaeg-upd9002-target-authority-transition-v1",
        "schema_version": 1,
        "scope": scope,
        "scoreboard_after_digest": candidate["scoreboard_digest"],
        "scoreboard_before_digest": before["scoreboard_digest"],
        "selected_hash_set_sha256": candidate[
            "selected_hash_set_sha256"
        ],
        "target_policy_after_id": policy["target_policy_id"],
        "target_policy_after_sha256": policy["target_policy_sha256"],
        "target_policy_before_id": policy["predecessor_policy"][
            "target_policy_id"
        ],
        "target_policy_before_sha256": policy["predecessor_policy"][
            "target_policy_sha256"
        ],
        "taxonomy_counts_after": policy["taxonomy_counts"]["after"],
        "taxonomy_counts_before": policy["taxonomy_counts"]["before"],
        "transition_kind": "target_authority_correction",
        "unaffected_applicable_failure_hash_set_sha256": (
            ratchet.hash_set_digest(after_failure_set)
        ),
        "unaffected_applicable_hash_set_sha256": ratchet.hash_set_digest(
            unaffected
        ),
        "unaffected_applicable_pass_hash_set_sha256": (
            ratchet.hash_set_digest(after_pass_set)
        ),
        "unaffected_applicable_result_sha256": identity_digest(result_rows),
    }
    validate_target_authority_transition(
        root, output_root, transition
    )
    write_json(output_path(output_root, output_relative), transition)
    print(
        "m60b-transition: "
        f"scope={scope} retired_pass="
        f"{transition['retired_applicable_pass_count']} "
        f"retired_failure="
        f"{transition['retired_applicable_failure_count']} "
        "newly_passing=0 newly_failing=0 changed_failure=0"
    )
    return transition


def g60b_artifact_paths(output_root: pathlib.Path) -> list[pathlib.Path]:
    roots = (
        output_root / "tests/ssts/authority/g60b",
        output_root / "tests/ssts/target_policy",
        output_root / "tests/ssts/scoreboard",
        output_root / "tests/ssts/transitions",
    )
    result = []
    manifest = output_root / "tests/ssts/target_policy/g60b_result_manifest.json"
    for base in roots:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if not path.is_file() or path == manifest:
                continue
            if (
                base.name in {"scoreboard", "transitions"}
                and not any(part.startswith("g60b") for part in path.parts)
            ):
                continue
            if base.name == "target_policy" and not any(
                part.startswith("g60b") for part in path.parts
            ):
                continue
            result.append(path)
    return sorted(set(result))


def generate_result_manifest(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    old_arch_ci: pathlib.Path,
    old_arch_full: pathlib.Path,
    old_fingerprint_full: pathlib.Path,
) -> dict[str, Any]:
    policy = read_json(output_path(output_root, TARGET_POLICY_PATH))
    validate_target_policy(policy)
    old_verification = [
        verify_old_policy_raw(
            root, dataset_root, old_arch_ci, "architectural", "ci"
        ),
        verify_old_policy_raw(
            root,
            dataset_root,
            old_arch_full,
            "architectural",
            "full",
        ),
        verify_old_policy_raw(
            root,
            dataset_root,
            old_fingerprint_full,
            "fingerprint",
            "full",
        ),
    ]
    scoreboards = {}
    for profile, scope in (
        ("architectural", "ci"),
        ("architectural", "full"),
        ("fingerprint", "full"),
    ):
        path = output_root / (
            f"tests/ssts/scoreboard/g60b_{profile}_{scope}.json"
        )
        value = read_json(path)
        validate_scoreboard_v2(value)
        ratchet.load_scoreboard_failures(path, scoreboard_v1_view(value))
        scoreboards[f"{profile}-{scope}"] = {
            "applicable": value["applicable"],
            "applicable_hash_set_sha256": value[
                "applicable_hash_set_sha256"
            ],
            "fail": value["fail"],
            "failure_hash_set_sha256": value[
                "failure_hash_set_sha256"
            ],
            "pass": value["pass"],
            "pass_hash_set_sha256": value["pass_hash_set_sha256"],
            "selected": value["selected"],
            "selected_hash_set_sha256": value[
                "selected_hash_set_sha256"
            ],
            "sha256": sha256_file(path),
            "timeout": value["timeouts"],
            "crash": value["crashes"],
        }
    transitions = {}
    for scope in ("ci", "full"):
        path = output_root / (
            f"tests/ssts/transitions/"
            f"g60b_architectural_{scope}_from_g60a.json"
        )
        value = read_json(path)
        validate_target_authority_transition(root, output_root, value)
        transitions[scope] = {
            "classification_change_count": value[
                "classification_change_count"
            ],
            "retired_applicable_failure_count": value[
                "retired_applicable_failure_count"
            ],
            "retired_applicable_failure_hash_set_sha256": value[
                "retired_applicable_failure_hash_set_sha256"
            ],
            "retired_applicable_pass_count": value[
                "retired_applicable_pass_count"
            ],
            "retired_applicable_pass_hash_set_sha256": value[
                "retired_applicable_pass_hash_set_sha256"
            ],
            "sha256": sha256_file(path),
            "unaffected_applicable_result_sha256": value[
                "unaffected_applicable_result_sha256"
            ],
        }
    artifacts = [
        {
            "bytes": path.stat().st_size,
            "path": path.relative_to(output_root).as_posix(),
            "sha256": sha256_file(path),
        }
        for path in g60b_artifact_paths(output_root)
    ]
    artifact_tree_sha256 = identity_digest(artifacts)
    authority_path = output_root / AUTHORITY_ROOT / "manifest.json"
    manifest = {
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifact_count": len(artifacts),
        "artifact_tree_sha256": artifact_tree_sha256,
        "artifacts": artifacts,
        "authority_manifest_sha256": sha256_file(authority_path),
        "candidate_gate": CANDIDATE_GATE,
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": policy["dataset_id"],
        "evaluated_sha": policy["evaluated_sha"],
        "historical_g43_reconciliation": policy[
            "historical_g43_reconciliation"
        ],
        "license": "BSD-2-Clause",
        "milestone": MILESTONE,
        "old_policy_reproduction": old_verification,
        "schema": "vaeg-upd9002-g60b-result-manifest-v1",
        "schema_version": 1,
        "scoreboards": scoreboards,
        "target_policy_id": policy["target_policy_id"],
        "target_policy_sha256": policy["target_policy_sha256"],
        "taxonomy_counts": policy["taxonomy_counts"],
        "transitions": transitions,
    }
    path = output_root / "tests/ssts/target_policy/g60b_result_manifest.json"
    write_json(path, manifest)
    print(
        "m60b-result-manifest: "
        f"artifacts={len(artifacts)} tree={artifact_tree_sha256}"
    )
    return manifest


def validate_result_manifest(
    root: pathlib.Path, output_root: pathlib.Path
) -> dict[str, Any]:
    path = output_root / "tests/ssts/target_policy/g60b_result_manifest.json"
    value = read_json(path)
    require_keys(
        value,
        {
            "approved_predecessor_gate",
            "approved_predecessor_sha",
            "artifact_count",
            "artifact_tree_sha256",
            "artifacts",
            "authority_manifest_sha256",
            "candidate_gate",
            "copyright",
            "dataset_id",
            "evaluated_sha",
            "historical_g43_reconciliation",
            "license",
            "milestone",
            "old_policy_reproduction",
            "schema",
            "schema_version",
            "scoreboards",
            "target_policy_id",
            "target_policy_sha256",
            "taxonomy_counts",
            "transitions",
        },
        "G60b result manifest",
    )
    if (
        value["schema"] != "vaeg-upd9002-g60b-result-manifest-v1"
        or value["schema_version"] != 1
        or value["candidate_gate"] != CANDIDATE_GATE
        or value["approved_predecessor_sha"] != APPROVED_PREDECESSOR_SHA
    ):
        raise M60bError("G60b result manifest identity differs")
    rows = value["artifacts"]
    if (
        not isinstance(rows, list)
        or [item.get("path") for item in rows]
        != sorted(item.get("path") for item in rows)
        or value["artifact_count"] != len(rows)
        or identity_digest(rows) != value["artifact_tree_sha256"]
    ):
        raise M60bError("G60b result artifact tree differs")
    for item in rows:
        relative = pathlib.Path(item["path"])
        artifact = output_root / relative
        if (
            not artifact.is_file()
            or artifact.stat().st_size != item["bytes"]
            or sha256_file(artifact) != item["sha256"]
        ):
            raise M60bError(f"G60b artifact differs: {relative}")
    policy = read_json(output_root / TARGET_POLICY_PATH)
    validate_target_policy(policy)
    if (
        value["target_policy_id"] != policy["target_policy_id"]
        or value["target_policy_sha256"] != policy["target_policy_sha256"]
        or value["evaluated_sha"] != policy["evaluated_sha"]
    ):
        raise M60bError("G60b result/target-policy identities differ")
    for profile, scope in (
        ("architectural", "ci"),
        ("architectural", "full"),
        ("fingerprint", "full"),
    ):
        scoreboard_path = output_root / (
            f"tests/ssts/scoreboard/g60b_{profile}_{scope}.json"
        )
        scoreboard = read_json(scoreboard_path)
        validate_scoreboard_v2(scoreboard)
        ratchet.load_scoreboard_failures(
            scoreboard_path, scoreboard_v1_view(scoreboard)
        )
    for scope in ("ci", "full"):
        transition = read_json(
            output_root
            / f"tests/ssts/transitions/"
            f"g60b_architectural_{scope}_from_g60a.json"
        )
        validate_target_authority_transition(
            root, output_root, transition
        )
    return value


def verify_new_policy_raw(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    old_raw_path: pathlib.Path,
    new_raw_path: pathlib.Path,
    profile: str,
    scope: str,
) -> dict[str, Any]:
    manifest = ssts.load_manifest(root / MANIFEST_PATH)
    old_raw = read_json(old_raw_path)
    new_raw = read_json(new_raw_path)
    for value, name in ((old_raw, "old"), (new_raw, "new")):
        if (
            value.get("schema") != "vaeg-upd9002-ssts-result-v1"
            or value.get("dataset_id") != manifest["dataset_id"]
            or value.get("profile") != scope
        ):
            raise M60bError(f"{name} raw policy identity differs")
        if profile == "fingerprint" and value.get(
            "flags_comparison"
        ) != "all16":
            raise M60bError(f"{name} fingerprint policy lacks all16 FLAGS")
        if profile == "architectural" and "flags_comparison" in value:
            raise M60bError(f"{name} architectural policy uses all16 FLAGS")
    with candidate_support_map(root) as candidate_map:
        enumeration = ratchet.enumerate_profiles(
            dataset_root,
            manifest,
            root / OLD_SUPPORT_MAP,
            candidate_map,
            scope,
        )
    old_failures = {
        key: ratchet.failure_entry(value)
        for key, value in ratchet.load_failure_records(old_raw_path).items()
    }
    new_failures = {
        key: ratchet.failure_entry(value)
        for key, value in ratchet.load_failure_records(new_raw_path).items()
    }
    before_applicable = set(enumeration["before_sets"]["applicable"])
    after_applicable = set(enumeration["after_sets"]["applicable"])
    unaffected = before_applicable & after_applicable
    if set(new_failures) != set(old_failures) & unaffected:
        raise M60bError("new policy changed an unaffected failure result")
    for record_hash, failure in new_failures.items():
        if failure != old_failures[record_hash]:
            raise M60bError("new policy changed an unaffected failure signature")
    old_passes = before_applicable - set(old_failures)
    new_passes = after_applicable - set(new_failures)
    if new_passes != old_passes & unaffected:
        raise M60bError("new policy changed an unaffected pass result")
    for value, name in ((old_raw, "old"), (new_raw, "new")):
        if value["result_counts"].get("timeout", 0):
            raise M60bError(f"{name} policy timeout count is nonzero")
        if value["result_counts"].get("crash", 0):
            raise M60bError(f"{name} policy crash count is nonzero")
    retired = before_applicable - after_applicable
    retired_passes = retired & old_passes
    retired_failures = retired & set(old_failures)
    if len(retired) != len(retired_passes) + len(retired_failures):
        raise M60bError("raw retired accounting is incomplete")
    return {
        "new_applicable": len(after_applicable),
        "new_fail": len(new_failures),
        "new_pass": len(new_passes),
        "profile": profile,
        "retired_applicable_failure_count": len(retired_failures),
        "retired_applicable_pass_count": len(retired_passes),
        "scope": scope,
        "unaffected_result_sha256": identity_digest(
            [
                {
                    "record_hash": record_hash,
                    "result": (
                        "failure"
                        if record_hash in new_failures
                        else "pass"
                    ),
                    "signature_sha256": (
                        new_failures[record_hash]["signature_sha256"]
                        if record_hash in new_failures
                        else None
                    ),
                }
                for record_hash in sorted(unaffected)
            ]
        ),
    }


def ci_enforce(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    evaluated_sha: str,
    output_root: pathlib.Path,
) -> None:
    require_sha(evaluated_sha, "evaluated_sha")
    if evaluated_sha == APPROVED_PREDECESSOR_SHA:
        raise M60bError("current-worktree self-comparison is forbidden")
    output_root.mkdir(parents=True, exist_ok=True)
    paths = {}
    for policy in ("g60a", "g60b"):
        raw = output_root / f"{policy}_architectural_ci.json"
        failures = output_root / f"{policy}_architectural_ci_failures"
        run_profile(
            root,
            dataset_root,
            worker,
            "ci",
            "architectural",
            policy,
            raw,
            failures,
        )
        paths[policy] = raw
    verify_old_policy_raw(
        root,
        dataset_root,
        paths["g60a"],
        "architectural",
        "ci",
    )
    result = verify_new_policy_raw(
        root,
        dataset_root,
        paths["g60a"],
        paths["g60b"],
        "architectural",
        "ci",
    )
    print(
        "m60b-ci: old G60a policy reproduced exactly; "
        f"new applicable={result['new_applicable']} "
        f"pass={result['new_pass']} fail={result['new_fail']} "
        "unaffected applicable results exact"
    )


def g60a_artifact_tree_digest(root: pathlib.Path) -> tuple[int, int, str]:
    paths = []
    for base in (
        root / "tests/ssts/scoreboard",
        root / "tests/ssts/transitions",
    ):
        for path in base.rglob("*"):
            if path.is_file() and any(
                part.startswith("g60a") for part in path.parts
            ):
                paths.append(path)
    paths.sort()
    rows = [
        {
            "bytes": path.stat().st_size,
            "path": path.relative_to(root).as_posix(),
            "sha256": sha256_file(path),
        }
        for path in paths
    ]
    return len(rows), sum(item["bytes"] for item in rows), identity_digest(rows)


def verify_git_paths_unchanged(
    root: pathlib.Path,
    base_sha: str,
    paths: list[str],
    label: str,
) -> None:
    result = subprocess.run(
        ["git", "diff", "--quiet", base_sha, "--", *sorted(set(paths))],
        cwd=root,
        check=False,
    )
    if result.returncode != 0:
        raise M60bError(f"{label} changed")


def verify_protected_git_diff(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    protected = [
        "tests/ssts/baseline",
        "tests/ssts/epochs/g43",
        "tests/ssts/evidence/g59",
        "tests/ssts/contracts",
        "tests/ssts/v20_dataset_manifest.json",
        "tests/ssts/approved_target_divergences.json",
        "tests/ssts/hardware_pending.json",
        "tests/ssts/gap_taxonomy.json",
        OLD_SUPPORT_MAP.as_posix(),
    ]
    if not (
        protected_evidence_only
        and (
            root
            / "docs/agents/tasks/M70_upd9002_prefix_string_closure.md"
        ).is_file()
    ):
        protected.append("tools/qa/upd9002_ssts.py")
    if not protected_evidence_only:
        protected.append("cpu/upd9002")
    protected.extend(
        path.relative_to(root).as_posix()
        for base in (
            root / "tests/ssts/scoreboard",
            root / "tests/ssts/transitions",
        )
        for path in base.rglob("*")
        if path.is_file()
        and any(
            part.startswith(("g58", "g60a")) for part in path.parts
        )
    )
    verify_git_paths_unchanged(
        root,
        APPROVED_PREDECESSOR_GIT_SHA,
        protected,
        "protected evidence, fixture, contract, or production cpu/upd9002",
    )


def verify_static(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    forward_milestone = (
        root / "docs/agents/tasks/M62_upd9002_semantics_bundle.md"
    ).is_file() or (
        root / "docs/agents/tasks/M70_upd9002_prefix_string_closure.md"
    ).is_file()
    protected_evidence_only = protected_evidence_only or forward_milestone
    try:
        ratchet.verify_static(root)
        m59.validate_pack(root, root / "tests/ssts/evidence/g59")
        m60a.verify_static(
            root, "e7f2325bc81310532091a8ca82914030fdb8b6ba"
        )
    except (
        ratchet.RatchetError,
        m59.EvidenceError,
        m60a.M60aEvidenceError,
    ) as error:
        raise M60bError(str(error)) from error
    count, byte_count, tree_digest = g60a_artifact_tree_digest(root)
    if (
        count != 36
        or byte_count != 28804763
        or tree_digest != G60A_ARTIFACT_TREE_SHA256
    ):
        raise M60bError("approved G60a artifact tree differs")
    transition_path = (
        root
        / "tests/ssts/transitions/g60a_architectural_full_from_g59.json"
    )
    if sha256_file(transition_path) != G60A_FULL_TRANSITION_SHA256:
        raise M60bError("approved G60a full transition differs")
    verify_protected_git_diff(root, protected_evidence_only)

    authority_manifest = root / AUTHORITY_ROOT / "manifest.json"
    policy_path = root / TARGET_POLICY_PATH
    result_manifest = (
        root / "tests/ssts/target_policy/g60b_result_manifest.json"
    )
    family = [
        authority_manifest.is_file(),
        policy_path.is_file(),
        result_manifest.is_file(),
    ]
    if any(family) and not all(family):
        raise M60bError("G60b evidence family is incomplete")
    if all(family):
        load_and_validate_authority_pack(root)
        validate_result_manifest(root, root)
        scope = (
            "protected evidence"
            if protected_evidence_only
            else "protected evidence and cpu/upd9002"
        )
        print(
            f"m60b-static: {scope}, ROM authority, target policy, "
            "scoreboards, transitions, and result manifest passed"
        )
    else:
        print(
            "m60b-static: implementation-only tree; protected "
            "G43/G58/G59/G60a evidence and cpu/upd9002 passed"
        )


def validate_change_entry(value: Any) -> None:
    required = {
        "after_classification",
        "before_classification",
        "form",
        "primary_opcode",
        "record_hash",
        "repeat_prefix",
        "transition_kind",
        "upstream_test_hash",
    }
    require_keys(value, required, "classification change")
    if value["transition_kind"] != "target_authority_correction":
        raise M60bError("wrong transition_kind")
    if value["before_classification"] != "applicable":
        raise M60bError("classification change did not leave applicable")
    if value["after_classification"] != "known_target_gap":
        raise M60bError("classification change has wrong destination")
    if value["primary_opcode"] not in {
        "0x6c",
        "0x6d",
        "0x6e",
        "0x6f",
    }:
        raise M60bError("opcode outside 6c-6f transitioned")
    require_sha256(value["record_hash"], "classification record hash")
    try:
        ratchet.require_sha1(
            value["upstream_test_hash"], "classification upstream hash"
        )
    except ratchet.RatchetError as error:
        raise M60bError(str(error)) from error


def enforce_authority_correction_contract(value: Any) -> None:
    required = {
        "authority_manifest_sha256",
        "classification_changes",
        "comparison_contract_after",
        "comparison_contract_before",
        "dataset_after",
        "dataset_before",
        "gap_kind",
        "protected_0f28_unchanged",
        "protected_66_67_unchanged",
        "retired_failure_hashes",
        "retired_pass_hashes",
        "selected_after",
        "selected_before",
        "transition_kind",
    }
    require_keys(value, required, "authority correction contract")
    if value["transition_kind"] != "target_authority_correction":
        raise M60bError("wrong transition_kind")
    require_sha256(
        value["authority_manifest_sha256"], "authority manifest digest"
    )
    if value["dataset_before"] != value["dataset_after"]:
        raise M60bError("dataset identity changed")
    if value["comparison_contract_before"] != value[
        "comparison_contract_after"
    ]:
        raise M60bError("comparison contract changed")
    if value["selected_before"] != value["selected_after"]:
        raise M60bError("selected hash set changed")
    if not value["protected_0f28_unchanged"]:
        raise M60bError("0f28 classification changed")
    if not value["protected_66_67_unchanged"]:
        raise M60bError("66/67 classification or gap kind changed")
    if value["gap_kind"] != "documented_silicon_absent":
        raise M60bError("authorized correction has wrong gap_kind")
    changes = value["classification_changes"]
    if not isinstance(changes, list) or not changes:
        raise M60bError("authority correction has no classification changes")
    for change in changes:
        validate_change_entry(change)
    changed = {item["record_hash"] for item in changes}
    passes = set(value["retired_pass_hashes"])
    failures = set(value["retired_failure_hashes"])
    if passes & failures:
        raise M60bError("retired pass/failure overlap")
    if passes | failures != changed:
        raise M60bError("retired sets are incomplete")


def expect_rejected(
    checks: list[str], name: str, function: Any, *arguments: Any
) -> None:
    try:
        function(*arguments)
    except (M60bError, ratchet.RatchetError, ssts.CorpusError):
        checks.append(name)
        return
    raise AssertionError(f"negative selftest was accepted: {name}")


def synthetic_change(record_hash: str = "1" * 64) -> dict[str, Any]:
    return {
        "after_classification": "known_target_gap",
        "before_classification": "applicable",
        "form": "6C",
        "primary_opcode": "0x6c",
        "record_hash": record_hash,
        "repeat_prefix": "none",
        "transition_kind": "target_authority_correction",
        "upstream_test_hash": "2" * 40,
    }


def synthetic_contract() -> dict[str, Any]:
    return {
        "authority_manifest_sha256": "3" * 64,
        "classification_changes": [synthetic_change()],
        "comparison_contract_after": ("architectural", "4" * 64),
        "comparison_contract_before": ("architectural", "4" * 64),
        "dataset_after": "dataset",
        "dataset_before": "dataset",
        "gap_kind": "documented_silicon_absent",
        "protected_0f28_unchanged": True,
        "protected_66_67_unchanged": True,
        "retired_failure_hashes": [],
        "retired_pass_hashes": ["1" * 64],
        "selected_after": "5" * 64,
        "selected_before": "5" * 64,
        "transition_kind": "target_authority_correction",
    }


def selftest(root: pathlib.Path) -> None:
    positive = []
    negative = []

    validate_rom_claim(ROM_SIZE, ROM_SHA256)
    positive.append("correct ROM identity")
    expect_rejected(
        negative,
        "wrong ROM SHA",
        validate_rom_claim,
        ROM_SIZE,
        "0" * 64,
    )
    expect_rejected(
        negative,
        "wrong ROM size",
        validate_rom_claim,
        ROM_SIZE - 1,
        ROM_SHA256,
    )

    table = bytes.fromhex("ff2000ff2200")
    parsed = parse_records(table, 0, len(table), 2, "synthetic")
    if len(parsed) != 2:
        raise AssertionError("synthetic table did not parse")
    positive.append("three-byte table")
    expect_rejected(
        negative,
        "truncated table",
        parse_records,
        table,
        0,
        len(table) + 1,
        2,
        "synthetic",
    )
    expect_rejected(
        negative,
        "malformed three-byte record",
        parse_records,
        table + b"\0",
        0,
        len(table) + 1,
        2,
        "synthetic",
    )
    expect_rejected(
        negative,
        "unexpected raw-record count",
        parse_records,
        table,
        0,
        len(table),
        3,
        "synthetic",
    )
    expect_rejected(
        negative,
        "missing table boundary",
        parse_records,
        table,
        0,
        0,
        0,
        "synthetic",
    )
    expect_rejected(
        negative,
        "missing mnemonic terminator",
        decode_high_bit_strings,
        b"ABC",
        0,
        1,
        "synthetic",
    )
    ambiguous_records = [
        {"group": "0x00", "index": 0, "mask": "0xff", "value": "0x20"},
        {"group": "0x01", "index": 1, "mask": "0xff", "value": "0x20"},
    ]
    ambiguous_mnemonics = [
        {"text": "ADD4S"},
        {"text": "SUB4S"},
    ]
    expect_rejected(
        negative,
        "ambiguous mask expansion",
        validate_expansion,
        ambiguous_records,
        ambiguous_mnemonics,
        "synthetic",
    )
    expect_rejected(
        negative,
        "incorrect group-to-mnemonic mapping",
        validate_expansion,
        ambiguous_records,
        ambiguous_mnemonics[:1],
        "synthetic",
    )
    duplicate_records = [
        {"group": "0x00", "index": 0, "mask": "0xff", "value": "0x20"},
        {"group": "0x00", "index": 1, "mask": "0xff", "value": "0x20"},
    ]
    duplicate_mnemonics = [{"text": "ADD4S"}, {"text": "ADD4S"}]
    expect_rejected(
        negative,
        "duplicate expanded opcode",
        validate_expansion,
        duplicate_records,
        duplicate_mnemonics,
        "synthetic",
    )
    expect_rejected(
        negative,
        "0f31 presence contradiction",
        validate_forbidden_0f_inventory,
        [{"second_opcode": "0x31"}],
    )
    expect_rejected(
        negative,
        "incomplete string-pool search range",
        string_pool_audit,
        b"\0" * (STRING_POOL_END - 1),
    )

    source = {
        "authorization": "metadata only",
        "observations": [
            {
                "claim": (
                    "Machine monitor investigation used; "
                    "0f ff imm8 BRKEM and 0f fe imm8 BRKFEM."
                ),
                "local_snapshot_sha256": EXPECTED_DEBUGGER_SNAPSHOTS[post],
                "post": post,
                "url": (
                    "https://yomi.tokyo/agate/ikura/i4004/"
                    f"1168522493/{post}"
                ),
            }
            for post in sorted(EXPECTED_DEBUGGER_SNAPSHOTS)
        ],
        "provenance": "public archive",
        "schema": "vaeg-upd9002-debugger-evidence-source-v1",
        "schema_version": 1,
    }
    validate_debugger_source(source)
    positive.append("debugger corroboration")
    missing = copy.deepcopy(source)
    missing["observations"].pop()
    expect_rejected(
        negative,
        "missing BRKFEM debugger corroboration",
        validate_debugger_source,
        missing,
    )
    wrong = copy.deepcopy(source)
    wrong["observations"][0]["local_snapshot_sha256"] = "0" * 64
    expect_rejected(
        negative,
        "debugger digest mismatch",
        validate_debugger_source,
        wrong,
    )

    fields, rows = read_support_rows(root / OLD_SUPPORT_MAP)
    del fields
    candidate = modify_support_rows(rows)
    if len(rows) != len(candidate):
        raise AssertionError("support overlay changed row cardinality")
    if sum(a != b for a, b in zip(rows, candidate)) != 12:
        raise AssertionError("support overlay changed an unexpected row count")
    for before, after in zip(rows, candidate):
        if int(before["opcode"], 16) in {0x66, 0x67} and before != after:
            raise AssertionError("support overlay changed 66/67")
    positive.append("exact support overlay")

    for instruction, expected in (
        ([0x6C], 0x6C),
        ([0x26, 0x6D], 0x6D),
        ([0xF3, 0x2E, 0x6E], 0x6E),
        ([0x64, 0xF0, 0x36, 0x6F], 0x6F),
    ):
        observed, _ = decode_primary_opcode(instruction)
        if observed != expected:
            raise AssertionError("prefixed primary opcode resolution differs")
    positive.append("prefixed structural selectors")
    non_target, _ = decode_primary_opcode([0xF3, 0xA4])
    if non_target in AUTHORIZED_PRIMARY_OPCODES:
        raise AssertionError("non-6c-6f opcode selected")
    expect_rejected(
        negative,
        "prefix-only instruction",
        decode_primary_opcode,
        [0xF3, 0x26],
    )
    synthetic_hashes = ["a" * 40]
    synthetic_selector = {
        "after_gap_kind": "documented_silicon_absent",
        "resolved_count": 1,
        "resolved_test_hashes": synthetic_hashes,
        "resolved_test_hashes_sha256": ratchet.upstream_hash_set_digest(
            synthetic_hashes
        ),
        "selector": {"opcode": "6c"},
        "selector_sha256": "1" * 64,
    }
    validate_selector_rules([synthetic_selector], 1)
    positive.append("closed structural selector")
    overlapping_selectors = [
        synthetic_selector,
        {
            **copy.deepcopy(synthetic_selector),
            "selector_sha256": "2" * 64,
        },
    ]
    expect_rejected(
        negative,
        "selector overlap",
        validate_selector_rules,
        overlapping_selectors,
        1,
    )
    expect_rejected(
        negative,
        "incomplete selector coverage",
        validate_selector_rules,
        [synthetic_selector],
        2,
    )
    digest_mismatch = copy.deepcopy(synthetic_selector)
    digest_mismatch["resolved_test_hashes_sha256"] = "0" * 64
    expect_rejected(
        negative,
        "selector hash-list digest mismatch",
        validate_selector_rules,
        [digest_mismatch],
        1,
    )
    scoped_policy = {
        "classification_selector_rules": [
            {
                "before_classification": "applicable",
                "form": "6C",
                "resolved_test_hashes": ["1" * 40, "2" * 40],
                "selector": {"repeat_prefix": "none"},
                "selector_sha256": "3" * 64,
            }
        ]
    }
    scoped_changes = [
        {
            "form": "6C",
            "repeat_prefix": "none",
            "upstream_test_hash": "1" * 40,
        }
    ]
    scoped_summary = compact_selector_changes(
        scoped_policy, scoped_changes
    )
    if (
        scoped_summary[0]["resolved_count"] != 1
        or scoped_summary[0]["resolved_test_hashes_sha256"]
        != ratchet.upstream_hash_set_digest(["1" * 40])
    ):
        raise AssertionError("scope-local selector summary differs")
    positive.append("scope-local selector summary")
    unknown_scoped_change = copy.deepcopy(scoped_changes)
    unknown_scoped_change[0]["upstream_test_hash"] = "4" * 40
    expect_rejected(
        negative,
        "scope-local selector ownership mismatch",
        compact_selector_changes,
        scoped_policy,
        unknown_scoped_change,
    )

    contract = synthetic_contract()
    enforce_authority_correction_contract(contract)
    positive.append("authority correction contract")
    mutations = [
        ("missing authority digest", "authority_manifest_sha256", None),
        ("missing gap kind", "gap_kind", None),
        ("wrong transition kind", "transition_kind", "semantic_fix"),
        ("dataset identity mismatch", "dataset_after", "other"),
        (
            "comparison-contract mismatch",
            "comparison_contract_after",
            ("fingerprint", "4" * 64),
        ),
        ("selected hash-set mismatch", "selected_after", "6" * 64),
        ("0f28 classification change", "protected_0f28_unchanged", False),
        ("66/67 policy change", "protected_66_67_unchanged", False),
        ("wrong gap kind", "gap_kind", "implementation_missing"),
    ]
    for name, field, value in mutations:
        mutated = copy.deepcopy(contract)
        if value is None:
            del mutated[field]
        else:
            mutated[field] = value
        expect_rejected(
            negative,
            name,
            enforce_authority_correction_contract,
            mutated,
        )
    outside = copy.deepcopy(contract)
    outside["classification_changes"][0]["primary_opcode"] = "0x70"
    expect_rejected(
        negative,
        "opcode outside 6c-6f transition",
        enforce_authority_correction_contract,
        outside,
    )
    outcome = copy.deepcopy(contract)
    outcome["classification_changes"][0]["result"] = "failure"
    expect_rejected(
        negative,
        "outcome-based selector",
        enforce_authority_correction_contract,
        outcome,
    )
    overlap = copy.deepcopy(contract)
    overlap["retired_failure_hashes"] = ["1" * 64]
    expect_rejected(
        negative,
        "retired pass/failure overlap",
        enforce_authority_correction_contract,
        overlap,
    )
    missing_retired = copy.deepcopy(contract)
    missing_retired["retired_pass_hashes"] = []
    expect_rejected(
        negative,
        "missing retired hash",
        enforce_authority_correction_contract,
        missing_retired,
    )
    extra_retired = copy.deepcopy(contract)
    extra_retired["retired_pass_hashes"].append("7" * 64)
    expect_rejected(
        negative,
        "extra retired hash",
        enforce_authority_correction_contract,
        extra_retired,
    )
    retired_pass = normalized_retired_entry(
        {
            "form": "6C",
            "prefix_class": "unprefixed",
            "primary_opcode": "0x6c",
            "record_hash": "1" * 64,
            "repeat_prefix": "none",
            "upstream_test_hash": "2" * 40,
        },
        "pass",
        None,
    )
    validate_retired_entry(retired_pass, "pass")
    invalid_retired_result = copy.deepcopy(retired_pass)
    invalid_retired_result["previous_result"] = "failure"
    expect_rejected(
        negative,
        "incorrect retired result",
        validate_retired_entry,
        invalid_retired_result,
        "pass",
    )
    expect_rejected(
        negative,
        "retired failure counted as newly passing",
        validate_retirement_result_sets,
        set(),
        {"1" * 64},
        set(),
        ["1" * 64],
        [],
    )
    expect_rejected(
        negative,
        "retired pass counted as candidate pass",
        validate_retirement_result_sets,
        {"1" * 64},
        set(),
        {"1" * 64},
        [],
        [],
    )

    selector = {"opcode": "0x6c", "result": "pass"}
    expect_rejected(
        negative,
        "pass/fail-based structural selector",
        ratchet.selector_digest,
        selector,
    )
    expect_rejected(
        negative,
        "open-ended selector",
        ratchet.selector_digest,
        {"opcode": "*"},
    )

    with tempfile.TemporaryDirectory(
        prefix="vaeg-m60b-selftest-"
    ) as temporary:
        base = pathlib.Path(temporary)
        payload = {
            "dataset_id": "synthetic",
            "primary_opcode": "0x6c",
            "retired": [normalized_retired_entry(
                {
                    "form": "6C",
                    "prefix_class": "unprefixed",
                    "primary_opcode": "0x6c",
                    "record_hash": "1" * 64,
                    "repeat_prefix": "none",
                    "upstream_test_hash": "2" * 40,
                },
                "pass",
                None,
            )],
            "retired_count": 1,
            "schema": "vaeg-upd9002-retired-applicable-v1",
            "schema_version": 1,
            "scope": "full",
        }
        copies = []
        for name in ("first", "second"):
            path = base / name / "6c.json.gz"
            ratchet.write_deterministic_gzip(path, payload)
            copies.append(path.read_bytes())
        if copies[0] != copies[1]:
            raise AssertionError("deterministic gzip output differs")
        invalid_shard = {
            "canonical_sha256": sha256_bytes(
                canonical_bytes(payload) + b"\n"
            ),
            "path": "first/6c.json.gz",
            "retired_count": 2,
            "sha256": sha256_file(base / "first/6c.json.gz"),
        }
        expect_rejected(
            negative,
            "retired count/digest mismatch",
            load_partition_rows,
            base,
            [invalid_shard],
            "retired_count",
            "retired",
        )
        (base / "tree-first").mkdir()
        (base / "tree-second").mkdir()
        (base / "tree-first/value").write_bytes(b"first")
        (base / "tree-second/value").write_bytes(b"second")
        expect_rejected(
            negative,
            "nondeterministic authority output",
            compare_trees,
            base / "tree-first",
            base / "tree-second",
        )
        protected_repository = base / "protected-repository"
        protected_repository.mkdir()
        protected_cases = {
            "immutable G43/M43 artifact mutation": (
                "tests/ssts/epochs/g43/manifest.json"
            ),
            "approved G58 artifact mutation": (
                "tests/ssts/scoreboard/g58_architectural_ci.json"
            ),
            "approved G59 artifact mutation": (
                "tests/ssts/evidence/g59/manifest.json"
            ),
            "approved G60a artifact mutation": (
                "tests/ssts/scoreboard/g60a_architectural_ci.json"
            ),
            "fixture modification": "tools/qa/upd9002_ssts.py",
            "production cpu tree change": "cpu/upd9002/cpucore.c",
        }
        for relative in protected_cases.values():
            path = protected_repository / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("protected\n", encoding="utf-8")
        subprocess.run(
            ["git", "init", "--quiet"],
            cwd=protected_repository,
            check=True,
        )
        subprocess.run(
            ["git", "add", "."],
            cwd=protected_repository,
            check=True,
        )
        subprocess.run(
            [
                "git",
                "-c",
                "user.name=VAEG selftest",
                "-c",
                "user.email=vaeg-selftest.invalid",
                "commit",
                "--quiet",
                "-m",
                "selftest seed",
            ],
            cwd=protected_repository,
            check=True,
        )
        for name, relative in protected_cases.items():
            path = protected_repository / relative
            path.write_text("mutated\n", encoding="utf-8")
            expect_rejected(
                negative,
                name,
                verify_git_paths_unchanged,
                protected_repository,
                "HEAD",
                [relative],
                name,
            )
            path.write_text("protected\n", encoding="utf-8")
    positive.append("deterministic gzip")

    expected_negative = {
        "wrong ROM SHA",
        "wrong ROM size",
        "truncated table",
        "malformed three-byte record",
        "unexpected raw-record count",
        "missing table boundary",
        "missing mnemonic terminator",
        "ambiguous mask expansion",
        "incorrect group-to-mnemonic mapping",
        "duplicate expanded opcode",
        "0f31 presence contradiction",
        "incomplete string-pool search range",
        "missing BRKFEM debugger corroboration",
        "debugger digest mismatch",
        "prefix-only instruction",
        "selector overlap",
        "incomplete selector coverage",
        "selector hash-list digest mismatch",
        "scope-local selector ownership mismatch",
        "missing authority digest",
        "missing gap kind",
        "wrong transition kind",
        "dataset identity mismatch",
        "comparison-contract mismatch",
        "selected hash-set mismatch",
        "0f28 classification change",
        "66/67 policy change",
        "wrong gap kind",
        "opcode outside 6c-6f transition",
        "outcome-based selector",
        "retired pass/failure overlap",
        "missing retired hash",
        "extra retired hash",
        "incorrect retired result",
        "retired failure counted as newly passing",
        "retired pass counted as candidate pass",
        "retired count/digest mismatch",
        "pass/fail-based structural selector",
        "open-ended selector",
        "nondeterministic authority output",
        "immutable G43/M43 artifact mutation",
        "approved G58 artifact mutation",
        "approved G59 artifact mutation",
        "approved G60a artifact mutation",
        "fixture modification",
        "production cpu tree change",
    }
    if set(negative) != expected_negative:
        raise AssertionError("M60b negative-test coverage differs")
    print(
        "m60b-selftest: "
        f"{len(positive)} positive and {len(negative)} fail-closed checks passed"
    )


def compare_trees(first: pathlib.Path, second: pathlib.Path) -> None:
    first_files = {
        path.relative_to(first): path.read_bytes()
        for path in first.rglob("*")
        if path.is_file()
    }
    second_files = {
        path.relative_to(second): path.read_bytes()
        for path in second.rglob("*")
        if path.is_file()
    }
    if first_files != second_files:
        raise M60bError("deterministic regeneration differs")


def verify_authority_with_rom(
    root: pathlib.Path,
    pack_root: pathlib.Path,
    rom_path: pathlib.Path,
    debugger_source: pathlib.Path,
) -> None:
    load_and_validate_authority_pack(pack_root)
    with tempfile.TemporaryDirectory(
        prefix="vaeg-m60b-authority-verify-"
    ) as temporary:
        base = pathlib.Path(temporary)
        first = base / "first"
        second = base / "second"
        generate_authority_pack(root, first, rom_path, debugger_source)
        generate_authority_pack(root, second, rom_path, debugger_source)
        compare_trees(first, second)
        committed = pack_root / AUTHORITY_ROOT
        regenerated = first / AUTHORITY_ROOT
        compare_trees(committed, regenerated)
    print(
        "m60b-authority-verify: ROM-bound extraction and two-run "
        "byte identity passed"
    )


def add_root(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--root", type=pathlib.Path, default=pathlib.Path(".")
    )


def add_output_root(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--output-root", type=pathlib.Path, required=True)


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    selftest_parser = subparsers.add_parser("selftest")
    add_root(selftest_parser)

    static = subparsers.add_parser("verify-static")
    add_root(static)
    static.add_argument("--protected-evidence-only", action="store_true")

    authority = subparsers.add_parser("generate-authority")
    add_root(authority)
    add_output_root(authority)
    authority.add_argument("--rom", type=pathlib.Path, required=True)
    authority.add_argument(
        "--debugger-evidence", type=pathlib.Path, required=True
    )

    verify_authority_parser = subparsers.add_parser("verify-authority")
    add_root(verify_authority_parser)
    verify_authority_parser.add_argument(
        "--pack-root", type=pathlib.Path, required=True
    )
    verify_authority_parser.add_argument(
        "--rom", type=pathlib.Path, required=True
    )
    verify_authority_parser.add_argument(
        "--debugger-evidence", type=pathlib.Path, required=True
    )

    policy = subparsers.add_parser("generate-policy")
    add_root(policy)
    add_output_root(policy)
    policy.add_argument("--dataset-root", type=pathlib.Path, required=True)
    policy.add_argument("--evaluated-sha", required=True)

    run = subparsers.add_parser("run-profile")
    add_root(run)
    run.add_argument("--dataset-root", type=pathlib.Path, required=True)
    run.add_argument("--worker", type=pathlib.Path, required=True)
    run.add_argument("--scope", choices=("ci", "full"), required=True)
    run.add_argument(
        "--profile",
        choices=("architectural", "fingerprint"),
        required=True,
    )
    run.add_argument("--policy", choices=("g60a", "g60b"), required=True)
    run.add_argument("--output", type=pathlib.Path, required=True)
    run.add_argument(
        "--failure-directory", type=pathlib.Path, required=True
    )

    old = subparsers.add_parser("verify-old-policy")
    add_root(old)
    old.add_argument("--dataset-root", type=pathlib.Path, required=True)
    old.add_argument("--raw-summary", type=pathlib.Path, required=True)
    old.add_argument("--scope", choices=("ci", "full"), required=True)
    old.add_argument(
        "--profile",
        choices=("architectural", "fingerprint"),
        required=True,
    )

    new = subparsers.add_parser("verify-new-policy")
    add_root(new)
    new.add_argument("--dataset-root", type=pathlib.Path, required=True)
    new.add_argument("--old-raw-summary", type=pathlib.Path, required=True)
    new.add_argument("--new-raw-summary", type=pathlib.Path, required=True)
    new.add_argument("--scope", choices=("ci", "full"), required=True)
    new.add_argument(
        "--profile",
        choices=("architectural", "fingerprint"),
        required=True,
    )

    scoreboard = subparsers.add_parser("generate-scoreboard")
    add_root(scoreboard)
    add_output_root(scoreboard)
    scoreboard.add_argument(
        "--dataset-root", type=pathlib.Path, required=True
    )
    scoreboard.add_argument(
        "--raw-summary", type=pathlib.Path, required=True
    )
    scoreboard.add_argument(
        "--profile",
        choices=("architectural", "fingerprint"),
        required=True,
    )
    scoreboard.add_argument(
        "--scope", choices=("ci", "full"), required=True
    )
    scoreboard.add_argument("--evaluated-sha", required=True)
    scoreboard.add_argument(
        "--output-relative", type=pathlib.Path, required=True
    )
    scoreboard.add_argument(
        "--failure-directory-relative", type=pathlib.Path, required=True
    )

    transition = subparsers.add_parser("generate-transition")
    add_root(transition)
    add_output_root(transition)
    transition.add_argument(
        "--dataset-root", type=pathlib.Path, required=True
    )
    transition.add_argument(
        "--candidate-relative", type=pathlib.Path, required=True
    )
    transition.add_argument(
        "--output-relative", type=pathlib.Path, required=True
    )
    transition.add_argument(
        "--scope", choices=("ci", "full"), required=True
    )

    result = subparsers.add_parser("generate-result-manifest")
    add_root(result)
    add_output_root(result)
    result.add_argument("--dataset-root", type=pathlib.Path, required=True)
    result.add_argument("--old-arch-ci", type=pathlib.Path, required=True)
    result.add_argument("--old-arch-full", type=pathlib.Path, required=True)
    result.add_argument(
        "--old-fingerprint-full", type=pathlib.Path, required=True
    )

    ci = subparsers.add_parser("ci-enforce")
    add_root(ci)
    ci.add_argument("--dataset-root", type=pathlib.Path, required=True)
    ci.add_argument("--worker", type=pathlib.Path, required=True)
    ci.add_argument("--evaluated-sha", required=True)
    ci.add_argument("--output-root", type=pathlib.Path, required=True)
    return parser.parse_args(list(argv))


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        root = arguments.root.resolve()
        if arguments.command == "selftest":
            selftest(root)
        elif arguments.command == "verify-static":
            verify_static(root, arguments.protected_evidence_only)
        elif arguments.command == "generate-authority":
            generate_authority_pack(
                root,
                arguments.output_root.resolve(),
                arguments.rom.resolve(),
                arguments.debugger_evidence.resolve(),
            )
        elif arguments.command == "verify-authority":
            verify_authority_with_rom(
                root,
                arguments.pack_root.resolve(),
                arguments.rom.resolve(),
                arguments.debugger_evidence.resolve(),
            )
        elif arguments.command == "generate-policy":
            generate_target_policy(
                root,
                arguments.output_root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.evaluated_sha,
            )
        elif arguments.command == "run-profile":
            run_profile(
                root,
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.scope,
                arguments.profile,
                arguments.policy,
                arguments.output.resolve(),
                arguments.failure_directory.resolve(),
            )
        elif arguments.command == "verify-old-policy":
            result = verify_old_policy_raw(
                root,
                arguments.dataset_root.resolve(),
                arguments.raw_summary.resolve(),
                arguments.profile,
                arguments.scope,
            )
            print(
                "m60b-old-policy: "
                + canonical_bytes(result).decode("ascii")
            )
        elif arguments.command == "verify-new-policy":
            result = verify_new_policy_raw(
                root,
                arguments.dataset_root.resolve(),
                arguments.old_raw_summary.resolve(),
                arguments.new_raw_summary.resolve(),
                arguments.profile,
                arguments.scope,
            )
            print(
                "m60b-new-policy: "
                + canonical_bytes(result).decode("ascii")
            )
        elif arguments.command == "generate-scoreboard":
            generate_candidate_scoreboard(
                root,
                arguments.output_root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.raw_summary.resolve(),
                arguments.profile,
                arguments.scope,
                arguments.evaluated_sha,
                arguments.output_relative,
                arguments.failure_directory_relative,
            )
        elif arguments.command == "generate-transition":
            generate_transition(
                root,
                arguments.output_root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.candidate_relative,
                arguments.output_relative,
                arguments.scope,
            )
        elif arguments.command == "generate-result-manifest":
            generate_result_manifest(
                root,
                arguments.output_root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.old_arch_ci.resolve(),
                arguments.old_arch_full.resolve(),
                arguments.old_fingerprint_full.resolve(),
            )
        elif arguments.command == "ci-enforce":
            ci_enforce(
                root,
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.evaluated_sha,
                arguments.output_root.resolve(),
            )
        else:
            raise AssertionError(arguments.command)
    except (
        M60bError,
        OSError,
        UnicodeError,
        json.JSONDecodeError,
        ratchet.RatchetError,
        ssts.CorpusError,
    ) as error:
        print(f"m60b-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

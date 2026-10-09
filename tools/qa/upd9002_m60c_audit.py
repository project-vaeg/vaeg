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
"""Generate and verify the G60c FPO2/main-dispatch authority audit."""

from __future__ import annotations

import argparse
import copy
import gzip
import hashlib
import json
import pathlib
import platform
import re
import subprocess
import sys
import tempfile
import zlib
from collections import Counter, defaultdict
from typing import Any, Callable

import upd9002_m60b_authority as m60b
import upd9002_m60c_erratum as erratum
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M60c"
CANDIDATE_GATE = "G60c"
APPROVED_PREDECESSOR_GATE = "G60b"
# Evidence keeps the original identity; Git topology follows rewritten history.
APPROVED_PREDECESSOR_SHA = "4e5d74d0d9f675df2342353b8bfdbb2e5cded768"
APPROVED_PREDECESSOR_GIT_SHA = "77d81230c84a51426ebd0115153fc4f4ba76e6f8"
G60B_EVALUATED_SHA = "23c5de2a7d28b35dd184201dee8d101607178510"
G60B_CI_URL = "https://github.com/nakatamaho/vaeg/actions/runs/30144447279"
G60B_TARGET_POLICY_ID = (
    "upd9002-g60b-"
    "eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93"
)
G60B_TARGET_POLICY_SHA256 = (
    "eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93"
)
G60B_AUTHORITY_MANIFEST_SHA256 = (
    "f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac"
)
G60B_TRANSITION_SHA256 = (
    "2396a03ec5b64033406f200458ef9d3d1e8bf805a551385c8bfbb07ed9493cdd"
)
G60B_ARTIFACT_TREE_SHA256 = (
    "af1d979faa3d75019e3df6d419f6caf870c33dbb19b18a5c0d8010f33bd695c5"
)
DATASET_ID = (
    "ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-"
    "1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4"
)
COMPARISON_CONTRACTS = {
    "architectural": {
        "id": "upd9002-v20-architectural-v1",
        "sha256": (
            "aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5"
        ),
    },
    "fingerprint": {
        "id": "upd9002-v20-fingerprint-v1",
        "sha256": (
            "47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e"
        ),
    },
}
SELECTED_HASH_SETS = {
    "ci": "d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6",
    "full": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
}
APPLICABLE_HASH_SETS = {
    "ci": "5a00d3fa15d55b38630015e771163fe58aff544d5f13342beac01a8236a485a1",
    "full": "a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65",
}

AUTHORITY_ROOT = pathlib.Path("tests/ssts/authority/g60c")
RESULT_MANIFEST_PATH = pathlib.Path(
    "tests/ssts/authority/g60c_result_manifest.json"
)
TRANSITION_PATH = pathlib.Path(
    "tests/ssts/transitions/g60c_target_authority_from_g60b.json"
)
SUPPORT_MAP_PATH = pathlib.Path(
    "tools/qa/golden/upd9002_support_map_m48.csv"
)
DATASET_MANIFEST_PATH = pathlib.Path("tests/ssts/v20_dataset_manifest.json")

ROM_SHA256 = m60b.ROM_SHA256
ROM_SIZE = m60b.ROM_SIZE
ROM_SHA1 = m60b.ROM_SHA1
ROM_CRC32 = m60b.ROM_CRC32
ROM_BANK_FILE_BASE = m60b.ROM_BANK_FILE_BASE

GROUP_START = 0x668FD
GROUP_END = 0x66921
GROUP_RECORD_COUNT = 12
GROUP_HANDLER_POINTER_START = 0x662FF
GROUP_HANDLER_POINTER_END = 0x66311
GROUP_SEGMENT_HANDLER_START = 0x660BC
GROUP_SEGMENT_HANDLER_END = 0x660DC

DECODER_ENTRY_START = 0x658F3
DECODER_ENTRY_END = 0x6592C
MAIN_DECODER_START = 0x6592C
MAIN_DECODER_END = 0x6598D
GROUP_DECODER_START = 0x6598D
GROUP_DECODER_END = 0x659D3
EXTENSION_DECODER_START = 0x659D3
EXTENSION_DECODER_END = 0x65A3C
FPU_DECODER_START = 0x65A3C
FPU_DECODER_END = 0x65B2C
FPU_MATCHER_START = 0x65B2C
FPU_MATCHER_END = 0x65B79

FPU_TABLES = (
    {
        "group_handler_pointer_end": 0x66325,
        "group_handler_pointer_start": 0x6631B,
        "mnemonic_end": 0x66D59,
        "mnemonic_start": 0x66CF4,
        "name": "memory-arithmetic",
        "record_count": 22,
        "start": 0x66B3C,
        "end": 0x66BAA,
    },
    {
        "group_handler_pointer_end": 0x66331,
        "group_handler_pointer_start": 0x66325,
        "mnemonic_end": 0x66D98,
        "mnemonic_start": 0x66D59,
        "name": "memory-load-store-environment",
        "record_count": 13,
        "start": 0x66BAA,
        "end": 0x66BEB,
    },
    {
        "group_handler_pointer_end": 0x66337,
        "group_handler_pointer_start": 0x66331,
        "mnemonic_end": 0x66E00,
        "mnemonic_start": 0x66D98,
        "name": "register-arithmetic",
        "record_count": 23,
        "start": 0x66BEB,
        "end": 0x66C5E,
    },
    {
        "group_handler_pointer_end": 0x66339,
        "group_handler_pointer_start": 0x66337,
        "mnemonic_end": 0x66E9A,
        "mnemonic_start": 0x66E00,
        "name": "register-constant-transcendental-control",
        "record_count": 29,
        "start": 0x66C5E,
        "end": 0x66CEF,
    },
)

EXPECTED_GROUP_RECORDS = (
    "fef600",
    "ffff01",
    "ff8f02",
    "fefe03",
    "fef604",
    "fc8005",
    "fe8006",
    "fcd007",
    "fcd007",
    "fec007",
    "fec007",
    "e72608",
)
EXPECTED_GROUP_HANDLER_POINTERS = (
    "0x5fbe",
    "0x5fd4",
    "0x5ff3",
    "0x5ff9",
    "0x601c",
    "0x6022",
    "0x6067",
    "0x606d",
    "0x60bc",
)
EXPECTED_GROUP_OVERLAPS = {
    "0x80": ((5, "0x05"), (6, "0x06")),
    "0x81": ((5, "0x05"), (6, "0x06")),
    "0xc0": ((9, "0x07"), (10, "0x07")),
    "0xc1": ((9, "0x07"), (10, "0x07")),
    "0xd0": ((7, "0x07"), (8, "0x07")),
    "0xd1": ((7, "0x07"), (8, "0x07")),
    "0xd2": ((7, "0x07"), (8, "0x07")),
    "0xd3": ((7, "0x07"), (8, "0x07")),
    "0xf6": ((0, "0x00"), (4, "0x04")),
    "0xf7": ((0, "0x00"), (4, "0x04")),
    "0xff": ((1, "0x01"), (3, "0x03")),
}

EXPECTED_FPU_MNEMONICS = {
    "F2XM1",
    "FABS",
    "FADD",
    "FADDP",
    "FBLD",
    "FBSTP",
    "FCHS",
    "FCLEX",
    "FCOM",
    "FCOMP",
    "FCOMPP",
    "FDECSTP",
    "FDISI",
    "FDIV",
    "FDIVP",
    "FDIVR",
    "FDIVRP",
    "FENI",
    "FFREE",
    "FIADD",
    "FICOM",
    "FICOMP",
    "FIDIV",
    "FIDIVR",
    "FILD",
    "FIMUL",
    "FINCSTP",
    "FINIT",
    "FIST",
    "FISTP",
    "FISUB",
    "FISUBR",
    "FLD",
    "FLD1",
    "FLDCW",
    "FLDENV",
    "FLDL2E",
    "FLDL2T",
    "FLDLG2",
    "FLDLN2",
    "FLDPI",
    "FLDZ",
    "FMUL",
    "FMULP",
    "FNOP",
    "FPATAN",
    "FPREM",
    "FPTAN",
    "FRNDINT",
    "FRSTOR",
    "FSAVE",
    "FSCALE",
    "FSQRT",
    "FST",
    "FSTCW",
    "FSTENV",
    "FSTP",
    "FSTSW",
    "FSUB",
    "FSUBP",
    "FSUBR",
    "FSUBRP",
    "FTST",
    "FXAM",
    "FXCH",
    "FXTRACT",
    "FYL2X",
    "FYL2XP1",
}

PROFILE_IDENTITIES = {
    "architectural_ci": {
        "applicable": 165300,
        "applicable_hash_set_sha256": (
            "5a00d3fa15d55b38630015e771163fe58aff544d5f13342beac01a8236a485a1"
        ),
        "executed": 165300,
        "fail": 8121,
        "failure_hash_set_sha256": (
            "04ffb31baf4f66e60f0e700ebe8713ad7b1d582352160dc02c61415619426603"
        ),
        "failure_signature_index_sha256": (
            "a67c9135780ce1df35486b2c2a28fe3e309be97c30045b4761ed5a8c652a4132"
        ),
        "pass": 157179,
        "pass_hash_set_sha256": (
            "ae83f89ed1bfe34088197012683c726d9dab5a76700bb247efcf9a7e6e0d8bff"
        ),
        "selected": 180000,
        "selected_hash_set_sha256": (
            "d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6"
        ),
    },
    "architectural_full": {
        "applicable": 1438594,
        "applicable_hash_set_sha256": (
            "a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65"
        ),
        "executed": 1438594,
        "fail": 59941,
        "failure_hash_set_sha256": (
            "9c69f518d168e1675e4d1fcf59db031dc1bce058db3fdfe9f6cb66b8d2bec91d"
        ),
        "failure_signature_index_sha256": (
            "776c17c19e52cc03becc83464dfcdcb0b1590532812deac7e93c8955f0c1a473"
        ),
        "pass": 1378653,
        "pass_hash_set_sha256": (
            "898b7e6e66fa9ea4e475bfc78db5a0cf8c3d6b109276fac6db0f68e05a1c27f2"
        ),
        "selected": 1562502,
        "selected_hash_set_sha256": (
            "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7"
        ),
    },
    "fingerprint_full": {
        "applicable": 1438594,
        "applicable_hash_set_sha256": (
            "a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65"
        ),
        "executed": 1438594,
        "fail": 162379,
        "failure_hash_set_sha256": (
            "2d825411e958ec980317d0c29d06aee8f9e7337c036f1d5703014311e53c2cc4"
        ),
        "failure_signature_index_sha256": (
            "84c40271c91cb8d01e3545ada60c50e35594e790ce29e9b4d609c75e1d59d3bb"
        ),
        "pass": 1276215,
        "pass_hash_set_sha256": (
            "691242bb0a05324fe3be261653863db45f45fd495c83d6798d8491a3e8db42db"
        ),
        "selected": 1562502,
        "selected_hash_set_sha256": (
            "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7"
        ),
    },
}

SST_EXPECTED_DIGESTS = {
    "66": {
        "ci_record": "097473645186cac101e486b4aa4e4251c350cbfabb22c2de3e5cde88e26c5237",
        "ci_upstream": "97eebdca07524d67df6060224d21f92b4497730ac78de97f7393cf28525c0b71",
        "full_record": "3363f95f044fb79587633d0958549e10ce6c92f9589cd58f934cee4d83d3e443",
        "full_upstream": "b152b91a7de513d6d04d52b7ceeca97fd4cbdfd91d1839635bcbac24f9a20c53",
    },
    "67": {
        "ci_record": "0f4f361b232e6b799acd8562855c1050005fbc9ceeda5a7f8eba63983d36d709",
        "ci_upstream": "8deb20f385ae8c6afa61e44eee9684e9f103d7509cf3fc58cf52c8569eab1c02",
        "full_record": "2ffee1efb6ec206ddab14ed3ded2cb526009d9415b58fba65f4a8afdf40abb7f",
        "full_upstream": "95cbfde2144580fad8db102bd688cadae5354acfc973f1b33b9e33921a6493be",
    },
}
SST_COMBINED_DIGESTS = {
    "ci": "e118526ebc141af8ba63b993c0f1d5027eed5e3ae383a4b2c95742d368071988",
    "full": "9619ad38620df14f4f5c1e4e34c5b811631dc92d0046d0597eb4bd3b9b06b58f",
}


class M60cError(ValueError):
    """Raised when an M60c authority or governance invariant fails closed."""


def canonical_bytes(value: Any) -> bytes:
    return ratchet.canonical_bytes(value)


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    return ratchet.sha256_file(path)


def identity_digest(value: Any) -> str:
    return sha256_bytes(canonical_bytes(value))


def write_json(path: pathlib.Path, value: Any) -> None:
    ratchet.write_json(path, value)


def read_json(path: pathlib.Path) -> Any:
    return ratchet.read_json(path)


def hex_offset(value: int) -> str:
    return f"0x{value:05x}"


def read_u16le(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset:offset + 2], "little")


def require_sha(value: str, field: str) -> str:
    try:
        return ratchet.require_sha(value, field)
    except ratchet.RatchetError as error:
        raise M60cError(str(error)) from error


def validate_analysis_sha(value: str) -> None:
    require_sha(value, "analysis/evaluated SHA")
    if value == APPROVED_PREDECESSOR_SHA:
        raise M60cError("analysis/evaluated SHA must identify the M60c implementation")


def validate_rom(data: bytes) -> None:
    if len(data) != ROM_SIZE:
        raise M60cError(f"ROM size differs: {len(data)}")
    if sha256_bytes(data) != ROM_SHA256:
        raise M60cError("ROM SHA-256 differs")


def bounded_slice(data: bytes, start: int, end: int, label: str) -> bytes:
    if start < 0 or end <= start or end > len(data):
        raise M60cError(f"{label}: invalid or truncated boundary")
    return data[start:end]


def parse_group_dispatch(data: bytes) -> tuple[dict[str, Any], dict[str, Any]]:
    if GROUP_END - GROUP_START != GROUP_RECORD_COUNT * 3:
        raise M60cError("group dispatch boundary/count contract differs")
    try:
        records = m60b.parse_records(
            data, GROUP_START, GROUP_END, GROUP_RECORD_COUNT, "M60c group dispatch"
        )
    except m60b.M60bError as error:
        raise M60cError(str(error)) from error
    if tuple(item["raw"] for item in records) != EXPECTED_GROUP_RECORDS:
        raise M60cError("group dispatch raw record inventory differs")

    owners: dict[str, list[tuple[int, str]]] = defaultdict(list)
    decoded = []
    for record in records:
        expanded = m60b.expand_record(record)
        for opcode in expanded:
            owners[opcode].append((record["index"], record["group"]))
        decoded.append(
            {
                "expanded_primary_opcodes": expanded,
                "group": record["group"],
                "index": record["index"],
                "ordered_candidate": True,
            }
        )
    overlaps = {
        opcode: tuple(entries)
        for opcode, entries in sorted(owners.items())
        if len(entries) > 1
    }
    if overlaps != EXPECTED_GROUP_OVERLAPS:
        raise M60cError("group dispatch overlap ownership differs")

    pointer_bytes = bounded_slice(
        data,
        GROUP_HANDLER_POINTER_START,
        GROUP_HANDLER_POINTER_END,
        "group handler pointer table",
    )
    pointers = tuple(
        f"0x{read_u16le(pointer_bytes, offset):04x}"
        for offset in range(0, len(pointer_bytes), 2)
    )
    if pointers != EXPECTED_GROUP_HANDLER_POINTERS:
        raise M60cError("group handler pointer mapping differs")

    segment_record = decoded[11]
    if segment_record["expanded_primary_opcodes"] != [
        "0x26",
        "0x2e",
        "0x36",
        "0x3e",
    ] or segment_record["group"] != "0x08":
        raise M60cError("e7/26 segment-override expansion differs")
    segment_code = bounded_slice(
        data,
        GROUP_SEGMENT_HANDLER_START,
        GROUP_SEGMENT_HANDLER_END,
        "group 8 segment handler",
    )
    required_segment_code = (
        bytes.fromhex("b103d2e82403a39608"),
        bytes.fromhex("c606950800"),
        bytes.fromhex("c68407013a"),
    )
    if not all(pattern in segment_code for pattern in required_segment_code):
        raise M60cError("group 8 handler does not prove segment-override ownership")

    raw_document = {
        "end_exclusive": hex_offset(GROUP_END),
        "handler_pointer_end_exclusive": hex_offset(GROUP_HANDLER_POINTER_END),
        "handler_pointer_raw": pointer_bytes.hex(),
        "handler_pointer_sha256": sha256_bytes(pointer_bytes),
        "handler_pointer_start": hex_offset(GROUP_HANDLER_POINTER_START),
        "raw_record_count": len(records),
        "raw_records": records,
        "raw_slice_sha256": sha256_bytes(data[GROUP_START:GROUP_END]),
        "record_width": 3,
        "schema": "vaeg-upd9002-m60c-group-dispatch-raw-v1",
        "schema_version": 1,
        "start": hex_offset(GROUP_START),
    }
    decoded_document = {
        "decoded_records": decoded,
        "group_handler_pointers": list(pointers),
        "ordered_overlap_candidates": [
            {
                "owners": [
                    {"group": group, "record_index": index}
                    for index, group in entries
                ],
                "primary_opcode": opcode,
            }
            for opcode, entries in sorted(overlaps.items())
        ],
        "record_interpretation": (
            "Ordered (mask, value, group) primary-opcode candidates; the "
            "decoder invokes the candidate matcher before selecting a group handler."
        ),
        "schema": "vaeg-upd9002-m60c-group-dispatch-decoded-v1",
        "schema_version": 1,
        "segment_override_proof": {
            "expanded_primary_opcodes": segment_record[
                "expanded_primary_opcodes"
            ],
            "group": "0x08",
            "handler_code_end_exclusive": hex_offset(
                GROUP_SEGMENT_HANDLER_END
            ),
            "handler_code_sha256": sha256_bytes(segment_code),
            "handler_code_start": hex_offset(GROUP_SEGMENT_HANDLER_START),
            "handler_pointer": "0x60bc",
            "interpretation": (
                "The handler derives a two-bit segment selector from opcode "
                "bits 3-4, stores it, emits the selected segment mnemonic, "
                "and appends a colon."
            ),
            "proven": True,
            "raw_record": "e72608",
        },
    }
    return raw_document, decoded_document


def parse_fpu_record_table(
    data: bytes, table: dict[str, Any], table_index: int
) -> tuple[list[dict[str, Any]], list[dict[str, Any]], list[str]]:
    raw = bounded_slice(data, table["start"], table["end"], table["name"])
    if len(raw) != table["record_count"] * 5:
        raise M60cError(f"{table['name']}: FPU record boundary/count differs")
    try:
        mnemonics, mnemonic_end = m60b.decode_high_bit_strings(
            data,
            table["mnemonic_start"],
            table["record_count"],
            f"{table['name']} mnemonics",
        )
    except m60b.M60bError as error:
        raise M60cError(str(error)) from error
    if mnemonic_end != table["mnemonic_end"]:
        raise M60cError(f"{table['name']}: mnemonic boundary differs")

    pointer_raw = bounded_slice(
        data,
        table["group_handler_pointer_start"],
        table["group_handler_pointer_end"],
        f"{table['name']} group handler table",
    )
    if len(pointer_raw) % 2:
        raise M60cError(f"{table['name']}: malformed handler pointer table")
    handler_pointers = [
        f"0x{read_u16le(pointer_raw, offset):04x}"
        for offset in range(0, len(pointer_raw), 2)
    ]

    raw_records = []
    decoded = []
    for index in range(table["record_count"]):
        offset = index * 5
        mask = int.from_bytes(raw[offset:offset + 2], "little")
        value = int.from_bytes(raw[offset + 2:offset + 4], "little")
        group = raw[offset + 4]
        if value & mask != value:
            raise M60cError(f"{table['name']}: value has bits outside mask")
        if group >= len(handler_pointers):
            raise M60cError(f"{table['name']}: missing group/handler link")
        forms = []
        for opcode in range(0xD8, 0xE0):
            for following in range(256):
                word = (opcode << 8) | following
                if word & mask == value:
                    forms.append(f"0x{word:04x}")
        if not forms:
            raise M60cError(f"{table['name']}: record has no D8-DF expansion")
        raw_records.append(
            {
                "group": f"0x{group:02x}",
                "index": index,
                "mask16_le": f"0x{mask:04x}",
                "offset": hex_offset(table["start"] + offset),
                "raw": raw[offset:offset + 5].hex(),
                "value16_le": f"0x{value:04x}",
            }
        )
        decoded.append(
            {
                "expanded_opcode_modrm_words": forms,
                "group": f"0x{group:02x}",
                "handler_pointer": handler_pointers[group],
                "index": index,
                "mnemonic": mnemonics[index]["text"],
                "owner": f"table-{table_index}-record-{index:02d}",
                "primary_opcodes": sorted(
                    {f"0x{int(item[2:4], 16):02x}" for item in forms}
                ),
            }
        )
    return raw_records, decoded, handler_pointers


def parse_fpu_authority(
    data: bytes,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    raw_tables = []
    decoded_tables = []
    mnemonic_rows = []
    owners = set()
    all_primary = set()
    for table_index, table in enumerate(FPU_TABLES, start=1):
        raw_records, decoded, handler_pointers = parse_fpu_record_table(
            data, table, table_index
        )
        raw_slice = data[table["start"]:table["end"]]
        mnemonic_slice = data[table["mnemonic_start"]:table["mnemonic_end"]]
        pointer_slice = data[
            table["group_handler_pointer_start"]:
            table["group_handler_pointer_end"]
        ]
        raw_tables.append(
            {
                "end_exclusive": hex_offset(table["end"]),
                "group_handler_pointer_end_exclusive": hex_offset(
                    table["group_handler_pointer_end"]
                ),
                "group_handler_pointer_sha256": sha256_bytes(pointer_slice),
                "group_handler_pointer_start": hex_offset(
                    table["group_handler_pointer_start"]
                ),
                "group_handler_pointers": handler_pointers,
                "name": table["name"],
                "raw_record_count": len(raw_records),
                "raw_records": raw_records,
                "raw_slice_sha256": sha256_bytes(raw_slice),
                "record_width": 5,
                "search_order": 5 - table_index,
                "start": hex_offset(table["start"]),
            }
        )
        decoded_tables.append(
            {
                "decoded_records": decoded,
                "name": table["name"],
                "search_order": 5 - table_index,
            }
        )
        for item in decoded:
            if item["owner"] in owners:
                raise M60cError("duplicate FPU mnemonic ownership")
            owners.add(item["owner"])
            all_primary.update(item["primary_opcodes"])
        decoded_strings, _ = m60b.decode_high_bit_strings(
            data,
            table["mnemonic_start"],
            table["record_count"],
            f"{table['name']} mnemonic offsets",
        )
        if len(decoded) != len(decoded_strings):
            raise M60cError("FPU mnemonic decoded-string count mismatch")
        for index, (item, decoded_string) in enumerate(
            zip(decoded, decoded_strings)
        ):
            mnemonic_rows.append(
                {
                    "address": decoded_string["offset"],
                    "mnemonic": item["mnemonic"],
                    "owner": item["owner"],
                    "raw": decoded_string["raw"],
                    "table": table["name"],
                    "table_record_index": index,
                }
            )

    if all_primary != {f"0x{opcode:02x}" for opcode in range(0xD8, 0xE0)}:
        raise M60cError("FPU records do not own exactly D8-DF")
    if any(opcode in all_primary for opcode in {"0x66", "0x67"}):
        raise M60cError("FPU record inventory unexpectedly owns 66/67")
    mnemonic_inventory = {row["mnemonic"] for row in mnemonic_rows}
    if mnemonic_inventory != EXPECTED_FPU_MNEMONICS:
        raise M60cError("FPU mnemonic inventory is incomplete")
    if len(owners) != sum(table["record_count"] for table in FPU_TABLES):
        raise M60cError("FPU record/mnemonic ownership is incomplete")

    raw_document = {
        "record_format": "(mask16-le, value16-le, group8)",
        "schema": "vaeg-upd9002-m60c-fpu-dispatch-raw-v1",
        "schema_version": 1,
        "tables": raw_tables,
        "total_raw_record_count": sum(
            table["raw_record_count"] for table in raw_tables
        ),
    }
    decoded_document = {
        "matched_word": (
            "The decoder swaps the fetched word so the primary opcode is "
            "the high byte and the following instruction byte is the low byte."
        ),
        "primary_opcode_inventory": sorted(all_primary),
        "schema": "vaeg-upd9002-m60c-fpu-dispatch-decoded-v1",
        "schema_version": 1,
        "search_order": [
            "register-constant-transcendental-control",
            "register-arithmetic",
            "memory-load-store-environment",
            "memory-arithmetic",
            "D8-DF register-ModR/M fallback",
        ],
        "tables": sorted(decoded_tables, key=lambda item: item["search_order"]),
        "total_expanded_form_count": sum(
            len(record["expanded_opcode_modrm_words"])
            for table in decoded_tables
            for record in table["decoded_records"]
        ),
        "total_raw_record_count": sum(
            len(table["decoded_records"]) for table in decoded_tables
        ),
    }
    mnemonic_document = {
        "encoding": "ASCII with bit 7 set on the final byte",
        "individual_mnemonic_count": len(EXPECTED_FPU_MNEMONICS),
        "individual_mnemonics": sorted(EXPECTED_FPU_MNEMONICS),
        "ownership_rows": sorted(
            mnemonic_rows, key=lambda item: (item["table"], item["table_record_index"])
        ),
        "record_mnemonic_count": len(mnemonic_rows),
        "schema": "vaeg-upd9002-m60c-fpu-mnemonic-map-v1",
        "schema_version": 1,
    }
    return raw_document, decoded_document, mnemonic_document


def code_range(
    data: bytes,
    start: int,
    end: int,
    incoming: str,
    outgoing: str,
    pseudocode: list[str],
    accepts_66: bool,
    accepts_67: bool,
    bytes_after_primary: str,
) -> dict[str, Any]:
    raw = bounded_slice(data, start, end, f"decoder path {start:05x}")
    return {
        "accepts_primary_0x66": accepts_66,
        "accepts_primary_0x67": accepts_67,
        "bytes_consumed_after_primary": bytes_after_primary,
        "end_exclusive": hex_offset(end),
        "incoming_condition": incoming,
        "neutral_pseudocode": pseudocode,
        "outgoing": outgoing,
        "raw_bytes": raw.hex(),
        "raw_sha256": sha256_bytes(raw),
        "reachable_from_normal_disassembler_entry": True,
        "start": hex_offset(start),
    }


def build_decoder_paths(
    data: bytes,
    group_decoded: dict[str, Any],
    fpu_decoded: dict[str, Any],
) -> dict[str, Any]:
    entry = bounded_slice(
        data, DECODER_ENTRY_START, DECODER_ENTRY_END, "disassembler entry"
    )
    main = bounded_slice(data, MAIN_DECODER_START, MAIN_DECODER_END, "main decoder")
    group = bounded_slice(
        data, GROUP_DECODER_START, GROUP_DECODER_END, "group decoder"
    )
    extension = bounded_slice(
        data,
        EXTENSION_DECODER_START,
        EXTENSION_DECODER_END,
        "0f extension decoder",
    )
    fpu = bounded_slice(data, FPU_DECODER_START, FPU_DECODER_END, "FPU decoder")
    matcher = bounded_slice(
        data, FPU_MATCHER_START, FPU_MATCHER_END, "FPU table matcher"
    )
    if bytes.fromhex("bf50638b3647083c0f") not in main:
        raise M60cError("normal entry/main-table control flow differs")
    if bytes.fromhex("bffc68478b1e4d08") not in group:
        raise M60cError("main-miss/group-table control flow differs")
    if not extension.startswith(bytes.fromhex("3c0f")):
        raise M60cError("group-miss 0f/FPU branch differs")
    required_fpu = (
        bytes.fromhex("268b0786e0"),
        bytes.fromhex("80e4f880fcd8"),
        bytes.fromhex("24c03cc0"),
    )
    if not all(pattern in fpu for pattern in required_fpu):
        raise M60cError("FPU alternate/fallback control flow differs")
    if bytes.fromhex("2e2315") not in matcher or bytes.fromhex("2e3b15") not in matcher:
        raise M60cError("FPU mask/value matcher differs")

    group_union = {
        opcode
        for row in group_decoded["decoded_records"]
        for opcode in row["expanded_primary_opcodes"]
    }
    fpu_union = set(fpu_decoded["primary_opcode_inventory"])
    if {"0x66", "0x67"} & group_union:
        raise M60cError("group dispatch recognizes 66/67")
    if {"0x66", "0x67"} & fpu_union:
        raise M60cError("FPU tables recognize 66/67")

    paths = [
        code_range(
            data,
            DECODER_ENTRY_START,
            DECODER_ENTRY_END,
            "normal disassembler entry",
            "ordinary primary dispatch",
            [
                "fetch the current instruction byte",
                "invoke the primary dispatcher",
            ],
            False,
            False,
            "delegated",
        ),
        code_range(
            data,
            MAIN_DECODER_START,
            MAIN_DECODER_END,
            "normal entry with a primary byte",
            "matching main record, group dispatch on miss, or 0f path",
            [
                "scan the bounded ordinary primary record table",
                "invoke the record-specific matcher",
                "on complete miss enter group dispatch",
            ],
            False,
            False,
            "record-specific",
        ),
        code_range(
            data,
            GROUP_DECODER_START,
            GROUP_DECODER_END,
            "ordinary primary table miss",
            "matching group handler or extension/FPU path on miss",
            [
                "scan all twelve ordered group candidates",
                "invoke the candidate matcher before selecting its handler",
                "on complete miss test the 0f extension path",
            ],
            False,
            False,
            "record-specific",
        ),
        code_range(
            data,
            EXTENSION_DECODER_START,
            EXTENSION_DECODER_END,
            "group table miss",
            "0f table when primary is 0f; otherwise FPU alternate path",
            [
                "if primary equals 0f scan the complete 0f table",
                "otherwise enter the alternate FPU decoder",
            ],
            False,
            False,
            "0f consumes extension byte; alternate path fetches following byte",
        ),
        code_range(
            data,
            FPU_DECODER_START,
            FPU_DECODER_END,
            "non-0f ordinary/group-table miss",
            "four FPU tables, then D8-DF register fallback, then unknown",
            [
                "fetch the following instruction byte",
                "form primary:following as a 16-bit match word",
                "search all four bounded FPU tables",
                "on miss require primary in D8-DF and register ModR/M",
                "otherwise reject as unknown",
            ],
            False,
            False,
            "one following byte before table/fallback decision",
        ),
        code_range(
            data,
            FPU_MATCHER_START,
            FPU_MATCHER_END,
            "one of four FPU table searches",
            "matched group handler or next FPU table",
            [
                "apply 16-bit mask to primary:following word",
                "compare with the table value",
                "on match select the linked group handler and mnemonic",
            ],
            False,
            False,
            "ModR/M/operand parsing delegated to matched handler",
        ),
    ]
    return {
        "all_reachable_alternatives_bounded": True,
        "fallback_primary_range": ["0xd8", "0xdf"],
        "fpu_table_primary_inventory": sorted(fpu_union),
        "group_primary_inventory": sorted(group_union),
        "normal_entry_code_sha256": sha256_bytes(entry),
        "paths": paths,
        "primary_0x66_result": "rejected-as-unknown",
        "primary_0x67_result": "rejected-as-unknown",
        "schema": "vaeg-upd9002-m60c-decoder-paths-v1",
        "schema_version": 1,
    }


def support_row_for_dispatch(
    support: dict[tuple[str, int, str], dict[str, str]],
    dispatch: dict[str, Any],
) -> dict[str, str]:
    key = (
        dispatch["support_mode"],
        int(dispatch["opcode"], 16),
        dispatch["support_subopcode"],
    )
    row = support.get(key)
    if row is None:
        raise M60cError("66/67 support-map ownership is missing")
    return dict(sorted(row.items()))


def extract_sst_audit(
    root: pathlib.Path, dataset_root: pathlib.Path
) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    ssts.verify_fast(dataset_root, manifest)
    metadata = json.loads(
        (dataset_root / ssts.SUITE_PATH / "metadata.json").read_text(
            encoding="utf-8"
        )
    )
    ssts.validate_metadata(metadata)
    support = ssts.load_support_map(root / SUPPORT_MAP_PATH)

    all_rows = []
    summaries = {}
    combined_ci = {}
    combined_full = {}
    for form in ("66", "67"):
        path = dataset_root / ssts.SUITE_PATH / f"{form}.json.gz"
        if not path.is_file():
            raise M60cError(f"missing SST corpus shard {form}")
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            records = json.load(stream)
        full_records = ssts.profile_records(records, "full")
        ci_records = ssts.profile_records(records, "ci")
        ci_hashes = {
            sha256_bytes(canonical_bytes(record)) for record in ci_records
        }
        by_record = {}
        by_upstream = {}
        prefix_counts: Counter[str] = Counter()
        mod_counts: Counter[str] = Counter()
        length_counts: Counter[str] = Counter()
        metadata_names: Counter[str] = Counter()
        for raw_record in full_records:
            record = ssts.validate_record(
                raw_record, f"{form}:{raw_record.get('idx')}"
            )
            primary, prefixes = m60b.decode_primary_opcode(record["bytes"])
            if primary != int(form, 16):
                raise M60cError(f"{form}: first-byte-only or wrong primary selection")
            classification = ssts.classify_record(
                form, record, metadata, support
            )
            if (
                classification["classification"] != "upstream_nonblocking"
                or classification["metadata_status"] != "fpu"
                or classification["metadata_arch"] != "v30"
                or classification["dispatch"] is None
            ):
                raise M60cError(f"{form}: classification/metadata ownership differs")
            dispatch = classification["dispatch"]
            support_row = support_row_for_dispatch(support, dispatch)
            if (
                support_row["target"] != "v30_reserved"
                or support_row["classification"] != "known_target_gap"
            ):
                raise M60cError(f"{form}: support-map row differs")
            record_hash = sha256_bytes(canonical_bytes(record))
            if record_hash in by_record or record["hash"] in by_upstream:
                raise M60cError(f"{form}: duplicate selected case hash")
            prefix_class = (
                "unprefixed"
                if not prefixes
                else "+".join(f"{byte:02x}" for byte in prefixes)
            )
            modrm_index = len(prefixes) + 1
            if modrm_index >= len(record["bytes"]):
                raise M60cError(f"{form}: missing FPO2 ModR/M byte")
            modrm = record["bytes"][modrm_index]
            row = {
                "architectural_ci": {
                    "executed": False,
                    "fail": 0,
                    "pass": 0,
                    "selected": record_hash in ci_hashes,
                },
                "architectural_full": {
                    "executed": False,
                    "fail": 0,
                    "pass": 0,
                    "selected": True,
                },
                "classification_ownership": (
                    "upstream metadata status fpu takes precedence over the "
                    "support-map known-target-gap row"
                ),
                "complete_instruction_bytes": "".join(
                    f"{byte:02x}" for byte in record["bytes"]
                ),
                "dispatch": dispatch,
                "fingerprint_full": {
                    "executed": False,
                    "fail": 0,
                    "pass": 0,
                    "selected": True,
                },
                "gap_kind": None,
                "modrm": f"0x{modrm:02x}",
                "modrm_mod": (modrm >> 6) & 3,
                "modrm_reg": (modrm >> 3) & 7,
                "prefix_sequence": [f"0x{byte:02x}" for byte in prefixes],
                "primary_opcode": f"0x{primary:02x}",
                "record_hash": record_hash,
                "support_map_row": support_row,
                "top_level_classification": classification["classification"],
                "upstream_metadata_architecture": classification["metadata_arch"],
                "upstream_metadata_mnemonic": record["name"],
                "upstream_metadata_status": classification["metadata_status"],
                "upstream_test_hash": record["hash"],
            }
            by_record[record_hash] = row
            by_upstream[record["hash"]] = row
            all_rows.append(row)
            combined_full[record_hash] = row
            if record_hash in ci_hashes:
                combined_ci[record_hash] = row
            prefix_counts[prefix_class] += 1
            mod_counts[str((modrm >> 6) & 3)] += 1
            length_counts[str(len(record["bytes"]))] += 1
            metadata_names[record["name"].split(" ", 1)[0].lower()] += 1

        if len(by_record) != 5000 or len(ci_hashes) != 500:
            raise M60cError(f"{form}: selected-hash coverage is incomplete")
        ci_rows = {digest: by_record[digest] for digest in ci_hashes}
        ci_upstream = {row["upstream_test_hash"]: row for row in ci_rows.values()}
        expected = SST_EXPECTED_DIGESTS[form]
        actual = {
            "ci_record": ratchet.hash_set_digest(ci_rows),
            "ci_upstream": ratchet.upstream_hash_set_digest(ci_upstream),
            "full_record": ratchet.hash_set_digest(by_record),
            "full_upstream": ratchet.upstream_hash_set_digest(by_upstream),
        }
        if actual != expected:
            raise M60cError(f"{form}: selected hash-set digest differs")
        summaries[form] = {
            "architectural_ci": {
                "classification": "upstream_nonblocking",
                "executed": 0,
                "fail": 0,
                "pass": 0,
                "selected": 500,
            },
            "architectural_full": {
                "classification": "upstream_nonblocking",
                "executed": 0,
                "fail": 0,
                "pass": 0,
                "selected": 5000,
            },
            "fingerprint_full": {
                "classification": "upstream_nonblocking",
                "executed": 0,
                "fail": 0,
                "pass": 0,
                "selected": 5000,
            },
            "full_instruction_length_counts": dict(sorted(length_counts.items())),
            "full_modrm_mod_counts": dict(sorted(mod_counts.items())),
            "full_prefix_class_counts": dict(sorted(prefix_counts.items())),
            "full_record_hash_set_sha256": actual["full_record"],
            "full_resolved_count": 5000,
            "full_upstream_hash_set_sha256": actual["full_upstream"],
            "gap_kind": None,
            "metadata_architecture": "v30",
            "metadata_mnemonic_roots": dict(sorted(metadata_names.items())),
            "metadata_status": "fpu",
            "primary_opcode": f"0x{int(form, 16):02x}",
            "support_map_mode": "v30op",
            "support_map_target": "v30_reserved",
            "top_level_classification": "upstream_nonblocking",
            "ci_record_hash_set_sha256": actual["ci_record"],
            "ci_resolved_count": 500,
            "ci_upstream_hash_set_sha256": actual["ci_upstream"],
        }

    if ratchet.hash_set_digest(combined_ci) != SST_COMBINED_DIGESTS["ci"]:
        raise M60cError("combined CI 66/67 hash set differs")
    if ratchet.hash_set_digest(combined_full) != SST_COMBINED_DIGESTS["full"]:
        raise M60cError("combined full 66/67 hash set differs")
    all_rows.sort(key=lambda item: item["record_hash"])
    summary = {
        "combined_ci_record_hash_set_sha256": SST_COMBINED_DIGESTS["ci"],
        "combined_ci_selected_count": 1000,
        "combined_full_record_hash_set_sha256": SST_COMBINED_DIGESTS["full"],
        "combined_full_selected_count": 10000,
        "forms": summaries,
        "schema": "vaeg-upd9002-m60c-primary-66-67-sst-audit-v1",
        "schema_version": 1,
        "selection_rule": (
            "Decode the primary opcode after all recognized segment, repeat, "
            "and lock prefixes; never select by first raw byte alone."
        ),
        "top_level_classification_changes": [],
    }
    return summary, all_rows


def ordinary_primary_reference(root: pathlib.Path, rom_data: bytes) -> dict[str, Any]:
    source = read_json(
        root / "tests/ssts/authority/g60b/dispatch_primary_6c_6f.json"
    )
    raw = bounded_slice(
        rom_data,
        m60b.DISPATCH_MAIN_START,
        m60b.DISPATCH_MAIN_END,
        "ordinary primary table",
    )
    mnemonics = bounded_slice(
        rom_data,
        m60b.DISPATCH_MAIN_MNEMONIC_START,
        m60b.DISPATCH_MAIN_MNEMONIC_END,
        "ordinary primary mnemonic table",
    )
    inventory = source.get("inventory")
    if (
        source.get("raw_record_count") != 140
        or source.get("start") != "0x66350"
        or source.get("end_exclusive") != "0x664f4"
        or sha256_bytes(raw) != source.get("raw_slice_sha256")
        or not isinstance(inventory, list)
    ):
        raise M60cError("G60b ordinary-primary authority differs")
    represented = {row["primary_opcode"] for row in inventory}
    if {"0x66", "0x67"} & represented:
        raise M60cError("ordinary primary table unexpectedly contains 66/67")
    return {
        "absence_is_not_sufficient_for_target_absence": True,
        "end_exclusive": "0x664f4",
        "g60b_source_path": (
            "tests/ssts/authority/g60b/dispatch_primary_6c_6f.json"
        ),
        "g60b_source_sha256": sha256_file(
            root
            / "tests/ssts/authority/g60b/dispatch_primary_6c_6f.json"
        ),
        "mnemonic_end_exclusive": "0x666fa",
        "mnemonic_raw_sha256": sha256_bytes(mnemonics),
        "mnemonic_start": "0x66515",
        "primary_0x66_present": False,
        "primary_0x67_present": False,
        "raw_record_count": 140,
        "raw_slice_sha256": sha256_bytes(raw),
        "schema": "vaeg-upd9002-m60c-ordinary-primary-reference-v1",
        "schema_version": 1,
        "start": "0x66350",
    }


def historical_erratum_document() -> dict[str, Any]:
    return {
        "exact_sets": {
            name: {"count": count, "sha256": digest}
            for name, (count, digest) in sorted(erratum.HISTORICAL_SETS.items())
        },
        "interpretation": (
            "G60a retired failures were 6E=0 and 6F=641. The values 417 "
            "and 224 are unchanged-signature and changed-signature subsets "
            "of the same G43 OUTS transition population, not opcode counts."
        ),
        "prospective_master_path": "docs/agents/UPD9002_SEMANTICS_MIGRATION.md",
        "schema": "vaeg-upd9002-m60c-historical-label-erratum-v1",
        "schema_version": 1,
    }


def support_conclusions(
    decoder_paths: dict[str, Any],
    sst_summary: dict[str, Any],
) -> dict[str, Any]:
    if not decoder_paths["all_reachable_alternatives_bounded"]:
        raise M60cError("cannot prove absence with an unbounded decoder alternative")
    for form in ("66", "67"):
        if sst_summary["forms"][form]["top_level_classification"] != (
            "upstream_nonblocking"
        ):
            raise M60cError("66/67 top-level classification differs")
    return {
        "evidence_limits": (
            "This proves monitor disassembler target authority for these "
            "encodings. It does not prove complete uPD9002 silicon semantics."
        ),
        "forbidden_inferences": {
            "another_cpu_undefined_means_target_absent": False,
            "failure_list_absence_means_passing": False,
            "generic_string_absence_means_target_absent": False,
            "ordinary_main_table_absence_means_target_absent": False,
        },
        "forms": {
            form: {
                "gap_kind_after": None,
                "gap_kind_before": None,
                "hardware_pending_after": None,
                "positive_absence_evidence": [
                    "ordinary primary table excludes the encoding",
                    "complete ordered group table excludes the encoding",
                    "0f branch is unreachable for this non-0f primary",
                    "all four FPU tables own only D8-DF primary opcodes",
                    "D8-DF register fallback positively rejects the encoding",
                ],
                "resolved_hash_set_sha256": sst_summary["forms"][form][
                    "full_record_hash_set_sha256"
                ],
                "resolved_count": 5000,
                "support_conclusion": "target_absence_proven",
                "top_level_classification_after": "upstream_nonblocking",
                "top_level_classification_before": "upstream_nonblocking",
            }
            for form in ("66", "67")
        },
        "formal_support_conclusion": "target_absence_proven",
        "hardware_pending_changes": [],
        "schema": "vaeg-upd9002-m60c-support-conclusions-v1",
        "schema_version": 1,
        "taxonomy_changes": [],
        "top_level_classification_changes": [],
    }


def validate_support_conclusions(value: Any) -> None:
    if not isinstance(value, dict):
        raise M60cError("support conclusions are malformed")
    conclusion = value.get("formal_support_conclusion")
    if conclusion not in {
        "target_support_proven",
        "target_absence_proven",
        "target_support_unverified",
    }:
        raise M60cError("unknown formal support conclusion")
    forbidden = value.get("forbidden_inferences")
    if (
        not isinstance(forbidden, dict)
        or set(forbidden.values()) != {False}
        or set(forbidden) != {
            "another_cpu_undefined_means_target_absent",
            "failure_list_absence_means_passing",
            "generic_string_absence_means_target_absent",
            "ordinary_main_table_absence_means_target_absent",
        }
    ):
        raise M60cError("forbidden non-evidence inference entered conclusion")
    if conclusion == "target_absence_proven":
        for form in ("66", "67"):
            item = value.get("forms", {}).get(form, {})
            if (
                item.get("support_conclusion") != "target_absence_proven"
                or len(item.get("positive_absence_evidence", [])) != 5
                or item.get("top_level_classification_before")
                != item.get("top_level_classification_after")
            ):
                raise M60cError("positive target-absence proof is incomplete")
    if conclusion == "target_support_proven":
        for form in ("66", "67"):
            item = value.get("forms", {}).get(form, {})
            if (
                item.get("support_conclusion") != "target_support_proven"
                or not item.get("positive_support_evidence")
            ):
                raise M60cError("positive target-support proof is incomplete")
    hardware_pending = value.get("hardware_pending_changes", [])
    if conclusion == "target_support_unverified":
        if not isinstance(hardware_pending, list) or not hardware_pending:
            raise M60cError(
                "unverified target support lacks exact hardware-pending coverage"
            )
        for item in hardware_pending:
            if (
                not isinstance(item, dict)
                or item.get("form") not in {"66", "67"}
                or item.get("resolved_count") not in {500, 5000}
                or re.fullmatch(
                    r"[0-9a-f]{64}",
                    str(item.get("resolved_hash_set_sha256", "")),
                )
                is None
                or "*" in str(item.get("selector", ""))
            ):
                raise M60cError(
                    "hardware-pending selector/hash ownership is not exact"
                )
    elif hardware_pending:
        raise M60cError("resolved support conclusion changed hardware-pending")
    for item in value.get("forms", {}).values():
        gap_kind = item.get("gap_kind_after")
        item_conclusion = item.get("support_conclusion")
        if gap_kind == "documented_silicon_absent" and (
            item_conclusion != "target_absence_proven"
        ):
            raise M60cError("documented absence lacks positive absence proof")
        if gap_kind == "implementation_missing" and (
            item_conclusion != "target_support_proven"
        ):
            raise M60cError("implementation gap lacks positive support proof")
        if gap_kind == "target_support_unverified" and (
            item_conclusion != "target_support_unverified"
        ):
            raise M60cError("unverified gap kind lacks matching conclusion")
    if value.get("top_level_classification_changes") != []:
        raise M60cError("top-level classification change is forbidden")


def scoreboard_identities(root: pathlib.Path) -> dict[str, Any]:
    result = {}
    paths = {
        "architectural_ci": "g60b_architectural_ci.json",
        "architectural_full": "g60b_architectural_full.json",
        "fingerprint_full": "g60b_fingerprint_full.json",
    }
    for name, filename in paths.items():
        path = root / "tests/ssts/scoreboard" / filename
        value = read_json(path)
        expected = PROFILE_IDENTITIES[name]
        actual = {field: value.get(field) for field in expected}
        if actual != expected:
            raise M60cError(f"approved G60b {name} scoreboard differs")
        if (
            value.get("target_policy_id") != G60B_TARGET_POLICY_ID
            or value.get("target_policy_sha256") != G60B_TARGET_POLICY_SHA256
            or value.get("timeouts") != 0
            or value.get("crashes") != 0
        ):
            raise M60cError(f"approved G60b {name} policy/result differs")
        result[name] = {
            **expected,
            "comparison_contract_id": value["comparison_contract_id"],
            "comparison_contract_sha256": value[
                "comparison_contract_sha256"
            ],
            "scoreboard_digest": value["scoreboard_digest"],
            "scoreboard_file_sha256": sha256_file(path),
        }
    return result


def artifact_entry(
    path: pathlib.Path, relative: pathlib.Path, row_count: int
) -> dict[str, Any]:
    return {
        "bytes": path.stat().st_size,
        "path": relative.as_posix(),
        "row_count": row_count,
        "sha256": sha256_file(path),
    }


def write_authority_pack(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    rom_path: pathlib.Path,
    evaluated_sha: str,
) -> tuple[dict[str, Any], str]:
    validate_analysis_sha(evaluated_sha)
    rom_data = rom_path.read_bytes()
    validate_rom(rom_data)
    destination = output_root / AUTHORITY_ROOT
    if destination.exists() and any(destination.iterdir()):
        raise M60cError(f"authority output is not empty: {destination}")
    destination.mkdir(parents=True, exist_ok=True)

    group_raw, group_decoded = parse_group_dispatch(rom_data)
    fpu_raw, fpu_decoded, fpu_mnemonics = parse_fpu_authority(rom_data)
    decoder_paths = build_decoder_paths(rom_data, group_decoded, fpu_decoded)
    sst_summary, sst_rows = extract_sst_audit(root, dataset_root)
    ordinary = ordinary_primary_reference(root, rom_data)
    conclusions = support_conclusions(decoder_paths, sst_summary)
    validate_support_conclusions(conclusions)
    scoreboards = scoreboard_identities(root)

    source_provenance = {
        "authorization": (
            "The complete copyrighted ROM remains out of tree. G60c stores "
            "only decoded records, minimal raw table bytes, code-range bytes, "
            "hashes, and neutral analysis."
        ),
        "crc32": ROM_CRC32,
        "g60b_authority_manifest_sha256": G60B_AUTHORITY_MANIFEST_SHA256,
        "mapping": "canonical address equals ROM file offset",
        "rom_role": "PC-88VA2 main varom00 monitor ROM",
        "rom_sha1": ROM_SHA1,
        "rom_sha256": ROM_SHA256,
        "rom_size": ROM_SIZE,
        "schema": "vaeg-upd9002-m60c-source-provenance-v1",
        "schema_version": 1,
    }
    documents: dict[str, Any] = {
        "decoder_paths_66_67.json": decoder_paths,
        "fpu_d8_df_decoded.json": fpu_decoded,
        "fpu_d8_df_raw.json": fpu_raw,
        "fpu_mnemonic_map.json": fpu_mnemonics,
        "group_dispatch_decoded.json": group_decoded,
        "group_dispatch_raw.json": group_raw,
        "historical_label_erratum.json": historical_erratum_document(),
        "ordinary_primary_table_reference.json": ordinary,
        "primary_66_67_sst_audit.json": sst_summary,
        "source_provenance.json": source_provenance,
        "support_conclusions.json": conclusions,
    }
    row_counts: dict[str, int] = {
        "decoder_paths_66_67.json": len(decoder_paths["paths"]),
        "fpu_d8_df_decoded.json": fpu_decoded["total_raw_record_count"],
        "fpu_d8_df_raw.json": fpu_raw["total_raw_record_count"],
        "fpu_mnemonic_map.json": fpu_mnemonics["record_mnemonic_count"],
        "group_dispatch_decoded.json": len(group_decoded["decoded_records"]),
        "group_dispatch_raw.json": group_raw["raw_record_count"],
        "historical_label_erratum.json": len(erratum.HISTORICAL_SETS),
        "ordinary_primary_table_reference.json": ordinary["raw_record_count"],
        "primary_66_67_sst_audit.json": 2,
        "source_provenance.json": 1,
        "support_conclusions.json": 2,
    }
    for name, value in sorted(documents.items()):
        write_json(destination / name, value)

    case_path = destination / "primary_66_67_sst_cases.json.gz"
    raw_digest, canonical_digest = ratchet.write_deterministic_gzip(
        case_path, sst_rows
    )
    case_relative = AUTHORITY_ROOT / case_path.name
    artifacts = [
        artifact_entry(destination / name, AUTHORITY_ROOT / name, row_counts[name])
        for name in sorted(documents)
    ]
    artifacts.append(
        {
            "bytes": case_path.stat().st_size,
            "canonical_sha256": canonical_digest,
            "path": case_relative.as_posix(),
            "row_count": len(sst_rows),
            "sha256": raw_digest,
        }
    )
    artifacts.sort(key=lambda item: item["path"])

    architectural = scoreboards["architectural_full"]
    fingerprint = scoreboards["fingerprint_full"]
    manifest = {
        "analysis_evaluated_sha": evaluated_sha,
        "applicable_hash_set_sha256": {
            "ci": scoreboards["architectural_ci"][
                "applicable_hash_set_sha256"
            ],
            "full": architectural["applicable_hash_set_sha256"],
        },
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifacts": artifacts,
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": {
            "architectural": {
                "id": architectural["comparison_contract_id"],
                "sha256": architectural["comparison_contract_sha256"],
            },
            "fingerprint": {
                "id": fingerprint["comparison_contract_id"],
                "sha256": fingerprint["comparison_contract_sha256"],
            },
        },
        "compression_environment": {
            "gzip_writer": "repository-raw-deflate-gzip-v1",
            "python": platform.python_version(),
            "zlib_compile": zlib.ZLIB_VERSION,
            "zlib_runtime": zlib.ZLIB_RUNTIME_VERSION,
        },
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": read_json(root / "tests/ssts/scoreboard/g60b_architectural_full.json")[
            "dataset_id"
        ],
        "formal_support_conclusion": "target_absence_proven",
        "g60b_authority_manifest_sha256": G60B_AUTHORITY_MANIFEST_SHA256,
        "g60b_target_policy_id": G60B_TARGET_POLICY_ID,
        "g60b_target_policy_sha256": G60B_TARGET_POLICY_SHA256,
        "generator": {
            "path": "tools/qa/upd9002_m60c_audit.py",
            "version": 1,
        },
        "license": "BSD-2-Clause",
        "milestone": MILESTONE,
        "primary_66_67": {
            "ci_executed": 0,
            "ci_selected": 1000,
            "full_executed": 0,
            "full_selected": 10000,
            "record_hash_set_sha256": SST_COMBINED_DIGESTS["full"],
        },
        "profile_identities": scoreboards,
        "rom_mapping": {
            "address_identity": "rom-file-offset",
            "bank_file_base": hex_offset(ROM_BANK_FILE_BASE),
        },
        "rom_sha256": ROM_SHA256,
        "rom_size": ROM_SIZE,
        "schema": "vaeg-upd9002-m60c-authority-manifest-v1",
        "schema_version": 1,
        "selected_hash_set_sha256": {
            "ci": scoreboards["architectural_ci"][
                "selected_hash_set_sha256"
            ],
            "full": architectural["selected_hash_set_sha256"],
        },
        "table_boundaries": {
            "fpu_tables": [
                {
                    "end_exclusive": hex_offset(table["end"]),
                    "mnemonic_end_exclusive": hex_offset(
                        table["mnemonic_end"]
                    ),
                    "mnemonic_start": hex_offset(table["mnemonic_start"]),
                    "name": table["name"],
                    "record_count": table["record_count"],
                    "start": hex_offset(table["start"]),
                }
                for table in FPU_TABLES
            ],
            "group_dispatch": {
                "end_exclusive": hex_offset(GROUP_END),
                "record_count": GROUP_RECORD_COUNT,
                "start": hex_offset(GROUP_START),
            },
        },
        "taxonomy_registry_result": {
            "gap_kind_changes": [],
            "hardware_pending_changes": [],
            "top_level_classification_changes": [],
        },
    }
    manifest_path = destination / "manifest.json"
    write_json(manifest_path, manifest)
    manifest_sha = sha256_file(manifest_path)
    print(
        "m60c-authority: "
        f"rom={ROM_SHA256} cases=10000 group_records=12 "
        f"fpu_records=87 conclusion=target_absence_proven "
        f"manifest_sha256={manifest_sha}"
    )
    return manifest, manifest_sha


def write_transition_and_result_manifest(
    root: pathlib.Path,
    output_root: pathlib.Path,
    authority_manifest: dict[str, Any],
    authority_manifest_sha256: str,
    evaluated_sha: str,
) -> tuple[dict[str, Any], dict[str, Any]]:
    scoreboards = authority_manifest["profile_identities"]
    transition = {
        "applicable_hash_set_after_sha256": {
            "ci": scoreboards["architectural_ci"][
                "applicable_hash_set_sha256"
            ],
            "full": scoreboards["architectural_full"][
                "applicable_hash_set_sha256"
            ],
        },
        "applicable_hash_set_before_sha256": {
            "ci": scoreboards["architectural_ci"][
                "applicable_hash_set_sha256"
            ],
            "full": scoreboards["architectural_full"][
                "applicable_hash_set_sha256"
            ],
        },
        "authority_manifest_sha256": authority_manifest_sha256,
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "changed_failure_count": 0,
        "comparison_contract_ids": authority_manifest[
            "comparison_contracts"
        ],
        "dataset_id": authority_manifest["dataset_id"],
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": [],
        "hardware_pending_changes": [],
        "newly_failing": [],
        "newly_passing": [],
        "primary_66_67_hashes_sha256": SST_COMBINED_DIGESTS["full"],
        "schema": "vaeg-upd9002-m60c-authority-transition-v1",
        "schema_version": 1,
        "selected_hash_set_sha256": authority_manifest[
            "selected_hash_set_sha256"
        ],
        "support_conclusion": "target_absence_proven",
        "target_policy_after_id": G60B_TARGET_POLICY_ID,
        "target_policy_before_id": G60B_TARGET_POLICY_ID,
        "top_level_classification_changes": [],
        "transition_kind": "target_authority_audit",
    }
    validate_transition(transition)
    transition_path = output_root / TRANSITION_PATH
    write_json(transition_path, transition)

    authority_rows = {
        item["path"]: item["row_count"]
        for item in authority_manifest["artifacts"]
    }
    authority_rows[(AUTHORITY_ROOT / "manifest.json").as_posix()] = 1
    authority_rows[TRANSITION_PATH.as_posix()] = 1
    artifact_paths = sorted(
        [
            path
            for path in (output_root / AUTHORITY_ROOT).rglob("*")
            if path.is_file()
        ]
        + [transition_path]
    )
    artifacts = []
    for path in artifact_paths:
        relative = path.relative_to(output_root)
        artifacts.append(
            artifact_entry(
                path,
                relative,
                authority_rows[relative.as_posix()],
            )
        )
    result_manifest = {
        "analysis_evaluated_sha": evaluated_sha,
        "artifact_tree_sha256": identity_digest(artifacts),
        "artifacts": artifacts,
        "authority_manifest_sha256": authority_manifest_sha256,
        "candidate_gate": CANDIDATE_GATE,
        "profile_identities": scoreboards,
        "schema": "vaeg-upd9002-m60c-result-manifest-v1",
        "schema_version": 1,
        "transition_sha256": sha256_file(transition_path),
    }
    write_json(output_root / RESULT_MANIFEST_PATH, result_manifest)
    print(
        "m60c-transition: "
        f"transition_sha256={result_manifest['transition_sha256']} "
        f"artifact_tree_sha256={result_manifest['artifact_tree_sha256']}"
    )
    return transition, result_manifest


def validate_transition(value: Any) -> None:
    if not isinstance(value, dict):
        raise M60cError("authority transition is malformed")
    if (
        value.get("before_gate") != APPROVED_PREDECESSOR_GATE
        or value.get("before_sha") != APPROVED_PREDECESSOR_SHA
        or value.get("transition_kind") != "target_authority_audit"
        or value.get("target_policy_before_id") != G60B_TARGET_POLICY_ID
        or value.get("target_policy_after_id") != G60B_TARGET_POLICY_ID
        or value.get("dataset_id") != DATASET_ID
        or value.get("comparison_contract_ids") != COMPARISON_CONTRACTS
        or value.get("selected_hash_set_sha256") != SELECTED_HASH_SETS
    ):
        raise M60cError("authority transition identity differs")
    if (
        value.get("applicable_hash_set_before_sha256")
        != value.get("applicable_hash_set_after_sha256")
        or value.get("applicable_hash_set_before_sha256")
        != APPLICABLE_HASH_SETS
        or value.get("newly_passing") != []
        or value.get("newly_failing") != []
        or value.get("changed_failure_count") != 0
        or value.get("top_level_classification_changes") != []
        or value.get("gap_kind_changes") != []
        or value.get("hardware_pending_changes") != []
    ):
        raise M60cError("authority audit changed governed semantics or policy")
    if value.get("support_conclusion") != "target_absence_proven":
        raise M60cError("authority transition support conclusion differs")


def validate_case_row(row: Any) -> None:
    if not isinstance(row, dict):
        raise M60cError("66/67 case row is malformed")
    record_hash = row.get("record_hash")
    upstream_hash = row.get("upstream_test_hash")
    if (
        not isinstance(record_hash, str)
        or re.fullmatch(r"[0-9a-f]{64}", record_hash) is None
        or not isinstance(upstream_hash, str)
        or re.fullmatch(r"[0-9a-f]{40}", upstream_hash) is None
    ):
        raise M60cError("66/67 case hash is malformed")
    encoded = row.get("complete_instruction_bytes")
    if (
        not isinstance(encoded, str)
        or len(encoded) % 2
        or re.fullmatch(r"[0-9a-f]+", encoded) is None
    ):
        raise M60cError("66/67 instruction bytes are malformed")
    instruction = list(bytes.fromhex(encoded))
    prefixes = row.get("prefix_sequence")
    if (
        not isinstance(prefixes, list)
        or not all(
            isinstance(item, str)
            and re.fullmatch(r"0x[0-9a-f]{2}", item) is not None
            for item in prefixes
        )
    ):
        raise M60cError("66/67 prefix sequence is malformed")
    decoded_primary, decoded_prefixes = m60b.decode_primary_opcode(instruction)
    if [f"0x{byte:02x}" for byte in decoded_prefixes] != prefixes:
        raise M60cError("66/67 prefix decoding differs")
    primary = row.get("primary_opcode")
    if primary not in {"0x66", "0x67"} or decoded_primary != int(primary, 16):
        raise M60cError("first-byte-only or incomplete primary decoding")
    if (
        row.get("top_level_classification") != "upstream_nonblocking"
        or row.get("gap_kind") is not None
        or row.get("upstream_metadata_status") != "fpu"
        or row.get("upstream_metadata_architecture") != "v30"
        or not str(row.get("upstream_metadata_mnemonic", "")).lower().startswith(
            "fpo2"
        )
    ):
        raise M60cError("66/67 classification or metadata evidence differs")
    dispatch = row.get("dispatch")
    support = row.get("support_map_row")
    if (
        not isinstance(dispatch, dict)
        or dispatch.get("opcode") != primary
        or dispatch.get("support_mode") != "v30op"
        or dispatch.get("support_target") != "v30_reserved"
        or not isinstance(support, dict)
        or support.get("mode") != "v30op"
        or support.get("opcode") != primary
        or support.get("target") != "v30_reserved"
        or support.get("classification") != "known_target_gap"
    ):
        raise M60cError("66/67 support-map ownership differs")
    for profile in ("architectural_ci", "architectural_full", "fingerprint_full"):
        result = row.get(profile)
        if (
            not isinstance(result, dict)
            or result.get("executed") is not False
            or result.get("pass") != 0
            or result.get("fail") != 0
            or not isinstance(result.get("selected"), bool)
        ):
            raise M60cError("66/67 selected/executed/pass/fail evidence differs")
    if (
        row["architectural_full"]["selected"] is not True
        or row["fingerprint_full"]["selected"] is not True
    ):
        raise M60cError("66/67 full-profile selection is incomplete")


def validate_sst_case_table(
    summary: Any, rows: Any, enforce_epoch_counts: bool = True
) -> None:
    if not isinstance(summary, dict) or not isinstance(rows, list):
        raise M60cError("66/67 SST audit is malformed")
    record_hashes = set()
    upstream_hashes = set()
    by_form: dict[str, dict[str, dict[str, Any]]] = {
        "66": {},
        "67": {},
    }
    ci_by_form = {"66": {}, "67": {}}
    for row in rows:
        validate_case_row(row)
        record_hash = row["record_hash"]
        upstream_hash = row["upstream_test_hash"]
        if record_hash in record_hashes or upstream_hash in upstream_hashes:
            raise M60cError("duplicate 66/67 case hash")
        record_hashes.add(record_hash)
        upstream_hashes.add(upstream_hash)
        form = row["primary_opcode"][2:]
        by_form[form][record_hash] = row
        if row["architectural_ci"]["selected"]:
            ci_by_form[form][record_hash] = row
    if not enforce_epoch_counts:
        return
    if len(rows) != 10000 or any(len(by_form[form]) != 5000 for form in by_form):
        raise M60cError("complete selected 66/67 coverage differs")
    if any(len(ci_by_form[form]) != 500 for form in ci_by_form):
        raise M60cError("CI selected 66/67 coverage differs")
    for form in ("66", "67"):
        expected = SST_EXPECTED_DIGESTS[form]
        if (
            ratchet.hash_set_digest(by_form[form]) != expected["full_record"]
            or ratchet.hash_set_digest(ci_by_form[form]) != expected["ci_record"]
        ):
            raise M60cError(f"{form}: case-table hash ownership differs")
        form_summary = summary.get("forms", {}).get(form, {})
        if (
            form_summary.get("full_resolved_count") != 5000
            or form_summary.get("ci_resolved_count") != 500
            or form_summary.get("top_level_classification")
            != "upstream_nonblocking"
            or form_summary.get("gap_kind") is not None
        ):
            raise M60cError(f"{form}: summary count/classification differs")
    if (
        summary.get("combined_full_record_hash_set_sha256")
        != SST_COMBINED_DIGESTS["full"]
        or summary.get("combined_ci_record_hash_set_sha256")
        != SST_COMBINED_DIGESTS["ci"]
        or summary.get("top_level_classification_changes") != []
    ):
        raise M60cError("combined 66/67 summary identity differs")


def validate_group_artifacts(raw: Any, decoded: Any) -> None:
    if not isinstance(raw, dict) or not isinstance(decoded, dict):
        raise M60cError("group-dispatch artifacts are malformed")
    records = raw.get("raw_records")
    decoded_records = decoded.get("decoded_records")
    if (
        raw.get("start") != hex_offset(GROUP_START)
        or raw.get("end_exclusive") != hex_offset(GROUP_END)
        or raw.get("raw_record_count") != GROUP_RECORD_COUNT
        or raw.get("record_width") != 3
        or not isinstance(records, list)
        or tuple(item.get("raw") for item in records) != EXPECTED_GROUP_RECORDS
        or not isinstance(decoded_records, list)
        or len(decoded_records) != GROUP_RECORD_COUNT
        or decoded.get("group_handler_pointers")
        != list(EXPECTED_GROUP_HANDLER_POINTERS)
    ):
        raise M60cError("group-dispatch boundary, count, or mapping differs")
    segment = decoded.get("segment_override_proof", {})
    if (
        segment.get("raw_record") != "e72608"
        or segment.get("expanded_primary_opcodes")
        != ["0x26", "0x2e", "0x36", "0x3e"]
        or segment.get("group") != "0x08"
        or segment.get("handler_pointer") != "0x60bc"
        or segment.get("proven") is not True
    ):
        raise M60cError("e7/26 segment-override interpretation differs")


def validate_fpu_artifacts(
    raw: Any, decoded: Any, mnemonics: Any
) -> None:
    if not all(isinstance(value, dict) for value in (raw, decoded, mnemonics)):
        raise M60cError("FPU authority artifacts are malformed")
    raw_tables = raw.get("tables")
    decoded_tables = decoded.get("tables")
    rows = mnemonics.get("ownership_rows")
    if (
        raw.get("total_raw_record_count") != 87
        or decoded.get("total_raw_record_count") != 87
        or not isinstance(raw_tables, list)
        or len(raw_tables) != 4
        or not isinstance(decoded_tables, list)
        or len(decoded_tables) != 4
        or not isinstance(rows, list)
        or len(rows) != 87
    ):
        raise M60cError("FPU table boundary or record count differs")
    expected_boundaries = {
        table["name"]: (
            hex_offset(table["start"]),
            hex_offset(table["end"]),
            table["record_count"],
        )
        for table in FPU_TABLES
    }
    observed_boundaries = {
        table.get("name"): (
            table.get("start"),
            table.get("end_exclusive"),
            table.get("raw_record_count"),
        )
        for table in raw_tables
    }
    if observed_boundaries != expected_boundaries:
        raise M60cError("FPU table boundaries differ")
    if decoded.get("primary_opcode_inventory") != [
        f"0x{opcode:02x}" for opcode in range(0xD8, 0xE0)
    ]:
        raise M60cError("FPU primary-opcode ownership differs")
    if (
        set(mnemonics.get("individual_mnemonics", []))
        != EXPECTED_FPU_MNEMONICS
        or mnemonics.get("individual_mnemonic_count")
        != len(EXPECTED_FPU_MNEMONICS)
    ):
        raise M60cError("FPU mnemonic inventory is incomplete")
    owners = [row.get("owner") for row in rows]
    if len(set(owners)) != 87 or None in owners:
        raise M60cError("duplicate or missing FPU mnemonic ownership")
    handler_links = [
        record.get("handler_pointer")
        for table in decoded_tables
        for record in table.get("decoded_records", [])
    ]
    if len(handler_links) != 87 or any(link is None for link in handler_links):
        raise M60cError("FPU group/handler link is incomplete")


def validate_decoder_artifact(value: Any) -> None:
    if not isinstance(value, dict):
        raise M60cError("decoder-path artifact is malformed")
    paths = value.get("paths")
    if (
        value.get("all_reachable_alternatives_bounded") is not True
        or value.get("primary_0x66_result") != "rejected-as-unknown"
        or value.get("primary_0x67_result") != "rejected-as-unknown"
        or value.get("fallback_primary_range") != ["0xd8", "0xdf"]
        or value.get("fpu_table_primary_inventory")
        != [f"0x{opcode:02x}" for opcode in range(0xD8, 0xE0)]
        or not isinstance(paths, list)
        or len(paths) != 6
    ):
        raise M60cError("decoder-path coverage or conclusion differs")
    for path in paths:
        if (
            path.get("reachable_from_normal_disassembler_entry") is not True
            or path.get("accepts_primary_0x66") is not False
            or path.get("accepts_primary_0x67") is not False
            or re.fullmatch(r"[0-9a-f]{64}", str(path.get("raw_sha256"))) is None
            or not path.get("neutral_pseudocode")
        ):
            raise M60cError("unreachable or incomplete path was used as proof")


def validate_pack(root: pathlib.Path) -> tuple[dict[str, Any], str]:
    pack = root / AUTHORITY_ROOT
    manifest_path = pack / "manifest.json"
    manifest = read_json(manifest_path)
    if (
        manifest.get("schema") != "vaeg-upd9002-m60c-authority-manifest-v1"
        or manifest.get("schema_version") != 1
        or manifest.get("milestone") != MILESTONE
        or manifest.get("candidate_gate") != CANDIDATE_GATE
        or manifest.get("approved_predecessor_sha") != APPROVED_PREDECESSOR_SHA
        or manifest.get("rom_sha256") != ROM_SHA256
        or manifest.get("g60b_target_policy_id") != G60B_TARGET_POLICY_ID
        or manifest.get("g60b_authority_manifest_sha256")
        != G60B_AUTHORITY_MANIFEST_SHA256
        or manifest.get("dataset_id") != DATASET_ID
        or manifest.get("comparison_contracts") != COMPARISON_CONTRACTS
        or manifest.get("selected_hash_set_sha256") != SELECTED_HASH_SETS
        or manifest.get("applicable_hash_set_sha256")
        != APPLICABLE_HASH_SETS
    ):
        raise M60cError("authority manifest identity differs")
    validate_analysis_sha(manifest.get("analysis_evaluated_sha", ""))
    artifacts = manifest.get("artifacts")
    if (
        not isinstance(artifacts, list)
        or [item.get("path") for item in artifacts]
        != sorted(item.get("path") for item in artifacts)
        or len({item.get("path") for item in artifacts}) != len(artifacts)
    ):
        raise M60cError("authority artifact inventory is malformed")
    values = {}
    case_rows = None
    for item in artifacts:
        relative = pathlib.PurePosixPath(item.get("path", ""))
        if (
            relative.is_absolute()
            or ".." in relative.parts
            or pathlib.PurePosixPath(*relative.parts[:-1])
            != pathlib.PurePosixPath(AUTHORITY_ROOT.as_posix())
        ):
            raise M60cError("authority artifact path is unsafe")
        path = root / pathlib.Path(relative)
        if (
            not path.is_file()
            or path.stat().st_size != item.get("bytes")
            or sha256_file(path) != item.get("sha256")
        ):
            raise M60cError(f"authority artifact digest differs: {relative}")
        if path.suffix == ".gz":
            try:
                value = ratchet.read_deterministic_gzip(path)
            except ratchet.RatchetError as error:
                raise M60cError(str(error)) from error
            if not isinstance(value, list) or len(value) != item.get("row_count"):
                raise M60cError("compressed case-table row count differs")
            if sha256_bytes(canonical_bytes(value) + b"\n") != item.get(
                "canonical_sha256"
            ):
                raise M60cError("compressed case-table canonical digest differs")
            case_rows = value
        else:
            value = read_json(path)
            if path.read_bytes() != canonical_bytes(value) + b"\n":
                raise M60cError("authority JSON is not canonical")
            values[path.name] = value
    validate_support_conclusions(values["support_conclusions.json"])
    validate_group_artifacts(
        values["group_dispatch_raw.json"],
        values["group_dispatch_decoded.json"],
    )
    validate_fpu_artifacts(
        values["fpu_d8_df_raw.json"],
        values["fpu_d8_df_decoded.json"],
        values["fpu_mnemonic_map.json"],
    )
    validate_decoder_artifact(values["decoder_paths_66_67.json"])
    validate_sst_case_table(
        values["primary_66_67_sst_audit.json"], case_rows
    )
    if values["primary_66_67_sst_audit.json"][
        "combined_full_record_hash_set_sha256"
    ] != SST_COMBINED_DIGESTS["full"]:
        raise M60cError("authority pack 66/67 coverage differs")
    if values["fpu_d8_df_decoded.json"][
        "primary_opcode_inventory"
    ] != [f"0x{opcode:02x}" for opcode in range(0xD8, 0xE0)]:
        raise M60cError("authority pack FPU inventory differs")
    if values["group_dispatch_decoded.json"]["segment_override_proof"][
        "expanded_primary_opcodes"
    ] != ["0x26", "0x2e", "0x36", "0x3e"]:
        raise M60cError("authority pack segment-override proof differs")
    if manifest_path.read_bytes() != canonical_bytes(manifest) + b"\n":
        raise M60cError("authority manifest is not canonical")
    return manifest, sha256_file(manifest_path)


def validate_result_manifest(root: pathlib.Path) -> None:
    manifest, manifest_sha = validate_pack(root)
    transition_path = root / TRANSITION_PATH
    transition = read_json(transition_path)
    validate_transition(transition)
    if transition.get("authority_manifest_sha256") != manifest_sha:
        raise M60cError("transition authority-manifest digest differs")
    result_path = root / RESULT_MANIFEST_PATH
    result = read_json(result_path)
    if (
        result.get("schema") != "vaeg-upd9002-m60c-result-manifest-v1"
        or result.get("analysis_evaluated_sha")
        != manifest["analysis_evaluated_sha"]
        or result.get("authority_manifest_sha256") != manifest_sha
        or result.get("transition_sha256") != sha256_file(transition_path)
    ):
        raise M60cError("result manifest identity differs")
    artifacts = result.get("artifacts")
    expected_paths = sorted(
        [item["path"] for item in manifest["artifacts"]]
        + [
            (AUTHORITY_ROOT / "manifest.json").as_posix(),
            TRANSITION_PATH.as_posix(),
        ]
    )
    if (
        not isinstance(artifacts, list)
        or [item.get("path") for item in artifacts] != expected_paths
        or RESULT_MANIFEST_PATH.as_posix() in expected_paths
    ):
        raise M60cError("result artifact inventory is malformed")
    for item in artifacts:
        path = root / item["path"]
        if (
            not path.is_file()
            or path.stat().st_size != item["bytes"]
            or sha256_file(path) != item["sha256"]
        ):
            raise M60cError("result artifact digest differs")
    if result.get("artifact_tree_sha256") != identity_digest(artifacts):
        raise M60cError("result artifact-tree digest differs")


def verify_git_unchanged(
    root: pathlib.Path, paths: list[str], label: str
) -> None:
    result = subprocess.run(
        [
            "git",
            "diff",
            "--quiet",
            APPROVED_PREDECESSOR_GIT_SHA,
            "--",
            *paths,
        ],
        cwd=root,
        check=False,
    )
    if result.returncode != 0:
        raise M60cError(f"{label} changed")


def verify_protected_paths(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    protected = [
        "tests/ssts/approved_target_divergences.json",
        "tests/ssts/baseline",
        "tests/ssts/contracts",
        "tests/ssts/epochs/g43",
        "tests/ssts/evidence/g59",
        "tests/ssts/gap_taxonomy.json",
        "tests/ssts/hardware_pending.json",
        "tests/ssts/v20_dataset_manifest.json",
        "tests/ssts/authority/g60b",
    ]
    if not (
        protected_evidence_only
        and (
            root
            / "docs/agents/tasks/M70_upd9002_prefix_string_closure.md"
        ).is_file()
    ):
        protected.append("tools/qa/upd9002_ssts.py")
    if protected_evidence_only:
        protected.extend(
            path.relative_to(root).as_posix()
            for path in (root / "tests/ssts/target_policy").glob("g60b*")
        )
    else:
        protected.extend(
            [
                "tests/ssts/target_policy",
                SUPPORT_MAP_PATH.as_posix(),
                "cpu/upd9002",
            ]
        )
    verify_git_unchanged(
        root,
        protected,
        "production semantics, policy, fixture, contract, or protected evidence",
    )
    protected_scoreboards = [
        path.relative_to(root).as_posix()
        for path in (root / "tests/ssts/scoreboard").glob("*")
        if path.name.startswith(("g58", "g60a", "g60b"))
    ]
    verify_git_unchanged(
        root, protected_scoreboards, "approved scoreboard evidence"
    )
    protected_transitions = [
        path.relative_to(root).as_posix()
        for path in (root / "tests/ssts/transitions").glob("*")
        if path.name.startswith(("g58", "g60a", "g60b"))
    ]
    verify_git_unchanged(
        root, protected_transitions, "approved transition evidence"
    )


def verify_static(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    forward_milestone = (
        root / "docs/agents/tasks/M62_upd9002_semantics_bundle.md"
    ).is_file()
    protected_evidence_only = protected_evidence_only or forward_milestone
    erratum.verify_static(root)
    try:
        m60b.verify_static(root, protected_evidence_only)
    except m60b.M60bError as error:
        raise M60cError(str(error)) from error
    verify_protected_paths(root, protected_evidence_only)
    family = [
        (root / AUTHORITY_ROOT / "manifest.json").is_file(),
        (root / TRANSITION_PATH).is_file(),
        (root / RESULT_MANIFEST_PATH).is_file(),
    ]
    if any(family) and not all(family):
        raise M60cError("G60c evidence family is incomplete")
    if all(family):
        validate_result_manifest(root)
        scope = (
            "protected evidence/policy"
            if protected_evidence_only
            else "protected evidence/policy/semantics"
        )
        print(f"m60c-static: {scope} and complete G60c authority audit passed")
    else:
        print(
            "m60c-static: implementation-only tree; protected evidence, "
            "target policy, selected/applicable sets, and cpu/upd9002 passed"
        )


def verify_authority(
    root: pathlib.Path, rom_path: pathlib.Path
) -> None:
    manifest, manifest_sha = validate_pack(root)
    with tempfile.TemporaryDirectory(prefix="vaeg-m60c-authority-verify-") as temp:
        temporary = pathlib.Path(temp)
        dataset_marker = manifest["dataset_id"]
        # ROM-only regeneration proves every table and code-range assertion.
        rom_data = rom_path.read_bytes()
        validate_rom(rom_data)
        group_raw, group_decoded = parse_group_dispatch(rom_data)
        fpu_raw, fpu_decoded, fpu_mnemonics = parse_fpu_authority(rom_data)
        decoder = build_decoder_paths(rom_data, group_decoded, fpu_decoded)
        ordinary = ordinary_primary_reference(root, rom_data)
        observed = {
            "decoder_paths_66_67.json": decoder,
            "fpu_d8_df_decoded.json": fpu_decoded,
            "fpu_d8_df_raw.json": fpu_raw,
            "fpu_mnemonic_map.json": fpu_mnemonics,
            "group_dispatch_decoded.json": group_decoded,
            "group_dispatch_raw.json": group_raw,
            "ordinary_primary_table_reference.json": ordinary,
        }
        for name, value in observed.items():
            tracked = read_json(root / AUTHORITY_ROOT / name)
            if tracked != value:
                raise M60cError(f"ROM regeneration differs: {name}")
        if not dataset_marker.startswith("ssts-v20-"):
            raise M60cError("authority manifest dataset identity differs")
        (temporary / "verified").write_text(manifest_sha + "\n", encoding="ascii")
    print(
        "m60c-authority-verify: exact ROM re-extraction passed; "
        f"manifest_sha256={manifest_sha}"
    )


def expect_rejected(action: Callable[[], Any], label: str) -> None:
    try:
        action()
    except (M60cError, m60b.M60bError, ratchet.RatchetError):
        return
    raise AssertionError(f"negative test was accepted: {label}")


def selftest() -> None:
    negative = []

    def reject(action: Callable[[], Any], label: str) -> None:
        expect_rejected(action, label)
        negative.append(label)

    reject(lambda: validate_analysis_sha("0" * 39), "malformed evaluated SHA")
    reject(
        lambda: validate_analysis_sha(APPROVED_PREDECESSOR_SHA),
        "predecessor used as evaluated SHA",
    )
    reject(lambda: validate_rom(b"\0" * ROM_SIZE), "wrong ROM SHA")
    reject(
        lambda: bounded_slice(b"\0" * 4, 0, 5, "test"),
        "truncated table",
    )
    reject(
        lambda: m60b.parse_records(b"\0" * 4, 0, 4, 1, "malformed"),
        "malformed three-byte record",
    )

    good_conclusion = {
        "formal_support_conclusion": "target_absence_proven",
        "forbidden_inferences": {
            "another_cpu_undefined_means_target_absent": False,
            "failure_list_absence_means_passing": False,
            "generic_string_absence_means_target_absent": False,
            "ordinary_main_table_absence_means_target_absent": False,
        },
        "forms": {
            form: {
                "positive_absence_evidence": ["a", "b", "c", "d", "e"],
                "support_conclusion": "target_absence_proven",
                "top_level_classification_before": "upstream_nonblocking",
                "top_level_classification_after": "upstream_nonblocking",
            }
            for form in ("66", "67")
        },
        "top_level_classification_changes": [],
    }
    validate_support_conclusions(good_conclusion)
    for field, label in (
        ("generic_string_absence_means_target_absent", "generic string absence"),
        ("ordinary_main_table_absence_means_target_absent", "main-table absence"),
        ("failure_list_absence_means_passing", "failure-list absence"),
        ("another_cpu_undefined_means_target_absent", "other-core behavior"),
    ):
        bad = copy.deepcopy(good_conclusion)
        bad["forbidden_inferences"][field] = True
        reject(
            lambda bad=bad: validate_support_conclusions(bad),
            f"{label} used as proof",
        )
    bad = copy.deepcopy(good_conclusion)
    bad["forms"]["66"]["positive_absence_evidence"] = ["main table"]
    reject(
        lambda: validate_support_conclusions(bad),
        "incomplete reachable-path proof",
    )
    bad = copy.deepcopy(good_conclusion)
    bad["top_level_classification_changes"] = [{"form": "66"}]
    reject(
        lambda: validate_support_conclusions(bad),
        "top-level classification change",
    )
    bad = copy.deepcopy(good_conclusion)
    bad["formal_support_conclusion"] = "probably_absent"
    reject(lambda: validate_support_conclusions(bad), "unknown conclusion")
    bad = copy.deepcopy(good_conclusion)
    bad["formal_support_conclusion"] = "target_support_unverified"
    for item in bad["forms"].values():
        item["support_conclusion"] = "target_support_unverified"
    reject(
        lambda: validate_support_conclusions(bad),
        "unverified support without hardware-pending coverage",
    )
    bad["hardware_pending_changes"] = [
        {
            "form": "66",
            "resolved_count": 5000,
            "resolved_hash_set_sha256": "d" * 64,
            "selector": "opcode=*",
        }
    ]
    reject(
        lambda: validate_support_conclusions(bad),
        "generic open-ended hardware question",
    )
    bad["hardware_pending_changes"][0]["selector"] = "primary_opcode=0x66"
    bad["hardware_pending_changes"][0]["resolved_hash_set_sha256"] = "bad"
    reject(
        lambda: validate_support_conclusions(bad),
        "hardware-pending hash mismatch",
    )
    bad = copy.deepcopy(good_conclusion)
    bad["forms"]["66"]["gap_kind_after"] = "implementation_missing"
    reject(
        lambda: validate_support_conclusions(bad),
        "implementation_missing without positive support",
    )
    bad = copy.deepcopy(good_conclusion)
    bad["forms"]["66"]["support_conclusion"] = "target_support_unverified"
    bad["forms"]["66"]["gap_kind_after"] = "documented_silicon_absent"
    reject(
        lambda: validate_support_conclusions(bad),
        "documented_silicon_absent without positive absence",
    )

    transition = {
        "applicable_hash_set_after_sha256": APPLICABLE_HASH_SETS,
        "applicable_hash_set_before_sha256": APPLICABLE_HASH_SETS,
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "changed_failure_count": 0,
        "comparison_contract_ids": COMPARISON_CONTRACTS,
        "dataset_id": DATASET_ID,
        "gap_kind_changes": [],
        "hardware_pending_changes": [],
        "newly_failing": [],
        "newly_passing": [],
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "support_conclusion": "target_absence_proven",
        "target_policy_after_id": G60B_TARGET_POLICY_ID,
        "target_policy_before_id": G60B_TARGET_POLICY_ID,
        "top_level_classification_changes": [],
        "transition_kind": "target_authority_audit",
    }
    validate_transition(transition)
    mutations = (
        ("top_level_classification_changes", [{"form": "66"}], "classification"),
        ("newly_passing", ["a"], "newly passing"),
        ("newly_failing", ["a"], "newly failing"),
        ("gap_kind_changes", [{"form": "66"}], "unauthorized gap kind"),
        ("hardware_pending_changes", [{"form": "*"}], "open hardware question"),
        ("transition_kind", "target_authority_gap_kind_correction", "wrong transition"),
        ("target_policy_after_id", "changed", "policy ID"),
    )
    for field, value, label in mutations:
        bad = copy.deepcopy(transition)
        bad[field] = value
        reject(lambda bad=bad: validate_transition(bad), label)
    bad = copy.deepcopy(transition)
    bad["applicable_hash_set_after_sha256"] = {"full": "b"}
    reject(lambda: validate_transition(bad), "applicable set change")
    bad = copy.deepcopy(transition)
    bad["selected_hash_set_sha256"] = {"full": "b"}
    reject(lambda: validate_transition(bad), "selected hash-set change")
    bad = copy.deepcopy(transition)
    bad["dataset_id"] = "different"
    reject(lambda: validate_transition(bad), "dataset identity change")
    bad = copy.deepcopy(transition)
    bad["comparison_contract_ids"]["architectural"]["sha256"] = "e" * 64
    reject(lambda: validate_transition(bad), "comparison-contract change")

    case = {
        "architectural_ci": {
            "executed": False,
            "fail": 0,
            "pass": 0,
            "selected": True,
        },
        "architectural_full": {
            "executed": False,
            "fail": 0,
            "pass": 0,
            "selected": True,
        },
        "classification_ownership": "metadata fpu",
        "complete_instruction_bytes": "2666c0",
        "dispatch": {
            "opcode": "0x66",
            "support_mode": "v30op",
            "support_subopcode": "-",
            "support_target": "v30_reserved",
        },
        "fingerprint_full": {
            "executed": False,
            "fail": 0,
            "pass": 0,
            "selected": True,
        },
        "gap_kind": None,
        "modrm": "0xc0",
        "modrm_mod": 3,
        "modrm_reg": 0,
        "prefix_sequence": ["0x26"],
        "primary_opcode": "0x66",
        "record_hash": "a" * 64,
        "support_map_row": {
            "basis": "final-root-target",
            "classification": "known_target_gap",
            "mode": "v30op",
            "opcode": "0x66",
            "subopcode": "-",
            "target": "v30_reserved",
        },
        "top_level_classification": "upstream_nonblocking",
        "upstream_metadata_architecture": "v30",
        "upstream_metadata_mnemonic": "fpo2 ax",
        "upstream_metadata_status": "fpu",
        "upstream_test_hash": "b" * 40,
    }
    validate_case_row(case)
    case_mutations = (
        ("record_hash", "a" * 63, "malformed record hash"),
        ("upstream_test_hash", "b" * 39, "malformed upstream hash"),
        ("complete_instruction_bytes", "66x0", "malformed instruction"),
        ("prefix_sequence", [], "incorrect prefix decoding"),
        ("primary_opcode", "0x67", "first-byte-only selection"),
        ("top_level_classification", "known_target_gap", "classification drift"),
        ("gap_kind", "implementation_missing", "invented gap kind"),
        ("upstream_metadata_status", None, "missing metadata"),
    )
    for field, value, label in case_mutations:
        bad_case = copy.deepcopy(case)
        bad_case[field] = value
        reject(lambda bad_case=bad_case: validate_case_row(bad_case), label)
    bad_case = copy.deepcopy(case)
    bad_case["support_map_row"]["target"] = "missing"
    reject(lambda: validate_case_row(bad_case), "missing support-map ownership")
    bad_case = copy.deepcopy(case)
    bad_case["architectural_full"]["executed"] = True
    reject(
        lambda: validate_case_row(bad_case),
        "selected/executed inconsistency",
    )
    duplicate_rows = [case, copy.deepcopy(case)]
    reject(
        lambda: validate_sst_case_table({}, duplicate_rows, False),
        "duplicate selected case hash",
    )
    reject(
        lambda: validate_sst_case_table({}, [case], True),
        "missing selected case hash",
    )

    group_raw = {
        "end_exclusive": hex_offset(GROUP_END),
        "raw_record_count": 12,
        "raw_records": [
            {"raw": raw} for raw in EXPECTED_GROUP_RECORDS
        ],
        "record_width": 3,
        "start": hex_offset(GROUP_START),
    }
    group_decoded = {
        "decoded_records": [{} for _ in range(12)],
        "group_handler_pointers": list(EXPECTED_GROUP_HANDLER_POINTERS),
        "segment_override_proof": {
            "expanded_primary_opcodes": ["0x26", "0x2e", "0x36", "0x3e"],
            "group": "0x08",
            "handler_pointer": "0x60bc",
            "proven": True,
            "raw_record": "e72608",
        },
    }
    validate_group_artifacts(group_raw, group_decoded)
    for field, value, label in (
        ("start", "0x66900", "wrong group-table start"),
        ("end_exclusive", "0x66920", "wrong group-table end"),
        ("raw_record_count", 11, "incomplete twelve-record decode"),
        ("record_width", 4, "wrong group record width"),
    ):
        bad_raw = copy.deepcopy(group_raw)
        bad_raw[field] = value
        reject(
            lambda bad_raw=bad_raw: validate_group_artifacts(
                bad_raw, group_decoded
            ),
            label,
        )
    bad_group = copy.deepcopy(group_decoded)
    bad_group["group_handler_pointers"][-1] = None
    reject(
        lambda: validate_group_artifacts(group_raw, bad_group),
        "incomplete group-handler link",
    )
    bad_raw = copy.deepcopy(group_raw)
    bad_raw["raw_records"][2]["raw"] = "fc8002"
    reject(
        lambda: validate_group_artifacts(bad_raw, group_decoded),
        "overlapping or ambiguous mask expansion",
    )
    bad_group = copy.deepcopy(group_decoded)
    bad_group["segment_override_proof"]["expanded_primary_opcodes"] = [
        "0x26"
    ]
    reject(
        lambda: validate_group_artifacts(group_raw, bad_group),
        "incorrect e7/26 segment expansion",
    )

    fpu_raw = {
        "tables": [
            {
                "end_exclusive": hex_offset(table["end"]),
                "name": table["name"],
                "raw_record_count": table["record_count"],
                "start": hex_offset(table["start"]),
            }
            for table in FPU_TABLES
        ],
        "total_raw_record_count": 87,
    }
    decoded_records = [
        {"handler_pointer": "0x5c00"} for _ in range(87)
    ]
    fpu_decoded = {
        "primary_opcode_inventory": [
            f"0x{opcode:02x}" for opcode in range(0xD8, 0xE0)
        ],
        "tables": [
            {"decoded_records": decoded_records[:22]},
            {"decoded_records": decoded_records[22:35]},
            {"decoded_records": decoded_records[35:58]},
            {"decoded_records": decoded_records[58:]},
        ],
        "total_raw_record_count": 87,
    }
    fpu_mnemonics = {
        "individual_mnemonic_count": len(EXPECTED_FPU_MNEMONICS),
        "individual_mnemonics": sorted(EXPECTED_FPU_MNEMONICS),
        "ownership_rows": [
            {"owner": f"owner-{index:02d}"} for index in range(87)
        ],
    }
    validate_fpu_artifacts(fpu_raw, fpu_decoded, fpu_mnemonics)
    bad_raw = copy.deepcopy(fpu_raw)
    bad_raw["tables"][0]["start"] = "0x66b3b"
    reject(
        lambda: validate_fpu_artifacts(
            bad_raw, fpu_decoded, fpu_mnemonics
        ),
        "incorrect D8-DF table boundary",
    )
    bad_names = copy.deepcopy(fpu_mnemonics)
    bad_names["individual_mnemonics"].remove("FADD")
    reject(
        lambda: validate_fpu_artifacts(fpu_raw, fpu_decoded, bad_names),
        "incomplete FPU mnemonic table",
    )
    bad_names = copy.deepcopy(fpu_mnemonics)
    bad_names["ownership_rows"][1]["owner"] = "owner-00"
    reject(
        lambda: validate_fpu_artifacts(fpu_raw, fpu_decoded, bad_names),
        "duplicate mnemonic ownership",
    )
    bad_decoded = copy.deepcopy(fpu_decoded)
    bad_decoded["tables"][0]["decoded_records"][0][
        "handler_pointer"
    ] = None
    reject(
        lambda: validate_fpu_artifacts(fpu_raw, bad_decoded, fpu_mnemonics),
        "missing FPU group-handler link",
    )
    bad_decoded = copy.deepcopy(fpu_decoded)
    bad_decoded["primary_opcode_inventory"] = ["0x66", "0x67"]
    reject(
        lambda: validate_fpu_artifacts(fpu_raw, bad_decoded, fpu_mnemonics),
        "FPO2 conclusion without bounded dispatch trace",
    )

    decoder_path = {
        "accepts_primary_0x66": False,
        "accepts_primary_0x67": False,
        "neutral_pseudocode": ["bounded path"],
        "raw_sha256": "c" * 64,
        "reachable_from_normal_disassembler_entry": True,
    }
    decoder = {
        "all_reachable_alternatives_bounded": True,
        "fallback_primary_range": ["0xd8", "0xdf"],
        "fpu_table_primary_inventory": [
            f"0x{opcode:02x}" for opcode in range(0xD8, 0xE0)
        ],
        "paths": [copy.deepcopy(decoder_path) for _ in range(6)],
        "primary_0x66_result": "rejected-as-unknown",
        "primary_0x67_result": "rejected-as-unknown",
    }
    validate_decoder_artifact(decoder)
    bad_decoder = copy.deepcopy(decoder)
    bad_decoder["paths"][0][
        "reachable_from_normal_disassembler_entry"
    ] = False
    reject(
        lambda: validate_decoder_artifact(bad_decoder),
        "unreachable path presented as support",
    )
    bad_decoder = copy.deepcopy(decoder)
    bad_decoder["paths"] = bad_decoder["paths"][:-1]
    reject(
        lambda: validate_decoder_artifact(bad_decoder),
        "incomplete decoder alternative coverage",
    )

    # Parser/format checks use small deterministic fixtures.
    record = {
        "group": "0x08",
        "index": 0,
        "mask": "0xe7",
        "value": "0x26",
    }
    if m60b.expand_record(record) != ["0x26", "0x2e", "0x36", "0x3e"]:
        raise AssertionError("positive e7/26 expansion differs")
    bad_record = dict(record)
    bad_record["mask"] = "0xff"
    reject(
        lambda: (
            None
            if m60b.expand_record(bad_record)
            == ["0x26", "0x2e", "0x36", "0x3e"]
            else (_ for _ in ()).throw(M60cError("wrong segment expansion"))
        ),
        "incorrect e7/26 expansion",
    )
    reject(
        lambda: validate_support_conclusions(
            {
                **good_conclusion,
                "forms": {
                    **good_conclusion["forms"],
                    "66": {
                        **good_conclusion["forms"]["66"],
                        "top_level_classification_after": "known_target_gap",
                    },
                },
            }
        ),
        "promotion/demotion of 66",
    )

    payload = [{"record_hash": "a" * 64}, {"record_hash": "b" * 64}]
    with tempfile.TemporaryDirectory(prefix="vaeg-m60c-selftest-") as temp:
        first = pathlib.Path(temp) / "first.json.gz"
        second = pathlib.Path(temp) / "second.json.gz"
        ratchet.write_deterministic_gzip(first, payload)
        ratchet.write_deterministic_gzip(second, payload)
        if first.read_bytes() != second.read_bytes():
            raise AssertionError("deterministic gzip output differs")
        nondeterministic = pathlib.Path(temp) / "bad.json.gz"
        nondeterministic.write_bytes(gzip.compress(canonical_bytes(payload), mtime=1))
        reject(
            lambda: ratchet.read_deterministic_gzip(nondeterministic),
            "nondeterministic compression",
        )

    print(
        "m60c-selftest: 4 positive and "
        f"{len(negative)} fail-closed checks passed"
    )


def deterministic_regeneration(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    rom_path: pathlib.Path,
    evaluated_sha: str,
) -> None:
    with tempfile.TemporaryDirectory(prefix="vaeg-m60c-regenerate-a-") as a:
        with tempfile.TemporaryDirectory(prefix="vaeg-m60c-regenerate-b-") as b:
            for destination in (pathlib.Path(a), pathlib.Path(b)):
                manifest, manifest_sha = write_authority_pack(
                    root,
                    destination,
                    dataset_root,
                    rom_path,
                    evaluated_sha,
                )
                write_transition_and_result_manifest(
                    root,
                    destination,
                    manifest,
                    manifest_sha,
                    evaluated_sha,
                )
            files_a = sorted(
                path.relative_to(a)
                for path in pathlib.Path(a).rglob("*")
                if path.is_file()
            )
            files_b = sorted(
                path.relative_to(b)
                for path in pathlib.Path(b).rglob("*")
                if path.is_file()
            )
            if files_a != files_b:
                raise M60cError("deterministic regeneration file inventory differs")
            for relative in files_a:
                if (pathlib.Path(a) / relative).read_bytes() != (
                    pathlib.Path(b) / relative
                ).read_bytes():
                    raise M60cError(
                        f"deterministic regeneration differs: {relative}"
                    )
    print(
        "m60c-regenerate: complete authority/transition artifacts are "
        "byte-identical in the pinned environment"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest")
    static = subparsers.add_parser("verify-static")
    static.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    static.add_argument("--protected-evidence-only", action="store_true")
    generate = subparsers.add_parser("generate")
    generate.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    generate.add_argument("--output-root", type=pathlib.Path, required=True)
    generate.add_argument("--dataset-root", type=pathlib.Path, required=True)
    generate.add_argument("--rom", type=pathlib.Path, required=True)
    generate.add_argument("--evaluated-sha", required=True)
    verify = subparsers.add_parser("verify-authority")
    verify.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    verify.add_argument("--rom", type=pathlib.Path, required=True)
    regenerate = subparsers.add_parser("regenerate-twice")
    regenerate.add_argument(
        "--root", type=pathlib.Path, default=pathlib.Path(".")
    )
    regenerate.add_argument("--dataset-root", type=pathlib.Path, required=True)
    regenerate.add_argument("--rom", type=pathlib.Path, required=True)
    regenerate.add_argument("--evaluated-sha", required=True)
    arguments = parser.parse_args()
    try:
        if arguments.command == "selftest":
            selftest()
        elif arguments.command == "verify-static":
            verify_static(
                arguments.root.resolve(), arguments.protected_evidence_only
            )
        elif arguments.command == "generate":
            root = arguments.root.resolve()
            output_root = arguments.output_root.resolve()
            manifest, digest = write_authority_pack(
                root,
                output_root,
                arguments.dataset_root.resolve(),
                arguments.rom.resolve(),
                arguments.evaluated_sha,
            )
            write_transition_and_result_manifest(
                root,
                output_root,
                manifest,
                digest,
                arguments.evaluated_sha,
            )
        elif arguments.command == "verify-authority":
            verify_authority(
                arguments.root.resolve(), arguments.rom.resolve()
            )
        else:
            deterministic_regeneration(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.rom.resolve(),
                arguments.evaluated_sha,
            )
    except (
        M60cError,
        OSError,
        json.JSONDecodeError,
        ssts.CorpusError,
        ratchet.RatchetError,
        m60b.M60bError,
    ) as error:
        print(f"m60c-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

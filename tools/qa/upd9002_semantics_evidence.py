#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""Generate and validate the behavior-neutral M59 uPD9002 evidence pack."""

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
from collections.abc import Iterable
from typing import Any

import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


sys.dont_write_bytecode = True

APPROVED_PREDECESSOR_GATE = "G58"
APPROVED_PREDECESSOR_SHA = "bc8a55c6da1082b85b794068e0d933e31fe46b13"
G58_EVALUATED_SHA = "d384dbc4d0b7dcece50499a9847b2bd9eb8b3d53"
CANDIDATE_GATE = "G59"
MILESTONE = "M59"
SCHEMA_VERSION = 1
GENERATOR_VERSION = "1"
G58_TREE_SHA256 = "44776ad3a961ae564517ae0aa17e2987d3732c9d949774cbe77dce413092e1c2"
G43_MANIFEST_SHA256 = (
    "77dd1e53f325f3910bd727d3dec4b9c1e23c005f0b306c085a34569e9cf5b23f"
)
M47_DOCUMENT_MANIFEST_SHA256 = (
    "2fe6d19336d091f31f98211ae9056e68a9ab453505ce72632ed40fb3fd2819cc"
)
M47_REPORT_SHA256 = (
    "3a0543f960a5b79a55f48d5fee071325b0b5549c4e73c7b680a4879a928734c3"
)
DATASET_ID = (
    "ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-"
    "1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4"
)
FULL_SELECTED_HASH_SET_SHA256 = (
    "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7"
)
FULL_APPLICABLE_HASH_SET_SHA256 = (
    "7de13cbd54e709e0d0d0abefedac876306c8a67c7936f6a26c983362fed6d23c"
)
ARCHITECTURAL_CONTRACT_ID = "upd9002-v20-architectural-v1"
ARCHITECTURAL_CONTRACT_SHA256 = (
    "aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5"
)
FINGERPRINT_CONTRACT_ID = "upd9002-v20-fingerprint-v1"
FINGERPRINT_CONTRACT_SHA256 = (
    "47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e"
)

SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
FORM_RE = re.compile(
    r"^(?:[0-9A-F]{2}|[0-9A-F]{2}\.[0-7]|0F[0-9A-F]{2})$"
)
CONCLUSION_STATUSES = {"proven", "hypothesis", "underdetermined"}
CLASSIFICATIONS = {
    "applicable",
    "known_target_gap",
    "expected_target_divergence",
    "unsupported_fixture",
    "upstream_nonblocking",
}
GAP_KINDS = {
    "documented_silicon_absent",
    "implementation_missing",
    "target_support_unverified",
}
PREFIXES = {0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0xF0, 0xF1, 0xF2, 0xF3}
SEGMENT_PREFIXES = {0x26: "es", 0x2E: "cs", 0x36: "ss", 0x3E: "ds"}
RM_WORD_REGISTERS = ("ax", "cx", "dx", "bx", "sp", "bp", "si", "di")
FLAG_BITS = {"cf": 0, "pf": 2, "af": 4, "zf": 6, "sf": 7, "of": 11}
REQUIRED_COUNTS = {0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 255}
REQUIRED_IMMEDIATES = {0, 1, 2, 9, 10, 11, 16, 255}

ITEM_FORMS: dict[str, tuple[str, ...]] = {
    "flags": ("9C", "9D", "9E", "9F", "CC", "CD", "CE"),
    "canary": ("C6", "C7", "F7.2"),
    "d4_d5": ("D4", "D5"),
    "0f28_0f2a": ("0F28", "0F2A"),
    "shifts": tuple(
        f"{opcode}.{subform}"
        for opcode in ("C0", "C1", "D2", "D3")
        for subform in range(8)
    ),
    "ff7_bound": ("62", "FF.7"),
}

ROW_KEYS = {
    "actual_final_state",
    "analysis",
    "architectural_mismatch_kinds",
    "case_hash",
    "conclusion_status",
    "diagnostic_replayed",
    "evidence_notes",
    "executed",
    "expected_final_state",
    "fingerprint_mismatch_kinds",
    "fingerprint_outcome",
    "flags_comparison",
    "form",
    "gap_kind",
    "initial_architectural_state",
    "instruction_bytes",
    "item",
    "logical_addresses",
    "official_outcome",
    "physical_addresses",
    "primary_partition",
    "profile",
    "scope",
    "selected",
    "structural_partitions",
    "top_level_classification",
    "upstream_case_hash",
}


class EvidenceError(ValueError):
    """An evidence input, schema, or deterministic invariant failed closed."""


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    ).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value) + b"\n")


def read_canonical_json(path: pathlib.Path) -> Any:
    try:
        raw = path.read_bytes()
        value = json.loads(raw.decode("utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise EvidenceError(f"{path}: invalid UTF-8 JSON: {error}") from error
    if raw != canonical_bytes(value) + b"\n":
        raise EvidenceError(f"{path}: JSON serialization is not canonical")
    return value


def require_keys(value: Any, keys: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != keys:
        actual = sorted(value) if isinstance(value, dict) else type(value).__name__
        raise EvidenceError(
            f"{where}: schema keys differ: expected={sorted(keys)!r} actual={actual!r}"
        )
    return value


def require_count(value: Any, where: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise EvidenceError(f"{where}: expected non-negative integer")
    return value


def require_sha1(value: Any, where: str) -> str:
    if not isinstance(value, str) or SHA1_RE.fullmatch(value) is None:
        raise EvidenceError(f"{where}: expected lowercase SHA-1")
    return value


def require_sha256(value: Any, where: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        raise EvidenceError(f"{where}: expected lowercase SHA-256")
    return value


def require_commit(value: Any, where: str) -> str:
    if not isinstance(value, str) or SHA1_RE.fullmatch(value) is None:
        raise EvidenceError(f"{where}: expected 40-hex commit SHA")
    return value


def require_exact(value: Any, expected: Any, where: str) -> Any:
    if value != expected:
        raise EvidenceError(f"{where}: identity differs")
    return value


def require_file_identity(
    path: pathlib.Path, expected_bytes: Any, expected_sha256: Any, where: str
) -> None:
    byte_count = require_count(expected_bytes, f"{where}.bytes")
    digest = require_sha256(expected_sha256, f"{where}.sha256")
    if path.stat().st_size != byte_count:
        raise EvidenceError(f"{where}: byte count differs")
    if sha256_file(path) != digest:
        raise EvidenceError(f"{where}: artifact digest mismatch")


def require_row_count(value: Any, actual: int, where: str) -> None:
    if require_count(value, f"{where}.rows") != actual:
        raise EvidenceError(f"{where}: manifest row count differs")


def hash_set_digest(values: Iterable[str]) -> str:
    ordered = sorted(values)
    if len(ordered) != len(set(ordered)):
        raise EvidenceError("hash set contains duplicates")
    for index, value in enumerate(ordered):
        require_sha256(value, f"hash_set[{index}]")
    return sha256_bytes(canonical_bytes(ordered))


def g58_paths(root: pathlib.Path) -> list[pathlib.Path]:
    paths = [
        *root.glob("tests/ssts/scoreboard/g58*.json"),
        *root.glob("tests/ssts/scoreboard/g58*/*.json.gz"),
        *root.glob("tests/ssts/transitions/g58*.json"),
    ]
    return sorted((path for path in paths if path.is_file()), key=lambda path: path.as_posix())


def tree_digest(root: pathlib.Path, paths: Iterable[pathlib.Path]) -> str:
    rows = [
        {
            "path": path.relative_to(root).as_posix(),
            "sha256": sha256_file(path),
            "size": path.stat().st_size,
        }
        for path in sorted(paths, key=lambda item: item.as_posix())
    ]
    return sha256_bytes(canonical_bytes(rows))


def verify_protected_inputs(root: pathlib.Path) -> dict[str, Any]:
    paths = g58_paths(root)
    if len(paths) != 34:
        raise EvidenceError(f"approved G58 artifact count differs: {len(paths)}")
    digest = tree_digest(root, paths)
    require_exact(digest, G58_TREE_SHA256, "approved G58 artifacts")
    g43 = root / "tests/ssts/epochs/g43/manifest.json"
    require_exact(
        sha256_file(g43), G43_MANIFEST_SHA256, "immutable G43 manifest"
    )
    try:
        ratchet.verify_static(root)
    except ratchet.RatchetError as error:
        raise EvidenceError(str(error)) from error
    architectural = read_canonical_json(
        root / "tests/ssts/scoreboard/g58_architectural_full.json"
    )
    fingerprint = read_canonical_json(
        root / "tests/ssts/scoreboard/g58_fingerprint_full.json"
    )
    for value, profile in (
        (architectural, "architectural"),
        (fingerprint, "fingerprint"),
    ):
        if value["evaluated_sha"] != G58_EVALUATED_SHA:
            raise EvidenceError(f"G58 {profile} evaluated_sha changed")
        if value["dataset_id"] != DATASET_ID:
            raise EvidenceError(f"G58 {profile} dataset changed")
        if value["selected_hash_set_sha256"] != FULL_SELECTED_HASH_SET_SHA256:
            raise EvidenceError(f"G58 {profile} selected set changed")
        if value["applicable_hash_set_sha256"] != FULL_APPLICABLE_HASH_SET_SHA256:
            raise EvidenceError(f"G58 {profile} applicable set changed")
    return {
        "g43_manifest_sha256": G43_MANIFEST_SHA256,
        "g58_artifact_count": len(paths),
        "g58_artifact_tree_sha256": digest,
    }


def hex_registers(registers: dict[str, int]) -> dict[str, str]:
    return {name: f"{registers[name]:04x}" for name in ssts.REGISTER_ORDER}


def ram_entries(memory: dict[int, int] | list[list[int]]) -> list[dict[str, str]]:
    source = (
        {address: value for address, value in memory}
        if isinstance(memory, list)
        else memory
    )
    return [
        {"address": f"{address:05x}", "value": f"{source[address]:02x}"}
        for address in sorted(source)
    ]


def state_object(
    registers: dict[str, int],
    memory: dict[int, int],
    termination: str,
    execution: dict[str, Any],
) -> dict[str, Any]:
    return {
        "execution": execution,
        "ram": ram_entries(memory),
        "registers": hex_registers(registers),
        "termination": termination,
    }


def expected_execution(record: dict[str, Any], form: str) -> dict[str, Any]:
    if form == "F4":
        return {"kind": "halt", "interrupt_vector": None}
    memory = {address: value for address, value in record["initial"]["ram"]}
    expected = ssts.expected_registers(record)
    vectors = []
    for vector in range(256):
        base = vector * 4
        if all(base + offset in memory for offset in range(4)):
            ip = memory[base] | memory[base + 1] << 8
            cs = memory[base + 2] | memory[base + 3] << 8
            if expected["ip"] == ip and expected["cs"] == cs:
                vectors.append(vector)
    if len(vectors) == 1:
        return {"kind": "interrupt", "interrupt_vector": vectors[0]}
    if len(vectors) > 1:
        return {"kind": "underdetermined", "interrupt_vector": None}
    return {"kind": "normal", "interrupt_vector": None}


def instruction_layout(record: dict[str, Any], form: str) -> dict[str, Any]:
    data = record["bytes"]
    position = 0
    segment_override = None
    while position < len(data) and data[position] in PREFIXES:
        if data[position] in SEGMENT_PREFIXES:
            segment_override = SEGMENT_PREFIXES[data[position]]
        position += 1
    prefix_count = position
    if position >= len(data):
        raise EvidenceError(f"{form}: prefix-only instruction")
    opcode = data[position]
    position += 1
    second_byte = None
    if opcode == 0x0F:
        if position >= len(data):
            raise EvidenceError(f"{form}: missing 0F second byte")
        second_byte = data[position]
        position += 1
    modrm = None
    group = form.split(".", 1)[0] in ssts.GROUP_FORMS or form in {"62", "0F28", "0F2A"}
    if group:
        if position >= len(data):
            raise EvidenceError(f"{form}: missing ModR/M")
        modrm = data[position]
        position += 1
    displacement = 0
    displacement_width = 0
    mod = None
    rm = None
    if modrm is not None:
        mod = modrm >> 6
        rm = modrm & 7
        if mod == 0 and rm == 6:
            if position + 2 > len(data):
                raise EvidenceError(f"{form}: missing direct displacement")
            displacement = data[position] | data[position + 1] << 8
            displacement_width = 16
            position += 2
        elif mod == 1:
            if position >= len(data):
                raise EvidenceError(f"{form}: missing byte displacement")
            displacement = data[position]
            if displacement & 0x80:
                displacement -= 0x100
            displacement_width = 8
            position += 1
        elif mod == 2:
            if position + 2 > len(data):
                raise EvidenceError(f"{form}: missing word displacement")
            displacement = data[position] | data[position + 1] << 8
            if displacement & 0x8000:
                displacement -= 0x10000
            displacement_width = 16
            position += 2
    immediate_width = {
        "C6": 8,
        "C7": 16,
        "C0": 8,
        "C1": 8,
        "D4": 8,
        "D5": 8,
    }.get(form.split(".", 1)[0], 0)
    immediate = None
    if immediate_width:
        byte_count = immediate_width // 8
        if position + byte_count > len(data):
            raise EvidenceError(f"{form}: missing immediate")
        immediate = sum(data[position + index] << (8 * index) for index in range(byte_count))
    return {
        "displacement": displacement,
        "displacement_width": displacement_width,
        "immediate": immediate,
        "immediate_width": immediate_width,
        "mod": mod,
        "modrm": modrm,
        "opcode": opcode,
        "operand_position": position,
        "prefix_count": prefix_count,
        "register_or_memory": (
            None if mod is None else ("register" if mod == 3 else "memory")
        ),
        "rm": rm,
        "second_byte": second_byte,
        "segment_override": segment_override,
    }


def effective_address(
    layout: dict[str, Any], registers: dict[str, int], width: int
) -> dict[str, Any] | None:
    mod = layout["mod"]
    rm = layout["rm"]
    if mod is None or mod == 3 or rm is None:
        return None
    if mod == 0 and rm == 6:
        raw_offset = layout["displacement"]
        default_segment = "ds"
    else:
        bases = (
            ("bx", "si"),
            ("bx", "di"),
            ("bp", "si"),
            ("bp", "di"),
            ("si",),
            ("di",),
            ("bp",),
            ("bx",),
        )
        raw_offset = sum(registers[name] for name in bases[rm])
        raw_offset += layout["displacement"]
        default_segment = "ss" if rm in {2, 3, 6} else "ds"
    offset = raw_offset & 0xFFFF
    segment_name = layout["segment_override"] or default_segment
    segment = registers[segment_name]
    linear = (segment << 4) + offset
    physical = linear & 0xFFFFF
    return {
        "logical_offset": offset,
        "offset_wrap": raw_offset < 0 or raw_offset > 0xFFFF,
        "physical_address": physical,
        "physical_wrap": linear + width - 1 > 0xFFFFF,
        "segment": segment_name,
        "segment_value": segment,
    }


def register_operand(registers: dict[str, int], rm: int, width: int) -> int:
    if width == 16:
        return registers[RM_WORD_REGISTERS[rm]]
    if rm < 4:
        return registers[RM_WORD_REGISTERS[rm]] & 0xFF
    return (registers[RM_WORD_REGISTERS[rm - 4]] >> 8) & 0xFF


def destination_value(
    registers: dict[str, int],
    memory: dict[int, int],
    layout: dict[str, Any],
    address: dict[str, Any] | None,
    width: int,
) -> int | None:
    if layout["mod"] == 3:
        assert layout["rm"] is not None
        return register_operand(registers, layout["rm"], width)
    if address is None:
        return None
    physical = address["physical_address"]
    bytes_needed = width // 8
    if any(((physical + index) & 0xFFFFF) not in memory for index in range(bytes_needed)):
        return None
    return sum(
        memory[(physical + index) & 0xFFFFF] << (8 * index)
        for index in range(bytes_needed)
    )


def memory_from_entries(entries: list[dict[str, str]]) -> dict[int, int]:
    return {
        int(entry["address"], 16): int(entry["value"], 16)
        for entry in entries
    }


def word_at(memory: dict[int, int], addresses: list[int], index: int) -> int | None:
    first = addresses[index]
    second = addresses[index + 1]
    if first not in memory or second not in memory:
        return None
    return memory[first] | memory[second] << 8


def frame_analysis(
    record: dict[str, Any],
    expected_regs: dict[str, int],
    actual: dict[str, Any],
    expected_ram: dict[int, int],
) -> tuple[dict[str, Any], list[dict[str, str]], list[dict[str, str]], str]:
    initial = record["initial"]["regs"]
    actual_regs = actual["registers"]
    ss = initial["ss"]
    expected_sp = expected_regs["sp"]
    actual_sp = actual_regs["sp"]
    expected_logical = [(expected_sp + index) & 0xFFFF for index in range(6)]
    actual_logical = [(actual_sp + index) & 0xFFFF for index in range(6)]
    expected_physical = [((ss << 4) + offset) & 0xFFFFF for offset in expected_logical]
    actual_physical = [((ss << 4) + offset) & 0xFFFFF for offset in actual_logical]
    actual_ram = actual["ram"]
    expected_words = [
        word_at(expected_ram, expected_physical, offset) for offset in (0, 2, 4)
    ]
    actual_words = [
        word_at(actual_ram, actual_physical, offset) for offset in (0, 2, 4)
    ]
    segment_cross = expected_sp > 0xFFFA
    unmasked = [(ss << 4) + offset for offset in expected_logical]
    physical_cross = any(value > 0xFFFFF for value in unmasked)
    mapping_determined = all(value is not None for value in actual_words)
    if not mapping_determined:
        partition = "mapping-underdetermined"
    elif segment_cross and physical_cross:
        partition = "segment-and-physical-boundary"
    elif segment_cross:
        partition = "segment-boundary"
    elif physical_cross:
        partition = "physical-boundary"
    else:
        partition = "no-boundary"
    logical = [
        {"kind": "expected-frame-byte", "value": f"{value:04x}"}
        for value in expected_logical
    ] + [
        {"kind": "actual-frame-byte", "value": f"{value:04x}"}
        for value in actual_logical
    ]
    physical = [
        {"kind": "expected-frame-byte", "value": f"{value:05x}"}
        for value in expected_physical
    ] + [
        {"kind": "actual-frame-byte", "value": f"{value:05x}"}
        for value in actual_physical
    ]
    analysis = {
        "actual_final_flags": f"{actual_regs['flags']:04x}",
        "actual_final_sp": f"{actual_sp:04x}",
        "actual_frame_logical_addresses": [
            f"{value:04x}" for value in actual_logical
        ],
        "actual_frame_physical_addresses": [
            f"{value:05x}" for value in actual_physical
        ],
        "actual_saved_cs": (
            None if actual_words[1] is None else f"{actual_words[1]:04x}"
        ),
        "actual_saved_flags": (
            None if actual_words[2] is None else f"{actual_words[2]:04x}"
        ),
        "actual_saved_ip": (
            None if actual_words[0] is None else f"{actual_words[0]:04x}"
        ),
        "expected_final_sp": f"{expected_sp:04x}",
        "expected_final_flags": f"{expected_regs['flags']:04x}",
        "expected_frame_logical_addresses": [
            f"{value:04x}" for value in expected_logical
        ],
        "expected_frame_physical_addresses": [
            f"{value:05x}" for value in expected_physical
        ],
        "expected_saved_cs": (
            None if expected_words[1] is None else f"{expected_words[1]:04x}"
        ),
        "expected_saved_flags": (
            None if expected_words[2] is None else f"{expected_words[2]:04x}"
        ),
        "expected_saved_ip": (
            None if expected_words[0] is None else f"{expected_words[0]:04x}"
        ),
        "frame_mapping": "determined" if mapping_determined else "underdetermined",
        "initial_cs": f"{initial['cs']:04x}",
        "initial_flags": f"{initial['flags']:04x}",
        "initial_ip": f"{initial['ip']:04x}",
        "initial_sp": f"{initial['sp']:04x}",
        "initial_ss": f"{ss:04x}",
        "physical_boundary_crossed": physical_cross,
        "segment_boundary_crossed": segment_cross,
    }
    return analysis, logical, physical, partition


def flag_bit_values(value: int) -> dict[str, int]:
    return {name: (value >> bit) & 1 for name, bit in FLAG_BITS.items()}


def shift_value(value: int, count: int, width: int, subform: int) -> int:
    mask = (1 << width) - 1
    value &= mask
    for _ in range(count):
        if subform in {4, 6}:
            value = value << 1 & mask
        elif subform == 5:
            value >>= 1
        elif subform == 7:
            value = (value >> 1) | (value & (1 << (width - 1)))
        else:
            return value
    return value


def shift_count_observation(
    initial: int | None, expected: int | None, raw_count: int, width: int, subform: int
) -> dict[str, Any]:
    if initial is None or expected is None:
        return {
            "effective_count": None,
            "matches_mask_1f": None,
            "matches_raw": None,
            "status": "underdetermined",
        }
    raw_result = shift_value(initial, raw_count, width, subform)
    masked = raw_count & 0x1F
    masked_result = shift_value(initial, masked, width, subform)
    matches_raw = raw_result == expected
    matches_mask = masked_result == expected
    candidates = {
        candidate
        for candidate, matches in ((raw_count, matches_raw), (masked, matches_mask))
        if matches
    }
    return {
        "effective_count": next(iter(candidates)) if len(candidates) == 1 else None,
        "matches_mask_1f": matches_mask,
        "matches_raw": matches_raw,
        "status": (
            "unique-between-tested-models"
            if len(candidates) == 1
            else ("ambiguous-between-tested-models" if candidates else "no-tested-model")
        ),
    }


def item_analysis(
    item: str,
    form: str,
    record: dict[str, Any],
    expected_regs: dict[str, int],
    expected_ram: dict[int, int],
    actual: dict[str, Any],
    architectural_mismatches: list[str],
) -> tuple[dict[str, Any], dict[str, Any], str, list[dict[str, str]], list[dict[str, str]]]:
    layout = instruction_layout(record, form)
    expected_execution_value = expected_execution(record, form)
    actual_execution = actual["execution_result"]
    partitions: dict[str, Any] = {
        "actual_execution_kind": actual_execution["kind"],
        "displacement_width": layout["displacement_width"],
        "expected_execution_kind": expected_execution_value["kind"],
        "form": form,
        "immediate_width": layout["immediate_width"],
        "mismatch_class": (
            "none" if not architectural_mismatches else "+".join(architectural_mismatches)
        ),
        "modrm_mode": layout["mod"],
        "register_or_memory": layout["register_or_memory"],
    }
    analysis: dict[str, Any] = {}
    logical: list[dict[str, str]] = []
    physical: list[dict[str, str]] = []
    primary = form.lower()

    if item == "flags":
        if form in {"CC", "CD", "CE"}:
            if expected_execution(record, form)["kind"] == "interrupt":
                analysis, logical, physical, primary = frame_analysis(
                    record, expected_regs, actual, expected_ram
                )
            else:
                analysis = {
                    "actual_final_flags": f"{actual['registers']['flags']:04x}",
                    "expected_final_flags": f"{expected_regs['flags']:04x}",
                    "initial_flags": f"{record['initial']['regs']['flags']:04x}",
                }
                primary = "no-interrupt-expected"
        elif form == "9C":
            initial = record["initial"]["regs"]
            expected_sp = expected_regs["sp"]
            actual_sp = actual["registers"]["sp"]
            expected_addresses = [
                ((initial["ss"] << 4) + ((expected_sp + index) & 0xFFFF)) & 0xFFFFF
                for index in range(2)
            ]
            actual_addresses = [
                ((initial["ss"] << 4) + ((actual_sp + index) & 0xFFFF)) & 0xFFFFF
                for index in range(2)
            ]
            expected_logical = [(expected_sp + index) & 0xFFFF for index in range(2)]
            actual_logical = [(actual_sp + index) & 0xFFFF for index in range(2)]
            segment_cross = expected_sp == 0xFFFF
            physical_cross = any(
                (initial["ss"] << 4) + offset > 0xFFFFF
                for offset in expected_logical
            )
            expected_word = word_at(expected_ram, expected_addresses, 0)
            actual_word = word_at(actual["ram"], actual_addresses, 0)
            analysis = {
                "actual_stack_logical_addresses": [
                    f"{value:04x}" for value in actual_logical
                ],
                "actual_pushed_flags": (
                    None if actual_word is None else f"{actual_word:04x}"
                ),
                "actual_stack_physical_addresses": [
                    f"{value:05x}" for value in actual_addresses
                ],
                "expected_pushed_flags": (
                    None if expected_word is None else f"{expected_word:04x}"
                ),
                "expected_stack_logical_addresses": [
                    f"{value:04x}" for value in expected_logical
                ],
                "expected_stack_physical_addresses": [
                    f"{value:05x}" for value in expected_addresses
                ],
                "initial_flags": f"{initial['flags']:04x}",
                "physical_boundary_crossed": physical_cross,
                "segment_boundary_crossed": segment_cross,
            }
            logical = [
                {"kind": "expected-stack-byte", "value": f"{value:04x}"}
                for value in expected_logical
            ] + [
                {"kind": "actual-stack-byte", "value": f"{value:04x}"}
                for value in actual_logical
            ]
            physical = [
                {"kind": "expected-stack-byte", "value": f"{value:05x}"}
                for value in expected_addresses
            ] + [
                {"kind": "actual-stack-byte", "value": f"{value:05x}"}
                for value in actual_addresses
            ]
            if segment_cross and physical_cross:
                primary = "pushf-segment-and-physical-boundary"
            elif segment_cross:
                primary = "pushf-segment-boundary"
            elif physical_cross:
                primary = "pushf-physical-boundary"
            else:
                primary = "pushf-no-boundary"
        elif form == "9F":
            analysis = {
                "actual_ah": f"{actual['registers']['ax'] >> 8:02x}",
                "expected_ah": f"{expected_regs['ax'] >> 8:02x}",
                "initial_flags": f"{record['initial']['regs']['flags']:04x}",
            }
            primary = "lahf-image"
        elif form == "9D":
            initial = record["initial"]["regs"]
            address = ((initial["ss"] << 4) + initial["sp"]) & 0xFFFFF
            initial_memory = {a: v for a, v in record["initial"]["ram"]}
            input_addresses = [address, (address + 1) & 0xFFFFF]
            input_word = word_at(initial_memory, input_addresses, 0)
            if input_word is None:
                raise EvidenceError(f"9D:{record['hash']}: stack input is absent")
            analysis = {
                "actual_final_flags": f"{actual['registers']['flags']:04x}",
                "expected_final_flags": f"{expected_regs['flags']:04x}",
                "initial_flags": f"{initial['flags']:04x}",
                "stack_input_flags": f"{input_word:04x}",
            }
            primary = "popf-load"
        elif form == "9E":
            analysis = {
                "actual_final_flags": f"{actual['registers']['flags']:04x}",
                "expected_final_flags": f"{expected_regs['flags']:04x}",
                "initial_ah": f"{record['initial']['regs']['ax'] >> 8:02x}",
                "initial_flags": f"{record['initial']['regs']['flags']:04x}",
            }
            primary = "sahf-load"

    if item in {"canary", "shifts", "0f28_0f2a"}:
        width = 16 if form in {"C7", "F7.2"} or form.startswith(("C1.", "D3.")) else 8
        address = effective_address(layout, record["initial"]["regs"], width // 8)
        expected_value = destination_value(
            expected_regs, expected_ram, layout, address, width
        )
        actual_value = destination_value(
            actual["registers"], actual["ram"], layout, address, width
        )
        initial_ram = {a: v for a, v in record["initial"]["ram"]}
        initial_value = destination_value(
            record["initial"]["regs"], initial_ram, layout, address, width
        )
        analysis.update(
            {
                "actual_destination": (
                    None if actual_value is None else f"{actual_value:0{width // 4}x}"
                ),
                "expected_destination": (
                    None if expected_value is None else f"{expected_value:0{width // 4}x}"
                ),
                "initial_destination": (
                    None if initial_value is None else f"{initial_value:0{width // 4}x}"
                ),
                "operand_width": width,
            }
        )
        if address is not None:
            analysis.update(
                {
                    "logical_offset": f"{address['logical_offset']:04x}",
                    "offset_wrap": address["offset_wrap"],
                    "physical_address": f"{address['physical_address']:05x}",
                    "physical_wrap": address["physical_wrap"],
                    "segment": address["segment"],
                    "segment_value": f"{address['segment_value']:04x}",
                }
            )
            partitions.update(
                {
                    "offset_wrap": address["offset_wrap"],
                    "physical_wrap": address["physical_wrap"],
                    "segment": address["segment"],
                }
            )
            logical.append(
                {"kind": "operand", "value": f"{address['logical_offset']:04x}"}
            )
            physical.append(
                {"kind": "operand", "value": f"{address['physical_address']:05x}"}
            )
        else:
            partitions.update(
                {"offset_wrap": False, "physical_wrap": False, "segment": None}
            )
        primary = (
            f"{layout['register_or_memory'] or 'no-modrm'}:"
            f"{'pass' if not architectural_mismatches else 'failure'}"
        )

    if item == "d4_d5":
        immediate = layout["immediate"]
        analysis = {
            "actual_ax": f"{actual['registers']['ax']:04x}",
            "actual_flags": f"{actual['registers']['flags']:04x}",
            "expected_ax": f"{expected_regs['ax']:04x}",
            "expected_flags": f"{expected_regs['flags']:04x}",
            "immediate": immediate,
            "initial_ax": f"{record['initial']['regs']['ax']:04x}",
            "initial_flags": f"{record['initial']['regs']['flags']:04x}",
        }
        partitions["immediate"] = immediate
        primary = (
            f"immediate-{immediate}"
            if immediate in REQUIRED_IMMEDIATES
            else "immediate-other"
        )

    if item == "shifts":
        width = analysis["operand_width"]
        raw_count = (
            layout["immediate"]
            if form.startswith(("C0.", "C1."))
            else record["initial"]["regs"]["cx"] & 0xFF
        )
        subform = int(form.split(".")[1])
        initial_value = (
            None
            if analysis["initial_destination"] is None
            else int(analysis["initial_destination"], 16)
        )
        expected_value = (
            None
            if analysis["expected_destination"] is None
            else int(analysis["expected_destination"], 16)
        )
        analysis.update(
            {
                "actual_flags_bits": flag_bit_values(actual["registers"]["flags"]),
                "actual_count_observation": shift_count_observation(
                    initial_value,
                    actual_value,
                    raw_count,
                    width,
                    subform,
                ),
                "count_observation": shift_count_observation(
                    initial_value, expected_value, raw_count, width, subform
                ),
                "expected_flags_bits": flag_bit_values(expected_regs["flags"]),
                "initial_cf": record["initial"]["regs"]["flags"] & 1,
                "initial_sign_bit": (
                    None if initial_value is None else (initial_value >> (width - 1)) & 1
                ),
                "raw_count": raw_count,
                "subform": subform,
            }
        )
        partitions.update(
            {
                "count_source": "immediate" if form.startswith(("C0.", "C1.")) else "cl",
                "count_value": raw_count,
                "effective_count": analysis["count_observation"]["effective_count"],
                "initial_cf": analysis["initial_cf"],
                "initial_sign_bit": analysis["initial_sign_bit"],
                "operand_width": width,
                "subform": subform,
            }
        )
        primary = (
            f"{'rotate' if subform < 4 else 'shift'}:"
            f"{layout['register_or_memory']}:{width}"
        )

    if item == "0f28_0f2a":
        initial_ip = record["initial"]["regs"]["ip"]
        analysis.update(
            {
                "actual_ip_delta": (
                    actual["registers"]["ip"] - initial_ip
                ) & 0xFFFF,
                "encoded_length": len(record["bytes"]),
                "expected_ip_delta": (expected_regs["ip"] - initial_ip) & 0xFFFF,
                "prefix_count": layout["prefix_count"],
                "sst_observed_instruction": (
                    "ROL4" if form == "0F28" else "ROR4"
                ),
            }
        )
        primary = form.lower()

    if item == "ff7_bound":
        expected_exec = expected_execution_value
        actual_exec = actual_execution
        analysis = {
            "actual_execution": actual_exec,
            "expected_execution": expected_exec,
        }
        if form == "62":
            expected_int5 = (
                expected_exec["kind"] == "interrupt"
                and expected_exec["interrupt_vector"] == 5
            )
            actual_int5 = (
                actual_exec["interrupt_count"] == 1
                and actual_exec["interrupt_vector"] == 5
            )
            analysis["actual_range_result"] = (
                "interrupt-5" if actual_int5 else "normal"
            )
            analysis["expected_range_result"] = (
                "interrupt-5" if expected_int5 else "normal"
            )
        if form == "62" and expected_int5:
            frame, logical, physical, _ = frame_analysis(
                record, expected_regs, actual, expected_ram
            )
            analysis["interrupt_frame"] = frame
            frame_addresses = set(frame["expected_frame_physical_addresses"])
            failure = ssts.make_failure(
                DATASET_ID,
                "full",
                form,
                "applicable",
                0xFFFF,
                {
                    "record": record,
                    "record_digest": sha256_bytes(canonical_bytes(record)),
                    "watch": sorted(expected_ram),
                    "expected_ram": expected_ram,
                },
                "ok",
                actual,
            )["content"]
            non_frame_ram = [
                difference
                for difference in failure["ram_differences"]
                if difference["address"] not in frame_addresses
            ]
            non_frame_registers = [
                difference
                for difference in failure["register_differences"]
                if difference["register"] not in {"flags"}
            ]
            frame_only = (
                actual_int5
                and not non_frame_ram
                and not non_frame_registers
                and set(failure["mismatch_kinds"]).issubset({"ram", "registers"})
            )
            if expected_int5 != actual_int5:
                analysis["failure_partition"] = "range-result-mismatch"
            elif frame_only:
                analysis["failure_partition"] = "stack-frame-mismatch"
            elif non_frame_registers:
                analysis["failure_partition"] = "register-mismatch"
            elif non_frame_ram:
                analysis["failure_partition"] = "ram-mismatch"
            elif expected_exec["kind"] != actual_exec["kind"]:
                analysis["failure_partition"] = "termination-mismatch"
            else:
                analysis["failure_partition"] = "other"
        elif form == "62":
            if not architectural_mismatches:
                analysis["failure_partition"] = (
                    "interrupt" if actual_int5 else "normal-completion"
                )
            elif expected_int5 != actual_int5:
                analysis["failure_partition"] = "range-result-mismatch"
            elif "registers" in architectural_mismatches:
                analysis["failure_partition"] = "register-mismatch"
            elif "ram" in architectural_mismatches:
                analysis["failure_partition"] = "ram-mismatch"
            elif expected_exec["kind"] != actual_exec["kind"]:
                analysis["failure_partition"] = "termination-mismatch"
            else:
                analysis["failure_partition"] = "other"
        else:
            analysis["failure_partition"] = (
                "normal-completion"
                if actual_exec["kind"] == "normal"
                else "interrupt-or-exception"
            )
        primary = analysis["failure_partition"]

    partitions["primary"] = primary
    return analysis, partitions, primary, logical, physical


def gap_kind_map(root: pathlib.Path) -> tuple[dict[str, str], dict[str, Any]]:
    known = json.loads(
        (root / "tests/ssts/baseline/upd9002_v20_known_gaps.json").read_text(
            encoding="utf-8"
        )
    )
    taxonomy = json.loads(
        (root / "tests/ssts/gap_taxonomy.json").read_text(encoding="utf-8")
    )
    annotations = {
        entry["selector_sha256"]: entry for entry in taxonomy["annotations"]
    }
    result: dict[str, str] = {}
    zero_f28: dict[str, Any] | None = None
    for rule in known["rules"]:
        selector_digest = sha256_bytes(canonical_bytes(rule["selector"]))
        annotation = annotations.get(selector_digest)
        if annotation is None:
            raise EvidenceError("gap taxonomy selector is missing")
        for upstream_hash in rule["resolved_test_hashes"]:
            if upstream_hash in result:
                raise EvidenceError("gap taxonomy ownership overlaps")
            result[upstream_hash] = annotation["gap_kind"]
        if rule["selector"]["metadata_form"] == "0F28":
            zero_f28 = {
                "gap_kind": annotation["gap_kind"],
                "resolved_count": annotation["resolved_count"],
                "resolved_test_hashes_sha256": annotation[
                    "resolved_test_hashes_sha256"
                ],
                "selector": rule["selector"],
                "selector_sha256": selector_digest,
            }
    if zero_f28 is None:
        raise EvidenceError("0F28 known-gap selector is missing")
    return result, zero_f28


def make_row(
    item: str,
    form: str,
    record: dict[str, Any],
    resolved: dict[str, Any],
    gap_kinds: dict[str, str],
    actual: dict[str, Any],
) -> dict[str, Any]:
    expected_regs = ssts.expected_registers(record)
    watch, expected_ram = ssts.expected_memory(record)
    context = {
        "record": record,
        "record_digest": sha256_bytes(canonical_bytes(record)),
        "watch": watch,
        "expected_ram": expected_ram,
    }
    architectural = ssts.make_failure(
        DATASET_ID,
        "full",
        form,
        resolved["classification"],
        resolved["flags_mask"],
        context,
        "ok",
        actual,
    )["content"]
    fingerprint = ssts.make_failure(
        DATASET_ID,
        "full",
        form,
        resolved["classification"],
        0xFFFF,
        context,
        "ok",
        actual,
    )["content"]
    architectural_mismatches = architectural["mismatch_kinds"]
    fingerprint_mismatches = fingerprint["mismatch_kinds"]
    applicable = resolved["classification"] == "applicable"
    analysis, partitions, primary, logical, physical = item_analysis(
        item,
        form,
        record,
        expected_regs,
        expected_ram,
        actual,
        architectural_mismatches,
    )
    expected_exec = expected_execution(record, form)
    row = {
        "actual_final_state": state_object(
            actual["registers"],
            actual["ram"],
            actual["termination"],
            actual["execution_result"],
        ),
        "analysis": analysis,
        "architectural_mismatch_kinds": architectural_mismatches,
        "case_hash": context["record_digest"],
        "conclusion_status": "proven",
        "diagnostic_replayed": not applicable,
        "evidence_notes": (
            [
                "Diagnostic replay is outside the blocking denominator; "
                "it does not alter classification or establish a passing reference."
            ]
            if not applicable
            else []
        ),
        "executed": applicable,
        "expected_final_state": state_object(
            expected_regs,
            expected_ram,
            ssts.expected_termination(form, record),
            expected_exec,
        ),
        "fingerprint_mismatch_kinds": fingerprint_mismatches,
        "fingerprint_outcome": (
            "skip"
            if not applicable
            else ("pass" if not fingerprint_mismatches else "fail")
        ),
        "flags_comparison": {
            "actual_all16": f"{actual['registers']['flags']:04x}",
            "actual_masked": (
                f"{actual['registers']['flags'] & resolved['flags_mask']:04x}"
            ),
            "expected_all16": f"{expected_regs['flags']:04x}",
            "expected_masked": f"{expected_regs['flags'] & resolved['flags_mask']:04x}",
            "metadata_mask": f"{resolved['flags_mask']:04x}",
        },
        "form": form,
        "gap_kind": (
            gap_kinds[record["hash"]]
            if resolved["classification"] == "known_target_gap"
            else None
        ),
        "initial_architectural_state": {
            "ram": ram_entries(record["initial"]["ram"]),
            "registers": hex_registers(record["initial"]["regs"]),
        },
        "instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "item": item,
        "logical_addresses": logical,
        "official_outcome": (
            "skip"
            if not applicable
            else ("pass" if not architectural_mismatches else "fail")
        ),
        "physical_addresses": physical,
        "primary_partition": primary,
        "profile": "architectural",
        "scope": "full",
        "selected": True,
        "structural_partitions": partitions,
        "top_level_classification": resolved["classification"],
        "upstream_case_hash": record["hash"],
    }
    validate_row(row, f"{item}:{form}:{record['hash']}")
    return row


def validate_row(value: Any, where: str) -> dict[str, Any]:
    row = require_keys(value, ROW_KEYS, where)
    require_sha256(row["case_hash"], f"{where}.case_hash")
    require_sha1(row["upstream_case_hash"], f"{where}.upstream_case_hash")
    if row["item"] not in ITEM_FORMS:
        raise EvidenceError(f"{where}.item: unknown item")
    if not isinstance(row["form"], str) or FORM_RE.fullmatch(row["form"]) is None:
        raise EvidenceError(f"{where}.form: malformed form")
    if row["form"] not in ITEM_FORMS[row["item"]]:
        raise EvidenceError(f"{where}.form: form does not belong to item")
    if row["top_level_classification"] not in CLASSIFICATIONS:
        raise EvidenceError(f"{where}.classification: unknown classification")
    if row["gap_kind"] is not None and row["gap_kind"] not in GAP_KINDS:
        raise EvidenceError(f"{where}.gap_kind: unknown gap kind")
    if (row["top_level_classification"] == "known_target_gap") != (
        row["gap_kind"] is not None
    ):
        raise EvidenceError(f"{where}.gap_kind: inconsistent with classification")
    if row["conclusion_status"] not in CONCLUSION_STATUSES:
        raise EvidenceError(f"{where}.conclusion_status: unknown status")
    if row["selected"] is not True:
        raise EvidenceError(f"{where}.selected: evidence population must be selected")
    if not isinstance(row["executed"], bool) or not isinstance(
        row["diagnostic_replayed"], bool
    ):
        raise EvidenceError(f"{where}: execution flags must be booleans")
    if row["executed"] and row["top_level_classification"] != "applicable":
        raise EvidenceError(f"{where}: non-applicable row marked executed")
    if row["executed"] == row["diagnostic_replayed"]:
        raise EvidenceError(f"{where}: row must be profile-executed or diagnostic replay")
    if row["actual_final_state"] is None:
        raise EvidenceError(f"{where}: expected-only evidence row")
    if row["official_outcome"] not in {"pass", "fail", "skip"}:
        raise EvidenceError(f"{where}.official_outcome: invalid")
    if row["fingerprint_outcome"] not in {"pass", "fail", "skip"}:
        raise EvidenceError(f"{where}.fingerprint_outcome: invalid")
    if row["executed"] and row["official_outcome"] == "skip":
        raise EvidenceError(f"{where}: executed row cannot skip")
    if not row["executed"] and row["official_outcome"] != "skip":
        raise EvidenceError(f"{where}: diagnostic row must retain skip outcome")
    if not isinstance(row["primary_partition"], str) or not row["primary_partition"]:
        raise EvidenceError(f"{where}: missing exclusive primary partition")
    if not isinstance(row["structural_partitions"], dict):
        raise EvidenceError(f"{where}: malformed structural partitions")
    if row["structural_partitions"].get("primary") != row["primary_partition"]:
        raise EvidenceError(f"{where}: primary partition overlap or mismatch")
    if any(
        not isinstance(value, list)
        for value in (
            row["architectural_mismatch_kinds"],
            row["fingerprint_mismatch_kinds"],
            row["evidence_notes"],
            row["logical_addresses"],
            row["physical_addresses"],
        )
    ):
        raise EvidenceError(f"{where}: malformed array field")
    if (
        not isinstance(row["instruction_bytes"], str)
        or not row["instruction_bytes"]
        or re.fullmatch(r"(?:[0-9a-f]{2})+", row["instruction_bytes"]) is None
    ):
        raise EvidenceError(f"{where}.instruction_bytes: malformed")
    if row["profile"] != "architectural" or row["scope"] != "full":
        raise EvidenceError(f"{where}: profile or scope differs")
    return row


def validate_case_table(value: Any, where: str) -> list[dict[str, Any]]:
    table = require_keys(
        value,
        {"item", "row_count", "rows", "schema", "schema_version"},
        where,
    )
    if (
        table["schema"] != "vaeg-upd9002-semantics-evidence-cases-v1"
        or table["schema_version"] != SCHEMA_VERSION
    ):
        raise EvidenceError(f"{where}: unknown case-table schema version")
    if table["item"] not in ITEM_FORMS:
        raise EvidenceError(f"{where}.item: unknown")
    if not isinstance(table["rows"], list):
        raise EvidenceError(f"{where}.rows: expected array")
    if require_count(table["row_count"], f"{where}.row_count") != len(table["rows"]):
        raise EvidenceError(f"{where}: row-count mismatch")
    hashes = []
    for index, row in enumerate(table["rows"]):
        validate_row(row, f"{where}.rows[{index}]")
        if row["item"] != table["item"]:
            raise EvidenceError(f"{where}: row belongs to a different item")
        hashes.append(row["case_hash"])
    if hashes != sorted(hashes):
        raise EvidenceError(f"{where}: nondeterministic row order")
    if len(hashes) != len(set(hashes)):
        raise EvidenceError(f"{where}: duplicate case hash")
    return table["rows"]


SUMMARY_KEYS = {
    "case_hash_set_sha256",
    "conclusions",
    "item",
    "mismatch_classes",
    "populations",
    "primary_partitions",
    "representative_case_hashes",
    "row_count",
    "schema",
    "schema_version",
    "specialized_analysis",
    "termination_classes",
}


def validate_summary(
    value: Any, rows: list[dict[str, Any]], where: str
) -> dict[str, Any]:
    summary = require_keys(value, SUMMARY_KEYS, where)
    if (
        summary["schema"] != "vaeg-upd9002-semantics-evidence-summary-v1"
        or summary["schema_version"] != SCHEMA_VERSION
    ):
        raise EvidenceError(f"{where}: unknown summary schema version")
    if summary["item"] not in ITEM_FORMS:
        raise EvidenceError(f"{where}: unknown item")
    if summary["row_count"] != len(rows):
        raise EvidenceError(f"{where}: summary row count differs")
    if summary["case_hash_set_sha256"] != hash_set_digest(
        row["case_hash"] for row in rows
    ):
        raise EvidenceError(f"{where}: case hash-set digest differs")
    expected_partitions = dict(
        sorted(Counter(row["primary_partition"] for row in rows).items())
    )
    if summary["primary_partitions"] != expected_partitions:
        raise EvidenceError(f"{where}: incomplete or overlapping primary partitions")
    representatives = summary["representative_case_hashes"]
    if not isinstance(representatives, list) or not representatives:
        raise EvidenceError(f"{where}: missing representative evidence")
    if representatives != sorted(set(representatives)):
        raise EvidenceError(f"{where}: representative hashes are not unique and ordered")
    case_hashes = {row["case_hash"] for row in rows}
    if any(value not in case_hashes for value in representatives):
        raise EvidenceError(f"{where}: representative absent from machine table")
    if not isinstance(summary["conclusions"], list) or not summary["conclusions"]:
        raise EvidenceError(f"{where}: missing conclusions")
    for index, conclusion in enumerate(summary["conclusions"]):
        require_keys(
            conclusion,
            {"case_count", "case_hash_set_sha256", "statement", "status"},
            f"{where}.conclusions[{index}]",
        )
        if conclusion["status"] not in CONCLUSION_STATUSES:
            raise EvidenceError(f"{where}: invalid conclusion status")
        require_count(conclusion["case_count"], f"{where}.conclusions[{index}].case_count")
        require_sha256(
            conclusion["case_hash_set_sha256"],
            f"{where}.conclusions[{index}].case_hash_set_sha256",
        )
    return summary


def population_rows(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    fingerprint: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    for row in rows:
        key = (row["form"], row["top_level_classification"])
        groups[key]["selected"] += 1
        groups[key]["executed"] += int(row["executed"])
        groups[key][row["official_outcome"]] += 1
        fingerprint[key][row["fingerprint_outcome"]] += 1
    result = []
    for form, classification in sorted(groups):
        values = groups[(form, classification)]
        result.append(
            {
                "classification": classification,
                "executed": values["executed"],
                "fail": values["fail"],
                "fingerprint_fail": fingerprint[(form, classification)]["fail"],
                "fingerprint_pass": fingerprint[(form, classification)]["pass"],
                "fingerprint_skip": fingerprint[(form, classification)]["skip"],
                "form": form,
                "pass": values["pass"],
                "selected": values["selected"],
                "skip": values["skip"],
            }
        )
    return result


def bit_rule(samples: list[tuple[str, int, int]]) -> dict[str, Any]:
    if not samples:
        return {"representative_case_hash": None, "rule": "undetermined"}
    copied = all(source == output for _, source, output in samples)
    outputs = {output for _, _, output in samples}
    if copied:
        rule = "copied"
    elif outputs == {0}:
        rule = "forced-zero"
    elif outputs == {1}:
        rule = "forced-one"
    elif len({source for _, source, _ in samples}) < 2:
        rule = "undetermined"
    else:
        rule = "condition-dependent"
    return {"representative_case_hash": samples[0][0], "rule": rule}


def correlated_bit_rule(
    samples: list[tuple[str, int, int]], source_width: int
) -> dict[str, Any]:
    if not samples:
        return {
            "representative_case_hash": None,
            "rule": "undetermined",
            "source_bit": None,
        }
    outputs = {output for _, _, output in samples}
    if outputs == {0}:
        return {
            "representative_case_hash": samples[0][0],
            "rule": "forced-zero",
            "source_bit": None,
        }
    if outputs == {1}:
        return {
            "representative_case_hash": samples[0][0],
            "rule": "forced-one",
            "source_bit": None,
        }
    candidates = [
        bit
        for bit in range(source_width)
        if all(((source >> bit) & 1) == output for _, source, output in samples)
    ]
    if len(candidates) == 1:
        return {
            "representative_case_hash": samples[0][0],
            "rule": "copied",
            "source_bit": candidates[0],
        }
    return {
        "representative_case_hash": samples[0][0],
        "rule": "condition-dependent" if not candidates else "undetermined",
        "source_bit": None,
    }


def loaded_bit_rule(
    samples: list[tuple[str, int, int, int]], source_width: int, target_bit: int
) -> dict[str, Any]:
    if not samples:
        return {
            "representative_case_hash": None,
            "rule": "undetermined",
            "source_bit": None,
        }
    output_values = {output for _, _, _, output in samples}
    if output_values == {0}:
        return {
            "representative_case_hash": samples[0][0],
            "rule": "forced-zero",
            "source_bit": None,
        }
    if output_values == {1}:
        return {
            "representative_case_hash": samples[0][0],
            "rule": "forced-one",
            "source_bit": None,
        }
    preserved = all(
        ((initial >> target_bit) & 1) == output
        for _, initial, _, output in samples
    )
    source_candidates = [
        bit
        for bit in range(source_width)
        if all(((source >> bit) & 1) == output for _, _, source, output in samples)
    ]
    if preserved and source_candidates:
        rule = "undetermined"
        source_bit = None
    elif preserved:
        rule = "preserved"
        source_bit = None
    elif len(source_candidates) == 1:
        rule = "loadable"
        source_bit = source_candidates[0]
    else:
        rule = "undetermined"
        source_bit = None
    return {
        "representative_case_hash": samples[0][0],
        "rule": rule,
        "source_bit": source_bit,
    }


def flags_specialized(rows: list[dict[str, Any]]) -> dict[str, Any]:
    pushed: dict[str, list[tuple[str, int, int]]] = defaultdict(list)
    actual_pushed: dict[str, list[tuple[str, int, int]]] = defaultdict(list)
    pushf: dict[str, list[tuple[str, int, int]]] = defaultdict(list)
    actual_pushf: dict[str, list[tuple[str, int, int]]] = defaultdict(list)
    lahf: dict[str, list[tuple[str, int, int]]] = defaultdict(list)
    actual_lahf: dict[str, list[tuple[str, int, int]]] = defaultdict(list)
    popf: dict[str, list[tuple[str, int, int, int]]] = defaultdict(list)
    actual_popf: dict[str, list[tuple[str, int, int, int]]] = defaultdict(list)
    sahf: dict[str, list[tuple[str, int, int, int]]] = defaultdict(list)
    actual_sahf: dict[str, list[tuple[str, int, int, int]]] = defaultdict(list)
    boundary = Counter()
    pushf_boundary = Counter()
    for row in rows:
        if row["form"] in {"CC", "CD", "CE"}:
            boundary[row["primary_partition"]] += 1
            analysis = row["analysis"]
            if "expected_saved_flags" not in analysis or analysis["expected_saved_flags"] is None:
                continue
            initial = int(analysis["initial_flags"], 16)
            expected = int(analysis["expected_saved_flags"], 16)
            actual = (
                None
                if analysis["actual_saved_flags"] is None
                else int(analysis["actual_saved_flags"], 16)
            )
            for bit in range(16):
                pushed[str(bit)].append(
                    (row["case_hash"], (initial >> bit) & 1, (expected >> bit) & 1)
                )
                if actual is not None:
                    actual_pushed[str(bit)].append(
                        (row["case_hash"], (initial >> bit) & 1, (actual >> bit) & 1)
                    )
        elif row["form"] == "9C":
            analysis = row["analysis"]
            pushf_boundary[row["primary_partition"]] += 1
            initial = int(analysis["initial_flags"], 16)
            expected = (
                None
                if analysis["expected_pushed_flags"] is None
                else int(analysis["expected_pushed_flags"], 16)
            )
            actual = (
                None
                if analysis["actual_pushed_flags"] is None
                else int(analysis["actual_pushed_flags"], 16)
            )
            for bit in range(16):
                if expected is not None:
                    pushf[str(bit)].append(
                        (row["case_hash"], (initial >> bit) & 1, (expected >> bit) & 1)
                    )
                if actual is not None:
                    actual_pushf[str(bit)].append(
                        (row["case_hash"], (initial >> bit) & 1, (actual >> bit) & 1)
                    )
        elif row["form"] == "9F":
            analysis = row["analysis"]
            initial = int(analysis["initial_flags"], 16)
            expected = int(analysis["expected_ah"], 16)
            actual = int(analysis["actual_ah"], 16)
            for bit in range(8):
                lahf[str(bit)].append(
                    (row["case_hash"], initial, (expected >> bit) & 1)
                )
                actual_lahf[str(bit)].append(
                    (row["case_hash"], initial, (actual >> bit) & 1)
                )
        elif row["form"] == "9D":
            analysis = row["analysis"]
            initial = int(analysis["initial_flags"], 16)
            source = int(analysis["stack_input_flags"], 16)
            expected = int(analysis["expected_final_flags"], 16)
            actual = int(analysis["actual_final_flags"], 16)
            for bit in range(16):
                popf[str(bit)].append(
                    (row["case_hash"], initial, source, (expected >> bit) & 1)
                )
                actual_popf[str(bit)].append(
                    (row["case_hash"], initial, source, (actual >> bit) & 1)
                )
        elif row["form"] == "9E":
            analysis = row["analysis"]
            initial = int(analysis["initial_flags"], 16)
            source = int(analysis["initial_ah"], 16)
            expected = int(analysis["expected_final_flags"], 16)
            actual = int(analysis["actual_final_flags"], 16)
            for bit in range(16):
                sahf[str(bit)].append(
                    (row["case_hash"], initial, source, (expected >> bit) & 1)
                )
                actual_sahf[str(bit)].append(
                    (row["case_hash"], initial, source, (actual >> bit) & 1)
                )
    return {
        "actual_interrupt_pushed_flags_bit_rules": {
            bit: bit_rule(actual_pushed[bit]) for bit in map(str, range(16))
        },
        "actual_lahf_ah_bit_rules": {
            bit: correlated_bit_rule(actual_lahf[bit], 16)
            for bit in map(str, range(8))
        },
        "actual_popf_bit_rules": {
            bit: loaded_bit_rule(
                [
                    (case_hash, initial, source, output)
                    for case_hash, initial, source, output in actual_popf[bit]
                ],
                16,
                int(bit),
            )
            for bit in map(str, range(16))
        },
        "actual_pushf_bit_rules": {
            bit: bit_rule(actual_pushf[bit]) for bit in map(str, range(16))
        },
        "actual_sahf_bit_rules": {
            bit: loaded_bit_rule(
                [
                    (case_hash, initial, source, output)
                    for case_hash, initial, source, output in actual_sahf[bit]
                ],
                8,
                int(bit),
            )
            for bit in map(str, range(16))
        },
        "frame_boundary_counts": dict(sorted(boundary.items())),
        "interrupt_pushed_flags_bit_rules": {
            bit: bit_rule(pushed[bit]) for bit in map(str, range(16))
        },
        "lahf_ah_bit_rules": {
            bit: correlated_bit_rule(lahf[bit], 16) for bit in map(str, range(8))
        },
        "materialization_domains": [
            "guest-visible stack images",
            "guest-visible AH image",
            "final architectural FLAGS with metadata mask",
            "full 16-bit diagnostic FLAGS fingerprint",
        ],
        "popf_bit_rules": {
            bit: loaded_bit_rule(popf[bit], 16, int(bit))
            for bit in map(str, range(16))
        },
        "popf_sahf_interrupt_shared_primitive": {
            "statement": (
                "The observed outcome overlap does not prove that stack "
                "materialization and FLAGS load instructions share one implementation primitive."
            ),
            "status": "underdetermined",
        },
        "pushf_boundary_counts": dict(sorted(pushf_boundary.items())),
        "pushf_bit_rules": {
            bit: bit_rule(pushf[bit]) for bit in map(str, range(16))
        },
        "sahf_bit_rules": {
            bit: loaded_bit_rule(sahf[bit], 8, int(bit))
            for bit in map(str, range(16))
        },
    }


def d4_d5_specialized(rows: list[dict[str, Any]]) -> dict[str, Any]:
    groups: dict[tuple[str, int], Counter[str]] = defaultdict(Counter)
    representatives: dict[tuple[str, int], str] = {}
    for row in rows:
        immediate = row["analysis"]["immediate"]
        if immediate not in REQUIRED_IMMEDIATES:
            continue
        key = (row["form"], immediate)
        groups[key][row["official_outcome"]] += 1
        groups[key][f"fingerprint_{row['fingerprint_outcome']}"] += 1
        groups[key][
            f"expected_{row['expected_final_state']['execution']['kind']}"
        ] += 1
        groups[key][
            f"actual_{row['actual_final_state']['execution']['kind']}"
        ] += 1
        groups[key]["selected"] += 1
        representatives.setdefault(key, row["case_hash"])
    return {
        "required_immediate_strata": [
            {
                "actual_execution_classes": {
                    kind.removeprefix("actual_"): count
                    for kind, count in sorted(groups[(form, immediate)].items())
                    if kind.startswith("actual_")
                },
                "executed": groups[(form, immediate)]["selected"],
                "expected_execution_classes": {
                    kind.removeprefix("expected_"): count
                    for kind, count in sorted(groups[(form, immediate)].items())
                    if kind.startswith("expected_")
                },
                "fail": groups[(form, immediate)]["fail"],
                "fingerprint_fail": groups[(form, immediate)]["fingerprint_fail"],
                "fingerprint_pass": groups[(form, immediate)]["fingerprint_pass"],
                "form": form,
                "immediate": immediate,
                "pass": groups[(form, immediate)]["pass"],
                "representative_case_hash": representatives.get((form, immediate)),
                "selected": groups[(form, immediate)]["selected"],
            }
            for form in ("D4", "D5")
            for immediate in sorted(REQUIRED_IMMEDIATES)
        ]
    }


def canary_specialized(rows: list[dict[str, Any]]) -> dict[str, Any]:
    mode_results: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    structural_results: dict[tuple[Any, ...], Counter[str]] = defaultdict(Counter)
    wrap_results: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    c6_c7_noop: dict[str, list[str]] = defaultdict(list)
    f7_low_byte_only = []
    unexpected_flags = []
    for row in rows:
        partitions = row["structural_partitions"]
        mode = partitions["register_or_memory"]
        mode_results[(row["form"], mode)][row["official_outcome"]] += 1
        structural_key = (
            row["form"],
            mode,
            partitions["modrm_mode"],
            partitions["displacement_width"],
            partitions["immediate_width"],
            partitions["segment"],
            partitions["offset_wrap"],
            partitions["physical_wrap"],
            partitions["expected_execution_kind"],
            partitions["actual_execution_kind"],
            partitions["mismatch_class"],
        )
        structural_results[structural_key][row["official_outcome"]] += 1
        if partitions.get("offset_wrap"):
            wrap_results[(row["form"], "offset16")][row["official_outcome"]] += 1
        if partitions.get("physical_wrap"):
            wrap_results[(row["form"], "physical20")][row["official_outcome"]] += 1
        if (
            row["form"] in {"C6", "C7"}
            and row["official_outcome"] == "fail"
            and row["analysis"]["actual_destination"]
            == row["analysis"]["initial_destination"]
        ):
            c6_c7_noop[row["form"]].append(row["case_hash"])
        if row["form"] == "F7.2" and row["official_outcome"] == "fail":
            analysis = row["analysis"]
            initial = int(analysis["initial_destination"], 16)
            actual = int(analysis["actual_destination"], 16)
            physical_address = int(analysis["physical_address"], 16)
            if (
                actual & 0xFF == (~initial) & 0xFF
                and actual >> 8 == initial >> 8
                and physical_address < 0xA0000
                and physical_address & 1 == 0
            ):
                f7_low_byte_only.append(row["case_hash"])
        if (
            row["flags_comparison"]["expected_all16"]
            != row["flags_comparison"]["actual_all16"]
        ):
            unexpected_flags.append(row["case_hash"])
    return {
        "c6_c7_register_noop_failures": {
            form: {
                "count": len(values),
                "hash_set_sha256": hash_set_digest(values),
            }
            for form, values in sorted(c6_c7_noop.items())
        },
        "cause_assessments": [
            {
                "cause": "C6/C7 register-form execution",
                "statement": (
                    "Every C6/C7 failure is a ModR/M register form whose actual "
                    "destination remains equal to its initial value; every memory form passes."
                ),
                "status": "proven",
            },
            {
                "cause": "F7 /2 low-memory aligned word read-modify-write width",
                "statement": (
                    "Every F7 /2 failure is an even physical address below 0xa0000 "
                    "where the low byte is inverted and the high byte remains initial."
                ),
                "status": "proven",
            },
            {
                "cause": "effective-address calculation",
                "statement": (
                    "The failing F7 /2 low byte is updated at the expected address; "
                    "a general effective-address error is not sufficient to explain the cluster."
                ),
                "status": "proven",
            },
            {
                "cause": "displacement/immediate fetch ordering",
                "statement": (
                    "Final-state evidence alone cannot determine transient fetch order."
                ),
                "status": "underdetermined",
            },
            {
                "cause": "single shared primitive",
                "statement": (
                    "C6/C7 fail only in register forms while F7 /2 fails only in a "
                    "specific memory-word path; one shared implementation cause is not supported."
                ),
                "status": "underdetermined",
            },
        ],
        "f7_low_byte_only_failures": {
            "count": len(f7_low_byte_only),
            "hash_set_sha256": hash_set_digest(f7_low_byte_only),
        },
        "mode_results": [
            {
                "fail": values["fail"],
                "form": form,
                "mode": mode,
                "pass": values["pass"],
            }
            for (form, mode), values in sorted(mode_results.items())
        ],
        "structural_results": [
            {
                "actual_execution_kind": actual_execution,
                "displacement_width": displacement_width,
                "expected_execution_kind": expected_execution,
                "fail": values["fail"],
                "form": form,
                "immediate_width": immediate_width,
                "mismatch_class": mismatch_class,
                "modrm_mode": modrm_mode,
                "offset_wrap": offset_wrap,
                "pass": values["pass"],
                "physical_wrap": physical_wrap,
                "register_or_memory": mode,
                "segment": segment,
            }
            for (
                form,
                mode,
                modrm_mode,
                displacement_width,
                immediate_width,
                segment,
                offset_wrap,
                physical_wrap,
                expected_execution,
                actual_execution,
                mismatch_class,
            ), values in sorted(structural_results.items(), key=lambda item: str(item[0]))
        ],
        "unexpected_flags_changes": {
            "count": len(unexpected_flags),
            "hash_set_sha256": hash_set_digest(unexpected_flags),
        },
        "wrap_results": [
            {
                "fail": values["fail"],
                "form": form,
                "pass": values["pass"],
                "wrap_kind": kind,
            }
            for (form, kind), values in sorted(wrap_results.items())
        ],
    }


def zero_f_specialized(
    rows: list[dict[str, Any]],
    zero_f28: dict[str, Any],
    target_evidence: dict[str, Any],
) -> dict[str, Any]:
    replay_destination_unchanged = []
    reserved_length_consumption = []
    ror_expected_full_transfer = []
    ror_actual_nibble_merge = []
    for row in rows:
        analysis = row["analysis"]
        initial_destination = int(analysis["initial_destination"], 16)
        initial_ax = int(row["initial_architectural_state"]["registers"]["ax"], 16)
        expected_ax = int(row["expected_final_state"]["registers"]["ax"], 16)
        actual_ax = int(row["actual_final_state"]["registers"]["ax"], 16)
        if (
            row["form"] == "0F28"
            and analysis["actual_destination"] == analysis["initial_destination"]
            and actual_ax == initial_ax
        ):
            replay_destination_unchanged.append(row["case_hash"])
        if (
            row["form"] == "0F28"
            and analysis["actual_ip_delta"] == analysis["prefix_count"] + 2
        ):
            reserved_length_consumption.append(row["case_hash"])
        if row["form"] == "0F2A":
            if expected_ax & 0xFF == initial_destination:
                ror_expected_full_transfer.append(row["case_hash"])
            if actual_ax & 0xFF == (
                (initial_ax & 0xF0) | (initial_destination & 0x0F)
            ):
                ror_actual_nibble_merge.append(row["case_hash"])
    return {
        "g58_0f28_taxonomy": zero_f28,
        "primary_target_evidence": target_evidence,
        "sst_observed_0f28": {
            "statement": (
                "The SST expected state describes ROL4. M59 records all 5,000 "
                "expected states and a separate non-blocking diagnostic replay."
            ),
            "status": "proven",
        },
        "sst_observed_0f2a": {
            "expected_full_source_to_al_count": len(ror_expected_full_transfer),
            "expected_full_source_to_al_hash_set_sha256": hash_set_digest(
                ror_expected_full_transfer
            ),
            "actual_low_nibble_merge_count": len(ror_actual_nibble_merge),
            "actual_low_nibble_merge_hash_set_sha256": hash_set_digest(
                ror_actual_nibble_merge
            ),
            "status": "proven",
        },
        "target_diagnostic_0f28_destination_and_ax_unchanged": {
            "count": len(replay_destination_unchanged),
            "hash_set_sha256": hash_set_digest(replay_destination_unchanged),
            "status": "proven",
        },
        "target_diagnostic_0f28_reserved_length_consumption": {
            "count": len(reserved_length_consumption),
            "hash_set_sha256": hash_set_digest(reserved_length_consumption),
            "statement": (
                "Actual IP advances by the dispatch-neutral prefix count plus "
                "the two opcode bytes; ModR/M and displacement bytes are not consumed."
            ),
            "status": "proven",
        },
        "target_support_conclusion": {
            "status": "underdetermined",
            "statement": (
                "Available NEC V20/V30 primary documentation establishes the "
                "V-series instruction, but no inspected uPD9002/PC-88VA primary "
                "manual establishes target support or absence."
            ),
        },
    }


def shifts_specialized(rows: list[dict[str, Any]]) -> dict[str, Any]:
    groups: dict[tuple[str, int], Counter[str]] = defaultdict(Counter)
    structural: dict[tuple[Any, ...], Counter[str]] = defaultdict(Counter)
    expected_model = Counter()
    actual_model = Counter()
    flag_matches: dict[tuple[str, int, str], Counter[str]] = defaultdict(Counter)
    for row in rows:
        raw_count = row["analysis"]["raw_count"]
        if raw_count in REQUIRED_COUNTS:
            groups[(row["form"], raw_count)][row["official_outcome"]] += 1
            groups[(row["form"], raw_count)]["selected"] += 1
            partitions = row["structural_partitions"]
            key = (
                row["form"],
                partitions["operand_width"],
                partitions["register_or_memory"],
                partitions["subform"],
                partitions["count_source"],
                raw_count,
                partitions["initial_sign_bit"],
                partitions["initial_cf"],
                partitions["expected_execution_kind"],
                partitions["actual_execution_kind"],
            )
            structural[key][row["official_outcome"]] += 1
        if int(row["form"].split(".")[1]) >= 4:
            expected = row["analysis"]["count_observation"]
            actual = row["analysis"]["actual_count_observation"]
            expected_model[
                f"raw={expected['matches_raw']},mask1f={expected['matches_mask_1f']}"
            ] += 1
            actual_model[
                f"raw={actual['matches_raw']},mask1f={actual['matches_mask_1f']}"
            ] += 1
            if raw_count in REQUIRED_COUNTS:
                for flag in FLAG_BITS:
                    flag_matches[(row["form"], raw_count, flag)][
                        "match"
                        if row["analysis"]["expected_flags_bits"][flag]
                        == row["analysis"]["actual_flags_bits"][flag]
                        else "mismatch"
                    ] += 1
    return {
        "actual_count_model_observations": dict(sorted(actual_model.items())),
        "expected_count_model_observations": dict(sorted(expected_model.items())),
        "required_count_flag_observations": [
            {
                "count": count,
                "flag": flag,
                "form": form,
                "match": values["match"],
                "mismatch": values["mismatch"],
            }
            for (form, count, flag), values in sorted(flag_matches.items())
        ],
        "required_count_strata": [
            {
                "count": count,
                "fail": groups[(form, count)]["fail"],
                "form": form,
                "pass": groups[(form, count)]["pass"],
                "selected": groups[(form, count)]["selected"],
            }
            for form in ITEM_FORMS["shifts"]
            for count in sorted(REQUIRED_COUNTS)
            if groups[(form, count)]["selected"]
        ],
        "required_structural_strata": [
            {
                "actual_execution_kind": actual_execution,
                "count": count,
                "count_source": count_source,
                "expected_execution_kind": expected_execution,
                "fail": values["fail"],
                "form": form,
                "initial_cf": initial_cf,
                "initial_sign_bit": initial_sign,
                "operand_width": width,
                "pass": values["pass"],
                "register_or_memory": mode,
                "subform": subform,
            }
            for (
                form,
                width,
                mode,
                subform,
                count_source,
                count,
                initial_sign,
                initial_cf,
                expected_execution,
                actual_execution,
            ), values in sorted(structural.items(), key=lambda item: str(item[0]))
        ],
    }


def control_specialized(rows: list[dict[str, Any]]) -> dict[str, Any]:
    frame_only = sorted(
        row["case_hash"]
        for row in rows
        if row["form"] == "62"
        and row["analysis"]["failure_partition"] == "stack-frame-mismatch"
    )
    non_frame = sorted(
        row["case_hash"]
        for row in rows
        if row["form"] == "62"
        and row["official_outcome"] == "fail"
        and row["analysis"]["failure_partition"] != "stack-frame-mismatch"
    )
    ff_termination: dict[str, list[str]] = defaultdict(list)
    ff_pop_like = []
    ff_register_sp_zero = []
    bound_execution = Counter()
    bound_partitions: dict[str, list[str]] = defaultdict(list)
    bound_underdetermined = []
    for row in rows:
        if row["form"] == "FF.7":
            key = row["actual_final_state"]["execution"]["kind"]
            ff_termination[key].append(row["case_hash"])
            initial_sp = int(
                row["initial_architectural_state"]["registers"]["sp"], 16
            )
            actual_sp = int(row["actual_final_state"]["registers"]["sp"], 16)
            layout = bytes.fromhex(row["instruction_bytes"])
            position = 0
            while layout[position] in PREFIXES:
                position += 1
            rm = layout[position + 1] & 7
            mod = layout[position + 1] >> 6
            initial_registers = row["initial_architectural_state"]["registers"]
            ss = int(initial_registers["ss"], 16)
            stack_addresses = [
                ((ss << 4) + ((initial_sp + index) & 0xFFFF)) & 0xFFFFF
                for index in range(2)
            ]
            initial_ram = memory_from_entries(
                row["initial_architectural_state"]["ram"]
            )
            stack_word = word_at(initial_ram, stack_addresses, 0)
            if (
                (
                    not (mod == 3 and rm == 4)
                    and (actual_sp - initial_sp) & 0xFFFF == 2
                )
                or (
                    mod == 3
                    and rm == 4
                    and stack_word is not None
                    and actual_sp == stack_word
                )
            ):
                ff_pop_like.append(row["case_hash"])
            if mod == 3 and rm == 4 and actual_sp == 0:
                ff_register_sp_zero.append(row["case_hash"])
        else:
            expected = row["analysis"]["expected_execution"]
            actual = row["analysis"]["actual_execution"]
            bound_partitions[row["analysis"]["failure_partition"]].append(
                row["case_hash"]
            )
            frame = row["analysis"].get("interrupt_frame")
            if (
                frame is not None
                and frame["frame_mapping"] == "underdetermined"
                and row["analysis"]["failure_partition"]
                not in {"range-result-mismatch"}
            ):
                bound_underdetermined.append(row["case_hash"])
            bound_execution[
                (
                    expected["kind"],
                    expected["interrupt_vector"],
                    actual["kind"],
                    actual["interrupt_vector"],
                )
            ] += 1
    return {
        "bound_execution_matrix": [
            {
                "actual_kind": actual_kind,
                "actual_vector": actual_vector,
                "count": count,
                "expected_kind": expected_kind,
                "expected_vector": expected_vector,
            }
            for (
                expected_kind,
                expected_vector,
                actual_kind,
                actual_vector,
            ), count in sorted(bound_execution.items(), key=lambda item: str(item[0]))
        ],
        "bound_frame_only": {
            "count": len(frame_only),
            "hash_set_sha256": hash_set_digest(frame_only),
        },
        "bound_non_frame_only": {
            "count": len(non_frame),
            "hash_set_sha256": hash_set_digest(non_frame),
        },
        "bound_failure_partitions": {
            key: {
                "count": len(values),
                "hash_set_sha256": hash_set_digest(values),
            }
            for key, values in sorted(bound_partitions.items())
        },
        "bound_frame_corruption_underdetermined": {
            "count": len(bound_underdetermined),
            "hash_set_sha256": hash_set_digest(bound_underdetermined),
            "status": "underdetermined",
        },
        "ff7_termination_classes": {
            key: {
                "count": len(values),
                "hash_set_sha256": hash_set_digest(values),
            }
            for key, values in sorted(ff_termination.items())
        },
        "ff7_pop_like_final_stack_effect": {
            "count": len(ff_pop_like),
            "hash_set_sha256": hash_set_digest(ff_pop_like),
            "statement": (
                "Final SP advances by two for every form except ModR/M register "
                "destination SP; this is consistent with, but does not prove, a POP path."
            ),
            "status": "proven",
        },
        "ff7_register_sp_zero_result": {
            "count": len(ff_register_sp_zero),
            "hash_set_sha256": hash_set_digest(ff_register_sp_zero),
            "statement": (
                "The ModR/M register-SP forms finish with SP zero; final-state "
                "evidence does not prove transient read ordering."
            ),
            "status": "underdetermined",
        },
    }


def choose_representatives(item: str, rows: list[dict[str, Any]]) -> list[str]:
    groups: dict[tuple[Any, ...], str] = {}
    for row in rows:
        key: tuple[Any, ...] = (
            row["form"],
            row["official_outcome"],
            tuple(row["architectural_mismatch_kinds"]),
            row["primary_partition"],
        )
        groups.setdefault(key, row["case_hash"])
        if item == "d4_d5" and row["analysis"]["immediate"] in REQUIRED_IMMEDIATES:
            groups.setdefault(
                (row["form"], "immediate", row["analysis"]["immediate"]),
                row["case_hash"],
            )
        if item == "shifts" and row["analysis"]["raw_count"] in REQUIRED_COUNTS:
            groups.setdefault(
                (row["form"], "count", row["analysis"]["raw_count"]),
                row["case_hash"],
            )
    return sorted(set(groups.values()))


def build_summary(
    item: str,
    rows: list[dict[str, Any]],
    zero_f28: dict[str, Any],
    target_evidence: dict[str, Any],
) -> dict[str, Any]:
    mismatch = Counter(
        mismatch
        for row in rows
        for mismatch in row["architectural_mismatch_kinds"]
    )
    terminations = Counter(
        row["actual_final_state"]["execution"]["kind"] for row in rows
    )
    all_hashes = [row["case_hash"] for row in rows]
    failing = [
        row["case_hash"] for row in rows if row["official_outcome"] == "fail"
    ]
    if item == "flags":
        specialized = flags_specialized(rows)
        statement = (
            "The complete selected FLAGS-materialization population was replayed "
            "with expected and actual state recorded side by side."
        )
    elif item == "canary":
        specialized = canary_specialized(rows)
        statement = (
            "F7 /2, C6, and C7 were partitioned independently; correlation does "
            "not by itself prove a shared implementation primitive."
        )
    elif item == "d4_d5":
        specialized = d4_d5_specialized(rows)
        statement = (
            "D4 and D5 were both selected and executed; architectural and "
            "all-16-bit fingerprint outcomes are reported separately."
        )
    elif item == "0f28_0f2a":
        specialized = zero_f_specialized(rows, zero_f28, target_evidence)
        statement = (
            "0F28 remains a non-blocking known gap; diagnostic replay is not a "
            "passing reference and does not change its approved classification."
        )
    elif item == "shifts":
        specialized = shifts_specialized(rows)
        statement = (
            "Rotate and shift forms were selected and executed; raw and &0x1f "
            "count models are tested as competing observations, not assumptions."
        )
    else:
        specialized = control_specialized(rows)
        statement = (
            "FF /7 and BOUND were partitioned by observed execution and exact "
            "frame-only versus additional mismatch hash sets."
        )
    return {
        "case_hash_set_sha256": hash_set_digest(all_hashes),
        "conclusions": [
            {
                "case_count": len(all_hashes),
                "case_hash_set_sha256": hash_set_digest(all_hashes),
                "statement": statement,
                "status": "proven",
            },
            {
                "case_count": len(failing),
                "case_hash_set_sha256": hash_set_digest(failing),
                "statement": (
                    "Implementation-cause attribution remains a hypothesis or "
                    "underdetermined unless separately stated in specialized analysis."
                ),
                "status": "underdetermined",
            },
        ],
        "item": item,
        "mismatch_classes": dict(sorted(mismatch.items())),
        "populations": population_rows(rows),
        "primary_partitions": dict(
            sorted(Counter(row["primary_partition"] for row in rows).items())
        ),
        "representative_case_hashes": choose_representatives(item, rows),
        "row_count": len(rows),
        "schema": "vaeg-upd9002-semantics-evidence-summary-v1",
        "schema_version": SCHEMA_VERSION,
        "specialized_analysis": specialized,
        "termination_classes": dict(sorted(terminations.items())),
    }


def representative_markdown(
    item: str, rows: list[dict[str, Any]], hashes: list[str]
) -> bytes:
    by_hash = {row["case_hash"]: row for row in rows}
    lines = [
        "<!--",
        "Copyright (c) 2026 Nakata Maho",
        "",
        "SPDX-License-Identifier: BSD-2-Clause",
        "-->",
        f"# M59 representative evidence: {item}",
        "",
        "These records are selected deterministically from the complete machine table.",
        "Expected and actual final state are shown independently.",
        "",
    ]
    for case_hash in hashes:
        row = by_hash[case_hash]
        lines.extend(
            [
                f"## `{case_hash}`",
                "",
                f"- Form: `{row['form']}`",
                f"- Upstream hash: `{row['upstream_case_hash']}`",
                f"- Instruction: `{row['instruction_bytes']}`",
                f"- Classification: `{row['top_level_classification']}`",
                f"- Official outcome: `{row['official_outcome']}`",
                f"- Primary partition: `{row['primary_partition']}`",
                f"- Conclusion status: `{row['conclusion_status']}`",
                f"- Architectural mismatches: "
                f"`{','.join(row['architectural_mismatch_kinds']) or 'none'}`",
                f"- Logical addresses: "
                f"`{json.dumps(row['logical_addresses'], sort_keys=True)}`",
                f"- Physical addresses: "
                f"`{json.dumps(row['physical_addresses'], sort_keys=True)}`",
                f"- Item analysis: `{json.dumps(row['analysis'], sort_keys=True)}`",
                "- Initial registers: "
                f"`{json.dumps(row['initial_architectural_state']['registers'], sort_keys=True)}`",
                f"- Expected final: `{json.dumps(row['expected_final_state'], sort_keys=True)}`",
                f"- Actual final: `{json.dumps(row['actual_final_state'], sort_keys=True)}`",
                "",
            ]
        )
    return ("\n".join(lines).rstrip("\n") + "\n").encode("utf-8")


def scoreboard_population(
    root: pathlib.Path, profile: str
) -> dict[tuple[str, str], dict[str, Any]]:
    path = root / (
        "tests/ssts/scoreboard/g58_architectural_full.json"
        if profile == "architectural"
        else "tests/ssts/scoreboard/g58_fingerprint_full.json"
    )
    value = read_canonical_json(path)
    return {
        (record["form"], record["classification"]): record
        for record in value["records"]
    }


def validate_population_identity(
    architectural: dict[tuple[str, str], dict[str, Any]],
    fingerprint: dict[tuple[str, str], dict[str, Any]],
    observed: dict[tuple[str, str], Counter[str]],
    observed_fingerprint: dict[tuple[str, str], Counter[str]],
    forms: set[str],
) -> None:
    expected_keys = {key for key in architectural if key[0] in forms}
    if set(observed) != expected_keys or set(observed_fingerprint) != expected_keys:
        raise EvidenceError("analysis population classification partitions drifted")
    for key in sorted(observed):
        for source, expected in (
            (observed[key], architectural[key]),
            (observed_fingerprint[key], fingerprint[key]),
        ):
            actual_tuple = (
                source["selected"],
                source["executed"],
                source["pass"],
                source["fail"],
            )
            expected_tuple = (
                expected["selected"],
                expected["executed"],
                expected["pass"],
                expected["fail"],
            )
            if actual_tuple != expected_tuple:
                raise EvidenceError(
                    f"{key}: evidence population differs from G58: "
                    f"expected={expected_tuple} actual={actual_tuple}"
                )


def verify_population_against_g58(
    root: pathlib.Path, rows: list[dict[str, Any]]
) -> None:
    architectural = scoreboard_population(root, "architectural")
    fingerprint = scoreboard_population(root, "fingerprint")
    observed: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    observed_fingerprint: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    forms = {row["form"] for row in rows}
    for row in rows:
        key = (row["form"], row["top_level_classification"])
        observed[key]["selected"] += 1
        observed[key]["executed"] += int(row["executed"])
        observed[key][row["official_outcome"]] += 1
        observed_fingerprint[key]["selected"] += 1
        observed_fingerprint[key]["executed"] += int(row["executed"])
        observed_fingerprint[key][row["fingerprint_outcome"]] += 1
    validate_population_identity(
        architectural, fingerprint, observed, observed_fingerprint, forms
    )


def command_first_line(arguments: list[str]) -> str:
    try:
        completed = subprocess.run(
            arguments,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError) as error:
        return f"unavailable: {error}"
    return completed.stdout.splitlines()[0] if completed.stdout else "unknown"


def environment_identity() -> dict[str, Any]:
    return {
        "compiler": command_first_line(["cc", "--version"]),
        "gzip": command_first_line(["gzip", "--version"]),
        "host": platform.platform(),
        "python": platform.python_version(),
        "python_gzip_module": gzip.__file__,
        "zlib_compile": zlib.ZLIB_VERSION,
        "zlib_runtime": zlib.ZLIB_RUNTIME_VERSION,
    }


def validate_schema_documents(root: pathlib.Path) -> None:
    cases = read_canonical_json(
        root / "tests/ssts/schema/evidence-cases-v1.json"
    )
    manifest = read_canonical_json(
        root / "tests/ssts/schema/evidence-manifest-v1.json"
    )
    for name, schema in (("cases", cases), ("manifest", manifest)):
        if (
            schema.get("$schema")
            != "https://json-schema.org/draft/2020-12/schema"
            or schema.get("type") != "object"
            or schema.get("additionalProperties") is not False
            or schema.get("copyright") != "Copyright (c) 2026 Nakata Maho"
            or schema.get("license") != "BSD-2-Clause"
        ):
            raise EvidenceError(f"{name} schema: identity or root policy differs")
        if set(schema.get("required", [])) != set(schema.get("properties", {})):
            raise EvidenceError(f"{name} schema: required/property keys differ")
    if set(cases["properties"]) != {
        "item",
        "row_count",
        "rows",
        "schema",
        "schema_version",
    }:
        raise EvidenceError("case schema: table keys differ")
    row_schema = cases["properties"]["rows"].get("items", {})
    if (
        row_schema.get("type") != "object"
        or row_schema.get("additionalProperties") is not False
        or set(row_schema.get("required", [])) != ROW_KEYS
        or set(row_schema.get("properties", {})) != ROW_KEYS
    ):
        raise EvidenceError("case schema: row keys differ")
    if set(manifest["properties"]) != MANIFEST_KEYS:
        raise EvidenceError("manifest schema: keys differ")
    markdown = (root / "tests/ssts/schema/evidence-pack-v1.md").read_bytes()
    if not markdown.endswith(b"\n") or b"\r" in markdown:
        raise EvidenceError("evidence schema prose: noncanonical text")
    try:
        markdown.decode("utf-8")
    except UnicodeDecodeError as error:
        raise EvidenceError("evidence schema prose: invalid UTF-8") from error


def schema_references(root: pathlib.Path) -> dict[str, Any]:
    validate_schema_documents(root)
    paths = (
        "tests/ssts/schema/evidence-pack-v1.md",
        "tests/ssts/schema/evidence-cases-v1.json",
        "tests/ssts/schema/evidence-manifest-v1.json",
    )
    return {
        "files": [
            {
                "path": path,
                "sha256": sha256_file(root / path),
                "size": (root / path).stat().st_size,
            }
            for path in paths
        ],
        "schema": "vaeg-upd9002-semantics-evidence-schema-refs-v1",
        "schema_version": SCHEMA_VERSION,
    }


def target_evidence_identity(root: pathlib.Path) -> dict[str, Any]:
    manifest_path = pathlib.Path(
        "docs/agents/research/m47_upd9002_rep0f_documents.json"
    )
    report_path = pathlib.Path(
        "docs/agents/reports/m47_upd9002_rep0f_correctness.md"
    )
    require_exact(
        sha256_file(root / manifest_path),
        M47_DOCUMENT_MANIFEST_SHA256,
        "M47 NEC document manifest",
    )
    require_exact(
        sha256_file(root / report_path), M47_REPORT_SHA256, "accepted M47 report"
    )
    manifest = json.loads((root / manifest_path).read_text(encoding="utf-8"))
    documents = [
        {
            "availability": document["availability"],
            "describes": document["describes"],
            "sha256": document["sha256"],
            "title": document["title"],
        }
        for document in manifest["documents"]
    ]
    return {
        "documents": documents,
        "manifest_path": manifest_path.as_posix(),
        "manifest_sha256": M47_DOCUMENT_MANIFEST_SHA256,
        "report_path": report_path.as_posix(),
        "report_sha256": M47_REPORT_SHA256,
    }


def validate_schema_references(root: pathlib.Path, value: Any) -> None:
    require_keys(value, {"files", "schema", "schema_version"}, "schema references")
    require_exact(
        value["schema"],
        "vaeg-upd9002-semantics-evidence-schema-refs-v1",
        "schema references identifier",
    )
    require_exact(value["schema_version"], SCHEMA_VERSION, "schema references version")
    require_exact(value, schema_references(root), "schema references")


def generate(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    output: pathlib.Path,
    analysis_evaluated_sha: str,
    timeout: float,
) -> dict[str, Any]:
    require_commit(analysis_evaluated_sha, "analysis_evaluated_sha")
    if output.exists() and any(output.iterdir()):
        raise EvidenceError(f"output directory is not empty: {output}")
    output.mkdir(parents=True, exist_ok=True)
    protected = verify_protected_inputs(root)
    manifest_path = root / "tests/ssts/v20_dataset_manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest["dataset_id"] != DATASET_ID:
        raise EvidenceError("dataset manifest identity differs")
    ssts.verify_fast(dataset_root, manifest)
    metadata = json.loads(
        (dataset_root / ssts.SUITE_PATH / "metadata.json").read_text(
            encoding="utf-8"
        )
    )
    ssts.validate_metadata(metadata)
    support = ssts.load_support_map(
        root / "tools/qa/golden/upd9002_support_map_m48.csv"
    )
    gaps, zero_f28 = gap_kind_map(root)
    target_evidence = target_evidence_identity(root)
    all_rows: list[dict[str, Any]] = []
    item_rows: dict[str, list[dict[str, Any]]] = {}
    for item, forms in ITEM_FORMS.items():
        rows: list[dict[str, Any]] = []
        for form in forms:
            path = dataset_root / ssts.SUITE_PATH / f"{form}.json.gz"
            if not path.is_file():
                raise EvidenceError(f"missing corpus form {form}")
            with gzip.open(path, "rt", encoding="utf-8") as stream:
                records = ssts.profile_records(json.load(stream), "full")
            resolved_values = []
            for record in records:
                ssts.validate_record(record, f"{form}:{record.get('idx', '?')}")
                resolved_values.append(
                    ssts.classify_record(form, record, metadata, support)
                )
            replayed = ssts.run_worker_contained(worker, records, timeout)
            if len(replayed) != len(records):
                raise EvidenceError(f"{form}: replay count differs")
            for record, resolved, (status, actual) in zip(
                records, resolved_values, replayed
            ):
                if status != "ok" or actual is None:
                    raise EvidenceError(f"{form}:{record['hash']}: {status}")
                rows.append(
                    make_row(item, form, record, resolved, gaps, actual)
                )
            print(
                f"ssts-evidence: item={item} form={form} rows={len(records)}",
                file=sys.stderr,
                flush=True,
            )
        rows.sort(key=lambda row: row["case_hash"])
        if len(rows) != len({row["case_hash"] for row in rows}):
            raise EvidenceError(f"{item}: duplicate canonical case hash")
        verify_population_against_g58(root, rows)
        item_rows[item] = rows
        all_rows.extend(rows)

    artifacts: list[dict[str, Any]] = []
    schema_value = schema_references(root)
    schema_path = output / "schema/schema_refs.json"
    write_json(schema_path, schema_value)
    artifacts.append(artifact_entry(output, schema_path, "schema", 3))

    for item in ITEM_FORMS:
        rows = item_rows[item]
        table = {
            "item": item,
            "row_count": len(rows),
            "rows": rows,
            "schema": "vaeg-upd9002-semantics-evidence-cases-v1",
            "schema_version": SCHEMA_VERSION,
        }
        case_path = output / f"cases/{item}.json.gz"
        ratchet.write_deterministic_gzip(case_path, table)
        artifacts.append(artifact_entry(output, case_path, "cases", len(rows)))
        summary = build_summary(item, rows, zero_f28, target_evidence)
        validate_summary(summary, rows, f"generated:{item}")
        summary_path = output / f"summaries/{item}.json"
        write_json(summary_path, summary)
        artifacts.append(artifact_entry(output, summary_path, "summary", len(rows)))
        representative_path = output / f"representative/{item}.md"
        representative_path.parent.mkdir(parents=True, exist_ok=True)
        representative_path.write_bytes(
            representative_markdown(
                item, rows, summary["representative_case_hashes"]
            )
        )
        artifacts.append(
            artifact_entry(
                output,
                representative_path,
                "representative",
                len(summary["representative_case_hashes"]),
            )
        )
    artifacts.sort(key=lambda entry: entry["path"])
    architectural = read_canonical_json(
        root / "tests/ssts/scoreboard/g58_architectural_full.json"
    )
    fingerprint = read_canonical_json(
        root / "tests/ssts/scoreboard/g58_fingerprint_full.json"
    )
    value = {
        "analysis_evaluated_sha": analysis_evaluated_sha,
        "applicable_hash_set_sha256": FULL_APPLICABLE_HASH_SET_SHA256,
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifacts": artifacts,
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": [
            {
                "comparison_contract_id": ARCHITECTURAL_CONTRACT_ID,
                "comparison_contract_sha256": ARCHITECTURAL_CONTRACT_SHA256,
                "profile": "architectural",
            },
            {
                "comparison_contract_id": FINGERPRINT_CONTRACT_ID,
                "comparison_contract_sha256": FINGERPRINT_CONTRACT_SHA256,
                "profile": "fingerprint",
            },
        ],
        "dataset_id": DATASET_ID,
        "environment": environment_identity(),
        "g58_evaluated_sha": G58_EVALUATED_SHA,
        "generator": {
            "canonical_json": "sorted-keys-compact-utf8-lf",
            "deterministic_gzip": "raw-deflate-level9-mtime0-os255",
            "name": "tools/qa/upd9002_semantics_evidence.py",
            "version": GENERATOR_VERSION,
        },
        "milestone": MILESTONE,
        "profile_identity": {
            "architectural_failure_hash_set_sha256": architectural[
                "failure_hash_set_sha256"
            ],
            "architectural_failure_signature_index_sha256": architectural[
                "failure_signature_index_sha256"
            ],
            "fingerprint_failure_hash_set_sha256": fingerprint[
                "failure_hash_set_sha256"
            ],
            "fingerprint_failure_signature_index_sha256": fingerprint[
                "failure_signature_index_sha256"
            ],
        },
        "protected_inputs": protected,
        "schema": "vaeg-upd9002-semantics-evidence-manifest-v1",
        "schema_version": SCHEMA_VERSION,
        "selected_hash_set_sha256": FULL_SELECTED_HASH_SET_SHA256,
        "total_case_rows": len(all_rows),
    }
    write_json(output / "manifest.json", value)
    validate_pack(root, output)
    print(
        f"ssts-evidence: generated items={len(ITEM_FORMS)} "
        f"rows={len(all_rows)} manifest_sha256={sha256_file(output / 'manifest.json')}"
    )
    return value


def artifact_entry(
    output: pathlib.Path, path: pathlib.Path, kind: str, rows: int
) -> dict[str, Any]:
    return {
        "bytes": path.stat().st_size,
        "kind": kind,
        "path": path.relative_to(output).as_posix(),
        "rows": rows,
        "sha256": sha256_file(path),
    }


MANIFEST_KEYS = {
    "analysis_evaluated_sha",
    "applicable_hash_set_sha256",
    "approved_predecessor_gate",
    "approved_predecessor_sha",
    "artifacts",
    "candidate_gate",
    "comparison_contracts",
    "dataset_id",
    "environment",
    "g58_evaluated_sha",
    "generator",
    "milestone",
    "profile_identity",
    "protected_inputs",
    "schema",
    "schema_version",
    "selected_hash_set_sha256",
    "total_case_rows",
}


def expected_profile_identity(root: pathlib.Path) -> dict[str, str]:
    architectural = read_canonical_json(
        root / "tests/ssts/scoreboard/g58_architectural_full.json"
    )
    fingerprint = read_canonical_json(
        root / "tests/ssts/scoreboard/g58_fingerprint_full.json"
    )
    return {
        "architectural_failure_hash_set_sha256": architectural[
            "failure_hash_set_sha256"
        ],
        "architectural_failure_signature_index_sha256": architectural[
            "failure_signature_index_sha256"
        ],
        "fingerprint_failure_hash_set_sha256": fingerprint[
            "failure_hash_set_sha256"
        ],
        "fingerprint_failure_signature_index_sha256": fingerprint[
            "failure_signature_index_sha256"
        ],
    }


def validate_manifest_identity(
    value: Any,
    protected_inputs: dict[str, Any],
    profile_identity: dict[str, str],
) -> dict[str, Any]:
    manifest = require_keys(value, MANIFEST_KEYS, "manifest")
    if (
        manifest["schema"] != "vaeg-upd9002-semantics-evidence-manifest-v1"
        or manifest["schema_version"] != SCHEMA_VERSION
    ):
        raise EvidenceError("manifest: unknown schema version")
    expected_epoch = {
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "candidate_gate": CANDIDATE_GATE,
        "dataset_id": DATASET_ID,
        "g58_evaluated_sha": G58_EVALUATED_SHA,
        "milestone": MILESTONE,
    }
    for key, expected in expected_epoch.items():
        require_exact(manifest[key], expected, f"manifest.{key}")
    require_commit(manifest["analysis_evaluated_sha"], "analysis_evaluated_sha")
    require_exact(
        manifest["selected_hash_set_sha256"],
        FULL_SELECTED_HASH_SET_SHA256,
        "manifest.selected_hash_set_sha256",
    )
    require_exact(
        manifest["applicable_hash_set_sha256"],
        FULL_APPLICABLE_HASH_SET_SHA256,
        "manifest.applicable_hash_set_sha256",
    )
    require_exact(
        manifest["comparison_contracts"],
        [
            {
                "comparison_contract_id": ARCHITECTURAL_CONTRACT_ID,
                "comparison_contract_sha256": ARCHITECTURAL_CONTRACT_SHA256,
                "profile": "architectural",
            },
            {
                "comparison_contract_id": FINGERPRINT_CONTRACT_ID,
                "comparison_contract_sha256": FINGERPRINT_CONTRACT_SHA256,
                "profile": "fingerprint",
            },
        ],
        "manifest.comparison_contracts",
    )
    require_exact(
        manifest["generator"],
        {
            "canonical_json": "sorted-keys-compact-utf8-lf",
            "deterministic_gzip": "raw-deflate-level9-mtime0-os255",
            "name": "tools/qa/upd9002_semantics_evidence.py",
            "version": GENERATOR_VERSION,
        },
        "manifest.generator",
    )
    environment = require_keys(
        manifest["environment"],
        {
            "compiler",
            "gzip",
            "host",
            "python",
            "python_gzip_module",
            "zlib_compile",
            "zlib_runtime",
        },
        "manifest.environment",
    )
    if any(not isinstance(entry, str) or not entry for entry in environment.values()):
        raise EvidenceError("manifest.environment: expected nonempty strings")
    require_exact(
        manifest["protected_inputs"],
        protected_inputs,
        "manifest.protected_inputs",
    )
    require_exact(
        manifest["profile_identity"],
        profile_identity,
        "manifest.profile_identity",
    )
    require_count(manifest["total_case_rows"], "manifest.total_case_rows")
    if not isinstance(manifest["artifacts"], list):
        raise EvidenceError("manifest.artifacts: expected array")
    return manifest


def validate_manifest_total(manifest: dict[str, Any], total: int) -> None:
    if manifest["total_case_rows"] != total:
        raise EvidenceError("manifest: total row count differs")


def validate_pack(root: pathlib.Path, pack: pathlib.Path) -> dict[str, Any]:
    protected_inputs = verify_protected_inputs(root)
    manifest = validate_manifest_identity(
        read_canonical_json(pack / "manifest.json"),
        protected_inputs,
        expected_profile_identity(root),
    )
    artifacts = manifest["artifacts"]
    paths = [entry.get("path") for entry in artifacts if isinstance(entry, dict)]
    if paths != sorted(paths) or len(paths) != len(set(paths)):
        raise EvidenceError("manifest.artifacts: nondeterministic or duplicate paths")
    expected_paths = {"schema/schema_refs.json"}
    for item in ITEM_FORMS:
        expected_paths.update(
            {
                f"cases/{item}.json.gz",
                f"summaries/{item}.json",
                f"representative/{item}.md",
            }
        )
    if set(paths) != expected_paths:
        raise EvidenceError("manifest.artifacts: incomplete artifact family")
    rows_by_item: dict[str, list[dict[str, Any]]] = {}
    for index, raw_entry in enumerate(artifacts):
        entry = require_keys(
            raw_entry,
            {"bytes", "kind", "path", "rows", "sha256"},
            f"manifest.artifacts[{index}]",
        )
        relative = pathlib.PurePosixPath(entry["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise EvidenceError("manifest artifact path escapes pack")
        path = pack / relative
        if not path.is_file():
            raise EvidenceError(f"missing artifact {entry['path']}")
        require_file_identity(
            path, entry["bytes"], entry["sha256"], entry["path"]
        )
        if require_count(entry["rows"], f"{entry['path']}.rows") < 0:
            raise AssertionError("unreachable")
        if entry["kind"] == "cases":
            try:
                table = ratchet.read_deterministic_gzip(path)
            except ratchet.RatchetError as error:
                raise EvidenceError(str(error)) from error
            rows = validate_case_table(table, entry["path"])
            item = table["item"]
            require_row_count(entry["rows"], len(rows), entry["path"])
            rows_by_item[item] = rows
        elif entry["kind"] == "summary":
            read_canonical_json(path)
        elif entry["kind"] == "schema":
            validate_schema_references(root, read_canonical_json(path))
        elif entry["kind"] == "representative":
            raw = path.read_bytes()
            try:
                text = raw.decode("utf-8")
            except UnicodeDecodeError as error:
                raise EvidenceError(f"{path}: invalid representative UTF-8") from error
            if not text.endswith("\n") or text.endswith("\n\n") or "\r" in text:
                raise EvidenceError(f"{path}: noncanonical representative text")
        else:
            raise EvidenceError(f"{entry['path']}: unknown artifact kind")
    if set(rows_by_item) != set(ITEM_FORMS):
        raise EvidenceError("case-table item family is incomplete")
    total = 0
    for item, rows in rows_by_item.items():
        summary = validate_summary(
            read_canonical_json(pack / f"summaries/{item}.json"),
            rows,
            f"summaries/{item}.json",
        )
        representative = (
            pack / f"representative/{item}.md"
        ).read_text(encoding="utf-8")
        for case_hash in summary["representative_case_hashes"]:
            if f"`{case_hash}`" not in representative:
                raise EvidenceError(
                    f"representative/{item}.md: missing representative {case_hash}"
                )
        total += len(rows)
    validate_manifest_total(manifest, total)
    print(
        f"ssts-evidence-static: items={len(rows_by_item)} rows={total} "
        "protected G58/G43 inputs passed"
    )
    return manifest


def reject(label: str, callback: Any, tests: list[str]) -> None:
    try:
        callback()
    except (EvidenceError, ratchet.RatchetError):
        tests.append(label)
        return
    raise AssertionError(f"{label}: invalid input was accepted")


def synthetic_row() -> dict[str, Any]:
    registers = {name: "0000" for name in ssts.REGISTER_ORDER}
    execution = {"kind": "normal", "interrupt_vector": None}
    return {
        "actual_final_state": {
            "execution": execution,
            "ram": [],
            "registers": registers,
            "termination": "normal",
        },
        "analysis": {},
        "architectural_mismatch_kinds": [],
        "case_hash": "1" * 64,
        "conclusion_status": "proven",
        "diagnostic_replayed": False,
        "evidence_notes": [],
        "executed": True,
        "expected_final_state": {
            "execution": execution,
            "ram": [],
            "registers": registers,
            "termination": "normal",
        },
        "fingerprint_mismatch_kinds": [],
        "fingerprint_outcome": "pass",
        "flags_comparison": {
            "actual_all16": "0000",
            "actual_masked": "0000",
            "expected_all16": "0000",
            "expected_masked": "0000",
            "metadata_mask": "ffff",
        },
        "form": "D4",
        "gap_kind": None,
        "initial_architectural_state": {"ram": [], "registers": registers},
        "instruction_bytes": "d400",
        "item": "d4_d5",
        "logical_addresses": [],
        "official_outcome": "pass",
        "physical_addresses": [],
        "primary_partition": "immediate-0",
        "profile": "architectural",
        "scope": "full",
        "selected": True,
        "structural_partitions": {"primary": "immediate-0"},
        "top_level_classification": "applicable",
        "upstream_case_hash": "2" * 40,
    }


def synthetic_manifest() -> tuple[dict[str, Any], dict[str, Any], dict[str, str]]:
    protected = {
        "g43_manifest_sha256": G43_MANIFEST_SHA256,
        "g58_artifact_count": 34,
        "g58_artifact_tree_sha256": G58_TREE_SHA256,
    }
    profile_identity = {
        "architectural_failure_hash_set_sha256": "3" * 64,
        "architectural_failure_signature_index_sha256": "4" * 64,
        "fingerprint_failure_hash_set_sha256": "5" * 64,
        "fingerprint_failure_signature_index_sha256": "6" * 64,
    }
    manifest = {
        "analysis_evaluated_sha": "a" * 40,
        "applicable_hash_set_sha256": FULL_APPLICABLE_HASH_SET_SHA256,
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifacts": [],
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": [
            {
                "comparison_contract_id": ARCHITECTURAL_CONTRACT_ID,
                "comparison_contract_sha256": ARCHITECTURAL_CONTRACT_SHA256,
                "profile": "architectural",
            },
            {
                "comparison_contract_id": FINGERPRINT_CONTRACT_ID,
                "comparison_contract_sha256": FINGERPRINT_CONTRACT_SHA256,
                "profile": "fingerprint",
            },
        ],
        "dataset_id": DATASET_ID,
        "environment": {
            "compiler": "synthetic",
            "gzip": "synthetic",
            "host": "synthetic",
            "python": "synthetic",
            "python_gzip_module": "synthetic",
            "zlib_compile": "synthetic",
            "zlib_runtime": "synthetic",
        },
        "g58_evaluated_sha": G58_EVALUATED_SHA,
        "generator": {
            "canonical_json": "sorted-keys-compact-utf8-lf",
            "deterministic_gzip": "raw-deflate-level9-mtime0-os255",
            "name": "tools/qa/upd9002_semantics_evidence.py",
            "version": GENERATOR_VERSION,
        },
        "milestone": MILESTONE,
        "profile_identity": profile_identity,
        "protected_inputs": protected,
        "schema": "vaeg-upd9002-semantics-evidence-manifest-v1",
        "schema_version": SCHEMA_VERSION,
        "selected_hash_set_sha256": FULL_SELECTED_HASH_SET_SHA256,
        "total_case_rows": 1,
    }
    return manifest, protected, profile_identity


def selftest() -> None:
    tests: list[str] = []
    row = synthetic_row()
    validate_row(row, "synthetic")
    tests.append("valid evidence row")
    table = {
        "item": "d4_d5",
        "row_count": 1,
        "rows": [row],
        "schema": "vaeg-upd9002-semantics-evidence-cases-v1",
        "schema_version": 1,
    }
    validate_case_table(table, "synthetic-table")
    tests.append("valid case table")

    def mutated_row(field: str, value: Any) -> dict[str, Any]:
        result = copy.deepcopy(row)
        if value is None:
            del result[field]
        else:
            result[field] = value
        return result

    reject(
        "missing case hash",
        lambda: validate_row(mutated_row("case_hash", None), "negative"),
        tests,
    )
    reject(
        "malformed case hash",
        lambda: validate_row(mutated_row("case_hash", "bad"), "negative"),
        tests,
    )
    reject(
        "invalid conclusion status",
        lambda: validate_row(mutated_row("conclusion_status", "probable"), "negative"),
        tests,
    )
    reject(
        "invalid classification",
        lambda: validate_row(
            mutated_row("top_level_classification", "ignored"), "negative"
        ),
        tests,
    )
    reject(
        "selected executed inconsistency",
        lambda: validate_row(mutated_row("selected", False), "negative"),
        tests,
    )
    reject(
        "expected-only evidence",
        lambda: validate_row(mutated_row("actual_final_state", None), "negative"),
        tests,
    )
    overlapping = copy.deepcopy(row)
    overlapping["structural_partitions"]["primary"] = "different-partition"
    reject(
        "exclusive partition overlap",
        lambda: validate_row(overlapping, "negative"),
        tests,
    )
    wrong_version = copy.deepcopy(table)
    wrong_version["schema_version"] = 2
    reject(
        "unknown schema version",
        lambda: validate_case_table(wrong_version, "negative"),
        tests,
    )
    duplicate = copy.deepcopy(table)
    duplicate["rows"].append(copy.deepcopy(row))
    duplicate["row_count"] = 2
    reject(
        "duplicate case hash",
        lambda: validate_case_table(duplicate, "negative"),
        tests,
    )
    wrong_count = copy.deepcopy(table)
    wrong_count["row_count"] = 2
    reject(
        "row count mismatch",
        lambda: validate_case_table(wrong_count, "negative"),
        tests,
    )
    unordered = copy.deepcopy(table)
    second = copy.deepcopy(row)
    second["case_hash"] = "0" * 64
    unordered["rows"].append(second)
    unordered["row_count"] = 2
    reject(
        "nondeterministic row order",
        lambda: validate_case_table(unordered, "negative"),
        tests,
    )
    with tempfile.TemporaryDirectory(prefix="vaeg-m59-selftest-") as temporary:
        root = pathlib.Path(temporary)
        canonical = root / "canonical.json"
        write_json(canonical, table)
        read_canonical_json(canonical)
        tests.append("canonical JSON")
        noncanonical = root / "noncanonical.json"
        noncanonical.write_text(json.dumps(table, indent=2) + "\n", encoding="utf-8")
        reject(
            "nondeterministic JSON serialization",
            lambda: read_canonical_json(noncanonical),
            tests,
        )
        first = root / "first.json.gz"
        second_path = root / "second.json.gz"
        ratchet.write_deterministic_gzip(first, table)
        ratchet.write_deterministic_gzip(second_path, table)
        if first.read_bytes() != second_path.read_bytes():
            raise AssertionError("deterministic compression differs")
        tests.append("deterministic compression")
        nondeterministic = root / "bad.json.gz"
        nondeterministic.write_bytes(
            gzip.compress(canonical_bytes(table) + b"\n", mtime=1)
        )
        reject(
            "nondeterministic compression rejected",
            lambda: ratchet.read_deterministic_gzip(nondeterministic),
            tests,
        )
        protected_root = root / "protected"
        protected_root.mkdir()
        protected_file = protected_root / "file"
        protected_file.write_bytes(b"approved")
        approved_digest = tree_digest(
            protected_root, [protected_file]
        )
        protected_file.write_bytes(b"modified")
        reject(
            "modified approved G58 evidence",
            lambda: require_exact(
                tree_digest(protected_root, [protected_file]),
                approved_digest,
                "approved G58 artifacts",
            ),
            tests,
        )
        immutable = root / "immutable"
        immutable.write_bytes(b"m43")
        expected = sha256_file(immutable)
        immutable.write_bytes(b"M43")
        reject(
            "modified immutable M43 evidence",
            lambda: require_exact(
                sha256_file(immutable),
                expected,
                "immutable G43 manifest",
            ),
            tests,
        )
        reject(
            "manifest artifact digest mismatch",
            lambda: require_file_identity(
                canonical,
                canonical.stat().st_size,
                "0" * 64,
                "synthetic artifact",
            ),
            tests,
        )
        reject(
            "manifest artifact row-count mismatch",
            lambda: require_row_count(2, 1, "synthetic artifact"),
            tests,
        )
    summary = {
        "case_hash_set_sha256": hash_set_digest([row["case_hash"]]),
        "conclusions": [
            {
                "case_count": 1,
                "case_hash_set_sha256": hash_set_digest([row["case_hash"]]),
                "statement": "synthetic",
                "status": "proven",
            }
        ],
        "item": "d4_d5",
        "mismatch_classes": {},
        "populations": [],
        "primary_partitions": {"immediate-0": 1},
        "representative_case_hashes": [row["case_hash"]],
        "row_count": 1,
        "schema": "vaeg-upd9002-semantics-evidence-summary-v1",
        "schema_version": 1,
        "specialized_analysis": {},
        "termination_classes": {"normal": 1},
    }
    validate_summary(summary, [row], "synthetic-summary")
    tests.append("valid summary")
    missing_rep = copy.deepcopy(summary)
    missing_rep["representative_case_hashes"] = []
    reject(
        "missing representative",
        lambda: validate_summary(missing_rep, [row], "negative"),
        tests,
    )
    absent_rep = copy.deepcopy(summary)
    absent_rep["representative_case_hashes"] = ["3" * 64]
    reject(
        "representative absent from machine table",
        lambda: validate_summary(absent_rep, [row], "negative"),
        tests,
    )
    incomplete = copy.deepcopy(summary)
    incomplete["primary_partitions"] = {}
    reject(
        "incomplete partition coverage",
        lambda: validate_summary(incomplete, [row], "negative"),
        tests,
    )
    invalid_conclusion = copy.deepcopy(summary)
    invalid_conclusion["conclusions"][0]["status"] = "probable"
    reject(
        "invalid summary conclusion",
        lambda: validate_summary(invalid_conclusion, [row], "negative"),
        tests,
    )
    manifest, protected, profile_identity = synthetic_manifest()
    validate_manifest_identity(manifest, protected, profile_identity)
    tests.append("valid manifest schema and identity")
    selected_drift = copy.deepcopy(manifest)
    selected_drift["selected_hash_set_sha256"] = "0" * 64
    reject(
        "selected set drift",
        lambda: validate_manifest_identity(
            selected_drift, protected, profile_identity
        ),
        tests,
    )
    applicable_drift = copy.deepcopy(manifest)
    applicable_drift["applicable_hash_set_sha256"] = "0" * 64
    reject(
        "applicable set drift",
        lambda: validate_manifest_identity(
            applicable_drift, protected, profile_identity
        ),
        tests,
    )
    contract_drift = copy.deepcopy(manifest)
    contract_drift["comparison_contracts"][0][
        "comparison_contract_sha256"
    ] = "0" * 64
    reject(
        "comparison contract drift",
        lambda: validate_manifest_identity(
            contract_drift, protected, profile_identity
        ),
        tests,
    )
    reject(
        "manifest count mismatch",
        lambda: validate_manifest_total(manifest, 2),
        tests,
    )
    expected_record = {
        "selected": 1,
        "executed": 1,
        "pass": 1,
        "fail": 0,
    }
    expected_population = {("D4", "applicable"): expected_record}
    observed = {("D4", "applicable"): Counter(expected_record)}
    validate_population_identity(
        expected_population,
        expected_population,
        observed,
        copy.deepcopy(observed),
        {"D4"},
    )
    tests.append("valid G58 population identity")
    classification_drift = {
        ("D4", "known_target_gap"): Counter(expected_record)
    }
    reject(
        "classification set drift",
        lambda: validate_population_identity(
            expected_population,
            expected_population,
            classification_drift,
            copy.deepcopy(classification_drift),
            {"D4"},
        ),
        tests,
    )
    print(f"ssts-evidence-selftest: {len(tests)}/{len(tests)} passed")
    for label in tests:
        print(f"  passed: {label}")


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest", help="run positive and fail-closed tests")
    static = subparsers.add_parser(
        "verify-static", help="validate the committed G59 evidence pack"
    )
    static.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    generate_parser = subparsers.add_parser(
        "generate", help="replay and emit the complete deterministic evidence pack"
    )
    generate_parser.add_argument(
        "--root", type=pathlib.Path, default=pathlib.Path(".")
    )
    generate_parser.add_argument("--dataset-root", type=pathlib.Path, required=True)
    generate_parser.add_argument("--worker", type=pathlib.Path, required=True)
    generate_parser.add_argument("--output", type=pathlib.Path, required=True)
    generate_parser.add_argument("--analysis-evaluated-sha", required=True)
    generate_parser.add_argument("--shard-timeout", type=float, default=300.0)
    validate = subparsers.add_parser(
        "validate", help="validate an explicitly supplied evidence pack"
    )
    validate.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    validate.add_argument("--pack", type=pathlib.Path, required=True)
    return parser.parse_args(list(argv))


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
        elif arguments.command == "verify-static":
            root = arguments.root.resolve()
            validate_pack(root, root / "tests/ssts/evidence/g59")
        elif arguments.command == "validate":
            validate_pack(arguments.root.resolve(), arguments.pack.resolve())
        elif arguments.command == "generate":
            generate(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.output.resolve(),
                arguments.analysis_evaluated_sha,
                arguments.shard_timeout,
            )
        else:
            raise AssertionError(f"unknown command {arguments.command}")
    except (EvidenceError, OSError, ratchet.RatchetError, ssts.CorpusError) as error:
        print(f"ssts-evidence-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

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
"""Verify and identify the pinned SingleStepTests V20 native corpus."""

from __future__ import annotations

import argparse
import csv
import gzip
import hashlib
import json
import pathlib
import re
import struct
import subprocess
import sys
import tempfile
from collections import Counter
from typing import Any, Iterable


UPSTREAM_URL = "https://github.com/SingleStepTests/v20"
UPSTREAM_COMMIT = "9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21"
SUITE_PATH = "v1_native"
IDENTITY_POLICY = "vaeg-upd9002-ssts-v20-v1"
EXPECTED_METADATA_SHA256 = (
    "71c12e705960941a73981891852674649c3332539579634ea34d1dae40c1795a"
)
EXPECTED_LICENSE_SHA256 = (
    "cb3882ef501e91281c1948f71ed62bfca6562345a98f5fc1b69824f62b6e1557"
)
EXPECTED_README_VERSION = "1.0.3"
EXPECTED_METADATA_VERSION = "1.0.2"

ROOT_KEYS = {
    "author",
    "cpu",
    "cpu_detail",
    "date",
    "generator",
    "github",
    "opcodes",
    "syntax_version",
    "version",
}
OPCODE_KEYS = {"arch", "flags", "flags-mask", "reg", "status"}
REGISTER_KEYS = {"arch", "flags", "flags-mask", "percentage", "status"}
PRIMARY_STATUSES = {"normal", "prefix", "fpu", "extension", "undocumented"}
REGISTER_STATUSES = {"normal", "undefined", "alias", "undocumented"}
RECORD_KEYS = {"name", "bytes", "initial", "final", "cycles", "hash", "idx"}
STATE_KEYS = {"regs", "ram", "queue"}
REGISTER_KEYS_IN_RECORD = {
    "ax",
    "bx",
    "cx",
    "dx",
    "sp",
    "bp",
    "si",
    "di",
    "cs",
    "ss",
    "ds",
    "es",
    "ip",
    "flags",
}
REGISTER_ORDER = (
    "ax", "bx", "cx", "dx", "sp", "bp", "si", "di",
    "cs", "ss", "ds", "es", "ip", "flags",
)
SEGMENT_PREFIXES = {0x26, 0x2E, 0x36, 0x3E}
SEGMENT_PREFIX_REGISTERS = {
    0x26: "es",
    0x2E: "cs",
    0x36: "ss",
    0x3E: "ds",
}
REPEAT_PREFIXES = {
    0x64: "repnc",
    0x65: "repc",
    0xF2: "repne",
    0xF3: "repe",
}
IGNORED_PREFIXES = {0xF0, 0xF1}
M70_REPNC_STRING_OPCODES = {
    0xa4, 0xa5, 0xa6, 0xa7, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf
}
SUPPORT_TABLE_FAMILIES = {
    "upd9002": {
        "root": "upd9002op",
        "repc": "upd9002op_repc",
        "repe": "upd9002op_repe",
        "repne": "upd9002op_repne",
        "repnc": "upd9002op_repnc",
        "0f": "upd9002op_0f",
        "f6": "c_ope0xf6_table",
        "f7": "c_ope0xf7_table",
    },
    "v30": {
        "root": "v30op",
        "repc": "v30op_repc",
        "repe": "v30op_repe",
        "repne": "v30op_repne",
        "repnc": "v30op_repnc",
        "0f": "v30op_0f",
        "f6": "v30ope0xf6_table",
        "f7": "v30ope0xf7_table",
    },
}
GROUP_FORMS = {
    "80",
    "81",
    "82",
    "83",
    "8F",
    "C0",
    "C1",
    "C6",
    "C7",
    "D0",
    "D1",
    "D2",
    "D3",
    "F6",
    "F7",
    "FE",
    "FF",
}
HEX40 = re.compile(r"^[0-9a-f]{40}$")


class CorpusError(RuntimeError):
    """An identity, schema, or corpus verification failure."""


def canonical_bytes(value: Any) -> bytes:
    """Return the M43 canonical JSON representation."""

    return json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    ).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while True:
            block = stream.read(1024 * 1024)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest()


def require_keys(value: Any, expected: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise CorpusError(f"{where}: expected object, got {type(value).__name__}")
    actual = set(value)
    if actual != expected:
        raise CorpusError(
            f"{where}: schema keys differ: expected={sorted(expected)!r} "
            f"actual={sorted(actual)!r}"
        )
    return value


def validate_metadata(metadata: Any) -> dict[str, Any]:
    root = require_keys(metadata, ROOT_KEYS, "metadata")
    if not isinstance(root["opcodes"], dict):
        raise CorpusError("metadata.opcodes: expected object")

    primary_status = Counter()
    primary_arch = Counter()
    register_status = Counter()
    register_arch = Counter()
    group_count = 0
    register_count = 0
    for opcode, raw_entry in sorted(root["opcodes"].items()):
        if not re.fullmatch(r"[0-9A-F]{2}(?:[0-9A-F]{2})?", opcode):
            raise CorpusError(f"metadata.opcodes.{opcode}: invalid opcode key")
        entry = require_keys(raw_entry, set(raw_entry) & OPCODE_KEYS,
                             f"metadata.opcodes.{opcode}")
        unknown = set(entry) - OPCODE_KEYS
        if unknown:
            raise CorpusError(
                f"metadata.opcodes.{opcode}: unknown keys {sorted(unknown)!r}"
            )
        status = entry.get("status")
        if status is None:
            primary_status["missing"] += 1
        elif status not in PRIMARY_STATUSES:
            raise CorpusError(
                f"metadata.opcodes.{opcode}: unknown status {status!r}"
            )
        else:
            primary_status[status] += 1
        primary_arch[str(entry.get("arch", "missing"))] += 1
        if "flags-mask" in entry and not isinstance(entry["flags-mask"], int):
            raise CorpusError(f"metadata.opcodes.{opcode}.flags-mask: expected int")
        if "flags" in entry and (
            not isinstance(entry["flags"], str)
            or not re.fullmatch(r"[.oszapc]{8}", entry["flags"])
        ):
            raise CorpusError(
                f"metadata.opcodes.{opcode}.flags: expected eight-character mask"
            )
        if "reg" not in entry:
            continue
        if status is not None:
            raise CorpusError(
                f"metadata.opcodes.{opcode}: group parent must have missing status"
            )
        registers = entry["reg"]
        if not isinstance(registers, dict):
            raise CorpusError(f"metadata.opcodes.{opcode}.reg: expected object")
        group_count += 1
        for register, raw_register in sorted(registers.items()):
            if register not in {str(index) for index in range(8)}:
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg: invalid key {register!r}"
                )
            reg = require_keys(
                raw_register,
                set(raw_register) & REGISTER_KEYS,
                f"metadata.opcodes.{opcode}.reg.{register}",
            )
            unknown = set(reg) - REGISTER_KEYS
            if unknown:
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}: unknown keys "
                    f"{sorted(unknown)!r}"
                )
            reg_status = reg.get("status", "normal")
            if reg_status not in REGISTER_STATUSES:
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}: unknown status "
                    f"{reg_status!r}"
                )
            register_status[reg_status] += 1
            register_arch[str(reg.get("arch", "missing"))] += 1
            register_count += 1
            if "flags-mask" in reg and not isinstance(reg["flags-mask"], int):
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}.flags-mask: "
                    "expected int"
                )
            if "flags" in reg and (
                not isinstance(reg["flags"], str)
                or not re.fullmatch(r"[.oszapc]{8}", reg["flags"])
            ):
                raise CorpusError(
                    f"metadata.opcodes.{opcode}.reg.{register}.flags: "
                    "expected eight-character mask"
                )

    observed = {
        "opcode_count": len(root["opcodes"]),
        "primary_status_counts": dict(sorted(primary_status.items())),
        "primary_arch_counts": dict(sorted(primary_arch.items())),
        "group_parent_count": group_count,
        "register_entry_count": register_count,
        "register_status_counts": dict(sorted(register_status.items())),
        "register_arch_counts": dict(sorted(register_arch.items())),
    }
    expected = {
        "opcode_count": 282,
        "primary_status_counts": {
            "extension": 1,
            "fpu": 10,
            "missing": 17,
            "normal": 243,
            "prefix": 10,
            "undocumented": 1,
        },
        "primary_arch_counts": {"186": 10, "86": 221, "missing": 19, "v30": 32},
        "group_parent_count": 17,
        "register_entry_count": 136,
        "register_status_counts": {
            "alias": 11,
            "normal": 92,
            "undefined": 27,
            "undocumented": 6,
        },
        "register_arch_counts": {"186": 19, "86": 116, "missing": 1},
    }
    if observed != expected:
        raise CorpusError(
            "metadata vocabulary/counts differ from the pinned schema: "
            f"expected={expected!r} actual={observed!r}"
        )
    return observed


def validate_record(record: Any, where: str) -> dict[str, Any]:
    value = require_keys(record, RECORD_KEYS, where)
    if not isinstance(value["bytes"], list) or not value["bytes"]:
        raise CorpusError(f"{where}.bytes: expected nonempty array")
    if any(not isinstance(byte, int) or byte < 0 or byte > 0xff
           for byte in value["bytes"]):
        raise CorpusError(f"{where}.bytes: byte outside 0..255")
    if not isinstance(value["idx"], int) or value["idx"] < 0:
        raise CorpusError(f"{where}.idx: expected nonnegative int")
    if not isinstance(value["hash"], str) or not HEX40.fullmatch(value["hash"]):
        raise CorpusError(f"{where}.hash: expected lowercase 40-hex string")
    if not isinstance(value["name"], str):
        raise CorpusError(f"{where}.name: expected string")
    if not isinstance(value["cycles"], list):
        raise CorpusError(f"{where}.cycles: expected array")
    for state_name in ("initial", "final"):
        state = require_keys(value[state_name], STATE_KEYS, f"{where}.{state_name}")
        regs = state["regs"]
        if not isinstance(regs, dict):
            raise CorpusError(f"{where}.{state_name}.regs: expected object")
        if state_name == "initial" and set(regs) != REGISTER_KEYS_IN_RECORD:
            raise CorpusError(
                f"{where}.{state_name}.regs: expected all architectural registers"
            )
        if state_name == "final" and not set(regs).issubset(
            REGISTER_KEYS_IN_RECORD
        ):
            raise CorpusError(
                f"{where}.{state_name}.regs: unknown register"
            )
        if any(not isinstance(reg, int) or reg < 0 or reg > 0xffff
               for reg in regs.values()):
            raise CorpusError(f"{where}.{state_name}.regs: invalid 16-bit value")
        if not isinstance(state["ram"], list):
            raise CorpusError(f"{where}.{state_name}.ram: expected array")
        for item in state["ram"]:
            if (not isinstance(item, list) or len(item) != 2 or
                    not isinstance(item[0], int) or not 0 <= item[0] <= 0xfffff or
                    not isinstance(item[1], int) or not 0 <= item[1] <= 0xff):
                raise CorpusError(f"{where}.{state_name}.ram: invalid address/value")
        if not isinstance(state["queue"], list) or any(
            not isinstance(byte, int) or not 0 <= byte <= 0xff
            for byte in state["queue"]
        ):
            raise CorpusError(f"{where}.{state_name}.queue: invalid queue")
    return value


def corpus_files(dataset_root: pathlib.Path) -> list[pathlib.Path]:
    suite = dataset_root / SUITE_PATH
    paths = sorted(suite.glob("*.json.gz"), key=lambda path: path.name)
    if not paths:
        raise CorpusError(f"no corpus shards found below {suite}")
    return paths


def load_support_map(path: pathlib.Path) -> dict[tuple[str, int, str], dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            rows = list(csv.DictReader(stream))
    except OSError as error:
        raise CorpusError(f"cannot read support map {path}: {error}") from error
    expected_fields = {
        "mode", "opcode", "subopcode", "target", "classification", "basis"
    }
    if not rows or set(rows[0]) != expected_fields:
        raise CorpusError(f"{path}: unsupported support-map schema")
    result = {}
    for row in rows:
        if row["classification"] not in {"implemented", "known_target_gap"}:
            raise CorpusError(
                f"{path}: unknown support classification {row['classification']!r}"
            )
        key = (row["mode"], int(row["opcode"], 16), row["subopcode"])
        if key in result:
            raise CorpusError(f"{path}: duplicate support-map key {key!r}")
        result[key] = row
    if len(result) not in {1296, 1552}:
        raise CorpusError(
            f"{path}: expected 1296 or 1552 support rows, got {len(result)}"
        )
    return result


def support_table_family(
    support: dict[tuple[str, int, str], dict[str, str]],
) -> dict[str, str]:
    """Return the table-name family used by this support map generation."""

    if ("upd9002op", 0x00, "-") in support:
        return SUPPORT_TABLE_FAMILIES["upd9002"]
    if ("v30op", 0x00, "-") in support:
        return SUPPORT_TABLE_FAMILIES["v30"]
    raise CorpusError("support map does not contain a recognized root table")


def metadata_for_form(
    metadata: dict[str, Any], form: str
) -> tuple[dict[str, Any], dict[str, Any], str, str, int]:
    opcode_key = form.split(".", 1)[0]
    opcodes = metadata["opcodes"]
    if opcode_key not in opcodes:
        raise CorpusError(f"{form}: no metadata entry for {opcode_key}")
    parent = opcodes[opcode_key]
    entry = parent
    if "." in form:
        base, register = form.split(".", 1)
        if base not in GROUP_FORMS or register not in {str(i) for i in range(8)}:
            raise CorpusError(f"{form}: unsupported structural form")
        try:
            entry = parent["reg"][register]
        except (KeyError, TypeError) as error:
            raise CorpusError(f"{form}: no metadata register entry") from error
    status = entry.get("status", "normal")
    if status not in REGISTER_STATUSES | PRIMARY_STATUSES:
        raise CorpusError(f"{form}: unknown resolved metadata status {status!r}")
    arch = entry.get("arch", parent.get("arch"))
    if arch is None:
        arch = "missing"
    if arch not in {"86", "186", "v30", "missing"}:
        raise CorpusError(f"{form}: unknown resolved architecture {arch!r}")
    flags_mask = entry.get("flags-mask", parent.get("flags-mask", 0xffff))
    if not isinstance(flags_mask, int) or not 0 <= flags_mask <= 0xffff:
        raise CorpusError(f"{form}: invalid resolved flags mask")
    return parent, entry, status, arch, flags_mask


def resolve_dispatch(
    form: str,
    instruction: list[int],
    support: dict[tuple[str, int, str], dict[str, str]],
) -> dict[str, Any]:
    position = 0
    repeat = "none"
    segment_prefixes = []
    lock_prefixes = []
    while position < len(instruction):
        byte = instruction[position]
        if byte in SEGMENT_PREFIXES:
            segment_prefixes.append(byte)
        elif byte in REPEAT_PREFIXES:
            repeat = REPEAT_PREFIXES[byte]
        elif byte in IGNORED_PREFIXES:
            lock_prefixes.append(byte)
        else:
            break
        position += 1
    if position >= len(instruction):
        raise CorpusError(f"{form}: instruction contains prefixes only")
    opcode = instruction[position]
    position += 1
    second_byte = None
    modrm_reg = None
    if opcode == 0x0f:
        if position >= len(instruction):
            raise CorpusError(f"{form}: 0f instruction lacks a second byte")
        second_byte = instruction[position]
        position += 1
    if form.split(".", 1)[0] in GROUP_FORMS:
        if position >= len(instruction):
            raise CorpusError(f"{form}: group instruction lacks ModR/M")
        modrm_reg = (instruction[position] >> 3) & 7

    expected_key = form.split(".", 1)[0]
    if expected_key.startswith("0F"):
        if opcode != 0x0f or second_byte != int(expected_key[2:], 16):
            raise CorpusError(
                f"{form}: bytes do not match extended opcode identity"
            )
    elif opcode != int(expected_key, 16):
        raise CorpusError(
            f"{form}: final opcode {opcode:02x} does not match shard identity"
        )
    if "." in form and modrm_reg != int(form.split(".", 1)[1]):
        raise CorpusError(f"{form}: ModR/M reg does not match shard identity")

    family = support_table_family(support)
    if (
        repeat == "repnc"
        and opcode in M70_REPNC_STRING_OPCODES
        and (family["repnc"], opcode, "-") in support
    ):
        support_key = (family["repnc"], opcode, "-")
    elif repeat == "repnc":
        support_key = (family["root"], 0x64, "-")
    else:
        mode = {
            "none": family["root"],
            "repc": family["repc"],
            "repe": family["repe"],
            "repne": family["repne"],
        }[repeat]
        if mode == family["root"] and opcode == 0x0f:
            support_key = (family["0f"], 0x0f, f"0x{second_byte:02x}")
        elif mode == family["root"] and opcode in {0xf6, 0xf7}:
            support_key = (
                family[f"{opcode:02x}"], opcode, f"/{modrm_reg}"
            )
        else:
            support_key = (mode, opcode, "-")
    if support_key not in support:
        raise CorpusError(f"{form}: no M42 support-map row for {support_key!r}")
    row = support[support_key]
    return {
        "repeat_prefix": repeat,
        "segment_prefixes": [f"0x{byte:02x}" for byte in segment_prefixes],
        "lock_prefixes": [f"0x{byte:02x}" for byte in lock_prefixes],
        "opcode": f"0x{opcode:02x}",
        "second_byte": None if second_byte is None else f"0x{second_byte:02x}",
        "modrm_reg": modrm_reg,
        "support_mode": support_key[0],
        "support_subopcode": support_key[2],
        "support_target": row["target"],
        "support_classification": row["classification"],
    }


def classify_record(
    form: str,
    record: dict[str, Any],
    metadata: dict[str, Any],
    support: dict[tuple[str, int, str], dict[str, str]],
) -> dict[str, Any]:
    _, _, status, arch, flags_mask = metadata_for_form(metadata, form)
    if record["initial"]["queue"]:
        return {
            "classification": "unsupported_fixture",
            "reason": "prefetch queue injection is not available in the M42 harness",
            "metadata_status": status,
            "metadata_arch": arch,
            "flags_mask": flags_mask,
            "dispatch": None,
        }
    dispatch = resolve_dispatch(form, record["bytes"], support)
    if status in {"fpu", "undefined"}:
        classification = "upstream_nonblocking"
        reason = f"upstream metadata status {status} is outside blocking semantics"
    elif dispatch["support_classification"] == "known_target_gap":
        classification = "known_target_gap"
        reason = "the exact M42 final dispatch form is absent from the current target"
    else:
        classification = "applicable"
        reason = "implemented target form with a defined V20 final-state oracle"
    return {
        "classification": classification,
        "reason": reason,
        "metadata_status": status,
        "metadata_arch": arch,
        "flags_mask": flags_mask,
        "dispatch": dispatch,
    }


def gap_selector(form: str, resolved: dict[str, Any]) -> dict[str, Any]:
    dispatch = resolved["dispatch"]
    return {
        "metadata_form": form,
        "opcode": dispatch["opcode"],
        "second_byte": dispatch["second_byte"],
        "modrm_reg": dispatch["modrm_reg"],
        "repeat_prefix": dispatch["repeat_prefix"],
        "segment_prefix_constraint": "dispatch-neutral-any",
        "lock_prefix_constraint": "dispatch-neutral-any",
        "support_mode": dispatch["support_mode"],
        "support_subopcode": dispatch["support_subopcode"],
        "support_target": dispatch["support_target"],
    }


def profile_records(records: list[dict[str, Any]], profile: str) -> list[dict[str, Any]]:
    empty = [record for record in records if not record["initial"]["queue"]]
    empty.sort(key=lambda record: record["hash"])
    if profile == "ci":
        return empty[:500]
    if profile == "full":
        return empty
    raise CorpusError(f"unknown profile {profile!r}")


def expected_registers(record: dict[str, Any]) -> dict[str, int]:
    expected = dict(record["initial"]["regs"])
    expected.update(record["final"]["regs"])
    return {name: expected[name] for name in REGISTER_ORDER}


def expected_memory(record: dict[str, Any]) -> tuple[list[int], dict[int, int]]:
    memory = {address: value for address, value in record["initial"]["ram"]}
    memory.update({address: value for address, value in record["final"]["ram"]})
    addresses = sorted(memory)
    return addresses, {address: memory[address] for address in addresses}


def expected_io(record: dict[str, Any]) -> list[dict[str, Any]]:
    result = []
    cycles = record["cycles"]
    for index, cycle in enumerate(cycles):
        if (
            not isinstance(cycle, list)
            or len(cycle) != 11
            or cycle[7] not in {"IOR", "IOW"}
            or cycle[8] != "T1"
        ):
            continue
        bus_type = cycle[7]
        port = cycle[1]
        value = None
        for following in cycles[index + 1:index + 5]:
            if (
                isinstance(following, list)
                and len(following) == 11
                and following[8] == "T3"
            ):
                value = following[6]
                break
        if (
            not isinstance(port, int)
            or not 0 <= port <= 0xffff
            or not isinstance(value, int)
            or not 0 <= value <= 0xff
        ):
            raise CorpusError(
                f"record {record['hash']}: cannot resolve {bus_type} cycle"
            )
        result.append(
            {
                "direction": "read" if bus_type == "IOR" else "write",
                "port": port,
                "value": value,
            }
        )
    return result


def translated_initial_ram(record: dict[str, Any]) -> list[list[int]]:
    """Translate the pinned V20 OUTS segment-override fixture convention.

    The pinned corpus records OUTS source bytes at their default DS-relative
    physical addresses even when a segment override is present.  The hardware
    bus cycles identify the overridden physical addresses and carry the same
    byte values.  Mirror only those cycle-proven values to the effective
    addresses, and fail closed on any inconsistent record.
    """

    position = 0
    segment_prefix = None
    instruction = record["bytes"]
    while position < len(instruction):
        byte = instruction[position]
        if byte in SEGMENT_PREFIXES:
            segment_prefix = byte
        elif byte not in set(REPEAT_PREFIXES) | IGNORED_PREFIXES:
            break
        position += 1
    if position >= len(instruction) or instruction[position] not in {0x6E, 0x6F}:
        return [list(entry) for entry in record["initial"]["ram"]]
    if segment_prefix is None or segment_prefix == 0x3E:
        return [list(entry) for entry in record["initial"]["ram"]]

    registers = record["initial"]["regs"]
    selected_register = SEGMENT_PREFIX_REGISTERS[segment_prefix]
    selected_base = registers[selected_register] << 4
    default_base = registers["ds"] << 4
    entries = [list(entry) for entry in record["initial"]["ram"]]
    memory: dict[int, int] = {}
    for address, value in entries:
        if address in memory and memory[address] != value:
            raise CorpusError(
                f"record {record['hash']}: conflicting initial RAM values at "
                f"0x{address:05x}"
            )
        memory[address] = value

    cycles = record["cycles"]
    for index, cycle in enumerate(cycles):
        if (
            not isinstance(cycle, list)
            or len(cycle) != 11
            or cycle[7] != "MEMR"
            or cycle[8] != "T1"
        ):
            continue
        effective_address = cycle[1]
        if not isinstance(effective_address, int):
            raise CorpusError(
                f"record {record['hash']}: OUTS MEMR lacks a physical address"
            )
        effective_address &= 0xFFFFF
        value = None
        observed_segments = set()
        for following in cycles[index + 1:index + 6]:
            if not isinstance(following, list) or len(following) != 11:
                continue
            if following[8] == "T1":
                break
            if following[2] != "--":
                observed_segments.add(following[2])
            if following[8] == "T3":
                value = following[6]
                break
        expected_segment = selected_register.upper()
        if value is None or not isinstance(value, int) or not 0 <= value <= 0xFF:
            raise CorpusError(
                f"record {record['hash']}: cannot resolve OUTS MEMR data"
            )
        if observed_segments != {expected_segment}:
            raise CorpusError(
                f"record {record['hash']}: OUTS segment evidence differs: "
                f"expected={expected_segment} actual={sorted(observed_segments)!r}"
            )
        offset = (effective_address - selected_base) & 0xFFFF
        default_address = (default_base + offset) & 0xFFFFF
        if memory.get(default_address) != value:
            raise CorpusError(
                f"record {record['hash']}: OUTS default source does not match "
                f"cycle data at 0x{default_address:05x}"
            )
        if effective_address in memory:
            if memory[effective_address] != value:
                raise CorpusError(
                    f"record {record['hash']}: OUTS effective source conflicts "
                    f"at 0x{effective_address:05x}"
                )
            continue
        memory[effective_address] = value
        entries.append([effective_address, value])
    return entries


def expected_termination(form: str, record: dict[str, Any]) -> str:
    if form == "F4":
        return "halt"
    if form not in {"F6.6", "F6.7", "F7.6", "F7.7"}:
        return "normal"
    initial_memory = {
        address: value for address, value in record["initial"]["ram"]
    }
    vector_ip = initial_memory.get(0, 0) | (initial_memory.get(1, 0) << 8)
    vector_cs = initial_memory.get(2, 0) | (initial_memory.get(3, 0) << 8)
    registers = expected_registers(record)
    if registers["ip"] == vector_ip and registers["cs"] == vector_cs:
        return "type0"
    return "normal"


def write_request(path: pathlib.Path, records: list[dict[str, Any]]) -> list[dict[str, Any]]:
    contexts = []
    with path.open("wb") as stream:
        stream.write(b"V20REQ2\0")
        stream.write(struct.pack("<I", len(records)))
        for record in records:
            digest = sha256_bytes(canonical_bytes(record))
            stream.write(bytes.fromhex(digest))
            registers = record["initial"]["regs"]
            stream.write(struct.pack("<14H", *(registers[name] for name in REGISTER_ORDER)))
            ram = translated_initial_ram(record)
            stream.write(struct.pack("<I", len(ram)))
            for address, value in ram:
                stream.write(struct.pack("<IB", address, value))
            watch, expected_ram = expected_memory(record)
            stream.write(struct.pack("<I", len(watch)))
            for address in watch:
                stream.write(struct.pack("<I", address))
            contexts.append(
                {
                    "record": record,
                    "record_digest": digest,
                    "watch": watch,
                    "expected_ram": expected_ram,
                }
            )
    return contexts


def read_exact_binary(stream: Any, size: int, where: str) -> bytes:
    value = stream.read(size)
    if len(value) != size:
        raise CorpusError(f"{where}: truncated worker response")
    return value


def instruction_opcode(record: dict[str, Any]) -> tuple[int, int | None]:
    prefixes = {0x26, 0x2E, 0x36, 0x3E, 0xF0, 0xF2, 0xF3}
    data = record["bytes"]
    index = 0
    while index < len(data) and data[index] in prefixes:
        index += 1
    if index == len(data):
        return data[-1], None
    opcode = data[index]
    following = data[index + 1] if index + 1 < len(data) else None
    return opcode, following


def classify_execution_result(
    record: dict[str, Any],
    worker_termination: str,
    interrupt_count: int,
    interrupt_vector: int,
) -> dict[str, Any]:
    if worker_termination == "halt":
        return {
            "kind": "halt",
            "interrupt_count": interrupt_count,
            "interrupt_vector": None if interrupt_count == 0 else interrupt_vector,
            "termination": "halt",
        }
    if interrupt_count == 0:
        return {
            "kind": "normal",
            "interrupt_count": 0,
            "interrupt_vector": None,
            "termination": "normal",
        }
    opcode, following = instruction_opcode(record)
    if opcode in {0xCC, 0xCD, 0xCE}:
        kind = "software_interrupt"
        termination = "normal"
    elif (
        opcode in {0xF6, 0xF7}
        and following is not None
        and ((following >> 3) & 7) in {6, 7}
        and interrupt_vector == 0
    ):
        kind = "divide_error"
        termination = "type0"
    else:
        kind = "exception"
        termination = "type0" if interrupt_vector == 0 else "normal"
    return {
        "kind": kind,
        "interrupt_count": interrupt_count,
        "interrupt_vector": interrupt_vector,
        "termination": termination,
    }


def read_response(path: pathlib.Path, contexts: list[dict[str, Any]]) -> list[dict[str, Any]]:
    results = []
    with path.open("rb") as stream:
        if read_exact_binary(stream, 8, "response magic") != b"V20RSP2\0":
            raise CorpusError("worker returned an unsupported response schema")
        count = struct.unpack("<I", read_exact_binary(stream, 4, "response count"))[0]
        if count != len(contexts):
            raise CorpusError(
                f"worker response count mismatch: expected={len(contexts)} actual={count}"
            )
        for context in contexts:
            digest = read_exact_binary(stream, 32, "record digest").hex()
            if digest != context["record_digest"]:
                raise CorpusError("worker response record digest is out of order")
            termination_code = read_exact_binary(stream, 1, "termination")[0]
            if termination_code not in {0, 1, 2}:
                raise CorpusError(f"worker returned unknown termination {termination_code}")
            interrupt_count = struct.unpack(
                "<I", read_exact_binary(stream, 4, "interrupt count")
            )[0]
            interrupt_vector = read_exact_binary(stream, 1, "interrupt vector")[0]
            values = struct.unpack(
                "<14H", read_exact_binary(stream, 28, "register state")
            )
            actual_registers = dict(zip(REGISTER_ORDER, values))
            watch_count = struct.unpack(
                "<I", read_exact_binary(stream, 4, "watch count")
            )[0]
            if watch_count != len(context["watch"]):
                raise CorpusError("worker response watch count mismatch")
            watch_values = read_exact_binary(stream, watch_count, "watch values")
            actual_ram = {
                address: value
                for address, value in zip(context["watch"], watch_values)
            }
            io_count = struct.unpack(
                "<I", read_exact_binary(stream, 4, "I/O count")
            )[0]
            if io_count > 1024:
                raise CorpusError("worker response I/O count exceeds protocol limit")
            io = []
            for _ in range(io_count):
                port, value, direction = struct.unpack(
                    "<HBB", read_exact_binary(stream, 4, "I/O event")
                )
                if direction not in {0, 1}:
                    raise CorpusError("worker response has unknown I/O direction")
                io.append(
                    {
                        "direction": "read" if direction == 0 else "write",
                        "port": port,
                        "value": value,
                    }
                )
            worker_termination = {
                0: "normal",
                1: "type0",
                2: "halt",
            }[termination_code]
            execution_result = classify_execution_result(
                context["record"],
                worker_termination,
                interrupt_count,
                interrupt_vector,
            )
            results.append(
                {
                    "termination": execution_result["termination"],
                    "execution_result": execution_result,
                    "registers": actual_registers,
                    "ram": actual_ram,
                    "io": io,
                }
            )
        if stream.read(1):
            raise CorpusError("worker response contains trailing bytes")
    return results


def run_worker_once(
    worker: pathlib.Path,
    records: list[dict[str, Any]],
    timeout: float,
) -> tuple[str, list[dict[str, Any]] | None]:
    with tempfile.TemporaryDirectory(prefix="vaeg-ssts-shard-") as temporary:
        root = pathlib.Path(temporary)
        request = root / "request.bin"
        response = root / "response.bin"
        contexts = write_request(request, records)
        try:
            completed = subprocess.run(
                [str(worker), "--upd9002-ssts-worker", str(request), str(response)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                timeout=timeout,
                check=False,
                text=True,
            )
        except subprocess.TimeoutExpired:
            return "timeout", None
        if completed.returncode != 0:
            return "crash", None
        try:
            return "ok", read_response(response, contexts)
        except (CorpusError, OSError):
            return "crash", None


def run_worker_contained(
    worker: pathlib.Path,
    records: list[dict[str, Any]],
    timeout: float,
) -> list[tuple[str, dict[str, Any] | None]]:
    if not records:
        return []
    status, results = run_worker_once(worker, records, timeout)
    if status == "ok":
        assert results is not None
        return [("ok", result) for result in results]
    if len(records) == 1:
        return [(status, None)]
    middle = len(records) // 2
    return (
        run_worker_contained(worker, records[:middle], timeout)
        + run_worker_contained(worker, records[middle:], timeout)
    )


def hex_registers(registers: dict[str, int]) -> dict[str, str]:
    return {name: f"{registers[name]:04x}" for name in REGISTER_ORDER}


def make_failure(
    dataset_id: str,
    profile: str,
    form: str,
    classification: str,
    flags_mask: int,
    context: dict[str, Any],
    status: str,
    actual: dict[str, Any] | None,
) -> dict[str, Any]:
    record = context["record"]
    expected_regs = expected_registers(record)
    expected_ram = context["expected_ram"]
    expected_events = expected_io(record)
    expected_term = expected_termination(form, record)
    initial_digest = sha256_bytes(canonical_bytes(record["initial"]))
    mismatch_kinds = []
    register_differences = []
    ram_differences = []
    io_differences = []
    masked_flags = {
        "mask": f"{flags_mask:04x}",
        "expected": f"{expected_regs['flags'] & flags_mask:04x}",
        "actual": None,
    }
    actual_state: Any = None
    actual_term = status
    if actual is None:
        mismatch_kinds.append(status)
    else:
        actual_term = actual["termination"]
        for name in REGISTER_ORDER:
            if name == "flags":
                continue
            if expected_regs[name] != actual["registers"][name]:
                register_differences.append(
                    {
                        "register": name,
                        "expected": f"{expected_regs[name]:04x}",
                        "actual": f"{actual['registers'][name]:04x}",
                    }
                )
        masked_actual = actual["registers"]["flags"] & flags_mask
        masked_flags["actual"] = f"{masked_actual:04x}"
        if masked_actual != (expected_regs["flags"] & flags_mask):
            register_differences.append(
                {
                    "register": "flags",
                    "expected": masked_flags["expected"],
                    "actual": masked_flags["actual"],
                }
            )
        if register_differences:
            mismatch_kinds.append("registers")
        for address in sorted(expected_ram):
            if expected_ram[address] != actual["ram"][address]:
                ram_differences.append(
                    {
                        "address": f"{address:05x}",
                        "expected": f"{expected_ram[address]:02x}",
                        "actual": f"{actual['ram'][address]:02x}",
                    }
                )
        if ram_differences:
            mismatch_kinds.append("ram")
        if expected_events != actual["io"]:
            mismatch_kinds.append("io")
            maximum = max(len(expected_events), len(actual["io"]))
            for index in range(maximum):
                expected_event = expected_events[index] if index < len(expected_events) else None
                actual_event = actual["io"][index] if index < len(actual["io"]) else None
                if expected_event != actual_event:
                    io_differences.append(
                        {
                            "index": index,
                            "expected": expected_event,
                            "actual": actual_event,
                        }
                    )
        if expected_term != actual_term:
            mismatch_kinds.append("termination")
        actual_state = {
            "registers": hex_registers(actual["registers"]),
            "termination": actual_term,
        }
    content = {
        "dataset_id": dataset_id,
        "profile": profile,
        "record_hash": context["record_digest"],
        "upstream_test_hash": record["hash"],
        "opcode_form": form,
        "classification": classification,
        "initial_state_digest": initial_digest,
        "expected_state": {
            "registers": hex_registers(expected_regs),
            "termination": expected_term,
        },
        "actual_state": actual_state,
        "masked_flags_comparison": masked_flags,
        "ram_differences": ram_differences,
        "io_differences": io_differences,
        "exception_or_halt_result": {
            "expected": expected_term,
            "actual": actual_term,
        },
        "timeout_crash_status": status if status in {"timeout", "crash"} else "none",
        "instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "mismatch_kinds": sorted(set(mismatch_kinds)),
        "register_differences": register_differences,
    }
    signature_bytes = json.dumps(
        content, ensure_ascii=True, separators=(",", ":"), sort_keys=False
    ).encode("utf-8")
    return {
        "signature_sha256": sha256_bytes(signature_bytes),
        "content": content,
    }


def compare_result(
    dataset_id: str,
    profile: str,
    form: str,
    resolved: dict[str, Any],
    context: dict[str, Any],
    worker_status: str,
    actual: dict[str, Any] | None,
) -> tuple[str, dict[str, Any] | None]:
    failure = make_failure(
        dataset_id,
        profile,
        form,
        resolved["classification"],
        resolved["flags_mask"],
        context,
        worker_status,
        actual,
    )
    if not failure["content"]["mismatch_kinds"]:
        return "pass", None
    if worker_status in {"timeout", "crash"}:
        return worker_status, failure
    return "semantic_failure", failure


def difficult_family(form: str, record: dict[str, Any]) -> list[str]:
    families = []
    byte_set = set(record["bytes"])
    if 0x65 in byte_set and form in {"6C", "6D", "6E", "6F"}:
        families.append("repc-string-io")
    if 0x64 in byte_set and form in {"6C", "6D", "6E", "6F"}:
        families.append("repnc-string-io")
    if form in {"A6", "A7"} and byte_set & {0xF2, 0xF3}:
        families.append("repe-repne-cmps-order")
    if form.startswith(("C0.", "C1.", "D2.", "D3.")):
        families.append("shift-count-six-bit")
    if form in {"F6.6", "F6.7", "F7.6", "F7.7"}:
        families.append("div-idiv")
        if form in {"F6.7", "F7.7"} and byte_set & {0xF2, 0xF3}:
            families.append("rep-idiv")
    if form.startswith("0F"):
        families.append("0f-extension")
    return families


def run_profile(
    dataset_root: pathlib.Path,
    manifest: dict[str, Any],
    support_map_path: pathlib.Path,
    worker: pathlib.Path,
    profile: str,
    timeout: float,
    flags_comparison: str = "defined",
) -> dict[str, Any]:
    if flags_comparison not in {"defined", "all16"}:
        raise CorpusError(f"unknown FLAGS comparison: {flags_comparison}")
    verify_fast(dataset_root, manifest)
    if not worker.is_file():
        raise CorpusError(f"worker executable is missing: {worker}")
    metadata = json.loads(
        (dataset_root / SUITE_PATH / "metadata.json").read_text(encoding="utf-8")
    )
    validate_metadata(metadata)
    support = load_support_map(support_map_path)
    category_counts = Counter()
    result_counts = Counter()
    termination_counts = Counter()
    difficult = Counter()
    difficult_results = Counter()
    failures = []
    per_form = []
    selected_testsets = []
    selected_total = 0
    executed_total = 0
    for shard_number, path in enumerate(corpus_files(dataset_root), 1):
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            records = json.load(stream)
        form = path.name.removesuffix(".json.gz").upper()
        selected = profile_records(records, profile)
        resolved_records = []
        runnable = []
        selected_hashes = []
        form_categories = Counter()
        form_results = Counter()
        for record in selected:
            validate_record(record, f"{path.name}:{record.get('idx', '?')}")
            resolved = classify_record(form, record, metadata, support)
            classification = resolved["classification"]
            if classification == "unsupported_fixture":
                raise CorpusError(f"{form}:{record['hash']}: selected unsupported fixture")
            resolved_records.append((record, resolved))
            category_counts[classification] += 1
            form_categories[classification] += 1
            selected_hashes.append(record["hash"])
            for family in difficult_family(form, record):
                difficult[f"{family}:{classification}"] += 1
            if classification == "applicable":
                runnable.append(record)
            else:
                result_counts["skip"] += 1
                form_results["skip"] += 1
                for family in difficult_family(form, record):
                    difficult_results[f"{family}:skip"] += 1
        contained = run_worker_contained(worker, runnable, timeout)
        if len(contained) != len(runnable):
            raise CorpusError(f"{form}: worker result count mismatch")
        runnable_index = 0
        for record, resolved in resolved_records:
            if resolved["classification"] != "applicable":
                continue
            worker_status, actual = contained[runnable_index]
            runnable_index += 1
            watch, expected_ram = expected_memory(record)
            context = {
                "record": record,
                "record_digest": sha256_bytes(canonical_bytes(record)),
                "watch": watch,
                "expected_ram": expected_ram,
            }
            comparison_resolved = resolved
            if flags_comparison == "all16":
                comparison_resolved = dict(resolved)
                comparison_resolved["flags_mask"] = 0xffff
            outcome, failure = compare_result(
                manifest["dataset_id"], profile, form, comparison_resolved,
                context, worker_status, actual,
            )
            result_counts[outcome] += 1
            form_results[outcome] += 1
            for family in difficult_family(form, record):
                difficult_results[f"{family}:{outcome}"] += 1
            executed_total += 1
            if actual is not None:
                termination_counts[actual["termination"]] += 1
            if failure is not None:
                failures.append(failure)
        selected_hashes.sort()
        selected_testsets.append(
            {
                "form": form,
                "selected_count": len(selected_hashes),
                "selected_test_hashes_sha256": sha256_bytes(
                    canonical_bytes(selected_hashes)
                ),
            }
        )
        per_form.append(
            {
                "form": form,
                "selected_count": len(selected),
                "classification_counts": dict(sorted(form_categories.items())),
                "result_counts": dict(sorted(form_results.items())),
            }
        )
        selected_total += len(selected)
        print(
            f"ssts-run: {shard_number:03d}/{len(corpus_files(dataset_root)):03d} "
            f"{form} selected={len(selected)} executable={len(runnable)} "
            f"results={dict(sorted(form_results.items()))}",
            file=sys.stderr,
            flush=True,
        )
    failures.sort(
        key=lambda failure: (
            failure["content"]["opcode_form"],
            failure["content"]["record_hash"],
        )
    )
    selection_digest = sha256_bytes(canonical_bytes(selected_testsets))
    result = {
        "schema": "vaeg-upd9002-ssts-result-v1",
        "dataset_id": manifest["dataset_id"],
        "dataset_digest": manifest["dataset_digest"],
        "metadata_sha256": manifest["metadata_sha256"],
        "corpus_sha256": manifest["corpus_sha256"],
        "upstream_commit": UPSTREAM_COMMIT,
        "profile": profile,
        "profile_definition": (
            "empty initial queue; stable upstream-hash order; maximum 500 per form"
            if profile == "ci"
            else "all records with an empty initial prefetch queue"
        ),
        "selection_digest": selection_digest,
        "selected_records": selected_total,
        "executed_records": executed_total,
        "classification_counts": dict(sorted(category_counts.items())),
        "result_counts": dict(sorted(result_counts.items())),
        "termination_counts": dict(sorted(termination_counts.items())),
        "prefetched_unsupported_fixture_records": manifest["counts"][
            "prefetched_records"
        ],
        "failure_signature_count": len(failures),
        "failure_signatures": failures,
        "difficult_family_counts": dict(sorted(difficult.items())),
        "difficult_family_result_counts": dict(sorted(difficult_results.items())),
        "per_form": per_form,
    }
    if flags_comparison == "all16":
        result["flags_comparison"] = "all16"
    return result


def externalize_failure_signatures(
    result: dict[str, Any], directory: pathlib.Path
) -> None:
    failures = result.pop("failure_signatures")
    groups: dict[str, list[dict[str, Any]]] = {}
    signature_index = []
    for failure in failures:
        form = failure["content"]["opcode_form"]
        group = form[0].lower()
        groups.setdefault(group, []).append(failure)
        signature_index.append(
            {
                "record_hash": failure["content"]["record_hash"],
                "signature_sha256": failure["signature_sha256"],
            }
        )
    directory.mkdir(parents=True, exist_ok=True)
    files = []
    for group, values in sorted(groups.items()):
        path = directory / f"{group}.json.gz"
        payload = {
            "schema": "vaeg-upd9002-ssts-failures-v1",
            "dataset_id": result["dataset_id"],
            "profile": result["profile"],
            "group": group,
            "failure_count": len(values),
            "failure_signatures": values,
        }
        serialized = canonical_bytes(payload) + b"\n"
        with path.open("wb") as stream:
            with gzip.GzipFile(
                filename="",
                mode="wb",
                compresslevel=9,
                fileobj=stream,
                mtime=0,
            ) as compressed:
                compressed.write(serialized)
        files.append(
            {
                "path": f"{directory.name}/{path.name}",
                "failure_count": len(values),
                "canonical_sha256": sha256_bytes(serialized),
            }
        )
    signature_index.sort(key=lambda item: item["record_hash"])
    result["failure_signature_index_sha256"] = sha256_bytes(
        canonical_bytes(signature_index)
    )
    result["failure_signature_files"] = files


def verify_failure_files(summary_path: pathlib.Path, result: dict[str, Any]) -> None:
    files = result.get("failure_signature_files", [])
    if not isinstance(files, list):
        raise CorpusError("failure_signature_files: expected array")
    count = 0
    for item in files:
        if not isinstance(item, dict) or set(item) != {
            "path", "failure_count", "canonical_sha256"
        }:
            raise CorpusError("failure signature file entry has unknown schema")
        path = summary_path.parent / item["path"]
        if not path.is_file():
            raise CorpusError(f"failure signature file is missing: {path}")
        with gzip.open(path, "rb") as stream:
            serialized = stream.read()
        digest = sha256_bytes(serialized)
        if digest != item["canonical_sha256"]:
            raise CorpusError(
                f"failure signature file digest mismatch: {path} "
                f"expected={item['canonical_sha256']} actual={digest}"
            )
        count += item["failure_count"]
    if count != result.get("failure_signature_count"):
        raise CorpusError(
            "failure signature sidecar count differs from result summary"
        )


def load_failures(summary_path: pathlib.Path) -> tuple[dict[str, Any], dict[str, Any]]:
    result = json.loads(summary_path.read_text(encoding="utf-8"))
    if result.get("schema") != "vaeg-upd9002-ssts-result-v1":
        raise CorpusError(f"{summary_path}: unknown result schema")
    verify_failure_files(summary_path, result)
    failures = {}
    for item in result["failure_signature_files"]:
        with gzip.open(summary_path.parent / item["path"], "rt", encoding="utf-8") as stream:
            payload = json.load(stream)
        if payload.get("schema") != "vaeg-upd9002-ssts-failures-v1":
            raise CorpusError(f"{item['path']}: unknown failure sidecar schema")
        if payload.get("failure_count") != len(payload.get("failure_signatures", [])):
            raise CorpusError(f"{item['path']}: failure count differs from content")
        for failure in payload["failure_signatures"]:
            record_hash = failure["content"]["record_hash"]
            if record_hash in failures:
                raise CorpusError(f"duplicate failure record hash {record_hash}")
            failures[record_hash] = failure
    if len(failures) != result["failure_signature_count"]:
        raise CorpusError(f"{summary_path}: unique failure count differs from summary")
    return result, failures


def transition_reason(
    old_failure: dict[str, Any], new_failure: dict[str, Any] | None
) -> str:
    old_content = old_failure["content"]
    old_kinds = set(old_content["mismatch_kinds"])
    form = old_content["opcode_form"]
    if new_failure is None and form in {"6E", "6F"} and old_kinds == {"io"}:
        return "segment_override_outs_fixture_translation"
    if new_failure is not None:
        new_kinds = set(new_failure["content"]["mismatch_kinds"])
        if (
            form == "6F"
            and "io" in old_kinds
            and "io" in new_kinds
            and old_content["io_differences"]
            != new_failure["content"]["io_differences"]
        ):
            return "segment_override_outs_fixture_translation"
        if form == "CD" and "termination" in old_kinds and "termination" not in new_kinds:
            return "software_interrupt_execution_result_classification"
        if (
            form in {"F6.6", "F6.7", "F7.6", "F7.7"}
            and old_content["exception_or_halt_result"]
            != new_failure["content"]["exception_or_halt_result"]
        ):
            return "divide_error_execution_result_classification"
    raise CorpusError(
        f"record {old_content['record_hash']}: unclassified baseline transition"
    )


def create_transition_profile(
    old_path: pathlib.Path, new_path: pathlib.Path
) -> dict[str, Any]:
    old_result, old_failures = load_failures(old_path)
    new_result, new_failures = load_failures(new_path)
    for field in (
        "dataset_id", "dataset_digest", "metadata_sha256", "corpus_sha256",
        "upstream_commit", "profile", "profile_definition", "selection_digest",
        "selected_records", "executed_records", "classification_counts",
    ):
        if old_result.get(field) != new_result.get(field):
            raise CorpusError(f"transition changes immutable profile field {field}")
    removed = []
    entered = []
    changed = []
    for record_hash in sorted(old_failures.keys() - new_failures.keys()):
        old_failure = old_failures[record_hash]
        removed.append(
            {
                "record_hash": record_hash,
                "opcode_form": old_failure["content"]["opcode_form"],
                "old_signature_sha256": old_failure["signature_sha256"],
                "reason": transition_reason(old_failure, None),
            }
        )
    for record_hash in sorted(new_failures.keys() - old_failures.keys()):
        failure = new_failures[record_hash]
        entered.append(
            {
                "record_hash": record_hash,
                "opcode_form": failure["content"]["opcode_form"],
                "new_signature_sha256": failure["signature_sha256"],
                "reason": "unexpected_new_semantic_failure",
            }
        )
    for record_hash in sorted(old_failures.keys() & new_failures.keys()):
        old_failure = old_failures[record_hash]
        new_failure = new_failures[record_hash]
        if old_failure["signature_sha256"] == new_failure["signature_sha256"]:
            continue
        changed.append(
            {
                "record_hash": record_hash,
                "opcode_form": old_failure["content"]["opcode_form"],
                "old_signature_sha256": old_failure["signature_sha256"],
                "new_signature_sha256": new_failure["signature_sha256"],
                "old_mismatch_kinds": old_failure["content"]["mismatch_kinds"],
                "new_mismatch_kinds": new_failure["content"]["mismatch_kinds"],
                "reason": transition_reason(old_failure, new_failure),
            }
        )
    if entered:
        raise CorpusError("corrected baseline unexpectedly adds semantic failures")
    changed_records = len(removed) + len(entered) + len(changed)
    if changed_records != len(
        (old_failures.keys() ^ new_failures.keys())
        | {
            record_hash
            for record_hash in old_failures.keys() & new_failures.keys()
            if old_failures[record_hash]["signature_sha256"]
            != new_failures[record_hash]["signature_sha256"]
        }
    ):
        raise AssertionError("transition report omitted a changed record")
    return {
        "profile": old_result["profile"],
        "old_summary_sha256": sha256_file(old_path),
        "new_summary_sha256": sha256_file(new_path),
        "old_failure_count": len(old_failures),
        "new_failure_count": len(new_failures),
        "old_signature_index_sha256": old_result["failure_signature_index_sha256"],
        "new_signature_index_sha256": new_result["failure_signature_index_sha256"],
        "removed_from_semantic_failure": removed,
        "newly_entering_semantic_failure": entered,
        "reclassified_as_unsupported_fixture": [],
        "changed_failure_signatures": changed,
        "unchanged_failure_count": len(old_failures) - len(removed) - len(changed),
    }


def create_transition_report(
    old_ci: pathlib.Path,
    old_full: pathlib.Path,
    new_ci: pathlib.Path,
    new_full: pathlib.Path,
    old_known_gaps: pathlib.Path,
    new_known_gaps: pathlib.Path,
) -> dict[str, Any]:
    old_gap_bytes = old_known_gaps.read_bytes()
    new_gap_bytes = new_known_gaps.read_bytes()
    if old_gap_bytes != new_gap_bytes:
        raise CorpusError("known-gap selector file changed across corrected baselines")
    gaps = json.loads(new_gap_bytes)
    return {
        "schema": "vaeg-upd9002-ssts-transition-v1",
        "dataset_id": json.loads(new_ci.read_text(encoding="utf-8"))["dataset_id"],
        "known_gaps": {
            "byte_identical": True,
            "sha256": sha256_bytes(new_gap_bytes),
            "rule_count": gaps["rule_count"],
            "resolved_record_count": gaps["resolved_record_count"],
            "known_target_gap_expanded": False,
        },
        "profiles": [
            create_transition_profile(old_ci, new_ci),
            create_transition_profile(old_full, new_full),
        ],
    }


def report_result(summary_path: pathlib.Path) -> None:
    result = json.loads(summary_path.read_text(encoding="utf-8"))
    if result.get("schema") != "vaeg-upd9002-ssts-result-v1":
        raise CorpusError(f"{summary_path}: unknown result schema")
    verify_failure_files(summary_path, result)
    report = {
        "dataset_id": result.get("dataset_id"),
        "profile": result.get("profile"),
        "selected_records": result.get("selected_records"),
        "executed_records": result.get("executed_records"),
        "classification_counts": result.get("classification_counts"),
        "result_counts": result.get("result_counts"),
        "termination_counts": result.get("termination_counts"),
        "failure_signature_count": result.get("failure_signature_count"),
        "failure_signature_index_sha256": result.get(
            "failure_signature_index_sha256"
        ),
        "external_data_skipped": False,
    }
    if any(value is None for value in report.values()):
        raise CorpusError(f"{summary_path}: incomplete result summary")
    print(f"ssts-report: {canonical_bytes(report).decode()}")


def classify_profile(
    dataset_root: pathlib.Path,
    manifest: dict[str, Any],
    support_map_path: pathlib.Path,
    profile: str,
) -> tuple[dict[str, Any], dict[str, Any]]:
    verify_fast(dataset_root, manifest)
    metadata = json.loads(
        (dataset_root / SUITE_PATH / "metadata.json").read_text(encoding="utf-8")
    )
    validate_metadata(metadata)
    support = load_support_map(support_map_path)
    classifications = Counter()
    all_classifications = Counter()
    per_form = []
    known_rules: dict[bytes, dict[str, Any]] = {}
    selection_testsets = []
    selected_total = 0
    for shard_number, path in enumerate(corpus_files(dataset_root), 1):
        relative = path.relative_to(dataset_root).as_posix()
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            records = json.load(stream)
        form = path.name.removesuffix(".json.gz").upper()
        selected = profile_records(records, profile)
        selected_hashes = []
        form_counts = Counter()
        for index, raw_record in enumerate(records):
            record = validate_record(raw_record, f"{relative}[{index}]")
            resolved = classify_record(form, record, metadata, support)
            all_classifications[resolved["classification"]] += 1
        for record in selected:
            resolved = classify_record(form, record, metadata, support)
            classification = resolved["classification"]
            if classification == "unsupported_fixture":
                raise CorpusError(
                    f"{form}:{record['hash']}: empty-queue profile became unsupported"
                )
            classifications[classification] += 1
            form_counts[classification] += 1
            selected_hashes.append(record["hash"])
            if classification == "known_target_gap":
                selector = gap_selector(form, resolved)
                selector_bytes = canonical_bytes(selector)
                rule = known_rules.setdefault(
                    selector_bytes,
                    {
                        "selector": selector,
                        "reason": resolved["reason"],
                        "evidence": {
                            "support_map": support_map_path.as_posix(),
                            "support_classification": "known_target_gap",
                            "metadata_status": resolved["metadata_status"],
                            "metadata_arch": resolved["metadata_arch"],
                        },
                        "resolved_test_hashes": [],
                    },
                )
                rule["resolved_test_hashes"].append(record["hash"])
        selected_hashes.sort()
        selection_testsets.append(
            {
                "form": form,
                "selected_count": len(selected_hashes),
                "selected_test_hashes_sha256": sha256_bytes(
                    canonical_bytes(selected_hashes)
                ),
            }
        )
        per_form.append(
            {
                "form": form,
                "selected_count": len(selected),
                "classification_counts": dict(sorted(form_counts.items())),
            }
        )
        selected_total += len(selected)
        print(
            f"ssts-classify: {shard_number:03d}/{len(corpus_files(dataset_root)):03d} "
            f"{form} selected={len(selected)}",
            file=sys.stderr,
            flush=True,
        )
    selection_digest = sha256_bytes(canonical_bytes(selection_testsets))
    result = {
        "schema": "vaeg-upd9002-ssts-classification-v1",
        "dataset_id": manifest["dataset_id"],
        "dataset_digest": manifest["dataset_digest"],
        "profile": profile,
        "profile_definition": (
            "empty initial queue; stable upstream-hash order; maximum 500 per form"
            if profile == "ci"
            else "all records with an empty initial prefetch queue"
        ),
        "selection_digest": selection_digest,
        "selected_records": selected_total,
        "classification_counts": dict(sorted(classifications.items())),
        "all_corpus_classification_counts": dict(sorted(all_classifications.items())),
        "per_form": per_form,
    }
    rules = []
    for selector_bytes, rule in sorted(known_rules.items()):
        del selector_bytes
        hashes = sorted(rule["resolved_test_hashes"])
        rule["resolved_test_hashes"] = hashes
        rule["resolved_count"] = len(hashes)
        rule["resolved_test_hashes_sha256"] = sha256_bytes(canonical_bytes(hashes))
        rules.append(rule)
    known_gaps = {
        "schema": "vaeg-upd9002-ssts-known-gaps-v1",
        "dataset_id": manifest["dataset_id"],
        "profile": profile,
        "rule_count": len(rules),
        "resolved_record_count": sum(rule["resolved_count"] for rule in rules),
        "rules": rules,
    }
    return result, known_gaps


def create_manifest(dataset_root: pathlib.Path) -> dict[str, Any]:
    metadata_path = dataset_root / SUITE_PATH / "metadata.json"
    license_path = dataset_root / "LICENSE"
    readme_path = dataset_root / "README.md"
    for path in (metadata_path, license_path, readme_path):
        if not path.is_file():
            raise CorpusError(f"required upstream file is missing: {path}")
    metadata_digest = sha256_file(metadata_path)
    license_digest = sha256_file(license_path)
    if metadata_digest != EXPECTED_METADATA_SHA256:
        raise CorpusError(
            f"metadata digest mismatch: expected={EXPECTED_METADATA_SHA256} "
            f"actual={metadata_digest}"
        )
    if license_digest != EXPECTED_LICENSE_SHA256:
        raise CorpusError(
            f"license digest mismatch: expected={EXPECTED_LICENSE_SHA256} "
            f"actual={license_digest}"
        )
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    schema = validate_metadata(metadata)
    if metadata.get("version") != EXPECTED_METADATA_VERSION:
        raise CorpusError(
            f"metadata version changed: expected={EXPECTED_METADATA_VERSION!r} "
            f"actual={metadata.get('version')!r}"
        )
    readme_match = re.search(
        r"^### Current Version: ([^\s]+)$",
        readme_path.read_text(encoding="utf-8"),
        re.MULTILINE,
    )
    if readme_match is None or readme_match.group(1) != EXPECTED_README_VERSION:
        raise CorpusError(
            "README version changed: "
            f"expected={EXPECTED_README_VERSION!r} "
            f"actual={readme_match.group(1) if readme_match else None!r}"
        )

    files = []
    testsets = []
    total_records = 0
    empty_queue_records = 0
    prefetched_records = 0
    for shard_number, path in enumerate(corpus_files(dataset_root), 1):
        relative = path.relative_to(dataset_root).as_posix()
        compressed_digest = sha256_file(path)
        try:
            with gzip.open(path, "rt", encoding="utf-8") as stream:
                records = json.load(stream)
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            raise CorpusError(f"{relative}: cannot decode shard: {error}") from error
        if not isinstance(records, list) or not records:
            raise CorpusError(f"{relative}: expected nonempty record array")
        record_digests = []
        upstream_hashes = set()
        shard_empty = 0
        shard_prefetched = 0
        for index, raw_record in enumerate(records):
            record = validate_record(raw_record, f"{relative}[{index}]")
            if record["hash"] in upstream_hashes:
                raise CorpusError(f"{relative}: duplicate upstream hash {record['hash']}")
            upstream_hashes.add(record["hash"])
            record_digests.append(sha256_bytes(canonical_bytes(record)))
            if record["initial"]["queue"]:
                shard_prefetched += 1
            else:
                shard_empty += 1
        form = path.name.removesuffix(".json.gz").upper()
        testset_content = {
            "form": form,
            "record_digests": sorted(record_digests),
        }
        testset_digest = sha256_bytes(canonical_bytes(testset_content))
        files.append(
            {
                "path": relative,
                "size": path.stat().st_size,
                "sha256": compressed_digest,
            }
        )
        testsets.append(
            {
                "form": form,
                "path": relative,
                "record_count": len(records),
                "empty_queue_count": shard_empty,
                "prefetched_count": shard_prefetched,
                "opcode_testset_digest": testset_digest,
            }
        )
        total_records += len(records)
        empty_queue_records += shard_empty
        prefetched_records += shard_prefetched
        print(
            f"ssts-manifest: {shard_number:03d}/{len(corpus_files(dataset_root)):03d} "
            f"{form} records={len(records)}",
            file=sys.stderr,
            flush=True,
        )

    corpus_digest = sha256_bytes(canonical_bytes(files))
    dataset_content = {
        "identity_policy": IDENTITY_POLICY,
        "metadata_sha256": metadata_digest,
        "corpus_sha256": corpus_digest,
        "opcode_testsets": [
            {
                "form": item["form"],
                "record_count": item["record_count"],
                "opcode_testset_digest": item["opcode_testset_digest"],
            }
            for item in sorted(testsets, key=lambda item: item["form"])
        ],
    }
    dataset_digest = sha256_bytes(canonical_bytes(dataset_content))
    dataset_id = f"ssts-v20-{UPSTREAM_COMMIT}-{dataset_digest}"
    return {
        "schema": "vaeg-upd9002-ssts-manifest-v1",
        "upstream": {
            "repository": UPSTREAM_URL,
            "commit": UPSTREAM_COMMIT,
            "suite": SUITE_PATH,
            "license": "MIT",
            "license_sha256": license_digest,
            "readme_version_informational": EXPECTED_README_VERSION,
            "metadata_version_informational": EXPECTED_METADATA_VERSION,
        },
        "identity_policy": IDENTITY_POLICY,
        "metadata_sha256": metadata_digest,
        "metadata_schema": schema,
        "corpus_sha256": corpus_digest,
        "dataset_digest": dataset_digest,
        "dataset_id": dataset_id,
        "counts": {
            "shards": len(files),
            "records": total_records,
            "empty_queue_records": empty_queue_records,
            "prefetched_records": prefetched_records,
        },
        "files": files,
        "opcode_testsets": sorted(testsets, key=lambda item: item["form"]),
    }


def load_manifest(path: pathlib.Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise CorpusError(f"cannot read manifest {path}: {error}") from error
    if not isinstance(value, dict) or value.get("schema") != (
        "vaeg-upd9002-ssts-manifest-v1"
    ):
        raise CorpusError(f"{path}: unsupported manifest schema")
    return value


def verify_fast(dataset_root: pathlib.Path, expected: dict[str, Any]) -> None:
    upstream = expected.get("upstream")
    if not isinstance(upstream, dict) or (
        upstream.get("repository") != UPSTREAM_URL
        or upstream.get("commit") != UPSTREAM_COMMIT
        or upstream.get("suite") != SUITE_PATH
    ):
        raise CorpusError("manifest upstream identity is not the approved pin")
    if expected.get("identity_policy") != IDENTITY_POLICY:
        raise CorpusError("manifest identity policy differs from the approved policy")
    expected_files = expected.get("files")
    if not isinstance(expected_files, list):
        raise CorpusError("manifest.files: expected array")
    actual_paths = [path.relative_to(dataset_root).as_posix()
                    for path in corpus_files(dataset_root)]
    listed_paths = [item.get("path") for item in expected_files]
    if actual_paths != listed_paths:
        raise CorpusError("corpus shard list differs from manifest")
    for item in expected_files:
        path = dataset_root / item["path"]
        actual_size = path.stat().st_size
        if actual_size != item["size"]:
            raise CorpusError(
                f"{item['path']}: size mismatch expected={item['size']} "
                f"actual={actual_size}"
            )
        actual_digest = sha256_file(path)
        if actual_digest != item["sha256"]:
            raise CorpusError(
                f"{item['path']}: digest mismatch expected={item['sha256']} "
                f"actual={actual_digest}"
            )
    metadata_digest = sha256_file(dataset_root / SUITE_PATH / "metadata.json")
    if metadata_digest != expected.get("metadata_sha256"):
        raise CorpusError("metadata digest differs from manifest")
    license_digest = sha256_file(dataset_root / "LICENSE")
    if license_digest != expected["upstream"].get("license_sha256"):
        raise CorpusError("license digest differs from manifest")


def run_git(arguments: list[str], cwd: pathlib.Path | None = None) -> str:
    try:
        result = subprocess.run(
            ["git", *arguments],
            cwd=cwd,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError) as error:
        detail = getattr(error, "stderr", "") or str(error)
        raise CorpusError(f"git {' '.join(arguments)} failed: {detail.strip()}") from error
    return result.stdout.strip()


def acquire(cache_root: pathlib.Path) -> pathlib.Path:
    cache_root.mkdir(parents=True, exist_ok=True)
    destination = cache_root / f"singlesteptests-v20-{UPSTREAM_COMMIT}"
    if not destination.exists():
        run_git(["clone", "--filter=blob:none", "--no-checkout", UPSTREAM_URL,
                 str(destination)])
    if not (destination / ".git").exists():
        raise CorpusError(f"cache destination is not a Git checkout: {destination}")
    run_git(["fetch", "--depth", "1", "origin", UPSTREAM_COMMIT], destination)
    run_git(["checkout", "--detach", UPSTREAM_COMMIT], destination)
    actual = run_git(["rev-parse", "HEAD"], destination)
    if actual != UPSTREAM_COMMIT:
        raise CorpusError(
            f"checkout identity mismatch: expected={UPSTREAM_COMMIT} actual={actual}"
        )
    if run_git(["status", "--short"], destination):
        raise CorpusError(f"upstream checkout is not clean: {destination}")
    print(f"ssts-acquire: checkout={destination} commit={actual}")
    return destination


def write_json(path: pathlib.Path, value: Any, compact: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if compact:
        content = json.dumps(
            value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
        )
    else:
        content = json.dumps(value, ensure_ascii=True, indent=2, sort_keys=True)
    path.write_text(content + "\n", encoding="utf-8")


def selftest() -> None:
    record = {
        "name": "nop",
        "bytes": [0x90],
        "initial": {
            "regs": {name: 0 for name in REGISTER_KEYS_IN_RECORD},
            "ram": [[0, 0x90]],
            "queue": [],
        },
        "final": {
            "regs": {name: 0 for name in REGISTER_KEYS_IN_RECORD},
            "ram": [[0, 0x90]],
            "queue": [],
        },
        "cycles": [],
        "hash": "0" * 40,
        "idx": 0,
    }
    validate_record(record, "synthetic")
    first = sha256_bytes(canonical_bytes(record))
    second = sha256_bytes(canonical_bytes(dict(reversed(list(record.items())))))
    if first != second:
        raise CorpusError("canonical record digest depends on object key order")
    bad = json.loads(json.dumps(record))
    bad["ignored"] = True
    try:
        validate_record(bad, "synthetic-bad")
    except CorpusError:
        pass
    else:
        raise CorpusError("unknown record field was silently accepted")

    unknown_metadata = {key: "synthetic" for key in ROOT_KEYS}
    unknown_metadata["opcodes"] = {
        "00": {"status": "invalid", "arch": "86"}
    }
    try:
        validate_metadata(unknown_metadata)
    except CorpusError as error:
        if "unknown status" not in str(error):
            raise
    else:
        raise CorpusError("unknown metadata status was silently accepted")

    unclassified_record = json.loads(json.dumps(record))
    unclassified_record["bytes"] = [0x90]
    try:
        classify_record(
            "90",
            unclassified_record,
            {"opcodes": {"90": {"status": "normal", "arch": "86"}}},
            {
                ("upd9002op", 0x00, "-"): {
                    "target": "_add_ea_r8",
                    "classification": "implemented",
                },
            },
        )
    except CorpusError as error:
        if "no M42 support-map row" not in str(error):
            raise
    else:
        raise CorpusError("unclassified dispatch form was silently accepted")

    legacy_support = {
        ("v30op", 0x00, "-"): {
            "target": "_add_ea_r8",
            "classification": "implemented",
        },
    }
    folded_support = {
        ("upd9002op", 0x00, "-"): {
            "target": "_add_ea_r8",
            "classification": "implemented",
        },
    }
    for expected_mode, support_map in (
        ("v30op", legacy_support),
        ("upd9002op", folded_support),
    ):
        classified = classify_record(
            "00",
            {"bytes": [0x00], "initial": {"queue": []}},
            {"opcodes": {"00": {"status": "normal", "arch": "86"}}},
            support_map,
        )
        dispatch = classified.get("dispatch") or {}
        if dispatch.get("support_mode") != expected_mode:
            raise CorpusError(
                f"support-map family resolver selected {dispatch.get('support_mode')}"
            )

    for expected_mode, support_map in (
        (
            "v30op",
            {
                ("v30op", 0x00, "-"): {
                    "target": "_add_ea_r8",
                    "classification": "implemented",
                },
                ("v30op", 0x64, "-"): {
                    "target": "prefix_0x64",
                    "classification": "known_target_gap",
                },
            },
        ),
        (
            "upd9002op_repnc",
            {
                ("upd9002op", 0x00, "-"): {
                    "target": "_add_ea_r8",
                    "classification": "implemented",
                },
                ("upd9002op_repnc", 0xA4, "-"): {
                    "target": "_movsb",
                    "classification": "implemented",
                },
            },
        ),
    ):
        classified = classify_record(
            "A4",
            {"bytes": [0x64, 0xA4], "initial": {"queue": []}},
            {"opcodes": {"A4": {"status": "normal", "arch": "86"}}},
            support_map,
        )
        dispatch = classified.get("dispatch") or {}
        if dispatch.get("support_mode") != expected_mode:
            raise CorpusError(
                f"REPNC support-map family resolver selected {dispatch.get('support_mode')}"
            )

    override_record = json.loads(json.dumps(record))
    override_record["name"] = "segment-overridden outsb"
    override_record["bytes"] = [0x36, 0x6E]
    override_record["initial"]["regs"].update(
        {"ds": 0x1000, "ss": 0x2000, "si": 0x0010}
    )
    override_record["initial"]["ram"] = [[0x10010, 0x5A]]
    override_record["cycles"] = [
        [1, 0x20010, "--", "---", "---", 0, 0, "MEMR", "T1", "-", 0],
        [0, 0, "SS", "R--", "---", 0, 0, "MEMR", "T2", "-", 0],
        [0, 0, "SS", "R--", "---", 0, 0x5A, "PASV", "T3", "-", 0],
    ]
    translated = dict(translated_initial_ram(override_record))
    if translated.get(0x10010) != 0x5A or translated.get(0x20010) != 0x5A:
        raise CorpusError("segment-override source translation did not mirror data")
    conflicting_override = json.loads(json.dumps(override_record))
    conflicting_override["initial"]["ram"][0][1] = 0xA5
    try:
        translated_initial_ram(conflicting_override)
    except CorpusError as error:
        if "default source does not match" not in str(error):
            raise
    else:
        raise CorpusError("segment-override source mismatch was silently invented")

    with tempfile.TemporaryDirectory(prefix="vaeg-ssts-selftest-") as temporary:
        root = pathlib.Path(temporary)
        suite = root / SUITE_PATH
        suite.mkdir()
        shard = suite / "00.json.gz"
        shard.write_bytes(b"not-a-gzip-stream")
        (suite / "metadata.json").write_text("{}\n", encoding="utf-8")
        (root / "LICENSE").write_text("synthetic\n", encoding="utf-8")
        synthetic_manifest = {
            "upstream": {
                "repository": UPSTREAM_URL,
                "commit": UPSTREAM_COMMIT,
                "suite": SUITE_PATH,
                "license_sha256": sha256_file(root / "LICENSE"),
            },
            "identity_policy": IDENTITY_POLICY,
            "metadata_sha256": sha256_file(suite / "metadata.json"),
            "files": [
                {
                    "path": f"{SUITE_PATH}/00.json.gz",
                    "size": shard.stat().st_size,
                    "sha256": "0" * 64,
                }
            ],
        }
        try:
            verify_fast(root, synthetic_manifest)
        except CorpusError as error:
            if "digest mismatch" not in str(error):
                raise
        else:
            raise CorpusError("corpus digest mismatch was silently accepted")

        summary = root / "synthetic.json"
        failures = root / "synthetic_failures"
        result = {
            "schema": "vaeg-upd9002-ssts-result-v1",
            "dataset_id": "synthetic",
            "profile": "ci",
            "selected_records": 1,
            "executed_records": 1,
            "classification_counts": {"applicable": 1},
            "result_counts": {"semantic_failure": 1},
            "termination_counts": {"normal": 1},
            "failure_signature_count": 1,
            "failure_signatures": [
                {
                    "content": {
                        "record_hash": "0" * 40,
                        "opcode_form": "90",
                    },
                    "signature_sha256": "0" * 64,
                }
            ],
        }
        externalize_failure_signatures(result, failures)
        write_json(summary, result, compact=True)
        report_result(summary)
    print(
        "ssts-selftest: canonical digest, fail-closed schema/status/form, and "
        "digest rejection passed"
    )


def worker_selftest(worker: pathlib.Path) -> None:
    registers = {name: 0 for name in REGISTER_KEYS_IN_RECORD}
    registers.update({"cs": 0, "ip": 0x0100, "flags": 0xf002})
    record = {
        "name": "synthetic hlt termination",
        "bytes": [0xf4],
        "initial": {
            "regs": registers,
            "ram": [[0x0100, 0xf4]],
            "queue": [],
        },
        "final": {
            "regs": registers,
            "ram": [[0x0100, 0xf4]],
            "queue": [],
        },
        "cycles": [],
        "hash": "0" * 40,
        "idx": 0,
    }
    outs_registers = dict(registers)
    outs_registers.update(
        {"dx": 0x1234, "ds": 0x1000, "ss": 0x2000, "si": 0x0010}
    )
    outs_record = {
        "name": "synthetic segment-overridden outsb",
        "bytes": [0x36, 0x6E],
        "initial": {
            "regs": outs_registers,
            "ram": [[0x0100, 0x36], [0x0101, 0x6E], [0x10010, 0x5A]],
            "queue": [],
        },
        "final": {
            "regs": {"si": 0x0011, "ip": 0x0102},
            "ram": [],
            "queue": [],
        },
        "cycles": [
            [1, 0x20010, "--", "---", "---", 0, 0, "MEMR", "T1", "-", 0],
            [0, 0, "SS", "R--", "---", 0, 0, "MEMR", "T2", "-", 0],
            [0, 0, "SS", "R--", "---", 0, 0x5A, "PASV", "T3", "-", 0],
        ],
        "hash": "1" * 40,
        "idx": 1,
    }
    int_registers = dict(registers)
    int_record = {
        "name": "synthetic explicit int 00",
        "bytes": [0xCD, 0x00],
        "initial": {
            "regs": int_registers,
            "ram": [
                [0x0000, 0x34], [0x0001, 0x12],
                [0x0002, 0x78], [0x0003, 0x56],
                [0x0100, 0xCD], [0x0101, 0x00],
            ],
            "queue": [],
        },
        "final": {"regs": {}, "ram": [], "queue": []},
        "cycles": [],
        "hash": "2" * 40,
        "idx": 2,
    }
    divide_registers = dict(registers)
    divide_registers["ax"] = 0
    divide_record = {
        "name": "synthetic divide error",
        "bytes": [0xF6, 0xF0],
        "initial": {
            "regs": divide_registers,
            "ram": [
                [0x0000, 0x34], [0x0001, 0x12],
                [0x0002, 0x78], [0x0003, 0x56],
                [0x0100, 0xF6], [0x0101, 0xF0],
            ],
            "queue": [],
        },
        "final": {"regs": {}, "ram": [], "queue": []},
        "cycles": [],
        "hash": "3" * 40,
        "idx": 3,
    }
    target_registers = dict(registers)
    target_record = {
        "name": "synthetic normal completion at IVT0 target",
        "bytes": [0x90],
        "initial": {
            "regs": target_registers,
            "ram": [
                [0x0000, 0x01], [0x0001, 0x01],
                [0x0002, 0x00], [0x0003, 0x00],
                [0x0100, 0x90],
            ],
            "queue": [],
        },
        "final": {"regs": {}, "ram": [], "queue": []},
        "cycles": [],
        "hash": "4" * 40,
        "idx": 4,
    }
    validate_record(record, "synthetic-worker")
    validate_record(outs_record, "synthetic-worker-override")
    validate_record(int_record, "synthetic-worker-int00")
    validate_record(divide_record, "synthetic-worker-divide")
    validate_record(target_record, "synthetic-worker-ivt0-target")
    status, results = run_worker_once(
        worker,
        [record, outs_record, int_record, divide_record, target_record],
        30.0,
    )
    if status != "ok" or results is None or len(results) != 5:
        raise CorpusError(f"synthetic worker failed with status {status}")
    actual = results[0]
    if actual["termination"] != "halt":
        raise CorpusError(
            "synthetic HLT was not classified as halt: "
            f"{actual['termination']}"
        )
    if actual["registers"]["cs"] != 0 or actual["registers"]["ip"] != 0x0100:
        raise CorpusError("synthetic HLT did not retain its restart address")
    outs_actual = results[1]
    if outs_actual["registers"]["si"] != 0x0011 or outs_actual["io"] != [
        {"direction": "write", "port": 0x1234, "value": 0x5A}
    ]:
        raise CorpusError(
            "segment-overridden OUTSB did not use the translated source byte"
        )
    int_actual = results[2]
    if int_actual["termination"] != "normal" or int_actual["execution_result"] != {
        "kind": "software_interrupt",
        "interrupt_count": 1,
        "interrupt_vector": 0,
        "termination": "normal",
    }:
        raise CorpusError("explicit INT 00 was not classified as a software interrupt")
    divide_actual = results[3]
    if divide_actual["termination"] != "type0" or divide_actual["execution_result"] != {
        "kind": "divide_error",
        "interrupt_count": 1,
        "interrupt_vector": 0,
        "termination": "type0",
    }:
        raise CorpusError("F6 divide error was not classified as type0")
    target_actual = results[4]
    if target_actual["registers"]["ip"] != 0x0101 or target_actual[
        "execution_result"
    ] != {
        "kind": "normal",
        "interrupt_count": 0,
        "interrupt_vector": None,
        "termination": "normal",
    }:
        raise CorpusError("ordinary completion at the IVT0 target was misclassified")
    print(
        "ssts-worker-selftest: HLT, segment override, INT 00, divide error, "
        "and IVT0-target completion passed"
    )


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    acquire_parser = subparsers.add_parser(
        "acquire", help="acquire the exact pinned upstream commit"
    )
    acquire_parser.add_argument("--cache-root", required=True, type=pathlib.Path)
    manifest = subparsers.add_parser("manifest", help="create a full manifest")
    manifest.add_argument("--dataset-root", required=True, type=pathlib.Path)
    manifest.add_argument("--output", required=True, type=pathlib.Path)
    verify = subparsers.add_parser("verify", help="verify pinned compressed bytes")
    verify.add_argument("--dataset-root", required=True, type=pathlib.Path)
    verify.add_argument("--manifest", required=True, type=pathlib.Path)
    classify = subparsers.add_parser(
        "classify", help="classify a deterministic comparison profile"
    )
    classify.add_argument("--dataset-root", required=True, type=pathlib.Path)
    classify.add_argument("--manifest", required=True, type=pathlib.Path)
    classify.add_argument("--support-map", required=True, type=pathlib.Path)
    classify.add_argument("--profile", required=True, choices=("ci", "full"))
    classify.add_argument("--output", required=True, type=pathlib.Path)
    classify.add_argument("--known-gaps", required=True, type=pathlib.Path)
    run = subparsers.add_parser("run", help="execute a comparison profile")
    run.add_argument("--dataset-root", required=True, type=pathlib.Path)
    run.add_argument("--manifest", required=True, type=pathlib.Path)
    run.add_argument("--support-map", required=True, type=pathlib.Path)
    run.add_argument("--worker", required=True, type=pathlib.Path)
    run.add_argument("--profile", required=True, choices=("ci", "full"))
    run.add_argument(
        "--flags-comparison",
        choices=("defined", "all16"),
        default="defined",
        help="compare metadata-defined FLAGS bits or all 16 V20 fingerprint bits",
    )
    run.add_argument("--shard-timeout", type=float, default=120.0)
    run.add_argument("--output", required=True, type=pathlib.Path)
    run.add_argument("--failure-directory", type=pathlib.Path)
    run.add_argument("--expect", type=pathlib.Path)
    report = subparsers.add_parser(
        "report", help="verify and print a canonical result summary"
    )
    report.add_argument("--summary", required=True, type=pathlib.Path)
    transition = subparsers.add_parser(
        "transition", help="record a complete corrected-baseline transition"
    )
    transition.add_argument("--old-ci", required=True, type=pathlib.Path)
    transition.add_argument("--old-full", required=True, type=pathlib.Path)
    transition.add_argument("--new-ci", required=True, type=pathlib.Path)
    transition.add_argument("--new-full", required=True, type=pathlib.Path)
    transition.add_argument("--old-known-gaps", required=True, type=pathlib.Path)
    transition.add_argument("--new-known-gaps", required=True, type=pathlib.Path)
    transition.add_argument("--output", required=True, type=pathlib.Path)
    skip = subparsers.add_parser(
        "external-skip", help="report an unavailable external-data comparison"
    )
    skip.add_argument("--reason", required=True)
    subparsers.add_parser("selftest", help="run synthetic parser tests")
    worker_test = subparsers.add_parser(
        "worker-selftest", help="run synthetic execution-worker tests"
    )
    worker_test.add_argument("--worker", required=True, type=pathlib.Path)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "manifest":
            value = create_manifest(arguments.dataset_root.resolve())
            write_json(arguments.output, value)
            print(
                "ssts-manifest: "
                f"dataset_id={value['dataset_id']} "
                f"records={value['counts']['records']}"
            )
        elif arguments.command == "acquire":
            acquire(arguments.cache_root.resolve())
        elif arguments.command == "verify":
            value = load_manifest(arguments.manifest)
            verify_fast(arguments.dataset_root.resolve(), value)
            print(
                "ssts-verify: verified "
                f"dataset_id={value['dataset_id']} files={len(value['files'])}"
            )
        elif arguments.command == "report":
            report_result(arguments.summary.resolve())
        elif arguments.command == "transition":
            value = create_transition_report(
                arguments.old_ci.resolve(),
                arguments.old_full.resolve(),
                arguments.new_ci.resolve(),
                arguments.new_full.resolve(),
                arguments.old_known_gaps.resolve(),
                arguments.new_known_gaps.resolve(),
            )
            write_json(arguments.output, value)
            counts = {
                profile["profile"]: {
                    "removed": len(profile["removed_from_semantic_failure"]),
                    "entered": len(profile["newly_entering_semantic_failure"]),
                    "changed": len(profile["changed_failure_signatures"]),
                }
                for profile in value["profiles"]
            }
            print(f"ssts-transition: {canonical_bytes(counts).decode()}")
        elif arguments.command == "external-skip":
            print(
                "ssts-report: "
                + canonical_bytes(
                    {
                        "external_data_skipped": True,
                        "reason": arguments.reason,
                    }
                ).decode()
            )
            return 77
        elif arguments.command == "classify":
            manifest = load_manifest(arguments.manifest)
            result, known_gaps = classify_profile(
                arguments.dataset_root.resolve(),
                manifest,
                arguments.support_map.resolve(),
                arguments.profile,
            )
            write_json(arguments.output, result)
            write_json(arguments.known_gaps, known_gaps)
            print(
                "ssts-classify: "
                f"dataset_id={result['dataset_id']} profile={result['profile']} "
                f"selected={result['selected_records']} "
                f"categories={canonical_bytes(result['classification_counts']).decode()}"
            )
        elif arguments.command == "run":
            if arguments.shard_timeout <= 0:
                raise CorpusError("shard timeout must be positive")
            if arguments.flags_comparison == "all16" and arguments.expect is not None:
                raise CorpusError(
                    "--expect is unavailable for the diagnostic all16 profile"
                )
            manifest = load_manifest(arguments.manifest)
            result = run_profile(
                arguments.dataset_root.resolve(),
                manifest,
                arguments.support_map.resolve(),
                arguments.worker.resolve(),
                arguments.profile,
                arguments.shard_timeout,
                arguments.flags_comparison,
            )
            if arguments.failure_directory is not None:
                externalize_failure_signatures(
                    result, arguments.failure_directory.resolve()
                )
            write_json(arguments.output, result, compact=True)
            if arguments.expect is not None:
                expected = json.loads(arguments.expect.read_text(encoding="utf-8"))
                verify_failure_files(arguments.expect, expected)
                if result != expected:
                    raise CorpusError(
                        f"{arguments.profile} result differs from {arguments.expect}"
                    )
            print(
                "ssts-result: "
                f"dataset_id={result['dataset_id']} profile={result['profile']} "
                f"selected={result['selected_records']} "
                f"executed={result['executed_records']} "
                f"results={canonical_bytes(result['result_counts']).decode()}"
            )
        elif arguments.command == "selftest":
            selftest()
        elif arguments.command == "worker-selftest":
            worker_selftest(arguments.worker.resolve())
        else:
            raise AssertionError(arguments.command)
    except (CorpusError, OSError) as error:
        print(f"ssts-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

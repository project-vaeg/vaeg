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
"""Audit, generate, and verify the M60e real-mode IRET evidence."""

from __future__ import annotations

import argparse
import copy
import gzip
import hashlib
import json
import pathlib
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import zlib
from collections import Counter, defaultdict
from collections.abc import Callable, Iterable
from typing import Any

import upd9002_m60b_authority as m60b
import upd9002_m60c_audit as m60c
import upd9002_m60d_frame_audit as m60d
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M60e"
CANDIDATE_GATE = "G60e"
APPROVED_PREDECESSOR_GATE = "G60d"
# Evidence keeps the original identity; Git topology starts at the current
# main-history checkpoint established after the protected-history rewrite.
APPROVED_PREDECESSOR_SHA = "8736f8afe6d8eeb58e58c7afdaf5951e2306cb63"
APPROVED_PREDECESSOR_GIT_SHA = "a187d14427b7532e16487653f8ffb6fe37c9703d"
G60D_EVALUATED_SHA = "ada55de79751c04e44d02abf7ecd6851b55c9763"
G60D_CI_URL = "https://github.com/nakatamaho/vaeg/actions/runs/30155594048"
TARGET_POLICY_ID = m60d.TARGET_POLICY_ID
TARGET_POLICY_SHA256 = m60d.TARGET_POLICY_SHA256
DATASET_ID = m60d.DATASET_ID
CONTRACTS = m60d.CONTRACTS
SELECTED_HASH_SETS = m60d.SELECTED_HASH_SETS
APPLICABLE_HASH_SETS = m60d.APPLICABLE_HASH_SETS
BEFORE_PROFILE_IDENTITIES = m60d.PROFILE_IDENTITIES
EMPTY_HASH_SET_SHA256 = ratchet.hash_set_digest([])
DATASET_MANIFEST_PATH = pathlib.Path("tests/ssts/v20_dataset_manifest.json")
G60D_RESULT_MANIFEST_PATH = pathlib.Path(
    "tests/ssts/evidence/g60d_result_manifest.json"
)
G60D_RESULT_MANIFEST_SHA256 = (
    "fb5f58d80de03b7829415d93b81041e44982c4fb2f1474929338be215e9dd43c"
)
G60D_EVIDENCE_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g60d/manifest.json")
G60D_EVIDENCE_MANIFEST_SHA256 = (
    "5e6e6d4a6946c19bfad59f32fce5dfded345881f17977f8f4add6b011a32c69d"
)
G60D_SCOREBOARD_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/scoreboard/g60d_architectural_ci.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/scoreboard/g60d_architectural_full.json"
    ),
    "fingerprint_full": pathlib.Path(
        "tests/ssts/scoreboard/g60d_fingerprint_full.json"
    ),
}
SCOREBOARD_PATHS = {
    key: pathlib.Path(str(path).replace("g60d_", "g60e_"))
    for key, path in G60D_SCOREBOARD_PATHS.items()
}
FAILURE_DIRECTORY_PATHS = {
    key: pathlib.Path(str(path).removesuffix(".json") + "_failures")
    for key, path in SCOREBOARD_PATHS.items()
}
TRANSITION_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/transitions/g60e_architectural_ci_from_g60d.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/transitions/g60e_architectural_full_from_g60d.json"
    ),
}
EVIDENCE_ROOT = pathlib.Path("tests/ssts/evidence/g60e")
RESULT_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g60e_result_manifest.json")
RANKING_JSON_PATH = pathlib.Path(
    "tests/ssts/rankings/g60e_architectural_full.json"
)
RANKING_MD_PATH = pathlib.Path(
    "tests/ssts/rankings/g60e_architectural_full.md"
)
REPORT_PATH = pathlib.Path("docs/agents/reports/m60e_upd9002_iret.md")
EXPECTED_CF_SELECTED = 5000
EXPECTED_CF_PASS_BEFORE = 1231
EXPECTED_CF_FAILURE_BEFORE = 3769
EXPECTED_CF_PASS_AFTER = 5000
EXPECTED_CF_FAILURE_AFTER = 0
IRET_STACK_MASK = 0x0FD7
IRET_FORCED_BITS = 0xF002
VALID_RULES = {
    "loadable",
    "preserved",
    "forced-zero",
    "forced-one",
    "condition-dependent",
    "undetermined",
}
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")


class M60eError(RuntimeError):
    """A fail-closed M60e validation failure with a stable reason code."""

    def __init__(self, code: str, message: str):
        super().__init__(f"{code}: {message}")
        self.code = code


def reject(code: str, message: str) -> None:
    raise M60eError(code, message)


def validate_candidate_scoreboard(value: dict[str, Any]) -> None:
    """Validate a v2 scoreboard against the M60e epoch identity."""

    previous = (
        m60b.APPROVED_PREDECESSOR_GATE,
        m60b.APPROVED_PREDECESSOR_SHA,
        m60b.CANDIDATE_GATE,
    )
    try:
        m60b.APPROVED_PREDECESSOR_GATE = APPROVED_PREDECESSOR_GATE
        m60b.APPROVED_PREDECESSOR_SHA = APPROVED_PREDECESSOR_SHA
        m60b.CANDIDATE_GATE = CANDIDATE_GATE
        m60b.validate_scoreboard_v2(value)
    finally:
        (
            m60b.APPROVED_PREDECESSOR_GATE,
            m60b.APPROVED_PREDECESSOR_SHA,
            m60b.CANDIDATE_GATE,
        ) = previous


def require(condition: bool, code: str, message: str) -> None:
    if not condition:
        reject(code, message)


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


def read_json(path: pathlib.Path) -> Any:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value) + b"\n")


def output_path(output_root: pathlib.Path, relative: pathlib.Path) -> pathlib.Path:
    return output_root / relative


def require_sha(value: Any, where: str, pattern: re.Pattern[str] = HEX64) -> str:
    require(
        isinstance(value, str) and pattern.fullmatch(value) is not None,
        "malformed-hash",
        where,
    )
    return value


def hex_registers(registers: dict[str, int]) -> dict[str, str]:
    return {name: f"{registers[name]:04x}" for name in ssts.REGISTER_ORDER}


def ram_rows(memory: dict[int, int]) -> list[dict[str, str]]:
    return [
        {"address": f"{address:05x}", "value": f"{memory[address]:02x}"}
        for address in sorted(memory)
    ]


def load_scoreboard_failures(
    root: pathlib.Path, summary_path: pathlib.Path
) -> dict[str, dict[str, Any]]:
    summary = read_json(root / summary_path)
    failures = ratchet.load_scoreboard_failures(root / summary_path, summary)
    return failures


def verify_predecessor(root: pathlib.Path) -> None:
    require(
        sha256_file(root / G60D_EVIDENCE_MANIFEST_PATH)
        == G60D_EVIDENCE_MANIFEST_SHA256,
        "protected-artifact-mutation",
        "G60d evidence manifest differs",
    )
    result = read_json(root / G60D_RESULT_MANIFEST_PATH)
    require(
        result.get("analysis_evaluated_sha") == G60D_EVALUATED_SHA,
        "wrong-predecessor-evaluated-sha",
        "G60d evaluated SHA differs",
    )
    require(
        result.get("candidate_gate") == "G60d",
        "wrong-predecessor-gate",
        "G60d result manifest gate differs",
    )
    require(
        result.get("evidence_manifest_sha256")
        == G60D_EVIDENCE_MANIFEST_SHA256,
        "protected-artifact-mutation",
        "G60d result-to-manifest binding differs",
    )
    # The report-approved digest is the direct artifact identity.  The result
    # manifest itself is checked canonically here without changing history.
    require(
        (root / G60D_RESULT_MANIFEST_PATH).read_bytes()
        == canonical_bytes(result) + b"\n",
        "nondeterministic-json",
        "G60d result manifest is not canonical",
    )
    try:
        m60c.erratum.verify_static(root)
        m60d.validate_manifest(root)
    except (m60c.erratum.ErratumError, m60d.M60dError) as error:
        reject("protected-artifact-mutation", str(error))


def verify_raw_predecessor_profile(
    root: pathlib.Path, raw_path: pathlib.Path, profile_key: str
) -> dict[str, Any]:
    try:
        raw, _, _ = m60d.verify_raw_profile(root, raw_path, profile_key)
    except m60d.M60dError as error:
        reject("predecessor-profile-drift", str(error))
    return raw


def expected_stack(
    record: dict[str, Any],
) -> tuple[list[int], list[int], list[int], list[int]]:
    regs = record["initial"]["regs"]
    ss = regs["ss"]
    sp = regs["sp"]
    offsets = [((sp + index) & 0xFFFF) for index in range(6)]
    physical = [(((ss << 4) + offset) & 0xFFFFF) for offset in offsets]
    initial_memory = {address: value for address, value in record["initial"]["ram"]}
    require(
        all(address in initial_memory for address in physical),
        "missing-stack-byte",
        record["hash"],
    )
    values = [initial_memory[address] for address in physical]
    words = [
        values[index] | (values[index + 1] << 8)
        for index in (0, 2, 4)
    ]
    return offsets, physical, values, words


def boundary_partition(
    ss: int, offsets: list[int], physical: list[int]
) -> str:
    segment_wrap = offsets[0] + 5 > 0xFFFF
    linear = [(ss << 4) + offset for offset in offsets]
    physical_wrap = any(value > 0xFFFFF for value in linear)
    if segment_wrap and physical_wrap:
        return "segment-and-physical-wrap"
    if segment_wrap:
        return "segment-offset-wrap"
    if physical_wrap:
        return "physical-wrap"
    require(
        physical == [value & 0xFFFFF for value in linear],
        "physical-address-derivation",
        "physical address derivation differs",
    )
    return "ordinary"


def make_case_row(
    record: dict[str, Any],
    resolved: dict[str, Any],
    status: str,
    actual: dict[str, Any] | None,
) -> dict[str, Any]:
    require(status == "ok" and actual is not None, "worker-result", record["hash"])
    expected_regs = ssts.expected_registers(record)
    watch, expected_ram = ssts.expected_memory(record)
    record_hash = ssts.sha256_bytes(ssts.canonical_bytes(record))
    context = {
        "record": record,
        "record_digest": record_hash,
        "watch": watch,
        "expected_ram": expected_ram,
    }
    failure = ssts.make_failure(
        DATASET_ID,
        "full",
        "CF",
        resolved["classification"],
        resolved["flags_mask"],
        context,
        status,
        actual,
    )["content"]
    offsets, physical, stack_bytes, words = expected_stack(record)
    initial_regs = record["initial"]["regs"]
    partition = boundary_partition(initial_regs["ss"], offsets, physical)
    expected_words = [
        expected_regs["ip"],
        expected_regs["cs"],
        expected_regs["flags"],
    ]
    actual_words = [
        actual["registers"]["ip"],
        actual["registers"]["cs"],
        actual["registers"]["flags"],
    ]
    unrelated = [
        name for name in ssts.REGISTER_ORDER
        if name not in {"ip", "cs", "sp", "flags"}
    ]
    prefix_bytes = []
    for byte in record["bytes"]:
        if byte == 0xCF:
            break
        prefix_bytes.append(byte)
    require(
        0xCF in record["bytes"],
        "first-byte-only-selector",
        "CF selector did not resolve the final opcode",
    )
    return {
        "actual_final_ram": ram_rows(actual["ram"]),
        "actual_logical_stack_addresses": [
            f"{initial_regs['ss']:04x}:{offset:04x}" for offset in offsets
        ],
        "actual_physical_stack_addresses": [
            f"{address:05x}" for address in physical
        ],
        "actual_registers": hex_registers(actual["registers"]),
        "actual_restored_words": {
            name: f"{value:04x}"
            for name, value in zip(("ip", "cs", "flags"), actual_words)
        },
        "actual_termination": actual["execution_result"]["termination"],
        "architectural_outcome": (
            "pass" if not failure["mismatch_kinds"] else "fail"
        ),
        "boundary_partition": partition,
        "case_hash": record_hash,
        "conclusion_status": "proven",
        "evidence_notes": (
            "Final-state reconstruction proves the consumed values and final "
            "SP; it does not claim transient bus read ordering."
        ),
        "executed": True,
        "expected_final_ram": ram_rows(expected_ram),
        "expected_logical_stack_addresses": [
            f"{initial_regs['ss']:04x}:{offset:04x}" for offset in offsets
        ],
        "expected_physical_stack_addresses": [
            f"{address:05x}" for address in physical
        ],
        "expected_registers": hex_registers(expected_regs),
        "expected_restored_words": {
            name: f"{value:04x}"
            for name, value in zip(("ip", "cs", "flags"), expected_words)
        },
        "expected_termination": ssts.expected_termination("CF", record),
        "initial_registers": hex_registers(initial_regs),
        "instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "mismatch_kinds": failure["mismatch_kinds"],
        "prefix_sequence": [f"{byte:02x}" for byte in prefix_bytes],
        "profile": "architectural",
        "scope": "full",
        "selected": True,
        "stack_bytes": [f"{value:02x}" for value in stack_bytes],
        "stack_words_little_endian": {
            name: f"{value:04x}"
            for name, value in zip(("ip", "cs", "flags"), words)
        },
        "top_level_classification": resolved["classification"],
        "unrelated_registers": {
            "actual": {
                name: f"{actual['registers'][name]:04x}" for name in unrelated
            },
            "expected": {
                name: f"{expected_regs[name]:04x}" for name in unrelated
            },
        },
        "upstream_case_hash": record["hash"],
    }


def derive_rule(rows: list[dict[str, Any]], field: str, bit: int) -> dict[str, Any]:
    stack_values = [
        (int(row["stack_words_little_endian"]["flags"], 16) >> bit) & 1
        for row in rows
    ]
    initial_values = [
        (int(row["initial_registers"]["flags"], 16) >> bit) & 1
        for row in rows
    ]
    output_values = [
        (int(row[field]["flags"], 16) >> bit) & 1 for row in rows
    ]
    load_matches = sum(left == right for left, right in zip(output_values, stack_values))
    preserve_matches = sum(
        left == right for left, right in zip(output_values, initial_values)
    )
    if load_matches == len(rows) and len(set(stack_values)) == 2:
        rule = "loadable"
    elif not any(output_values) and any(stack_values):
        rule = "forced-zero"
    elif all(output_values) and not all(stack_values):
        rule = "forced-one"
    elif preserve_matches == len(rows) and any(
        left != right for left, right in zip(stack_values, initial_values)
    ):
        rule = "preserved"
    elif len(set(output_values)) > 1:
        rule = "condition-dependent"
    else:
        rule = "undetermined"
    representatives = []
    for desired in (0, 1):
        for row, stack, initial, output in zip(
            rows, stack_values, initial_values, output_values
        ):
            if output == desired:
                representatives.append(
                    {
                        "case_hash": row["case_hash"],
                        "initial": initial,
                        "output": output,
                        "stack": stack,
                    }
                )
                break
    return {
        "bit": bit,
        "counterexample_count": len(rows) - (
            load_matches if rule == "loadable" else preserve_matches
        ),
        "representative_cases": representatives,
        "rule": rule,
        "supporting_case_count": (
            load_matches
            if rule == "loadable"
            else (
                preserve_matches
                if rule == "preserved"
                else sum(value == output_values[0] for value in output_values)
            )
        ),
    }


def derive_flags_rules(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    rules = []
    for bit in range(16):
        expected = derive_rule(rows, "expected_restored_words", bit)
        actual = derive_rule(rows, "actual_restored_words", bit)
        require(
            expected["rule"] in VALID_RULES and actual["rule"] in VALID_RULES,
            "unsupported-flags-rule",
            f"bit {bit}",
        )
        rules.append(
            {
                "actual_rule": actual["rule"],
                "architectural_relevance": bit in {0, 2, 4, 6, 7, 8, 9, 10, 11},
                "bit": bit,
                "counterexample_count": expected["counterexample_count"],
                "expected_rule": expected["rule"],
                "fingerprint_relevance": True,
                "representative_cases": expected["representative_cases"],
                "supporting_case_count": expected["supporting_case_count"],
            }
        )
    return rules


def run_cf_audit(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    expected_phase: str,
) -> dict[str, Any]:
    require(
        expected_phase in {"pre-fix", "post-fix"},
        "invalid-audit-phase",
        expected_phase,
    )
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    require(
        manifest["dataset_id"] == DATASET_ID,
        "dataset-drift",
        "dataset identity differs",
    )
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    corpus_path = dataset_root / ssts.SUITE_PATH / "CF.json.gz"
    with gzip.open(corpus_path, "rt", encoding="utf-8") as stream:
        records = json.load(stream)
    selected = ssts.profile_records(records, "full")
    require(
        len(selected) == EXPECTED_CF_SELECTED,
        "incomplete-cf-population",
        f"selected={len(selected)}",
    )
    with m60b.candidate_support_map(root) as support_path:
        support = ssts.load_support_map(support_path)
        resolved_rows = [
            ssts.classify_record("CF", record, metadata, support)
            for record in selected
        ]
        require(
            all(row["classification"] == "applicable" for row in resolved_rows),
            "cf-classification",
            "CF is not completely applicable",
        )
        results = ssts.run_worker_contained(worker, selected, 120.0)
    require(
        len(results) == len(selected),
        "missing-executed-cf-record",
        "worker result count differs",
    )
    rows = [
        make_case_row(record, resolved, status, actual)
        for record, resolved, (status, actual) in zip(
            selected, resolved_rows, results
        )
    ]
    rows.sort(key=lambda row: row["case_hash"])
    require(
        len({row["case_hash"] for row in rows}) == len(rows),
        "duplicate-case-hash",
        "CF table contains duplicates",
    )
    pass_hashes = [
        row["case_hash"] for row in rows if row["architectural_outcome"] == "pass"
    ]
    failure_hashes = [
        row["case_hash"] for row in rows if row["architectural_outcome"] == "fail"
    ]
    before_failures = load_scoreboard_failures(
        root, G60D_SCOREBOARD_PATHS["architectural_full"]
    )
    approved_cf_failures = sorted(
        record_hash
        for record_hash, value in before_failures.items()
        if value["form"] == "CF"
    )
    require(
        len(approved_cf_failures) == EXPECTED_CF_FAILURE_BEFORE,
        "predecessor-cf-count",
        f"approved failures={len(approved_cf_failures)}",
    )
    if expected_phase == "pre-fix":
        require(
            len(pass_hashes) == EXPECTED_CF_PASS_BEFORE
            and failure_hashes == approved_cf_failures,
            "pre-fix-cf-result",
            f"pass={len(pass_hashes)} fail={len(failure_hashes)}",
        )
    else:
        require(
            len(pass_hashes) == EXPECTED_CF_PASS_AFTER and not failure_hashes,
            "post-fix-cf-result",
            f"pass={len(pass_hashes)} fail={len(failure_hashes)}",
        )
    for row in rows:
        words = row["stack_words_little_endian"]
        expected = row["expected_restored_words"]
        require(words["ip"] == expected["ip"], "wrong-stack-word-order", row["case_hash"])
        require(words["cs"] == expected["cs"], "ip-cs-swap", row["case_hash"])
        stack_flags = int(words["flags"], 16)
        derived_flags = (stack_flags & IRET_STACK_MASK) | IRET_FORCED_BITS
        require(
            derived_flags == int(expected["flags"], 16),
            "independent-flags-rule",
            row["case_hash"],
        )
        require(
            int(row["expected_registers"]["sp"], 16)
            == ((int(row["initial_registers"]["sp"], 16) + 6) & 0xFFFF),
            "wrong-sp-increment",
            row["case_hash"],
        )
    flags_rules = derive_flags_rules(rows)
    boundary_counts = dict(
        sorted(Counter(row["boundary_partition"] for row in rows).items())
    )
    return {
        "approved_cf_failure_hashes": approved_cf_failures,
        "boundary_counts": boundary_counts,
        "failure_hashes": failure_hashes,
        "flags_rules": flags_rules,
        "pass_hashes": pass_hashes,
        "rows": rows,
        "worker_sha256": sha256_file(worker),
    }


def write_audit_directory(
    output: pathlib.Path,
    audit: dict[str, Any],
    phase: str,
    source_sha: str,
) -> None:
    require_sha(source_sha, "source_sha", HEX40)
    output.mkdir(parents=True, exist_ok=True)
    table = {
        "row_count": len(audit["rows"]),
        "rows": audit["rows"],
        "schema": "vaeg-upd9002-m60e-iret-cases-v1",
        "schema_version": 1,
    }
    ratchet.write_deterministic_gzip(output / "iret_cases.json.gz", table)
    rules = {
        "row_count": len(audit["flags_rules"]),
        "rows": audit["flags_rules"],
        "schema": "vaeg-upd9002-m60e-iret-flags-rules-v1",
        "schema_version": 1,
    }
    write_json(output / "iret_flags_bit_rules.json", rules)
    boundary = {
        "partitions": audit["boundary_counts"],
        "schema": "vaeg-upd9002-m60e-iret-boundary-summary-v1",
        "schema_version": 1,
    }
    write_json(output / "iret_boundary_summary.json", boundary)
    summary = {
        "architectural_failure_count": len(audit["failure_hashes"]),
        "architectural_failure_hash_set_sha256": ratchet.hash_set_digest(
            audit["failure_hashes"]
        ),
        "architectural_pass_count": len(audit["pass_hashes"]),
        "architectural_pass_hash_set_sha256": ratchet.hash_set_digest(
            audit["pass_hashes"]
        ),
        "dataset_id": DATASET_ID,
        "executed": len(audit["rows"]),
        "phase": phase,
        "schema": "vaeg-upd9002-m60e-iret-audit-v1",
        "schema_version": 1,
        "selected": len(audit["rows"]),
        "source_sha": source_sha,
        "top_level_classification": "applicable",
        "worker_sha256": audit["worker_sha256"],
    }
    write_json(output / "iret_audit.json", summary)


def load_audit_directory(path: pathlib.Path, phase: str) -> dict[str, Any]:
    summary = read_json(path / "iret_audit.json")
    require(
        summary.get("phase") == phase
        and summary.get("selected") == EXPECTED_CF_SELECTED
        and summary.get("executed") == EXPECTED_CF_SELECTED,
        "audit-identity",
        f"{phase} audit identity differs",
    )
    with gzip.open(path / "iret_cases.json.gz", "rt", encoding="utf-8") as stream:
        table = json.load(stream)
    rows = table.get("rows")
    require(
        isinstance(rows, list)
        and table.get("row_count") == len(rows)
        and len(rows) == EXPECTED_CF_SELECTED,
        "audit-row-count",
        f"{phase} case table differs",
    )
    require(
        rows == sorted(rows, key=lambda row: row["case_hash"]),
        "nondeterministic-row-order",
        f"{phase} case rows are not sorted",
    )
    require(
        len({row["case_hash"] for row in rows}) == len(rows),
        "duplicate-case-hash",
        f"{phase} case rows overlap",
    )
    rules = read_json(path / "iret_flags_bit_rules.json")
    require(
        rules.get("row_count") == 16
        and [row["bit"] for row in rules.get("rows", [])] == list(range(16)),
        "missing-flags-rule-coverage",
        f"{phase} FLAGS rules differ",
    )
    boundary = read_json(path / "iret_boundary_summary.json")
    return {
        "boundary": boundary,
        "rows": rows,
        "rules": rules["rows"],
        "summary": summary,
    }


def enumerate_current_policy(
    root: pathlib.Path, dataset_root: pathlib.Path, scope: str
) -> dict[str, Any]:
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    with m60b.candidate_support_map(root) as support_path:
        enumeration = ratchet.enumerate_profiles(
            dataset_root,
            manifest,
            support_path,
            support_path,
            scope,
        )
    require(
        not enumeration["classification_changes"],
        "classification-taxonomy-registry-change",
        "same-policy enumeration changed classification",
    )
    require(
        enumeration["selected_hash_set_sha256"] == SELECTED_HASH_SETS[scope],
        "selected-set-drift",
        scope,
    )
    require(
        enumeration["after_set_digests"]["applicable"]
        == APPLICABLE_HASH_SETS[scope],
        "applicable-set-drift",
        scope,
    )
    return enumeration


def generate_scoreboard(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    raw_path: pathlib.Path,
    profile_key: str,
    evaluated_sha: str,
) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    require_sha(evaluated_sha, "evaluated_sha", HEX40)
    profile, scope = profile_key.rsplit("_", 1)
    raw = read_json(raw_path)
    require(
        raw.get("schema") == "vaeg-upd9002-ssts-result-v1"
        and raw.get("dataset_id") == DATASET_ID
        and raw.get("profile") == scope,
        "candidate-profile-identity",
        profile_key,
    )
    if profile == "fingerprint":
        require(
            raw.get("flags_comparison") == "all16",
            "comparison-domain-conflation",
            "fingerprint profile lacks all16 FLAGS",
        )
    else:
        require(
            "flags_comparison" not in raw,
            "comparison-domain-conflation",
            "architectural profile uses fingerprint FLAGS",
        )
    require(
        raw.get("selected_records") == BEFORE_PROFILE_IDENTITIES[profile_key]["selected"]
        and raw.get("executed_records")
        == BEFORE_PROFILE_IDENTITIES[profile_key]["executed"],
        "selected-set-drift",
        f"{profile_key} selected/executed counts differ",
    )
    result_counts = raw.get("result_counts", {})
    require(
        result_counts.get("timeout", 0) == 0
        and result_counts.get("crash", 0) == 0,
        "timeout-crash",
        profile_key,
    )
    enumeration = enumerate_current_policy(root, dataset_root, scope)
    failures_raw = ratchet.load_failure_records(raw_path)
    failures = {
        record_hash: ratchet.failure_entry(value)
        for record_hash, value in failures_raw.items()
    }
    applicable = set(enumeration["after_sets"]["applicable"])
    require(
        set(failures) <= applicable,
        "candidate-failure-outside-applicable",
        profile_key,
    )
    pass_hashes = sorted(applicable - set(failures))
    passed = result_counts.get("pass", 0)
    failed = sum(
        result_counts.get(kind, 0)
        for kind in ("semantic_failure", "timeout", "crash")
    )
    require(
        passed == len(pass_hashes) and failed == len(failures),
        "candidate-result-arithmetic",
        profile_key,
    )
    rows = ratchet.build_scoreboard_rows(
        raw, enumeration["after_form_counts"], failures_raw
    )
    failure_dir = output_path(output_root, FAILURE_DIRECTORY_PATHS[profile_key])
    shards, index_digest, canonical_sidecars, raw_sidecars = (
        ratchet.write_failure_shards(
            failures_raw,
            profile,
            scope,
            DATASET_ID,
            failure_dir,
        )
    )
    require(
        index_digest == raw["failure_signature_index_sha256"],
        "failure-signature-drift",
        profile_key,
    )
    mismatch_classes: Counter[str] = Counter()
    for failure in failures.values():
        mismatch_classes.update(failure["mismatch_classes"])
    before = read_json(root / G60D_SCOREBOARD_PATHS[profile_key])
    scoreboard = copy.deepcopy(before)
    scoreboard.update(
        {
            "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
            "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
            "crashes": 0,
            "epoch_gate": CANDIDATE_GATE,
            "evaluated_sha": evaluated_sha,
            "fail": failed,
            "failure_hash_set_sha256": ratchet.hash_set_digest(failures),
            "failure_shards": shards,
            "failure_sidecar_canonical_set_sha256": canonical_sidecars,
            "failure_sidecar_raw_set_sha256": raw_sidecars,
            "failure_signature_index_sha256": index_digest,
            "mismatch_classes": dict(sorted(mismatch_classes.items())),
            "pass": passed,
            "pass_hash_set_sha256": ratchet.hash_set_digest(pass_hashes),
            "raw_result_summary_sha256": sha256_file(raw_path),
            "records": rows,
            "scoreboard_digest": sha256_bytes(canonical_bytes(rows)),
            "termination_classes": raw["termination_counts"],
            "timeouts": 0,
        }
    )
    require(
        scoreboard["dataset_id"] == DATASET_ID
        and scoreboard["target_policy_id"] == TARGET_POLICY_ID
        and scoreboard["target_policy_sha256"] == TARGET_POLICY_SHA256
        and scoreboard["selected_hash_set_sha256"] == SELECTED_HASH_SETS[scope]
        and scoreboard["applicable_hash_set_sha256"]
        == APPLICABLE_HASH_SETS[scope],
        "candidate-profile-identity",
        f"{profile_key} governing identity differs",
    )
    try:
        validate_candidate_scoreboard(scoreboard)
    except m60b.M60bError as error:
        reject("candidate-scoreboard-schema", str(error))
    write_json(output_path(output_root, SCOREBOARD_PATHS[profile_key]), scoreboard)
    return scoreboard, failures


def write_transition(
    root: pathlib.Path,
    output_root: pathlib.Path,
    profile_key: str,
    scoreboard: dict[str, Any],
    failures_after: dict[str, dict[str, Any]],
    evaluated_sha: str,
) -> dict[str, Any]:
    before_summary = read_json(root / G60D_SCOREBOARD_PATHS[profile_key])
    failures_before = load_scoreboard_failures(
        root, G60D_SCOREBOARD_PATHS[profile_key]
    )
    before_set = set(failures_before)
    after_set = set(failures_after)
    newly_passing = sorted(before_set - after_set)
    newly_failing = sorted(after_set - before_set)
    changed = sorted(
        record_hash
        for record_hash in before_set & after_set
        if failures_before[record_hash]["signature_sha256"]
        != failures_after[record_hash]["signature_sha256"]
    )
    require(not newly_failing, "new-failure", profile_key)
    require(not changed, "changed-failure-not-enumerated", profile_key)
    cf_failure_count_before = sum(
        value["form"] == "CF" for value in failures_before.values()
    )
    require(
        len(newly_passing) == cf_failure_count_before,
        "ungoverned-newly-passing",
        f"{profile_key}: count={len(newly_passing)}",
    )
    require(
        all(failures_before[item]["form"] == "CF" for item in newly_passing),
        "ungoverned-newly-passing",
        f"{profile_key}: non-CF improvement",
    )
    before_rows = {
        (row["form"], row["classification"]): row
        for row in before_summary["records"]
    }
    after_rows = {
        (row["form"], row["classification"]): row
        for row in scoreboard["records"]
    }
    decreases = [
        form
        for (form, classification), before_row in before_rows.items()
        if classification == "applicable"
        and after_rows[(form, classification)]["pass"] < before_row["pass"]
    ]
    require(not decreases, "per-form-pass-decrease", repr(decreases))
    profile, scope = profile_key.rsplit("_", 1)
    transition = {
        "applicable_hash_set_sha256": APPLICABLE_HASH_SETS[scope],
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "cf_failure_count_after": 0,
        "cf_failure_count_before": cf_failure_count_before,
        "changed_failure_count": 0,
        "changed_failure_shards": [],
        "comparison_contract_ids": {profile: CONTRACTS[profile]},
        "dataset_id": DATASET_ID,
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": [],
        "hardware_pending_changes": [],
        "newly_failing": newly_failing,
        "newly_failing_hash_set_sha256": ratchet.hash_set_digest(newly_failing),
        "newly_passing": newly_passing,
        "newly_passing_hash_set_sha256": ratchet.hash_set_digest(newly_passing),
        "profile": profile,
        "schema": "vaeg-upd9002-m60e-iret-transition-v1",
        "schema_version": 1,
        "scope": scope,
        "scoreboard_after_digest": scoreboard["scoreboard_digest"],
        "scoreboard_before_digest": before_summary["scoreboard_digest"],
        "selected_hash_set_sha256": SELECTED_HASH_SETS[scope],
        "target_policy_id": TARGET_POLICY_ID,
        "top_level_classification_changes": [],
        "transition_kind": "iret_restoration_semantics",
    }
    write_json(output_path(output_root, TRANSITION_PATHS[profile_key]), transition)
    return transition


def write_ranking(
    root: pathlib.Path,
    output_root: pathlib.Path,
    scoreboard: dict[str, Any],
    failures: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    failures_by_form: dict[str, list[str]] = defaultdict(list)
    mismatch_by_form: dict[str, Counter[str]] = defaultdict(Counter)
    termination_by_form: dict[str, Counter[str]] = defaultdict(Counter)
    for record_hash, failure in failures.items():
        form = failure["form"]
        failures_by_form[form].append(record_hash)
        mismatch_by_form[form].update(failure["mismatch_classes"])
        termination_by_form[form][failure["actual_termination"]] += 1
    before = read_json(root / G60D_SCOREBOARD_PATHS["architectural_full"])
    before_rows = {
        (row["form"], row["classification"]): row for row in before["records"]
    }
    rows = []
    cumulative = 0
    total = scoreboard["fail"]
    for record in scoreboard["records"]:
        if record["classification"] != "applicable":
            continue
        form = record["form"]
        failed = record["fail"]
        cumulative += 0
        rows.append(
            {
                "change_from_g60d": (
                    failed - before_rows[(form, "applicable")]["fail"]
                ),
                "classification": "applicable",
                "executed": record["executed"],
                "fail": failed,
                "failure_hash_set_sha256": ratchet.hash_set_digest(
                    failures_by_form[form]
                ),
                "form": form,
                "mismatch_classes": dict(sorted(mismatch_by_form[form].items())),
                "opcode": record["opcode"],
                "pass": record["pass"],
                "selected": record["selected"],
                "subform": record["subform"],
                "termination_classes": dict(
                    sorted(termination_by_form[form].items())
                ),
            }
        )
    rows.sort(key=lambda row: (-row["fail"], row["form"]))
    for row in rows:
        cumulative += row["fail"]
        row["cumulative_failure_count"] = cumulative
        row["cumulative_share_ppm"] = (
            0 if total == 0 else (cumulative * 1_000_000) // total
        )
    require(
        sum(row["fail"] for row in rows) == total,
        "ranking-total-mismatch",
        "ranking does not reconcile",
    )
    family_counts: Counter[str] = Counter()
    for row in rows:
        family = (
            "0f-extension"
            if row["form"].startswith("0F")
            else (
                "shift"
                if row["form"].startswith(("C0.", "C1.", "D2.", "D3."))
                else (
                    "divide"
                    if row["form"] in {"F6.6", "F6.7", "F7.6", "F7.7"}
                    else row["opcode"]
                )
            )
        )
        family_counts[family] += row["fail"]
    ranking = {
        "architectural_full_failure_count": total,
        "cf_post_m60e": next(row for row in rows if row["form"] == "CF"),
        "family_failure_counts": dict(
            sorted(family_counts.items(), key=lambda item: (-item[1], item[0]))
        ),
        "row_count": len(rows),
        "rows": rows,
        "schema": "vaeg-upd9002-m60e-failure-ranking-v1",
        "schema_version": 1,
    }
    write_json(output_path(output_root, RANKING_JSON_PATH), ranking)
    top = rows[:30]
    lines = [
        "<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD. -->",
        "",
        "# G60e architectural-full failure ranking",
        "",
        f"Total remaining failures: **{total:,}**.",
        "",
        "| Rank | Form | Pass | Fail | Change from G60d | Cumulative |",
        "| ---: | :--- | ---: | ---: | ---: | ---: |",
    ]
    for index, row in enumerate(top, 1):
        lines.append(
            f"| {index} | `{row['form']}` | {row['pass']:,} | "
            f"{row['fail']:,} | {row['change_from_g60d']:+,} | "
            f"{row['cumulative_share_ppm'] / 10000:.2f}% |"
        )
    lines.extend(
        [
            "",
            "A form omitted from this top-30 view is not thereby proven green; "
            "the machine-readable table is complete.",
            "",
        ]
    )
    md_path = output_path(output_root, RANKING_MD_PATH)
    md_path.parent.mkdir(parents=True, exist_ok=True)
    md_path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return ranking


def artifact_entry(
    output_root: pathlib.Path, relative: pathlib.Path
) -> dict[str, Any]:
    path = output_path(output_root, relative)
    row_count = 1
    if relative.suffix == ".json":
        value = read_json(path)
        if isinstance(value, dict):
            row_count = value.get(
                "row_count",
                value.get("failure_count", len(value.get("artifacts", [None]))),
            )
    elif relative.suffix == ".gz":
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            value = json.load(stream)
        row_count = value.get("row_count", value.get("failure_count", 1))
    return {
        "bytes": path.stat().st_size,
        "path": relative.as_posix(),
        "row_count": row_count,
        "sha256": sha256_file(path),
    }


def write_evidence(
    root: pathlib.Path,
    output_root: pathlib.Path,
    pre: dict[str, Any],
    post: dict[str, Any],
    evaluated_sha: str,
    scoreboards: dict[str, dict[str, Any]],
    failures: dict[str, dict[str, dict[str, Any]]],
    transitions: dict[str, dict[str, Any]],
) -> tuple[dict[str, Any], dict[str, Any]]:
    pre_by_hash = {row["case_hash"]: row for row in pre["rows"]}
    post_by_hash = {row["case_hash"]: row for row in post["rows"]}
    require(
        set(pre_by_hash) == set(post_by_hash),
        "incomplete-cf-population",
        "pre/post CF case ownership differs",
    )
    merged_rows = []
    for record_hash in sorted(post_by_hash):
        row = copy.deepcopy(post_by_hash[record_hash])
        before = pre_by_hash[record_hash]
        row["pre_fix_actual_registers"] = before["actual_registers"]
        row["pre_fix_actual_restored_words"] = before["actual_restored_words"]
        row["pre_fix_actual_termination"] = before["actual_termination"]
        row["pre_fix_mismatch_kinds"] = before["mismatch_kinds"]
        row["post_fix_actual_registers"] = row["actual_registers"]
        row["post_fix_actual_restored_words"] = row["actual_restored_words"]
        row["post_fix_actual_termination"] = row["actual_termination"]
        row["post_fix_mismatch_kinds"] = row["mismatch_kinds"]
        merged_rows.append(row)
    table = {
        "row_count": len(merged_rows),
        "rows": merged_rows,
        "schema": "vaeg-upd9002-m60e-iret-cases-v1",
        "schema_version": 1,
    }
    table_path = EVIDENCE_ROOT / "iret_cases.json.gz"
    ratchet.write_deterministic_gzip(output_path(output_root, table_path), table)
    combined_rules = []
    for before, after in zip(pre["rules"], post["rules"]):
        require(
            before["bit"] == after["bit"]
            and before["expected_rule"] == after["expected_rule"],
            "independent-flags-rule",
            "pre/post expected FLAGS rule differs",
        )
        combined_rules.append(
            {
                **after,
                "post_fix_actual_rule": after["actual_rule"],
                "pre_fix_actual_rule": before["actual_rule"],
            }
        )
    rules_value = {
        "row_count": 16,
        "rows": combined_rules,
        "schema": "vaeg-upd9002-m60e-iret-flags-rules-v1",
        "schema_version": 1,
    }
    rules_path = EVIDENCE_ROOT / "iret_flags_bit_rules.json"
    write_json(output_path(output_root, rules_path), rules_value)
    boundary_path = EVIDENCE_ROOT / "iret_boundary_summary.json"
    write_json(
        output_path(output_root, boundary_path),
        {
            "partitions": post["boundary"]["partitions"],
            "schema": "vaeg-upd9002-m60e-iret-boundary-summary-v1",
            "schema_version": 1,
        },
    )
    pre_summary = pre["summary"]
    post_summary = post["summary"]
    audit_path = EVIDENCE_ROOT / "iret_audit.json"
    write_json(
        output_path(output_root, audit_path),
        {
            "candidate_gate": CANDIDATE_GATE,
            "classification": "applicable",
            "executed": EXPECTED_CF_SELECTED,
            "failure_after": post_summary["architectural_failure_count"],
            "failure_after_sha256": post_summary[
                "architectural_failure_hash_set_sha256"
            ],
            "failure_before": pre_summary["architectural_failure_count"],
            "failure_before_sha256": pre_summary[
                "architectural_failure_hash_set_sha256"
            ],
            "milestone": MILESTONE,
            "pass_after": post_summary["architectural_pass_count"],
            "pass_before": pre_summary["architectural_pass_count"],
            "schema": "vaeg-upd9002-m60e-iret-audit-v1",
            "schema_version": 1,
            "selected": EXPECTED_CF_SELECTED,
        },
    )
    reps_dir = EVIDENCE_ROOT / "representative"
    representatives = []
    selectors = [
        ("ordinary", lambda row: row["boundary_partition"] == "ordinary"),
        (
            "physical-wrap",
            lambda row: row["boundary_partition"] == "physical-wrap",
        ),
        (
            "pre-fix-bits-3-5",
            lambda row: any(
                name == "registers"
                for name in row["pre_fix_mismatch_kinds"]
            )
            and (
                int(row["stack_words_little_endian"]["flags"], 16) & 0x28
            )
            != 0,
        ),
    ]
    for label, selector in selectors:
        chosen = next((row for row in merged_rows if selector(row)), None)
        if chosen is None:
            continue
        representatives.append((label, chosen))
    rep_path = reps_dir / "iret.md"
    lines = [
        "<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD. -->",
        "",
        "# G60e IRET representatives",
        "",
    ]
    for label, row in representatives:
        lines.extend(
            [
                f"## {label}",
                "",
                f"- Case hash: `{row['case_hash']}`",
                f"- Bytes: `{row['instruction_bytes']}`",
                f"- Boundary: `{row['boundary_partition']}`",
                f"- Pre-fix mismatches: "
                f"`{','.join(row['pre_fix_mismatch_kinds']) or 'none'}`",
                f"- Post-fix mismatches: "
                f"`{','.join(row['post_fix_mismatch_kinds']) or 'none'}`",
                "",
            ]
        )
    rep_output = output_path(output_root, rep_path)
    rep_output.parent.mkdir(parents=True, exist_ok=True)
    rep_output.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    ranking = write_ranking(
        root,
        output_root,
        scoreboards["architectural_full"],
        failures["architectural_full"],
    )
    artifact_paths = [
        audit_path,
        boundary_path,
        rules_path,
        table_path,
        rep_path,
        *SCOREBOARD_PATHS.values(),
        *TRANSITION_PATHS.values(),
        RANKING_JSON_PATH,
        RANKING_MD_PATH,
    ]
    for key in FAILURE_DIRECTORY_PATHS:
        artifact_paths.extend(
            path.relative_to(output_root)
            for path in output_path(
                output_root, FAILURE_DIRECTORY_PATHS[key]
            ).glob("*.json.gz")
        )
    artifact_paths = sorted(set(artifact_paths), key=lambda path: path.as_posix())
    artifacts = [artifact_entry(output_root, path) for path in artifact_paths]
    artifact_tree_sha256 = sha256_bytes(canonical_bytes(artifacts))
    manifest = {
        "applicable_hash_set_sha256": APPLICABLE_HASH_SETS,
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifact_tree_sha256": artifact_tree_sha256,
        "artifacts": artifacts,
        "boundary_partitions": post["boundary"]["partitions"],
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "evaluated_sha": evaluated_sha,
        "generator": {
            "path": "tools/qa/upd9002_m60e_iret.py",
            "sha256": sha256_file(root / "tools/qa/upd9002_m60e_iret.py"),
            "version": 1,
        },
        "iret_flags_rule_sha256": sha256_bytes(canonical_bytes(combined_rules)),
        "milestone": MILESTONE,
        "newly_failing_count": 0,
        "newly_failing_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "newly_passing_count": EXPECTED_CF_FAILURE_BEFORE,
        "newly_passing_hash_set_sha256": transitions[
            "architectural_full"
        ]["newly_passing_hash_set_sha256"],
        "post_fix_cf_failure_count": 0,
        "post_fix_cf_failure_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "post_fix_cf_pass_count": EXPECTED_CF_SELECTED,
        "pre_fix_cf_failure_count": EXPECTED_CF_FAILURE_BEFORE,
        "pre_fix_cf_failure_hash_set_sha256": pre_summary[
            "architectural_failure_hash_set_sha256"
        ],
        "pre_fix_cf_pass_count": EXPECTED_CF_PASS_BEFORE,
        "ranking_sha256": sha256_file(output_path(output_root, RANKING_JSON_PATH)),
        "schema": "vaeg-upd9002-m60e-evidence-manifest-v1",
        "schema_version": 1,
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "target_policy_id": TARGET_POLICY_ID,
        "worker_sha256": post_summary["worker_sha256"],
        "environment": {
            "gzip_module": gzip.__name__,
            "platform": platform.platform(),
            "python": platform.python_version(),
            "zlib": zlib.ZLIB_VERSION,
        },
    }
    manifest_path = EVIDENCE_ROOT / "manifest.json"
    write_json(output_path(output_root, manifest_path), manifest)
    result = {
        "artifact_tree_sha256": artifact_tree_sha256,
        "candidate_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "evidence_manifest_sha256": sha256_file(
            output_path(output_root, manifest_path)
        ),
        "profile_identities": {
            key: {
                field: scoreboards[key][field]
                for field in (
                    "applicable",
                    "applicable_hash_set_sha256",
                    "executed",
                    "fail",
                    "failure_hash_set_sha256",
                    "failure_signature_index_sha256",
                    "pass",
                    "pass_hash_set_sha256",
                    "selected",
                    "selected_hash_set_sha256",
                )
            }
            for key in scoreboards
        },
        "ranking_failure_total": ranking["architectural_full_failure_count"],
        "schema": "vaeg-upd9002-m60e-result-manifest-v1",
        "schema_version": 1,
        "transition_sha256": {
            key: sha256_file(output_path(output_root, path))
            for key, path in TRANSITION_PATHS.items()
        },
    }
    write_json(output_path(output_root, RESULT_MANIFEST_PATH), result)
    return manifest, result


def generate(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    pre_fix_audit: pathlib.Path,
    post_fix_audit: pathlib.Path,
    raw_paths: dict[str, pathlib.Path],
    evaluated_sha: str,
) -> dict[str, Any]:
    verify_predecessor(root)
    pre = load_audit_directory(pre_fix_audit, "pre-fix")
    post = load_audit_directory(post_fix_audit, "post-fix")
    scoreboards: dict[str, dict[str, Any]] = {}
    failures: dict[str, dict[str, dict[str, Any]]] = {}
    for key in ("architectural_ci", "architectural_full", "fingerprint_full"):
        scoreboards[key], failures[key] = generate_scoreboard(
            root,
            output_root,
            dataset_root,
            raw_paths[key],
            key,
            evaluated_sha,
        )
    transitions = {
        key: write_transition(
            root,
            output_root,
            key,
            scoreboards[key],
            failures[key],
            evaluated_sha,
        )
        for key in ("architectural_ci", "architectural_full")
    }
    manifest, result = write_evidence(
        root,
        output_root,
        pre,
        post,
        evaluated_sha,
        scoreboards,
        failures,
        transitions,
    )
    return {
        "artifact_tree_sha256": manifest["artifact_tree_sha256"],
        "evidence_manifest_sha256": result["evidence_manifest_sha256"],
        "transition_sha256": result["transition_sha256"],
    }


def tree_file_identities(root: pathlib.Path) -> list[dict[str, Any]]:
    paths = sorted(
        (
            path for path in root.rglob("*")
            if path.is_file()
        ),
        key=lambda path: path.relative_to(root).as_posix(),
    )
    return [
        {
            "path": path.relative_to(root).as_posix(),
            "sha256": sha256_file(path),
            "size": path.stat().st_size,
        }
        for path in paths
    ]


def regenerate_twice(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    pre_fix_audit: pathlib.Path,
    post_fix_audit: pathlib.Path,
    raw_paths: dict[str, pathlib.Path],
    evaluated_sha: str,
) -> dict[str, Any]:
    with tempfile.TemporaryDirectory(prefix="vaeg-m60e-regen-a-") as first_name:
        with tempfile.TemporaryDirectory(prefix="vaeg-m60e-regen-b-") as second_name:
            first = pathlib.Path(first_name)
            second = pathlib.Path(second_name)
            first_result = generate(
                root,
                first,
                dataset_root,
                pre_fix_audit,
                post_fix_audit,
                raw_paths,
                evaluated_sha,
            )
            second_result = generate(
                root,
                second,
                dataset_root,
                pre_fix_audit,
                post_fix_audit,
                raw_paths,
                evaluated_sha,
            )
            require(
                tree_file_identities(first) == tree_file_identities(second),
                "nondeterministic-generation",
                "two complete generations differ",
            )
            if output_root.exists():
                for relative in (
                    EVIDENCE_ROOT,
                    RESULT_MANIFEST_PATH,
                    *SCOREBOARD_PATHS.values(),
                    *FAILURE_DIRECTORY_PATHS.values(),
                    *TRANSITION_PATHS.values(),
                    RANKING_JSON_PATH,
                    RANKING_MD_PATH,
                ):
                    target = output_path(output_root, relative)
                    if target.is_dir():
                        shutil.rmtree(target)
                    elif target.exists():
                        target.unlink()
            for source in sorted(first.rglob("*")):
                if not source.is_file():
                    continue
                relative = source.relative_to(first)
                target = output_path(output_root, relative)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(source, target)
            return first_result


def git_diff_names(
    root: pathlib.Path, revision: str, paths: list[str]
) -> list[str]:
    completed = subprocess.run(
        ["git", "diff", "--name-only", revision, "--", *paths],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    require(
        completed.returncode == 0,
        "git-diff",
        completed.stderr.strip() or "cannot inspect changed paths",
    )
    return completed.stdout.splitlines()


def verify_protected_paths(root: pathlib.Path) -> None:
    protected = [
        "tests/ssts/contracts/",
        "tests/ssts/epochs/g43/",
        "tests/ssts/evidence/g59/",
        "tests/ssts/evidence/g60d/",
        "tests/ssts/authority/g60b/",
        "tests/ssts/authority/g60c/",
        "tests/ssts/target_policy/g60b",
        "tests/ssts/gap_taxonomy.json",
        "tests/ssts/hardware_pending.json",
        "tests/ssts/approved_target_divergences.json",
    ]
    protected.extend(
        [
            f"tests/ssts/scoreboard/{prefix}"
            for prefix in ("g58", "g60a", "g60b", "g60d")
        ]
    )
    protected.extend(
        [
            f"tests/ssts/transitions/{prefix}"
            for prefix in ("g58", "g60a", "g60b", "g60d")
        ]
    )
    changed = git_diff_names(
        root, f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD", protected
    )
    require(
        not changed,
        "protected-artifact-mutation",
        repr(changed),
    )
    fixture_paths = [
        path.relative_to(root).as_posix()
        for path in root.glob("tests/ssts/**/*")
        if path.is_file()
        and (
            path.name.endswith(".fixture")
            or "fixtures" in path.parts
        )
    ]
    if fixture_paths:
        changed = git_diff_names(
            root, f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD", fixture_paths
        )
        require(not changed, "protected-artifact-mutation", repr(changed))


def verify_semantic_diff(root: pathlib.Path) -> None:
    changed = git_diff_names(
        root,
        f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD",
        ["cpu/upd9002/"],
    )
    require(
        changed in ([], ["cpu/upd9002/upd9002_dispatch.c"]),
        "semantic-scope",
        repr(changed),
    )
    if not changed:
        return
    completed = subprocess.run(
        [
            "git",
            "diff",
            f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD",
            "--",
            "cpu/upd9002/upd9002_dispatch.c",
        ],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    require(completed.returncode == 0, "git-diff", completed.stderr)
    removed = [
        line[1:]
        for line in completed.stdout.splitlines()
        if line.startswith("-") and not line.startswith("---")
    ]
    added = [
        line[1:]
        for line in completed.stdout.splitlines()
        if line.startswith("+") and not line.startswith("+++")
    ]
    require(
        removed
        == [
            "\tflag = (flag & 0x0fff) | 0xf002;",
        ]
        and added
        == [
            "\tflag = (flag & 0x0fd7) | 0xf002;",
        ],
        "semantic-scope",
        "production diff is not the one evidence-proven IRET FLAGS line",
    )


def validate_generated_family(
    root: pathlib.Path, enforce_final_commit_scope: bool = True
) -> None:
    family = [
        (root / EVIDENCE_ROOT / "manifest.json").is_file(),
        (root / RESULT_MANIFEST_PATH).is_file(),
        all((root / path).is_file() for path in SCOREBOARD_PATHS.values()),
        all((root / path).is_file() for path in TRANSITION_PATHS.values()),
        (root / RANKING_JSON_PATH).is_file(),
        (root / RANKING_MD_PATH).is_file(),
    ]
    if not any(family):
        return
    require(all(family), "evidence-family-incomplete", repr(family))
    manifest_path = root / EVIDENCE_ROOT / "manifest.json"
    manifest = read_json(manifest_path)
    require(
        manifest_path.read_bytes() == canonical_bytes(manifest) + b"\n",
        "nondeterministic-json",
        manifest_path.as_posix(),
    )
    require(
        manifest.get("approved_predecessor_sha") == APPROVED_PREDECESSOR_SHA
        and manifest.get("target_policy_id") == TARGET_POLICY_ID
        and manifest.get("dataset_id") == DATASET_ID
        and manifest.get("selected_hash_set_sha256") == SELECTED_HASH_SETS
        and manifest.get("applicable_hash_set_sha256") == APPLICABLE_HASH_SETS,
        "evidence-identity",
        "manifest governing identity differs",
    )
    entries = manifest.get("artifacts")
    require(
        isinstance(entries, list)
        and entries == sorted(entries, key=lambda row: row["path"]),
        "nondeterministic-row-order",
        "artifact manifest order differs",
    )
    for entry in entries:
        relative = pathlib.Path(entry["path"])
        require(
            not relative.is_absolute() and ".." not in relative.parts,
            "unsafe-artifact-path",
            entry["path"],
        )
        path = root / relative
        require(
            path.is_file()
            and path.stat().st_size == entry["bytes"]
            and sha256_file(path) == entry["sha256"],
            "artifact-digest-mismatch",
            entry["path"],
        )
    require(
        sha256_bytes(canonical_bytes(entries))
        == manifest["artifact_tree_sha256"],
        "artifact-digest-mismatch",
        "artifact tree differs",
    )
    result_path = root / RESULT_MANIFEST_PATH
    result = read_json(result_path)
    require(
        result_path.read_bytes() == canonical_bytes(result) + b"\n",
        "nondeterministic-json",
        result_path.as_posix(),
    )
    require(
        result["evidence_manifest_sha256"] == sha256_file(manifest_path)
        and result["artifact_tree_sha256"] == manifest["artifact_tree_sha256"],
        "artifact-digest-mismatch",
        "result manifest binding differs",
    )
    for key, path in SCOREBOARD_PATHS.items():
        value = read_json(root / path)
        require(
            (root / path).read_bytes() == canonical_bytes(value) + b"\n",
            "nondeterministic-json",
            path.as_posix(),
        )
        try:
            validate_candidate_scoreboard(value)
            ratchet.load_scoreboard_failures(root / path, value)
        except (m60b.M60bError, ratchet.RatchetError) as error:
            reject("candidate-scoreboard-schema", f"{key}: {error}")
    for key, path in TRANSITION_PATHS.items():
        transition = read_json(root / path)
        validate_transition_identity(
            transition, manifest["evaluated_sha"], key
        )
    ranking = read_json(root / RANKING_JSON_PATH)
    require(
        ranking["architectural_full_failure_count"]
        == read_json(root / SCOREBOARD_PATHS["architectural_full"])["fail"]
        and sum(row["fail"] for row in ranking["rows"])
        == ranking["architectural_full_failure_count"],
        "ranking-total-mismatch",
        "ranking does not reconcile",
    )
    if not enforce_final_commit_scope:
        return
    completed = subprocess.run(
        ["git", "diff", "--name-only", "HEAD^..HEAD"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    require(completed.returncode == 0, "git-diff", completed.stderr)
    allowed = (
        REPORT_PATH.as_posix(),
        EVIDENCE_ROOT.as_posix() + "/",
        RESULT_MANIFEST_PATH.as_posix(),
        "tests/ssts/scoreboard/g60e_",
        "tests/ssts/transitions/g60e_",
        "tests/ssts/rankings/g60e_",
    )
    unexpected = [
        path
        for path in completed.stdout.splitlines()
        if not any(path == prefix or path.startswith(prefix) for prefix in allowed)
    ]
    require(
        not unexpected,
        "evidence-commit-scope",
        repr(unexpected),
    )


def verify_static(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    verify_predecessor(root)
    verify_protected_paths(root)
    if not protected_evidence_only:
        verify_semantic_diff(root)
    validate_generated_family(
        root, enforce_final_commit_scope=not protected_evidence_only
    )
    print(
        "m60e-static: predecessor, protected artifacts, deterministic evidence "
        "family"
        + (
            " passed for forward-milestone protection"
            if protected_evidence_only
            else ", bounded IRET semantic diff, and final commit scope passed"
        )
    )


def synthetic_decision() -> dict[str, Any]:
    governed = ["1" * 64]
    return {
        "applicable_hash_set_sha256": copy.deepcopy(APPLICABLE_HASH_SETS),
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "architectural_and_fingerprint_separate": True,
        "bound_range_unchanged": True,
        "byte_order": "little-endian",
        "cf_executed": EXPECTED_CF_SELECTED,
        "cf_selected": EXPECTED_CF_SELECTED,
        "changed_failure_count": 0,
        "comparison_contracts": copy.deepcopy(CONTRACTS),
        "dataset_id": DATASET_ID,
        "expected_and_actual_present": True,
        "evidence_commit_only": True,
        "flags_rule_count": 16,
        "flags_rule_values_valid": True,
        "governed_newly_passing": governed,
        "g60d_residual_frame_empty": True,
        "interrupt_entry_unchanged": True,
        "ip_cs_distinct": True,
        "iret_rules_independently_derived": True,
        "metadata_mask_unchanged": True,
        "newly_failing": [],
        "newly_passing": governed.copy(),
        "physical_wrap_accounted": True,
        "protected_artifacts_unchanged": True,
        "protected_behavior_changed": False,
        "ranking_failure_total": 56172,
        "scoreboard_failure_total": 56172,
        "segment_wrap_accounted": True,
        "selected_hash_set_sha256": copy.deepcopy(SELECTED_HASH_SETS),
        "sp_increment": 6,
        "stack_word_order": ["ip", "cs", "flags"],
        "target_policy_id": TARGET_POLICY_ID,
        "taxonomy_or_registry_changed": False,
        "div_idiv_arithmetic_unchanged": True,
    }


def validate_transition_identity(
    transition: dict[str, Any], evaluated_sha: str, label: str
) -> None:
    require(
        transition["before_sha"] == APPROVED_PREDECESSOR_SHA
        and transition["evaluated_sha"] == evaluated_sha
        and not transition["newly_failing"]
        and transition["changed_failure_count"] == 0
        and len(transition["newly_passing"])
        == transition["cf_failure_count_before"],
        "transition-identity",
        label,
    )


def validate_decision(value: Any) -> None:
    require(
        isinstance(value, dict) and set(value) == set(synthetic_decision()),
        "decision-schema",
        "decision keys differ",
    )
    require(
        value["approved_predecessor_sha"] == APPROVED_PREDECESSOR_SHA,
        "wrong-predecessor-sha",
        "G60d predecessor differs",
    )
    require(
        value["target_policy_id"] == TARGET_POLICY_ID,
        "wrong-target-policy-id",
        "target policy differs",
    )
    require(value["dataset_id"] == DATASET_ID, "dataset-drift", "dataset differs")
    require(
        value["comparison_contracts"] == CONTRACTS,
        "comparison-contract-drift",
        "contracts differ",
    )
    require(
        value["selected_hash_set_sha256"] == SELECTED_HASH_SETS,
        "selected-set-drift",
        "selected sets differ",
    )
    require(
        value["applicable_hash_set_sha256"] == APPLICABLE_HASH_SETS,
        "applicable-set-drift",
        "applicable sets differ",
    )
    require(
        not value["taxonomy_or_registry_changed"],
        "classification-taxonomy-registry-change",
        "governance state changed",
    )
    require(
        value["cf_selected"] == EXPECTED_CF_SELECTED,
        "incomplete-cf-population",
        "selected CF count differs",
    )
    require(
        value["cf_executed"] == EXPECTED_CF_SELECTED,
        "missing-executed-cf-record",
        "executed CF count differs",
    )
    require(
        value["expected_and_actual_present"],
        "expected-only-evidence",
        "actual state is missing",
    )
    require(
        value["stack_word_order"] == ["ip", "cs", "flags"],
        "wrong-stack-word-order",
        "IRET stack word order differs",
    )
    require(
        value["byte_order"] == "little-endian",
        "wrong-byte-order",
        "IRET byte order differs",
    )
    require(
        value["sp_increment"] == 6,
        "wrong-sp-increment",
        "IRET SP increment differs",
    )
    require(
        value["segment_wrap_accounted"],
        "missed-segment-wrap",
        "segment wrapping is not accounted",
    )
    require(
        value["physical_wrap_accounted"],
        "missed-physical-wrap",
        "physical wrapping is not accounted",
    )
    require(
        value["ip_cs_distinct"],
        "ip-cs-swap",
        "IP and CS ownership is ambiguous",
    )
    require(
        value["iret_rules_independently_derived"],
        "popf-rule-copy",
        "POPF was used as the IRET oracle",
    )
    require(
        value["flags_rule_count"] == 16,
        "missing-flags-rule-coverage",
        "FLAGS bit coverage differs",
    )
    require(
        value["flags_rule_values_valid"],
        "unsupported-flags-rule",
        "FLAGS rule vocabulary differs",
    )
    require(
        value["architectural_and_fingerprint_separate"],
        "comparison-domain-conflation",
        "architectural and fingerprint domains were conflated",
    )
    require(
        value["metadata_mask_unchanged"],
        "metadata-mask-change",
        "comparison metadata mask changed",
    )
    require(
        value["interrupt_entry_unchanged"],
        "interrupt-entry-change",
        "interrupt entry changed",
    )
    require(
        value["g60d_residual_frame_empty"],
        "g60d-residual-frame-regression",
        "G60d residual frame set is no longer empty",
    )
    require(
        value["bound_range_unchanged"],
        "bound-range-change",
        "BOUND range behavior changed",
    )
    require(
        value["div_idiv_arithmetic_unchanged"],
        "divide-arithmetic-change",
        "DIV/IDIV arithmetic changed",
    )
    require(
        value["protected_artifacts_unchanged"],
        "protected-artifact-mutation",
        "approved artifact changed",
    )
    require(not value["newly_failing"], "new-failure", "new failure present")
    require(
        set(value["newly_passing"]) <= set(value["governed_newly_passing"]),
        "ungoverned-newly-passing",
        "newly passing hash lies outside CF ownership",
    )
    require(
        value["changed_failure_count"] == 0,
        "changed-failure-not-enumerated",
        "changed failure remains",
    )
    require(
        not value["protected_behavior_changed"],
        "protected-behavior-regression",
        "protected M60a/M60d behavior changed",
    )
    require(
        value["evidence_commit_only"],
        "evidence-commit-scope",
        "evidence commit contains implementation",
    )
    require(
        value["ranking_failure_total"] == value["scoreboard_failure_total"],
        "ranking-total-mismatch",
        "ranking does not reconcile",
    )


def expect_rejection(
    label: str, expected_code: str, callback: Callable[[], None]
) -> None:
    try:
        callback()
    except M60eError as error:
        if error.code != expected_code:
            raise AssertionError(
                f"{label}: expected rejection {expected_code}, got {error.code}"
            ) from error
        return
    raise AssertionError(f"{label}: validation unexpectedly accepted mutation")


def selftest() -> None:
    validate_decision(synthetic_decision())
    synthetic_transition = {
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "cf_failure_count_before": 2,
        "changed_failure_count": 0,
        "evaluated_sha": "3" * 40,
        "newly_failing": [],
        "newly_passing": ["1" * 64, "2" * 64],
    }
    validate_transition_identity(
        synthetic_transition, synthetic_transition["evaluated_sha"], "synthetic"
    )
    mutations: list[tuple[str, str, Callable[[dict[str, Any]], None]]] = [
        ("predecessor", "wrong-predecessor-sha",
         lambda value: value.__setitem__("approved_predecessor_sha", "0" * 40)),
        ("policy", "wrong-target-policy-id",
         lambda value: value.__setitem__("target_policy_id", "wrong")),
        ("dataset", "dataset-drift",
         lambda value: value.__setitem__("dataset_id", "wrong")),
        ("contract", "comparison-contract-drift",
         lambda value: value["comparison_contracts"]["architectural"].__setitem__(
             "id", "wrong")),
        ("selected", "selected-set-drift",
         lambda value: value["selected_hash_set_sha256"].__setitem__("ci", "0" * 64)),
        ("applicable", "applicable-set-drift",
         lambda value: value["applicable_hash_set_sha256"].__setitem__(
             "full", "0" * 64)),
        ("governance", "classification-taxonomy-registry-change",
         lambda value: value.__setitem__("taxonomy_or_registry_changed", True)),
        ("CF selection", "incomplete-cf-population",
         lambda value: value.__setitem__("cf_selected", 4999)),
        ("CF execution", "missing-executed-cf-record",
         lambda value: value.__setitem__("cf_executed", 4999)),
        ("expected-only", "expected-only-evidence",
         lambda value: value.__setitem__("expected_and_actual_present", False)),
        ("word order", "wrong-stack-word-order",
         lambda value: value.__setitem__(
             "stack_word_order", ["flags", "cs", "ip"])),
        ("byte order", "wrong-byte-order",
         lambda value: value.__setitem__("byte_order", "big-endian")),
        ("SP increment", "wrong-sp-increment",
         lambda value: value.__setitem__("sp_increment", 4)),
        ("segment wrap", "missed-segment-wrap",
         lambda value: value.__setitem__("segment_wrap_accounted", False)),
        ("physical wrap", "missed-physical-wrap",
         lambda value: value.__setitem__("physical_wrap_accounted", False)),
        ("IP/CS swap", "ip-cs-swap",
         lambda value: value.__setitem__("ip_cs_distinct", False)),
        ("POPF analogy", "popf-rule-copy",
         lambda value: value.__setitem__(
             "iret_rules_independently_derived", False)),
        ("FLAGS coverage", "missing-flags-rule-coverage",
         lambda value: value.__setitem__("flags_rule_count", 15)),
        ("FLAGS vocabulary", "unsupported-flags-rule",
         lambda value: value.__setitem__("flags_rule_values_valid", False)),
        ("domain conflation", "comparison-domain-conflation",
         lambda value: value.__setitem__(
             "architectural_and_fingerprint_separate", False)),
        ("metadata mask", "metadata-mask-change",
         lambda value: value.__setitem__("metadata_mask_unchanged", False)),
        ("interrupt entry", "interrupt-entry-change",
         lambda value: value.__setitem__("interrupt_entry_unchanged", False)),
        ("G60d frame", "g60d-residual-frame-regression",
         lambda value: value.__setitem__("g60d_residual_frame_empty", False)),
        ("BOUND range", "bound-range-change",
         lambda value: value.__setitem__("bound_range_unchanged", False)),
        ("DIV arithmetic", "divide-arithmetic-change",
         lambda value: value.__setitem__(
             "div_idiv_arithmetic_unchanged", False)),
        ("protected artifact", "protected-artifact-mutation",
         lambda value: value.__setitem__(
             "protected_artifacts_unchanged", False)),
        ("new failure", "new-failure",
         lambda value: value["newly_failing"].append("2" * 64)),
        ("ungoverned pass", "ungoverned-newly-passing",
         lambda value: value["newly_passing"].append("2" * 64)),
        ("changed failure", "changed-failure-not-enumerated",
         lambda value: value.__setitem__("changed_failure_count", 1)),
        ("protected regression", "protected-behavior-regression",
         lambda value: value.__setitem__("protected_behavior_changed", True)),
        ("evidence scope", "evidence-commit-scope",
         lambda value: value.__setitem__("evidence_commit_only", False)),
        ("ranking", "ranking-total-mismatch",
         lambda value: value.__setitem__("ranking_failure_total", 56171)),
    ]
    for label, code, mutation in mutations:
        value = synthetic_decision()
        mutation(value)
        expect_rejection(label, code, lambda candidate=value: validate_decision(candidate))
    bad_transition = copy.deepcopy(synthetic_transition)
    bad_transition["cf_failure_count_before"] = 3
    expect_rejection(
        "scope-specific transition count",
        "transition-identity",
        lambda: validate_transition_identity(
            bad_transition, bad_transition["evaluated_sha"], "synthetic"
        ),
    )
    rows = [
        {"case_hash": "2" * 64},
        {"case_hash": "1" * 64},
    ]
    expect_rejection(
        "row ordering",
        "nondeterministic-row-order",
        lambda: require(
            rows == sorted(rows, key=lambda row: row["case_hash"]),
            "nondeterministic-row-order",
            "rows are not sorted",
        ),
    )
    with tempfile.TemporaryDirectory(prefix="vaeg-m60e-selftest-") as name:
        directory = pathlib.Path(name)
        payload = {
            "rows": [{"case_hash": "1" * 64}],
            "schema": "synthetic",
            "schema_version": 1,
        }
        first = directory / "first.json.gz"
        second = directory / "second.json.gz"
        ratchet.write_deterministic_gzip(first, payload)
        ratchet.write_deterministic_gzip(second, payload)
        if first.read_bytes() != second.read_bytes():
            raise AssertionError("deterministic gzip differs")
        json_path = directory / "canonical.json"
        write_json(json_path, payload)
        if json_path.read_bytes() != canonical_bytes(payload) + b"\n":
            raise AssertionError("canonical JSON differs")
    print(
        f"m60e-selftest: {len(mutations) + 2} fail-closed mutations rejected "
        "at the intended reason code; surrounding validation could not mask "
        "the target check; deterministic JSON/gzip passed"
    )


def add_root(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest")
    predecessor = subparsers.add_parser("verify-predecessor")
    add_root(predecessor)
    for name in ("architectural-ci", "architectural-full", "fingerprint-full"):
        predecessor.add_argument(f"--{name}-raw", type=pathlib.Path, required=True)
    static = subparsers.add_parser("verify-static")
    add_root(static)
    static.add_argument("--protected-evidence-only", action="store_true")
    audit = subparsers.add_parser("audit")
    add_root(audit)
    audit.add_argument("--dataset-root", type=pathlib.Path, required=True)
    audit.add_argument("--worker", type=pathlib.Path, required=True)
    audit.add_argument("--phase", choices=("pre-fix", "post-fix"), required=True)
    audit.add_argument("--source-sha", required=True)
    audit.add_argument("--output", type=pathlib.Path, required=True)
    for command in ("generate", "regenerate-twice"):
        generator = subparsers.add_parser(command)
        add_root(generator)
        generator.add_argument(
            "--dataset-root", type=pathlib.Path, required=True
        )
        generator.add_argument(
            "--pre-fix-audit", type=pathlib.Path, required=True
        )
        generator.add_argument(
            "--post-fix-audit", type=pathlib.Path, required=True
        )
        generator.add_argument(
            "--architectural-ci-raw", type=pathlib.Path, required=True
        )
        generator.add_argument(
            "--architectural-full-raw", type=pathlib.Path, required=True
        )
        generator.add_argument(
            "--fingerprint-full-raw", type=pathlib.Path, required=True
        )
        generator.add_argument("--evaluated-sha", required=True)
        generator.add_argument("--output-root", type=pathlib.Path, required=True)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
            return 0
        root = arguments.root.resolve()
        if arguments.command == "verify-static":
            verify_static(root, arguments.protected_evidence_only)
        elif arguments.command == "verify-predecessor":
            verify_predecessor(root)
            verify_raw_predecessor_profile(
                root, arguments.architectural_ci_raw.resolve(), "architectural_ci"
            )
            verify_raw_predecessor_profile(
                root,
                arguments.architectural_full_raw.resolve(),
                "architectural_full",
            )
            verify_raw_predecessor_profile(
                root,
                arguments.fingerprint_full_raw.resolve(),
                "fingerprint_full",
            )
            print("m60e-predecessor: exact G60d identities reproduced")
        elif arguments.command == "audit":
            verify_predecessor(root)
            audit = run_cf_audit(
                root,
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.phase,
            )
            write_audit_directory(
                arguments.output.resolve(),
                audit,
                arguments.phase,
                arguments.source_sha,
            )
            print(
                "m60e-audit: "
                f"phase={arguments.phase} selected={len(audit['rows'])} "
                f"pass={len(audit['pass_hashes'])} "
                f"fail={len(audit['failure_hashes'])}"
            )
        elif arguments.command in {"generate", "regenerate-twice"}:
            raw_paths = {
                "architectural_ci": arguments.architectural_ci_raw.resolve(),
                "architectural_full": (
                    arguments.architectural_full_raw.resolve()
                ),
                "fingerprint_full": arguments.fingerprint_full_raw.resolve(),
            }
            callback = (
                regenerate_twice
                if arguments.command == "regenerate-twice"
                else generate
            )
            result = callback(
                root,
                arguments.output_root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.pre_fix_audit.resolve(),
                arguments.post_fix_audit.resolve(),
                raw_paths,
                arguments.evaluated_sha,
            )
            print(
                "m60e-generate: "
                f"artifact_tree={result['artifact_tree_sha256']} "
                f"manifest={result['evidence_manifest_sha256']}"
            )
        else:
            raise AssertionError(arguments.command)
    except (
        M60eError,
        m60b.M60bError,
        m60c.M60cError,
        m60d.M60dError,
        OSError,
        ssts.CorpusError,
    ) as error:
        print(f"m60e-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

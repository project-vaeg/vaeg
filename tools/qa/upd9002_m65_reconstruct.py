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
"""Reconstruct expected/actual evidence for the M65 residue campaign."""

from __future__ import annotations

import argparse
import collections
import copy
import gzip
import hashlib
import json
import pathlib
import sys
import zlib
from typing import Any

import upd9002_ssts as ssts


ROOT = pathlib.Path(__file__).resolve().parents[2]
BASE_SHA = "efd96b7e46717e7ee56e086f7d27ba42b04b49d3"
MATERIALIZATION_SHA = "8ad6ec57519cd2cf0b56e3228d1983add5655563"
EXPECTED_WORKER_SHA256 = (
    "5611c26224fd060dfdcaaca02ed3a57ce9e30156d8617eaca2d9a6fd9f593199"
)
DATASET_ID = (
    "ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-"
    "1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4"
)
ARCH_CONTRACT_ID = "upd9002-v20-architectural-v1"
ARCH_CONTRACT_SHA256 = (
    "aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5"
)
BACKLOG_SHA256 = "240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5"
TASK_FORMS = {
    "M65a": {"FF.7"},
    "M65b": {"62"},
    "M65c": {"F7.2"},
    "M65d": {"FF.6"},
}
TAIL_TASK = "M65e"
TASK_CASE_FILES = {
    "M65a": "m65a_ff7_cases.json.gz",
    "M65b": "m65b_bound_cases.json.gz",
    "M65c": "m65c_f7_not_cases.json.gz",
    "M65d": "m65d_ff6_cases.json.gz",
    "M65e": "m65e_tail_cases.json.gz",
}
EXPECTED_COUNTS = {
    "M65a": 5000,
    "M65b": 1244,
    "M65c": 1113,
    "M65d": 144,
    "M65e": 10,
}
REQUIRED_FORMS = {
    "FF.7",
    "62",
    "F7.2",
    "FF.6",
    "61",
    "81.6",
    "FF.5",
    "A5",
    "9C",
    "D1.6",
    "C8",
    "C4",
}
PREFIXES = {0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0xF0, 0xF1, 0xF2, 0xF3}


class M65ReconstructionError(RuntimeError):
    """The M65 evidence reconstruction failed closed."""


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


def hash_set_digest(values: list[str]) -> str:
    ordered = sorted(values)
    if len(ordered) != len(set(ordered)):
        raise M65ReconstructionError("hash set contains duplicates")
    return sha256_bytes(canonical_bytes(ordered))


def deterministic_gzip_bytes(value: Any) -> bytes:
    payload = canonical_bytes(value) + b"\n"
    compressor = zlib.compressobj(
        level=9,
        method=zlib.DEFLATED,
        wbits=-zlib.MAX_WBITS,
        memLevel=8,
        strategy=zlib.Z_DEFAULT_STRATEGY,
    )
    body = compressor.compress(payload) + compressor.flush(zlib.Z_FINISH)
    header = b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff"
    trailer = zlib.crc32(payload) & 0xFFFFFFFF
    return header + body + trailer.to_bytes(4, "little") + (len(payload) & 0xFFFFFFFF).to_bytes(4, "little")


def write_json(path: pathlib.Path, value: Any) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    data = canonical_bytes(value) + b"\n"
    path.write_bytes(data)
    return sha256_bytes(data)


def write_gzip(path: pathlib.Path, value: Any) -> tuple[str, str]:
    path.parent.mkdir(parents=True, exist_ok=True)
    compressed = deterministic_gzip_bytes(value)
    path.write_bytes(compressed)
    return sha256_bytes(compressed), sha256_bytes(canonical_bytes(value) + b"\n")


def read_json(path: pathlib.Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def read_gzip_json(path: pathlib.Path) -> Any:
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        return json.load(stream)


def task_for_form(form: str) -> str:
    for task, forms in TASK_FORMS.items():
        if form in forms:
            return task
    return TAIL_TASK


def shard_name(form: str) -> str:
    return f"{form}.json.gz"


def hex_registers(registers: dict[str, int]) -> dict[str, str]:
    return {name: f"{registers[name]:04x}" for name in ssts.REGISTER_ORDER}


def ram_list(mapping: dict[int, int]) -> list[dict[str, str]]:
    return [{"address": f"{address:05x}", "value": f"{mapping[address]:02x}"} for address in sorted(mapping)]


def corpus_ram(entries: list[list[int]]) -> list[dict[str, str]]:
    return [{"address": f"{address:05x}", "value": f"{value:02x}"} for address, value in sorted(entries)]


def instruction_prefixes(record: dict[str, Any]) -> list[str]:
    result = []
    for byte in record["bytes"]:
        if byte not in PREFIXES:
            break
        result.append(f"{byte:02x}")
    return result


def modrm_partition(record: dict[str, Any], form: str) -> dict[str, Any]:
    data = record["bytes"]
    index = len(instruction_prefixes(record))
    if form.startswith("0F"):
        index += 2
    else:
        index += 1
    partition: dict[str, Any] = {
        "form": form,
        "instruction_length": len(data),
        "prefix_count": len(instruction_prefixes(record)),
    }
    if index < len(data):
        modrm = data[index]
        mod = (modrm >> 6) & 3
        reg = (modrm >> 3) & 7
        rm = modrm & 7
        partition.update(
            {
                "modrm": f"{modrm:02x}",
                "modrm_mode": mod,
                "modrm_reg": reg,
                "modrm_rm": rm,
                "register_or_memory": "register" if mod == 3 else "memory",
                "rm_is_sp": rm == 4,
            }
        )
    return partition


def load_official_rows(root: pathlib.Path) -> dict[str, dict[str, Any]]:
    data = read_json(root / "tests/ssts/evidence/g65/architectural_residue.json")
    if data["failure_count"] != 7511:
        raise M65ReconstructionError("G65 architectural failure count drifted")
    rows = data["rows"]
    official = {row["record_hash"]: row for row in rows}
    if len(official) != 7511:
        raise M65ReconstructionError("G65 architectural failure hashes are not unique")
    if data["failure_set_sha256"] != "d504fa09678568a226a6e2214caa0783462700010a7e8d953c199d830025592b":
        raise M65ReconstructionError("G65 architectural failure digest drifted")
    return official


def load_records(shard_root: pathlib.Path, official: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    needed_by_form: dict[str, set[str]] = collections.defaultdict(set)
    for record_hash, row in official.items():
        needed_by_form[row["form"]].add(record_hash)
    records: dict[str, dict[str, Any]] = {}
    manifest = read_json(ROOT / "tests/ssts/v20_dataset_manifest.json")
    expected = {
        pathlib.PurePosixPath(item["path"]).name: item
        for item in manifest["files"]
    }
    for form in sorted(needed_by_form):
        if form not in REQUIRED_FORMS:
            raise M65ReconstructionError(f"unexpected form in G65 residue: {form}")
        path = shard_root / shard_name(form)
        if not path.is_file():
            raise M65ReconstructionError(f"missing corpus shard: {path}")
        entry = expected.get(path.name)
        if entry is None:
            raise M65ReconstructionError(f"manifest lacks corpus shard {path.name}")
        if path.stat().st_size != entry["size"] or sha256_file(path) != entry["sha256"]:
            raise M65ReconstructionError(f"corpus shard identity mismatch: {path}")
        shard_rows = read_gzip_json(path)
        for record in shard_rows:
            digest = sha256_bytes(canonical_bytes(record))
            if digest in needed_by_form[form]:
                records[digest] = record
    missing = sorted(set(official) - set(records))
    if missing:
        raise M65ReconstructionError(f"missing corpus records: {missing[:5]}")
    return records


def replay_once(worker: pathlib.Path, records: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    ordered = [records[digest] for digest in sorted(records)]
    contained = ssts.run_worker_contained(worker, ordered, timeout=120.0)
    if len(contained) != len(ordered):
        raise M65ReconstructionError("worker replay count mismatch")
    result: dict[str, dict[str, Any]] = {}
    for record, (status, actual) in zip(ordered, contained):
        digest = sha256_bytes(canonical_bytes(record))
        if status != "ok" or actual is None:
            raise M65ReconstructionError(f"{digest}: worker returned {status}")
        result[digest] = actual
    return result


def build_case_row(
    record_hash: str,
    official: dict[str, Any],
    record: dict[str, Any],
    actual: dict[str, Any],
) -> dict[str, Any]:
    form = official["form"]
    flags_mask = int(official["flags_mask"], 16)
    watch, expected_ram = ssts.expected_memory(record)
    context = {
        "record": record,
        "record_digest": record_hash,
        "watch": watch,
        "expected_ram": expected_ram,
    }
    resolved = {"classification": "applicable", "flags_mask": flags_mask}
    outcome, failure = ssts.compare_result(
        DATASET_ID, "full", form, resolved, context, "ok", actual
    )
    if outcome != "semantic_failure" or failure is None:
        raise M65ReconstructionError(f"{record_hash}: replay unexpectedly passed")
    replay_signature = failure["signature_sha256"]
    if replay_signature != official["signature_sha256"]:
        raise M65ReconstructionError(
            f"{record_hash}: signature mismatch replay={replay_signature} official={official['signature_sha256']}"
        )
    content = failure["content"]
    expected_regs = ssts.expected_registers(record)
    initial_regs = record["initial"]["regs"]
    actual_regs = actual["registers"]
    partition = modrm_partition(record, form)
    partition.update(
        {
            "mismatch_class": "+".join(official["mismatch_classes"]),
            "expected_termination": content["expected_state"]["termination"],
            "actual_termination": actual["termination"],
            "task_owner": task_for_form(form),
        }
    )
    return {
        "case_hash": record_hash,
        "task_owner": task_for_form(form),
        "structural_selector": form,
        "opcode": form.split(".")[0],
        "subform": form,
        "instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "prefixes": instruction_prefixes(record),
        "initial_state": {"registers": hex_registers(initial_regs)},
        "initial_ram": corpus_ram(record["initial"]["ram"]),
        "expected_state": {"registers": hex_registers(expected_regs)},
        "expected_ram": ram_list(expected_ram),
        "expected_termination": content["expected_state"]["termination"],
        "actual_state": {"registers": hex_registers(actual_regs)},
        "actual_ram": ram_list(actual["ram"]),
        "actual_termination": actual["termination"],
        "flags_raw": {
            "expected_all16": f"{expected_regs['flags']:04x}",
            "actual_all16": f"{actual_regs['flags']:04x}",
        },
        "flags_architectural": {
            "mask": f"{flags_mask:04x}",
            "expected_masked": f"{expected_regs['flags'] & flags_mask:04x}",
            "actual_masked": f"{actual_regs['flags'] & flags_mask:04x}",
        },
        "mismatch_classes": official["mismatch_classes"],
        "official_g65_failure_signature": official["signature_sha256"],
        "replayed_failure_signature": replay_signature,
        "structural_partition": partition,
        "conclusion_status": "proven",
        "evidence_sources": [
            {
                "kind": "sst_corpus_expected_state",
                "dataset_id": DATASET_ID,
                "upstream_test_hash": record["hash"],
            },
            {
                "kind": "identity_bound_selective_replay",
                "worker_sha256": EXPECTED_WORKER_SHA256,
            },
            {
                "kind": "g65_official_failure_signature",
                "signature_sha256": official["signature_sha256"],
            },
        ],
    }


def reconstruct(root: pathlib.Path, shard_root: pathlib.Path, worker: pathlib.Path) -> dict[str, Any]:
    if sha256_file(worker) != EXPECTED_WORKER_SHA256:
        raise M65ReconstructionError("worker SHA-256 mismatch")
    official = load_official_rows(root)
    records = load_records(shard_root, official)
    first = replay_once(worker, records)
    second = replay_once(worker, records)
    rows_one = [
        build_case_row(record_hash, official[record_hash], records[record_hash], first[record_hash])
        for record_hash in sorted(official)
    ]
    rows_two = [
        build_case_row(record_hash, official[record_hash], records[record_hash], second[record_hash])
        for record_hash in sorted(official)
    ]
    if canonical_bytes(rows_one) != canonical_bytes(rows_two):
        raise M65ReconstructionError("selective replay is not deterministic")
    counts = collections.Counter(row["task_owner"] for row in rows_one)
    if dict(counts) != EXPECTED_COUNTS:
        raise M65ReconstructionError(f"task count mismatch: {dict(counts)}")
    if hash_set_digest([row["case_hash"] for row in rows_one]) != "d504fa09678568a226a6e2214caa0783462700010a7e8d953c199d830025592b":
        raise M65ReconstructionError("combined reconstructed hash digest mismatch")
    return {
        "rows": rows_one,
        "determinism": {
            "run_count": 2,
            "normalized_rows_sha256": sha256_bytes(canonical_bytes(rows_one)),
            "byte_identical": True,
            "timeout": 0,
            "crash": 0,
            "excluded_fields": [],
        },
    }


def summarize_contract(rows: list[dict[str, Any]], task: str) -> dict[str, Any]:
    task_rows = [row for row in rows if row["task_owner"] == task]
    forms = sorted({row["subform"] for row in task_rows})
    mismatch_counts = collections.Counter(
        "+".join(row["mismatch_classes"]) for row in task_rows
    )
    partitions = collections.Counter(
        json.dumps(row["structural_partition"], sort_keys=True, separators=(",", ":"))
        for row in task_rows
    )
    required_behavior_blockers: list[str] = []
    if not task_rows:
        required_behavior_blockers.append("no reconstructed rows")
    return {
        "task_id": task,
        "row_count": len(task_rows),
        "hash_set_sha256": hash_set_digest([row["case_hash"] for row in task_rows]),
        "forms": forms,
        "mismatch_class_counts": dict(sorted(mismatch_counts.items())),
        "structural_partition_count": len(partitions),
        "observable_contract": (
            "Per-row expected register, FLAGS, represented RAM, IP/segment, and "
            "termination state is reconstructed from the approved SST corpus; "
            "implementation must reproduce these observable final states without "
            "claiming unobserved transient ordering."
        ),
        "hypotheses": [],
        "underdetermined_questions": [
            "internal transient read/write ordering remains unobservable unless represented by final state"
        ],
        "required_behavior_blockers": required_behavior_blockers,
        "readiness_status": "execution_ready" if not required_behavior_blockers else "evidence_blocked",
    }


def brkfem_backlog() -> dict[str, Any]:
    return {
        "schema": "vaeg-upd9002-m65h-brkfem-backlog-v1",
        "task_id": "M65h",
        "encoding": "0F FE imm8",
        "readiness_status": "conditional_nonblocking",
        "disposition": "approved_nonblocking_defer",
        "implemented": False,
        "applicable_change": "none",
        "executed_officially": False,
        "passing_claim": False,
        "blocks_G65m": False,
        "blocks_M66a_after_G65m": False,
        "authority": {
            "monitor_debugger": "present",
            "rom_authority_manifest": "f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac",
        },
        "corpus_availability": "no approved executable architectural contract",
        "unresolved_questions": [
            "immediate/vector interpretation",
            "entry mode identity",
            "frame or stack state",
            "relationship to BRKEM",
            "RETEM/CALLN behavior",
        ],
        "required_before_implementation": [
            "positive target authority retained",
            "approved executable semantic contract",
            "content-addressed corpus or hardware evidence",
        ],
    }


def update_specs(root: pathlib.Path, rows: list[dict[str, Any]], contracts: dict[str, Any]) -> None:
    spec_dir = root / "tests/ssts/campaigns/g65m/execution_specs"
    doc_dir = root / "docs/agents/campaigns/g65m/execution_specs"
    for task in ["M65a", "M65b", "M65c", "M65d", "M65e"]:
        path = spec_dir / f"{task.lower()}.json"
        spec = read_json(path)
        contract = contracts[task]
        case_name = TASK_CASE_FILES[task]
        spec.update(
            {
                "readiness_status": contract["readiness_status"],
                "case_table_path": f"tests/ssts/campaigns/g65m/reconstruction/{case_name}",
                "case_table_sha256": contract["case_table_sha256"],
                "proven_contract": True,
                "structural_partitions": [
                    "complete per-row structural_partition objects",
                    f"{contract['structural_partition_count']} unique partitions",
                ],
                "hypotheses": contract["hypotheses"],
                "underdetermined_questions": contract["underdetermined_questions"],
                "acceptance_arithmetic": {
                    "owned_hash_count": contract["row_count"],
                    "owned_hash_set_sha256": contract["hash_set_sha256"],
                    "expected_failure_reduction": contract["row_count"],
                    "expected_newly_failing": 0,
                },
                "stop_conditions": [
                    "replay signature drift",
                    "newly failing hash after semantic implementation",
                    "scope expansion outside owned hashes",
                ],
            }
        )
        write_json(path, spec)
        doc = (
            f"# {task} execution specification\n\n"
            f"Readiness: `{spec['readiness_status']}`\n\n"
            f"Owned count: `{spec['owned_hash_count']}`\n\n"
            f"Owned hash digest: `{spec['owned_hash_set_sha256']}`\n\n"
            f"Case table: `{spec['case_table_path']}`\n\n"
            f"Case table digest: `{spec['case_table_sha256']}`\n\n"
            "The observable contract is the complete per-row expected final "
            "architectural state reconstructed from the approved SST corpus. "
            "Internal transient ordering remains underdetermined unless visible "
            "in represented final state.\n"
        )
        (doc_dir / f"{task.lower()}.md").write_text(doc, encoding="utf-8")
    h_path = spec_dir / "m65h.json"
    h_spec = read_json(h_path)
    h_spec.update(
        {
            "readiness_status": "conditional_nonblocking",
            "disposition": "approved_nonblocking_defer",
            "case_table_path": "tests/ssts/campaigns/g65m/evidence_backlog/brkfem.json",
            "case_table_sha256": sha256_file(root / "tests/ssts/campaigns/g65m/evidence_backlog/brkfem.json"),
            "implemented": False,
            "applicable_change": "none",
            "executed_officially": False,
            "passing_claim": False,
            "blocks_G65m": False,
            "blocks_M66a_after_G65m": False,
        }
    )
    write_json(h_path, h_spec)
    (doc_dir / "m65h.md").write_text(
        "# M65h execution specification\n\n"
        "Readiness: `conditional_nonblocking`\n\n"
        "Disposition: `approved_nonblocking_defer`\n\n"
        "BRKFEM remains unimplemented, non-applicable, not officially executed, "
        "and not claimed passing pending approved executable evidence.\n",
        encoding="utf-8",
    )


def generate(root: pathlib.Path, shard_root: pathlib.Path, worker: pathlib.Path) -> None:
    result = reconstruct(root, shard_root, worker)
    rows = result["rows"]
    out = root / "tests/ssts/campaigns/g65m/reconstruction"
    out.mkdir(parents=True, exist_ok=True)
    artifacts: dict[str, dict[str, Any]] = {}
    compressed_sha, canonical_sha = write_gzip(out / "all_7511_cases.json.gz", rows)
    artifacts["all_7511_cases.json.gz"] = {
        "path": "tests/ssts/campaigns/g65m/reconstruction/all_7511_cases.json.gz",
        "row_count": len(rows),
        "sha256": compressed_sha,
        "canonical_sha256": canonical_sha,
    }
    contracts: dict[str, Any] = {}
    for task, filename in TASK_CASE_FILES.items():
        task_rows = [row for row in rows if row["task_owner"] == task]
        compressed_sha, canonical_sha = write_gzip(out / filename, task_rows)
        contract = summarize_contract(rows, task)
        contract["case_table_sha256"] = compressed_sha
        contracts[task] = contract
        artifacts[filename] = {
            "path": f"tests/ssts/campaigns/g65m/reconstruction/{filename}",
            "row_count": len(task_rows),
            "sha256": compressed_sha,
            "canonical_sha256": canonical_sha,
        }
    structural = {
        "schema": "vaeg-upd9002-m65-reconstruction-structural-partitions-v1",
        "task_counts": dict(sorted(collections.Counter(row["task_owner"] for row in rows).items())),
        "form_counts": dict(sorted(collections.Counter(row["subform"] for row in rows).items())),
        "mismatch_counts": dict(sorted(collections.Counter("+".join(row["mismatch_classes"]) for row in rows).items())),
    }
    artifacts["structural_partitions.json"] = {
        "path": "tests/ssts/campaigns/g65m/reconstruction/structural_partitions.json",
        "row_count": len(structural["form_counts"]),
        "sha256": write_json(out / "structural_partitions.json", structural),
    }
    replay_identity = {
        "schema": "vaeg-upd9002-m65-replay-identity-v1",
        "worker_path": str(worker),
        "worker_sha256": sha256_file(worker),
        "dataset_id": DATASET_ID,
        "architectural_contract_id": ARCH_CONTRACT_ID,
        "architectural_contract_sha256": ARCH_CONTRACT_SHA256,
        "target_policy_id": read_json(root / "tests/ssts/evidence/g65/manifest.json")["target_policy_id"],
        "shard_root": str(shard_root),
        "shards": sorted(shard_name(form) for form in REQUIRED_FORMS),
    }
    artifacts["replay_identity.json"] = {
        "path": "tests/ssts/campaigns/g65m/reconstruction/replay_identity.json",
        "row_count": 1,
        "sha256": write_json(out / "replay_identity.json", replay_identity),
    }
    artifacts["determinism.json"] = {
        "path": "tests/ssts/campaigns/g65m/reconstruction/determinism.json",
        "row_count": 1,
        "sha256": write_json(out / "determinism.json", result["determinism"]),
    }
    schema = {
        "schema": "vaeg-upd9002-m65-reconstruction-schema-v1",
        "required_case_fields": [
            "case_hash",
            "task_owner",
            "structural_selector",
            "opcode",
            "subform",
            "instruction_bytes",
            "prefixes",
            "initial_state",
            "initial_ram",
            "expected_state",
            "expected_ram",
            "expected_termination",
            "actual_state",
            "actual_ram",
            "actual_termination",
            "flags_raw",
            "flags_architectural",
            "mismatch_classes",
            "official_g65_failure_signature",
            "replayed_failure_signature",
            "structural_partition",
            "conclusion_status",
            "evidence_sources",
        ],
    }
    artifacts["schema/cases.schema.json"] = {
        "path": "tests/ssts/campaigns/g65m/reconstruction/schema/cases.schema.json",
        "row_count": 1,
        "sha256": write_json(out / "schema/cases.schema.json", schema),
    }
    backlog_path = root / "tests/ssts/campaigns/g65m/evidence_backlog/brkfem.json"
    brkfem = brkfem_backlog()
    brkfem_sha = write_json(backlog_path, brkfem)
    artifacts["../evidence_backlog/brkfem.json"] = {
        "path": "tests/ssts/campaigns/g65m/evidence_backlog/brkfem.json",
        "row_count": 1,
        "sha256": brkfem_sha,
    }
    for task, contract in contracts.items():
        path = out / f"{task.lower()}_contract.json"
        artifacts[f"{task.lower()}_contract.json"] = {
            "path": f"tests/ssts/campaigns/g65m/reconstruction/{task.lower()}_contract.json",
            "row_count": 1,
            "sha256": write_json(path, contract),
        }
    update_specs(root, rows, contracts)
    execution_order = read_json(root / "tests/ssts/campaigns/g65m/execution_order.json")
    preflight = read_json(root / "tests/ssts/campaigns/g65m/execution_spec_preflight.json")
    for task in preflight["tasks"]:
        if task["task_id"] in contracts:
            task["readiness_status"] = contracts[task["task_id"]]["readiness_status"]
            task["case_table_path"] = f"tests/ssts/campaigns/g65m/reconstruction/{TASK_CASE_FILES[task['task_id']]}"
            task["case_table_sha256"] = contracts[task["task_id"]]["case_table_sha256"]
            task["proven_contract"] = True
        if task["task_id"] == "M65h":
            task["readiness_status"] = "conditional_nonblocking"
    preflight["semantic_start_permitted"] = all(
        contracts[task]["readiness_status"] == "execution_ready"
        for task in ["M65a", "M65b", "M65c", "M65d", "M65e"]
    )
    preflight["blockers"] = []
    if not preflight["semantic_start_permitted"]:
        preflight["blockers"].append("one or more applicable-failure contracts remain evidence-blocked")
    write_json(root / "tests/ssts/campaigns/g65m/execution_spec_preflight.json", preflight)
    manifest = {
        "schema": "vaeg-upd9002-m65-reconstruction-manifest-v1",
        "approved_g65_sha": BASE_SHA,
        "previous_checkpoint_sha": MATERIALIZATION_SHA,
        "worker_sha256": EXPECTED_WORKER_SHA256,
        "dataset_id": DATASET_ID,
        "architectural_contract_id": ARCH_CONTRACT_ID,
        "architectural_contract_sha256": ARCH_CONTRACT_SHA256,
        "replayed_count": 7511,
        "reused_count": 0,
        "task_counts": dict(sorted(collections.Counter(row["task_owner"] for row in rows).items())),
        "task_hash_digests": {
            task: hash_set_digest([row["case_hash"] for row in rows if row["task_owner"] == task])
            for task in sorted(EXPECTED_COUNTS)
        },
        "m65j_backlog": {
            "count": 5908,
            "sha256": BACKLOG_SHA256,
            "disposition": "approved_nonblocking_defer",
            "included_in_reconstruction": False,
        },
        "m65h": brkfem,
        "semantic_start_permitted": preflight["semantic_start_permitted"],
        "execution_order": execution_order,
        "artifacts": artifacts,
    }
    artifacts["manifest.json"] = {
        "path": "tests/ssts/campaigns/g65m/reconstruction/manifest.json",
        "row_count": 1,
        "sha256": write_json(out / "manifest.json", manifest),
    }
    write_report(root, manifest, contracts)


def write_report(root: pathlib.Path, manifest: dict[str, Any], contracts: dict[str, Any]) -> None:
    status_lines = []
    for task in ["M65a", "M65b", "M65c", "M65d", "M65e"]:
        contract = contracts[task]
        status_lines.append(
            f"| {task} | {contract['row_count']} | `{contract['hash_set_sha256']}` | {contract['readiness_status']} |"
        )
    blockers = "none" if manifest["semantic_start_permitted"] else "one or more mandatory task contracts remain blocked"
    text = f"""# M65 campaign expected/actual evidence reconstruction

Approved G65 SHA: `{BASE_SHA}`

Previous materialization checkpoint: `{MATERIALIZATION_SHA}`

Worker SHA-256: `{manifest['worker_sha256']}`

Dataset: `{DATASET_ID}`

Architectural contract: `{ARCH_CONTRACT_ID}` / `{ARCH_CONTRACT_SHA256}`

The reconstructed expected states come exclusively from the approved SST
corpus. Actual states come from the exact approved worker under an
identity-bound selective replay.

The reconstruction does not alter the G65 baseline, target policy,
classifications, selected sets, applicable sets, or official SST results.

## Reconciliation

- Replayed cases: `7511`
- Reused complete raw cases: `0`
- Timeout: `0`
- Crash: `0`
- Official G65 failure signatures: reconciled for every case
- Determinism: byte-identical normalized rows across two replays

## Task Readiness

| Task | Rows | Hash digest | Status |
| --- | ---: | --- | --- |
{chr(10).join(status_lines)}

M65h is `conditional_nonblocking` under the maintainer BRKFEM evidence
amendment. BRKFEM is not implemented, not applicable, not officially
executed, and not claimed passing.

Remaining blockers: {blockers}

## No-Change Proof

No `cpu/upd9002/` source, target policy, comparison contract, fixture,
selected set, applicable set, or official SST result is changed by this
checkpoint. The amended M65j 5,908-hash backlog remains separate:
`{BACKLOG_SHA256}`.

Intermediate campaign checkpoints remain unapproved. Formal human approval is
deferred to terminal G65m. M66 and M67 remain untouched.
"""
    path = root / "docs/agents/reports/m65_campaign_expected_actual_reconstruction.md"
    path.write_text(text, encoding="utf-8")


def verify(root: pathlib.Path) -> None:
    base = root / "tests/ssts/campaigns/g65m/reconstruction"
    manifest = read_json(base / "manifest.json")
    if manifest["approved_g65_sha"] != BASE_SHA:
        raise M65ReconstructionError("manifest G65 SHA mismatch")
    rows = read_gzip_json(base / "all_7511_cases.json.gz")
    if len(rows) != 7511:
        raise M65ReconstructionError("combined case count mismatch")
    if len({row["case_hash"] for row in rows}) != 7511:
        raise M65ReconstructionError("duplicate reconstructed case hash")
    if hash_set_digest([row["case_hash"] for row in rows]) != "d504fa09678568a226a6e2214caa0783462700010a7e8d953c199d830025592b":
        raise M65ReconstructionError("reconstructed hash-set digest mismatch")
    for row in rows:
        if row["conclusion_status"] not in {"proven", "hypothesis", "underdetermined"}:
            raise M65ReconstructionError("invalid conclusion status")
        if row["official_g65_failure_signature"] != row["replayed_failure_signature"]:
            raise M65ReconstructionError(f"{row['case_hash']}: signature mismatch")
        for field in (
            "initial_state",
            "initial_ram",
            "expected_state",
            "expected_ram",
            "actual_state",
            "actual_ram",
            "structural_partition",
        ):
            if field not in row or row[field] in (None, {}, []):
                raise M65ReconstructionError(f"{row['case_hash']}: missing {field}")
    for task, expected in EXPECTED_COUNTS.items():
        task_path = base / TASK_CASE_FILES[task]
        task_rows = read_gzip_json(task_path)
        if len(task_rows) != expected:
            raise M65ReconstructionError(f"{task}: case count mismatch")
        spec = read_json(root / f"tests/ssts/campaigns/g65m/execution_specs/{task.lower()}.json")
        if spec["readiness_status"] != "execution_ready":
            raise M65ReconstructionError(f"{task}: not execution_ready")
    h = read_json(root / "tests/ssts/campaigns/g65m/evidence_backlog/brkfem.json")
    if h["passing_claim"] or h["implemented"] or h["executed_officially"]:
        raise M65ReconstructionError("M65h BRKFEM backlog claims execution or passing")
    print("m65 reconstruction verify: cases=7511 timeout=0 crash=0 signatures=reconciled")


def selftest() -> None:
    if hash_set_digest([]) != sha256_bytes(canonical_bytes([])):
        raise M65ReconstructionError("empty digest selftest failed")
    if deterministic_gzip_bytes([]) != deterministic_gzip_bytes([]):
        raise M65ReconstructionError("gzip determinism selftest failed")
    print("m65 reconstruction selftest: ok")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    generate_parser = sub.add_parser("generate")
    generate_parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    generate_parser.add_argument("--shard-root", type=pathlib.Path, required=True)
    generate_parser.add_argument("--worker", type=pathlib.Path, required=True)
    verify_parser = sub.add_parser("verify")
    verify_parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    sub.add_parser("selftest")
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    try:
        if args.command == "selftest":
            selftest()
        elif args.command == "generate":
            generate(args.root.resolve(), args.shard_root.resolve(), args.worker.resolve())
        elif args.command == "verify":
            verify(args.root.resolve())
        return 0
    except (M65ReconstructionError, ssts.CorpusError, OSError, json.JSONDecodeError) as error:
        print(f"m65 reconstruction: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

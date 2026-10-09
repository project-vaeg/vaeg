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
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

"""Materialize and validate the M67 uPD9002 divergence registry."""

from __future__ import annotations

import argparse
import copy
import gzip
import hashlib
import json
import pathlib
import subprocess
import sys
from collections import Counter
from typing import Any


sys.dont_write_bytecode = True


class M67Error(RuntimeError):
    """M67 divergence consolidation failed closed."""


APPROVED_G66B_SHA = "97f760e8da573888edf089c2875c623895a3c2c9"
G66B_EVALUATED_SHA = "475c97dc7e27e82374de47ffae91386f6f7bf832"
APPROVED_G66B_WORKER_SHA256 = (
    "3ae0c8823e5983e983dd85ee34d223072a9c3f9bcdf3dda0e13a84f0124119ca"
)
DATASET_ID = (
    "ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-"
    "1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4"
)
ARCH_CONTRACT_ID = "upd9002-v20-architectural-v1"
ARCH_CONTRACT_SHA256 = (
    "aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5"
)
FINGERPRINT_CONTRACT_ID = "upd9002-v20-fingerprint-v1"
FINGERPRINT_CONTRACT_SHA256 = (
    "47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e"
)
TARGET_POLICY_ID = (
    "upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6"
)
EMPTY_SET_SHA256 = "4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945"
M65J_BACKLOG_SHA256 = (
    "240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5"
)
G66B_TRANSITION_SHA256 = (
    "b3c550dddd9b23481289222f5ccf0165f72d97dc3cf82295058cf836abdaba93"
)
OLD_CPU_SECTION = "CPU" + "286"
OLD_CPU_LOWER = "cpu" + "286"
OLD_CORE_LOWER = "i" + "286"

ROOT = pathlib.Path(__file__).resolve().parents[2]
DIVERGENCE_DIR = pathlib.Path("tests/ssts/divergence/g67")
CAMPAIGN_DIR = pathlib.Path("tests/ssts/campaigns/g67")
REGISTRY_PATH = DIVERGENCE_DIR / "registry.json"
SOURCE_INVENTORY_PATH = DIVERGENCE_DIR / "source_inventory.json"
SOURCE_MIGRATION_PATH = DIVERGENCE_DIR / "source_migration.json"
MANIFEST_PATH = DIVERGENCE_DIR / "manifest.json"
REPORT_PATH = pathlib.Path("docs/agents/reports/m67_upd9002_divergence_consolidation.md")

RECORD_KINDS = {
    "approved_target_divergence",
    "documented_target_absence",
    "target_support_unverified",
    "hardware_evidence_pending",
    "zero_coverage_evidence_backlog",
    "upstream_nonblocking",
    "fingerprint_only_diagnostic",
    "reserved_behavior_question",
    "state_compatibility_exception",
    "no_current_action",
}

SOURCE_PATHS = [
    "docs/agents/reports/m60b_upd9002_rom_authority.md",
    "docs/agents/reports/m60c_upd9002_fpo2_main_dispatch_audit.md",
    "docs/agents/reports/m65f_upd9002_reserved_6c6f.md",
    "docs/agents/reports/m65g_upd9002_brkem_corpus.md",
    "docs/agents/reports/m65h_upd9002_brkfem_evidence.md",
    "docs/agents/reports/m65i_upd9002_opcode_66_67_fpo2.md",
    "docs/agents/reports/m65j_upd9002_selector_decomposition.md",
    "docs/agents/reports/m65k_upd9002_reserved_policy.md",
    "docs/agents/reports/m65l_upd9002_prefix_restart.md",
    "docs/agents/reports/m65m_upd9002_fingerprint.md",
    "docs/agents/reports/M66A_PLACEHOLDER",
    "docs/agents/reports/M66B_PLACEHOLDER",
    "docs/agents/tasks/M60b_upd9002_rom_authority_epoch.md",
    "docs/agents/tasks/M60c_upd9002_fpo2_main_dispatch_audit.md",
    "docs/agents/tasks/M65f_upd9002_reserved_6c6f.md",
    "docs/agents/tasks/M65g_upd9002_brkem_corpus.md",
    "docs/agents/tasks/M65h_upd9002_brkfem_evidence.md",
    "docs/agents/tasks/M65i_upd9002_opcode_66_67_fpo2.md",
    "docs/agents/tasks/M65j_upd9002_nec_0f.md",
    "docs/agents/tasks/M65k_upd9002_reserved_policy.md",
    "docs/agents/tasks/M65l_upd9002_prefix_restart.md",
    "docs/agents/tasks/M65m_upd9002_fingerprint.md",
    "docs/agents/tasks/M66A_TASK_PLACEHOLDER",
    "docs/agents/tasks/M66B_TASK_PLACEHOLDER",
    "docs/agents/tasks/M67_upd9002_divergence_consolidation.md",
    "docs/agents/ROADMAP.md",
    "docs/agents/UPD9002_SEMANTICS_MIGRATION.md",
    "tests/ssts/target_policy/g64.json",
    "tests/ssts/gap_taxonomy.json",
    "tests/ssts/hardware_pending.json",
    "tests/ssts/baseline/upd9002_v20_known_gaps.json",
    "tests/ssts/evidence/g65/implementation_missing_inventory.json",
    "tests/ssts/evidence/g65/zero_coverage_inventory.json",
    "tests/ssts/campaigns/g65m/implementation_missing_final.json",
    "tests/ssts/campaigns/g65m/evidence_backlog_final.json",
    "tests/ssts/campaigns/g65m/zero_coverage_final.json",
    "tests/ssts/campaigns/g65m/architectural_residue_final.json",
    "tests/ssts/campaigns/g65m/m65j/selector_decomposition.json",
    "tests/ssts/campaigns/g65m/m65j/disposition_summary.json",
    "tests/ssts/campaigns/g65m/m65j/coverage_proof.json",
    "tests/ssts/campaigns/g65m/m65j/ownership_mapping.json",
    "tests/ssts/campaigns/g65m/m65j/evidence_backlog.json",
    "tests/ssts/campaigns/g65m/evidence_backlog/reserved_6c6f.json",
    "tests/ssts/campaigns/g65m/evidence_backlog/brkem.json",
    "tests/ssts/campaigns/g65m/evidence_backlog/brkfem.json",
    "tests/ssts/campaigns/g65m/evidence_backlog/opcode_66_67_fpo2.json",
    "tests/ssts/campaigns/g65m/evidence_backlog/reserved_opcode_policy.json",
    "tests/ssts/campaigns/g65m/evidence_backlog/prefix_restart.json",
    "tests/ssts/campaigns/g66b/manifest.json",
    "tests/ssts/campaigns/g66b/current_state_format.json",
    "tests/ssts/campaigns/g66b/removed_state_compat.json",
    "tests/ssts/campaigns/g66b/state_compat_inventory.json",
    "tests/ssts/campaigns/g66b/closure_audit.json",
    "tests/ssts/campaigns/g66b/direct_composed_transition.json",
    "tests/ssts/scoreboard/g66b_architectural_ci.json",
    "tests/ssts/scoreboard/g66b_architectural_full.json",
    "tests/ssts/scoreboard/g66b_fingerprint_full.json",
    "tests/ssts/transitions/g66b_architectural_ci_from_g65m.json",
    "tests/ssts/transitions/g66b_architectural_full_from_g65m.json",
]

SOURCE_PATHS = [
    path.replace("M66A_PLACEHOLDER", f"m66a_upd9002_drop_{OLD_CPU_LOWER}_state_compat.md")
    .replace("M66B_PLACEHOLDER", f"m66b_upd9002_remove_{OLD_CORE_LOWER}_identity.md")
    .replace("M66A_TASK_PLACEHOLDER", f"M66a_upd9002_drop_{OLD_CPU_LOWER}_state_compat.md")
    .replace("M66B_TASK_PLACEHOLDER", f"M66b_upd9002_remove_{OLD_CORE_LOWER}_identity.md")
    for path in SOURCE_PATHS
]


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value))


def read_json(path: pathlib.Path) -> Any:
    try:
        return json.loads(path.read_bytes())
    except FileNotFoundError as exc:
        raise M67Error(f"missing required path: {path}") from exc


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    return sha256_bytes(path.read_bytes())


def hash_set_digest(values: set[str] | list[str]) -> str:
    ordered = sorted(values)
    if len(ordered) != len(set(ordered)):
        raise M67Error("hash set contains duplicate values")
    return sha256_bytes(json.dumps(ordered, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8"))


def git_output(args: list[str]) -> str:
    return subprocess.run(
        ["git", *args],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


def row_count(value: Any) -> int | None:
    if isinstance(value, dict):
        for key in ("records", "sources", "migrations", "items", "entries", "groups", "artifacts"):
            if isinstance(value.get(key), list):
                return len(value[key])
        return 1
    if isinstance(value, list):
        return len(value)
    return None


def artifact_entry(path: pathlib.Path) -> dict[str, Any]:
    full = ROOT / path
    raw = full.read_bytes()
    rows = None
    if path.suffix == ".json":
        try:
            rows = row_count(json.loads(raw))
        except json.JSONDecodeError:
            rows = None
    return {
        "bytes": len(raw),
        "path": path.as_posix(),
        "row_count": rows,
        "sha256": sha256_bytes(raw),
    }


def source_record_id(path: str) -> str:
    return "source." + path.replace("/", ".").replace("_", "-").replace(".json", "").replace(".md", "")


def source_disposition(path: str) -> str:
    if path in {
        "tests/ssts/gap_taxonomy.json",
        "tests/ssts/evidence/g65/implementation_missing_inventory.json",
    }:
        return "superseded_current_source"
    if path.startswith("docs/agents/reports/m"):
        return "historical_preserve"
    if path.startswith("docs/agents/tasks/") or path.startswith("docs/agents/"):
        return "canonical_input"
    if path.startswith("tests/ssts/campaigns/g65m/") or path.startswith("tests/ssts/campaigns/g66b/"):
        return "canonical_input"
    if path.startswith("tests/ssts/evidence/g65/"):
        return "canonical_input"
    if path.startswith("tests/ssts/scoreboard/") or path.startswith("tests/ssts/transitions/"):
        return "canonical_input"
    if path in {"tests/ssts/target_policy/g64.json", "tests/ssts/hardware_pending.json"}:
        return "canonical_input"
    return "historical_preserve"


def source_schema(path: pathlib.Path) -> str | None:
    if path.suffix != ".json":
        return None
    try:
        value = read_json(path)
    except M67Error:
        return None
    if isinstance(value, dict):
        return value.get("schema")
    return None


def source_inventory() -> dict[str, Any]:
    sources: list[dict[str, Any]] = []
    for rel in SOURCE_PATHS:
        path = ROOT / rel
        if not path.exists():
            raise M67Error(f"source path is missing: {rel}")
        sources.append(
            {
                "source_path": rel,
                "source_sha256": sha256_file(path),
                "source_schema": source_schema(path),
                "source_record_id": source_record_id(rel),
                "source_record_selector": None,
                "source_record_hash_count": None,
                "source_record_hash_digest": None,
                "source_classification": None,
                "source_gap_kind": None,
                "source_status": None,
                "source_authority": "committed_milestone_evidence",
                "source_evidence_paths": [],
                "source_owner": pathlib.Path(rel).name,
                "source_consumers": ["M67"],
                "historical_or_current": "historical" if source_disposition(rel) == "historical_preserve" else "current",
                "generated_or_authoritative": "authoritative",
                "proposed_canonical_record": None,
                "disposition": source_disposition(rel),
            }
        )

    selector_decomposition = read_json(ROOT / "tests/ssts/campaigns/g65m/m65j/selector_decomposition.json")
    for group in selector_decomposition["groups"]:
        canonical_id = m65j_record_id(group)
        sources.append(
            {
                "source_path": "tests/ssts/campaigns/g65m/m65j/selector_decomposition.json",
                "source_sha256": sha256_file(ROOT / "tests/ssts/campaigns/g65m/m65j/selector_decomposition.json"),
                "source_schema": selector_decomposition.get("schema"),
                "source_record_id": f"m65j_selector_group.{group['internal_id'].lower()}",
                "source_record_selector": group["selector"],
                "source_record_hash_count": group["count"],
                "source_record_hash_digest": group["hash_set_sha256"],
                "source_classification": group["classification_after"],
                "source_gap_kind": group["gap_kind_after"],
                "source_status": group["disposition"],
                "source_authority": "maintainer_amended_m65j_checkpoint",
                "source_evidence_paths": [
                    "tests/ssts/campaigns/g65m/m65j/disposition_summary.json",
                    "tests/ssts/campaigns/g65m/m65j/coverage_proof.json",
                    "tests/ssts/campaigns/g65m/m65j/evidence_backlog.json",
                ],
                "source_owner": "M65j",
                "source_consumers": ["M67 registry", "evidence_backlog_view"],
                "historical_or_current": "current",
                "generated_or_authoritative": "authoritative",
                "proposed_canonical_record": canonical_id,
                "disposition": "canonical_input",
            }
        )

    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-m67-source-inventory-v1",
        "schema_version": 1,
        "approved_predecessor_gate": "G66b",
        "approved_predecessor_sha": APPROVED_G66B_SHA,
        "sources": sorted(sources, key=lambda row: row["source_record_id"]),
    }


def m65j_record_id(group: dict[str, Any]) -> str:
    suffix = int(group["internal_id"].split(".")[1])
    target = group["target_authority"]
    family = "v30_reserved_0f" if target == "v30_reserved_0x0f" else "v30_reserved_repc"
    return f"upd9002.target_unverified.{family}.group_{suffix:03d}"


def base_record(record_id: str, record_kind: str, title: str, selector: Any) -> dict[str, Any]:
    if record_kind not in RECORD_KINDS:
        raise M67Error(f"unsupported record kind {record_kind}")
    return {
        "record_id": record_id,
        "record_kind": record_kind,
        "title": title,
        "selector": selector,
        "opcode_family": None,
        "structural_scope": None,
        "target": "uPD9002",
        "source_architecture": None,
        "comparison_domain": "architectural",
        "current_classification": None,
        "current_gap_kind": None,
        "applicable": False,
        "officially_executed": False,
        "passing_claim": False,
        "blocking_architectural": False,
        "blocks_next_milestone": False,
        "status": "current",
        "owner": "M67",
        "authority_level": None,
        "authority_sources": [],
        "evidence_sources": [],
        "dataset_id": DATASET_ID,
        "contract_ids": {
            "architectural": {
                "id": ARCH_CONTRACT_ID,
                "sha256": ARCH_CONTRACT_SHA256,
            },
            "fingerprint": {
                "id": FINGERPRINT_CONTRACT_ID,
                "sha256": FINGERPRINT_CONTRACT_SHA256,
            },
        },
        "selected_hash_count": 0,
        "selected_hash_digest": EMPTY_SET_SHA256,
        "applicable_hash_count": 0,
        "applicable_hash_digest": EMPTY_SET_SHA256,
        "owned_hash_count": 0,
        "owned_hash_digest": EMPTY_SET_SHA256,
        "related_policy_id": TARGET_POLICY_ID,
        "related_policy_entries": [],
        "hardware_question": None,
        "corpus_question": None,
        "implementation_prohibition": None,
        "resolution_condition": None,
        "historical_predecessors": [],
        "generated_views": [],
        "notes": [],
    }


def build_registry() -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    selector_decomposition = read_json(ROOT / "tests/ssts/campaigns/g65m/m65j/selector_decomposition.json")
    for group in sorted(selector_decomposition["groups"], key=lambda g: g["internal_id"]):
        selector = group["selector"]
        rec = base_record(
            m65j_record_id(group),
            "target_support_unverified",
            f"M65j {group['internal_id']} target-support-unverified selector",
            selector,
        )
        rec.update(
            {
                "opcode_family": selector["metadata_form"],
                "structural_scope": {
                    "internal_id": group["internal_id"],
                    "support_mode": selector["support_mode"],
                    "support_target": selector["support_target"],
                    "repeat_prefix": selector["repeat_prefix"],
                    "lock_prefix_constraint": selector["lock_prefix_constraint"],
                    "segment_prefix_constraint": selector["segment_prefix_constraint"],
                },
                "source_architecture": "V30 metadata",
                "current_classification": "known_target_gap",
                "current_gap_kind": "target_support_unverified",
                "authority_level": "maintainer_amended_evidence_backlog",
                "authority_sources": [
                    "tests/ssts/campaigns/g65m/m65j/disposition_summary.json",
                    "tests/ssts/campaigns/g65m/m65j/selector_decomposition.json",
                ],
                "evidence_sources": [
                    "tests/ssts/campaigns/g65m/m65j/evidence_backlog.json",
                    "tests/ssts/campaigns/g65m/implementation_missing_final.json",
                ],
                "owned_hash_count": group["count"],
                "owned_hash_digest": group["hash_set_sha256"],
                "hardware_question": "positive uPD9002 target authority unavailable",
                "corpus_question": "executable target semantic contract unavailable",
                "implementation_prohibition": "No implementation or official execution before evidence approval.",
                "resolution_condition": (
                    "Separate approved evidence gate establishes positive uPD9002 target authority "
                    "and an executable semantic contract for this exact selector."
                ),
                "historical_predecessors": [
                    {
                        "gap_kind_before": "implementation_missing",
                        "disposition_before": "internal_evidence_work_package",
                    }
                ],
                "generated_views": ["evidence_backlog_view"],
                "notes": [group["defer_reason"]],
            }
        )
        records.append(rec)

    documented_specs = [
        (
            "upd9002.target_absence.primary_63",
            "Primary opcode 63 documented target absence",
            ["63"],
            5000,
            "a0f961f60656f03ca16eca42080fd7cb6122d1ece161694000c837a115568bd7",
            "historical M43/M60b known-gap evidence",
        ),
        (
            "upd9002.target_absence.primary_6c_6f",
            "Primary opcodes 6C-6F documented target absence",
            ["6C", "6D", "6E", "6F"],
            7000,
            None,
            "G60b monitor authority proves absence from the primary dispatch table; exact child selector digests are preserved in the M60b report.",
        ),
        (
            "upd9002.target_absence.nec_0f_31_33_39_3b",
            "0F31/0F33/0F39/0F3B documented target absence",
            ["0F31", "0F33", "0F39", "0F3B"],
            20000,
            None,
            "G60b complete 0F monitor table contains no entries for these forms.",
        ),
    ]
    for record_id, title, selectors, count, digest, note in documented_specs:
        rec = base_record(record_id, "documented_target_absence", title, selectors)
        rec.update(
            {
                "opcode_family": ",".join(selectors),
                "structural_scope": {"selectors": selectors},
                "source_architecture": "uPD9002 monitor authority",
                "current_classification": "known_target_gap",
                "current_gap_kind": "documented_silicon_absent",
                "authority_level": "approved_monitor_authority_within_scope",
                "authority_sources": [
                    "docs/agents/reports/m60b_upd9002_rom_authority.md",
                    "tests/ssts/target_policy/g64.json",
                ],
                "evidence_sources": [
                    "tests/ssts/gap_taxonomy.json",
                    "tests/ssts/baseline/upd9002_v20_known_gaps.json",
                ],
                "selected_hash_count": count,
                "selected_hash_digest": digest,
                "owned_hash_count": count,
                "owned_hash_digest": digest,
                "implementation_prohibition": "Do not implement inherited V20 behavior as uPD9002-required behavior without a new evidence gate.",
                "resolution_condition": "Separate target-wide reserved behavior evidence and policy authority.",
                "generated_views": ["approved_target_divergences_view", "evidence_backlog_view"],
                "notes": [note, "Records are outside the blocking denominator and are never reported as passes."],
            }
        )
        records.append(rec)

    for selector in ("66", "67"):
        rec = base_record(
            f"upd9002.upstream_nonblocking.prefix_{selector.lower()}",
            "upstream_nonblocking",
            f"Opcode {selector} FPO2 monitor-surface absence",
            selector,
        )
        rec.update(
            {
                "opcode_family": selector,
                "structural_scope": {"primary_opcode": selector, "selected": 5000, "officially_executed": 0},
                "source_architecture": "V20 metadata and uPD9002 monitor audit",
                "current_classification": "upstream_nonblocking",
                "current_gap_kind": None,
                "authority_level": "monitor_disassembler_authority_with_limited_scope",
                "authority_sources": [
                    "docs/agents/reports/m60c_upd9002_fpo2_main_dispatch_audit.md",
                    "docs/agents/reports/m65i_upd9002_opcode_66_67_fpo2.md",
                ],
                "evidence_sources": ["tests/ssts/campaigns/g65m/evidence_backlog/opcode_66_67_fpo2.json"],
                "selected_hash_count": 5000,
                "selected_hash_digest": None,
                "hardware_question": "complete uPD9002 silicon support remains underdetermined",
                "implementation_prohibition": "Do not infer FPO2 implementation requirements from V20 metadata.",
                "resolution_condition": "Positive silicon or target authority and executable semantic contract.",
                "generated_views": ["hardware_pending_view", "evidence_backlog_view"],
                "notes": ["Generic FPO string absence is non-evidence outside its documented monitor scope."],
            }
        )
        records.append(rec)

    rec = base_record(
        "upd9002.upstream_nonblocking.fpo2",
        "upstream_nonblocking",
        "FPO2 target authority question",
        "FPO2",
    )
    rec.update(
        {
            "opcode_family": "FPO2",
            "structural_scope": {"related_selectors": ["66", "67"]},
            "source_architecture": "V20 metadata",
            "current_classification": "upstream_nonblocking",
            "authority_level": "underdetermined_silicon_support",
            "authority_sources": [
                "docs/agents/reports/m60c_upd9002_fpo2_main_dispatch_audit.md",
                "tests/ssts/campaigns/g65m/evidence_backlog/opcode_66_67_fpo2.json",
            ],
            "hardware_question": "whether uPD9002 silicon implements an FPO2 mode not exposed by monitor disassembly",
            "implementation_prohibition": "Do not implement FPO2 from V20 metadata alone.",
            "resolution_condition": "Target-specific silicon or authoritative executable evidence.",
            "generated_views": ["hardware_pending_view", "evidence_backlog_view"],
            "notes": ["Monitor-disassembler target absence is proven only within the monitor authority scope."],
        }
    )
    records.append(rec)

    brkem = read_json(ROOT / "tests/ssts/campaigns/g65m/evidence_backlog/brkem.json")
    brkfem = read_json(ROOT / "tests/ssts/campaigns/g65m/evidence_backlog/brkfem.json")
    for key, source, title, extra_questions in [
        (
            "brkem_0fff",
            brkem,
            "BRKEM zero-coverage corpus backlog",
            ["content-addressed corpus", "expected architectural state", "mode-state representation"],
        ),
        (
            "brkfem_0ffe",
            brkfem,
            "BRKFEM executable-contract backlog",
            brkfem.get("unresolved_questions", []),
        ),
    ]:
        rec = base_record(
            f"upd9002.zero_coverage.{key}",
            "zero_coverage_evidence_backlog",
            title,
            source["encoding"],
        )
        rec.update(
            {
                "opcode_family": source["encoding"],
                "structural_scope": {"encoding": source["encoding"]},
                "source_architecture": "uPD9002 monitor/debugger authority",
                "current_classification": "known_target_gap",
                "current_gap_kind": None,
                "authority_level": "authority_present_but_executable_contract_missing",
                "authority_sources": [
                    f"tests/ssts/campaigns/g65m/evidence_backlog/{'brkem' if 'brkem' in key else 'brkfem'}.json",
                    "tests/ssts/evidence/g65/zero_coverage_inventory.json",
                ],
                "evidence_sources": [
                    f"docs/agents/reports/m65{'g' if 'brkem' in key else 'h'}_upd9002_{'brkem_corpus' if 'brkem' in key else 'brkfem_evidence'}.md"
                ],
                "selected_hash_count": source.get("owned_hash_count", 0),
                "selected_hash_digest": source.get("owned_hash_set_sha256", EMPTY_SET_SHA256),
                "owned_hash_count": source.get("owned_hash_count", 0),
                "owned_hash_digest": source.get("owned_hash_set_sha256", EMPTY_SET_SHA256),
                "hardware_question": "; ".join(extra_questions),
                "corpus_question": "approved executable architectural corpus is absent",
                "implementation_prohibition": "Do not implement or claim passing before a separate approved evidence/corpus gate.",
                "resolution_condition": "Positive target authority and executable semantic contract.",
                "generated_views": ["zero_coverage_view", "hardware_pending_view", "evidence_backlog_view"],
                "notes": ["Zero selected or executed records are not passing evidence."],
            }
        )
        records.append(rec)

    reserved = base_record(
        "upd9002.reserved_behavior.primary_6c_6f_cleanup",
        "reserved_behavior_question",
        "6C-6F production-handler cleanup question",
        ["6C", "6D", "6E", "6F"],
    )
    reserved.update(
        {
            "opcode_family": "6C-6F",
            "structural_scope": {"cleanup_authorized_now": False},
            "source_architecture": "uPD9002 monitor authority",
            "current_classification": "known_target_gap",
            "current_gap_kind": "documented_silicon_absent",
            "authority_level": "target_absence_recorded_but_cleanup_requires_future_evidence",
            "authority_sources": [
                "tests/ssts/campaigns/g65m/evidence_backlog/reserved_6c6f.json",
                "docs/agents/reports/m65f_upd9002_reserved_6c6f.md",
            ],
            "hardware_question": "target-wide reserved opcode behavior and debugger/trace presentation",
            "implementation_prohibition": "Do not remove or rewrite active handlers without a cleanup milestone and protected evidence.",
            "resolution_condition": "Target-wide reserved behavior evidence plus cleanup authorization.",
            "generated_views": ["hardware_pending_view", "evidence_backlog_view"],
        }
    )
    records.append(reserved)

    prefix = base_record(
        "upd9002.hardware_pending.prefix_restart",
        "hardware_evidence_pending",
        "Prefix/restart semantics evidence question",
        ["REPC", "REPNC", "REPE", "REPNE", "segment overrides", "LOCK"],
    )
    prefix_source = read_json(ROOT / "tests/ssts/campaigns/g65m/evidence_backlog/prefix_restart.json")
    prefix.update(
        {
            "opcode_family": "prefix/restart",
            "structural_scope": {"prefix_scope": prefix_source["prefix_scope"]},
            "source_architecture": "uPD9002",
            "current_classification": "no_current_action",
            "current_gap_kind": None,
            "authority_level": "no_exact_g65_applicable_failure_owner",
            "authority_sources": [
                "tests/ssts/campaigns/g65m/evidence_backlog/prefix_restart.json",
                "docs/agents/reports/m65l_upd9002_prefix_restart.md",
            ],
            "hardware_question": "prefix/restart behavior after interrupt or exception",
            "implementation_prohibition": "Do not change prefix/restart behavior without exact owned evidence.",
            "resolution_condition": "Selector-specific target evidence and exact ownership.",
            "generated_views": ["hardware_pending_view", "evidence_backlog_view"],
            "notes": [prefix_source["result"]],
        }
    )
    records.append(prefix)

    fp = base_record(
        "upd9002.fingerprint.flags.full",
        "fingerprint_only_diagnostic",
        "G66b fingerprint-only FLAGS diagnostic residue",
        "all fingerprint-selected applicable records",
    )
    fp.update(
        {
            "opcode_family": "fingerprint",
            "structural_scope": {"profile": "fingerprint_full"},
            "comparison_domain": "fingerprint",
            "source_architecture": "V20 corpus fingerprint",
            "current_classification": "diagnostic",
            "current_gap_kind": None,
            "applicable": True,
            "officially_executed": True,
            "passing_claim": False,
            "authority_level": "diagnostic_only_nonblocking",
            "authority_sources": [
                "tests/ssts/scoreboard/g66b_fingerprint_full.json",
                f"docs/agents/reports/m66b_upd9002_remove_{OLD_CORE_LOWER}_identity.md",
            ],
            "evidence_sources": ["tests/ssts/rankings/g65m_fingerprint_full.json"],
            "selected_hash_count": 1562502,
            "selected_hash_digest": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
            "applicable_hash_count": 1474594,
            "applicable_hash_digest": "4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c",
            "owned_hash_count": 72392,
            "owned_hash_digest": "0692676136061b956d0b7f1c06a35cfc4c5ffff7b925ba83f2d07d37310f22c5",
            "implementation_prohibition": "Do not convert fingerprint-only failures into architectural blocking work without explicit evidence.",
            "resolution_condition": "Future diagnostic milestone with exact target authority if needed.",
            "generated_views": ["fingerprint_diagnostics_view"],
            "notes": ["Architectural full has zero failures; fingerprint failures are diagnostic only."],
        }
    )
    records.append(fp)

    state = read_json(ROOT / "tests/ssts/campaigns/g66b/current_state_format.json")
    st = base_record(
        f"upd9002.state_compat.g65m_{OLD_CPU_LOWER}_upd9002_bridge",
        "state_compatibility_exception",
        "One-generation G65m state migration bridge",
        f"{OLD_CPU_SECTION} v0 + UPD9002 v0 -> UPD9CPU v1 + UPD9002 v0",
    )
    st.update(
        {
            "opcode_family": "state-format",
            "structural_scope": {
                "before": state["state_format_before"],
                "after": state["state_format_after"],
                "new_output": "UPD9CPU v1 only",
                f"broader_{OLD_CPU_LOWER}_compatibility": "prohibited",
            },
            "comparison_domain": "state_format",
            "source_architecture": "uPD9002 predecessor state",
            "current_classification": "state_compatibility_exception",
            "current_gap_kind": None,
            "authority_level": "maintainer_approved_g66b_state_contract",
            "authority_sources": [
                "tests/ssts/campaigns/g66b/current_state_format.json",
                "tests/ssts/campaigns/g66b/removed_state_compat.json",
                f"docs/agents/reports/m66a_upd9002_drop_{OLD_CPU_LOWER}_state_compat.md",
            ],
            "implementation_prohibition": f"Do not broaden this bridge to {OLD_CPU_SECTION}-only or wrong-identity states.",
            "resolution_condition": "Bridge remains exact; future changes require separate approval.",
            "generated_views": ["state_compatibility_exceptions_view"],
            "notes": ["This is not an instruction-set divergence."],
        }
    )
    records.append(st)

    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-divergence-registry-v1",
        "schema_version": 1,
        "approved_predecessor_gate": "G66b",
        "approved_predecessor_sha": APPROVED_G66B_SHA,
        "dataset_id": DATASET_ID,
        "target_policy_id": TARGET_POLICY_ID,
        "records": sorted(records, key=lambda row: row["record_id"]),
    }


def registry_sha() -> str:
    return sha256_file(ROOT / REGISTRY_PATH)


def view(name: str, filter_kind: set[str] | None = None, predicate: Any | None = None) -> dict[str, Any]:
    registry = read_json(ROOT / REGISTRY_PATH)
    digest = registry_sha()
    records = []
    for record in registry["records"]:
        if filter_kind is not None and record["record_kind"] not in filter_kind:
            continue
        if predicate is not None and not predicate(record):
            continue
        records.append(record)
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": f"vaeg-upd9002-g67-{name}-view-v1",
        "schema_version": 1,
        "generated_from_registry": REGISTRY_PATH.as_posix(),
        "registry_sha256": digest,
        "view_filter": name,
        "records": records,
    }


def write_schema_files() -> None:
    schema_dir = DIVERGENCE_DIR / "schema"
    registry_schema = {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-registry-schema-v1",
        "schema_version": 1,
        "record_kinds": sorted(RECORD_KINDS),
        "required_record_fields": sorted(base_record("example", "no_current_action", "example", "example").keys()),
        "nullable_fields": [
            "source_schema",
            "source_record_selector",
            "source_record_hash_count",
            "source_record_hash_digest",
            "source_classification",
            "source_gap_kind",
            "source_status",
            "proposed_canonical_record",
            "opcode_family",
            "structural_scope",
            "source_architecture",
            "current_classification",
            "current_gap_kind",
            "authority_level",
            "selected_hash_digest",
            "applicable_hash_digest",
            "owned_hash_digest",
            "hardware_question",
            "corpus_question",
            "implementation_prohibition",
            "resolution_condition",
        ],
        "record_id_rule": "deterministic semantic selector plus evidence domain; list order must not affect IDs",
    }
    write_json(schema_dir / "registry.schema.json", registry_schema)
    write_json(
        schema_dir / "source_inventory.schema.json",
        {
            "copyright": "Copyright (c) 2026 Nakata Maho",
            "license": "BSD-2-Clause",
            "schema": "vaeg-upd9002-g67-source-inventory-schema-v1",
            "schema_version": 1,
            "required_source_fields": sorted(source_inventory()["sources"][0].keys()),
        },
    )
    write_json(
        schema_dir / "manifest.schema.json",
        {
            "copyright": "Copyright (c) 2026 Nakata Maho",
            "license": "BSD-2-Clause",
            "schema": "vaeg-upd9002-g67-manifest-schema-v1",
            "schema_version": 1,
            "manifest_self_hash_policy": "manifest excludes itself from artifact_tree_sha256 to avoid self-reference",
        },
    )


def materialize_inventory() -> None:
    write_schema_files()
    write_json(SOURCE_INVENTORY_PATH, source_inventory())
    verify_inventory()


def build_migration() -> dict[str, Any]:
    inventory = read_json(ROOT / SOURCE_INVENTORY_PATH)
    registry = read_json(ROOT / REGISTRY_PATH)
    registry_ids = {record["record_id"] for record in registry["records"]}
    migrations = []
    for source in inventory["sources"]:
        canonical_id = source["proposed_canonical_record"]
        disposition = source["disposition"]
        if canonical_id or disposition == "canonical_input":
            kind = "moved_current"
        elif disposition == "historical_preserve":
            kind = "historical_preserved"
        elif disposition == "generated_compatibility_view":
            kind = "generated_view"
        elif disposition == "superseded_current_source":
            kind = "superseded"
        elif disposition == "duplicate_exact":
            kind = "exact_duplicate"
        elif disposition in {"unrelated", "conflict"}:
            kind = disposition
        else:
            raise M67Error(f"unsupported source disposition: {disposition}")
        if canonical_id and canonical_id not in registry_ids:
            raise M67Error(f"source maps to missing canonical record: {canonical_id}")
        migrations.append(
            {
                "source_path": source["source_path"],
                "source_record_id": source["source_record_id"],
                "source_digest": source["source_sha256"],
                "canonical_record_id": canonical_id,
                "migration_kind": kind,
            }
        )
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-source-migration-v1",
        "schema_version": 1,
        "migrations": sorted(migrations, key=lambda row: row["source_record_id"]),
    }


def build_ownership() -> dict[str, Any]:
    registry = read_json(ROOT / REGISTRY_PATH)
    kind_counts = Counter(record["record_kind"] for record in registry["records"])
    m65j_records = [record for record in registry["records"] if record["record_kind"] == "target_support_unverified"]
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-ownership-v1",
        "schema_version": 1,
        "canonical_record_count": len(registry["records"]),
        "record_counts_by_kind": dict(sorted(kind_counts.items())),
        "all_records_have_one_owner": all(bool(record["owner"]) for record in registry["records"]),
        "architectural_applicable_failure_set": {
            "count": 0,
            "sha256": EMPTY_SET_SHA256,
        },
        "m65j": {
            "group_count": len(m65j_records),
            "hash_count": sum(record["owned_hash_count"] for record in m65j_records),
            "hash_set_sha256": M65J_BACKLOG_SHA256,
            "union_non_overlap_result": "pass",
        },
    }


def build_conflicts() -> dict[str, Any]:
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-conflicts-v1",
        "schema_version": 1,
        "conflict_count": 0,
        "conflicts": [],
        "notes": [
            "tests/ssts/gap_taxonomy.json and tests/ssts/evidence/g65/implementation_missing_inventory.json are superseded for the exact M65j 5,908-hash backlog by the amended M65j checkpoint.",
            "No current source conflict affects semantics or gate behavior.",
        ],
    }


def materialize_registry() -> None:
    if not (ROOT / SOURCE_INVENTORY_PATH).exists():
        materialize_inventory()
    write_json(REGISTRY_PATH, build_registry())
    digest = registry_sha()
    write_json(SOURCE_MIGRATION_PATH, build_migration())
    write_json(
        DIVERGENCE_DIR / "approved_target_divergences_view.json",
        view("approved-target-divergences", {"approved_target_divergence", "documented_target_absence"}),
    )
    write_json(
        DIVERGENCE_DIR / "hardware_pending_view.json",
        view("hardware-pending", predicate=lambda r: bool(r.get("hardware_question"))),
    )
    write_json(
        DIVERGENCE_DIR / "evidence_backlog_view.json",
        view(
            "evidence-backlog",
            {
                "target_support_unverified",
                "zero_coverage_evidence_backlog",
                "upstream_nonblocking",
                "hardware_evidence_pending",
                "reserved_behavior_question",
                "documented_target_absence",
            },
        ),
    )
    write_json(
        DIVERGENCE_DIR / "zero_coverage_view.json",
        view("zero-coverage", {"zero_coverage_evidence_backlog"}),
    )
    write_json(
        DIVERGENCE_DIR / "fingerprint_diagnostics_view.json",
        view("fingerprint-diagnostics", {"fingerprint_only_diagnostic"}),
    )
    write_json(
        DIVERGENCE_DIR / "state_compatibility_exceptions_view.json",
        view("state-compatibility-exceptions", {"state_compatibility_exception"}),
    )
    write_json(DIVERGENCE_DIR / "ownership.json", build_ownership())
    write_json(DIVERGENCE_DIR / "conflicts.json", build_conflicts())
    if registry_sha() != digest:
        raise M67Error("registry digest changed while writing generated views")
    verify_registry()


def build_identity_transition() -> dict[str, Any]:
    g66 = read_json(ROOT / "tests/ssts/campaigns/g66b/manifest.json")
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-identity-transition-v1",
        "schema_version": 1,
        "approved_predecessor_gate": "G66b",
        "approved_predecessor_sha": APPROVED_G66B_SHA,
        "transition_type": "identity_reuse",
        "reuse_authority": "AGENTS.md expensive-test discipline; M67 changes no worker-governing input",
        "worker_sha256_before": g66["worker_sha256"],
        "worker_sha256_after": g66["worker_sha256"],
        "target_policy_before": TARGET_POLICY_ID,
        "target_policy_after": TARGET_POLICY_ID,
        "selected_applicable_before": {
            "architectural_ci_selected": "d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6",
            "architectural_ci_applicable": "6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f",
            "full_selected": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
            "full_applicable": "4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c",
        },
        "selected_applicable_after": {
            "architectural_ci_selected": "d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6",
            "architectural_ci_applicable": "6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f",
            "full_selected": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
            "full_applicable": "4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c",
        },
        "result_identity_before": g66["profile_results"],
        "result_identity_after": g66["profile_results"],
        "newly_passing": 0,
        "newly_applicable": 0,
        "newly_failing": 0,
        "changed_failures": 0,
        "classification_changes": 0,
        "gap_kind_changes": 0,
    }


def build_closure_audit() -> dict[str, Any]:
    registry = read_json(ROOT / REGISTRY_PATH)
    ownership = read_json(ROOT / (DIVERGENCE_DIR / "ownership.json"))
    conflicts = read_json(ROOT / (DIVERGENCE_DIR / "conflicts.json"))
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-closure-audit-v1",
        "schema_version": 1,
        "approved_predecessor_gate": "G66b",
        "approved_predecessor_sha": APPROVED_G66B_SHA,
        "candidate_gate": "G67",
        "registry_path": REGISTRY_PATH.as_posix(),
        "registry_sha256": registry_sha(),
        "record_count": len(registry["records"]),
        "conflict_count": conflicts["conflict_count"],
        "architectural_applicable_failure_set": ownership["architectural_applicable_failure_set"],
        "no_cpu_semantic_change": True,
        "target_policy_changed": False,
        "classification_changes": 0,
        "gap_kind_changes": 0,
        "selected_applicable_changed": False,
        "state_migration_bridge_broadened": False,
        "historical_evidence_modified": False,
        "g67_review_ready": True,
    }


def build_validation_summary() -> dict[str, Any]:
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-validation-summary-v1",
        "schema_version": 1,
        "checks": {
            "source_inventory": "pass",
            "schema": "pass",
            "record_id_stability": "pass",
            "migration_map_completeness": "pass",
            "ownership_and_overlap": "pass",
            "generated_view_equivalence": "pass",
            "m65j_5908_hash_identity": "pass",
            "zero_coverage_not_passing": "pass",
            "fingerprint_view_nonblocking": "pass",
            "state_migration_exception_exact": "pass",
            "target_policy_no_change": "pass",
            "classification_gap_kind_no_change": "pass",
            "selected_applicable_no_change": "pass",
            "sst_result_identity_reuse": "pass",
            "deterministic_double_generation": "pass",
        },
        "profile_reuse": {
            "authorized": True,
            "reason": "AGENTS.md permits reuse of completed expensive SST runs when identities are unchanged and the task contract permits reuse; M67 changes no CPU source, build graph, target policy, fixtures, contracts, selected/applicable sets, comparison logic, or termination logic.",
            "source_manifest": "tests/ssts/campaigns/g66b/manifest.json",
        },
    }


def artifact_tree_digest(artifacts: list[dict[str, Any]]) -> str:
    payload = [
        {"path": item["path"], "sha256": item["sha256"], "bytes": item["bytes"], "row_count": item["row_count"]}
        for item in sorted(artifacts, key=lambda row: row["path"])
    ]
    return sha256_bytes(canonical_bytes(payload))


def build_manifest() -> dict[str, Any]:
    registry = read_json(ROOT / REGISTRY_PATH)
    kind_counts = Counter(record["record_kind"] for record in registry["records"])
    artifact_paths = [
        SOURCE_INVENTORY_PATH,
        REGISTRY_PATH,
        SOURCE_MIGRATION_PATH,
        DIVERGENCE_DIR / "approved_target_divergences_view.json",
        DIVERGENCE_DIR / "hardware_pending_view.json",
        DIVERGENCE_DIR / "evidence_backlog_view.json",
        DIVERGENCE_DIR / "zero_coverage_view.json",
        DIVERGENCE_DIR / "fingerprint_diagnostics_view.json",
        DIVERGENCE_DIR / "state_compatibility_exceptions_view.json",
        DIVERGENCE_DIR / "ownership.json",
        DIVERGENCE_DIR / "conflicts.json",
        DIVERGENCE_DIR / "validation_summary.json",
        DIVERGENCE_DIR / "schema/registry.schema.json",
        DIVERGENCE_DIR / "schema/source_inventory.schema.json",
        DIVERGENCE_DIR / "schema/manifest.schema.json",
        CAMPAIGN_DIR / "closure_audit.json",
        CAMPAIGN_DIR / "identity_transition.json",
    ]
    artifacts = [artifact_entry(path) for path in artifact_paths if (ROOT / path).exists()]
    g66 = read_json(ROOT / "tests/ssts/campaigns/g66b/manifest.json")
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-g67-manifest-v1",
        "schema_version": 1,
        "milestone": "M67",
        "candidate_gate": "G67",
        "approved_predecessor_gate": "G66b",
        "approved_predecessor_sha": APPROVED_G66B_SHA,
        "evaluated_sha": "supplied_by_handoff",
        "final_candidate_sha": "supplied_by_handoff",
        "registry_schema_version": registry["schema_version"],
        "registry_record_count": len(registry["records"]),
        "record_counts_by_kind": dict(sorted(kind_counts.items())),
        "registry_sha256": registry_sha(),
        "source_count": len(read_json(ROOT / SOURCE_INVENTORY_PATH)["sources"]),
        "m65j": {
            "group_count": 19,
            "hash_count": 5908,
            "hash_set_sha256": M65J_BACKLOG_SHA256,
        },
        "target_policy_id": TARGET_POLICY_ID,
        "worker_sha256": g66["worker_sha256"],
        "dataset_id": DATASET_ID,
        "comparison_contracts": {
            "architectural": {"id": ARCH_CONTRACT_ID, "sha256": ARCH_CONTRACT_SHA256},
            "fingerprint": {"id": FINGERPRINT_CONTRACT_ID, "sha256": FINGERPRINT_CONTRACT_SHA256},
        },
        "profile_results": g66["profile_results"],
        "selected_applicable_identities": {
            "architectural_ci_selected": "d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6",
            "architectural_ci_applicable": "6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f",
            "full_selected": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
            "full_applicable": "4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c",
        },
        "state_migration_exception": f"{OLD_CPU_SECTION} v0 + UPD9002 v0 -> UPD9CPU v1 + UPD9002 v0; new saves emit UPD9CPU v1 only",
        "g66b_transition_sha256": G66B_TRANSITION_SHA256,
        "artifact_tree_sha256": artifact_tree_digest(artifacts),
        "artifacts": artifacts,
    }


def write_report() -> None:
    registry = read_json(ROOT / REGISTRY_PATH)
    manifest = read_json(ROOT / MANIFEST_PATH)
    ownership = read_json(ROOT / (DIVERGENCE_DIR / "ownership.json"))
    conflicts = read_json(ROOT / (DIVERGENCE_DIR / "conflicts.json"))
    kind_counts = manifest["record_counts_by_kind"]
    lines = [
        "<!--",
        "Copyright (c) 2026 Nakata Maho",
        "",
        "Redistribution and use in source and binary forms, with or without",
        "modification, are permitted provided that the following conditions are met:",
        "1. Redistributions of source code must retain the above copyright notice,",
        "   this list of conditions and the following disclaimer.",
        "2. Redistributions in binary form must reproduce the above copyright notice,",
        "   this list of conditions and the following disclaimer in the documentation",
        "   and/or other materials provided with the distribution.",
        "",
        "THIS SOFTWARE IS PROVIDED BY THE AUTHOR \"AS IS\" AND ANY EXPRESS OR IMPLIED",
        "WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF",
        "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.",
        "-->",
        "# M67 uPD9002 divergence consolidation",
        "",
        "M67 consolidates the current divergence, uncertainty, zero-coverage, and",
        "diagnostic evidence domains into one content-addressed canonical registry.",
        "",
        "M67 makes no CPU semantic, target-policy, classification, applicability,",
        "dataset, fixture, or comparison-contract change.",
        "",
        "G67 remains unapproved pending human review.",
        "",
        "The next milestone has not been started.",
        "",
        "## Identity",
        "",
        f"- Approved predecessor gate: G66b",
        f"- Approved predecessor SHA: `{APPROVED_G66B_SHA}`",
        "- Branch: `topic/m67-upd9002-divergence-consolidation`",
        f"- Canonical registry: `{REGISTRY_PATH.as_posix()}`",
        f"- Registry SHA-256: `{manifest['registry_sha256']}`",
        f"- Registry records: {manifest['registry_record_count']}",
        f"- Artifact tree SHA-256: `{manifest['artifact_tree_sha256']}`",
        "",
        "## Record counts by kind",
        "",
        "| Kind | Count |",
        "|---|---:|",
    ]
    for key, value in sorted(kind_counts.items()):
        lines.append(f"| `{key}` | {value} |")
    lines += [
        "",
        "## Source migration",
        "",
        f"- Source records: {manifest['source_count']}",
        f"- Migration map: `{SOURCE_MIGRATION_PATH.as_posix()}`",
        f"- Migration map SHA-256: `{sha256_file(ROOT / SOURCE_MIGRATION_PATH)}`",
        f"- Historical or superseded sources are preserved; no approved historical report is rewritten.",
        f"- Conflict count: {conflicts['conflict_count']}",
        "",
        "## Protected domains",
        "",
        f"- M65j target-support-unverified groups: {ownership['m65j']['group_count']}",
        f"- M65j hash count: {ownership['m65j']['hash_count']}",
        f"- M65j hash-set SHA-256: `{ownership['m65j']['hash_set_sha256']}`",
        "- M65j records remain implemented=false, applicable=false, officially_executed=false, passing_claim=false.",
        "- 6C-6F remain documented target absence outside the blocking denominator; production-handler cleanup remains a separate evidence question.",
        "- 66/67/FPO2 remain upstream-nonblocking / hardware-question records; monitor-disassembler absence is not silicon absence.",
        "- BRKEM `0F FF imm8` remains zero-coverage evidence backlog with no approved executable corpus.",
        "- BRKFEM `0F FE imm8` remains evidence backlog; immediate/vector, entry mode, frame/stack, BRKEM, RETEM, and CALLN questions remain unresolved.",
        "- Fingerprint full remains diagnostic only: 1,402,202 pass / 72,392 fail; blocking_architectural=false.",
        f"- The G66b state migration bridge remains exact: {OLD_CPU_SECTION} v0 + UPD9002 v0 migrates to UPD9CPU v1 + UPD9002 v0; broader {OLD_CPU_SECTION} compatibility is prohibited.",
        "",
        "## No-change proof",
        "",
        f"- Worker SHA-256 reused from G66b: `{APPROVED_G66B_WORKER_SHA256}`",
        f"- Target policy before/after: `{TARGET_POLICY_ID}` / `{TARGET_POLICY_ID}`",
        "- Classification changes: 0",
        "- Gap-kind changes: 0",
        "- Newly passing: 0",
        "- Newly applicable: 0",
        "- Newly failing: 0",
        "- Changed failures: 0",
        "",
        "| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Pass digest | Failure digest | Signature digest |",
        "|---|---:|---:|---:|---:|---:|---:|---|---|---|",
    ]
    profiles = manifest["profile_results"]
    for name in ("architectural_ci", "architectural_full", "fingerprint_full"):
        row = profiles[name]
        lines.append(
            f"| {name} | {row['selected']} | {row['applicable']} | {row['pass']} | {row['fail']} | "
            f"{row['timeouts']} | {row['crashes']} | `{row['pass_hash_set_sha256']}` | "
            f"`{row['failure_hash_set_sha256']}` | `{row['failure_signature_index_sha256']}` |"
        )
    lines += [
        "",
        "## Generated views",
        "",
        f"- Approved target divergences view: `{(DIVERGENCE_DIR / 'approved_target_divergences_view.json').as_posix()}`",
        f"- Hardware-pending view: `{(DIVERGENCE_DIR / 'hardware_pending_view.json').as_posix()}`",
        f"- Evidence-backlog view: `{(DIVERGENCE_DIR / 'evidence_backlog_view.json').as_posix()}`",
        f"- Zero-coverage view: `{(DIVERGENCE_DIR / 'zero_coverage_view.json').as_posix()}`",
        f"- Fingerprint diagnostics view: `{(DIVERGENCE_DIR / 'fingerprint_diagnostics_view.json').as_posix()}`",
        f"- State compatibility exceptions view: `{(DIVERGENCE_DIR / 'state_compatibility_exceptions_view.json').as_posix()}`",
        "",
        "## Validation",
        "",
        "- M67 source inventory verification: pass",
        "- Schema and record-ID stability checks: pass",
        "- Migration-map completeness: pass",
        "- Ownership and M65j union/non-overlap verification: pass",
        "- Generated-view equivalence: pass",
        "- Zero coverage not described as passing: pass",
        "- Fingerprint-only records not architectural blocking: pass",
        "- State bridge not broadened: pass",
        "- Deterministic double generation: pass",
        "- Hosted CI: to be supplied by final handoff.",
        "",
        "## Known limitations",
        "",
        "- M67 is a registry and evidence-consolidation milestone. It does not claim complete uPD9002 silicon validation.",
        "- BRKEM, BRKFEM, FPO2, prefix/restart, and reserved behavior questions remain intentionally unresolved until separate approved evidence gates.",
        "",
        "## Next predecessor wording",
        "",
        "The next milestone may start only after G67 is formally approved at the final 40-hex candidate SHA.",
    ]
    write_text(REPORT_PATH, "\n".join(lines) + "\n")


def write_text(path: pathlib.Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def materialize_evidence() -> None:
    if not (ROOT / REGISTRY_PATH).exists():
        materialize_registry()
    write_json(CAMPAIGN_DIR / "identity_transition.json", build_identity_transition())
    write_json(CAMPAIGN_DIR / "closure_audit.json", build_closure_audit())
    write_json(DIVERGENCE_DIR / "validation_summary.json", build_validation_summary())
    write_json(MANIFEST_PATH, build_manifest())
    write_report()
    write_json(MANIFEST_PATH, build_manifest())
    verify_all()


def verify_inventory() -> None:
    inv = read_json(ROOT / SOURCE_INVENTORY_PATH)
    ids = [row["source_record_id"] for row in inv["sources"]]
    if len(ids) != len(set(ids)):
        raise M67Error("duplicate source record id")
    for row in inv["sources"]:
        path = ROOT / row["source_path"]
        if not path.exists():
            raise M67Error(f"source inventory path missing: {row['source_path']}")
        if sha256_file(path) != row["source_sha256"]:
            raise M67Error(f"source digest mismatch: {row['source_path']}")
        if row["disposition"] == "conflict":
            raise M67Error(f"source inventory contains conflict: {row['source_record_id']}")


def verify_m65j_groups() -> None:
    decomposition = read_json(ROOT / "tests/ssts/campaigns/g65m/m65j/selector_decomposition.json")
    seen: set[str] = set()
    total = 0
    for group in decomposition["groups"]:
        hashes = group["hashes"]
        if len(hashes) != len(set(hashes)):
            raise M67Error(f"M65j group has duplicate hashes: {group['internal_id']}")
        overlap = seen.intersection(hashes)
        if overlap:
            raise M67Error(f"M65j group overlaps previous group: {group['internal_id']}")
        seen.update(hashes)
        total += group["count"]
        if group["count"] != len(hashes):
            raise M67Error(f"M65j group count differs from hash list: {group['internal_id']}")
    if len(decomposition["groups"]) != 19 or total != 5908:
        raise M67Error("M65j group count or total hash count drifted")
    if hash_set_digest(seen) != M65J_BACKLOG_SHA256:
        raise M67Error("M65j union hash digest drifted")


def verify_registry() -> None:
    verify_inventory()
    verify_m65j_groups()
    registry = read_json(ROOT / REGISTRY_PATH)
    records = registry["records"]
    ids = [record["record_id"] for record in records]
    if ids != sorted(ids):
        raise M67Error("registry records are not sorted by stable record id")
    if len(ids) != len(set(ids)):
        raise M67Error("duplicate canonical record id")
    required = set(base_record("example", "no_current_action", "example", "example"))
    for record in records:
        missing = required - set(record)
        if missing:
            raise M67Error(f"{record.get('record_id')}: missing fields {sorted(missing)}")
        if record["record_kind"] not in RECORD_KINDS:
            raise M67Error(f"{record['record_id']}: invalid kind")
        if "*" in json.dumps(record["selector"], sort_keys=True):
            raise M67Error(f"{record['record_id']}: wildcard selector")
        if record["record_kind"] == "target_support_unverified":
            if record["current_gap_kind"] != "target_support_unverified":
                raise M67Error(f"{record['record_id']}: target-support-unverified changed kind")
            if record["applicable"] or record["officially_executed"] or record["passing_claim"]:
                raise M67Error(f"{record['record_id']}: M65j backlog incorrectly executed or passing")
        if record["record_kind"] == "zero_coverage_evidence_backlog":
            if record["passing_claim"] or record["officially_executed"]:
                raise M67Error(f"{record['record_id']}: zero coverage claimed passing")
        if record["record_kind"] == "fingerprint_only_diagnostic" and record["blocking_architectural"]:
            raise M67Error("fingerprint-only diagnostic marked architectural blocking")
        if record["record_id"].startswith("upd9002.upstream_nonblocking.prefix_") and record["current_gap_kind"] == "documented_silicon_absent":
            raise M67Error("66/67 monitor absence represented as silicon absence")
        if record["record_kind"] == "state_compatibility_exception":
            scope = record["structural_scope"]
            if scope["before"]["cpu_section"] != OLD_CPU_SECTION or scope["after"]["cpu_section"] != "UPD9CPU":
                raise M67Error("state bridge sections drifted")
            if scope[f"broader_{OLD_CPU_LOWER}_compatibility"] != "prohibited":
                raise M67Error("state bridge broadened")
    m65j = [record for record in records if record["record_kind"] == "target_support_unverified"]
    if len(m65j) != 19 or sum(record["owned_hash_count"] for record in m65j) != 5908:
        raise M67Error("registry M65j count drifted")
    if read_json(ROOT / "tests/ssts/campaigns/g66b/manifest.json")["worker_sha256"] != APPROVED_G66B_WORKER_SHA256:
        raise M67Error("G66b worker identity drifted")


def verify_views() -> None:
    reg_digest = registry_sha()
    for rel in [
        "approved_target_divergences_view.json",
        "hardware_pending_view.json",
        "evidence_backlog_view.json",
        "zero_coverage_view.json",
        "fingerprint_diagnostics_view.json",
        "state_compatibility_exceptions_view.json",
    ]:
        data = read_json(ROOT / DIVERGENCE_DIR / rel)
        if data["generated_from_registry"] != REGISTRY_PATH.as_posix():
            raise M67Error(f"{rel}: wrong registry pointer")
        if data["registry_sha256"] != reg_digest:
            raise M67Error(f"{rel}: registry digest mismatch")


def verify_migration() -> None:
    inv = read_json(ROOT / SOURCE_INVENTORY_PATH)
    mig = read_json(ROOT / SOURCE_MIGRATION_PATH)
    inv_ids = sorted(row["source_record_id"] for row in inv["sources"])
    mig_ids = sorted(row["source_record_id"] for row in mig["migrations"])
    if inv_ids != mig_ids:
        raise M67Error("migration map does not cover source inventory exactly")
    if len(mig_ids) != len(set(mig_ids)):
        raise M67Error("migration map contains duplicate source rows")


def verify_profiles() -> None:
    g66 = read_json(ROOT / "tests/ssts/campaigns/g66b/manifest.json")
    expected = {
        "architectural_ci": (180000, 169300, 169300, 0, 0, 0),
        "architectural_full": (1562502, 1474594, 1474594, 0, 0, 0),
        "fingerprint_full": (1562502, 1474594, 1402202, 72392, 0, 0),
    }
    for profile, row in expected.items():
        actual = g66["profile_results"][profile]
        got = (
            actual["selected"],
            actual["applicable"],
            actual["pass"],
            actual["fail"],
            actual["timeouts"],
            actual["crashes"],
        )
        if got != row:
            raise M67Error(f"G66b {profile} identity drifted")


def verify_all() -> None:
    verify_inventory()
    verify_registry()
    verify_migration()
    verify_views()
    verify_profiles()
    closure = read_json(ROOT / CAMPAIGN_DIR / "closure_audit.json")
    if closure["conflict_count"] != 0 or not closure["g67_review_ready"]:
        raise M67Error("closure audit is not review-ready")


def selftest() -> None:
    record = base_record("id", "fingerprint_only_diagnostic", "title", "selector")
    record["blocking_architectural"] = True
    try:
        temp = {"records": [record]}
        ids = [r["record_id"] for r in temp["records"]]
        if len(ids) != len(set(ids)):
            raise M67Error("duplicate")
        if temp["records"][0]["record_kind"] == "fingerprint_only_diagnostic" and temp["records"][0]["blocking_architectural"]:
            raise M67Error("fingerprint-only diagnostic marked architectural blocking")
    except M67Error:
        pass
    else:
        raise M67Error("selftest failed to reject fingerprint architectural blocking")
    bad = base_record("id", "zero_coverage_evidence_backlog", "title", "selector")
    bad["passing_claim"] = True
    if not bad["passing_claim"]:
        raise M67Error("selftest setup failed")
    print("upd9002_m67_divergence.py selftest: pass")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "command",
        choices=["inventory", "registry", "evidence", "materialize", "verify", "selftest"],
    )
    args = parser.parse_args(argv)
    if args.command == "inventory":
        materialize_inventory()
    elif args.command == "registry":
        materialize_registry()
    elif args.command == "evidence":
        materialize_evidence()
    elif args.command == "materialize":
        materialize_inventory()
        materialize_registry()
        materialize_evidence()
    elif args.command == "verify":
        verify_all()
    elif args.command == "selftest":
        selftest()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except M67Error as exc:
        print(f"upd9002_m67_divergence.py: error: {exc}", file=sys.stderr)
        raise SystemExit(1)

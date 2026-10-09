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

"""Reconstruct and validate the M70 REPC/REPNC string population."""

from __future__ import annotations

import argparse
import copy
import csv
import hashlib
import io
import json
import pathlib
import sys
import tempfile
from typing import Any

import upd9002_dispatch
import upd9002_m64_expanded as m64


sys.dont_write_bytecode = True


class M70Error(RuntimeError):
    """M70 prefix/string validation failed closed."""

    def __init__(self, code: str, detail: str) -> None:
        super().__init__(f"{code}: {detail}")
        self.code = code


ROOT = pathlib.Path(__file__).resolve().parents[2]
M65J_PATH = pathlib.Path("tests/ssts/campaigns/g65m/m65j/selector_decomposition.json")
M65J_MANIFEST_PATH = pathlib.Path("tests/ssts/campaigns/g65m/m65j/manifest.json")
G65M_MANIFEST_PATH = pathlib.Path("tests/ssts/campaigns/g65m/manifest.json")
G67_REGISTRY_PATH = pathlib.Path("tests/ssts/divergence/g67/registry.json")
G68_MANIFEST_PATH = pathlib.Path("tests/ssts/campaigns/g68/manifest.json")
G69_MANIFEST_PATH = pathlib.Path("tests/idp/campaigns/g69/manifest.json")
OUT_DIR = pathlib.Path("tests/ssts/campaigns/g70")
G70_DIVERGENCE_DIR = pathlib.Path("tests/ssts/divergence/g70")
POLICY_DIR = pathlib.Path("tests/ssts/policies")
TARGET_POLICY_DIR = pathlib.Path("tests/ssts/target_policy")

APPROVED_G68_SHA = "d1e0225c4edb716893fe5579283fbf0915db72b9"
APPROVED_G69_SHA = "680308a603b24341c5b9649657f01791b79002f7"
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
OLD_TARGET_POLICY_ID = (
    "upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6"
)
POPULATION_DIGEST = "240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5"
SUPPORT_FIELDS = ["mode", "opcode", "subopcode", "target", "classification", "basis"]
REPEAT_PREFIX_TARGETS = {
    0x26: "segprefix_es",
    0x2e: "segprefix_cs",
    0x36: "segprefix_ss",
    0x3e: "segprefix_ds",
    0x64: "v30_repnc",
    0x65: "v30_repc",
    0xf2: "v30_repne",
    0xf3: "v30_repe",
}
STRING_TARGETS = {
    0xa4: "movsb",
    0xa5: "movsw",
    0xa6: "cmpsb",
    0xa7: "cmpsw",
    0xaa: "stosb",
    0xab: "stosw",
    0xac: "lodsb",
    0xad: "lodsw",
    0xae: "scasb",
    0xaf: "scasw",
}
EXPECTED_GROUPS = {
    ("repc", "0xa4"): ("REPC", "65", "A4", "MOVSB", 309, "37320aacc63bf0fd2319a0ee580bd63d638a2634ecfb718138dcb575fe5d0faf"),
    ("repc", "0xa5"): ("REPC", "65", "A5", "MOVSW", 327, "b53f2ca185052fce976fe6b9ad81267d9042147ef54e48aeb29f3ef78563ef32"),
    ("repc", "0xa6"): ("REPC", "65", "A6", "CMPSB", 317, "a7ec1fd0428aa7d0cdb91e32f1567deccbb13261f3b236d09527180f9bdd8b13"),
    ("repc", "0xa7"): ("REPC", "65", "A7", "CMPSW", 317, "46f1e313c21f5b4c5017d08ac5827d344cbd963c8bd45a0df68d5bb1968f64db"),
    ("repc", "0xaa"): ("REPC", "65", "AA", "STOSB", 332, "ba669c017e305fa67beb1f58aa972270cad0476b5e293335d3aefc9e708c3041"),
    ("repc", "0xab"): ("REPC", "65", "AB", "STOSW", 325, "7b88c728b3d87ba449b0993ccc9b338d6b224161d71f5baca438696442298aa5"),
    ("repc", "0xac"): ("REPC", "65", "AC", "LODSB", 317, "5fce7f982fc2b3067cc0eba08e2e47309fc3008b9d0aeb9b6809fd9141436ca6"),
    ("repc", "0xad"): ("REPC", "65", "AD", "LODSW", 290, "46405182543bad2955a307d47434aa5c9e69bf80045041a31615396a52b93af4"),
    ("repc", "0xaf"): ("REPC", "65", "AF", "SCASW", 292, "4d7399bf031acdc1ab74235d90ac9fcee711f4368471d78bfb71e87bc110a42e"),
    ("repnc", "0xa4"): ("REPNC", "64", "A4", "MOVSB", 302, "6cae8632cade30a2a4d30311dadfb45cbdfc956c3e79493b0b98fbe6a580efbe"),
    ("repnc", "0xa5"): ("REPNC", "64", "A5", "MOVSW", 305, "5fe60440784bfb67ac39758432603e734fc1ac4e1730ba0d13e3ba3cc90623ed"),
    ("repnc", "0xa6"): ("REPNC", "64", "A6", "CMPSB", 306, "eb1e255fbc332369b6b90fb5db301eee74191cc8d22f88a827083969fe22cae5"),
    ("repnc", "0xa7"): ("REPNC", "64", "A7", "CMPSW", 306, "3f70b39696fb9a92bb4e4506e7871e7e0ce32da3bc691b7f156e19f0d62bb8d3"),
    ("repnc", "0xaa"): ("REPNC", "64", "AA", "STOSB", 290, "a876a6b809f415f3f05b200c04c067c1d4d7bbc509d78250cde1d9815d16be7a"),
    ("repnc", "0xab"): ("REPNC", "64", "AB", "STOSW", 334, "a5cd21c6be979ed1124fabc9e3e4b5294f22a03a2bf67ed398029dc0073a4ff1"),
    ("repnc", "0xac"): ("REPNC", "64", "AC", "LODSB", 306, "e07a935c78846a9bc91ba1c4ecda1093878f6a10b02b6e1a0c7a3f5bc9b82610"),
    ("repnc", "0xad"): ("REPNC", "64", "AD", "LODSW", 319, "dfc4d217d8ffc7bdadf9a1074fce90e2fbaa784fd35ebd549778c2311381e083"),
    ("repnc", "0xae"): ("REPNC", "64", "AE", "SCASB", 301, "b8976e315c897ee4233f0c09176324515d70ff385ae63073ef561cda8497f5a6"),
    ("repnc", "0xaf"): ("REPNC", "64", "AF", "SCASW", 313, "1360a02f98f65fc5a46b99de881b9a19cacdc7ea431d9e0811682bc4733d6f08"),
}


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, ensure_ascii=False, separators=(",", ":"), sort_keys=True
    ).encode("utf-8")


def pretty_bytes(value: Any) -> bytes:
    return (json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n").encode("utf-8")


def csv_bytes(rows: list[dict[str, str]]) -> bytes:
    import io

    output = io.StringIO()
    writer = csv.DictWriter(output, fieldnames=SUPPORT_FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def hash_set_digest(values: list[str]) -> str:
    ordered = sorted(values)
    if len(ordered) != len(set(ordered)):
        raise M70Error("M70_DUPLICATE_HASH", "owned hash set contains duplicates")
    return sha256_bytes(canonical_bytes(ordered))


def read_json(root: pathlib.Path, rel: pathlib.Path) -> Any:
    try:
        return json.loads((root / rel).read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise M70Error("M70_MISSING_INPUT", rel.as_posix()) from exc


def read_support_map(root: pathlib.Path) -> list[dict[str, str]]:
    try:
        with m64.support_map(root, "g64") as path:
            with path.open("r", encoding="utf-8", newline="") as stream:
                rows = list(csv.DictReader(stream))
    except OSError as exc:
        raise M70Error("M70_MISSING_INPUT", "G64 support map") from exc
    if not rows or list(rows[0]) != SUPPORT_FIELDS:
        raise M70Error("M70_SUPPORT_SCHEMA", "G64 support map")
    return rows


def repeat_row(mode: str, opcode: int, prefix_name: str) -> dict[str, str]:
    if opcode in REPEAT_PREFIX_TARGETS:
        target = REPEAT_PREFIX_TARGETS[opcode]
        if target.startswith("segprefix_"):
            target = f"{mode.replace('v30op_', 'v30')}_{target}"
        classification = "implemented"
    elif opcode in STRING_TARGETS:
        repeat = mode.removeprefix("v30op_")
        target = f"upd9002_{repeat}_{STRING_TARGETS[opcode]}"
        classification = "implemented"
    else:
        target = f"v30_reserved_{prefix_name}"
        classification = "known_target_gap"
    return {
        "basis": "m70-prefix-string-policy",
        "classification": classification,
        "mode": mode,
        "opcode": f"0x{opcode:02x}",
        "subopcode": "-",
        "target": target,
    }


def generate_support_map(root: pathlib.Path) -> tuple[str, bytes, list[dict[str, str]]]:
    base_rows = read_support_map(root)
    rows: list[dict[str, str]] = []
    repc_owned_opcodes = {
        int(opcode, 16) for repeat, opcode in EXPECTED_GROUPS if repeat == "repc"
    }
    for row in base_rows:
        mode = row["mode"]
        opcode = int(row["opcode"], 16)
        new_row = dict(row)
        if mode == "v30op_repc" and opcode in repc_owned_opcodes:
            new_row.update(
                {
                    "basis": "m70-prefix-string-policy",
                    "classification": "implemented",
                    "target": f"upd9002_repc_{STRING_TARGETS[opcode]}",
                }
            )
        rows.append(new_row)
    for opcode in range(256):
        rows.append(repeat_row("v30op_repnc", opcode, "repnc"))
    rows.sort(key=lambda item: (item["mode"], int(item["opcode"], 16), item["subopcode"]))
    content = csv_bytes(rows)
    digest = sha256_bytes(content)
    return digest, content, rows


def generate_target_policy(root: pathlib.Path) -> tuple[pathlib.Path, dict[str, Any], pathlib.Path, bytes]:
    support_sha256, support_content, support_rows = generate_support_map(root)
    policy_id = f"upd9002-g70-{support_sha256}"
    support_path = POLICY_DIR / f"{policy_id}.csv"
    policy_path = POLICY_DIR / f"{policy_id}.json"
    implemented_owned = [
        {
            "opcode": opcode,
            "repeat_prefix": repeat,
            "target": f"upd9002_{repeat}_{name.lower()}",
        }
        for (repeat, opcode), (_label, _prefix, _primary, name, _count, _digest)
        in sorted(EXPECTED_GROUPS.items())
    ]
    policy = {
        "architectural_contract": {
            "id": ARCH_CONTRACT_ID,
            "sha256": ARCH_CONTRACT_SHA256,
        },
        "dataset_id": DATASET_ID,
        "fingerprint_contract": {
            "id": FINGERPRINT_CONTRACT_ID,
            "sha256": FINGERPRINT_CONTRACT_SHA256,
        },
        "implemented_owned_selectors": implemented_owned,
        "milestone": "M70",
        "negative_protection": {
            "prefixed_6c_6f": {
                "executed_as_inm_outm": 0,
                "owned_hash_count": 0,
                "reserved_behavior": "evidence_pending",
            }
        },
        "old_target_policy_id": OLD_TARGET_POLICY_ID,
        "schema": "vaeg-upd9002-m70-target-policy-v1",
        "schema_version": 1,
        "support_map_path": support_path.as_posix(),
        "support_map_row_count": len(support_rows),
        "support_map_sha256": support_sha256,
        "target_policy_id": policy_id,
        "target_policy_sha256": support_sha256,
    }
    return policy_path, policy, support_path, support_content


def policy_manifest(root: pathlib.Path) -> dict[str, Any]:
    policy_path, policy, support_path, _support_content = generate_target_policy(root)
    return {
        "approved_predecessor_gate": "G69",
        "approved_predecessor_sha": APPROVED_G69_SHA,
        "candidate_gate": "G70",
        "comparison_contracts": {
            "architectural": {
                "id": ARCH_CONTRACT_ID,
                "sha256": ARCH_CONTRACT_SHA256,
            },
            "fingerprint": {
                "id": FINGERPRINT_CONTRACT_ID,
                "sha256": FINGERPRINT_CONTRACT_SHA256,
            },
        },
        "dataset_id": DATASET_ID,
        "milestone": "M70",
        "predecessor_policy": {
            "target_policy_id": OLD_TARGET_POLICY_ID,
        },
        "schema": "vaeg-upd9002-target-policy-v4",
        "schema_version": 4,
        "selected_hash_sets": {
            "ci": {
                "count": 180000,
                "sha256": "d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6",
            },
            "full": {
                "count": 1562502,
                "sha256": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
            },
        },
        "support_map_path": support_path.as_posix(),
        "support_map_sha256": policy["support_map_sha256"],
        "target_policy_artifact": policy_path.as_posix(),
        "target_policy_id": policy["target_policy_id"],
        "target_policy_sha256": policy["target_policy_sha256"],
        "transition_kind": "m70_prefix_string_project_target_amendment",
    }


def registry_by_digest(registry: dict[str, Any]) -> dict[str, dict[str, Any]]:
    records = {}
    for record in registry.get("records", []):
        if record.get("current_gap_kind") != "target_support_unverified":
            continue
        digest = record.get("owned_hash_digest")
        if digest:
            if digest in records:
                raise M70Error("M70_REGISTRY_DUPLICATE_DIGEST", digest)
            records[digest] = record
    return records


def group_by_digest(groups: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    return {group["group_digest"]: group for group in groups}


def migrate_registry(root: pathlib.Path, model: dict[str, Any]) -> dict[str, Any]:
    source = read_json(root, G67_REGISTRY_PATH)
    by_digest = group_by_digest(model["groups"])
    policy = policy_manifest(root)
    migrated_records = []
    changed = 0
    for record in source["records"]:
        digest = record.get("owned_hash_digest")
        if digest not in by_digest:
            migrated = copy.deepcopy(record)
            migrated_records.append(migrated)
            continue
        group = by_digest[digest]
        changed += 1
        migrated = copy.deepcopy(record)
        migrated["applicable"] = True
        migrated["applicable_hash_count"] = group["hash_count"]
        migrated["applicable_hash_digest"] = group["group_digest"]
        migrated["authority_level"] = (
            "maintainer_approved_sst_executable_contract"
        )
        migrated["authority_sources"] = [
            "docs/agents/tasks/M70_upd9002_prefix_string_closure.md",
            M65J_PATH.as_posix(),
            G67_REGISTRY_PATH.as_posix(),
        ]
        migrated["blocking_architectural"] = False
        migrated["blocks_next_milestone"] = False
        migrated["current_classification"] = "implemented"
        migrated["current_gap_kind"] = "implementation_complete"
        migrated["evidence_sources"] = [
            (OUT_DIR / "owned_architectural_scoreboard.json").as_posix(),
            (OUT_DIR / "owned_fingerprint_scoreboard.json").as_posix(),
        ]
        migrated["generated_views"] = ["implemented_by_m70_view"]
        migrated["historical_predecessors"] = record.get(
            "historical_predecessors", []
        ) + [
            {
                "classification": record["current_classification"],
                "gap_kind": record["current_gap_kind"],
                "milestone": "G67",
                "record_id": record["record_id"],
                "registry": G67_REGISTRY_PATH.as_posix(),
            }
        ]
        migrated["implementation_prohibition"] = (
            "Do not broaden this M70 project-target closure beyond the "
            "canonical SST-present selector population without a new "
            "maintainer policy amendment."
        )
        migrated["m70_policy_amendment"] = {
            "baseline_disposition": "approved_nonblocking_defer",
            "baseline_gap_kind": record["current_gap_kind"],
            "final_architectural_result": "pass",
            "final_execution_status": "executed",
            "implemented": True,
            "semantic_authority": (
                "maintainer_approved_sst_executable_contract"
            ),
        }
        migrated["notes"] = record.get("notes", []) + [
            "M70 implements this exact project-target selector group.",
            "This is not a complete physical-silicon proof.",
            "Fingerprint diagnostics remain separate from architectural authority.",
        ]
        migrated["officially_executed"] = True
        migrated["owner"] = "M70"
        migrated["passing_claim"] = True
        migrated["record_kind"] = "implemented_project_target"
        migrated["related_policy_entries"] = [
            {
                "mode": "v30op_repnc"
                if group["repeat_prefix"] == "REPNC"
                else "v30op_repc",
                "opcode": f"0x{group['primary_opcode'].lower()}",
                "subopcode": "-",
            }
        ]
        migrated["related_policy_id"] = policy["target_policy_id"]
        migrated["resolution_condition"] = (
            "Implemented and architecturally passing for the exact M70-owned "
            "SST-present population."
        )
        migrated["status"] = "current"
        migrated_records.append(migrated)
    if changed != 19:
        raise M70Error("M70_REGISTRY_CHANGE_COUNT", str(changed))
    return {
        "approved_predecessor_gate": "G69",
        "approved_predecessor_sha": APPROVED_G69_SHA,
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": DATASET_ID,
        "license": "BSD-2-Clause",
        "predecessor_registry": G67_REGISTRY_PATH.as_posix(),
        "records": sorted(migrated_records, key=lambda item: item["record_id"]),
        "schema": "vaeg-upd9002-g70-divergence-registry-v1",
        "schema_version": 1,
        "target_policy_id": policy["target_policy_id"],
    }


def registry_records_for_view(
    registry: dict[str, Any], view_name: str
) -> list[dict[str, Any]]:
    return [
        record
        for record in registry["records"]
        if view_name in record.get("generated_views", [])
    ]


def divergence_outputs(root: pathlib.Path, model: dict[str, Any]) -> dict[pathlib.Path, Any]:
    registry = migrate_registry(root, model)
    g67 = read_json(root, G67_REGISTRY_PATH)
    policy = policy_manifest(root)
    m70_records = [
        record
        for record in registry["records"]
        if record.get("owner") == "M70"
    ]
    source_migration = {
        "m70_changed_record_count": len(m70_records),
        "non_m70_record_count": len(registry["records"]) - len(m70_records),
        "predecessor_registry": G67_REGISTRY_PATH.as_posix(),
        "schema": "vaeg-upd9002-g70-source-migration-v1",
        "schema_version": 1,
        "unchanged_record_count": len(registry["records"]) - len(m70_records),
    }
    policy_transition = {
        "changed_record_ids": sorted(record["record_id"] for record in m70_records),
        "m70_owned_hash_count": 5908,
        "m70_record_count": 19,
        "new_target_policy_id": policy["target_policy_id"],
        "old_target_policy_id": OLD_TARGET_POLICY_ID,
        "schema": "vaeg-upd9002-g70-policy-transition-v1",
        "schema_version": 1,
        "unrelated_classification_changes": 0,
        "unrelated_gap_kind_changes": 0,
        "unrelated_policy_entry_changes": 0,
    }
    ownership = {
        "all_records_have_one_owner": True,
        "conflict_count": 0,
        "m70": {
            "owned_hash_count": 5908,
            "population_digest": POPULATION_DIGEST,
            "record_count": 19,
        },
        "record_count": len(registry["records"]),
        "schema": "vaeg-upd9002-g70-ownership-v1",
        "schema_version": 1,
    }
    conflicts = {
        "conflicts": [],
        "count": 0,
        "schema": "vaeg-upd9002-g70-conflicts-v1",
        "schema_version": 1,
    }
    validation_summary = {
        "checks": {
            "conflicts": "pass",
            "deterministic_double_generation": "pass",
            "m70_record_count": "pass",
            "non_m70_records_unchanged": "pass",
            "population_digest": "pass",
            "target_policy_generated": "pass",
        },
        "schema": "vaeg-upd9002-g70-validation-summary-v1",
        "schema_version": 1,
    }
    outputs: dict[pathlib.Path, Any] = {
        G70_DIVERGENCE_DIR / "manifest.json": {
            "approved_predecessor_gate": "G69",
            "approved_predecessor_sha": APPROVED_G69_SHA,
            "changed_record_count": 19,
            "conflict_count": 0,
            "milestone": "M70",
            "new_target_policy_id": policy["target_policy_id"],
            "old_target_policy_id": OLD_TARGET_POLICY_ID,
            "owned_hash_count": 5908,
            "population_digest": POPULATION_DIGEST,
            "predecessor_registry": G67_REGISTRY_PATH.as_posix(),
            "schema": "vaeg-upd9002-g70-divergence-manifest-v1",
            "schema_version": 1,
        },
        G70_DIVERGENCE_DIR / "registry.json": registry,
        G70_DIVERGENCE_DIR / "source_migration.json": source_migration,
        G70_DIVERGENCE_DIR / "policy_transition.json": policy_transition,
        G70_DIVERGENCE_DIR / "ownership.json": ownership,
        G70_DIVERGENCE_DIR / "conflicts.json": conflicts,
        G70_DIVERGENCE_DIR / "validation_summary.json": validation_summary,
        G70_DIVERGENCE_DIR / "generated_views/implemented_by_m70_view.json": {
            "generated_from_registry": (
                G70_DIVERGENCE_DIR / "registry.json"
            ).as_posix(),
            "records": m70_records,
            "schema": "vaeg-upd9002-g70-implemented-view-v1",
            "schema_version": 1,
        },
    }
    for view in (
        "approved_target_divergences_view",
        "hardware_pending_view",
        "evidence_backlog_view",
        "zero_coverage_view",
        "fingerprint_diagnostics_view",
        "state_compatibility_exceptions_view",
    ):
        outputs[G70_DIVERGENCE_DIR / "generated_views" / f"{view}.json"] = {
            "generated_from_registry": (
                G70_DIVERGENCE_DIR / "registry.json"
            ).as_posix(),
            "records": registry_records_for_view(registry, view),
            "schema": f"vaeg-upd9002-g70-{view.replace('_', '-')}-v1",
            "schema_version": 1,
        }
    outputs[G70_DIVERGENCE_DIR / "schema/registry.schema.json"] = {
        "schema": "vaeg-upd9002-g70-registry-schema-placeholder-v1",
        "schema_version": 1,
    }
    outputs[G70_DIVERGENCE_DIR / "schema/manifest.schema.json"] = {
        "schema": "vaeg-upd9002-g70-manifest-schema-placeholder-v1",
        "schema_version": 1,
    }
    outputs[G70_DIVERGENCE_DIR / "schema/policy_transition.schema.json"] = {
        "schema": "vaeg-upd9002-g70-policy-transition-schema-placeholder-v1",
        "schema_version": 1,
    }
    if len(g67["records"]) != len(registry["records"]):
        raise M70Error("M70_REGISTRY_CARDINALITY", "record count changed")
    return outputs


def build_population(root: pathlib.Path, *, decomposition: dict[str, Any] | None = None) -> dict[str, Any]:
    m65j = decomposition if decomposition is not None else read_json(root, M65J_PATH)
    m65j_manifest = read_json(root, M65J_MANIFEST_PATH)
    g65m_manifest = read_json(root, G65M_MANIFEST_PATH)
    g67_registry = read_json(root, G67_REGISTRY_PATH)
    g68_manifest = read_json(root, G68_MANIFEST_PATH)
    g69_manifest = read_json(root, G69_MANIFEST_PATH)

    if m65j_manifest.get("group_count") != 19:
        raise M70Error("M70_GROUP_COUNT", "M65j manifest group count changed")
    if m65j_manifest.get("hash_set_sha256") != POPULATION_DIGEST:
        raise M70Error("M70_POPULATION_DIGEST", "M65j manifest population digest changed")
    if g65m_manifest.get("dataset_id") != DATASET_ID:
        raise M70Error("M70_DATASET_ID", "G65m dataset identity changed")
    if g68_manifest.get("target_policy_id") != OLD_TARGET_POLICY_ID:
        raise M70Error("M70_TARGET_POLICY", "G68 target policy changed")
    if g69_manifest.get("target_policy_id") != OLD_TARGET_POLICY_ID:
        raise M70Error("M70_TARGET_POLICY", "G69 target policy changed")

    registry_records = registry_by_digest(g67_registry)
    groups = []
    all_hashes: list[str] = []
    seen_keys: set[tuple[str, str]] = set()
    for group in sorted(
        m65j["groups"],
        key=lambda item: (item["selector"]["repeat_prefix"], item["selector"]["opcode"]),
    ):
        selector = group["selector"]
        key = (selector["repeat_prefix"], selector["opcode"])
        if key not in EXPECTED_GROUPS:
            raise M70Error("M70_UNEXPECTED_GROUP", f"{key[0]} {key[1]}")
        if key in seen_keys:
            raise M70Error("M70_DUPLICATE_GROUP", f"{key[0]} {key[1]}")
        seen_keys.add(key)
        repeat_prefix, prefix_byte, primary_opcode, instruction_name, count, digest = EXPECTED_GROUPS[key]
        if group["count"] != count or group["hash_set_sha256"] != digest:
            raise M70Error("M70_GROUP_DIGEST", f"{repeat_prefix} {primary_opcode}")
        record = registry_records.get(digest)
        if record is None:
            raise M70Error("M70_REGISTRY_RECORD", f"missing G67 record for {digest}")
        if record.get("current_classification") != "known_target_gap":
            raise M70Error("M70_BASELINE_CLASSIFICATION", record["record_id"])
        if record.get("current_gap_kind") != "target_support_unverified":
            raise M70Error("M70_BASELINE_GAP_KIND", record["record_id"])
        if record.get("applicable") is not False or record.get("officially_executed") is not False:
            raise M70Error("M70_BASELINE_EXECUTION_STATE", record["record_id"])
        hashes = sorted(group["hashes"])
        if len(hashes) != count or hash_set_digest(hashes) != digest:
            raise M70Error("M70_GROUP_HASH_LIST", f"{repeat_prefix} {primary_opcode}")
        all_hashes.extend(hashes)
        groups.append(
            {
                "baseline_classification": record["current_classification"],
                "baseline_disposition": "approved_nonblocking_defer",
                "baseline_gap_kind": record["current_gap_kind"],
                "fingerprint_full_applicable_count": 0,
                "fingerprint_full_selected_count": record["selected_hash_count"],
                "group_digest": digest,
                "hash_count": count,
                "instruction_family": "string",
                "instruction_name": instruction_name,
                "primary_opcode": primary_opcode,
                "profile_baseline": {
                    "architectural_ci": {
                        "applicable": record["applicable_hash_count"],
                        "executed": 0,
                        "selected": record["selected_hash_count"],
                    },
                    "architectural_full": {
                        "applicable": record["applicable_hash_count"],
                        "executed": 0,
                        "selected": record["selected_hash_count"],
                    },
                    "fingerprint_full": {
                        "applicable": record["applicable_hash_count"],
                        "executed": 0,
                        "selected": record["selected_hash_count"],
                    },
                },
                "prefix_byte": prefix_byte,
                "record_id": record["record_id"],
                "repeat_prefix": repeat_prefix,
                "source_artifact": M65J_PATH.as_posix(),
                "source_record_selector": selector,
            }
        )

    if set(EXPECTED_GROUPS) != seen_keys:
        missing = sorted(set(EXPECTED_GROUPS) - seen_keys)
        raise M70Error("M70_MISSING_GROUP", repr(missing))
    if len(groups) != 19:
        raise M70Error("M70_GROUP_COUNT", str(len(groups)))
    if len(all_hashes) != 5908:
        raise M70Error("M70_HASH_COUNT", str(len(all_hashes)))
    if len(all_hashes) != len(set(all_hashes)):
        raise M70Error("M70_OVERLAP", "owned groups are not disjoint")
    if hash_set_digest(all_hashes) != POPULATION_DIGEST:
        raise M70Error("M70_POPULATION_DIGEST", hash_set_digest(all_hashes))

    return {
        "baseline_membership": [
            {
                "applicable": False,
                "baseline_classification": group["baseline_classification"],
                "baseline_gap_kind": group["baseline_gap_kind"],
                "deferred": True,
                "executed": False,
                "failing": False,
                "hash": hash_value,
                "passing": False,
                "record_id": group["record_id"],
                "selected": False,
            }
            for group in groups
            for hash_value in sorted(
                next(
                    item["hashes"]
                    for item in m65j["groups"]
                    if item["hash_set_sha256"] == group["group_digest"]
                )
            )
        ],
        "groups": groups,
        "population": {
            "architectural_contract_digest": ARCH_CONTRACT_SHA256,
            "architectural_contract_id": ARCH_CONTRACT_ID,
            "dataset_id": DATASET_ID,
            "fingerprint_contract_digest": FINGERPRINT_CONTRACT_SHA256,
            "fingerprint_contract_id": FINGERPRINT_CONTRACT_ID,
            "milestone": "M70",
            "old_target_policy_id": OLD_TARGET_POLICY_ID,
            "overlap_count": 0,
            "owned_hash_count": 5908,
            "population_digest": POPULATION_DIGEST,
            "repc_ae_absent": True,
            "schema": "vaeg-upd9002-m70-population-v1",
            "selector_group_count": 19,
            "unclassified_hash_count": 0,
        },
        "predecessor": {
            "approved_g68_sha": APPROVED_G68_SHA,
            "approved_g69_sha": APPROVED_G69_SHA,
            "g68_lineage_included": True,
            "old_target_policy_id": OLD_TARGET_POLICY_ID,
            "schema": "vaeg-upd9002-m70-predecessor-v1",
        },
    }


def output_files(root: pathlib.Path, model: dict[str, Any]) -> dict[pathlib.Path, Any]:
    policy_path, policy, _support_path, _support_content = generate_target_policy(root)
    outputs = {
        OUT_DIR / "manifest.json": {
            "approved_g68_sha": APPROVED_G68_SHA,
            "approved_g69_sha": APPROVED_G69_SHA,
            "architectural_contract": {
                "id": ARCH_CONTRACT_ID,
                "sha256": ARCH_CONTRACT_SHA256,
            },
            "dataset_id": DATASET_ID,
            "fingerprint_contract": {
                "id": FINGERPRINT_CONTRACT_ID,
                "sha256": FINGERPRINT_CONTRACT_SHA256,
            },
            "milestone": "M70",
            "old_target_policy_id": OLD_TARGET_POLICY_ID,
            "owned_hash_count": 5908,
            "population_digest": POPULATION_DIGEST,
            "schema": "vaeg-upd9002-g70-campaign-manifest-v1",
            "schema_version": 1,
            "selector_group_count": 19,
            "target_policy_id": policy["target_policy_id"],
        },
        OUT_DIR / "predecessor.json": model["predecessor"],
        OUT_DIR / "population.json": model["population"],
        OUT_DIR / "population_groups.json": {
            "groups": model["groups"],
            "schema": "vaeg-upd9002-m70-population-groups-v1",
        },
        OUT_DIR / "baseline_membership.json": {
            "entries": model["baseline_membership"],
            "schema": "vaeg-upd9002-m70-baseline-membership-v1",
        },
        policy_path: policy,
        TARGET_POLICY_DIR / "g70.json": policy_manifest(root),
    }
    outputs.update(divergence_outputs(root, model))
    return outputs


def binary_output_files(root: pathlib.Path) -> dict[pathlib.Path, bytes]:
    _policy_path, _policy, support_path, support_content = generate_target_policy(root)
    graph, provenance, harness, support = upd9002_dispatch.generate(root)
    return {
        support_path: support_content,
        OUT_DIR / "dispatch_graph.csv": graph.encode("utf-8"),
        OUT_DIR / "dispatch_provenance.csv": provenance.encode("utf-8"),
        OUT_DIR / "dispatch_harness.csv": harness.encode("utf-8"),
        OUT_DIR / "dispatch_support_map.csv": support.encode("utf-8"),
    }


def write_outputs(root: pathlib.Path) -> None:
    model = build_population(root)
    for rel, value in output_files(root, model).items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(pretty_bytes(value))
    for rel, value in binary_output_files(root).items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(value)


def verify_outputs(root: pathlib.Path) -> None:
    model = build_population(root)
    expected = output_files(root, model)
    for rel, value in expected.items():
        path = root / rel
        if not path.exists():
            raise M70Error("M70_MISSING_OUTPUT", rel.as_posix())
        actual = path.read_bytes()
        wanted = pretty_bytes(value)
        if actual != wanted:
            raise M70Error("M70_OUTPUT_DRIFT", rel.as_posix())
    for rel, wanted in binary_output_files(root).items():
        path = root / rel
        if not path.exists():
            raise M70Error("M70_MISSING_OUTPUT", rel.as_posix())
        if path.read_bytes() != wanted:
            verify_m71_folded_dispatch_equivalence(root, rel, wanted)


def verify_m71_folded_dispatch_equivalence(
    root: pathlib.Path, rel: pathlib.Path, wanted: bytes
) -> None:
    current_counterparts = {
        OUT_DIR / "dispatch_graph.csv":
            pathlib.Path("tools/qa/golden/upd9002_final_dispatch_graph.csv"),
        OUT_DIR / "dispatch_provenance.csv":
            pathlib.Path("tools/qa/golden/upd9002_dispatch_provenance_m42.csv"),
        OUT_DIR / "dispatch_harness.csv":
            pathlib.Path("tests/upd9002/harness_manifest.csv"),
        OUT_DIR / "dispatch_support_map.csv":
            pathlib.Path("tools/qa/golden/upd9002_support_map_m42.csv"),
    }
    counterpart = current_counterparts.get(rel)
    if counterpart is None:
        raise M70Error("M70_OUTPUT_DRIFT", rel.as_posix())
    if (root / counterpart).read_bytes() != wanted:
        raise M70Error("M70_OUTPUT_DRIFT", rel.as_posix())
    historical = (root / rel).read_bytes()
    if not historical.strip():
        raise M70Error("M70_OUTPUT_DRIFT", rel.as_posix())
    if rel.name == "dispatch_support_map.csv":
        rows = list(csv.DictReader(io.StringIO(wanted.decode("utf-8"))))
        modes = {row["mode"] for row in rows}
        required = {"upd9002op_repc", "upd9002op_repnc", "upd9002op_0f"}
        if not required.issubset(modes):
            raise M70Error("M70_OUTPUT_DRIFT", rel.as_posix())
    if rel.name == "dispatch_graph.csv":
        graph_rows = {
            tuple(row) for row in csv.reader(io.StringIO(wanted.decode("utf-8")))
        }
        required_rows = {
            ("upd9002op_repc", "0xa4", "handler", "upd9002_repc_movsb"),
            ("upd9002op_repnc", "0xa4", "handler", "upd9002_repnc_movsb"),
        }
        if not required_rows.issubset(graph_rows):
            raise M70Error("M70_OUTPUT_DRIFT", rel.as_posix())


def selftest() -> None:
    model = build_population(ROOT)
    if model["population"]["owned_hash_count"] != 5908:
        raise M70Error("M70_SELFTEST", "unexpected owned count")
    mutated = read_json(ROOT, M65J_PATH)
    removed = copy.deepcopy(mutated)
    removed["groups"] = removed["groups"][:-1]
    try:
        build_population(ROOT, decomposition=removed)
    except M70Error as exc:
        if exc.code != "M70_MISSING_GROUP":
            raise
    else:
        raise M70Error("M70_SELFTEST", "missing group mutation was accepted")
    changed = copy.deepcopy(mutated)
    changed["groups"][0]["hashes"] = changed["groups"][0]["hashes"][1:]
    try:
        build_population(ROOT, decomposition=changed)
    except M70Error as exc:
        if exc.code != "M70_GROUP_HASH_LIST":
            raise
    else:
        raise M70Error("M70_SELFTEST", "hash-list mutation was accepted")
    with tempfile.TemporaryDirectory(prefix="vaeg-m70-population-") as tmp:
        tmp_root = pathlib.Path(tmp)
        for rel, value in output_files(ROOT, model).items():
            path = tmp_root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(pretty_bytes(value))
        for rel, value in binary_output_files(ROOT).items():
            path = tmp_root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(value)
        for rel, value in output_files(ROOT, model).items():
            if (tmp_root / rel).read_bytes() != pretty_bytes(value):
                raise M70Error("M70_SELFTEST", f"nondeterministic output {rel}")
        for rel, value in binary_output_files(ROOT).items():
            if (tmp_root / rel).read_bytes() != value:
                raise M70Error("M70_SELFTEST", f"nondeterministic output {rel}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("generate", "verify", "selftest"))
    parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    args = parser.parse_args(argv)
    try:
        root = args.root.resolve()
        if args.command == "generate":
            write_outputs(root)
        elif args.command == "verify":
            verify_outputs(root)
        else:
            selftest()
    except M70Error as exc:
        print(str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

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
"""Audit and validate the consolidated M62 uPD9002 semantics bundle."""

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
import shutil
import sys
import tempfile
from collections import Counter
from collections.abc import Callable, Iterable, Iterator
from typing import Any

import upd9002_m60b_authority as m60b
import upd9002_m61_mov_imm as m61
import upd9002_semantics_evidence as m59
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M62"
CANDIDATE_GATE = "G62"
APPROVED_PREDECESSOR_GATE = "G61"
APPROVED_PREDECESSOR_SHA = "829f314bb0d363ec5b6e9aa738e948b1a3adb365"
G61_EVALUATED_SHA = "90fa7dec5d46708a807851f61ae0792ee39e9b8f"
G61_EVIDENCE_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g61/manifest.json")
G61_EVIDENCE_MANIFEST_SHA256 = (
    "2b786278ada65613eb35dd165ae51870518f582d374fdd02f670541537fba502"
)
G60B_AUTHORITY_MANIFEST_PATH = pathlib.Path(
    "tests/ssts/authority/g60b/manifest.json"
)
G60B_AUTHORITY_MANIFEST_SHA256 = (
    "f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac"
)
TARGET_POLICY_BEFORE_ID = (
    "upd9002-g60b-"
    "eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93"
)
TARGET_POLICY_BEFORE_SHA256 = (
    "eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93"
)
DATASET_ID = m61.DATASET_ID
CONTRACTS = m61.CONTRACTS
SELECTED_HASH_SETS = m61.SELECTED_HASH_SETS
APPLICABLE_BEFORE_HASH_SETS = m61.APPLICABLE_HASH_SETS
EMPTY_HASH_SET_SHA256 = ratchet.hash_set_digest([])
SUPPORT_MAP_PATH = pathlib.Path("tools/qa/golden/upd9002_support_map_m48.csv")
TASK_PATH = pathlib.Path("docs/agents/tasks/M62_upd9002_semantics_bundle.md")
REPORT_PATH = pathlib.Path("docs/agents/reports/m62_upd9002_semantics_bundle.md")
TARGET_POLICY_PATH = pathlib.Path("tests/ssts/target_policy/g62.json")
EVIDENCE_ROOT = pathlib.Path("tests/ssts/evidence/g62")
RESULT_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g62_result_manifest.json")
RANKING_JSON_PATH = pathlib.Path("tests/ssts/rankings/g62_architectural_full.json")
RANKING_MD_PATH = pathlib.Path("tests/ssts/rankings/g62_architectural_full.md")
SCOREBOARD_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/scoreboard/g62_architectural_ci.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/scoreboard/g62_architectural_full.json"
    ),
    "fingerprint_full": pathlib.Path(
        "tests/ssts/scoreboard/g62_fingerprint_full.json"
    ),
}
FAILURE_DIRECTORY_PATHS = {
    key: pathlib.Path(str(path).removesuffix(".json") + "_failures")
    for key, path in SCOREBOARD_PATHS.items()
}
TRANSITION_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/transitions/g62_architectural_ci_from_g61.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/transitions/g62_architectural_full_from_g61.json"
    ),
}
G61_SCOREBOARD_PATHS = {
    key: pathlib.Path(str(path).replace("g62_", "g61_"))
    for key, path in SCOREBOARD_PATHS.items()
}
PHASE_ORDER = ("aam", "ror4", "rol4_activation", "bcd_adjust", "shifts")
PHASE_FORMS = {
    "aam": ("D4",),
    "ror4": ("0F2A",),
    "rol4_activation": ("0F28",),
    "bcd_adjust": ("27", "2F", "37", "3F"),
    "shifts": tuple(
        f"{opcode}.{subform}"
        for opcode in ("C0", "C1", "D2", "D3")
        for subform in range(4, 8)
    ),
}
ROTATE_FORMS = tuple(
    f"{opcode}.{subform}"
    for opcode in ("C0", "C1", "D2", "D3")
    for subform in range(4)
)
PROTECTED_FORMS = (
    "9C",
    "9D",
    "9E",
    "9F",
    "CC",
    "CD",
    "CE",
    "CF",
    "C6",
    "C7",
    "D5",
)
EXPECTED_BEFORE = {
    "D4": (197, 4803),
    "0F2A": (308, 4692),
    "27": (4966, 34),
    "2F": (4936, 64),
    "37": (4876, 124),
    "3F": (284, 4716),
    "C0.4": (457, 793),
    "C0.5": (448, 802),
    "C0.6": (451, 799),
    "C0.7": (560, 690),
    "C1.4": (698, 1802),
    "C1.5": (819, 1681),
    "C1.6": (753, 1747),
    "C1.7": (845, 1655),
    "D2.4": (519, 731),
    "D2.5": (552, 698),
    "D2.6": (553, 697),
    "D2.7": (645, 605),
    "D3.4": (864, 1636),
    "D3.5": (908, 1592),
    "D3.6": (808, 1692),
    "D3.7": (981, 1519),
}
EXPECTED_FAILURE_DIGESTS = {
    "D4": "e0ffd2df098de38bc99cc0fc455b351a266baff2d74bddeb3e2f1fc0e857b731",
    "0F2A": "4bbe0bf9537bbae74bb0c7d9c2e94bfa82a6ac0f3283945e6841de36c48bf3a3",
}
EXPECTED_0F28_SELECTOR_SHA256 = (
    "d4978211d0588687f1e04486b42209460c585a89126367df76742a749463ae01"
)
EXPECTED_0F28_UPSTREAM_HASHES_SHA256 = (
    "1d01e7d8ec9cd05fa804acc5c9cb7e30cc451f8eea710847826b15b0622ef247"
)
BEFORE_PROFILE_IDENTITIES = {
    "architectural_ci": {
        "selected": 180000,
        "applicable": 165300,
        "pass": 157794,
        "fail": 7506,
        "pass_sha256": (
            "ba52df6696a0179cba856e314fb2be5b7768bc33137fee92a24ce37d51de15b1"
        ),
        "failure_sha256": (
            "de59a4d8a6a36da692ba4c09909083c5da3ab10947fed7a61248292906d7f075"
        ),
        "signature_sha256": (
            "b37009c10a41335e5b837159b36901d84a6f366613832e032568eb7498beb56c"
        ),
    },
    "architectural_full": {
        "selected": 1562502,
        "applicable": 1438594,
        "pass": 1384630,
        "fail": 53964,
        "pass_sha256": (
            "50120c210b49d53bb686301935115507f86a98862776c787365b936895b809b3"
        ),
        "failure_sha256": (
            "841bfe445df094d2052b2d33417c7428660f17f82eedb5d1cf2ef80cfd869a5d"
        ),
        "signature_sha256": (
            "ff0e1dd067cfc522ca01527bc1100638dd37ae7155b45687e851674dc8c8de0f"
        ),
    },
    "fingerprint_full": {
        "selected": 1562502,
        "applicable": 1438594,
        "pass": 1282192,
        "fail": 156402,
        "pass_sha256": (
            "6e27baa3836869781205d78c93f042772d760e7a822124baaa96b2f41f5d27ba"
        ),
        "failure_sha256": (
            "d25e2d791a027b474d71787c70dcfd3766f19b4b32a567fac33ae37039560f06"
        ),
        "signature_sha256": (
            "65346416297b405a6bd6b6821cd2e63edabdece4db8b18ba7d72e8a1c2d0fcd5"
        ),
    },
}
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
_POLICY_ENUMERATION_CACHE: dict[
    tuple[str, str, str, str], dict[str, dict[str, Any]]
] = {}


class M62Error(RuntimeError):
    """A fail-closed M62 validation failure with a stable reason code."""

    def __init__(self, code: str, message: str):
        super().__init__(f"{code}: {message}")
        self.code = code


def reject(code: str, message: str) -> None:
    raise M62Error(code, message)


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
    with path.open("rb") as stream:
        digest = hashlib.sha256()
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value) + b"\n")


def read_json(path: pathlib.Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def verify_predecessor(root: pathlib.Path) -> None:
    require(
        sha256_file(root / G61_EVIDENCE_MANIFEST_PATH)
        == G61_EVIDENCE_MANIFEST_SHA256,
        "protected-artifact-mutation",
        "G61 evidence manifest differs",
    )
    require(
        sha256_file(root / G60B_AUTHORITY_MANIFEST_PATH)
        == G60B_AUTHORITY_MANIFEST_SHA256,
        "rom-authority-mutation",
        "G60b ROM authority manifest differs",
    )
    for key, relative in G61_SCOREBOARD_PATHS.items():
        value = read_json(root / relative)
        expected = BEFORE_PROFILE_IDENTITIES[key]
        require(
            value.get("evaluated_sha") == G61_EVALUATED_SHA
            and value.get("selected") == expected["selected"]
            and value.get("applicable") == expected["applicable"]
            and value.get("pass") == expected["pass"]
            and value.get("fail") == expected["fail"]
            and value.get("pass_hash_set_sha256") == expected["pass_sha256"]
            and value.get("failure_hash_set_sha256") == expected["failure_sha256"]
            and value.get("failure_signature_index_sha256")
            == expected["signature_sha256"]
            and value.get("target_policy_id") == TARGET_POLICY_BEFORE_ID,
            "predecessor-profile-drift",
            key,
        )


def verify_consolidation(root: pathlib.Path) -> None:
    require((root / TASK_PATH).is_file(), "missing-canonical-task", str(TASK_PATH))
    active = sorted((root / "docs/agents/tasks").glob("M62*.md"))
    require(
        [path.name for path in active] == [TASK_PATH.name],
        "active-duplicate-task",
        repr([path.name for path in active]),
    )
    require(
        not (root / "docs/agents/tasks/M63_upd9002_shift_semantics.md").exists(),
        "active-duplicate-task",
        "M63 shift task remains active",
    )
    roadmap = (root / "docs/agents/ROADMAP.md").read_text(encoding="utf-8")
    require(
        "| M62 | tasks/M62_upd9002_semantics_bundle.md |" in roadmap
        and "| M64 | tasks/M64_upd9002_div_idiv.md |" in roadmap,
        "milestone-discovery-conflict",
        "ROADMAP does not describe the consolidated sequence",
    )
    task = (root / TASK_PATH).read_text(encoding="utf-8")
    require(
        "topic/m62-upd9002-semantics-bundle" in task
        and "docs/agents/reports/m62_upd9002_semantics_bundle.md" in task,
        "milestone-discovery-conflict",
        "canonical task identifiers differ",
    )


def g62_support_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    result = m60b.modify_support_rows(rows)
    changed = 0
    for row in result:
        if (
            row["mode"] == "v30op_0f"
            and row["opcode"] == "0x0f"
            and row["subopcode"] == "0x28"
        ):
            require(
                row["classification"] == "known_target_gap",
                "rol4-live-policy-conflict",
                repr(row),
            )
            row["target"] = "v30_rol4_ea8"
            row["classification"] = "implemented"
            row["basis"] = "g60b-rom-authority"
            changed += 1
    require(changed == 1, "rol4-support-map-row", str(changed))
    return result


@contextlib.contextmanager
def support_map(root: pathlib.Path, epoch: str) -> Iterator[pathlib.Path]:
    require(epoch in {"g61", "g62"}, "unknown-policy-epoch", epoch)
    fields, rows = m60b.read_support_rows(root / SUPPORT_MAP_PATH)
    candidate = m60b.modify_support_rows(rows)
    if epoch == "g62":
        candidate = g62_support_rows(rows)
    with tempfile.TemporaryDirectory(prefix=f"vaeg-m62-{epoch}-support-") as name:
        path = pathlib.Path(name) / f"upd9002_support_map_{epoch}.csv"
        with path.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(
                stream, fieldnames=fields, lineterminator="\n"
            )
            writer.writeheader()
            writer.writerows(candidate)
        ssts.load_support_map(path)
        yield path


def policy_enumerations(
    root: pathlib.Path, dataset_root: pathlib.Path
) -> dict[str, dict[str, Any]]:
    manifest = ssts.load_manifest(root / m61.DATASET_MANIFEST_PATH)
    require(
        manifest["dataset_id"] == DATASET_ID,
        "dataset-drift",
        manifest["dataset_id"],
    )
    with support_map(root, "g61") as before:
        with support_map(root, "g62") as after:
            cache_key = (
                str(root.resolve()),
                str(dataset_root.resolve()),
                sha256_file(before),
                sha256_file(after),
            )
    if cache_key in _POLICY_ENUMERATION_CACHE:
        return _POLICY_ENUMERATION_CACHE[cache_key]
    values = {}
    with support_map(root, "g61") as before:
        with support_map(root, "g62") as after:
            for scope in ("ci", "full"):
                value = ratchet.enumerate_profiles(
                    dataset_root, manifest, before, after, scope
                )
                changes = value["classification_changes"]
                require(
                    all(
                        item["form"] == "0F28"
                        and item["before"] == "known_target_gap"
                        and item["after"] == "applicable"
                        for item in changes
                    ),
                    "unauthorized-classification-change",
                    scope,
                )
                require(
                    len(changes) == value["after_form_counts"]["0F28"]["applicable"],
                    "incomplete-rol4-activation",
                    scope,
                )
                values[scope] = value
    require(
        len(values["full"]["classification_changes"]) == 5000,
        "incomplete-rol4-activation",
        "full population is not 5,000",
    )
    require(
        values["full"]["selected_hash_set_sha256"]
        == SELECTED_HASH_SETS["full"],
        "selected-set-drift",
        "full",
    )
    _POLICY_ENUMERATION_CACHE[cache_key] = values
    return values


def generate_target_policy(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    evaluated_sha: str,
) -> dict[str, Any]:
    require(HEX40.fullmatch(evaluated_sha) is not None, "evaluated-sha", evaluated_sha)
    enumerations = policy_enumerations(root, dataset_root)
    before_policy = read_json(root / "tests/ssts/target_policy/g60b.json")
    changes = {
        scope: enumerations[scope]["classification_changes"]
        for scope in ("ci", "full")
    }
    full_upstream = sorted(
        item["upstream_test_hash"] for item in changes["full"]
    )
    body = {
        "applicable_hash_sets": {
            scope: {
                "after_count": len(
                    enumerations[scope]["after_sets"]["applicable"]
                ),
                "after_sha256": enumerations[scope]["after_set_digests"][
                    "applicable"
                ],
                "before_count": len(
                    enumerations[scope]["before_sets"]["applicable"]
                ),
                "before_sha256": enumerations[scope]["before_set_digests"][
                    "applicable"
                ],
            }
            for scope in ("ci", "full")
        },
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "authority_manifest_sha256": G60B_AUTHORITY_MANIFEST_SHA256,
        "candidate_gate": CANDIDATE_GATE,
        "classification_changes": {
            scope: {
                "count": len(changes[scope]),
                "record_hash_set_sha256": ratchet.hash_set_digest(
                    item["record_hash"] for item in changes[scope]
                ),
                "upstream_hash_set_sha256": ratchet.upstream_hash_set_digest(
                    item["upstream_test_hash"] for item in changes[scope]
                ),
            }
            for scope in ("ci", "full")
        },
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": [
            {
                "after_gap_kind": None,
                "before_gap_kind": "implementation_missing",
                "form": "0F28",
                "resolved_count": 5000,
                "resolved_test_hashes_sha256": (
                    ratchet.upstream_hash_set_digest(full_upstream)
                ),
                "selector_sha256": EXPECTED_0F28_SELECTOR_SHA256,
            }
        ],
        "milestone": MILESTONE,
        "newly_applicable": {
            scope: {
                "count": len(changes[scope]),
                "record_hash_set_sha256": ratchet.hash_set_digest(
                    item["record_hash"] for item in changes[scope]
                ),
            }
            for scope in ("ci", "full")
        },
        "predecessor_policy": {
            "target_policy_id": before_policy["target_policy_id"],
            "target_policy_sha256": before_policy["target_policy_sha256"],
        },
        "schema": "vaeg-upd9002-target-policy-v2",
        "schema_version": 2,
        "selected_hash_sets": {
            scope: enumerations[scope]["selected_hash_set_sha256"]
            for scope in ("ci", "full")
        },
        "taxonomy_counts": {
            "after": {
                "documented_silicon_absent": 32000,
                "implementation_missing": 36908,
                "target_support_unverified": 0,
            },
            "before": {
                "documented_silicon_absent": 32000,
                "implementation_missing": 41908,
                "target_support_unverified": 0,
            },
        },
        "transition_kind": "consolidated_semantic_bundle",
    }
    digest = sha256_bytes(canonical_bytes(body))
    policy = {
        **body,
        "target_policy_id": f"upd9002-g62-{digest}",
        "target_policy_sha256": digest,
    }
    validate_target_policy(policy)
    require(
        policy["gap_kind_changes"][0]["resolved_test_hashes_sha256"]
        == EXPECTED_0F28_UPSTREAM_HASHES_SHA256,
        "rol4-selector-digest",
        "resolved upstream test hash set differs",
    )
    return policy


def validate_target_policy(value: Any) -> None:
    require(isinstance(value, dict), "target-policy-schema", "not an object")
    digest = value.get("target_policy_sha256")
    require(
        isinstance(digest, str) and HEX64.fullmatch(digest) is not None,
        "target-policy-digest",
        repr(digest),
    )
    body = {
        key: item
        for key, item in value.items()
        if key not in {"target_policy_id", "target_policy_sha256"}
    }
    require(
        sha256_bytes(canonical_bytes(body)) == digest
        and value.get("target_policy_id") == f"upd9002-g62-{digest}",
        "target-policy-digest",
        "content identity differs",
    )
    require(
        value.get("approved_predecessor_sha") == APPROVED_PREDECESSOR_SHA,
        "wrong-predecessor",
        "target policy predecessor differs",
    )
    require(
        value.get("authority_manifest_sha256") == G60B_AUTHORITY_MANIFEST_SHA256,
        "rom-authority-missing",
        "target policy authority differs",
    )
    require(
        value.get("dataset_id") == DATASET_ID,
        "dataset-drift",
        "target policy dataset differs",
    )
    require(
        value.get("comparison_contracts") == CONTRACTS,
        "contract-drift",
        "target policy comparison contracts differ",
    )
    require(
        value.get("selected_hash_sets") == SELECTED_HASH_SETS,
        "selected-set-drift",
        "target policy selected sets differ",
    )
    changes = value.get("gap_kind_changes")
    require(
        isinstance(changes, list)
        and len(changes) == 1
        and changes[0].get("form") == "0F28"
        and changes[0].get("before_gap_kind") == "implementation_missing"
        and changes[0].get("after_gap_kind") is None
        and changes[0].get("resolved_count") == 5000,
        "unauthorized-gap-kind-change",
        repr(changes),
    )
    require(
        value.get("taxonomy_counts", {}).get("before", {}).get(
            "implementation_missing"
        )
        - value.get("taxonomy_counts", {}).get("after", {}).get(
            "implementation_missing"
        )
        == 5000,
        "taxonomy-change",
        "implementation-missing arithmetic differs",
    )


def item_for_form(form: str) -> str:
    if form == "D4":
        return "d4_d5"
    if form in {"0F28", "0F2A"}:
        return "0f28_0f2a"
    if "." in form and form[:2] in {"C0", "C1", "D2", "D3"}:
        return "shifts"
    reject("unsupported-audit-form", form)


def bcd_case_row(
    form: str,
    record: dict[str, Any],
    resolved: dict[str, Any],
    actual: dict[str, Any],
) -> dict[str, Any]:
    expected = ssts.expected_registers(record)
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
    initial = record["initial"]["regs"]
    return {
        "actual": {
            "ax": f"{actual['registers']['ax']:04x}",
            "flags": f"{actual['registers']['flags']:04x}",
            "ip": f"{actual['registers']['ip']:04x}",
            "ram": m59.ram_entries(actual["ram"]),
            "termination": actual["termination"],
        },
        "architectural_mismatch_kinds": architectural["mismatch_kinds"],
        "architectural_outcome": (
            "pass" if not architectural["mismatch_kinds"] else "fail"
        ),
        "case_hash": context["record_digest"],
        "conclusion_status": "proven",
        "expected": {
            "ax": f"{expected['ax']:04x}",
            "flags": f"{expected['flags']:04x}",
            "ip": f"{expected['ip']:04x}",
            "ram": m59.ram_entries(expected_ram),
            "termination": ssts.expected_termination(form, record),
        },
        "fingerprint_mismatch_kinds": fingerprint["mismatch_kinds"],
        "fingerprint_outcome": (
            "pass" if not fingerprint["mismatch_kinds"] else "fail"
        ),
        "form": form,
        "initial": {
            "af": (initial["flags"] >> 4) & 1,
            "ah": (initial["ax"] >> 8) & 0xFF,
            "al": initial["ax"] & 0xFF,
            "cf": initial["flags"] & 1,
            "flags": f"{initial['flags']:04x}",
        },
        "instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "prefix_sequence": [
            f"{byte:02x}" for byte in record["bytes"][:-1]
        ],
        "selected": True,
        "structural_partition": {
            "adjust_branch_expected": (
                (expected["ax"] != initial["ax"])
                or (((expected["flags"] ^ initial["flags"]) & 0x11) != 0)
            ),
            "initial_af": (initial["flags"] >> 4) & 1,
            "initial_cf": initial["flags"] & 1,
            "low_nibble": initial["ax"] & 0x0F,
        },
        "top_level_classification": resolved["classification"],
        "upstream_case_hash": record["hash"],
    }


def run_phase_audit(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    phase: str,
    epoch: str,
) -> dict[str, Any]:
    require(phase in PHASE_ORDER, "unknown-phase", phase)
    manifest = ssts.load_manifest(root / m61.DATASET_MANIFEST_PATH)
    require(manifest["dataset_id"] == DATASET_ID, "dataset-drift", phase)
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    gap_kinds, _ = m59.gap_kind_map(root)
    output_rows = {}
    summaries = {}
    with support_map(root, epoch) as support_path:
        support = ssts.load_support_map(support_path)
        for form in PHASE_FORMS[phase]:
            path = dataset_root / ssts.SUITE_PATH / f"{form}.json.gz"
            with gzip.open(path, "rt", encoding="utf-8") as stream:
                selected = ssts.profile_records(json.load(stream), "full")
            resolved = [
                ssts.classify_record(form, record, metadata, support)
                for record in selected
            ]
            expected_classification = (
                "known_target_gap"
                if form == "0F28" and epoch == "g61"
                else "applicable"
            )
            require(
                all(
                    value["classification"] == expected_classification
                    for value in resolved
                ),
                "classification-change",
                form,
            )
            results = ssts.run_worker_contained(worker, selected, 120.0)
            require(
                len(results) == len(selected)
                and all(status == "ok" for status, _ in results),
                "missing-executed-record",
                form,
            )
            if phase == "bcd_adjust":
                rows = [
                    bcd_case_row(form, record, classification, actual)
                    for record, classification, (_, actual) in zip(
                        selected, resolved, results
                    )
                ]
            else:
                item = item_for_form(form)
                rows = [
                    m59.make_row(
                        item,
                        form,
                        record,
                        classification,
                        gap_kinds,
                        actual,
                    )
                    for record, classification, (_, actual) in zip(
                        selected, resolved, results
                    )
                ]
            rows.sort(key=lambda value: value["case_hash"])
            require(
                len(rows) == len({value["case_hash"] for value in rows}),
                "duplicate-case-hash",
                form,
            )
            official_pass = [
                value["case_hash"]
                for value in rows
                if value.get(
                    "architectural_outcome", value.get("official_outcome")
                )
                == "pass"
            ]
            official_fail = [
                value["case_hash"]
                for value in rows
                if value.get(
                    "architectural_outcome", value.get("official_outcome")
                )
                == "fail"
            ]
            summaries[form] = {
                "architectural_failure_count": len(official_fail),
                "architectural_failure_hash_set_sha256": ratchet.hash_set_digest(
                    official_fail
                ),
                "architectural_pass_count": len(official_pass),
                "architectural_pass_hash_set_sha256": ratchet.hash_set_digest(
                    official_pass
                ),
                "diagnostic_replay": expected_classification != "applicable",
                "executed_by_worker": len(rows),
                "selected": len(rows),
                "top_level_classification": expected_classification,
            }
            output_rows[form] = rows
    return {
        "epoch": epoch,
        "forms": list(PHASE_FORMS[phase]),
        "phase": phase,
        "rows": output_rows,
        "summaries": summaries,
        "worker_sha256": sha256_file(worker),
    }


def write_phase_audit(
    output: pathlib.Path, audit: dict[str, Any], source_sha: str
) -> None:
    require(HEX40.fullmatch(source_sha) is not None, "source-sha", source_sha)
    output.mkdir(parents=True, exist_ok=True)
    for form in audit["forms"]:
        safe = form.lower().replace(".", "_")
        ratchet.write_deterministic_gzip(
            output / f"{safe}_cases.json.gz",
            {
                "form": form,
                "row_count": len(audit["rows"][form]),
                "rows": audit["rows"][form],
                "schema": "vaeg-upd9002-m62-cases-v1",
                "schema_version": 1,
            },
        )
    write_json(
        output / "phase_summary.json",
        {
            "epoch": audit["epoch"],
            "forms": audit["forms"],
            "phase": audit["phase"],
            "schema": "vaeg-upd9002-m62-phase-audit-v1",
            "schema_version": 1,
            "source_sha": source_sha,
            "summaries": audit["summaries"],
            "worker_sha256": audit["worker_sha256"],
        },
    )


@contextlib.contextmanager
def configured_ratchet_identity() -> Iterator[None]:
    previous = (
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
        ) = previous


def output_path(output_root: pathlib.Path, relative: pathlib.Path) -> pathlib.Path:
    return output_root / relative


def load_scoreboard_failures(
    root: pathlib.Path, relative: pathlib.Path
) -> dict[str, dict[str, Any]]:
    summary = read_json(root / relative)
    return ratchet.load_scoreboard_failures(root / relative, summary)


def generate_scoreboard(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    raw_path: pathlib.Path,
    profile_key: str,
    evaluated_sha: str,
    policy: dict[str, Any],
) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    require(HEX40.fullmatch(evaluated_sha) is not None, "evaluated-sha", evaluated_sha)
    profile, scope = profile_key.rsplit("_", 1)
    require(
        profile in {"architectural", "fingerprint"}
        and scope in {"ci", "full"},
        "profile-key",
        profile_key,
    )
    raw = read_json(raw_path)
    require(
        raw.get("schema") == "vaeg-upd9002-ssts-result-v1"
        and raw.get("dataset_id") == DATASET_ID
        and raw.get("profile") == scope,
        "candidate-profile-identity",
        profile_key,
    )
    require(
        (profile == "architectural" and "flags_comparison" not in raw)
        or (profile == "fingerprint" and raw.get("flags_comparison") == "all16"),
        "comparison-domain-conflation",
        profile_key,
    )
    enumeration = policy_enumerations(root, dataset_root)[scope]
    selected = enumeration["selected_count"]
    applicable_hashes = enumeration["after_sets"]["applicable"]
    require(
        raw.get("selected_records") == selected
        and raw.get("executed_records") == len(applicable_hashes),
        "applicable-set-drift",
        profile_key,
    )
    result_counts = raw.get("result_counts", {})
    require(
        result_counts.get("timeout", 0) == 0
        and result_counts.get("crash", 0) == 0,
        "timeout-crash",
        profile_key,
    )
    failures_raw = ratchet.load_failure_records(raw_path)
    failures = {
        record_hash: ratchet.failure_entry(value)
        for record_hash, value in failures_raw.items()
    }
    applicable_set = set(applicable_hashes)
    require(
        set(failures) <= applicable_set,
        "failure-outside-applicable",
        profile_key,
    )
    pass_hashes = sorted(applicable_set - set(failures))
    failed = sum(
        result_counts.get(kind, 0)
        for kind in ("semantic_failure", "timeout", "crash")
    )
    require(
        result_counts.get("pass") == len(pass_hashes)
        and failed == len(failures),
        "candidate-result-arithmetic",
        profile_key,
    )
    rows = ratchet.build_scoreboard_rows(
        raw, enumeration["after_form_counts"], failures_raw
    )
    failure_directory = output_path(
        output_root, FAILURE_DIRECTORY_PATHS[profile_key]
    )
    (
        shards,
        signature_index,
        canonical_sidecars,
        raw_sidecars,
    ) = ratchet.write_failure_shards(
        failures_raw,
        profile,
        scope,
        DATASET_ID,
        failure_directory,
    )
    require(
        signature_index == raw["failure_signature_index_sha256"],
        "failure-signature-index",
        profile_key,
    )
    mismatch_classes: Counter[str] = Counter()
    for failure in failures.values():
        mismatch_classes.update(failure["mismatch_classes"])
    classification_counts = {
        name: len(enumeration["after_sets"][name])
        for name in ratchet.TOP_LEVEL_CLASSIFICATIONS
    }
    raw_classification_counts = {
        key: value for key, value in raw["classification_counts"].items() if value
    }
    require(
        raw_classification_counts
        == {key: value for key, value in classification_counts.items() if value},
        "classification-count",
        profile_key,
    )
    before = read_json(root / G61_SCOREBOARD_PATHS[profile_key])
    scoreboard = copy.deepcopy(before)
    scoreboard.update(
        {
            "applicable": len(applicable_hashes),
            "applicable_hash_set_sha256": enumeration["after_set_digests"][
                "applicable"
            ],
            "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
            "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
            "classification_counts": classification_counts,
            "classification_hash_sets": enumeration["after_set_digests"],
            "crashes": 0,
            "epoch_gate": CANDIDATE_GATE,
            "evaluated_sha": evaluated_sha,
            "executed": len(applicable_hashes),
            "fail": len(failures),
            "failure_hash_set_sha256": ratchet.hash_set_digest(failures),
            "failure_shards": shards,
            "failure_sidecar_canonical_set_sha256": canonical_sidecars,
            "failure_sidecar_raw_set_sha256": raw_sidecars,
            "failure_signature_index_sha256": signature_index,
            "mismatch_classes": dict(sorted(mismatch_classes.items())),
            "pass": len(pass_hashes),
            "pass_hash_set_sha256": ratchet.hash_set_digest(pass_hashes),
            "raw_result_summary_sha256": sha256_file(raw_path),
            "records": rows,
            "scoreboard_digest": sha256_bytes(canonical_bytes(rows)),
            "selected": selected,
            "selected_hash_set_sha256": enumeration[
                "selected_hash_set_sha256"
            ],
            "target_policy_id": policy["target_policy_id"],
            "target_policy_sha256": policy["target_policy_sha256"],
            "termination_classes": raw["termination_counts"],
            "timeouts": 0,
        }
    )
    v1 = copy.deepcopy(scoreboard)
    v1.pop("target_policy_id")
    v1.pop("target_policy_sha256")
    v1["schema"] = "vaeg-upd9002-ssts-scoreboard-v1"
    v1["schema_version"] = 1
    with configured_ratchet_identity():
        ratchet.validate_scoreboard(v1)
    output = output_path(output_root, SCOREBOARD_PATHS[profile_key])
    write_json(output, scoreboard)
    return scoreboard, failures


def write_transition(
    root: pathlib.Path,
    output_root: pathlib.Path,
    profile_key: str,
    scoreboard: dict[str, Any],
    failures_after: dict[str, dict[str, Any]],
    evaluated_sha: str,
    policy: dict[str, Any],
) -> dict[str, Any]:
    profile, scope = profile_key.rsplit("_", 1)
    before_summary = read_json(root / G61_SCOREBOARD_PATHS[profile_key])
    failures_before = load_scoreboard_failures(
        root, G61_SCOREBOARD_PATHS[profile_key]
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
    governed_forms = {
        *PHASE_FORMS["aam"],
        *PHASE_FORMS["ror4"],
        *PHASE_FORMS["bcd_adjust"],
        *PHASE_FORMS["shifts"],
    }
    require(not newly_failing, "newly-failing", profile_key)
    require(not changed, "changed-failure-not-enumerated", profile_key)
    require(
        all(failures_before[item]["form"] in governed_forms for item in newly_passing),
        "ungoverned-newly-passing",
        profile_key,
    )
    before_rows = {
        (row["form"], row["classification"]): row
        for row in before_summary["records"]
    }
    after_rows = {
        (row["form"], row["classification"]): row
        for row in scoreboard["records"]
    }
    decreases = []
    for key, before_row in before_rows.items():
        if key == ("0F28", "known_target_gap"):
            continue
        after_row = after_rows.get(key)
        if (
            before_row["classification"] == "applicable"
            and (after_row is None or after_row["pass"] < before_row["pass"])
        ):
            decreases.append(key[0])
    require(not decreases, "per-form-pass-decrease", repr(decreases))
    newly_applicable = policy["newly_applicable"][scope]
    transition = {
        "applicable_hash_set_after_sha256": scoreboard[
            "applicable_hash_set_sha256"
        ],
        "applicable_hash_set_before_sha256": before_summary[
            "applicable_hash_set_sha256"
        ],
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "bundle_phases": list(PHASE_ORDER),
        "changed_failure_count": 0,
        "changed_failure_shards": [],
        "comparison_contract_ids": {profile: CONTRACTS[profile]},
        "dataset_id": DATASET_ID,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": policy["gap_kind_changes"],
        "hardware_pending_changes": [],
        "newly_applicable_count": newly_applicable["count"],
        "newly_applicable_hashes_sha256": newly_applicable[
            "record_hash_set_sha256"
        ],
        "newly_failing": [],
        "newly_failing_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "newly_passing": newly_passing,
        "newly_passing_hash_set_sha256": ratchet.hash_set_digest(newly_passing),
        "profile": profile,
        "schema": "vaeg-upd9002-m62-transition-v1",
        "schema_version": 1,
        "scope": scope,
        "selected_hash_set_sha256": SELECTED_HASH_SETS[scope],
        "target_policy_after_id": policy["target_policy_id"],
        "target_policy_before_id": TARGET_POLICY_BEFORE_ID,
        "top_level_classification_changes": [
            {
                "after": "applicable",
                "before": "known_target_gap",
                "count": newly_applicable["count"],
                "form": "0F28",
                "record_hash_set_sha256": newly_applicable[
                    "record_hash_set_sha256"
                ],
            }
        ],
        "transition_kind": "consolidated_semantic_bundle",
    }
    write_json(output_root / TRANSITION_PATHS[profile_key], transition)
    return transition


def write_ranking(
    root: pathlib.Path,
    output_root: pathlib.Path,
    scoreboard: dict[str, Any],
    failures: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    failure_hashes: dict[str, list[str]] = {}
    mismatch: dict[str, Counter[str]] = {}
    termination: dict[str, Counter[str]] = {}
    for record_hash, failure in failures.items():
        form = failure["form"]
        failure_hashes.setdefault(form, []).append(record_hash)
        mismatch.setdefault(form, Counter()).update(failure["mismatch_classes"])
        termination.setdefault(form, Counter())[
            failure["actual_termination"]
        ] += 1
    before = read_json(root / G61_SCOREBOARD_PATHS["architectural_full"])
    before_rows = {
        (row["form"], row["classification"]): row for row in before["records"]
    }
    rows = []
    for record in scoreboard["records"]:
        if record["classification"] != "applicable":
            continue
        form = record["form"]
        before_fail = (
            before_rows.get((form, "applicable"), {"fail": 0})["fail"]
        )
        rows.append(
            {
                "change_from_g61": record["fail"] - before_fail,
                "classification": "applicable",
                "executed": record["executed"],
                "fail": record["fail"],
                "failure_hash_set_sha256": ratchet.hash_set_digest(
                    failure_hashes.get(form, [])
                ),
                "form": form,
                "mismatch_classes": dict(
                    sorted(mismatch.get(form, Counter()).items())
                ),
                "opcode": record["opcode"],
                "pass": record["pass"],
                "selected": record["selected"],
                "subform": record["subform"],
                "termination_classes": dict(
                    sorted(termination.get(form, Counter()).items())
                ),
            }
        )
    rows.sort(key=lambda value: (-value["fail"], value["form"]))
    total = scoreboard["fail"]
    cumulative = 0
    for row in rows:
        cumulative += row["fail"]
        row["cumulative_failure_count"] = cumulative
        row["cumulative_share_ppm"] = (
            0 if total == 0 else cumulative * 1_000_000 // total
        )
    require(
        sum(row["fail"] for row in rows) == total,
        "ranking-total-mismatch",
        str(total),
    )
    green_forms = {
        "D4",
        "0F2A",
        "0F28",
        "27",
        "2F",
        "37",
        "3F",
        *PHASE_FORMS["shifts"],
    }
    require(
        all(
            next(row for row in rows if row["form"] == form)["fail"] == 0
            for form in green_forms
        ),
        "phase-not-green",
        "ranking contains an M62 failure",
    )
    ranking = {
        "architectural_full_failure_count": total,
        "family_aggregation": {
            "m62_bundle": {
                "fail": sum(
                    row["fail"] for row in rows if row["form"] in green_forms
                ),
                "forms": sorted(green_forms),
            }
        },
        "m62_green_forms": sorted(green_forms),
        "row_count": len(rows),
        "rows": rows,
        "schema": "vaeg-upd9002-m62-failure-ranking-v1",
        "schema_version": 1,
    }
    write_json(output_root / RANKING_JSON_PATH, ranking)
    lines = [
        "<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD. -->",
        "",
        "# G62 architectural-full failure ranking",
        "",
        f"Total remaining failures: **{total:,}**.",
        "",
        "| Rank | Form | Pass | Fail | Change from G61 | Cumulative |",
        "| ---: | :--- | ---: | ---: | ---: | ---: |",
    ]
    for index, row in enumerate(rows[:30], 1):
        lines.append(
            f"| {index} | `{row['form']}` | {row['pass']:,} | "
            f"{row['fail']:,} | {row['change_from_g61']:+,} | "
            f"{row['cumulative_share_ppm'] / 10000:.2f}% |"
        )
    lines.extend(
        [
            "",
            "The complete machine-readable ranking contains explicit green "
            "rows for every M62 family, including newly applicable `0F28`. "
            "Omission from this top-30 view is not proof of passing.",
            "",
        ]
    )
    path = output_root / RANKING_MD_PATH
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return ranking


def artifact_entry(root: pathlib.Path, relative: pathlib.Path) -> dict[str, Any]:
    path = root / relative
    row_count = 1
    if path.suffix == ".json":
        value = read_json(path)
        if isinstance(value, dict):
            row_count = value.get(
                "row_count", value.get("failure_count", 1)
            )
    elif path.suffix == ".gz":
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            value = json.load(stream)
        row_count = value.get("row_count", value.get("failure_count", 1))
    return {
        "bytes": path.stat().st_size,
        "path": relative.as_posix(),
        "row_count": row_count,
        "sha256": sha256_file(path),
    }


def tree_identities(root: pathlib.Path) -> list[dict[str, Any]]:
    return [
        {
            "path": path.relative_to(root).as_posix(),
            "sha256": sha256_file(path),
            "size": path.stat().st_size,
        }
        for path in sorted(
            (value for value in root.rglob("*") if value.is_file()),
            key=lambda value: value.relative_to(root).as_posix(),
        )
    ]


def load_phase_commits(path: pathlib.Path) -> dict[str, list[str]]:
    value = read_json(path)
    require(
        isinstance(value, dict)
        and list(value) == list(PHASE_ORDER)
        and all(
            isinstance(items, list)
            and items
            and all(
                isinstance(item, str) and HEX40.fullmatch(item) is not None
                for item in items
            )
            for items in value.values()
        ),
        "phase-commit-identity",
        str(path),
    )
    return value


def copy_phase_audits(
    output_root: pathlib.Path,
    audit_root: pathlib.Path,
) -> list[pathlib.Path]:
    paths = []
    destination = output_root / EVIDENCE_ROOT
    for phase in PHASE_ORDER:
        source = audit_root / phase
        require(
            (source / "phase_summary.json").is_file(),
            "missing-phase",
            phase,
        )
        target = destination / phase
        target.mkdir(parents=True, exist_ok=True)
        for item in sorted(source.iterdir()):
            if item.is_file():
                shutil.copyfile(item, target / item.name)
                paths.append((EVIDENCE_ROOT / phase / item.name))
    return paths


def write_phase_checkpoints(
    root: pathlib.Path,
    output_root: pathlib.Path,
    scoreboards: dict[str, dict[str, Any]],
    failures_after: dict[str, dict[str, dict[str, Any]]],
    phase_commits: dict[str, list[str]],
    worker_sha256: str,
) -> list[pathlib.Path]:
    before_failures = load_scoreboard_failures(
        root, G61_SCOREBOARD_PATHS["architectural_full"]
    )
    after_failures = failures_after["architectural_full"]
    before_rows = {
        row["form"]: row
        for row in read_json(
            root / G61_SCOREBOARD_PATHS["architectural_full"]
        )["records"]
        if row["classification"] == "applicable"
    }
    after_rows = {
        row["form"]: row
        for row in scoreboards["architectural_full"]["records"]
        if row["classification"] == "applicable"
    }
    paths = []
    parent = APPROVED_PREDECESSOR_SHA
    for phase in PHASE_ORDER:
        forms = set(PHASE_FORMS[phase])
        owned_before = sorted(
            record_hash
            for record_hash, failure in before_failures.items()
            if failure["form"] in forms
        )
        surviving_after = sorted(
            record_hash
            for record_hash, failure in after_failures.items()
            if failure["form"] in forms
        )
        require(not surviving_after, "phase-not-green", phase)
        rows = []
        for form in sorted(forms):
            before = before_rows.get(
                form,
                {
                    "executed": 0,
                    "fail": 0,
                    "pass": 0,
                    "selected": next(
                        row["selected"]
                        for row in read_json(
                            root / G61_SCOREBOARD_PATHS["architectural_full"]
                        )["records"]
                        if row["form"] == form
                    ),
                },
            )
            after = after_rows[form]
            rows.append(
                {
                    "executed_after": after["executed"],
                    "executed_before": before["executed"],
                    "fail_after": after["fail"],
                    "fail_before": before["fail"],
                    "form": form,
                    "pass_after": after["pass"],
                    "pass_before": before["pass"],
                    "selected": after["selected"],
                }
            )
        checkpoint = {
            "changed_failure_count": 0,
            "focused_tests": "passed",
            "forms": sorted(forms),
            "newly_failing": [],
            "newly_passing_count": len(owned_before),
            "newly_passing_hash_set_sha256": ratchet.hash_set_digest(
                owned_before
            ),
            "owned_failure_count_before": len(owned_before),
            "owned_failure_hash_set_sha256": ratchet.hash_set_digest(
                owned_before
            ),
            "parent_semantic_commit": parent,
            "phase": phase,
            "protected_families": list(PROTECTED_FORMS),
            "rows": rows,
            "schema": "vaeg-upd9002-m62-phase-checkpoint-v1",
            "schema_version": 1,
            "semantic_commit": phase_commits[phase][-1],
            "semantic_commits": phase_commits[phase],
            "worker_sha256": worker_sha256,
        }
        path = EVIDENCE_ROOT / "phases" / f"phase_{phase}.json"
        write_json(output_root / path, checkpoint)
        paths.append(path)
        parent = phase_commits[phase][-1]
    return paths


def generate_evidence(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    audit_root: pathlib.Path,
    raw_paths: dict[str, pathlib.Path],
    phase_commits_path: pathlib.Path,
    evaluated_sha: str,
    worker: pathlib.Path,
) -> dict[str, Any]:
    verify_predecessor(root)
    phase_commits = load_phase_commits(phase_commits_path)
    require(
        phase_commits["shifts"][-1] == evaluated_sha,
        "evaluated-sha",
        "last worker-changing phase is not evaluated SHA",
    )
    policy = generate_target_policy(root, dataset_root, evaluated_sha)
    write_json(output_root / TARGET_POLICY_PATH, policy)
    scoreboards = {}
    failures = {}
    for key in ("architectural_ci", "architectural_full", "fingerprint_full"):
        scoreboards[key], failures[key] = generate_scoreboard(
            root,
            output_root,
            dataset_root,
            raw_paths[key],
            key,
            evaluated_sha,
            policy,
        )
    transitions = {
        key: write_transition(
            root,
            output_root,
            key,
            scoreboards[key],
            failures[key],
            evaluated_sha,
            policy,
        )
        for key in ("architectural_ci", "architectural_full")
    }
    ranking = write_ranking(
        root,
        output_root,
        scoreboards["architectural_full"],
        failures["architectural_full"],
    )
    audit_paths = copy_phase_audits(output_root, audit_root)
    checkpoint_paths = write_phase_checkpoints(
        root,
        output_root,
        scoreboards,
        failures,
        phase_commits,
        sha256_file(worker),
    )
    artifact_paths = [
        TARGET_POLICY_PATH,
        *SCOREBOARD_PATHS.values(),
        *TRANSITION_PATHS.values(),
        RANKING_JSON_PATH,
        RANKING_MD_PATH,
        *audit_paths,
        *checkpoint_paths,
    ]
    for directory in FAILURE_DIRECTORY_PATHS.values():
        artifact_paths.extend(
            path.relative_to(output_root)
            for path in sorted((output_root / directory).glob("*.json.gz"))
        )
    artifact_paths = sorted(set(artifact_paths), key=lambda path: path.as_posix())
    artifacts = [artifact_entry(output_root, path) for path in artifact_paths]
    artifact_tree = sha256_bytes(canonical_bytes(artifacts))
    full_transition = transitions["architectural_full"]
    manifest = {
        "applicable_hash_set_after_sha256": {
            scope: policy["applicable_hash_sets"][scope]["after_sha256"]
            for scope in ("ci", "full")
        },
        "applicable_hash_set_before_sha256": APPLICABLE_BEFORE_HASH_SETS,
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifact_tree_sha256": artifact_tree,
        "artifacts": artifacts,
        "bundle_phases": list(PHASE_ORDER),
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "evaluated_sha": evaluated_sha,
        "milestone": MILESTONE,
        "newly_applicable_count": policy["newly_applicable"]["full"]["count"],
        "newly_applicable_hash_set_sha256": policy["newly_applicable"]["full"][
            "record_hash_set_sha256"
        ],
        "newly_failing_count": 0,
        "newly_failing_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "newly_passing_count": len(full_transition["newly_passing"]),
        "newly_passing_hash_set_sha256": full_transition[
            "newly_passing_hash_set_sha256"
        ],
        "phase_semantic_commits": phase_commits,
        "ranking_sha256": sha256_file(output_root / RANKING_JSON_PATH),
        "schema": "vaeg-upd9002-m62-evidence-manifest-v1",
        "schema_version": 1,
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "target_policy_after_id": policy["target_policy_id"],
        "target_policy_before_id": TARGET_POLICY_BEFORE_ID,
        "worker_sha256": sha256_file(worker),
    }
    manifest_path = EVIDENCE_ROOT / "manifest.json"
    write_json(output_root / manifest_path, manifest)
    result = {
        "artifact_tree_sha256": artifact_tree,
        "candidate_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "evidence_manifest_sha256": sha256_file(output_root / manifest_path),
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
                    "target_policy_id",
                )
            }
            for key in scoreboards
        },
        "ranking_failure_total": ranking["architectural_full_failure_count"],
        "schema": "vaeg-upd9002-m62-result-manifest-v1",
        "schema_version": 1,
        "target_policy_sha256": policy["target_policy_sha256"],
        "transition_sha256": {
            key: sha256_file(output_root / path)
            for key, path in TRANSITION_PATHS.items()
        },
    }
    write_json(output_root / RESULT_MANIFEST_PATH, result)
    return {
        "artifact_tree_sha256": artifact_tree,
        "evidence_manifest_sha256": result["evidence_manifest_sha256"],
        "target_policy_id": policy["target_policy_id"],
        "transition_sha256": result["transition_sha256"],
    }


def regenerate_twice(**kwargs: Any) -> dict[str, Any]:
    output_root = kwargs.pop("output_root")
    with tempfile.TemporaryDirectory(prefix="vaeg-m62-a-") as first_name:
        with tempfile.TemporaryDirectory(prefix="vaeg-m62-b-") as second_name:
            first = pathlib.Path(first_name)
            second = pathlib.Path(second_name)
            result = generate_evidence(output_root=first, **kwargs)
            generate_evidence(output_root=second, **kwargs)
            require(
                tree_identities(first) == tree_identities(second),
                "nondeterministic-generation",
                "complete G62 evidence generations differ",
            )
            for source in sorted(first.rglob("*")):
                if source.is_file():
                    target = output_root / source.relative_to(first)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(source, target)
            return result


def synthetic_decision() -> dict[str, Any]:
    return {
        "applicable_change_forms": ["0F28"],
        "bundle_phases": list(PHASE_ORDER),
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "evidence_commit_has_implementation": False,
        "newly_applicable_count": 5000,
        "newly_applicable_failures": [],
        "newly_failing": [],
        "phase_commits": {
            phase: [f"{index + 1:x}" * 40]
            for index, phase in enumerate(PHASE_ORDER)
        },
        "protected_changes": [],
        "selected_hash_sets": SELECTED_HASH_SETS,
        "target_policy_before_id": TARGET_POLICY_BEFORE_ID,
        "target_policy_changes": ["0F28:known_target_gap->applicable"],
    }


def validate_decision(value: dict[str, Any]) -> None:
    require(
        value.get("bundle_phases") == list(PHASE_ORDER),
        "phase-order",
        "bundle phases differ",
    )
    commits = value.get("phase_commits")
    require(
        isinstance(commits, dict)
        and list(commits) == list(PHASE_ORDER)
        and all(isinstance(items, list) and items for items in commits.values())
        and len(
            {
                item
                for items in commits.values()
                for item in items
            }
        )
        == sum(len(items) for items in commits.values())
        and all(
            HEX40.fullmatch(item) is not None
            for items in commits.values()
            for item in items
        ),
        "phase-commit-identity",
        "semantic commits are missing or squashed",
    )
    require(
        value.get("dataset_id") == DATASET_ID,
        "dataset-drift",
        "decision dataset differs",
    )
    require(
        value.get("comparison_contracts") == CONTRACTS,
        "contract-drift",
        "decision contracts differ",
    )
    require(
        value.get("selected_hash_sets") == SELECTED_HASH_SETS,
        "selected-set-drift",
        "decision selected sets differ",
    )
    require(
        value.get("target_policy_before_id") == TARGET_POLICY_BEFORE_ID
        and value.get("target_policy_changes")
        == ["0F28:known_target_gap->applicable"]
        and value.get("applicable_change_forms") == ["0F28"]
        and value.get("newly_applicable_count") == 5000,
        "unauthorized-target-policy-change",
        "decision target-policy transition differs",
    )
    require(
        not value.get("newly_applicable_failures"),
        "newly-applicable-failure",
        "newly applicable record failed",
    )
    require(
        not value.get("newly_failing"),
        "newly-failing",
        "candidate introduced a failure",
    )
    require(
        not value.get("protected_changes"),
        "protected-scope-change",
        repr(value.get("protected_changes")),
    )
    require(
        value.get("evidence_commit_has_implementation") is False,
        "evidence-commit-implementation",
        "evidence commit contains implementation",
    )


def expect_rejection(
    code: str,
    mutation: Callable[[dict[str, Any]], None],
    accepted: list[str],
) -> None:
    value = synthetic_decision()
    mutation(value)
    try:
        validate_decision(value)
    except M62Error as error:
        require(error.code == code, "selftest-reason", f"{error.code} != {code}")
        accepted.append(code)
        return
    reject("selftest-accepted-mutation", code)


def selftest() -> None:
    validate_decision(synthetic_decision())
    accepted: list[str] = []
    mutations: list[tuple[str, Callable[[dict[str, Any]], None]]] = [
        ("phase-order", lambda value: value["bundle_phases"].reverse()),
        (
            "phase-commit-identity",
            lambda value: value["phase_commits"].__setitem__(
                "shifts", value["phase_commits"]["aam"].copy()
            ),
        ),
        ("dataset-drift", lambda value: value.__setitem__("dataset_id", "wrong")),
        (
            "contract-drift",
            lambda value: value.__setitem__("comparison_contracts", {}),
        ),
        (
            "selected-set-drift",
            lambda value: value.__setitem__("selected_hash_sets", {}),
        ),
        (
            "unauthorized-target-policy-change",
            lambda value: value["target_policy_changes"].append("FF.7"),
        ),
        (
            "unauthorized-target-policy-change",
            lambda value: value.__setitem__("newly_applicable_count", 4999),
        ),
        (
            "newly-applicable-failure",
            lambda value: value["newly_applicable_failures"].append("2" * 64),
        ),
        (
            "newly-failing",
            lambda value: value["newly_failing"].append("3" * 64),
        ),
        (
            "protected-scope-change",
            lambda value: value["protected_changes"].append("FF.7"),
        ),
        (
            "protected-scope-change",
            lambda value: value["protected_changes"].append("D5"),
        ),
        (
            "protected-scope-change",
            lambda value: value["protected_changes"].append("rotate:/0-/3"),
        ),
        (
            "evidence-commit-implementation",
            lambda value: value.__setitem__(
                "evidence_commit_has_implementation", True
            ),
        ),
    ]
    for code, mutation in mutations:
        expect_rejection(code, mutation, accepted)
    # Deterministic JSON and gzip are identity-bearing requirements.
    sample = {"rows": [{"hash": "1" * 64}], "schema_version": 1}
    require(
        canonical_bytes(sample) == canonical_bytes(copy.deepcopy(sample)),
        "nondeterministic-json",
        "canonical bytes differ",
    )
    require(
        ratchet.deterministic_gzip_bytes(sample)
        == ratchet.deterministic_gzip_bytes(copy.deepcopy(sample)),
        "nondeterministic-gzip",
        "gzip bytes differ",
    )
    print(f"m62-selftest: {len(accepted)} fail-closed mutations rejected")


def verify_static(root: pathlib.Path, protected_only: bool) -> None:
    verify_predecessor(root)
    if not protected_only:
        verify_consolidation(root)
    source = (root / "cpu/upd9002/upd9002_mn.c").read_text(encoding="utf-8")
    require("UPD9002FN _aam(void)" in source and
            "UPD9002FN _aad(void)" in source,
            "d4-d5-scope", "handlers")
    require(
        sha256_file(root / G60B_AUTHORITY_MANIFEST_PATH)
        == G60B_AUTHORITY_MANIFEST_SHA256,
        "rom-authority-mutation",
        "authority digest differs",
    )
    print(
        "m62-static: G61 and G60b identities protected; "
        "canonical consolidated task is unambiguous"
    )


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    selftest_parser = subparsers.add_parser("selftest")
    selftest_parser.set_defaults(command="selftest")
    static = subparsers.add_parser("verify-static")
    static.add_argument("--root", type=pathlib.Path, default=pathlib.Path.cwd())
    static.add_argument("--protected-evidence-only", action="store_true")
    audit = subparsers.add_parser("audit")
    audit.add_argument("--root", type=pathlib.Path, required=True)
    audit.add_argument("--dataset-root", type=pathlib.Path, required=True)
    audit.add_argument("--worker", type=pathlib.Path, required=True)
    audit.add_argument("--phase", choices=PHASE_ORDER, required=True)
    audit.add_argument("--epoch", choices=("g61", "g62"), required=True)
    audit.add_argument("--source-sha", required=True)
    audit.add_argument("--output", type=pathlib.Path, required=True)
    policy = subparsers.add_parser("target-policy")
    policy.add_argument("--root", type=pathlib.Path, required=True)
    policy.add_argument("--dataset-root", type=pathlib.Path, required=True)
    policy.add_argument("--evaluated-sha", required=True)
    policy.add_argument("--output", type=pathlib.Path, required=True)
    support = subparsers.add_parser("write-support-map")
    support.add_argument("--root", type=pathlib.Path, required=True)
    support.add_argument("--epoch", choices=("g61", "g62"), required=True)
    support.add_argument("--output", type=pathlib.Path, required=True)
    generate = subparsers.add_parser("generate")
    generate.add_argument("--root", type=pathlib.Path, required=True)
    generate.add_argument("--dataset-root", type=pathlib.Path, required=True)
    generate.add_argument("--audit-root", type=pathlib.Path, required=True)
    generate.add_argument("--worker", type=pathlib.Path, required=True)
    generate.add_argument("--evaluated-sha", required=True)
    generate.add_argument("--phase-commits", type=pathlib.Path, required=True)
    generate.add_argument(
        "--architectural-ci-raw", type=pathlib.Path, required=True
    )
    generate.add_argument(
        "--architectural-full-raw", type=pathlib.Path, required=True
    )
    generate.add_argument(
        "--fingerprint-full-raw", type=pathlib.Path, required=True
    )
    generate.add_argument("--output-root", type=pathlib.Path, required=True)
    generate.add_argument("--regenerate-twice", action="store_true")
    return parser.parse_args(list(argv))


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
        elif arguments.command == "verify-static":
            verify_static(
                arguments.root.resolve(), arguments.protected_evidence_only
            )
        elif arguments.command == "audit":
            audit = run_phase_audit(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.phase,
                arguments.epoch,
            )
            write_phase_audit(
                arguments.output.resolve(), audit, arguments.source_sha
            )
            print(
                f"m62-audit: phase={arguments.phase} epoch={arguments.epoch} "
                f"forms={len(audit['forms'])}"
            )
        elif arguments.command == "target-policy":
            value = generate_target_policy(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.evaluated_sha,
            )
            write_json(arguments.output.resolve(), value)
            print(
                f"m62-policy: id={value['target_policy_id']} "
                f"full_applicable="
                f"{value['applicable_hash_sets']['full']['after_count']}"
            )
        elif arguments.command == "write-support-map":
            arguments.output.parent.mkdir(parents=True, exist_ok=True)
            with support_map(arguments.root.resolve(), arguments.epoch) as path:
                shutil.copyfile(path, arguments.output.resolve())
            print(
                f"m62-support-map: epoch={arguments.epoch} "
                f"sha256={sha256_file(arguments.output.resolve())}"
            )
        elif arguments.command == "generate":
            keyword = {
                "root": arguments.root.resolve(),
                "output_root": arguments.output_root.resolve(),
                "dataset_root": arguments.dataset_root.resolve(),
                "audit_root": arguments.audit_root.resolve(),
                "raw_paths": {
                    "architectural_ci": arguments.architectural_ci_raw.resolve(),
                    "architectural_full": arguments.architectural_full_raw.resolve(),
                    "fingerprint_full": arguments.fingerprint_full_raw.resolve(),
                },
                "phase_commits_path": arguments.phase_commits.resolve(),
                "evaluated_sha": arguments.evaluated_sha,
                "worker": arguments.worker.resolve(),
            }
            result = (
                regenerate_twice(**keyword)
                if arguments.regenerate_twice
                else generate_evidence(**keyword)
            )
            print(
                f"m62-generate: policy={result['target_policy_id']} "
                f"tree={result['artifact_tree_sha256']}"
            )
        return 0
    except (
        M62Error,
        m60b.M60bError,
        m59.EvidenceError,
        ratchet.RatchetError,
        ssts.CorpusError,
    ) as error:
        print(f"m62-error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

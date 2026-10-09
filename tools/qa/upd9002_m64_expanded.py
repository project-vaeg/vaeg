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
"""Audit expanded M64 ownership and derive the exact G64 target policy."""

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
from collections.abc import Iterable, Iterator
from typing import Any

import upd9002_m60b_authority as m60b
import upd9002_m61_mov_imm as m61
import upd9002_m62_bundle as m62
import upd9002_semantics_evidence as m59
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M64"
CANDIDATE_GATE = "G64"
APPROVED_PREDECESSOR_GATE = "G62"
APPROVED_PREDECESSOR_SHA = "70b8e94e96aef4cb79eed72c7813c4148c5c0dd8"
G62_EVALUATED_SHA = "2cdaed95072d74bbf7187ae854fb31d3886c995d"
G62_POLICY_ID = (
    "upd9002-g62-"
    "6961b0f295110d32d16799cb3799bedff7600b9b956bc4ad893eebc249140212"
)
G62_POLICY_SHA256 = (
    "6961b0f295110d32d16799cb3799bedff7600b9b956bc4ad893eebc249140212"
)
G62_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g62/manifest.json")
G62_MANIFEST_SHA256 = (
    "b15fef00aa66a342734781d5b6e1b9c183de2fbda8fb8bf5ccf0ee2f5753d847"
)
G60B_AUTHORITY_PATH = pathlib.Path("tests/ssts/authority/g60b/manifest.json")
G60B_AUTHORITY_SHA256 = (
    "f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac"
)
DATASET_ID = m61.DATASET_ID
CONTRACTS = m61.CONTRACTS
SELECTED_HASH_SETS = m61.SELECTED_HASH_SETS
SUPPORT_MAP_PATH = pathlib.Path("tools/qa/golden/upd9002_support_map_m48.csv")
DATASET_MANIFEST_PATH = m61.DATASET_MANIFEST_PATH
TARGET_POLICY_PATH = pathlib.Path("tests/ssts/target_policy/g64.json")
TASK_PATH = pathlib.Path("docs/agents/tasks/M64_upd9002_div_idiv.md")
REPORT_PATH = pathlib.Path("docs/agents/reports/m64_upd9002_div_idiv.md")
EVIDENCE_ROOT = pathlib.Path("tests/ssts/evidence/g64")
RESULT_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g64_result_manifest.json")
RANKING_JSON_PATH = pathlib.Path("tests/ssts/rankings/g64_architectural_full.json")
RANKING_MD_PATH = pathlib.Path("tests/ssts/rankings/g64_architectural_full.md")
SCOREBOARD_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/scoreboard/g64_architectural_ci.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/scoreboard/g64_architectural_full.json"
    ),
    "fingerprint_full": pathlib.Path(
        "tests/ssts/scoreboard/g64_fingerprint_full.json"
    ),
}
FAILURE_DIRECTORY_PATHS = {
    key: pathlib.Path(str(path).removesuffix(".json") + "_failures")
    for key, path in SCOREBOARD_PATHS.items()
}
TRANSITION_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/transitions/g64_architectural_ci_from_g62.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/transitions/g64_architectural_full_from_g62.json"
    ),
}
G62_SCOREBOARD_PATHS = {
    key: pathlib.Path(str(path).replace("g64_", "g62_"))
    for key, path in SCOREBOARD_PATHS.items()
}
APPLICABLE_BEFORE_HASH_SETS = {
    "ci": "440a621dea647cf11a4e8b834fc139c2c95f6081f294d717263ba8f42eb2a750",
    "full": "4e8cf0af125f3d8404912311fc18fc3c75952c4c27215256ae7dd983d095cdff",
}
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")

PHASE_FORMS = {
    "div_idiv": ("F6.6", "F6.7", "F7.6", "F7.7"),
    "add4s_sub4s_cmp4s": ("0F20", "0F22", "0F26"),
    "bit_operations": (
        "0F10", "0F11", "0F18", "0F19",
        "0F12", "0F13", "0F1A", "0F1B",
        "0F14", "0F15", "0F1C", "0F1D",
        "0F16", "0F17", "0F1E", "0F1F",
    ),
    "brkem": ("0FFF",),
}
PHASE_ORDER = tuple(PHASE_FORMS)
ACTIVATIONS = {
    "0F13": "v30_clr1_ea16_cl",
    "0F15": "v30_set1_ea16_cl",
    "0F16": "v30_not1_ea8_cl",
    "0F17": "v30_not1_ea16_cl",
    "0F1E": "v30_not1_ea8_i3",
    "0F1F": "v30_not1_ea16_i4",
    "0F26": "v30_cmp4s",
}
ACTIVATION_COUNTS_FULL = {
    "0F13": 5000,
    "0F15": 5000,
    "0F16": 5000,
    "0F17": 5000,
    "0F1E": 5000,
    "0F1F": 5000,
    "0F26": 1000,
}
EXPECTED_DIV_FAILURES = {
    "F6.6": 2561,
    "F6.7": 3716,
    "F7.6": 2486,
    "F7.7": 3723,
}
ROM_RECORDS = {
    "ADD4S": (("ff", "20", "00"), ("ff", "20", "01")),
    "SUB4S": (("ff", "22", "00"), ("ff", "22", "01")),
    "CMP4S": (("ff", "26", "00"), ("ff", "26", "01")),
    "TEST1": (("f6", "10", "03"),),
    "NOT1": (("f6", "16", "03"),),
    "CLR1": (("f6", "12", "03"),),
    "SET1": (("f6", "14", "03"),),
    "BRKEM": (("ff", "ff", "04"),),
}
BRKEM_COVERAGE = {
    "compatibility_scope": "no_v20_sst_cases",
    "selected": 0,
    "executed": 0,
    "sst_contract_status": "not_yet_present",
    "silicon_mode_identity": "underdetermined",
}
EMPTY_HASH_SET_SHA256 = ratchet.hash_set_digest([])
_POLICY_ENUMERATION_CACHE: dict[
    tuple[pathlib.Path, pathlib.Path], dict[str, dict[str, Any]]
] = {}
PROTECTED_FORMS = (
    "D4",
    "D5",
    "27",
    "2F",
    "37",
    "3F",
    "0F28",
    "0F2A",
    "62",
    "CF",
    "C6",
    "C7",
    "F7.2",
    "FF.7",
)


class M64Error(RuntimeError):
    """Raised when M64 evidence fails closed."""


def reject(code: str, detail: str) -> None:
    raise M64Error(f"{code}: {detail}")


def require(condition: bool, code: str, detail: str) -> None:
    if not condition:
        reject(code, detail)


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, ensure_ascii=True, sort_keys=True, separators=(",", ":")
    ).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_json(path: pathlib.Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value) + b"\n")


def verify_predecessor(root: pathlib.Path) -> None:
    require(
        sha256_file(root / G62_MANIFEST_PATH) == G62_MANIFEST_SHA256,
        "protected-g62-mutation",
        str(G62_MANIFEST_PATH),
    )
    require(
        sha256_file(root / G60B_AUTHORITY_PATH) == G60B_AUTHORITY_SHA256,
        "rom-authority-mutation",
        str(G60B_AUTHORITY_PATH),
    )
    policy = read_json(root / m62.TARGET_POLICY_PATH)
    require(
        policy.get("target_policy_id") == G62_POLICY_ID
        and policy.get("target_policy_sha256") == G62_POLICY_SHA256,
        "wrong-g62-policy",
        repr(policy.get("target_policy_id")),
    )


def expand_mask(mask: int, value: int) -> list[int]:
    require((value & mask) == value, "malformed-rom-record", f"{mask:02x}:{value:02x}")
    return [opcode for opcode in range(256) if (opcode & mask) == value]


def verify_rom_contract() -> None:
    require(
        expand_mask(0xF6, 0x10) == [0x10, 0x11, 0x18, 0x19],
        "f6-mask-expansion",
        "TEST1",
    )
    require(
        expand_mask(0xF6, 0x12) == [0x12, 0x13, 0x1A, 0x1B],
        "f6-mask-expansion",
        "CLR1",
    )
    require(
        expand_mask(0xF6, 0x14) == [0x14, 0x15, 0x1C, 0x1D],
        "f6-mask-expansion",
        "SET1",
    )
    require(
        expand_mask(0xF6, 0x16) == [0x16, 0x17, 0x1E, 0x1F],
        "f6-mask-expansion",
        "NOT1",
    )
    require(
        all(len(record) == 3 for records in ROM_RECORDS.values() for record in records),
        "raw-record-width",
        "ROM dispatch records are not three bytes wide",
    )
    require(
        ("ff", "fe", "04") not in {
            record for records in ROM_RECORDS.values() for record in records
        },
        "brkfem-included",
        "0FFE is outside M64",
    )


def g64_support_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    result = m62.g62_support_rows(rows)
    changed: set[str] = set()
    for row in result:
        if row["mode"] != "v30op_0f" or row["opcode"] != "0x0f":
            continue
        form = f"0F{int(row['subopcode'], 16):02X}"
        if form not in ACTIVATIONS:
            continue
        require(
            row["classification"] == "known_target_gap"
            and row["target"] == "v30_reserved_0x0f",
            "live-policy-conflict",
            repr(row),
        )
        row["classification"] = "implemented"
        row["target"] = ACTIVATIONS[form]
        row["basis"] = "g60b-rom-authority+m64-sst-contract"
        changed.add(form)
    require(changed == set(ACTIVATIONS), "partial-activation", repr(sorted(changed)))
    return result


@contextlib.contextmanager
def support_map(root: pathlib.Path, epoch: str) -> Iterator[pathlib.Path]:
    require(epoch in {"g62", "g64"}, "unknown-policy-epoch", epoch)
    fields, rows = m60b.read_support_rows(root / SUPPORT_MAP_PATH)
    candidate = m62.g62_support_rows(rows)
    if epoch == "g64":
        candidate = g64_support_rows(rows)
    with tempfile.TemporaryDirectory(prefix=f"vaeg-m64-{epoch}-support-") as name:
        path = pathlib.Path(name) / f"upd9002_support_map_{epoch}.csv"
        with path.open("w", encoding="utf-8", newline="") as stream:
            writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
            writer.writeheader()
            writer.writerows(candidate)
        ssts.load_support_map(path)
        yield path


def policy_enumerations(
    root: pathlib.Path, dataset_root: pathlib.Path
) -> dict[str, dict[str, Any]]:
    cache_key = (root.resolve(), dataset_root.resolve())
    cached = _POLICY_ENUMERATION_CACHE.get(cache_key)
    if cached is not None:
        return cached
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    require(manifest["dataset_id"] == DATASET_ID, "dataset-drift", "manifest")
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    predecessor = read_json(root / m62.TARGET_POLICY_PATH)
    values: dict[str, dict[str, Any]] = {}
    with support_map(root, "g62") as before, support_map(root, "g64") as after:
        before_support = ssts.load_support_map(before)
        after_support = ssts.load_support_map(after)
        work = {
            scope: {
                "after_sets": {
                    classification: []
                    for classification in ratchet.TOP_LEVEL_CLASSIFICATIONS
                },
                "after_form_counts": {},
                "changes": [],
                "selected_count": 0,
            }
            for scope in ("ci", "full")
        }
        for path in ssts.corpus_files(dataset_root):
            form = path.name.removesuffix(".json.gz").upper()
            with gzip.open(path, "rt", encoding="utf-8") as stream:
                records = ssts.profile_records(json.load(stream), "full")
            counts = {"ci": Counter(), "full": Counter()}
            for record_index, raw_record in enumerate(records):
                scopes = ("ci", "full") if record_index < 500 else ("full",)
                record = ssts.validate_record(
                    raw_record, f"{path.name}:{raw_record.get('idx')}"
                )
                before_classification = ssts.classify_record(
                    form, record, metadata, before_support
                )["classification"]
                after_classification = ssts.classify_record(
                    form, record, metadata, after_support
                )["classification"]
                record_hash = sha256_bytes(canonical_bytes(record))
                for scope in scopes:
                    work[scope]["selected_count"] += 1
                    counts[scope][after_classification] += 1
                    work[scope]["after_sets"][after_classification].append(
                        record_hash
                    )
                    if before_classification != after_classification:
                        work[scope]["changes"].append(
                            {
                                "after": after_classification,
                                "before": before_classification,
                                "form": form,
                                "record_hash": record_hash,
                                "upstream_test_hash": record["hash"],
                            }
                        )
            for scope in ("ci", "full"):
                work[scope]["after_form_counts"][form] = counts[scope]
        for scope in ("ci", "full"):
            after_sets = work[scope]["after_sets"]
            changes = work[scope]["changes"]
            after_form_counts = work[scope]["after_form_counts"]
            selected_count = work[scope]["selected_count"]
            changes.sort(key=lambda item: item["record_hash"])
            require(
                all(
                    item["form"] in ACTIVATIONS
                    and item["before"] == "known_target_gap"
                    and item["after"] == "applicable"
                    for item in changes
                ),
                "unauthorized-classification-change",
                scope,
            )
            predecessor_sets = predecessor["applicable_hash_sets"][scope]
            value = {
                "after_form_counts": after_form_counts,
                "after_set_digests": {
                    classification: ratchet.hash_set_digest(hashes)
                    for classification, hashes in after_sets.items()
                },
                "after_sets": after_sets,
                "before_set_digests": {
                    "applicable": predecessor_sets["after_sha256"]
                },
                "before_sets": {
                    "applicable": [None] * predecessor_sets["after_count"]
                },
                "classification_changes": changes,
                "selected_count": selected_count,
                "selected_hash_set_sha256": SELECTED_HASH_SETS[scope],
            }
            require(
                value["selected_hash_set_sha256"] == SELECTED_HASH_SETS[scope],
                "selected-set-drift",
                scope,
            )
            values[scope] = value
    full = Counter(item["form"] for item in values["full"]["classification_changes"])
    require(dict(full) == ACTIVATION_COUNTS_FULL, "partial-activation", repr(full))
    _POLICY_ENUMERATION_CACHE[cache_key] = values
    return values


def gap_selector_metadata(root: pathlib.Path) -> dict[str, dict[str, Any]]:
    known = read_json(root / "tests/ssts/baseline/upd9002_v20_known_gaps.json")
    taxonomy = read_json(root / "tests/ssts/gap_taxonomy.json")
    annotations = {entry["selector_sha256"]: entry for entry in taxonomy["annotations"]}
    result: dict[str, dict[str, Any]] = {}
    for rule in known["rules"]:
        form = rule["selector"]["metadata_form"]
        if form not in ACTIVATIONS:
            continue
        selector_sha = sha256_bytes(canonical_bytes(rule["selector"]))
        annotation = annotations.get(selector_sha)
        require(annotation is not None, "missing-gap-taxonomy", form)
        require(
            annotation["gap_kind"] == "implementation_missing",
            "unauthorized-gap-kind",
            form,
        )
        result[form] = {
            "before_gap_kind": "implementation_missing",
            "after_gap_kind": None,
            "form": form,
            "resolved_count": rule["resolved_count"],
            "resolved_test_hashes_sha256": annotation[
                "resolved_test_hashes_sha256"
            ],
            "selector_sha256": selector_sha,
        }
    require(set(result) == set(ACTIVATIONS), "missing-gap-selector", repr(result))
    return result


def generate_target_policy(
    root: pathlib.Path, dataset_root: pathlib.Path, evaluated_sha: str
) -> dict[str, Any]:
    require(HEX40.fullmatch(evaluated_sha) is not None, "evaluated-sha", evaluated_sha)
    enumerations = policy_enumerations(root, dataset_root)
    predecessor = read_json(root / m62.TARGET_POLICY_PATH)
    gaps = gap_selector_metadata(root)
    body = {
        "applicable_hash_sets": {
            scope: {
                "before_count": len(enumerations[scope]["before_sets"]["applicable"]),
                "before_sha256": enumerations[scope]["before_set_digests"]["applicable"],
                "after_count": len(enumerations[scope]["after_sets"]["applicable"]),
                "after_sha256": enumerations[scope]["after_set_digests"]["applicable"],
            }
            for scope in ("ci", "full")
        },
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "authority_manifest_sha256": G60B_AUTHORITY_SHA256,
        "brkem_coverage": BRKEM_COVERAGE,
        "candidate_gate": CANDIDATE_GATE,
        "classification_changes": {
            scope: {
                "count": len(enumerations[scope]["classification_changes"]),
                "forms": dict(
                    sorted(
                        Counter(
                            item["form"]
                            for item in enumerations[scope]["classification_changes"]
                        ).items()
                    )
                ),
                "record_hash_set_sha256": ratchet.hash_set_digest(
                    item["record_hash"]
                    for item in enumerations[scope]["classification_changes"]
                ),
                "upstream_hash_set_sha256": ratchet.upstream_hash_set_digest(
                    item["upstream_test_hash"]
                    for item in enumerations[scope]["classification_changes"]
                ),
            }
            for scope in ("ci", "full")
        },
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": [gaps[form] for form in sorted(gaps)],
        "milestone": MILESTONE,
        "predecessor_policy": {
            "target_policy_id": predecessor["target_policy_id"],
            "target_policy_sha256": predecessor["target_policy_sha256"],
        },
        "schema": "vaeg-upd9002-target-policy-v3",
        "schema_version": 3,
        "selected_hash_sets": {
            scope: enumerations[scope]["selected_hash_set_sha256"]
            for scope in ("ci", "full")
        },
        "taxonomy_counts": {
            "before": predecessor["taxonomy_counts"]["after"],
            "after": {
                "documented_silicon_absent": 32000,
                "implementation_missing": 5908,
                "target_support_unverified": 0,
            },
        },
        "transition_kind": "monitor_authority_activation",
    }
    digest = sha256_bytes(canonical_bytes(body))
    value = {
        **body,
        "target_policy_id": f"upd9002-g64-{digest}",
        "target_policy_sha256": digest,
    }
    validate_target_policy(value)
    return value


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
        and value.get("target_policy_id") == f"upd9002-g64-{digest}",
        "target-policy-digest",
        "content identity differs",
    )
    require(
        value.get("approved_predecessor_sha") == APPROVED_PREDECESSOR_SHA,
        "wrong-predecessor",
        "G62 SHA differs",
    )
    require(value.get("dataset_id") == DATASET_ID, "dataset-drift", "policy")
    require(value.get("comparison_contracts") == CONTRACTS, "contract-drift", "policy")
    require(
        value.get("selected_hash_sets") == SELECTED_HASH_SETS,
        "selected-set-drift",
        "policy",
    )
    require(
        value.get("authority_manifest_sha256") == G60B_AUTHORITY_SHA256,
        "rom-authority-missing",
        "policy",
    )
    changes = value.get("classification_changes", {}).get("full", {})
    require(
        changes.get("count") == 31000
        and changes.get("forms") == ACTIVATION_COUNTS_FULL,
        "partial-activation",
        repr(changes),
    )
    require(
        value.get("brkem_coverage") == BRKEM_COVERAGE,
        "fabricated-brkem-coverage",
        repr(value.get("brkem_coverage")),
    )
    require(
        {item.get("form") for item in value.get("gap_kind_changes", [])}
        == set(ACTIVATIONS),
        "unauthorized-gap-kind-change",
        repr(value.get("gap_kind_changes")),
    )


def load_selected_records(dataset_root: pathlib.Path, form: str) -> list[dict[str, Any]]:
    path = dataset_root / ssts.SUITE_PATH / f"{form}.json.gz"
    if form == "0FFF":
        require(not path.exists(), "unexpected-brkem-sst-shard", str(path))
        return []
    require(path.is_file(), "missing-sst-shard", form)
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        return ssts.profile_records(json.load(stream), "full")


def case_row(
    form: str,
    record: dict[str, Any],
    resolved: dict[str, Any],
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
    initial = record["initial"]["regs"]
    return {
        "actual_final_state": {
            "ram": m59.ram_entries(actual["ram"]),
            "registers": {
                key: f"{value:04x}" for key, value in sorted(actual["registers"].items())
            },
            "termination": actual["termination"],
        },
        "architectural_mismatch_kinds": architectural["mismatch_kinds"],
        "architectural_outcome": (
            "pass" if not architectural["mismatch_kinds"] else "fail"
        ),
        "case_hash": context["record_digest"],
        "complete_instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "conclusion_status": "proven",
        "evidence_notes": "Expected and actual worker states are recorded side by side.",
        "expected_final_state": {
            "ram": m59.ram_entries(expected_ram),
            "registers": {
                key: f"{value:04x}" for key, value in sorted(expected_regs.items())
            },
            "termination": ssts.expected_termination(form, record),
        },
        "fingerprint_mismatch_kinds": fingerprint["mismatch_kinds"],
        "fingerprint_outcome": (
            "pass" if not fingerprint["mismatch_kinds"] else "fail"
        ),
        "form": form,
        "initial_state": {
            "ram": m59.ram_entries({address: value for address, value in record["initial"]["ram"]}),
            "registers": {
                key: f"{value:04x}" for key, value in sorted(initial.items())
            },
        },
        "selected": True,
        "structural_partition": {
            "modrm_mod": (
                None if len(record["bytes"]) < 2 else (record["bytes"][-1] >> 6)
            ),
            "prefix_sequence": [
                f"{byte:02x}" for byte in record["bytes"][:-1]
            ],
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
    require(phase in PHASE_FORMS, "unknown-phase", phase)
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    require(manifest["dataset_id"] == DATASET_ID, "dataset-drift", phase)
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    rows_by_form: dict[str, list[dict[str, Any]]] = {}
    summaries: dict[str, dict[str, Any]] = {}
    with support_map(root, epoch) as support_path:
        support = ssts.load_support_map(support_path)
        for form in PHASE_FORMS[phase]:
            selected = load_selected_records(dataset_root, form)
            if form == "0FFF":
                rows_by_form[form] = []
                summaries[form] = {
                    **BRKEM_COVERAGE,
                    "record_hash_set_sha256": EMPTY_HASH_SET_SHA256,
                }
                continue
            resolved = [
                ssts.classify_record(form, record, metadata, support)
                for record in selected
            ]
            results = ssts.run_worker_contained(worker, selected, 120.0)
            require(
                len(results) == len(selected)
                and all(status == "ok" for status, _ in results),
                "missing-executed-record",
                form,
            )
            rows = [
                case_row(form, record, classification, actual)
                for record, classification, (_, actual) in zip(
                    selected, resolved, results
                )
            ]
            rows.sort(key=lambda item: item["case_hash"])
            require(
                len(rows) == len({item["case_hash"] for item in rows}),
                "duplicate-hash",
                form,
            )
            passing = [
                item["case_hash"]
                for item in rows
                if item["architectural_outcome"] == "pass"
            ]
            failing = [
                item["case_hash"]
                for item in rows
                if item["architectural_outcome"] == "fail"
            ]
            classifications = Counter(item["top_level_classification"] for item in rows)
            summaries[form] = {
                "architectural_failure_count": len(failing),
                "architectural_failure_hash_set_sha256": ratchet.hash_set_digest(failing),
                "architectural_pass_count": len(passing),
                "architectural_pass_hash_set_sha256": ratchet.hash_set_digest(passing),
                "classification_counts": dict(sorted(classifications.items())),
                "executed_by_worker": len(rows),
                "record_hash_set_sha256": ratchet.hash_set_digest(
                    item["case_hash"] for item in rows
                ),
                "selected": len(rows),
            }
            rows_by_form[form] = rows
    return {
        "epoch": epoch,
        "forms": list(PHASE_FORMS[phase]),
        "phase": phase,
        "rows": rows_by_form,
        "schema": "vaeg-upd9002-m64-phase-audit-v1",
        "schema_version": 1,
        "summaries": summaries,
        "worker_sha256": sha256_file(worker),
    }


def write_phase_audit(output: pathlib.Path, value: dict[str, Any], source_sha: str) -> None:
    require(HEX40.fullmatch(source_sha) is not None, "source-sha", source_sha)
    output.mkdir(parents=True, exist_ok=True)
    for form in value["forms"]:
        ratchet.write_deterministic_gzip(
            output / f"{form.lower().replace('.', '_')}_cases.json.gz",
            {
                "form": form,
                "row_count": len(value["rows"][form]),
                "rows": value["rows"][form],
                "schema": "vaeg-upd9002-m64-cases-v1",
                "schema_version": 1,
            },
        )
    write_json(
        output / "phase_summary.json",
        {
            key: item for key, item in value.items() if key != "rows"
        }
        | {"source_sha": source_sha},
    )


@contextlib.contextmanager
def configured_scoreboard_generator() -> Iterator[None]:
    """Bind the shared scoreboard writer to the G64 epoch."""
    replacements = {
        "APPROVED_PREDECESSOR_GATE": APPROVED_PREDECESSOR_GATE,
        "APPROVED_PREDECESSOR_SHA": APPROVED_PREDECESSOR_SHA,
        "CANDIDATE_GATE": CANDIDATE_GATE,
        "CONTRACTS": CONTRACTS,
        "DATASET_ID": DATASET_ID,
        "FAILURE_DIRECTORY_PATHS": FAILURE_DIRECTORY_PATHS,
        "G61_SCOREBOARD_PATHS": G62_SCOREBOARD_PATHS,
        "SCOREBOARD_PATHS": SCOREBOARD_PATHS,
        "SELECTED_HASH_SETS": SELECTED_HASH_SETS,
        "policy_enumerations": policy_enumerations,
    }
    previous = {name: getattr(m62, name) for name in replacements}
    for name, item in replacements.items():
        setattr(m62, name, item)
    try:
        yield
    finally:
        for name, item in previous.items():
            setattr(m62, name, item)


def generate_scoreboard(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    raw_path: pathlib.Path,
    profile_key: str,
    evaluated_sha: str,
    policy: dict[str, Any],
) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    with configured_scoreboard_generator():
        return m62.generate_scoreboard(
            root,
            output_root,
            dataset_root,
            raw_path,
            profile_key,
            evaluated_sha,
            policy,
        )


def load_scoreboard_failures(
    root: pathlib.Path, relative: pathlib.Path
) -> dict[str, dict[str, Any]]:
    summary = read_json(root / relative)
    return ratchet.load_scoreboard_failures(root / relative, summary)


def grouped_classification_changes(
    changes: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    grouped: dict[str, list[str]] = {}
    for change in changes:
        require(
            change["before"] == "known_target_gap"
            and change["after"] == "applicable"
            and change["form"] in ACTIVATIONS,
            "unauthorized-classification-change",
            repr(change),
        )
        grouped.setdefault(change["form"], []).append(change["record_hash"])
    return [
        {
            "after": "applicable",
            "before": "known_target_gap",
            "count": len(grouped[form]),
            "form": form,
            "record_hash_set_sha256": ratchet.hash_set_digest(grouped[form]),
        }
        for form in sorted(grouped)
    ]


def write_transition(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    profile_key: str,
    scoreboard: dict[str, Any],
    failures_after: dict[str, dict[str, Any]],
    evaluated_sha: str,
    policy: dict[str, Any],
) -> dict[str, Any]:
    profile, scope = profile_key.rsplit("_", 1)
    before_summary = read_json(root / G62_SCOREBOARD_PATHS[profile_key])
    failures_before = load_scoreboard_failures(
        root, G62_SCOREBOARD_PATHS[profile_key]
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
        *PHASE_FORMS["div_idiv"],
        *PHASE_FORMS["add4s_sub4s_cmp4s"],
        *PHASE_FORMS["bit_operations"],
    }
    require(not newly_failing, "newly-failing", profile_key)
    require(not changed, "changed-failure-not-enumerated", profile_key)
    require(
        all(
            failures_before[record_hash]["form"] in governed_forms
            for record_hash in newly_passing
        ),
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
        if before_row["classification"] != "applicable":
            continue
        after_row = after_rows.get(key)
        if after_row is None or after_row["pass"] < before_row["pass"]:
            decreases.append(key[0])
    require(not decreases, "per-form-pass-decrease", repr(decreases))
    enumerations = policy_enumerations(root, dataset_root)
    changes = enumerations[scope]["classification_changes"]
    newly_applicable = sorted(change["record_hash"] for change in changes)
    require(
        len(newly_applicable)
        == policy["classification_changes"][scope]["count"],
        "newly-applicable-count",
        profile_key,
    )
    div_forms = set(PHASE_FORMS["div_idiv"])
    requested_0f_forms = {
        *PHASE_FORMS["add4s_sub4s_cmp4s"],
        *PHASE_FORMS["bit_operations"],
    }
    div_before = sorted(
        record_hash
        for record_hash, failure in failures_before.items()
        if failure["form"] in div_forms
    )
    requested_0f_before = sorted(
        record_hash
        for record_hash, failure in failures_before.items()
        if failure["form"] in requested_0f_forms
    )
    transition = {
        "applicable_hash_set_after_sha256": scoreboard[
            "applicable_hash_set_sha256"
        ],
        "applicable_hash_set_before_sha256": before_summary[
            "applicable_hash_set_sha256"
        ],
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "changed_failure_count": 0,
        "changed_failure_shards": [],
        "comparison_contract_ids": {profile: CONTRACTS[profile]},
        "dataset_id": DATASET_ID,
        "div_idiv_failure_count_before": len(div_before),
        "div_idiv_failure_hash_set_sha256": ratchet.hash_set_digest(div_before),
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": policy["gap_kind_changes"],
        "hardware_pending_changes": [],
        "newly_applicable": newly_applicable,
        "newly_applicable_count": len(newly_applicable),
        "newly_applicable_hash_set_sha256": ratchet.hash_set_digest(
            newly_applicable
        ),
        "newly_failing": [],
        "newly_failing_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "newly_passing": newly_passing,
        "newly_passing_hash_set_sha256": ratchet.hash_set_digest(newly_passing),
        "phases": list(PHASE_ORDER),
        "profile": profile,
        "requested_0f_failure_count_before": len(requested_0f_before),
        "requested_0f_failure_hash_set_sha256": ratchet.hash_set_digest(
            requested_0f_before
        ),
        "schema": "vaeg-upd9002-m64-transition-v1",
        "schema_version": 1,
        "scope": scope,
        "selected_hash_set_sha256": SELECTED_HASH_SETS[scope],
        "target_policy_after_id": policy["target_policy_id"],
        "target_policy_before_id": G62_POLICY_ID,
        "top_level_classification_changes": grouped_classification_changes(
            changes
        ),
        "transition_kind": "div_idiv_and_monitor_0f_support",
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
    before = read_json(root / G62_SCOREBOARD_PATHS["architectural_full"])
    before_rows = {
        (row["form"], row["classification"]): row for row in before["records"]
    }
    rows = []
    for record in scoreboard["records"]:
        if record["classification"] != "applicable":
            continue
        form = record["form"]
        before_fail = before_rows.get((form, "applicable"), {"fail": 0})["fail"]
        rows.append(
            {
                "change_from_g62": record["fail"] - before_fail,
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
        *PHASE_FORMS["div_idiv"],
        *PHASE_FORMS["add4s_sub4s_cmp4s"],
        *PHASE_FORMS["bit_operations"],
        "0F28",
        "0F2A",
    }
    indexed = {row["form"]: row for row in rows}
    require(
        all(form in indexed and indexed[form]["fail"] == 0 for form in green_forms),
        "phase-not-green",
        "ranking contains an M64 failure",
    )
    ranking = {
        "architectural_full_failure_count": total,
        "brkem_coverage": BRKEM_COVERAGE,
        "family_aggregation": {
            "m64_div_idiv": {
                "fail": sum(indexed[form]["fail"] for form in PHASE_FORMS["div_idiv"]),
                "forms": list(PHASE_FORMS["div_idiv"]),
            },
            "m64_monitor_0f": {
                "fail": sum(
                    indexed[form]["fail"]
                    for form in green_forms
                    if form.startswith("0F")
                ),
                "forms": sorted(form for form in green_forms if form.startswith("0F")),
            },
        },
        "m64_green_forms": sorted(green_forms),
        "row_count": len(rows),
        "rows": rows,
        "schema": "vaeg-upd9002-m64-failure-ranking-v1",
        "schema_version": 1,
    }
    write_json(output_root / RANKING_JSON_PATH, ranking)
    lines = [
        "<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD. -->",
        "",
        "# G64 architectural-full failure ranking",
        "",
        f"Total remaining failures: **{total:,}**.",
        "",
        "| Rank | Form | Pass | Fail | Change from G62 | Cumulative |",
        "| ---: | :--- | ---: | ---: | ---: | ---: |",
    ]
    for index, row in enumerate(rows[:30], 1):
        lines.append(
            f"| {index} | `{row['form']}` | {row['pass']:,} | "
            f"{row['fail']:,} | {row['change_from_g62']:+,} | "
            f"{row['cumulative_share_ppm'] / 10000:.2f}% |"
        )
    lines.extend(
        [
            "",
            "All requested DIV/IDIV and SST-covered monitor `0F` forms have "
            "explicit zero-failure rows in the machine-readable ranking. "
            "`0FFF BRKEM` has metadata but no v20 SST shard, so it has zero "
            "selected and executed cases and is recorded separately rather "
            "than inferred to pass. `0F28 ROL4` and `0F2A ROR4` remain green. "
            "Omission from this top-30 view is not proof of passing.",
            "",
        ]
    )
    path = output_root / RANKING_MD_PATH
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return ranking


def load_phase_commits(path: pathlib.Path) -> dict[str, list[str]]:
    value = read_json(path)
    require(
        isinstance(value, dict)
        and set(value) == set(PHASE_ORDER)
        and all(isinstance(items, list) for items in value.values())
        and all(
            isinstance(item, str) and HEX40.fullmatch(item) is not None
            for items in value.values()
            for item in items
        )
        and all(value[phase] for phase in PHASE_ORDER[:-1])
        and value["brkem"] == [],
        "phase-commit-identity",
        str(path),
    )
    return {phase: value[phase] for phase in PHASE_ORDER}


def copy_phase_audits(
    output_root: pathlib.Path, audit_root: pathlib.Path
) -> list[pathlib.Path]:
    paths = []
    for phase in PHASE_ORDER:
        source = audit_root / phase
        require((source / "phase_summary.json").is_file(), "missing-phase", phase)
        target = output_root / EVIDENCE_ROOT / phase
        target.mkdir(parents=True, exist_ok=True)
        for item in sorted(source.iterdir()):
            if item.is_file():
                shutil.copyfile(item, target / item.name)
                paths.append(EVIDENCE_ROOT / phase / item.name)
    return paths


def write_phase_checkpoints(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    scoreboards: dict[str, dict[str, Any]],
    failures_after: dict[str, dict[str, dict[str, Any]]],
    phase_commits: dict[str, list[str]],
    worker_sha256: str,
) -> list[pathlib.Path]:
    before_scoreboard = read_json(
        root / G62_SCOREBOARD_PATHS["architectural_full"]
    )
    before_failures = load_scoreboard_failures(
        root, G62_SCOREBOARD_PATHS["architectural_full"]
    )
    after_failures = failures_after["architectural_full"]
    before_rows = {row["form"]: row for row in before_scoreboard["records"]}
    after_rows = {
        row["form"]: row
        for row in scoreboards["architectural_full"]["records"]
        if row["classification"] == "applicable"
    }
    changes = policy_enumerations(root, dataset_root)["full"][
        "classification_changes"
    ]
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
        newly_applicable = sorted(
            item["record_hash"] for item in changes if item["form"] in forms
        )
        rows = []
        for form in sorted(forms):
            before = before_rows.get(
                form,
                {
                    "classification": "not_in_dataset",
                    "executed": 0,
                    "fail": 0,
                    "pass": 0,
                    "selected": 0,
                },
            )
            after = after_rows.get(
                form,
                {
                    "classification": "not_in_dataset",
                    "executed": 0,
                    "fail": 0,
                    "pass": 0,
                    "selected": 0,
                },
            )
            rows.append(
                {
                    "classification_after": after["classification"],
                    "classification_before": before["classification"],
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
        commits = phase_commits[phase]
        checkpoint = {
            "brkem_coverage": BRKEM_COVERAGE if phase == "brkem" else None,
            "changed_failure_count": 0,
            "focused_tests": "passed",
            "forms": sorted(forms),
            "newly_applicable_count": len(newly_applicable),
            "newly_applicable_hash_set_sha256": ratchet.hash_set_digest(
                newly_applicable
            ),
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
            "schema": "vaeg-upd9002-m64-phase-checkpoint-v1",
            "schema_version": 1,
            "semantic_commit": commits[-1] if commits else None,
            "semantic_commits": commits,
            "worker_sha256": worker_sha256,
        }
        path = EVIDENCE_ROOT / "phases" / f"phase_{phase}.json"
        write_json(output_root / path, checkpoint)
        paths.append(path)
        if commits:
            parent = commits[-1]
    return paths


def artifact_entry(root: pathlib.Path, relative: pathlib.Path) -> dict[str, Any]:
    return m62.artifact_entry(root, relative)


def tree_identities(root: pathlib.Path) -> list[dict[str, Any]]:
    return m62.tree_identities(root)


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
        phase_commits["bit_operations"][-1] == evaluated_sha,
        "evaluated-sha",
        "last worker-changing phase is not evaluated SHA",
    )
    policy = generate_target_policy(root, dataset_root, evaluated_sha)
    write_json(output_root / TARGET_POLICY_PATH, policy)
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
            policy,
        )
    transitions = {
        key: write_transition(
            root,
            output_root,
            dataset_root,
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
    worker_sha256 = sha256_file(worker)
    checkpoint_paths = write_phase_checkpoints(
        root,
        output_root,
        dataset_root,
        scoreboards,
        failures,
        phase_commits,
        worker_sha256,
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
    before_failures = load_scoreboard_failures(
        root, G62_SCOREBOARD_PATHS["architectural_full"]
    )
    div_forms = set(PHASE_FORMS["div_idiv"])
    requested_0f_forms = {
        *PHASE_FORMS["add4s_sub4s_cmp4s"],
        *PHASE_FORMS["bit_operations"],
    }
    d_hashes = sorted(
        record_hash
        for record_hash, failure in before_failures.items()
        if failure["form"] in div_forms
    )
    o_hashes = sorted(
        record_hash
        for record_hash, failure in before_failures.items()
        if failure["form"] in requested_0f_forms
    )
    full_changes = policy_enumerations(root, dataset_root)["full"][
        "classification_changes"
    ]
    l_hashes = sorted(item["record_hash"] for item in full_changes)
    full_transition = transitions["architectural_full"]
    require(len(d_hashes) == 12486, "div-idiv-failure-count", str(len(d_hashes)))
    require(len(o_hashes) == 395, "requested-0f-failure-count", str(len(o_hashes)))
    require(len(l_hashes) == 31000, "newly-applicable-count", str(len(l_hashes)))
    require(
        scoreboards["architectural_full"]["fail"] == 7511
        and scoreboards["architectural_full"]["pass"] == 1467083,
        "full-arithmetic",
        repr(scoreboards["architectural_full"]),
    )
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
        "brkem_coverage": BRKEM_COVERAGE,
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "div_idiv_failure_count_before": len(d_hashes),
        "div_idiv_failure_hash_set_sha256": ratchet.hash_set_digest(d_hashes),
        "evaluated_sha": evaluated_sha,
        "milestone": MILESTONE,
        "newly_applicable_count": len(l_hashes),
        "newly_applicable_hash_set_sha256": ratchet.hash_set_digest(l_hashes),
        "newly_failing_count": 0,
        "newly_failing_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "newly_passing_count": len(full_transition["newly_passing"]),
        "newly_passing_hash_set_sha256": full_transition[
            "newly_passing_hash_set_sha256"
        ],
        "phase_semantic_commits": phase_commits,
        "ranking_sha256": sha256_file(output_root / RANKING_JSON_PATH),
        "requested_0f_failure_count_before": len(o_hashes),
        "requested_0f_failure_hash_set_sha256": ratchet.hash_set_digest(o_hashes),
        "rom_authority_manifest_sha256": G60B_AUTHORITY_SHA256,
        "schema": "vaeg-upd9002-m64-evidence-manifest-v1",
        "schema_version": 1,
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "target_policy_after_id": policy["target_policy_id"],
        "target_policy_before_id": G62_POLICY_ID,
        "worker_sha256": worker_sha256,
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
        "schema": "vaeg-upd9002-m64-result-manifest-v1",
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
    with tempfile.TemporaryDirectory(prefix="vaeg-m64-a-") as first_name:
        with tempfile.TemporaryDirectory(prefix="vaeg-m64-b-") as second_name:
            first = pathlib.Path(first_name)
            second = pathlib.Path(second_name)
            result = generate_evidence(output_root=first, **kwargs)
            generate_evidence(output_root=second, **kwargs)
            require(
                tree_identities(first) == tree_identities(second),
                "nondeterministic-generation",
                "complete G64 evidence generations differ",
            )
            for source in sorted(first.rglob("*")):
                if source.is_file():
                    target = output_root / source.relative_to(first)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(source, target)
            return result


def verify_evidence(root: pathlib.Path, dataset_root: pathlib.Path) -> None:
    manifest_path = root / EVIDENCE_ROOT / "manifest.json"
    result_path = root / RESULT_MANIFEST_PATH
    require(manifest_path.is_file(), "missing-evidence", str(manifest_path))
    require(result_path.is_file(), "missing-evidence", str(result_path))
    manifest = read_json(manifest_path)
    result = read_json(result_path)
    policy = read_json(root / TARGET_POLICY_PATH)
    validate_target_policy(policy)
    require(
        manifest.get("candidate_gate") == CANDIDATE_GATE
        and manifest.get("milestone") == MILESTONE
        and manifest.get("brkem_coverage") == BRKEM_COVERAGE,
        "evidence-identity",
        "manifest",
    )
    require(
        result.get("candidate_gate") == CANDIDATE_GATE
        and result.get("evaluated_sha") == manifest.get("evaluated_sha")
        and result.get("target_policy_sha256")
        == policy.get("target_policy_sha256"),
        "evidence-identity",
        "result manifest",
    )
    require(
        canonical_bytes(
            generate_target_policy(root, dataset_root, manifest["evaluated_sha"])
        )
        == canonical_bytes(policy),
        "target-policy-drift",
        "committed policy is not reproducible",
    )
    for artifact in manifest["artifacts"]:
        path = root / artifact["path"]
        require(path.is_file(), "missing-artifact", artifact["path"])
        require(
            path.stat().st_size == artifact["bytes"]
            and sha256_file(path) == artifact["sha256"],
            "artifact-digest",
            artifact["path"],
        )
    require(
        result["evidence_manifest_sha256"] == sha256_file(manifest_path),
        "manifest-digest",
        str(manifest_path),
    )
    print(
        "m64-evidence: exact G64 policy, profiles, transitions, "
        "BRKEM zero coverage, and artifact identities verified"
    )


def verify_static(root: pathlib.Path) -> None:
    verify_predecessor(root)
    verify_rom_contract()
    task = (root / TASK_PATH).read_text(encoding="utf-8")
    require(
        "topic/m64-upd9002-div-idiv" in task
        and "compatibility_scope = no_v20_sst_cases" in task
        and "contains no `0FFF.json.gz` shard" in task,
        "scope-expansion-documentation",
        str(TASK_PATH),
    )
    manifest = read_json(root / DATASET_MANIFEST_PATH)
    paths = {entry["path"] for entry in manifest["files"]}
    require(
        "v1_native/0FFF.json.gz" not in paths,
        "unexpected-brkem-sst-shard",
        "dataset manifest",
    )
    require(
        not (root / "tests/ssts/v1_native/0FFF.json.gz").exists(),
        "unexpected-brkem-sst-shard",
        "repository",
    )
    print(
        "m64-static: G62/G60b identities protected; requested ROM forms are "
        "bounded; BRKEM has exact zero SST-v20 coverage"
    )


def selftest() -> None:
    verify_rom_contract()
    good = {
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "authority_manifest_sha256": G60B_AUTHORITY_SHA256,
        "brkem_coverage": BRKEM_COVERAGE,
        "classification_changes": {
            "full": {"count": 31000, "forms": ACTIVATION_COUNTS_FULL}
        },
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "gap_kind_changes": [{"form": form} for form in sorted(ACTIVATIONS)],
        "selected_hash_sets": SELECTED_HASH_SETS,
    }

    def validate_body(value: dict[str, Any]) -> None:
        require(
            value["approved_predecessor_sha"] == APPROVED_PREDECESSOR_SHA,
            "wrong-predecessor",
            "selftest",
        )
        require(value["dataset_id"] == DATASET_ID, "dataset-drift", "selftest")
        require(value["comparison_contracts"] == CONTRACTS, "contract-drift", "selftest")
        require(value["selected_hash_sets"] == SELECTED_HASH_SETS, "selected-drift", "selftest")
        require(
            value["authority_manifest_sha256"] == G60B_AUTHORITY_SHA256,
            "authority-drift",
            "selftest",
        )
        require(value["brkem_coverage"] == BRKEM_COVERAGE, "fabricated-brkem", "selftest")
        require(
            value["classification_changes"]["full"]
            == {"count": 31000, "forms": ACTIVATION_COUNTS_FULL},
            "partial-activation",
            "selftest",
        )
        require(
            {item["form"] for item in value["gap_kind_changes"]} == set(ACTIVATIONS),
            "unauthorized-gap-kind",
            "selftest",
        )

    mutations = {
        "wrong-predecessor": lambda value: value.update(
            approved_predecessor_sha="0" * 40
        ),
        "dataset-drift": lambda value: value.update(dataset_id="wrong"),
        "contract-drift": lambda value: value.update(comparison_contracts={}),
        "selected-drift": lambda value: value.update(selected_hash_sets={}),
        "authority-drift": lambda value: value.update(authority_manifest_sha256="0" * 64),
        "fabricated-brkem": lambda value: value["brkem_coverage"].update(selected=1),
        "partial-activation": lambda value: value["classification_changes"]["full"].update(count=30999),
        "brkfem-included": lambda value: value["gap_kind_changes"].append({"form": "0FFE"}),
    }
    validate_body(copy.deepcopy(good))
    rejected = 0
    for code, mutation in mutations.items():
        value = copy.deepcopy(good)
        mutation(value)
        try:
            validate_body(value)
        except M64Error:
            rejected += 1
        else:
            reject("selftest-did-not-reject", code)
    sample = {"rows": [{"case_hash": "1" * 64}], "schema_version": 1}
    require(canonical_bytes(sample) == canonical_bytes(copy.deepcopy(sample)), "json", "nondeterministic")
    require(
        ratchet.deterministic_gzip_bytes(sample)
        == ratchet.deterministic_gzip_bytes(copy.deepcopy(sample)),
        "gzip",
        "nondeterministic",
    )
    print(f"m64-selftest: {rejected} fail-closed mutations rejected")


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest")
    static = subparsers.add_parser("verify-static")
    static.add_argument("--root", type=pathlib.Path, default=pathlib.Path.cwd())
    support = subparsers.add_parser("write-support-map")
    support.add_argument("--root", type=pathlib.Path, required=True)
    support.add_argument("--epoch", choices=("g62", "g64"), required=True)
    support.add_argument("--output", type=pathlib.Path, required=True)
    policy = subparsers.add_parser("target-policy")
    policy.add_argument("--root", type=pathlib.Path, required=True)
    policy.add_argument("--dataset-root", type=pathlib.Path, required=True)
    policy.add_argument("--evaluated-sha", required=True)
    policy.add_argument("--output", type=pathlib.Path, required=True)
    audit = subparsers.add_parser("audit")
    audit.add_argument("--root", type=pathlib.Path, required=True)
    audit.add_argument("--dataset-root", type=pathlib.Path, required=True)
    audit.add_argument("--worker", type=pathlib.Path, required=True)
    audit.add_argument("--phase", choices=PHASE_ORDER, required=True)
    audit.add_argument("--epoch", choices=("g62", "g64"), required=True)
    audit.add_argument("--source-sha", required=True)
    audit.add_argument("--output", type=pathlib.Path, required=True)
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
    evidence = subparsers.add_parser("verify-evidence")
    evidence.add_argument("--root", type=pathlib.Path, required=True)
    evidence.add_argument("--dataset-root", type=pathlib.Path, required=True)
    return parser.parse_args(list(argv))


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
        elif arguments.command == "verify-static":
            verify_static(arguments.root.resolve())
        elif arguments.command == "write-support-map":
            with support_map(arguments.root.resolve(), arguments.epoch) as source:
                arguments.output.parent.mkdir(parents=True, exist_ok=True)
                arguments.output.write_bytes(source.read_bytes())
            print(f"m64-support-map: epoch={arguments.epoch}")
        elif arguments.command == "target-policy":
            value = generate_target_policy(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.evaluated_sha,
            )
            write_json(arguments.output.resolve(), value)
            print(
                "m64-target-policy: "
                f"id={value['target_policy_id']} "
                f"full_applicable={value['applicable_hash_sets']['full']['after_count']}"
            )
        elif arguments.command == "audit":
            value = run_phase_audit(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.phase,
                arguments.epoch,
            )
            write_phase_audit(arguments.output.resolve(), value, arguments.source_sha)
            print(
                f"m64-audit: phase={arguments.phase} epoch={arguments.epoch} "
                f"forms={len(value['forms'])}"
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
                f"m64-generate: policy={result['target_policy_id']} "
                f"tree={result['artifact_tree_sha256']}"
            )
        elif arguments.command == "verify-evidence":
            verify_evidence(
                arguments.root.resolve(), arguments.dataset_root.resolve()
            )
    except (
        M64Error,
        m62.M62Error,
        m59.EvidenceError,
        ssts.CorpusError,
        OSError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"m64-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

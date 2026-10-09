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
"""Audit, generate, and verify M61 C6/C7 register-form evidence."""

from __future__ import annotations

import argparse
import copy
import gzip
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
import upd9002_m60e_iret as m60e
import upd9002_semantics_evidence as m59
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M61"
CANDIDATE_GATE = "G61"
APPROVED_PREDECESSOR_GATE = "G60e"
# Evidence keeps the original identity; Git topology starts at the current
# main-history checkpoint established after the protected-history rewrite.
APPROVED_PREDECESSOR_SHA = "a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b"
APPROVED_PREDECESSOR_GIT_SHA = "a187d14427b7532e16487653f8ffb6fe37c9703d"
G60E_EVALUATED_SHA = "7f815acb26f1be546bbcfd5de12972235dfd175c"
G60E_CI_URL = "https://github.com/nakatamaho/vaeg/actions/runs/30184747721"
DATASET_ID = m60e.DATASET_ID
CONTRACTS = m60e.CONTRACTS
TARGET_POLICY_ID = m60e.TARGET_POLICY_ID
TARGET_POLICY_SHA256 = m60e.TARGET_POLICY_SHA256
SELECTED_HASH_SETS = m60e.SELECTED_HASH_SETS
APPLICABLE_HASH_SETS = m60e.APPLICABLE_HASH_SETS
EMPTY_HASH_SET_SHA256 = ratchet.hash_set_digest([])
DATASET_MANIFEST_PATH = pathlib.Path("tests/ssts/v20_dataset_manifest.json")
G60E_EVIDENCE_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g60e/manifest.json")
G60E_EVIDENCE_MANIFEST_SHA256 = (
    "27909e9305d2bc49e491f5a4f81285433840a6a0d5397f82e741a6e4b10c44ae"
)
G60E_RESULT_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g60e_result_manifest.json")
G60E_SCOREBOARD_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/scoreboard/g60e_architectural_ci.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/scoreboard/g60e_architectural_full.json"
    ),
    "fingerprint_full": pathlib.Path(
        "tests/ssts/scoreboard/g60e_fingerprint_full.json"
    ),
}
SCOREBOARD_PATHS = {
    key: pathlib.Path(str(path).replace("g60e_", "g61_"))
    for key, path in G60E_SCOREBOARD_PATHS.items()
}
FAILURE_DIRECTORY_PATHS = {
    key: pathlib.Path(str(path).removesuffix(".json") + "_failures")
    for key, path in SCOREBOARD_PATHS.items()
}
TRANSITION_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/transitions/g61_architectural_ci_from_g60e.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/transitions/g61_architectural_full_from_g60e.json"
    ),
}
EVIDENCE_ROOT = pathlib.Path("tests/ssts/evidence/g61")
RESULT_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g61_result_manifest.json")
RANKING_JSON_PATH = pathlib.Path("tests/ssts/rankings/g61_architectural_full.json")
RANKING_MD_PATH = pathlib.Path("tests/ssts/rankings/g61_architectural_full.md")
REPORT_PATH = pathlib.Path("docs/agents/reports/m61_upd9002_mov_imm_register.md")
EXPECTED = {
    "C6": {
        "selected": 5000,
        "pass_before": 3912,
        "fail_before": 1088,
        "failure_sha256": (
            "2def4cc309f2a11b5950d4708ae1093e661e0d57e636c7f6600262d7efe8abe3"
        ),
        "register": 1249,
        "memory": 3751,
        "same_field_pass": 161,
    },
    "C7": {
        "selected": 5000,
        "pass_before": 3880,
        "fail_before": 1120,
        "failure_sha256": (
            "640e24a7c324690e73c72db449f3d6a750dca66b690fd35f021317c82816394a"
        ),
        "register": 1274,
        "memory": 3726,
        "same_field_pass": 154,
    },
}
BEFORE_PROFILE_IDENTITIES = {
    "architectural_ci": {
        "selected": 180000,
        "executed": 165300,
        "pass": 157561,
        "fail": 7739,
        "pass_sha256": (
            "dc6bdee9f856ca6102748ca442ac579adf8a7f05e01e02564766148a35825cdc"
        ),
        "failure_sha256": (
            "2ae38099d67c240ff5bf48a1c7643d1b6d6480e4e27d1b8967d87508d751ebd6"
        ),
        "signature_sha256": (
            "af14392ce957dfeaf770da595551fef8767bc7412eec06c10badbe9d7c8930b4"
        ),
    },
    "architectural_full": {
        "selected": 1562502,
        "executed": 1438594,
        "pass": 1382422,
        "fail": 56172,
        "pass_sha256": (
            "11958b52c4fa71e1ac38c22d7e305562ab00f408c453fa423955bcc3eb6882c4"
        ),
        "failure_sha256": (
            "2c2bae091f33ebcd334767d9a9597eab5707d45a4d66b5433b8b37b10ce367f7"
        ),
        "signature_sha256": (
            "4fc2d3603ec05633f4a4b63f574d92bb5b26140519f03e5e50c848d5066dd84b"
        ),
    },
    "fingerprint_full": {
        "selected": 1562502,
        "executed": 1438594,
        "pass": 1279984,
        "fail": 158610,
        "pass_sha256": (
            "17a6bc59e91efc7439621037842072c3ae0d0bf2f600307ae3ef407e1dafc542"
        ),
        "failure_sha256": (
            "795fdeb7c0469783f4863aeebf45c730118c7cccfede5b7804d5a55f7e1ae2cb"
        ),
        "signature_sha256": (
            "0c184c75164afe40cb5afddaa0aab635c24b131cf8925f5df9163c89d6e3d377"
        ),
    },
}
HEX40 = re.compile(r"^[0-9a-f]{40}$")
BYTE_REGISTERS = ("al", "cl", "dl", "bl", "ah", "ch", "dh", "bh")
WORD_REGISTERS = ("ax", "cx", "dx", "bx", "sp", "bp", "si", "di")


class M61Error(RuntimeError):
    """A fail-closed M61 validation failure with a stable reason code."""

    def __init__(self, code: str, message: str):
        super().__init__(f"{code}: {message}")
        self.code = code


def reject(code: str, message: str) -> None:
    raise M61Error(code, message)


def require(condition: bool, code: str, message: str) -> None:
    if not condition:
        reject(code, message)


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    ).encode("utf-8")


def sha256_file(path: pathlib.Path) -> str:
    return m60e.sha256_file(path)


def write_json(path: pathlib.Path, value: Any) -> None:
    m60e.write_json(path, value)


def read_json(path: pathlib.Path) -> Any:
    return m60e.read_json(path)


def verify_predecessor(root: pathlib.Path) -> None:
    require(
        sha256_file(root / G60E_EVIDENCE_MANIFEST_PATH)
        == G60E_EVIDENCE_MANIFEST_SHA256,
        "protected-artifact-mutation",
        "G60e evidence manifest differs",
    )
    result = read_json(root / G60E_RESULT_MANIFEST_PATH)
    require(
        result.get("candidate_gate") == "G60e"
        and result.get("evaluated_sha") == G60E_EVALUATED_SHA
        and result.get("evidence_manifest_sha256")
        == G60E_EVIDENCE_MANIFEST_SHA256,
        "wrong-predecessor-sha",
        "G60e result identity differs",
    )
    for key, path in G60E_SCOREBOARD_PATHS.items():
        value = read_json(root / path)
        expected = BEFORE_PROFILE_IDENTITIES[key]
        require(
            value["selected"] == expected["selected"]
            and value["executed"] == expected["executed"]
            and value["pass"] == expected["pass"]
            and value["fail"] == expected["fail"]
            and value["pass_hash_set_sha256"] == expected["pass_sha256"]
            and value["failure_hash_set_sha256"] == expected["failure_sha256"]
            and value["failure_signature_index_sha256"]
            == expected["signature_sha256"],
            "predecessor-profile-drift",
            key,
        )
    try:
        m60e.verify_static(root, protected_evidence_only=True)
    except m60e.M60eError as error:
        reject("protected-artifact-mutation", str(error))


def byte_register(registers: dict[str, int], code: int) -> int:
    word = registers[("ax", "cx", "dx", "bx")[code & 3]]
    return (word >> 8) & 0xFF if code >= 4 else word & 0xFF


def destination_value(form: str, registers: dict[str, int], code: int) -> int:
    if form == "C6":
        return byte_register(registers, code)
    return registers[WORD_REGISTERS[code]]


def make_case_row(
    form: str,
    record: dict[str, Any],
    resolved: dict[str, Any],
    actual: dict[str, Any],
) -> dict[str, Any]:
    row = m59.make_row("canary", form, record, resolved, {}, actual)
    layout = m59.instruction_layout(record, form)
    require(layout["modrm"] is not None, "missing-modrm", record["hash"])
    modrm = layout["modrm"]
    reg = (modrm >> 3) & 7
    rm = modrm & 7
    width = 8 if form == "C6" else 16
    register_form = layout["mod"] == 3
    initial = record["initial"]["regs"]
    expected = ssts.expected_registers(record)
    row["architectural_outcome"] = (
        "pass" if not row["architectural_mismatch_kinds"] else "fail"
    )
    row["actual_ip"] = f"{actual['registers']['ip']:04x}"
    row["expected_ip"] = f"{expected['ip']:04x}"
    row["initial_ip"] = f"{initial['ip']:04x}"
    row["modrm"] = {
        "byte": f"{modrm:02x}",
        "mod": layout["mod"],
        "reg_extension": reg,
        "rm": rm,
    }
    row["prefix_sequence"] = [
        f"{value:02x}" for value in record["bytes"][: layout["prefix_count"]]
    ]
    row["structural_form"] = "register" if register_form else "memory"
    row["immediate_value"] = f"{layout['immediate']:0{width // 4}x}"
    row["destination_width"] = width
    if register_form:
        names = BYTE_REGISTERS if form == "C6" else WORD_REGISTERS
        mask = (1 << width) - 1
        initial_destination = destination_value(form, initial, rm)
        expected_destination = destination_value(form, expected, rm)
        actual_destination = destination_value(form, actual["registers"], rm)
        row["destination_register"] = names[rm]
        row["wrong_extension_register"] = names[reg]
        row["initial_destination_value"] = (
            f"{initial_destination:0{width // 4}x}"
        )
        row["expected_destination_value"] = (
            f"{expected_destination:0{width // 4}x}"
        )
        row["actual_destination_value"] = (
            f"{actual_destination:0{width // 4}x}"
        )
        row["value_coincidence"] = initial_destination == layout["immediate"]
        row["reg_and_rm_same"] = reg == rm
        row["wrong_extension_register_changed"] = (
            destination_value(form, actual["registers"], reg)
            != destination_value(form, initial, reg)
            and reg != rm
        )
        require(
            expected_destination == (layout["immediate"] & mask),
            "expected-destination",
            row["case_hash"],
        )
    else:
        row["destination_register"] = None
        row["wrong_extension_register"] = None
        row["initial_destination_value"] = None
        row["expected_destination_value"] = None
        row["actual_destination_value"] = None
        row["value_coincidence"] = False
        row["reg_and_rm_same"] = False
        row["wrong_extension_register_changed"] = False
    return row


def run_audit(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    phase: str,
) -> dict[str, Any]:
    require(phase in {"pre-fix", "post-fix"}, "invalid-audit-phase", phase)
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    require(manifest["dataset_id"] == DATASET_ID, "dataset-drift", DATASET_ID)
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    all_rows: dict[str, list[dict[str, Any]]] = {}
    summaries: dict[str, dict[str, Any]] = {}
    with m60b.candidate_support_map(root) as support_path:
        support = ssts.load_support_map(support_path)
        for form in ("C6", "C7"):
            with gzip.open(
                dataset_root / ssts.SUITE_PATH / f"{form}.json.gz",
                "rt",
                encoding="utf-8",
            ) as stream:
                selected = ssts.profile_records(json.load(stream), "full")
            require(
                len(selected) == EXPECTED[form]["selected"],
                f"incomplete-{form.lower()}-population",
                str(len(selected)),
            )
            resolved = [
                ssts.classify_record(form, record, metadata, support)
                for record in selected
            ]
            require(
                all(value["classification"] == "applicable" for value in resolved),
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
            rows = [
                make_case_row(form, record, classification, actual)
                for record, classification, (_, actual) in zip(
                    selected, resolved, results
                )
            ]
            rows.sort(key=lambda value: value["case_hash"])
            require(
                len({value["case_hash"] for value in rows}) == len(rows),
                "duplicate-case-hash",
                form,
            )
            failures = [
                value["case_hash"]
                for value in rows
                if value["architectural_outcome"] == "fail"
            ]
            passes = [
                value["case_hash"]
                for value in rows
                if value["architectural_outcome"] == "pass"
            ]
            register_rows = [
                value for value in rows if value["structural_form"] == "register"
            ]
            memory_rows = [
                value for value in rows if value["structural_form"] == "memory"
            ]
            require(
                len(register_rows) == EXPECTED[form]["register"]
                and len(memory_rows) == EXPECTED[form]["memory"],
                "structural-partition",
                form,
            )
            require(
                all(value["architectural_outcome"] == "pass" for value in memory_rows),
                "memory-form-output-change",
                form,
            )
            same_field_passes = sum(
                value["reg_and_rm_same"]
                and value["architectural_outcome"] == "pass"
                for value in register_rows
            )
            require(
                same_field_passes == EXPECTED[form]["same_field_pass"],
                "register-pass-cause",
                form,
            )
            if phase == "pre-fix":
                require(
                    len(passes) == EXPECTED[form]["pass_before"]
                    and len(failures) == EXPECTED[form]["fail_before"]
                    and ratchet.hash_set_digest(failures)
                    == EXPECTED[form]["failure_sha256"],
                    "pre-fix-result",
                    form,
                )
                require(
                    all(
                        value["reg_and_rm_same"]
                        for value in register_rows
                        if value["architectural_outcome"] == "pass"
                    ),
                    "value-coincidence-misdiagnosis",
                    form,
                )
            else:
                require(
                    len(passes) == EXPECTED[form]["selected"] and not failures,
                    "post-fix-result",
                    form,
                )
            summaries[form] = {
                "architectural_failure_count": len(failures),
                "architectural_failure_hash_set_sha256": ratchet.hash_set_digest(
                    failures
                ),
                "architectural_pass_count": len(passes),
                "architectural_pass_hash_set_sha256": ratchet.hash_set_digest(passes),
                "executed": len(rows),
                "fingerprint_failure_count": sum(
                    value["fingerprint_outcome"] == "fail" for value in rows
                ),
                "memory_form_count": len(memory_rows),
                "memory_form_result_sha256": ratchet.hash_set_digest(
                    value["case_hash"] for value in memory_rows
                ),
                "register_form_count": len(register_rows),
                "register_same_field_pass_count": same_field_passes,
                "selected": len(rows),
                "value_coincidence_count": sum(
                    value["value_coincidence"] for value in register_rows
                ),
            }
            all_rows[form] = rows
    return {
        "rows": all_rows,
        "summaries": summaries,
        "worker_sha256": sha256_file(worker),
    }


def write_audit_directory(
    output: pathlib.Path, audit: dict[str, Any], phase: str, source_sha: str
) -> None:
    require(HEX40.fullmatch(source_sha) is not None, "source-sha", source_sha)
    output.mkdir(parents=True, exist_ok=True)
    for form in ("C6", "C7"):
        value = {
            "row_count": len(audit["rows"][form]),
            "rows": audit["rows"][form],
            "schema": "vaeg-upd9002-m61-mov-imm-cases-v1",
            "schema_version": 1,
        }
        ratchet.write_deterministic_gzip(
            output / f"{form.lower()}_cases.json.gz", value
        )
        write_json(
            output / f"{form.lower()}_summary.json",
            {
                **audit["summaries"][form],
                "form": form,
                "phase": phase,
                "schema": "vaeg-upd9002-m61-mov-imm-summary-v1",
                "schema_version": 1,
                "source_sha": source_sha,
                "top_level_classification": "applicable",
                "worker_sha256": audit["worker_sha256"],
            },
        )


def load_audit(path: pathlib.Path, phase: str) -> dict[str, Any]:
    rows: dict[str, list[dict[str, Any]]] = {}
    summaries = {}
    for form in ("C6", "C7"):
        summary = read_json(path / f"{form.lower()}_summary.json")
        require(summary["phase"] == phase, "audit-identity", form)
        with gzip.open(
            path / f"{form.lower()}_cases.json.gz", "rt", encoding="utf-8"
        ) as stream:
            table = json.load(stream)
        values = table["rows"]
        require(
            table["row_count"] == len(values) == EXPECTED[form]["selected"]
            and values == sorted(values, key=lambda value: value["case_hash"]),
            "audit-row-count",
            form,
        )
        rows[form] = values
        summaries[form] = summary
    return {"rows": rows, "summaries": summaries}


def candidate_scoreboard(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    raw_path: pathlib.Path,
    profile_key: str,
    evaluated_sha: str,
) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    names = (
        "MILESTONE",
        "CANDIDATE_GATE",
        "APPROVED_PREDECESSOR_GATE",
        "APPROVED_PREDECESSOR_SHA",
        "BEFORE_PROFILE_IDENTITIES",
        "G60D_SCOREBOARD_PATHS",
        "SCOREBOARD_PATHS",
        "FAILURE_DIRECTORY_PATHS",
    )
    previous = {name: getattr(m60e, name) for name in names}
    try:
        m60e.MILESTONE = MILESTONE
        m60e.CANDIDATE_GATE = CANDIDATE_GATE
        m60e.APPROVED_PREDECESSOR_GATE = APPROVED_PREDECESSOR_GATE
        m60e.APPROVED_PREDECESSOR_SHA = APPROVED_PREDECESSOR_SHA
        m60e.BEFORE_PROFILE_IDENTITIES = BEFORE_PROFILE_IDENTITIES
        m60e.G60D_SCOREBOARD_PATHS = G60E_SCOREBOARD_PATHS
        m60e.SCOREBOARD_PATHS = SCOREBOARD_PATHS
        m60e.FAILURE_DIRECTORY_PATHS = FAILURE_DIRECTORY_PATHS
        return m60e.generate_scoreboard(
            root,
            output_root,
            dataset_root,
            raw_path,
            profile_key,
            evaluated_sha,
        )
    except m60e.M60eError as error:
        reject(error.code, str(error))
    finally:
        for name, value in previous.items():
            setattr(m60e, name, value)


def write_transition(
    root: pathlib.Path,
    output_root: pathlib.Path,
    profile_key: str,
    scoreboard: dict[str, Any],
    failures_after: dict[str, dict[str, Any]],
    evaluated_sha: str,
) -> dict[str, Any]:
    failures_before = m60e.load_scoreboard_failures(
        root, G60E_SCOREBOARD_PATHS[profile_key]
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
    governed = sorted(
        record_hash
        for record_hash, value in failures_before.items()
        if value["form"] in {"C6", "C7"}
    )
    require(not newly_failing, "newly-failing", profile_key)
    require(not changed, "changed-failure-not-enumerated", profile_key)
    require(newly_passing == governed, "ungoverned-newly-passing", profile_key)
    transition = {
        "applicable_hash_set_sha256": APPLICABLE_HASH_SETS,
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "c6_failure_count_after": sum(
            value["form"] == "C6" for value in failures_after.values()
        ),
        "c6_failure_count_before": sum(
            value["form"] == "C6" for value in failures_before.values()
        ),
        "c7_failure_count_after": sum(
            value["form"] == "C7" for value in failures_after.values()
        ),
        "c7_failure_count_before": sum(
            value["form"] == "C7" for value in failures_before.values()
        ),
        "changed_failure_count": 0,
        "comparison_contract_ids": CONTRACTS,
        "dataset_id": DATASET_ID,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": [],
        "hardware_pending_changes": [],
        "newly_failing": [],
        "newly_failing_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "newly_passing": newly_passing,
        "newly_passing_hash_set_sha256": ratchet.hash_set_digest(newly_passing),
        "schema": "vaeg-upd9002-m61-transition-v1",
        "schema_version": 1,
        "scope": profile_key,
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "target_policy_id": TARGET_POLICY_ID,
        "top_level_classification_changes": [],
        "transition_kind": "mov_immediate_register_semantics",
    }
    write_json(output_root / TRANSITION_PATHS[profile_key], transition)
    return transition


def write_ranking(
    root: pathlib.Path,
    output_root: pathlib.Path,
    scoreboard: dict[str, Any],
    failures: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    by_form: dict[str, list[str]] = defaultdict(list)
    mismatch: dict[str, Counter[str]] = defaultdict(Counter)
    termination: dict[str, Counter[str]] = defaultdict(Counter)
    for record_hash, failure in failures.items():
        by_form[failure["form"]].append(record_hash)
        mismatch[failure["form"]].update(failure["mismatch_classes"])
        termination[failure["form"]][failure["actual_termination"]] += 1
    before = read_json(root / G60E_SCOREBOARD_PATHS["architectural_full"])
    before_rows = {
        (row["form"], row["classification"]): row for row in before["records"]
    }
    rows = []
    for record in scoreboard["records"]:
        if record["classification"] != "applicable":
            continue
        form = record["form"]
        rows.append(
            {
                "change_from_g60e": (
                    record["fail"] - before_rows[(form, "applicable")]["fail"]
                ),
                "classification": "applicable",
                "executed": record["executed"],
                "fail": record["fail"],
                "failure_hash_set_sha256": ratchet.hash_set_digest(by_form[form]),
                "form": form,
                "mismatch_classes": dict(sorted(mismatch[form].items())),
                "opcode": record["opcode"],
                "pass": record["pass"],
                "selected": record["selected"],
                "subform": record["subform"],
                "termination_classes": dict(sorted(termination[form].items())),
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
    ranking = {
        "architectural_full_failure_count": total,
        "c6_post_m61": next(row for row in rows if row["form"] == "C6"),
        "c7_post_m61": next(row for row in rows if row["form"] == "C7"),
        "row_count": len(rows),
        "rows": rows,
        "schema": "vaeg-upd9002-m61-failure-ranking-v1",
        "schema_version": 1,
    }
    write_json(output_root / RANKING_JSON_PATH, ranking)
    lines = [
        "<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD. -->",
        "",
        "# G61 architectural-full failure ranking",
        "",
        f"Total remaining failures: **{total:,}**.",
        "",
        "| Rank | Form | Pass | Fail | Change from G60e | Cumulative |",
        "| ---: | :--- | ---: | ---: | ---: | ---: |",
    ]
    for index, row in enumerate(rows[:30], 1):
        lines.append(
            f"| {index} | `{row['form']}` | {row['pass']:,} | "
            f"{row['fail']:,} | {row['change_from_g60e']:+,} | "
            f"{row['cumulative_share_ppm'] / 10000:.2f}% |"
        )
    lines.extend(
        [
            "",
            "C6 and C7 are present in the complete machine-readable ranking as "
            "5,000 pass / 0 fail. Omission from this top-30 view is not proof "
            "that any other form passes.",
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
                "row_count", value.get("failure_count", len(value.get("artifacts", [0])))
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
    evidence_paths = []
    for form in ("C6", "C7"):
        before = {row["case_hash"]: row for row in pre["rows"][form]}
        after = {row["case_hash"]: row for row in post["rows"][form]}
        require(set(before) == set(after), "incomplete-population", form)
        merged = []
        for record_hash in sorted(after):
            row = copy.deepcopy(after[record_hash])
            row["pre_fix_actual_destination_value"] = before[record_hash][
                "actual_destination_value"
            ]
            row["pre_fix_mismatch_kinds"] = before[record_hash][
                "architectural_mismatch_kinds"
            ]
            row["post_fix_mismatch_kinds"] = row["architectural_mismatch_kinds"]
            merged.append(row)
        cases_path = EVIDENCE_ROOT / f"{form.lower()}_cases.json.gz"
        ratchet.write_deterministic_gzip(
            output_root / cases_path,
            {
                "row_count": len(merged),
                "rows": merged,
                "schema": "vaeg-upd9002-m61-mov-imm-cases-v1",
                "schema_version": 1,
            },
        )
        evidence_paths.append(cases_path)
        summary_path = EVIDENCE_ROOT / f"{form.lower()}_summary.json"
        write_json(
            output_root / summary_path,
            {
                "candidate_gate": CANDIDATE_GATE,
                "failure_after": post["summaries"][form][
                    "architectural_failure_count"
                ],
                "failure_before": pre["summaries"][form][
                    "architectural_failure_count"
                ],
                "form": form,
                "memory_form_count": post["summaries"][form]["memory_form_count"],
                "pass_after": post["summaries"][form]["architectural_pass_count"],
                "pass_before": pre["summaries"][form]["architectural_pass_count"],
                "register_form_count": post["summaries"][form][
                    "register_form_count"
                ],
                "register_same_field_pass_count": pre["summaries"][form][
                    "register_same_field_pass_count"
                ],
                "schema": "vaeg-upd9002-m61-mov-imm-summary-v1",
                "schema_version": 1,
                "selected": EXPECTED[form]["selected"],
                "value_coincidence_count": pre["summaries"][form][
                    "value_coincidence_count"
                ],
            },
        )
        evidence_paths.append(summary_path)
    mapping_path = EVIDENCE_ROOT / "register_mapping.json"
    write_json(
        output_root / mapping_path,
        {
            "byte_registers_by_rm": list(BYTE_REGISTERS),
            "conclusion": (
                "The pre-fix implementation selected ModR/M reg bits 5:3; "
                "the executed SST population requires r/m bits 2:0."
            ),
            "conclusion_status": "proven",
            "schema": "vaeg-upd9002-m61-register-mapping-v1",
            "schema_version": 1,
            "word_registers_by_rm": list(WORD_REGISTERS),
        },
    )
    evidence_paths.append(mapping_path)
    memory_path = EVIDENCE_ROOT / "memory_form_protection.json"
    write_json(
        output_root / memory_path,
        {
            "forms": {
                form: {
                    "count": post["summaries"][form]["memory_form_count"],
                    "hash_set_sha256": post["summaries"][form][
                        "memory_form_result_sha256"
                    ],
                    "pre_fix_all_pass": True,
                    "post_fix_all_pass": True,
                }
                for form in ("C6", "C7")
            },
            "schema": "vaeg-upd9002-m61-memory-protection-v1",
            "schema_version": 1,
        },
    )
    evidence_paths.append(memory_path)
    rep_path = EVIDENCE_ROOT / "representative" / "mov_imm_register.md"
    lines = [
        "<!-- Copyright (c) 2026 Nakata Maho; 2-clause BSD. -->",
        "",
        "# G61 C6/C7 representatives",
        "",
    ]
    for form in ("C6", "C7"):
        rows = pre["rows"][form]
        for label, predicate in (
            ("wrong-register failure", lambda row: row["architectural_outcome"] == "fail"),
            ("same-field pre-fix pass", lambda row: row["reg_and_rm_same"]),
            ("memory-form protection", lambda row: row["structural_form"] == "memory"),
        ):
            row = next(value for value in rows if predicate(value))
            lines.extend(
                [
                    f"## {form} {label}",
                    "",
                    f"- Case hash: `{row['case_hash']}`",
                    f"- Bytes: `{row['instruction_bytes']}`",
                    f"- ModR/M: `{row['modrm']['byte']}`",
                    f"- Pre-fix mismatches: "
                    f"`{','.join(row['architectural_mismatch_kinds']) or 'none'}`",
                    "",
                ]
            )
    path = output_root / rep_path
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    evidence_paths.append(rep_path)
    ranking = write_ranking(
        root,
        output_root,
        scoreboards["architectural_full"],
        failures["architectural_full"],
    )
    artifact_paths = [
        *evidence_paths,
        *SCOREBOARD_PATHS.values(),
        *TRANSITION_PATHS.values(),
        RANKING_JSON_PATH,
        RANKING_MD_PATH,
    ]
    for directory in FAILURE_DIRECTORY_PATHS.values():
        artifact_paths.extend(
            path.relative_to(output_root)
            for path in (output_root / directory).glob("*.json.gz")
        )
    artifact_paths = sorted(set(artifact_paths), key=lambda value: value.as_posix())
    artifacts = [artifact_entry(output_root, path) for path in artifact_paths]
    artifact_tree = m60e.sha256_bytes(canonical_bytes(artifacts))
    manifest = {
        "applicable_hash_set_sha256": APPLICABLE_HASH_SETS,
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifact_tree_sha256": artifact_tree,
        "artifacts": artifacts,
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "environment": {
            "gzip_module": gzip.__name__,
            "platform": platform.platform(),
            "python": platform.python_version(),
            "zlib": zlib.ZLIB_VERSION,
        },
        "evaluated_sha": evaluated_sha,
        "generator": {
            "path": "tools/qa/upd9002_m61_mov_imm.py",
            "sha256": sha256_file(root / "tools/qa/upd9002_m61_mov_imm.py"),
            "version": 1,
        },
        "milestone": MILESTONE,
        "newly_failing_count": 0,
        "newly_failing_hash_set_sha256": EMPTY_HASH_SET_SHA256,
        "newly_passing_count": len(
            transitions["architectural_full"]["newly_passing"]
        ),
        "newly_passing_hash_set_sha256": transitions["architectural_full"][
            "newly_passing_hash_set_sha256"
        ],
        "pre_fix": pre["summaries"],
        "post_fix": post["summaries"],
        "ranking_sha256": sha256_file(output_root / RANKING_JSON_PATH),
        "schema": "vaeg-upd9002-m61-evidence-manifest-v1",
        "schema_version": 1,
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "target_policy_id": TARGET_POLICY_ID,
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
                )
            }
            for key in scoreboards
        },
        "ranking_failure_total": ranking["architectural_full_failure_count"],
        "schema": "vaeg-upd9002-m61-result-manifest-v1",
        "schema_version": 1,
        "transition_sha256": {
            key: sha256_file(output_root / path)
            for key, path in TRANSITION_PATHS.items()
        },
    }
    write_json(output_root / RESULT_MANIFEST_PATH, result)
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
    require(HEX40.fullmatch(evaluated_sha) is not None, "evaluated-sha", evaluated_sha)
    pre = load_audit(pre_fix_audit, "pre-fix")
    post = load_audit(post_fix_audit, "post-fix")
    scoreboards = {}
    failures = {}
    for key in ("architectural_ci", "architectural_full", "fingerprint_full"):
        scoreboards[key], failures[key] = candidate_scoreboard(
            root, output_root, dataset_root, raw_paths[key], key, evaluated_sha
        )
    transitions = {
        key: write_transition(
            root, output_root, key, scoreboards[key], failures[key], evaluated_sha
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


def regenerate_twice(**kwargs: Any) -> dict[str, Any]:
    output_root = kwargs.pop("output_root")
    with tempfile.TemporaryDirectory(prefix="vaeg-m61-a-") as first_name:
        with tempfile.TemporaryDirectory(prefix="vaeg-m61-b-") as second_name:
            first = pathlib.Path(first_name)
            second = pathlib.Path(second_name)
            first_result = generate(output_root=first, **kwargs)
            generate(output_root=second, **kwargs)
            require(
                tree_identities(first) == tree_identities(second),
                "nondeterministic-generation",
                "complete evidence generations differ",
            )
            for source in sorted(first.rglob("*")):
                if source.is_file():
                    target = output_root / source.relative_to(first)
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(source, target)
            return first_result


def git_diff_names(root: pathlib.Path, paths: list[str]) -> list[str]:
    completed = subprocess.run(
        [
            "git",
            "diff",
            "--name-only",
            f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD",
            "--",
            *paths,
        ],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    require(completed.returncode == 0, "git-diff", completed.stderr)
    return completed.stdout.splitlines()


def verify_semantic_diff(root: pathlib.Path) -> None:
    changed = git_diff_names(root, ["cpu/upd9002/"])
    require(
        changed == ["cpu/upd9002/i286c_mn.c"],
        "semantic-scope",
        repr(changed),
    )
    completed = subprocess.run(
        [
            "git",
            "diff",
            f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD",
            "--",
            "cpu/upd9002/i286c_mn.c",
        ],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
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
            "\t\tGET_PCBYTE(*(REG8_B53(op)))",
            "\t\tGET_PCWORD(*(REG16_B53(op)))",
        ]
        and added
        == [
            "\t\tGET_PCBYTE(*(REG8_B20(op)))",
            "\t\tGET_PCWORD(*(REG16_B20(op)))",
        ],
        "semantic-scope",
        "CPU diff is not the two evidence-proven destination selections",
    )


def verify_protected(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    protected = [
        "tests/ssts/contracts/",
        "tests/ssts/epochs/g43/",
        "tests/ssts/evidence/g59/",
        "tests/ssts/evidence/g60",
        "tests/ssts/authority/",
        "tests/ssts/target_policy/g60b.json",
        "tests/ssts/gap_taxonomy.json",
        "tests/ssts/hardware_pending.json",
        "tests/ssts/approved_target_divergences.json",
        "tests/ssts/scoreboard/g58",
        "tests/ssts/scoreboard/g60",
        "tests/ssts/transitions/g58",
        "tests/ssts/transitions/g60",
    ]
    require(
        not git_diff_names(root, protected),
        "protected-artifact-mutation",
        "approved evidence changed",
    )
    forbidden = [
        "cpu/upd9002/upd9002_dispatch.c",
        "cpu/upd9002/i286c_ope.c",
        "tests/ssts/contracts/",
        "tests/ssts/v20_dataset_manifest.json",
        "tests/ssts/gap_taxonomy.json",
        "tests/ssts/hardware_pending.json",
    ]
    if not protected_evidence_only:
        require(
            not git_diff_names(root, forbidden),
            "out-of-scope-change",
            "protected behavior changed",
        )


def validate_generated_family(root: pathlib.Path) -> None:
    paths = [
        EVIDENCE_ROOT / "manifest.json",
        RESULT_MANIFEST_PATH,
        *SCOREBOARD_PATHS.values(),
        *TRANSITION_PATHS.values(),
        RANKING_JSON_PATH,
        RANKING_MD_PATH,
    ]
    present = [(root / path).is_file() for path in paths]
    if not any(present):
        return
    require(all(present), "evidence-family-incomplete", repr(present))
    manifest = read_json(root / EVIDENCE_ROOT / "manifest.json")
    require(
        manifest["approved_predecessor_sha"] == APPROVED_PREDECESSOR_SHA
        and manifest["dataset_id"] == DATASET_ID
        and manifest["target_policy_id"] == TARGET_POLICY_ID
        and manifest["selected_hash_set_sha256"] == SELECTED_HASH_SETS
        and manifest["applicable_hash_set_sha256"] == APPLICABLE_HASH_SETS,
        "evidence-identity",
        "governing identity differs",
    )
    for entry in manifest["artifacts"]:
        path = root / entry["path"]
        require(
            path.is_file()
            and path.stat().st_size == entry["bytes"]
            and sha256_file(path) == entry["sha256"],
            "artifact-digest-mismatch",
            entry["path"],
        )
    require(
        m60e.sha256_bytes(canonical_bytes(manifest["artifacts"]))
        == manifest["artifact_tree_sha256"],
        "artifact-digest-mismatch",
        "artifact tree differs",
    )
    ranking = read_json(root / RANKING_JSON_PATH)
    scoreboard = read_json(root / SCOREBOARD_PATHS["architectural_full"])
    require(
        ranking["architectural_full_failure_count"] == scoreboard["fail"]
        and sum(row["fail"] for row in ranking["rows"]) == scoreboard["fail"],
        "ranking-total-mismatch",
        "ranking differs",
    )
    require(
        ranking["c6_post_m61"]["pass"] == 5000
        and ranking["c6_post_m61"]["fail"] == 0
        and ranking["c7_post_m61"]["pass"] == 5000
        and ranking["c7_post_m61"]["fail"] == 0,
        "post-fix-result",
        "ranking C6/C7 differs",
    )


def verify_static(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    forward_milestone = (
        root / "docs/agents/tasks/M62_upd9002_semantics_bundle.md"
    ).is_file()
    protected_evidence_only = protected_evidence_only or forward_milestone
    verify_predecessor(root)
    verify_protected(root, protected_evidence_only)
    if not protected_evidence_only:
        verify_semantic_diff(root)
    validate_generated_family(root)
    print(
        "m61-static: G60e protection and deterministic evidence family"
        + (
            " passed for forward-milestone protection"
            if protected_evidence_only
            else ", exact two-line C6/C7 semantic scope passed"
        )
    )


def synthetic_decision() -> dict[str, Any]:
    governed = ["1" * 64]
    return {
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "dataset_id": DATASET_ID,
        "contracts": copy.deepcopy(CONTRACTS),
        "target_policy_id": TARGET_POLICY_ID,
        "selected_sets": copy.deepcopy(SELECTED_HASH_SETS),
        "applicable_sets": copy.deepcopy(APPLICABLE_HASH_SETS),
        "classification_changed": False,
        "c6_complete": True,
        "c7_complete": True,
        "first_byte_only": False,
        "outcome_partition": False,
        "duplicate_or_missing_hash": False,
        "expected_and_actual": True,
        "same_field_is_execution": True,
        "byte_mapping_correct": True,
        "paired_byte_preserved": True,
        "word_width_correct": True,
        "unrelated_registers_preserved": True,
        "flags_unchanged": True,
        "ip_termination_unchanged": True,
        "memory_forms_unchanged": True,
        "effective_address_unchanged": True,
        "f7_unchanged": True,
        "b0_bf_unchanged": True,
        "protected_behavior_unchanged": True,
        "newly_passing": governed.copy(),
        "governed_newly_passing": governed,
        "newly_failing": [],
        "changed_failure_count": 0,
        "metadata_mask_unchanged": True,
        "fixtures_unchanged": True,
        "protected_artifacts_unchanged": True,
        "evidence_commit_only": True,
        "ranking_total": 53964,
        "scoreboard_total": 53964,
    }


def validate_decision(value: dict[str, Any]) -> None:
    require(value["approved_predecessor_sha"] == APPROVED_PREDECESSOR_SHA,
            "wrong-predecessor-sha", "predecessor")
    require(value["dataset_id"] == DATASET_ID, "dataset-drift", "dataset")
    require(value["contracts"] == CONTRACTS, "comparison-contract-drift", "contracts")
    require(value["target_policy_id"] == TARGET_POLICY_ID, "target-policy-drift", "policy")
    require(value["selected_sets"] == SELECTED_HASH_SETS, "selected-set-drift", "selected")
    require(value["applicable_sets"] == APPLICABLE_HASH_SETS,
            "applicable-set-drift", "applicable")
    checks = (
        ("classification_changed", False, "classification-taxonomy-registry-change"),
        ("c6_complete", True, "incomplete-c6-population"),
        ("c7_complete", True, "incomplete-c7-population"),
        ("first_byte_only", False, "first-byte-only-selector"),
        ("outcome_partition", False, "outcome-derived-partition"),
        ("duplicate_or_missing_hash", False, "duplicate-or-missing-hash"),
        ("expected_and_actual", True, "expected-only-evidence"),
        ("same_field_is_execution", True, "value-coincidence-misdiagnosis"),
        ("byte_mapping_correct", True, "c6-high-byte-mapping"),
        ("paired_byte_preserved", True, "c6-paired-byte-corruption"),
        ("word_width_correct", True, "c7-width-or-byte-order"),
        ("unrelated_registers_preserved", True, "unrelated-register-change"),
        ("flags_unchanged", True, "flags-change"),
        ("ip_termination_unchanged", True, "ip-or-termination-change"),
        ("memory_forms_unchanged", True, "memory-form-output-change"),
        ("effective_address_unchanged", True, "effective-address-change"),
        ("f7_unchanged", True, "f7-change"),
        ("b0_bf_unchanged", True, "b0-bf-change"),
        ("protected_behavior_unchanged", True, "protected-regression"),
        ("metadata_mask_unchanged", True, "comparison-mask-change"),
        ("fixtures_unchanged", True, "fixture-change"),
        ("protected_artifacts_unchanged", True, "protected-artifact-mutation"),
        ("evidence_commit_only", True, "evidence-commit-scope"),
    )
    for key, expected, code in checks:
        require(value[key] == expected, code, key)
    require(not value["newly_failing"], "newly-failing", "new failure")
    require(
        set(value["newly_passing"]) <= set(value["governed_newly_passing"]),
        "ungoverned-newly-passing",
        "new pass",
    )
    require(value["changed_failure_count"] == 0,
            "changed-failure-not-enumerated", "changed failure")
    require(value["ranking_total"] == value["scoreboard_total"],
            "ranking-total-mismatch", "ranking")


def expect_rejection(code: str, mutation: Callable[[dict[str, Any]], None]) -> None:
    value = synthetic_decision()
    mutation(value)
    try:
        validate_decision(value)
    except M61Error as error:
        if error.code != code:
            raise AssertionError(f"expected {code}, got {error.code}") from error
        return
    raise AssertionError(f"{code} mutation was accepted")


def selftest() -> None:
    validate_decision(synthetic_decision())
    mutations = [
        ("wrong-predecessor-sha",
         lambda v: v.__setitem__("approved_predecessor_sha", "0" * 40)),
        ("dataset-drift", lambda v: v.__setitem__("dataset_id", "wrong")),
        ("comparison-contract-drift", lambda v: v.__setitem__("contracts", {})),
        ("target-policy-drift", lambda v: v.__setitem__("target_policy_id", "wrong")),
        ("selected-set-drift", lambda v: v.__setitem__("selected_sets", {})),
        ("applicable-set-drift", lambda v: v.__setitem__("applicable_sets", {})),
        ("classification-taxonomy-registry-change",
         lambda v: v.__setitem__("classification_changed", True)),
        ("incomplete-c6-population", lambda v: v.__setitem__("c6_complete", False)),
        ("incomplete-c7-population", lambda v: v.__setitem__("c7_complete", False)),
        ("first-byte-only-selector", lambda v: v.__setitem__("first_byte_only", True)),
        ("outcome-derived-partition", lambda v: v.__setitem__("outcome_partition", True)),
        ("duplicate-or-missing-hash",
         lambda v: v.__setitem__("duplicate_or_missing_hash", True)),
        ("expected-only-evidence", lambda v: v.__setitem__("expected_and_actual", False)),
        ("value-coincidence-misdiagnosis",
         lambda v: v.__setitem__("same_field_is_execution", False)),
        ("c6-high-byte-mapping", lambda v: v.__setitem__("byte_mapping_correct", False)),
        ("c6-paired-byte-corruption",
         lambda v: v.__setitem__("paired_byte_preserved", False)),
        ("c7-width-or-byte-order", lambda v: v.__setitem__("word_width_correct", False)),
        ("unrelated-register-change",
         lambda v: v.__setitem__("unrelated_registers_preserved", False)),
        ("flags-change", lambda v: v.__setitem__("flags_unchanged", False)),
        ("ip-or-termination-change",
         lambda v: v.__setitem__("ip_termination_unchanged", False)),
        ("memory-form-output-change",
         lambda v: v.__setitem__("memory_forms_unchanged", False)),
        ("effective-address-change",
         lambda v: v.__setitem__("effective_address_unchanged", False)),
        ("f7-change", lambda v: v.__setitem__("f7_unchanged", False)),
        ("b0-bf-change", lambda v: v.__setitem__("b0_bf_unchanged", False)),
        ("protected-regression",
         lambda v: v.__setitem__("protected_behavior_unchanged", False)),
        ("newly-failing", lambda v: v["newly_failing"].append("2" * 64)),
        ("ungoverned-newly-passing",
         lambda v: v["newly_passing"].append("2" * 64)),
        ("changed-failure-not-enumerated",
         lambda v: v.__setitem__("changed_failure_count", 1)),
        ("comparison-mask-change",
         lambda v: v.__setitem__("metadata_mask_unchanged", False)),
        ("fixture-change", lambda v: v.__setitem__("fixtures_unchanged", False)),
        ("protected-artifact-mutation",
         lambda v: v.__setitem__("protected_artifacts_unchanged", False)),
        ("evidence-commit-scope",
         lambda v: v.__setitem__("evidence_commit_only", False)),
        ("ranking-total-mismatch", lambda v: v.__setitem__("ranking_total", 1)),
    ]
    for code, mutation in mutations:
        expect_rejection(code, mutation)
    with tempfile.TemporaryDirectory(prefix="vaeg-m61-selftest-") as name:
        directory = pathlib.Path(name)
        payload = {"row_count": 1, "rows": [{"case_hash": "1" * 64}]}
        first = directory / "first.json.gz"
        second = directory / "second.json.gz"
        ratchet.write_deterministic_gzip(first, payload)
        ratchet.write_deterministic_gzip(second, payload)
        require(first.read_bytes() == second.read_bytes(),
                "nondeterministic-gzip", "gzip differs")
        write_json(directory / "canonical.json", payload)
        require(
            (directory / "canonical.json").read_bytes() == canonical_bytes(payload) + b"\n",
            "nondeterministic-json",
            "JSON differs",
        )
    print(
        f"m61-selftest: {len(mutations)} fail-closed mutations rejected at "
        "their intended checks; deterministic JSON/gzip passed"
    )


def add_root(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("selftest")
    static = sub.add_parser("verify-static")
    add_root(static)
    audit = sub.add_parser("audit")
    add_root(audit)
    audit.add_argument("--dataset-root", type=pathlib.Path, required=True)
    audit.add_argument("--worker", type=pathlib.Path, required=True)
    audit.add_argument("--phase", choices=("pre-fix", "post-fix"), required=True)
    audit.add_argument("--source-sha", required=True)
    audit.add_argument("--output", type=pathlib.Path, required=True)
    for command in ("generate", "regenerate-twice"):
        generator = sub.add_parser(command)
        add_root(generator)
        generator.add_argument("--dataset-root", type=pathlib.Path, required=True)
        generator.add_argument("--pre-fix-audit", type=pathlib.Path, required=True)
        generator.add_argument("--post-fix-audit", type=pathlib.Path, required=True)
        generator.add_argument("--architectural-ci-raw", type=pathlib.Path, required=True)
        generator.add_argument(
            "--architectural-full-raw", type=pathlib.Path, required=True
        )
        generator.add_argument("--fingerprint-full-raw", type=pathlib.Path, required=True)
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
            verify_static(root)
        elif arguments.command == "audit":
            verify_predecessor(root)
            audit = run_audit(
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
                "m61-audit: "
                + " ".join(
                    f"{form} pass={audit['summaries'][form]['architectural_pass_count']} "
                    f"fail={audit['summaries'][form]['architectural_failure_count']}"
                    for form in ("C6", "C7")
                )
            )
        elif arguments.command in {"generate", "regenerate-twice"}:
            callback = regenerate_twice if arguments.command == "regenerate-twice" else generate
            result = callback(
                root=root,
                output_root=arguments.output_root.resolve(),
                dataset_root=arguments.dataset_root.resolve(),
                pre_fix_audit=arguments.pre_fix_audit.resolve(),
                post_fix_audit=arguments.post_fix_audit.resolve(),
                raw_paths={
                    "architectural_ci": arguments.architectural_ci_raw.resolve(),
                    "architectural_full": arguments.architectural_full_raw.resolve(),
                    "fingerprint_full": arguments.fingerprint_full_raw.resolve(),
                },
                evaluated_sha=arguments.evaluated_sha,
            )
            print(
                "m61-generate: "
                f"manifest={result['evidence_manifest_sha256']} "
                f"artifact_tree={result['artifact_tree_sha256']}"
            )
        else:
            raise AssertionError(arguments.command)
    except (
        M61Error,
        m60b.M60bError,
        m60e.M60eError,
        m59.EvidenceError,
        ssts.CorpusError,
        OSError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"m61-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

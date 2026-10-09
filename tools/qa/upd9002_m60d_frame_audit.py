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
"""Audit and close the M60d synchronous interrupt-frame residual."""

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
from typing import Any, Callable, Iterable

import upd9002_m60b_authority as m60b
import upd9002_m60c_audit as m60c
import upd9002_semantics_evidence as m59
import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M60d"
CANDIDATE_GATE = "G60d"
APPROVED_PREDECESSOR_GATE = "G60c"
# Evidence keeps the original identity; Git topology starts at the current
# main-history checkpoint established after the protected-history rewrite.
APPROVED_PREDECESSOR_SHA = "e425e55fc17117000ba5178a796de4444d897234"
APPROVED_PREDECESSOR_GIT_SHA = "a187d14427b7532e16487653f8ffb6fe37c9703d"
G60C_EVALUATED_SHA = "a9dd78bded5c1072f0285f00cf7759654da8b7d8"
G60C_CI_URL = "https://github.com/nakatamaho/vaeg/actions/runs/30148175007"
TARGET_POLICY_ID = (
    "upd9002-g60b-"
    "eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93"
)
TARGET_POLICY_SHA256 = (
    "eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93"
)
DATASET_ID = (
    "ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-"
    "1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4"
)
CONTRACTS = {
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
PROFILE_IDENTITIES = {
    "architectural_ci": {
        "applicable": 165300,
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
    },
    "architectural_full": {
        "applicable": 1438594,
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
    },
    "fingerprint_full": {
        "applicable": 1438594,
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
    },
}
PRIMARY_FRAME_COUNT = 12468
PRIMARY_FRAME_SHA256 = (
    "4498c8aa838f93aba7220f0cdacff34341d704a9cbe7f6d35d79b75219b41d0b"
)
BOUND_FRAME_COUNT = 3565
BOUND_FRAME_SHA256 = (
    "15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746"
)
BOUND_RANGE_COUNT = 1244
BOUND_RANGE_SHA256 = (
    "2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d"
)
DIV_DEPENDENCY_FORMS = {
    "F6.6": (
        48,
        "2223861f50be66681297264720400b50a760163404222b705cc01ae53aa62d5b",
    ),
    "F6.7": (
        64,
        "f120a2a55e5c992390762aa5303195dc198fc3e68d052845bc061536cbb74eac",
    ),
    "F7.6": (
        39,
        "f2e51ac87d951a68210f7917a7b4231c7dd9c31c3ace81895190e6938893eedb",
    ),
    "F7.7": (
        63,
        "b0da56dbe6553fa1de93418a2e4dc1165e94e6475a72eb2a224a94a163f6e6db",
    ),
}
DIV_DEPENDENCY_COUNT = 214
EMPTY_HASH_SET_SHA256 = ratchet.hash_set_digest([])

EVIDENCE_ROOT = pathlib.Path("tests/ssts/evidence/g60d")
RESULT_MANIFEST_PATH = pathlib.Path("tests/ssts/evidence/g60d_result_manifest.json")
TRANSITION_PATHS = {
    "architectural_ci": pathlib.Path(
        "tests/ssts/transitions/g60d_architectural_ci_from_g60c.json"
    ),
    "architectural_full": pathlib.Path(
        "tests/ssts/transitions/g60d_architectural_full_from_g60c.json"
    ),
}
SCOREBOARD_PATHS = {
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
APPROVED_SCOREBOARD_PATHS = {
    key: pathlib.Path(str(path).replace("g60d_", "g60b_"))
    for key, path in SCOREBOARD_PATHS.items()
}
FAILURE_DIRECTORY_PATHS = {
    key: pathlib.Path(str(path).removesuffix(".json") + "_failures")
    for key, path in SCOREBOARD_PATHS.items()
}
M59_FLAGS_PATH = pathlib.Path("tests/ssts/evidence/g59/cases/flags.json.gz")
M59_BOUND_PATH = pathlib.Path("tests/ssts/evidence/g59/cases/ff7_bound.json.gz")
M60A_TRANSITION_PATH = pathlib.Path(
    "tests/ssts/transitions/g60a_architectural_full_from_g59.json"
)
M60A_SUMMARY_PATH = pathlib.Path(
    "tests/ssts/transitions/g60a_flags_materialization_summary.json"
)
DATASET_MANIFEST_PATH = pathlib.Path("tests/ssts/v20_dataset_manifest.json")
REPORT_PATH = pathlib.Path(
    "docs/agents/reports/m60d_upd9002_interrupt_frame.md"
)
EVIDENCE_MANIFEST_PATH = EVIDENCE_ROOT / "manifest.json"
EVIDENCE_COMMIT_IDENTITY_ERROR = "M60D_EVIDENCE_COMMIT_IDENTITY"
EVIDENCE_COMMIT_SCOPE_ERROR = "M60D_EVIDENCE_COMMIT_SCOPE"
EVIDENCE_COMMIT_ALLOWED_PATHS = (
    "docs/agents/reports/m60d_upd9002_interrupt_frame.md",
    "tests/ssts/evidence/g60d/",
    "tests/ssts/evidence/g60d_result_manifest.json",
    "tests/ssts/scoreboard/g60d_",
    "tests/ssts/transitions/g60d_",
)
HEX40 = re.compile(r"^[0-9a-f]{40}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
FOCUS_FORMS = {"CC", "CD", "CE", "62"}
DIV_FORMS = set(DIV_DEPENDENCY_FORMS)


class M60dError(RuntimeError):
    """A fail-closed M60d audit or evidence validation failure."""


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


def require_sha(value: Any, where: str, pattern: re.Pattern[str] = HEX64) -> str:
    if not isinstance(value, str) or pattern.fullmatch(value) is None:
        raise M60dError(f"{where}: malformed content hash")
    return value


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value) + b"\n")


def read_json(path: pathlib.Path) -> Any:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def output_path(output_root: pathlib.Path, relative: pathlib.Path) -> pathlib.Path:
    return output_root / relative


def verify_upstream_static(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    before = (
        ratchet.APPROVED_PREDECESSOR_GATE,
        ratchet.APPROVED_PREDECESSOR_SHA,
        ratchet.EPOCH_GATE,
    )
    ratchet.APPROVED_PREDECESSOR_GATE = "G57"
    ratchet.APPROVED_PREDECESSOR_SHA = (
        "72322d5c9b8e40e4a988312aebe163a8190e2aa5"
    )
    ratchet.EPOCH_GATE = "G58"
    try:
        m60c.erratum.verify_static(root)
        try:
            m60b.verify_static(root, protected_evidence_only)
        except m60b.M60bError as error:
            raise M60dError(str(error)) from error
        family = [
            (root / m60c.AUTHORITY_ROOT / "manifest.json").is_file(),
            (root / m60c.TRANSITION_PATH).is_file(),
            (root / m60c.RESULT_MANIFEST_PATH).is_file(),
        ]
        if any(family) and not all(family):
            raise M60dError("G60c evidence family is incomplete")
        if all(family):
            m60c.validate_result_manifest(root)
    finally:
        (
            ratchet.APPROVED_PREDECESSOR_GATE,
            ratchet.APPROVED_PREDECESSOR_SHA,
            ratchet.EPOCH_GATE,
        ) = before


def hex_registers(registers: dict[str, int]) -> dict[str, str]:
    return {name: f"{registers[name]:04x}" for name in ssts.REGISTER_ORDER}


def ram_rows(memory: dict[int, int]) -> list[dict[str, str]]:
    return [
        {"address": f"{address:05x}", "value": f"{memory[address]:02x}"}
        for address in sorted(memory)
    ]


def load_table(path: pathlib.Path) -> list[dict[str, Any]]:
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        value = json.load(stream)
    if (
        not isinstance(value, dict)
        or not isinstance(value.get("rows"), list)
        or value.get("row_count") != len(value["rows"])
    ):
        raise M60dError(f"{path}: malformed M59 case table")
    return value["rows"]


def load_approved_populations(root: pathlib.Path) -> dict[str, Any]:
    flags = load_table(root / M59_FLAGS_PATH)
    primary = sorted(
        row["case_hash"]
        for row in flags
        if row["form"] in {"CC", "CD", "CE"}
        and row["primary_partition"] != "no-interrupt-expected"
    )
    ce_taken = sorted(
        row["case_hash"]
        for row in flags
        if row["form"] == "CE"
        and row["primary_partition"] != "no-interrupt-expected"
    )
    ce_not_taken = sorted(
        row["case_hash"]
        for row in flags
        if row["form"] == "CE"
        and row["primary_partition"] == "no-interrupt-expected"
    )
    if len(primary) != PRIMARY_FRAME_COUNT or ratchet.hash_set_digest(
        primary
    ) != PRIMARY_FRAME_SHA256:
        raise M60dError("M59 primary interrupt-frame population changed")
    if len(ce_taken) != 2468 or len(ce_not_taken) != 2532:
        raise M60dError("M59 taken/non-taken CE partition changed")

    bound = [row for row in load_table(root / M59_BOUND_PATH) if row["form"] == "62"]
    frame = sorted(
        row["case_hash"]
        for row in bound
        if row["primary_partition"] == "stack-frame-mismatch"
    )
    range_residual = sorted(
        row["case_hash"]
        for row in bound
        if row["primary_partition"] == "range-result-mismatch"
    )
    normal = sorted(
        row["case_hash"]
        for row in bound
        if row["primary_partition"] == "normal-completion"
    )
    if len(frame) != BOUND_FRAME_COUNT or ratchet.hash_set_digest(
        frame
    ) != BOUND_FRAME_SHA256:
        raise M60dError("M59 BOUND frame-only population changed")
    if len(range_residual) != BOUND_RANGE_COUNT or ratchet.hash_set_digest(
        range_residual
    ) != BOUND_RANGE_SHA256:
        raise M60dError("M59 BOUND range-residual population changed")
    if len(normal) != 191:
        raise M60dError("M59 BOUND previously passing population changed")
    if set(frame) & set(range_residual) or set(frame) & set(normal):
        raise M60dError("M59 BOUND partitions overlap")
    if len(set(frame) | set(range_residual) | set(normal)) != 5000:
        raise M60dError("M59 BOUND partitions are incomplete")

    transition = read_json(root / M60A_TRANSITION_PATH)
    if (
        transition.get("before_gate") != "G59"
        or transition.get("scope") != "full"
        or transition.get("profile") != "architectural"
    ):
        raise M60dError("G60a transition identity changed")
    newly_passing = transition.get("newly_passing")
    if (
        not isinstance(newly_passing, list)
        or newly_passing != sorted(set(newly_passing))
        or not all(HEX64.fullmatch(item) for item in newly_passing)
    ):
        raise M60dError("G60a newly-passing population is malformed")
    summary = read_json(root / M60A_SUMMARY_PATH)
    if not isinstance(summary.get("dependent_interrupt_frame_effects"), dict):
        raise M60dError("G60a dependent frame summary changed")
    return {
        "bound_frame": set(frame),
        "bound_normal": set(normal),
        "bound_range": set(range_residual),
        "ce_not_taken": set(ce_not_taken),
        "ce_taken": set(ce_taken),
        "g60a_newly_passing": set(newly_passing),
        "primary": set(primary),
    }


def load_scoreboard_failures(
    root: pathlib.Path, summary_path: pathlib.Path
) -> dict[str, dict[str, Any]]:
    summary = read_json(root / summary_path)
    failures: dict[str, dict[str, Any]] = {}
    for shard in summary["failure_shards"]:
        path = (root / summary_path).parent / shard["path"]
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            payload = json.load(stream)
        if payload.get("failure_count") != len(payload.get("failures", [])):
            raise M60dError(f"{path}: malformed approved failure shard")
        for entry in payload["failures"]:
            record_hash = entry.get("record_hash")
            require_sha(record_hash, f"{path}: record_hash")
            if record_hash in failures:
                raise M60dError("approved scoreboard failure ownership overlaps")
            failures[record_hash] = entry
    if len(failures) != summary["fail"]:
        raise M60dError("approved scoreboard failure count changed")
    return failures


def verify_raw_profile(
    root: pathlib.Path,
    raw_path: pathlib.Path,
    profile_key: str,
) -> tuple[dict[str, Any], dict[str, dict[str, Any]], dict[str, Any]]:
    raw = read_json(raw_path)
    approved_path = APPROVED_SCOREBOARD_PATHS[profile_key]
    approved = read_json(root / approved_path)
    expected = PROFILE_IDENTITIES[profile_key]
    profile, scope = profile_key.rsplit("_", 1)
    if (
        raw.get("schema") != "vaeg-upd9002-ssts-result-v1"
        or raw.get("dataset_id") != DATASET_ID
        or raw.get("profile") != scope
    ):
        raise M60dError(f"{profile_key}: raw profile identity changed")
    if profile == "fingerprint":
        if raw.get("flags_comparison") != "all16":
            raise M60dError("fingerprint raw profile lacks all16 FLAGS")
    elif "flags_comparison" in raw:
        raise M60dError("architectural raw profile changed comparison mode")
    if raw.get("selected_records") != expected["selected"]:
        raise M60dError(f"{profile_key}: selected count changed")
    if raw.get("executed_records") != expected["executed"]:
        raise M60dError(f"{profile_key}: executed count changed")
    result_counts = raw.get("result_counts", {})
    if result_counts.get("pass", 0) != expected["pass"]:
        raise M60dError(f"{profile_key}: pass count changed")
    fail = sum(
        result_counts.get(key, 0)
        for key in ("semantic_failure", "timeout", "crash")
    )
    if fail != expected["fail"]:
        raise M60dError(f"{profile_key}: failure count changed")
    if result_counts.get("timeout", 0) or result_counts.get("crash", 0):
        raise M60dError(f"{profile_key}: timeout or crash")
    failures_raw = ratchet.load_failure_records(raw_path)
    failures = {
        record_hash: ratchet.failure_entry(failure)
        for record_hash, failure in failures_raw.items()
    }
    if ratchet.hash_set_digest(failures) != expected["failure_hash_set_sha256"]:
        raise M60dError(f"{profile_key}: failure hash set changed")
    if raw.get("failure_signature_index_sha256") != expected[
        "failure_signature_index_sha256"
    ]:
        raise M60dError(f"{profile_key}: failure signatures changed")
    if failures != load_scoreboard_failures(root, approved_path):
        raise M60dError(f"{profile_key}: failure content changed")
    if raw.get("termination_counts") != approved["termination_classes"]:
        raise M60dError(f"{profile_key}: termination classes changed")
    raw_forms = {row["form"]: row for row in raw["per_form"]}
    if len(raw_forms) != len(raw["per_form"]):
        raise M60dError(f"{profile_key}: duplicate raw structural form")
    approved_forms: dict[str, dict[str, int]] = defaultdict(
        lambda: {"fail": 0, "pass": 0, "selected": 0}
    )
    for row in approved["records"]:
        for field in ("fail", "pass", "selected"):
            approved_forms[row["form"]][field] += row[field]
    for form, approved_counts in sorted(approved_forms.items()):
        raw_row = raw_forms.get(form)
        if raw_row is None:
            raise M60dError(f"{profile_key}: missing structural form")
        raw_fail = sum(
            raw_row["result_counts"].get(kind, 0)
            for kind in ("semantic_failure", "timeout", "crash")
        )
        if (
            raw_row["selected_count"] != approved_counts["selected"]
            or raw_row["result_counts"].get("pass", 0) != approved_counts["pass"]
            or raw_fail != approved_counts["fail"]
        ):
            raise M60dError(
                f"{profile_key}: per-form result changed: {form}"
            )
    return raw, failures_raw, approved


def expected_event(record: dict[str, Any], form: str) -> dict[str, Any]:
    observed = m59.expected_execution(record, form)
    if ssts.expected_termination(form, record) == "type0":
        return {"active": True, "kind": "exception", "vector": 0}
    if observed["kind"] == "interrupt":
        return {
            "active": True,
            "kind": "interrupt",
            "vector": observed["interrupt_vector"],
        }
    return {"active": False, "kind": "normal", "vector": None}


def actual_event(actual: dict[str, Any]) -> dict[str, Any]:
    execution = actual["execution_result"]
    return {
        "active": execution["interrupt_count"] > 0,
        "interrupt_count": execution["interrupt_count"],
        "kind": execution["kind"],
        "termination": execution["termination"],
        "vector": execution["interrupt_vector"],
    }


def vector_observation(
    record: dict[str, Any], vector: int | None
) -> dict[str, Any] | None:
    if vector is None:
        return None
    memory = {address: value for address, value in record["initial"]["ram"]}
    addresses = [vector * 4 + index for index in range(4)]
    if any(address not in memory for address in addresses):
        return {
            "addresses": [f"{address:05x}" for address in addresses],
            "mapping": "underdetermined",
            "target_cs": None,
            "target_ip": None,
        }
    return {
        "addresses": [f"{address:05x}" for address in addresses],
        "mapping": "determined",
        "target_cs": f"{memory[addresses[2]] | memory[addresses[3]] << 8:04x}",
        "target_ip": f"{memory[addresses[0]] | memory[addresses[1]] << 8:04x}",
    }


def compare_case(
    form: str,
    record: dict[str, Any],
    resolved: dict[str, Any],
    actual: dict[str, Any],
    ownership: str,
) -> tuple[dict[str, Any], list[str]]:
    expected_regs = ssts.expected_registers(record)
    watch, expected_ram = ssts.expected_memory(record)
    context = {
        "record": record,
        "record_digest": ssts.sha256_bytes(ssts.canonical_bytes(record)),
        "watch": watch,
        "expected_ram": expected_ram,
    }
    failure = ssts.make_failure(
        DATASET_ID,
        "full",
        form,
        resolved["classification"],
        resolved["flags_mask"],
        context,
        "ok",
        actual,
    )["content"]
    expected = expected_event(record, form)
    observed = actual_event(actual)
    reasons: list[str] = []
    non_frame_reasons: list[str] = []
    frame: dict[str, Any] | None = None
    decision_matches = (
        expected["active"] == observed["active"]
        and (
            not expected["active"]
            or expected["vector"] == observed["vector"]
        )
    )
    if expected["active"] and observed["active"] and decision_matches:
        frame_data, logical, physical, boundary = m59.frame_analysis(
            record, expected_regs, actual, expected_ram
        )
        checks = (
            ("final-sp", "expected_final_sp", "actual_final_sp"),
            (
                "frame-logical-address",
                "expected_frame_logical_addresses",
                "actual_frame_logical_addresses",
            ),
            (
                "frame-physical-address",
                "expected_frame_physical_addresses",
                "actual_frame_physical_addresses",
            ),
            ("saved-ip", "expected_saved_ip", "actual_saved_ip"),
            ("saved-cs", "expected_saved_cs", "actual_saved_cs"),
            ("saved-flags", "expected_saved_flags", "actual_saved_flags"),
        )
        for label, expected_key, actual_key in checks:
            if frame_data[expected_key] != frame_data[actual_key]:
                reasons.append(label)
        if expected_regs["ip"] != actual["registers"]["ip"]:
            reasons.append("final-target-ip")
        if expected_regs["cs"] != actual["registers"]["cs"]:
            reasons.append("final-target-cs")
        for label, bit in (("post-entry-tf", 8), ("post-entry-if", 9)):
            if (
                (expected_regs["flags"] >> bit) & 1
            ) != ((actual["registers"]["flags"] >> bit) & 1):
                reasons.append(label)
        frame = {
            "addresses": {"logical": logical, "physical": physical},
            "boundary_partition": boundary,
            "observables": frame_data,
        }
        if "saved-flags" in reasons and form in DIV_FORMS:
            expected_saved = frame_data["expected_saved_flags"]
            actual_saved = frame_data["actual_saved_flags"]
            post_entry_mask = 0xFCFF
            if (
                expected_saved is not None
                and actual_saved is not None
                and (int(expected_saved, 16) & post_entry_mask)
                == (expected_regs["flags"] & post_entry_mask)
                and (int(actual_saved, 16) & post_entry_mask)
                == (actual["registers"]["flags"] & post_entry_mask)
            ):
                reasons.remove("saved-flags")
                non_frame_reasons.append("divide-pre-event-flags-state")
                ownership = "divide-arithmetic-flags"
    elif expected["active"] != observed["active"]:
        ownership = (
            "bound-range-decision"
            if form == "62"
            else (
                "divide-arithmetic-decision"
                if form in DIV_FORMS
                else "event-decision"
            )
        )
    elif expected["active"] and expected["vector"] != observed["vector"]:
        ownership = "event-vector-decision"
    if "saved-flags" in reasons:
        raise M60dError(
            f"{context['record_digest']}: {form} ({ownership}) saved-FLAGS "
            "contradicts approved M60a: "
            f"expected={frame_data['expected_saved_flags']} "
            f"actual={frame_data['actual_saved_flags']} "
            f"boundary={boundary} upstream={record['hash']}"
        )
    return {
        "actual_event": observed,
        "actual_final_ram": ram_rows(actual["ram"]),
        "actual_final_registers": hex_registers(actual["registers"]),
        "architectural_mismatch_kinds": failure["mismatch_kinds"],
        "architectural_outcome": (
            "pass" if not failure["mismatch_kinds"] else "fail"
        ),
        "case_hash": context["record_digest"],
        "conclusion_status": "proven",
        "expected_event": expected,
        "expected_final_ram": ram_rows(expected_ram),
        "expected_final_registers": hex_registers(expected_regs),
        "form": form,
        "frame": frame,
        "frame_residual_reasons": sorted(set(reasons)),
        "initial_registers": hex_registers(record["initial"]["regs"]),
        "instruction_bytes": "".join(f"{byte:02x}" for byte in record["bytes"]),
        "non_frame_mismatch_reasons": sorted(set(non_frame_reasons)),
        "ownership": ownership,
        "top_level_classification": resolved["classification"],
        "upstream_case_hash": record["hash"],
        "vector_table": vector_observation(record, expected["vector"]),
    }, sorted(set(reasons))


def summarize_rows(
    rows: list[dict[str, Any]], name: str
) -> dict[str, Any]:
    hashes = [row["case_hash"] for row in rows]
    return {
        "architectural_outcomes": dict(
            sorted(Counter(row["architectural_outcome"] for row in rows).items())
        ),
        "frame_boundary_partitions": dict(
            sorted(
                Counter(
                    row["frame"]["boundary_partition"]
                    for row in rows
                    if row["frame"] is not None
                ).items()
            )
        ),
        "frame_residual_count": sum(
            bool(row["frame_residual_reasons"]) for row in rows
        ),
        "name": name,
        "resolved_count": len(rows),
        "resolved_hashes_sha256": ratchet.hash_set_digest(hashes),
        "schema": "vaeg-upd9002-m60d-population-summary-v1",
        "schema_version": 1,
    }


def artifact_entry(path: pathlib.Path, relative: pathlib.Path) -> dict[str, Any]:
    if relative.suffix == ".gz":
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            value = json.load(stream)
    elif relative.suffix == ".json":
        value = read_json(path)
    else:
        value = None
    if isinstance(value, dict):
        if isinstance(value.get("row_count"), int):
            rows = value["row_count"]
        elif isinstance(value.get("failure_count"), int):
            rows = value["failure_count"]
        elif isinstance(value.get("artifacts"), list):
            rows = len(value["artifacts"])
        else:
            rows = 1
    elif isinstance(value, list):
        rows = len(value)
    else:
        rows = 1
    return {
        "bytes": path.stat().st_size,
        "path": relative.as_posix(),
        "row_count": rows,
        "sha256": sha256_file(path),
    }


def write_representative(
    path: pathlib.Path,
    title: str,
    rows: list[dict[str, Any]],
    selectors: list[tuple[str, Callable[[dict[str, Any]], bool]]],
) -> list[str]:
    selected: list[tuple[str, dict[str, Any]]] = []
    for label, predicate in selectors:
        candidates = sorted(
            (row for row in rows if predicate(row)),
            key=lambda row: row["case_hash"],
        )
        if not candidates:
            raise M60dError(f"{title}: no representative for {label}")
        selected.append((label, candidates[0]))
    lines = [
        "<!--",
        "Copyright (c) 2026 Nakata Maho",
        "SPDX-License-Identifier: BSD-2-Clause",
        "-->",
        f"# {title}",
        "",
        "These cases are deterministic representatives of the complete machine table.",
        "",
    ]
    for label, row in selected:
        lines.extend(
            [
                f"## {label}",
                "",
                f"- Case hash: `{row['case_hash']}`",
                f"- Form: `{row['form']}`",
                f"- Bytes: `{row['instruction_bytes']}`",
                f"- Outcome: `{row['architectural_outcome']}`",
                f"- Ownership: `{row['ownership']}`",
                f"- Frame residuals: "
                f"`{','.join(row['frame_residual_reasons']) or 'none'}`",
                "",
            ]
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return [row["case_hash"] for _, row in selected]


def run_audit(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    architectural_full_raw: pathlib.Path,
) -> dict[str, Any]:
    populations = load_approved_populations(root)
    _, raw_failures, _ = verify_raw_profile(
        root, architectural_full_raw, "architectural_full"
    )
    failure_entries = {
        record_hash: ratchet.failure_entry(value)
        for record_hash, value in raw_failures.items()
    }
    failure_by_form_upstream = {
        (entry["form"], entry["upstream_test_hash"]): record_hash
        for record_hash, entry in failure_entries.items()
    }
    if len(failure_by_form_upstream) != len(failure_entries):
        raise M60dError("current failure ownership is ambiguous")
    manifest = ssts.load_manifest(root / DATASET_MANIFEST_PATH)
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / ssts.SUITE_PATH / "metadata.json")
    ssts.validate_metadata(metadata)
    g60a_newly_passing = populations["g60a_newly_passing"]
    focus_rows: list[dict[str, Any]] = []
    global_event_rows: list[dict[str, Any]] = []
    div_dependency: dict[str, list[str]] = defaultdict(list)
    seen_failures: set[str] = set()

    with m60b.candidate_support_map(root) as support_path:
        support = ssts.load_support_map(support_path)
        for corpus_path in ssts.corpus_files(dataset_root):
            form = corpus_path.name.removesuffix(".json.gz").upper()
            with gzip.open(corpus_path, "rt", encoding="utf-8") as stream:
                records = json.load(stream)
            selected = ssts.profile_records(records, "full")
            candidates: list[dict[str, Any]] = []
            roles: dict[str, set[str]] = defaultdict(set)
            for record in selected:
                failure_hash = failure_by_form_upstream.get((form, record["hash"]))
                record_hash: str | None = None
                if form in FOCUS_FORMS:
                    record_hash = ssts.sha256_bytes(ssts.canonical_bytes(record))
                    roles[record_hash].add("focus")
                if failure_hash is not None:
                    record_hash = record_hash or ssts.sha256_bytes(
                        ssts.canonical_bytes(record)
                    )
                    if record_hash != failure_hash:
                        raise M60dError(
                            f"{form}: current failure hash mapping changed"
                        )
                    roles[record_hash].add("global-failure")
                if form in DIV_FORMS:
                    record_hash = record_hash or ssts.sha256_bytes(
                        ssts.canonical_bytes(record)
                    )
                    if record_hash in g60a_newly_passing:
                        roles[record_hash].add("divide-dependency")
                if record_hash is not None and roles[record_hash]:
                    candidates.append(record)
            if not candidates:
                continue
            results = ssts.run_worker_contained(worker, candidates, 120.0)
            if len(results) != len(candidates):
                raise M60dError(f"{form}: worker result count changed")
            for record, (status, actual) in zip(candidates, results):
                if status != "ok" or actual is None:
                    raise M60dError(f"{form}:{record['hash']}: {status}")
                record_hash = ssts.sha256_bytes(ssts.canonical_bytes(record))
                resolved = ssts.classify_record(form, record, metadata, support)
                if resolved["classification"] != "applicable":
                    raise M60dError(
                        f"{form}:{record_hash}: focus/failure left applicable"
                    )
                ownership = (
                    "primary-synchronous-frame"
                    if form in {"CC", "CD", "CE"}
                    else (
                        "bound-frame-or-range"
                        if form == "62"
                        else (
                            "divide-exception-dependency"
                            if form in DIV_FORMS
                            else "global-frame-signature-scan"
                        )
                    )
                )
                row, residuals = compare_case(
                    form, record, resolved, actual, ownership
                )
                if (
                    "focus" in roles[record_hash]
                    or "divide-dependency" in roles[record_hash]
                ):
                    focus_rows.append(row)
                if "divide-dependency" in roles[record_hash]:
                    div_dependency[form].append(record_hash)
                if "global-failure" in roles[record_hash]:
                    seen_failures.add(record_hash)
                    if (
                        row["expected_event"]["active"]
                        or row["actual_event"]["active"]
                    ):
                        global_event_rows.append(row)
                    if residuals:
                        # Retained for the final exact Path A/Path B decision.
                        pass
    if seen_failures != set(failure_entries):
        missing = sorted(set(failure_entries) - seen_failures)
        raise M60dError(f"global scan missed {len(missing)} current failure(s)")

    focus_rows.sort(key=lambda row: row["case_hash"])
    if len(focus_rows) != 20214:
        raise M60dError(
            f"focused evidence row count changed: expected=20214 "
            f"actual={len(focus_rows)}"
        )
    if len({row["case_hash"] for row in focus_rows}) != len(focus_rows):
        raise M60dError("focused evidence contains duplicate hashes")
    by_form: dict[str, list[dict[str, Any]]] = defaultdict(list)
    by_hash = {row["case_hash"]: row for row in focus_rows}
    for row in focus_rows:
        by_form[row["form"]].append(row)
    for form in ("CC", "CD", "CE"):
        if len(by_form[form]) != 5000:
            raise M60dError(f"{form}: focused population changed")
        if any(row["architectural_outcome"] != "pass" for row in by_form[form]):
            raise M60dError(f"{form}: approved green population regressed")
    taken_ce = {
        row["case_hash"] for row in by_form["CE"] if row["expected_event"]["active"]
    }
    not_taken_ce = {
        row["case_hash"]
        for row in by_form["CE"]
        if not row["expected_event"]["active"]
    }
    if (
        taken_ce != populations["ce_taken"]
        or not_taken_ce != populations["ce_not_taken"]
    ):
        raise M60dError("current CE partition differs from M59")
    primary_now = {
        row["case_hash"]
        for row in focus_rows
        if row["form"] in {"CC", "CD", "CE"} and row["expected_event"]["active"]
    }
    if primary_now != populations["primary"]:
        raise M60dError("current primary frame population differs from M59")
    if any(by_hash[item]["frame_residual_reasons"] for item in primary_now):
        raise M60dError("primary synchronous frame residual remains")

    bound_rows = by_form["62"]
    bound_pass = {
        row["case_hash"]
        for row in bound_rows
        if row["architectural_outcome"] == "pass"
    }
    bound_fail = {
        row["case_hash"]
        for row in bound_rows
        if row["architectural_outcome"] == "fail"
    }
    if bound_pass != populations["bound_frame"] | populations["bound_normal"]:
        raise M60dError("post-M60a BOUND passing population changed")
    if bound_fail != populations["bound_range"]:
        raise M60dError("post-M60a BOUND residual population changed")
    if any(
        by_hash[item]["frame_residual_reasons"]
        for item in populations["bound_frame"]
    ):
        raise M60dError(
            "former BOUND frame-only population still has a frame residual"
        )

    dependency_hashes: list[str] = []
    for form, (expected_count, expected_digest) in DIV_DEPENDENCY_FORMS.items():
        hashes = sorted(div_dependency[form])
        if (
            len(hashes) != expected_count
            or ratchet.hash_set_digest(hashes) != expected_digest
        ):
            raise M60dError(f"{form}: exact divide dependency changed")
        dependency_hashes.extend(hashes)
        for record_hash in hashes:
            row = by_hash[record_hash]
            if row["architectural_outcome"] != "pass":
                raise M60dError(f"{form}: M60a divide dependency regressed")
            if row["frame_residual_reasons"]:
                raise M60dError(
                    f"{form}: divide dependency has a frame residual"
                )
    dependency_hashes.sort()
    if len(dependency_hashes) != DIV_DEPENDENCY_COUNT:
        raise M60dError("combined divide dependency count changed")

    residual_rows = sorted(
        (
            row
            for row in global_event_rows
            if row["frame_residual_reasons"]
            and row["expected_event"]["active"]
            and row["actual_event"]["active"]
            and row["expected_event"]["vector"] == row["actual_event"]["vector"]
        ),
        key=lambda row: row["case_hash"],
    )
    if residual_rows:
        forms = sorted(Counter(row["form"] for row in residual_rows).items())
        raise M60dError(
            "evidence-proven synchronous frame residual requires Path B: "
            f"count={len(residual_rows)} forms={forms}"
        )
    return {
        "bound_frame": populations["bound_frame"],
        "bound_normal": populations["bound_normal"],
        "bound_range": populations["bound_range"],
        "div_dependency_hashes": dependency_hashes,
        "focus_rows": focus_rows,
        "global_event_rows": sorted(
            global_event_rows, key=lambda row: row["case_hash"]
        ),
        "residual_rows": residual_rows,
    }


def write_scoreboard(
    root: pathlib.Path,
    output_root: pathlib.Path,
    raw_path: pathlib.Path,
    profile_key: str,
    evaluated_sha: str,
) -> dict[str, Any]:
    require_sha(evaluated_sha, "evaluated_sha", HEX40)
    _, failures_raw, approved = verify_raw_profile(root, raw_path, profile_key)
    profile, scope = profile_key.rsplit("_", 1)
    directory_relative = FAILURE_DIRECTORY_PATHS[profile_key]
    shards, failure_index, canonical_set, raw_set = ratchet.write_failure_shards(
        failures_raw,
        profile,
        scope,
        DATASET_ID,
        output_path(output_root, directory_relative),
    )
    expected = PROFILE_IDENTITIES[profile_key]
    if failure_index != expected["failure_signature_index_sha256"]:
        raise M60dError(f"{profile_key}: regenerated failure index changed")
    value = copy.deepcopy(approved)
    value.update(
        {
            "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
            "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
            "epoch_gate": CANDIDATE_GATE,
            "evaluated_sha": evaluated_sha,
            "failure_shards": shards,
            "failure_sidecar_canonical_set_sha256": canonical_set,
            "failure_sidecar_raw_set_sha256": raw_set,
            "raw_result_summary_sha256": sha256_file(raw_path),
        }
    )
    if (
        value["selected_hash_set_sha256"] != SELECTED_HASH_SETS[scope]
        or value["applicable_hash_set_sha256"] != APPLICABLE_HASH_SETS[scope]
        or value["pass_hash_set_sha256"] != expected["pass_hash_set_sha256"]
        or value["failure_hash_set_sha256"]
        != expected["failure_hash_set_sha256"]
        or value["target_policy_id"] != TARGET_POLICY_ID
        or value["target_policy_sha256"] != TARGET_POLICY_SHA256
    ):
        raise M60dError(f"{profile_key}: approved scoreboard identity changed")
    write_json(output_path(output_root, SCOREBOARD_PATHS[profile_key]), value)
    return value


def write_transition(
    output_root: pathlib.Path,
    profile_key: str,
    scoreboard: dict[str, Any],
    evaluated_sha: str,
) -> dict[str, Any]:
    transition = {
        "applicable_hash_set_sha256": scoreboard["applicable_hash_set_sha256"],
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "changed_failure_count": 0,
        "changed_failure_shards": [],
        "comparison_contract_ids": {
            scoreboard["profile"]: {
                "id": scoreboard["comparison_contract_id"],
                "sha256": scoreboard["comparison_contract_sha256"],
            }
        },
        "dataset_id": DATASET_ID,
        "epoch_gate": CANDIDATE_GATE,
        "evaluated_sha": evaluated_sha,
        "gap_kind_changes": [],
        "hardware_pending_changes": [],
        "m60d_outcome": "evidence_only_closure",
        "newly_failing": [],
        "newly_passing": [],
        "profile": scoreboard["profile"],
        "residual_frame_count_after": 0,
        "residual_frame_count_before": 0,
        "schema": "vaeg-upd9002-m60d-frame-transition-v1",
        "schema_version": 1,
        "scope": scoreboard["scope"],
        "selected_hash_set_sha256": scoreboard["selected_hash_set_sha256"],
        "semantic_change": False,
        "target_policy_id": TARGET_POLICY_ID,
        "top_level_classification_changes": [],
        "transition_kind": "synchronous_frame_residual",
    }
    write_json(output_path(output_root, TRANSITION_PATHS[profile_key]), transition)
    return transition


def write_evidence(
    root: pathlib.Path,
    output_root: pathlib.Path,
    audit: dict[str, Any],
    evaluated_sha: str,
    scoreboards: dict[str, dict[str, Any]],
) -> tuple[dict[str, Any], dict[str, Any]]:
    rows = audit["focus_rows"]
    by_form: dict[str, list[dict[str, Any]]] = defaultdict(list)
    by_hash = {row["case_hash"]: row for row in rows}
    for row in rows:
        by_form[row["form"]].append(row)
    evidence_files: list[pathlib.Path] = []

    table = {
        "row_count": len(rows),
        "rows": rows,
        "schema": "vaeg-upd9002-m60d-synchronous-frame-cases-v1",
        "schema_version": 1,
    }
    table_path = EVIDENCE_ROOT / "synchronous_frame_cases.json.gz"
    ratchet.write_deterministic_gzip(output_path(output_root, table_path), table)
    evidence_files.append(table_path)
    summaries = {
        "cc_summary.json": summarize_rows(by_form["CC"], "INT3"),
        "cd_summary.json": summarize_rows(by_form["CD"], "INT immediate"),
        "ce_summary.json": {
            **summarize_rows(by_form["CE"], "INTO"),
            "non_taken_count": sum(
                not row["expected_event"]["active"] for row in by_form["CE"]
            ),
            "taken_count": sum(
                row["expected_event"]["active"] for row in by_form["CE"]
            ),
        },
        "bound_frame_only_summary.json": {
            **summarize_rows(
                [by_hash[item] for item in sorted(audit["bound_frame"])],
                "BOUND former frame-only",
            ),
            "approved_sha256": BOUND_FRAME_SHA256,
        },
        "bound_range_residual_summary.json": {
            **summarize_rows(
                [by_hash[item] for item in sorted(audit["bound_range"])],
                "BOUND range residual",
            ),
            "approved_sha256": BOUND_RANGE_SHA256,
        },
        "divide_exception_dependency_summary.json": {
            **summarize_rows(
                [
                    by_hash[item]
                    for item in sorted(audit["div_dependency_hashes"])
                ],
                "M60a divide-exception dependency",
            ),
            "per_form": [
                {
                    "count": len(
                        [
                            item
                            for item in audit["div_dependency_hashes"]
                            if by_hash[item]["form"] == form
                        ]
                    ),
                    "form": form,
                    "hash_set_sha256": ratchet.hash_set_digest(
                        [
                            item
                            for item in audit["div_dependency_hashes"]
                            if by_hash[item]["form"] == form
                        ]
                    ),
                }
                for form in sorted(DIV_FORMS)
            ],
        },
    }
    for name, value in summaries.items():
        relative = EVIDENCE_ROOT / name
        write_json(output_path(output_root, relative), value)
        evidence_files.append(relative)

    global_scan = {
        "event_related_failure_count": len(audit["global_event_rows"]),
        "event_related_failure_hashes_sha256": ratchet.hash_set_digest(
            [row["case_hash"] for row in audit["global_event_rows"]]
        ),
        "event_related_forms": dict(
            sorted(
                Counter(
                    row["form"] for row in audit["global_event_rows"]
                ).items()
            )
        ),
        "residual_frame_count": len(audit["residual_rows"]),
        "residual_frame_hashes": [
            row["case_hash"] for row in audit["residual_rows"]
        ],
        "residual_frame_hashes_sha256": ratchet.hash_set_digest(
            [row["case_hash"] for row in audit["residual_rows"]]
        ),
        "scanned_architectural_failure_count": PROFILE_IDENTITIES[
            "architectural_full"
        ]["fail"],
        "schema": "vaeg-upd9002-m60d-global-frame-signature-scan-v1",
        "schema_version": 1,
    }
    global_path = EVIDENCE_ROOT / "global_frame_signature_scan.json"
    write_json(output_path(output_root, global_path), global_scan)
    evidence_files.append(global_path)
    audit_summary = {
        "bound": {
            "frame_only_count": len(audit["bound_frame"]),
            "frame_only_sha256": ratchet.hash_set_digest(audit["bound_frame"]),
            "normal_count": len(audit["bound_normal"]),
            "normal_sha256": ratchet.hash_set_digest(audit["bound_normal"]),
            "range_residual_count": len(audit["bound_range"]),
            "range_residual_sha256": ratchet.hash_set_digest(audit["bound_range"]),
        },
        "candidate_gate": CANDIDATE_GATE,
        "divide_exception_dependency": {
            "count": len(audit["div_dependency_hashes"]),
            "sha256": ratchet.hash_set_digest(audit["div_dependency_hashes"]),
        },
        "m60d_outcome": "evidence_only_closure",
        "milestone": MILESTONE,
        "primary_frame_population": {
            "count": PRIMARY_FRAME_COUNT,
            "sha256": PRIMARY_FRAME_SHA256,
        },
        "residual_frame_count": 0,
        "residual_frame_hashes_sha256": EMPTY_HASH_SET_SHA256,
        "schema": "vaeg-upd9002-m60d-synchronous-frame-audit-v1",
        "schema_version": 1,
        "semantic_change": False,
    }
    audit_path = EVIDENCE_ROOT / "synchronous_frame_audit.json"
    write_json(output_path(output_root, audit_path), audit_summary)
    evidence_files.append(audit_path)

    reps = EVIDENCE_ROOT / "representative"
    rep_specs = [
        (
            reps / "int3.md",
            "M60d INT3 frame representatives",
            by_form["CC"],
            [
                (
                    "ordinary frame",
                    lambda row: row["frame"]["boundary_partition"] == "no-boundary",
                ),
                (
                    "segment-wrap frame",
                    lambda row: row["frame"]["boundary_partition"]
                    == "segment-boundary",
                ),
                (
                    "physical-wrap frame",
                    lambda row: row["frame"]["boundary_partition"]
                    == "physical-boundary",
                ),
            ],
        ),
        (
            reps / "int_imm8.md",
            "M60d INT immediate frame representatives",
            by_form["CD"],
            [
                (
                    "ordinary frame",
                    lambda row: row["frame"]["boundary_partition"] == "no-boundary",
                ),
                (
                    "physical-wrap frame",
                    lambda row: row["frame"]["boundary_partition"]
                    == "physical-boundary",
                ),
            ],
        ),
        (
            reps / "into.md",
            "M60d INTO representatives",
            by_form["CE"],
            [
                ("OF clear", lambda row: not row["expected_event"]["active"]),
                ("OF set", lambda row: row["expected_event"]["active"]),
                (
                    "physical-wrap frame",
                    lambda row: row["frame"] is not None
                    and row["frame"]["boundary_partition"] == "physical-boundary",
                ),
            ],
        ),
        (
            reps / "bound.md",
            "M60d BOUND representatives",
            by_form["62"],
            [
                (
                    "former frame-only case",
                    lambda row: row["case_hash"] in audit["bound_frame"],
                ),
                (
                    "range residual",
                    lambda row: row["case_hash"] in audit["bound_range"],
                ),
                (
                    "normal completion",
                    lambda row: row["case_hash"] in audit["bound_normal"],
                ),
            ],
        ),
        (
            reps / "divide_exception.md",
            "M60d divide-exception representatives",
            [
                by_hash[item]
                for item in sorted(audit["div_dependency_hashes"])
            ],
            [
                (form, lambda row, selected=form: row["form"] == selected)
                for form in sorted(DIV_FORMS)
            ],
        ),
    ]
    for relative, title, selected_rows, selectors in rep_specs:
        write_representative(
            output_path(output_root, relative),
            title,
            selected_rows,
            selectors,
        )
        evidence_files.append(relative)

    for key in ("architectural_ci", "architectural_full"):
        write_transition(output_root, key, scoreboards[key], evaluated_sha)
    artifact_paths = list(evidence_files)
    artifact_paths.extend(SCOREBOARD_PATHS.values())
    for key in SCOREBOARD_PATHS:
        artifact_paths.extend(
            path.relative_to(output_root)
            for path in sorted(
                output_path(output_root, FAILURE_DIRECTORY_PATHS[key]).glob(
                    "*.json.gz"
                )
            )
        )
    artifact_paths.extend(TRANSITION_PATHS.values())
    artifact_paths = sorted(set(artifact_paths), key=lambda path: path.as_posix())
    artifacts = [
        artifact_entry(output_path(output_root, relative), relative)
        for relative in artifact_paths
    ]
    artifact_tree_sha256 = sha256_bytes(canonical_bytes(artifacts))
    manifest = {
        "applicable_hash_set_sha256": APPLICABLE_HASH_SETS,
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "artifact_tree_sha256": artifact_tree_sha256,
        "artifacts": artifacts,
        "candidate_gate": CANDIDATE_GATE,
        "comparison_contracts": CONTRACTS,
        "dataset_id": DATASET_ID,
        "divide_exception_dependency_count": DIV_DEPENDENCY_COUNT,
        "divide_exception_dependency_sha256": ratchet.hash_set_digest(
            audit["div_dependency_hashes"]
        ),
        "environment": {
            "gzip_module": gzip.__name__,
            "platform": platform.platform(),
            "python": platform.python_version(),
            "zlib": zlib.ZLIB_VERSION,
        },
        "evaluated_sha": evaluated_sha,
        "generator": {
            "path": "tools/qa/upd9002_m60d_frame_audit.py",
            "schema_version": 1,
            "sha256": sha256_file(
                root / "tools/qa/upd9002_m60d_frame_audit.py"
            ),
        },
        "m60d_outcome": "evidence_only_closure",
        "milestone": MILESTONE,
        "primary_frame_population_count": PRIMARY_FRAME_COUNT,
        "primary_frame_population_sha256": PRIMARY_FRAME_SHA256,
        "residual_frame_count": 0,
        "residual_frame_hashes_sha256": EMPTY_HASH_SET_SHA256,
        "schema": "vaeg-upd9002-m60d-evidence-manifest-v1",
        "schema_version": 1,
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "semantic_change": False,
        "target_policy_id": TARGET_POLICY_ID,
    }
    manifest_path = EVIDENCE_ROOT / "manifest.json"
    write_json(output_path(output_root, manifest_path), manifest)
    result = {
        "analysis_evaluated_sha": evaluated_sha,
        "artifact_tree_sha256": artifact_tree_sha256,
        "candidate_gate": CANDIDATE_GATE,
        "evidence_manifest_sha256": sha256_file(
            output_path(output_root, manifest_path)
        ),
        "m60d_outcome": "evidence_only_closure",
        "profile_identities": {
            key: {
                field: scoreboards[key][field]
                for field in (
                    "applicable",
                    "applicable_hash_set_sha256",
                    "comparison_contract_id",
                    "comparison_contract_sha256",
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
        "schema": "vaeg-upd9002-m60d-result-manifest-v1",
        "schema_version": 1,
        "semantic_change": False,
        "transition_sha256": {
            key: sha256_file(output_path(output_root, TRANSITION_PATHS[key]))
            for key in TRANSITION_PATHS
        },
    }
    write_json(output_path(output_root, RESULT_MANIFEST_PATH), result)
    return manifest, result


def generate(
    root: pathlib.Path,
    output_root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    architectural_ci_raw: pathlib.Path,
    architectural_full_raw: pathlib.Path,
    fingerprint_full_raw: pathlib.Path,
    evaluated_sha: str,
) -> None:
    require_sha(evaluated_sha, "evaluated_sha", HEX40)
    if evaluated_sha == APPROVED_PREDECESSOR_SHA:
        raise M60dError("current-worktree self-comparison is forbidden")
    verify_upstream_static(root)
    scoreboards = {
        "architectural_ci": write_scoreboard(
            root,
            output_root,
            architectural_ci_raw,
            "architectural_ci",
            evaluated_sha,
        ),
        "architectural_full": write_scoreboard(
            root,
            output_root,
            architectural_full_raw,
            "architectural_full",
            evaluated_sha,
        ),
        "fingerprint_full": write_scoreboard(
            root,
            output_root,
            fingerprint_full_raw,
            "fingerprint_full",
            evaluated_sha,
        ),
    }
    audit = run_audit(root, dataset_root, worker, architectural_full_raw)
    manifest, result = write_evidence(
        root, output_root, audit, evaluated_sha, scoreboards
    )
    print(
        "m60d-generate: outcome=evidence_only_closure semantic_change=false "
        f"rows={len(audit['focus_rows'])} residual=0 "
        f"manifest={result['evidence_manifest_sha256']} "
        f"artifact_tree={manifest['artifact_tree_sha256']}"
    )


def validate_manifest(root: pathlib.Path) -> None:
    manifest_path = root / EVIDENCE_ROOT / "manifest.json"
    manifest = read_json(manifest_path)
    if (
        manifest.get("schema") != "vaeg-upd9002-m60d-evidence-manifest-v1"
        or manifest.get("schema_version") != 1
        or manifest.get("milestone") != MILESTONE
        or manifest.get("candidate_gate") != CANDIDATE_GATE
        or manifest.get("approved_predecessor_sha") != APPROVED_PREDECESSOR_SHA
        or manifest.get("target_policy_id") != TARGET_POLICY_ID
        or manifest.get("dataset_id") != DATASET_ID
        or manifest.get("selected_hash_set_sha256") != SELECTED_HASH_SETS
        or manifest.get("applicable_hash_set_sha256") != APPLICABLE_HASH_SETS
        or manifest.get("m60d_outcome") != "evidence_only_closure"
        or manifest.get("semantic_change") is not False
        or manifest.get("residual_frame_count") != 0
        or manifest.get("residual_frame_hashes_sha256")
        != EMPTY_HASH_SET_SHA256
    ):
        raise M60dError("G60d evidence manifest identity changed")
    require_sha(manifest.get("evaluated_sha"), "manifest evaluated_sha", HEX40)
    artifacts = manifest.get("artifacts")
    if not isinstance(artifacts, list) or not artifacts:
        raise M60dError("G60d evidence manifest lacks artifacts")
    if artifacts != sorted(artifacts, key=lambda item: item["path"]):
        raise M60dError("G60d artifact order is nondeterministic")
    seen = set()
    for entry in artifacts:
        if not isinstance(entry, dict) or set(entry) != {
            "bytes",
            "path",
            "row_count",
            "sha256",
        }:
            raise M60dError("G60d artifact entry schema changed")
        relative = pathlib.Path(entry["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise M60dError("G60d artifact path escaped repository")
        if relative.as_posix() in seen:
            raise M60dError("G60d artifact path duplicated")
        seen.add(relative.as_posix())
        path = root / relative
        if (
            not path.is_file()
            or path.stat().st_size != entry["bytes"]
            or sha256_file(path) != entry["sha256"]
        ):
            raise M60dError(f"G60d artifact digest mismatch: {relative}")
    if (
        sha256_bytes(canonical_bytes(artifacts))
        != manifest["artifact_tree_sha256"]
    ):
        raise M60dError("G60d artifact-tree digest mismatch")
    table = load_table(root / EVIDENCE_ROOT / "synchronous_frame_cases.json.gz")
    hashes = [row.get("case_hash") for row in table]
    if (
        len(table) != 20214
        or hashes != sorted(hashes)
        or len(hashes) != len(set(hashes))
        or not all(
            isinstance(item, str) and HEX64.fullmatch(item) for item in hashes
        )
    ):
        raise M60dError("G60d machine table coverage/order changed")
    if any(row.get("frame_residual_reasons") for row in table):
        raise M60dError("G60d evidence-only closure contains a residual")
    result = read_json(root / RESULT_MANIFEST_PATH)
    if (
        result.get("schema") != "vaeg-upd9002-m60d-result-manifest-v1"
        or result.get("analysis_evaluated_sha") != manifest["evaluated_sha"]
        or result.get("artifact_tree_sha256")
        != manifest["artifact_tree_sha256"]
        or result.get("evidence_manifest_sha256") != sha256_file(manifest_path)
    ):
        raise M60dError("G60d result manifest identity changed")
    for key, path in SCOREBOARD_PATHS.items():
        value = read_json(root / path)
        expected = PROFILE_IDENTITIES[key]
        scope = key.rsplit("_", 1)[1]
        if (
            value.get("epoch_gate") != CANDIDATE_GATE
            or value.get("approved_predecessor_gate")
            != APPROVED_PREDECESSOR_GATE
            or value.get("approved_predecessor_sha") != APPROVED_PREDECESSOR_SHA
            or value.get("evaluated_sha") != manifest["evaluated_sha"]
            or value.get("target_policy_id") != TARGET_POLICY_ID
            or value.get("selected_hash_set_sha256") != SELECTED_HASH_SETS[scope]
            or value.get("applicable_hash_set_sha256")
            != APPLICABLE_HASH_SETS[scope]
            or value.get("pass") != expected["pass"]
            or value.get("fail") != expected["fail"]
            or value.get("pass_hash_set_sha256")
            != expected["pass_hash_set_sha256"]
            or value.get("failure_hash_set_sha256")
            != expected["failure_hash_set_sha256"]
            or value.get("failure_signature_index_sha256")
            != expected["failure_signature_index_sha256"]
            or value.get("timeouts") != 0
            or value.get("crashes") != 0
        ):
            raise M60dError(f"{key}: committed scoreboard identity changed")
    for key, path in TRANSITION_PATHS.items():
        transition = read_json(root / path)
        if (
            transition.get("before_sha") != APPROVED_PREDECESSOR_SHA
            or transition.get("evaluated_sha") != manifest["evaluated_sha"]
            or transition.get("m60d_outcome") != "evidence_only_closure"
            or transition.get("semantic_change") is not False
            or transition.get("newly_passing") != []
            or transition.get("newly_failing") != []
            or transition.get("changed_failure_count") != 0
            or transition.get("top_level_classification_changes") != []
            or transition.get("gap_kind_changes") != []
            or transition.get("hardware_pending_changes") != []
        ):
            raise M60dError(f"{key}: committed transition changed")


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
        "tests/ssts/authority/g60c",
        "tests/ssts/authority/g60b_result_manifest.json",
        "tests/ssts/authority/g60c_result_manifest.json",
        "tools/qa/golden/upd9002_support_map_m48.csv",
    ]
    if protected_evidence_only:
        protected.extend(
            path.relative_to(root).as_posix()
            for path in (root / "tests/ssts/target_policy").glob("g60b*")
        )
    else:
        protected.extend(["tests/ssts/target_policy", "cpu/upd9002"])
    completed = subprocess.run(
        [
            "git",
            "diff",
            "--exit-code",
            f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD",
            "--",
            *protected,
        ],
        cwd=root,
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode != 0:
        raise M60dError(
            "production semantics, policy, fixtures, or protected evidence changed"
        )
    scoreboards = [
        path.relative_to(root).as_posix()
        for path in (root / "tests/ssts/scoreboard").glob("*")
        if path.name.startswith(("g58", "g60a", "g60b", "g60c"))
    ]
    if scoreboards:
        completed = subprocess.run(
            [
                "git",
                "diff",
                "--exit-code",
                f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD",
                "--",
                *scoreboards,
            ],
            cwd=root,
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
        if completed.returncode != 0:
            raise M60dError("approved scoreboard evidence changed")
    transitions = [
        path.relative_to(root).as_posix()
        for path in (root / "tests/ssts/transitions").glob("*")
        if path.name.startswith(("g58", "g60a", "g60b", "g60c"))
    ]
    if transitions:
        completed = subprocess.run(
            [
                "git",
                "diff",
                "--exit-code",
                f"{APPROVED_PREDECESSOR_GIT_SHA}...HEAD",
                "--",
                *transitions,
            ],
            cwd=root,
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
        if completed.returncode != 0:
            raise M60dError("approved transition evidence changed")


def validate_evidence_commit_paths(paths: Iterable[str]) -> None:
    unexpected = [
        path
        for path in paths
        if not any(
            path == prefix or path.startswith(prefix)
            for prefix in EVIDENCE_COMMIT_ALLOWED_PATHS
        )
    ]
    if unexpected:
        raise M60dError(
            f"{EVIDENCE_COMMIT_SCOPE_ERROR}: evidence commit contains "
            f"implementation changes: {unexpected}"
        )


def find_evidence_commit(root: pathlib.Path) -> str:
    completed = subprocess.run(
        [
            "git",
            "log",
            "--format=%H",
            "--diff-filter=A",
            "--",
            EVIDENCE_MANIFEST_PATH.as_posix(),
        ],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise M60dError(
            f"{EVIDENCE_COMMIT_IDENTITY_ERROR}: cannot resolve evidence commit"
        )
    commits = completed.stdout.splitlines()
    if len(commits) != 1 or HEX40.fullmatch(commits[0]) is None:
        raise M60dError(
            f"{EVIDENCE_COMMIT_IDENTITY_ERROR}: expected one manifest-adding "
            f"commit, found {len(commits)}"
        )
    evidence_commit = commits[0]
    ancestry = subprocess.run(
        ["git", "merge-base", "--is-ancestor", evidence_commit, "HEAD"],
        cwd=root,
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    if ancestry.returncode != 0:
        raise M60dError(
            f"{EVIDENCE_COMMIT_IDENTITY_ERROR}: evidence commit is not an "
            "ancestor of HEAD"
        )
    parent = subprocess.run(
        ["git", "rev-parse", f"{evidence_commit}^"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    manifest = read_json(root / EVIDENCE_MANIFEST_PATH)
    if (
        parent.returncode != 0
        or HEX40.fullmatch(parent.stdout.strip()) is None
        or parent.stdout.strip() != manifest.get("evaluated_sha")
    ):
        raise M60dError(
            f"{EVIDENCE_COMMIT_IDENTITY_ERROR}: evidence commit parent does "
            "not match evaluated_sha"
        )
    return evidence_commit


def validate_evidence_commit_scope(root: pathlib.Path) -> None:
    evidence_commit = find_evidence_commit(root)
    completed = subprocess.run(
        [
            "git",
            "diff-tree",
            "--no-commit-id",
            "--name-only",
            "-r",
            evidence_commit,
        ],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0:
        raise M60dError(
            f"{EVIDENCE_COMMIT_SCOPE_ERROR}: cannot inspect evidence commit"
        )
    validate_evidence_commit_paths(completed.stdout.splitlines())


def verify_static(
    root: pathlib.Path, protected_evidence_only: bool = False
) -> None:
    forward_milestone = (
        root / "docs/agents/tasks/M62_upd9002_semantics_bundle.md"
    ).is_file()
    protected_evidence_only = protected_evidence_only or forward_milestone
    verify_upstream_static(root, protected_evidence_only)
    verify_protected_paths(root, protected_evidence_only)
    family = [
        (root / EVIDENCE_ROOT / "manifest.json").is_file(),
        (root / RESULT_MANIFEST_PATH).is_file(),
        all((root / path).is_file() for path in SCOREBOARD_PATHS.values()),
        all((root / path).is_file() for path in TRANSITION_PATHS.values()),
    ]
    if any(family) and not all(family):
        raise M60dError("G60d evidence family is incomplete")
    if all(family):
        validate_manifest(root)
        if not protected_evidence_only:
            validate_evidence_commit_scope(root)
        scope = (
            "protected evidence"
            if protected_evidence_only
            else "protected inputs and final evidence-only commit"
        )
        print(
            f"m60d-static: Path A evidence-only closure, {scope}, "
            "scoreboards, and transitions passed"
        )
    else:
        print(
            "m60d-static: implementation-only tree; G60c identity, protected "
            "inputs, target policy, and cpu/upd9002 passed"
        )


def synthetic_decision() -> dict[str, Any]:
    return {
        "applicable_hash_set_sha256": APPLICABLE_HASH_SETS,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "bound_frame_count": BOUND_FRAME_COUNT,
        "bound_frame_sha256": BOUND_FRAME_SHA256,
        "bound_overlap_count": 0,
        "bound_range_count": BOUND_RANGE_COUNT,
        "bound_range_sha256": BOUND_RANGE_SHA256,
        "changed_failure_count": 0,
        "comparison_contracts": CONTRACTS,
        "divide_dependency_count": DIV_DEPENDENCY_COUNT,
        "newly_failing": [],
        "newly_passing": [],
        "primary_frame_count": PRIMARY_FRAME_COUNT,
        "primary_frame_sha256": PRIMARY_FRAME_SHA256,
        "residual_frame_count": 0,
        "residual_frame_hashes": [],
        "selected_hash_set_sha256": SELECTED_HASH_SETS,
        "semantic_change": False,
        "target_policy_id": TARGET_POLICY_ID,
        "taxonomy_changes": [],
        "top_level_classification_changes": [],
    }


def validate_decision(value: Any) -> None:
    if not isinstance(value, dict) or set(value) != set(synthetic_decision()):
        raise M60dError("decision schema changed")
    if value["approved_predecessor_sha"] != APPROVED_PREDECESSOR_SHA:
        raise M60dError("wrong G60c predecessor SHA")
    if value["target_policy_id"] != TARGET_POLICY_ID:
        raise M60dError("wrong target-policy ID")
    if value["selected_hash_set_sha256"] != SELECTED_HASH_SETS:
        raise M60dError("selected set drift")
    if value["applicable_hash_set_sha256"] != APPLICABLE_HASH_SETS:
        raise M60dError("applicable set drift")
    if value["comparison_contracts"] != CONTRACTS:
        raise M60dError("comparison contract drift")
    if value["top_level_classification_changes"] or value["taxonomy_changes"]:
        raise M60dError("classification or taxonomy change")
    if (
        value["primary_frame_count"] != PRIMARY_FRAME_COUNT
        or value["primary_frame_sha256"] != PRIMARY_FRAME_SHA256
    ):
        raise M60dError("primary frame coverage changed")
    if (
        value["bound_frame_count"] != BOUND_FRAME_COUNT
        or value["bound_frame_sha256"] != BOUND_FRAME_SHA256
        or value["bound_range_count"] != BOUND_RANGE_COUNT
        or value["bound_range_sha256"] != BOUND_RANGE_SHA256
        or value["bound_overlap_count"] != 0
    ):
        raise M60dError("BOUND partition changed")
    if value["divide_dependency_count"] != DIV_DEPENDENCY_COUNT:
        raise M60dError("divide dependency derivation changed")
    if value["newly_failing"]:
        raise M60dError("new failure")
    if value["changed_failure_count"]:
        raise M60dError("changed failure not enumerated")
    if value["semantic_change"]:
        if not value["residual_frame_hashes"]:
            raise M60dError("semantic edit lacks exact residual ownership")
    elif value["residual_frame_count"] or value["residual_frame_hashes"]:
        raise M60dError("Path A contains a residual or semantic edit")
    if value["newly_passing"]:
        raise M60dError("Path A contains newly passing hashes")


def expect_rejection(label: str, callback: Callable[[], None]) -> None:
    try:
        callback()
    except M60dError:
        return
    raise AssertionError(f"selftest did not reject {label}")


def selftest() -> None:
    valid = synthetic_decision()
    validate_decision(valid)
    mutations: list[tuple[str, Callable[[dict[str, Any]], None]]] = [
        (
            "wrong predecessor",
            lambda value: value.__setitem__("approved_predecessor_sha", "0" * 40),
        ),
        (
            "wrong target policy",
            lambda value: value.__setitem__("target_policy_id", "wrong"),
        ),
        (
            "selected drift",
            lambda value: value["selected_hash_set_sha256"].__setitem__(
                "ci", "0" * 64
            ),
        ),
        (
            "applicable drift",
            lambda value: value["applicable_hash_set_sha256"].__setitem__(
                "full", "0" * 64
            ),
        ),
        (
            "contract drift",
            lambda value: value["comparison_contracts"][
                "architectural"
            ].__setitem__("id", "wrong"),
        ),
        (
            "classification change",
            lambda value: value["top_level_classification_changes"].append("x"),
        ),
        (
            "taxonomy change",
            lambda value: value["taxonomy_changes"].append("x"),
        ),
        (
            "missing primary coverage",
            lambda value: value.__setitem__("primary_frame_count", 12467),
        ),
        (
            "wrong primary digest",
            lambda value: value.__setitem__("primary_frame_sha256", "0" * 64),
        ),
        (
            "missing BOUND frame hash",
            lambda value: value.__setitem__("bound_frame_count", 3564),
        ),
        (
            "extra BOUND frame hash",
            lambda value: value.__setitem__("bound_frame_count", 3566),
        ),
        (
            "BOUND frame digest",
            lambda value: value.__setitem__("bound_frame_sha256", "0" * 64),
        ),
        (
            "BOUND range digest",
            lambda value: value.__setitem__("bound_range_sha256", "0" * 64),
        ),
        (
            "BOUND overlap",
            lambda value: value.__setitem__("bound_overlap_count", 1),
        ),
        (
            "missing divide derivation",
            lambda value: value.__setitem__("divide_dependency_count", 213),
        ),
        (
            "new failure",
            lambda value: value["newly_failing"].append("0" * 64),
        ),
        (
            "new passing under Path A",
            lambda value: value["newly_passing"].append("0" * 64),
        ),
        (
            "changed failure",
            lambda value: value.__setitem__("changed_failure_count", 1),
        ),
        (
            "Path A semantic edit",
            lambda value: value.__setitem__("semantic_change", True),
        ),
        (
            "Path A residual",
            lambda value: value.__setitem__("residual_frame_count", 1),
        ),
    ]
    for label, mutation in mutations:
        candidate = copy.deepcopy(valid)
        mutation(candidate)
        expect_rejection(label, lambda value=candidate: validate_decision(value))
    with tempfile.TemporaryDirectory(prefix="vaeg-m60d-selftest-") as temp:
        root = pathlib.Path(temp)
        payload = {
            "row_count": 1,
            "rows": [{"case_hash": "0" * 64}],
            "schema": "synthetic",
            "schema_version": 1,
        }
        first = root / "first.json.gz"
        second = root / "second.json.gz"
        ratchet.write_deterministic_gzip(first, payload)
        ratchet.write_deterministic_gzip(second, payload)
        if first.read_bytes() != second.read_bytes():
            raise AssertionError("deterministic gzip selftest failed")
        if canonical_bytes(payload) != canonical_bytes(copy.deepcopy(payload)):
            raise AssertionError("canonical JSON selftest failed")
    disallowed = [
        "cpu/upd9002/instr.c",
        "tests/ssts/gap_taxonomy.json",
        "tests/ssts/hardware_pending.json",
        "tests/ssts/transitions/g60c_target_authority_from_g60b.json",
        "tools/qa/upd9002_m60d_frame_audit.py",
    ]
    validate_evidence_commit_paths(
        [
            "docs/agents/reports/m60d_upd9002_interrupt_frame.md",
            "tests/ssts/evidence/g60d/manifest.json",
            "tests/ssts/evidence/g60d_result_manifest.json",
            "tests/ssts/scoreboard/g60d_architectural_ci.json",
            "tests/ssts/transitions/g60d_architectural_ci_from_g60c.json",
        ]
    )
    for path in disallowed:
        try:
            validate_evidence_commit_paths([path])
        except M60dError as error:
            if not str(error).startswith(f"{EVIDENCE_COMMIT_SCOPE_ERROR}:"):
                raise AssertionError(
                    f"wrong evidence-scope rejection for {path}: {error}"
                ) from error
        else:
            raise AssertionError(f"evidence commit allowlist admitted {path}")
    print(
        "m60d-selftest: 3 positive and "
        f"{len(mutations) + len(disallowed)} fail-closed checks passed"
    )


def deterministic_regeneration(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    architectural_ci_raw: pathlib.Path,
    architectural_full_raw: pathlib.Path,
    fingerprint_full_raw: pathlib.Path,
    evaluated_sha: str,
) -> None:
    with tempfile.TemporaryDirectory(prefix="vaeg-m60d-regenerate-a-") as first:
        with tempfile.TemporaryDirectory(
            prefix="vaeg-m60d-regenerate-b-"
        ) as second:
            first_root = pathlib.Path(first)
            second_root = pathlib.Path(second)
            generate(
                root,
                first_root,
                dataset_root,
                worker,
                architectural_ci_raw,
                architectural_full_raw,
                fingerprint_full_raw,
                evaluated_sha,
            )
            generate(
                root,
                second_root,
                dataset_root,
                worker,
                architectural_ci_raw,
                architectural_full_raw,
                fingerprint_full_raw,
                evaluated_sha,
            )
            first_files = sorted(
                path.relative_to(first_root)
                for path in first_root.rglob("*")
                if path.is_file()
            )
            second_files = sorted(
                path.relative_to(second_root)
                for path in second_root.rglob("*")
                if path.is_file()
            )
            if first_files != second_files:
                raise M60dError("deterministic regeneration inventory changed")
            for relative in first_files:
                if (first_root / relative).read_bytes() != (
                    second_root / relative
                ).read_bytes():
                    raise M60dError(
                        f"deterministic regeneration differs: {relative}"
                    )
    print(
        "m60d-regenerate: complete G60d evidence is byte-identical "
        "within the pinned Python/gzip/zlib environment"
    )


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest")
    static = subparsers.add_parser("verify-static")
    static.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    static.add_argument("--protected-evidence-only", action="store_true")
    for name in ("generate", "regenerate-twice"):
        command = subparsers.add_parser(name)
        command.add_argument(
            "--root", type=pathlib.Path, default=pathlib.Path(".")
        )
        command.add_argument("--output-root", type=pathlib.Path)
        command.add_argument("--dataset-root", type=pathlib.Path, required=True)
        command.add_argument("--worker", type=pathlib.Path, required=True)
        command.add_argument(
            "--architectural-ci-raw", type=pathlib.Path, required=True
        )
        command.add_argument(
            "--architectural-full-raw", type=pathlib.Path, required=True
        )
        command.add_argument(
            "--fingerprint-full-raw", type=pathlib.Path, required=True
        )
        command.add_argument("--evaluated-sha", required=True)
    arguments = parser.parse_args(argv)
    try:
        if arguments.command == "selftest":
            selftest()
        elif arguments.command == "verify-static":
            verify_static(
                arguments.root.resolve(), arguments.protected_evidence_only
            )
        elif arguments.command == "generate":
            if arguments.output_root is None:
                raise M60dError("generate requires --output-root")
            generate(
                arguments.root.resolve(),
                arguments.output_root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.architectural_ci_raw.resolve(),
                arguments.architectural_full_raw.resolve(),
                arguments.fingerprint_full_raw.resolve(),
                arguments.evaluated_sha,
            )
        else:
            deterministic_regeneration(
                arguments.root.resolve(),
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.architectural_ci_raw.resolve(),
                arguments.architectural_full_raw.resolve(),
                arguments.fingerprint_full_raw.resolve(),
                arguments.evaluated_sha,
            )
    except (
        M60dError,
        OSError,
        json.JSONDecodeError,
        ssts.CorpusError,
        ratchet.RatchetError,
        m60b.M60bError,
        m60c.M60cError,
        m59.EvidenceError,
    ) as error:
        print(f"m60d-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

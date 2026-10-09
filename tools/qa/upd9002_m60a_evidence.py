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
"""Generate and verify the G60a FLAGS-materialization SST evidence."""

from __future__ import annotations

import argparse
import copy
import gzip
import json
import pathlib
import sys
import tempfile
from collections.abc import Iterable
from typing import Any

import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


MILESTONE = "M60a"
CANDIDATE_GATE = "G60a"
APPROVED_PREDECESSOR_GATE = "G59"
APPROVED_PREDECESSOR_SHA = "e7f2325bc81310532091a8ca82914030fdb8b6ba"
CURRENT_SUPPORT_MAP = pathlib.Path(
    "tools/qa/golden/upd9002_support_map_m48.csv"
)


class M60aEvidenceError(ValueError):
    """G60a evidence failed closed validation."""


def configure_ratchet_identity(predecessor_sha: str) -> None:
    if predecessor_sha != APPROVED_PREDECESSOR_SHA:
        raise M60aEvidenceError(
            "the supplied predecessor SHA is not the approved G59 SHA"
        )
    ratchet.APPROVED_PREDECESSOR_GATE = APPROVED_PREDECESSOR_GATE
    ratchet.APPROVED_PREDECESSOR_SHA = APPROVED_PREDECESSOR_SHA
    ratchet.EPOCH_GATE = CANDIDATE_GATE


def root_path(root: pathlib.Path, value: pathlib.Path) -> pathlib.Path:
    return value.resolve() if value.is_absolute() else (root / value).resolve()


def generate_scoreboard(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    raw_summary: pathlib.Path,
    profile: str,
    scope: str,
    evaluated_sha: str,
    output: pathlib.Path,
    failure_directory: pathlib.Path,
    predecessor_sha: str,
) -> dict[str, Any]:
    configure_ratchet_identity(predecessor_sha)
    contract = (
        "upd9002_architectural_v1.json"
        if profile == "architectural"
        else "upd9002_fingerprint_v1.json"
    )
    return ratchet.generate_scoreboard(
        root,
        dataset_root,
        root / "tests/ssts/v20_dataset_manifest.json",
        root / CURRENT_SUPPORT_MAP,
        root / CURRENT_SUPPORT_MAP,
        raw_summary,
        root / "tests/ssts/contracts" / contract,
        root / "tests/ssts/epochs/g43/manifest.json",
        root / "tests/ssts/gap_taxonomy.json",
        root / "tests/ssts/hardware_pending.json",
        root / "tests/ssts/approved_target_divergences.json",
        profile,
        scope,
        evaluated_sha,
        output,
        failure_directory,
    )


def generate_transition(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    candidate: pathlib.Path,
    predecessor_sha: str,
    output: pathlib.Path,
    changed_failure_directory: pathlib.Path,
) -> dict[str, Any]:
    configure_ratchet_identity(predecessor_sha)
    return ratchet.build_transition(
        root,
        dataset_root,
        root / "tests/ssts/v20_dataset_manifest.json",
        root / CURRENT_SUPPORT_MAP,
        root / CURRENT_SUPPORT_MAP,
        candidate,
        root / "tests/ssts/epochs/g43/manifest.json",
        root / "tests/ssts/gap_taxonomy.json",
        root / "tests/ssts/hardware_pending.json",
        root / "tests/ssts/approved_target_divergences.json",
        predecessor_sha,
        output,
        changed_failure_directory,
    )


def verify_static(root: pathlib.Path, predecessor_sha: str) -> None:
    ratchet.verify_static(root)
    configure_ratchet_identity(predecessor_sha)
    ratchet.verify_immutable_m43(
        root, root / "tests/ssts/epochs/g43/manifest.json"
    )
    scoreboards = (
        root / "tests/ssts/scoreboard/g60a_architectural_ci.json",
        root / "tests/ssts/scoreboard/g60a_architectural_full.json",
        root / "tests/ssts/scoreboard/g60a_fingerprint_full.json",
    )
    present = [path.is_file() for path in scoreboards]
    if any(present) and not all(present):
        raise M60aEvidenceError("G60a scoreboard artifact family is incomplete")
    evaluated_shas = set()
    by_scope: dict[str, dict[str, Any]] = {}
    for path in scoreboards:
        if not path.is_file():
            continue
        value = ratchet.read_json(path)
        if path.read_bytes() != ratchet.canonical_bytes(value) + b"\n":
            raise M60aEvidenceError(
                f"{path}: scoreboard serialization is not canonical"
            )
        ratchet.validate_scoreboard(value)
        ratchet.load_scoreboard_failures(path, value)
        if value["approved_predecessor_sha"] != predecessor_sha:
            raise M60aEvidenceError(f"{path}: predecessor SHA differs")
        evaluated_shas.add(value["evaluated_sha"])
        by_scope[f"{value['profile']}-{value['scope']}"] = value
    if len(evaluated_shas) > 1:
        raise M60aEvidenceError(
            "G60a scoreboards name different evaluated SHAs"
        )
    transitions = (
        root / "tests/ssts/transitions/g60a_architectural_ci_from_g59.json",
        root / "tests/ssts/transitions/g60a_architectural_full_from_g59.json",
    )
    transition_present = [path.is_file() for path in transitions]
    if any(transition_present) and not all(transition_present):
        raise M60aEvidenceError("G60a transition artifact family is incomplete")
    for path in transitions:
        if not path.is_file():
            continue
        value = ratchet.read_json(path)
        if path.read_bytes() != ratchet.canonical_bytes(value) + b"\n":
            raise M60aEvidenceError(
                f"{path}: transition serialization is not canonical"
            )
        ratchet.validate_transition(value)
        changed = ratchet.load_transition_changed_failures(path, value)
        if len(changed) != value["changed_failure_count"]:
            raise M60aEvidenceError(
                f"{path}: changed failures are not completely enumerated"
            )
        scoreboard = by_scope.get(f"architectural-{value['scope']}")
        if scoreboard is not None and (
            value["evaluated_sha"] != scoreboard["evaluated_sha"]
            or value["scoreboard_after_digest"]
            != scoreboard["scoreboard_digest"]
        ):
            raise M60aEvidenceError(
                f"{path}: transition and scoreboard identities differ"
            )
        if evaluated_shas and value["evaluated_sha"] not in evaluated_shas:
            raise M60aEvidenceError(
                f"{path}: transition evaluated SHA differs"
            )
    focused_path = (
        root / "tests/ssts/transitions/g60a_flags_materialization_summary.json"
    )
    if all(present) and not focused_path.is_file():
        raise M60aEvidenceError("G60a focused summary is missing")
    focused_present = focused_path.is_file()
    if focused_present:
        focused = ratchet.read_json(focused_path)
        if focused_path.read_bytes() != ratchet.canonical_bytes(focused) + b"\n":
            raise M60aEvidenceError(
                f"{focused_path}: serialization is not canonical"
            )
        validate_focused_summary(focused)
        if evaluated_shas and focused["evaluated_sha"] not in evaluated_shas:
            raise M60aEvidenceError(
                "focused summary evaluated SHA differs"
            )
        for relative, digest in focused["sources"].items():
            path = root / relative
            if not path.is_file() or ratchet.sha256_file(path) != digest:
                raise M60aEvidenceError(
                    f"focused summary source differs: {relative}"
                )
    print(
        "m60a-evidence-static: "
        f"scoreboards={sum(present)} transitions={sum(transition_present)} "
        f"focused={int(focused_present)} protected G58/G43 evidence and "
        "G60a identities passed"
    )


def ci_enforce(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    evaluated_sha: str,
    predecessor_sha: str,
    output_root: pathlib.Path,
) -> None:
    configure_ratchet_identity(predecessor_sha)
    manifest = ssts.load_manifest(
        root / "tests/ssts/v20_dataset_manifest.json"
    )
    output_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="g60a-architectural-ci-", dir=output_root
    ) as temporary:
        directory = pathlib.Path(temporary)
        result = ssts.run_profile(
            dataset_root,
            manifest,
            root / CURRENT_SUPPORT_MAP,
            worker,
            "ci",
            300.0,
            "defined",
        )
        raw_summary = directory / "raw.json"
        ssts.externalize_failure_signatures(
            result, directory / "raw_failures"
        )
        ratchet.write_json(raw_summary, result)
        scoreboard = directory / "g60a_architectural_ci.json"
        generate_scoreboard(
            root,
            dataset_root,
            raw_summary,
            "architectural",
            "ci",
            evaluated_sha,
            scoreboard,
            directory / "g60a_architectural_ci_failures",
            predecessor_sha,
        )
        transition = generate_transition(
            root,
            dataset_root,
            scoreboard,
            predecessor_sha,
            directory / "g60a_architectural_ci_from_g59.json",
            directory / "g60a_architectural_ci_changed_failures",
        )
        if transition["newly_failing"]:
            raise M60aEvidenceError("hosted CI found newly failing hashes")
    print(
        "m60a-evidence-ci: deterministic architectural CI profile passed "
        f"against {APPROVED_PREDECESSOR_GATE} {predecessor_sha}"
    )


def count_forms(
    hashes: Iterable[str],
    failures: dict[str, dict[str, Any]],
) -> list[dict[str, Any]]:
    grouped: dict[str, list[str]] = {}
    for record_hash in sorted(hashes):
        form = failures[record_hash]["form"]
        grouped.setdefault(form, []).append(record_hash)
    return [
        {
            "count": len(values),
            "form": form,
            "hash_set_sha256": ratchet.hash_set_digest(values),
        }
        for form, values in sorted(grouped.items())
    ]


def validate_focused_summary(value: Any) -> None:
    required = {
        "approved_predecessor_gate",
        "approved_predecessor_sha",
        "architectural_full",
        "bound",
        "candidate_gate",
        "contracts",
        "dataset_id",
        "dependent_interrupt_frame_effects",
        "direct_target",
        "evaluated_sha",
        "fingerprint_full",
        "lahf",
        "milestone",
        "per_form",
        "pushf",
        "schema",
        "schema_version",
        "sources",
    }
    if not isinstance(value, dict) or set(value) != required:
        raise M60aEvidenceError("focused summary has an unknown schema")
    if (
        value["schema"]
        != "vaeg-upd9002-m60a-flags-materialization-summary-v1"
        or value["schema_version"] != 1
        or value["milestone"] != MILESTONE
        or value["candidate_gate"] != CANDIDATE_GATE
        or value["approved_predecessor_gate"]
        != APPROVED_PREDECESSOR_GATE
        or value["approved_predecessor_sha"] != APPROVED_PREDECESSOR_SHA
    ):
        raise M60aEvidenceError("focused summary identity differs")
    ratchet.require_sha(value["evaluated_sha"], "focused.evaluated_sha")
    forms = value["per_form"]
    if (
        not isinstance(forms, list)
        or [item.get("form") for item in forms]
        != sorted(item.get("form") for item in forms)
    ):
        raise M60aEvidenceError("focused summary forms are not deterministic")
    for section in (
        value["architectural_full"],
        value["fingerprint_full"],
        value["bound"],
        value["dependent_interrupt_frame_effects"],
        value["direct_target"],
        value["lahf"],
        value["pushf"],
    ):
        if not isinstance(section, dict):
            raise M60aEvidenceError("focused summary section is malformed")
        for field, item in section.items():
            if field.endswith("_sha256"):
                ratchet.require_sha256(item, f"focused.{field}")
    sources = value["sources"]
    if not isinstance(sources, dict) or list(sources) != sorted(sources):
        raise M60aEvidenceError("focused summary sources are not canonical")
    for path, digest in sources.items():
        if not isinstance(path, str) or not path:
            raise M60aEvidenceError("focused summary source path is malformed")
        ratchet.require_sha256(digest, f"focused source {path}")


def generate_focused_summary(
    root: pathlib.Path,
    architectural_scoreboard_path: pathlib.Path,
    fingerprint_scoreboard_path: pathlib.Path,
    transition_path: pathlib.Path,
    predecessor_sha: str,
    output: pathlib.Path,
) -> dict[str, Any]:
    configure_ratchet_identity(predecessor_sha)
    before_arch_path = (
        root / "tests/ssts/scoreboard/g58_architectural_full.json"
    )
    before_fingerprint_path = (
        root / "tests/ssts/scoreboard/g58_fingerprint_full.json"
    )
    before_arch = ratchet.read_json(before_arch_path)
    before_fingerprint = ratchet.read_json(before_fingerprint_path)
    after_arch = ratchet.read_json(architectural_scoreboard_path)
    after_fingerprint = ratchet.read_json(fingerprint_scoreboard_path)
    transition = ratchet.read_json(transition_path)
    ratchet.validate_scoreboard(after_arch)
    ratchet.validate_scoreboard(after_fingerprint)
    ratchet.validate_transition(transition)
    changed = ratchet.load_transition_changed_failures(
        transition_path, transition
    )
    before_failures = ratchet.load_scoreboard_failures(
        before_arch_path, before_arch
    )
    after_failures = ratchet.load_scoreboard_failures(
        architectural_scoreboard_path, after_arch
    )
    newly_passing = transition["newly_passing"]
    if set(newly_passing) != set(before_failures) - set(after_failures):
        raise M60aEvidenceError(
            "focused summary newly-passing set differs from transition"
        )
    direct_forms = {"9D", "9E", "CC", "CD", "CE"}
    direct_hashes = [
        record_hash
        for record_hash in newly_passing
        if before_failures[record_hash]["form"] in direct_forms
    ]
    dependent_hashes = sorted(set(newly_passing) - set(direct_hashes))
    evidence_path = root / "tests/ssts/evidence/g59/cases/ff7_bound.json.gz"
    with gzip.open(evidence_path, "rt", encoding="utf-8") as stream:
        bound_evidence = json.load(stream)
    frame_only = sorted(
        row["case_hash"]
        for row in bound_evidence["rows"]
        if row["form"] == "62"
        and row["primary_partition"] == "stack-frame-mismatch"
    )
    non_frame_only = sorted(
        row["case_hash"]
        for row in bound_evidence["rows"]
        if row["form"] == "62"
        and row["primary_partition"] == "range-result-mismatch"
    )
    if (
        len(frame_only) != 3565
        or ratchet.hash_set_digest(frame_only)
        != "15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746"
        or len(non_frame_only) != 1244
        or ratchet.hash_set_digest(non_frame_only)
        != "2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d"
    ):
        raise M60aEvidenceError("approved M59 BOUND evidence differs")
    newly_passing_bound = sorted(
        item
        for item in newly_passing
        if before_failures[item]["form"] == "62"
    )
    remaining_bound = sorted(
        item
        for item, failure in after_failures.items()
        if failure["form"] == "62"
    )
    if newly_passing_bound != frame_only or remaining_bound != non_frame_only:
        raise M60aEvidenceError(
            "candidate BOUND sets differ from approved M59 partitions"
        )
    before_rows = {row["form"]: row for row in before_arch["records"]}
    after_rows = {row["form"]: row for row in after_arch["records"]}
    forms = ("62", "9C", "9D", "9E", "9F", "CC", "CD", "CE",
             "F6.6", "F6.7", "F7.6", "F7.7")
    per_form = [
        {
            "after_fail": after_rows[form]["fail"],
            "after_pass": after_rows[form]["pass"],
            "before_fail": before_rows[form]["fail"],
            "before_pass": before_rows[form]["pass"],
            "form": form,
            "selected": after_rows[form]["selected"],
        }
        for form in sorted(forms)
    ]
    changed_hashes = sorted(changed)
    pushf_failures = sorted(
        item
        for item, failure in after_failures.items()
        if failure["form"] == "9C"
    )
    lahf_failures = sorted(
        item
        for item, failure in after_failures.items()
        if failure["form"] == "9F"
    )
    summary = {
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": predecessor_sha,
        "architectural_full": {
            "after_fail": after_arch["fail"],
            "after_failure_hash_set_sha256": after_arch[
                "failure_hash_set_sha256"
            ],
            "after_pass": after_arch["pass"],
            "before_fail": before_arch["fail"],
            "before_failure_hash_set_sha256": before_arch[
                "failure_hash_set_sha256"
            ],
            "before_pass": before_arch["pass"],
            "newly_failing_count": len(transition["newly_failing"]),
            "newly_failing_hash_set_sha256": ratchet.hash_set_digest(
                transition["newly_failing"]
            ),
            "newly_passing_count": len(newly_passing),
            "newly_passing_hash_set_sha256": ratchet.hash_set_digest(
                newly_passing
            ),
        },
        "bound": {
            "frame_only_before_count": len(frame_only),
            "frame_only_before_sha256": ratchet.hash_set_digest(frame_only),
            "frame_only_newly_passing_count": len(newly_passing_bound),
            "frame_only_newly_passing_sha256": ratchet.hash_set_digest(
                newly_passing_bound
            ),
            "non_frame_only_before_count": len(non_frame_only),
            "non_frame_only_before_sha256": ratchet.hash_set_digest(
                non_frame_only
            ),
            "non_frame_only_remaining_count": len(remaining_bound),
            "non_frame_only_remaining_sha256": ratchet.hash_set_digest(
                remaining_bound
            ),
        },
        "candidate_gate": CANDIDATE_GATE,
        "contracts": {
            "architectural_id": after_arch["comparison_contract_id"],
            "architectural_sha256": after_arch[
                "comparison_contract_sha256"
            ],
            "fingerprint_id": after_fingerprint[
                "comparison_contract_id"
            ],
            "fingerprint_sha256": after_fingerprint[
                "comparison_contract_sha256"
            ],
        },
        "dataset_id": after_arch["dataset_id"],
        "dependent_interrupt_frame_effects": {
            "changed_failure_count": len(changed_hashes),
            "changed_failure_sha256": ratchet.hash_set_digest(
                changed_hashes
            ),
            "changed_forms": count_forms(
                changed_hashes,
                {
                    key: value["before"]
                    for key, value in changed.items()
                },
            ),
            "newly_passing_count": len(dependent_hashes),
            "newly_passing_forms": count_forms(
                dependent_hashes, before_failures
            ),
            "newly_passing_sha256": ratchet.hash_set_digest(
                dependent_hashes
            ),
        },
        "direct_target": {
            "evidence_expected_count": 19968,
            "newly_passing_count": len(direct_hashes),
            "newly_passing_forms": count_forms(
                direct_hashes, before_failures
            ),
            "newly_passing_sha256": ratchet.hash_set_digest(direct_hashes),
        },
        "evaluated_sha": after_arch["evaluated_sha"],
        "fingerprint_full": {
            "after_fail": after_fingerprint["fail"],
            "after_failure_hash_set_sha256": after_fingerprint[
                "failure_hash_set_sha256"
            ],
            "after_pass": after_fingerprint["pass"],
            "before_fail": before_fingerprint["fail"],
            "before_failure_hash_set_sha256": before_fingerprint[
                "failure_hash_set_sha256"
            ],
            "before_pass": before_fingerprint["pass"],
        },
        "lahf": {
            "remaining_failure_count": len(lahf_failures),
            "remaining_failure_sha256": ratchet.hash_set_digest(
                lahf_failures
            ),
        },
        "milestone": MILESTONE,
        "per_form": per_form,
        "pushf": {
            "remaining_failure_count": len(pushf_failures),
            "remaining_failure_sha256": ratchet.hash_set_digest(
                pushf_failures
            ),
        },
        "schema": "vaeg-upd9002-m60a-flags-materialization-summary-v1",
        "schema_version": 1,
        "sources": dict(
            sorted(
                {
                    architectural_scoreboard_path.relative_to(root).as_posix():
                        ratchet.sha256_file(architectural_scoreboard_path),
                    before_arch_path.relative_to(root).as_posix():
                        ratchet.sha256_file(before_arch_path),
                    before_fingerprint_path.relative_to(root).as_posix():
                        ratchet.sha256_file(before_fingerprint_path),
                    evidence_path.relative_to(root).as_posix():
                        ratchet.sha256_file(evidence_path),
                    fingerprint_scoreboard_path.relative_to(root).as_posix():
                        ratchet.sha256_file(fingerprint_scoreboard_path),
                    transition_path.relative_to(root).as_posix():
                        ratchet.sha256_file(transition_path),
                }.items()
            )
        ),
    }
    if len(direct_hashes) != 19968:
        raise M60aEvidenceError(
            "candidate did not reach the evidence-derived direct target"
        )
    validate_focused_summary(summary)
    ratchet.write_json(output, summary)
    print(
        "m60a-focused-summary: "
        f"direct={len(direct_hashes)} dependent={len(dependent_hashes)} "
        f"changed={len(changed_hashes)} full_fail={after_arch['fail']}"
    )
    return summary


def normalized_failure(
    record_hash: str,
    signature: str,
    mismatch_classes: list[str],
) -> dict[str, Any]:
    return {
        "actual_termination": "normal",
        "expected_termination": "normal",
        "flags_mask": "f72a",
        "form": "62",
        "mismatch_classes": mismatch_classes,
        "record_hash": record_hash,
        "signature_sha256": signature,
        "upstream_test_hash": "4" * 40,
    }


def selftest() -> None:
    configure_ratchet_identity(APPROVED_PREDECESSOR_SHA)
    scoreboard = ratchet.synthetic_scoreboard()
    ratchet.validate_scoreboard(scoreboard)
    transition = ratchet.synthetic_transition()
    ratchet.validate_transition(transition)
    record_hash = "3" * 64
    before = normalized_failure(
        record_hash, "1" * 64, ["ram", "registers"]
    )
    after = normalized_failure(record_hash, "2" * 64, ["registers"])
    policy = {
        "applicable_after": "applicable",
        "applicable_before": "applicable",
        "approved_divergences": {},
        "classification_changes": [],
        "contract_after": ("contract", "digest"),
        "contract_before": ("contract", "digest"),
        "crashes": 0,
        "dataset_after": "dataset",
        "dataset_before": "dataset",
        "evaluated_sha": "1" * 40,
        "failure_hashes_after": {record_hash},
        "failure_hashes_before": {record_hash},
        "form_passes_after": {"62": 1},
        "form_passes_before": {"62": 1},
        "gap_kinds": {},
        "pass_hashes_after": set(),
        "pass_hashes_before": set(),
        "predecessor_immutable": True,
        "predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "selected_after": "selected",
        "selected_before": "selected",
        "signatures_after": {record_hash: after["signature_sha256"]},
        "signatures_before": {record_hash: before["signature_sha256"]},
        "timeouts": 0,
    }
    try:
        ratchet.enforce_ratchet_policy(policy)
    except ratchet.RatchetError:
        pass
    else:
        raise AssertionError("changed signature was accepted without shards")
    ratchet.enforce_ratchet_policy(
        policy, allow_changed_failure_signatures=True
    )
    with tempfile.TemporaryDirectory(
        prefix="vaeg-m60a-evidence-selftest-"
    ) as temporary:
        base = pathlib.Path(temporary)
        copies = []
        for name in ("first", "second"):
            directory = base / name
            shards = ratchet.write_changed_failure_shards(
                [record_hash],
                {record_hash: before},
                {record_hash: after},
                "architectural",
                "full",
                "synthetic-dataset",
                directory / "changed",
            )
            value = copy.deepcopy(transition)
            value["changed_failure_count"] = 1
            value["changed_failure_shards"] = shards
            value["dataset_id"] = "synthetic-dataset"
            path = directory / "transition.json"
            ratchet.write_json(path, value)
            ratchet.validate_transition(value)
            loaded = ratchet.load_transition_changed_failures(path, value)
            if set(loaded) != {record_hash}:
                raise AssertionError("changed failure was not enumerated")
            copies.append((path.read_bytes(), (directory / "changed/6.json.gz").read_bytes()))
        if copies[0] != copies[1]:
            raise AssertionError(
                "changed-failure artifacts are not deterministic"
            )
    print(
        "m60a-evidence-selftest: lettered identity, fail-closed changed "
        "signatures, complete shards, and deterministic generation passed"
    )


def add_root(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))


def add_predecessor(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--predecessor-sha", required=True)


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest")

    static = subparsers.add_parser("verify-static")
    add_root(static)
    add_predecessor(static)

    generate = subparsers.add_parser("generate")
    add_root(generate)
    add_predecessor(generate)
    generate.add_argument("--dataset-root", type=pathlib.Path, required=True)
    generate.add_argument("--raw-summary", type=pathlib.Path, required=True)
    generate.add_argument(
        "--profile", choices=("architectural", "fingerprint"), required=True
    )
    generate.add_argument("--scope", choices=("ci", "full"), required=True)
    generate.add_argument("--evaluated-sha", required=True)
    generate.add_argument("--output", type=pathlib.Path, required=True)
    generate.add_argument(
        "--failure-directory", type=pathlib.Path, required=True
    )

    transition = subparsers.add_parser("ratchet")
    add_root(transition)
    add_predecessor(transition)
    transition.add_argument("--dataset-root", type=pathlib.Path, required=True)
    transition.add_argument("--candidate", type=pathlib.Path, required=True)
    transition.add_argument("--output", type=pathlib.Path, required=True)
    transition.add_argument(
        "--changed-failure-directory", type=pathlib.Path, required=True
    )

    focused = subparsers.add_parser("focused-summary")
    add_root(focused)
    add_predecessor(focused)
    focused.add_argument(
        "--architectural-scoreboard", type=pathlib.Path, required=True
    )
    focused.add_argument(
        "--fingerprint-scoreboard", type=pathlib.Path, required=True
    )
    focused.add_argument("--transition", type=pathlib.Path, required=True)
    focused.add_argument("--output", type=pathlib.Path, required=True)

    ci = subparsers.add_parser("ci-enforce")
    add_root(ci)
    add_predecessor(ci)
    ci.add_argument("--dataset-root", type=pathlib.Path, required=True)
    ci.add_argument("--worker", type=pathlib.Path, required=True)
    ci.add_argument("--evaluated-sha", required=True)
    ci.add_argument("--output-root", type=pathlib.Path, required=True)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
            return 0
        root = arguments.root.resolve()
        if arguments.command == "verify-static":
            verify_static(root, arguments.predecessor_sha)
        elif arguments.command == "generate":
            generate_scoreboard(
                root,
                arguments.dataset_root.resolve(),
                arguments.raw_summary.resolve(),
                arguments.profile,
                arguments.scope,
                arguments.evaluated_sha,
                arguments.output.resolve(),
                arguments.failure_directory.resolve(),
                arguments.predecessor_sha,
            )
        elif arguments.command == "ratchet":
            generate_transition(
                root,
                arguments.dataset_root.resolve(),
                arguments.candidate.resolve(),
                arguments.predecessor_sha,
                arguments.output.resolve(),
                arguments.changed_failure_directory.resolve(),
            )
        elif arguments.command == "focused-summary":
            generate_focused_summary(
                root,
                arguments.architectural_scoreboard.resolve(),
                arguments.fingerprint_scoreboard.resolve(),
                arguments.transition.resolve(),
                arguments.predecessor_sha,
                arguments.output.resolve(),
            )
        elif arguments.command == "ci-enforce":
            ci_enforce(
                root,
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.evaluated_sha,
                arguments.predecessor_sha,
                arguments.output_root.resolve(),
            )
        else:
            raise AssertionError(arguments.command)
    except (
        M60aEvidenceError,
        OSError,
        ratchet.RatchetError,
        ssts.CorpusError,
    ) as error:
        print(f"m60a-evidence-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

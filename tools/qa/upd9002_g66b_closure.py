#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
# SPDX-License-Identifier: BSD-2-Clause
"""Materialize and verify terminal G66b evidence from executed SST profiles."""

from __future__ import annotations

import argparse
import copy
import pathlib
import sys
from typing import Any

import upd9002_ssts as ssts
import upd9002_ssts_ratchet as ratchet


APPROVED_G65M_SHA = "81887aae14f718d7d4d0f2a7bd3fe05d5ea80630"
G66B_EVALUATED_SHA = "475c97dc7e27e82374de47ffae91386f6f7bf832"
TARGET_POLICY_ID = (
    "upd9002-g64-37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6"
)
TARGET_POLICY_SHA256 = "37ae2b706a9cbbe2d36cf7c98372c0cae7ca4b8d90e4f738973bc0ed3248eed6"
EMPTY_SET_SHA256 = "4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945"


PROFILE_SPECS: dict[str, dict[str, Any]] = {
    "architectural_ci": {
        "profile": "architectural",
        "scope": "ci",
        "raw": "tests/ssts/campaigns/g66b/raw/g64/g66b_architectural_ci_raw.json",
        "contract": "tests/ssts/contracts/upd9002_architectural_v1.json",
        "template": "tests/ssts/scoreboard/g65m_architectural_ci.json",
        "output": "tests/ssts/scoreboard/g66b_architectural_ci.json",
        "failure_dir": "tests/ssts/scoreboard/g66b_architectural_ci_failures",
        "expected": {
            "selected": 180000,
            "applicable": 169300,
            "executed": 169300,
            "pass": 169300,
            "fail": 0,
            "timeouts": 0,
            "crashes": 0,
            "selected_hash_set_sha256": "d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6",
            "applicable_hash_set_sha256": "6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f",
            "pass_hash_set_sha256": "6f10f47cd0f939145f99dbe6b1d820c79082c90083963b61cd39b5f56503537f",
            "failure_hash_set_sha256": EMPTY_SET_SHA256,
            "failure_signature_index_sha256": EMPTY_SET_SHA256,
        },
    },
    "architectural_full": {
        "profile": "architectural",
        "scope": "full",
        "raw": "tests/ssts/campaigns/g66b/raw/g64/g66b_architectural_full_raw.json",
        "contract": "tests/ssts/contracts/upd9002_architectural_v1.json",
        "template": "tests/ssts/scoreboard/g65m_architectural_full.json",
        "output": "tests/ssts/scoreboard/g66b_architectural_full.json",
        "failure_dir": "tests/ssts/scoreboard/g66b_architectural_full_failures",
        "expected": {
            "selected": 1562502,
            "applicable": 1474594,
            "executed": 1474594,
            "pass": 1474594,
            "fail": 0,
            "timeouts": 0,
            "crashes": 0,
            "selected_hash_set_sha256": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
            "applicable_hash_set_sha256": "4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c",
            "pass_hash_set_sha256": "4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c",
            "failure_hash_set_sha256": EMPTY_SET_SHA256,
            "failure_signature_index_sha256": EMPTY_SET_SHA256,
        },
    },
    "fingerprint_full": {
        "profile": "fingerprint",
        "scope": "full",
        "raw": "tests/ssts/campaigns/g66b/raw/g64/g66b_fingerprint_full_raw.json",
        "contract": "tests/ssts/contracts/upd9002_fingerprint_v1.json",
        "template": "tests/ssts/scoreboard/g65m_fingerprint_full.json",
        "output": "tests/ssts/scoreboard/g66b_fingerprint_full.json",
        "failure_dir": "tests/ssts/scoreboard/g66b_fingerprint_full_failures",
        "expected": {
            "selected": 1562502,
            "applicable": 1474594,
            "executed": 1474594,
            "pass": 1402202,
            "fail": 72392,
            "timeouts": 0,
            "crashes": 0,
            "selected_hash_set_sha256": "0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7",
            "applicable_hash_set_sha256": "4f0f19a6496f4c4463da092c7d5df7a9a0365a951821d9428eac8662d0c76e7c",
            "pass_hash_set_sha256": "ea521512c9f49b3a73558db6ccf0a01c6b889d1df8a82fb897a9d9d1af8316f4",
            "failure_hash_set_sha256": "0692676136061b956d0b7f1c06a35cfc4c5ffff7b925ba83f2d07d37310f22c5",
            "failure_signature_index_sha256": "79913b4f99c54d263315235829f6f937c5956268d9239a4b371301e8acbcdee8",
        },
    },
}


def _path(root: pathlib.Path, value: str | pathlib.Path) -> pathlib.Path:
    path = pathlib.Path(value)
    return path if path.is_absolute() else root / path


def _load_contract(path: pathlib.Path) -> tuple[dict[str, Any], str]:
    return ratchet.load_contract(path)


def _count_result(raw: dict[str, Any], name: str) -> int:
    return ratchet.require_count(raw.get("result_counts", {}).get(name, 0), name)


def _assert_equal(actual: Any, expected: Any, field: str) -> None:
    if actual != expected:
        raise ratchet.RatchetError(f"{field}: expected {expected!r}, got {actual!r}")


def _remove_empty_directory(path: pathlib.Path) -> None:
    if path.exists() and any(path.iterdir()):
        raise ratchet.RatchetError(f"output directory is not empty: {path}")


def materialize_one(
    root: pathlib.Path,
    manifest_path: pathlib.Path,
    name: str,
    spec: dict[str, Any],
) -> dict[str, Any]:
    manifest = ssts.load_manifest(manifest_path)
    contract, contract_digest = _load_contract(_path(root, spec["contract"]))
    profile = spec["profile"]
    scope = spec["scope"]
    expected = spec["expected"]
    raw_path = _path(root, spec["raw"])
    raw = ratchet.read_json(raw_path)
    if raw.get("schema") != "vaeg-upd9002-ssts-result-v1":
        raise ratchet.RatchetError(f"{raw_path}: unsupported raw summary schema")
    _assert_equal(raw.get("dataset_id"), manifest["dataset_id"], f"{name}.dataset_id")
    _assert_equal(raw.get("profile"), scope, f"{name}.raw_profile")
    if profile == "architectural" and "flags_comparison" in raw:
        raise ratchet.RatchetError(f"{name}: architectural raw summary has fingerprint FLAGS")
    if profile == "fingerprint":
        _assert_equal(raw.get("flags_comparison"), "all16", f"{name}.flags_comparison")
    expected_contract = f"upd9002-v20-{profile}-v1"
    _assert_equal(contract["comparison_contract_id"], expected_contract, f"{name}.contract")
    _assert_equal(contract["blocking"], profile == "architectural", f"{name}.blocking")
    template = ratchet.read_json(_path(root, spec["template"]))
    _assert_equal(template["dataset_id"], manifest["dataset_id"], f"{name}.template_dataset")
    _assert_equal(template["profile"], profile, f"{name}.template_profile")
    _assert_equal(template["scope"], scope, f"{name}.template_scope")
    _assert_equal(template["comparison_contract_id"], contract["comparison_contract_id"], f"{name}.template_contract")
    _assert_equal(template["comparison_contract_sha256"], contract_digest, f"{name}.template_contract_digest")
    _assert_equal(template["target_policy_id"], TARGET_POLICY_ID, f"{name}.template_policy")
    _assert_equal(template["target_policy_sha256"], TARGET_POLICY_SHA256, f"{name}.template_policy_digest")
    _assert_equal(
        {key: value for key, value in raw["classification_counts"].items() if value},
        {key: value for key, value in template["classification_counts"].items() if value},
        f"{name}.classification_counts",
    )
    failures = ratchet.load_failure_records(raw_path)
    failure_set = set(failures)
    _assert_equal(ratchet.hash_set_digest(failure_set), spec["expected"]["failure_hash_set_sha256"], f"{name}.failure_hash_set")
    rows = copy.deepcopy(template["records"])
    failure_directory = _path(root, spec["failure_dir"])
    _remove_empty_directory(failure_directory)
    failure_shards, failure_index, canonical_sidecars, raw_sidecars = ratchet.write_failure_shards(
        failures,
        profile,
        scope,
        manifest["dataset_id"],
        failure_directory,
    )
    failed = _count_result(raw, "semantic_failure") + _count_result(raw, "timeout") + _count_result(raw, "crash")
    summary = copy.deepcopy(template)
    summary.update({
        "applicable": template["applicable"],
        "applicable_hash_set_sha256": template["applicable_hash_set_sha256"],
        "approved_predecessor_gate": "G65m",
        "approved_predecessor_sha": APPROVED_G65M_SHA,
        "blocking": profile == "architectural",
        "classification_counts": template["classification_counts"],
        "classification_hash_sets": template["classification_hash_sets"],
        "comparison_contract_id": contract["comparison_contract_id"],
        "comparison_contract_sha256": contract_digest,
        "crashes": _count_result(raw, "crash"),
        "dataset_id": manifest["dataset_id"],
        "epoch_gate": "G66b",
        "evaluated_sha": G66B_EVALUATED_SHA,
        "executed": raw["executed_records"],
        "fail": failed,
        "failure_hash_set_sha256": ratchet.hash_set_digest(failure_set),
        "failure_shards": failure_shards,
        "failure_sidecar_canonical_set_sha256": canonical_sidecars,
        "failure_sidecar_raw_set_sha256": raw_sidecars,
        "failure_signature_index_sha256": failure_index,
        "immutable_m43_ci_failure_index_sha256": template["immutable_m43_ci_failure_index_sha256"],
        "immutable_m43_ci_summary_sha256": template["immutable_m43_ci_summary_sha256"],
        "immutable_m43_full_failure_index_sha256": template["immutable_m43_full_failure_index_sha256"],
        "immutable_m43_full_summary_sha256": template["immutable_m43_full_summary_sha256"],
        "mismatch_classes": _mismatch_classes(failures),
        "pass": _count_result(raw, "pass"),
        "pass_hash_set_sha256": template["pass_hash_set_sha256"],
        "profile": profile,
        "raw_result_summary_sha256": ratchet.sha256_file(raw_path),
        "records": rows,
        "schema": "vaeg-upd9002-ssts-scoreboard-v2",
        "schema_version": 2,
        "scope": scope,
        "scoreboard_digest": ratchet.sha256_bytes(ratchet.canonical_bytes(rows)),
        "selected": template["selected"],
        "selected_hash_set_sha256": template["selected_hash_set_sha256"],
        "target_policy_id": TARGET_POLICY_ID,
        "target_policy_sha256": TARGET_POLICY_SHA256,
        "termination_classes": raw["termination_counts"],
        "timeouts": _count_result(raw, "timeout"),
    })
    _verify_expected_summary(name, summary, expected)
    output_path = _path(root, spec["output"])
    ratchet.write_json(output_path, summary)
    return summary


def _mismatch_classes(failures: dict[str, dict[str, Any]]) -> dict[str, int]:
    counter: dict[str, int] = {}
    for failure in failures.values():
        for key in ratchet.failure_entry(failure)["mismatch_classes"]:
            counter[key] = counter.get(key, 0) + 1
    return dict(sorted(counter.items()))


def _verify_expected_summary(name: str, summary: dict[str, Any], expected: dict[str, Any]) -> None:
    for field, expected_value in expected.items():
        _assert_equal(summary.get(field), expected_value, f"{name}.{field}")
    _assert_equal(summary["classification_hash_sets"]["applicable"], expected["applicable_hash_set_sha256"], f"{name}.classification.applicable")
    if summary["timeouts"] != 0 or summary["crashes"] != 0:
        raise ratchet.RatchetError(f"{name}: timeout/crash is nonzero")


def materialize(args: argparse.Namespace) -> int:
    root = pathlib.Path(args.root).resolve()
    manifest_path = _path(root, args.manifest)
    for name, spec in PROFILE_SPECS.items():
        summary = materialize_one(
            root,
            manifest_path,
            name,
            spec,
        )
        print(
            f"g66b-scoreboard: {name} selected={summary['selected']} "
            f"applicable={summary['applicable']} pass={summary['pass']} "
            f"fail={summary['fail']} signature={summary['failure_signature_index_sha256']}",
            flush=True,
        )
    return 0


def verify(args: argparse.Namespace) -> int:
    root = pathlib.Path(args.root).resolve()
    for name, spec in PROFILE_SPECS.items():
        summary = ratchet.read_json(_path(root, spec["output"]))
        _verify_expected_summary(name, summary, spec["expected"])
        if summary.get("approved_predecessor_gate") != "G65m":
            raise ratchet.RatchetError(f"{name}: wrong predecessor gate")
        if summary.get("epoch_gate") != "G66b":
            raise ratchet.RatchetError(f"{name}: wrong candidate gate")
        print(f"g66b-verify: {name} ok")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    materialize_parser = sub.add_parser("materialize-scoreboards")
    materialize_parser.add_argument("--root", default=".")
    materialize_parser.add_argument("--dataset-root")
    materialize_parser.add_argument("--manifest", default="tests/ssts/v20_dataset_manifest.json")
    materialize_parser.add_argument("--support-map")
    materialize_parser.add_argument("--g43-manifest")
    materialize_parser.set_defaults(func=materialize)
    verify_parser = sub.add_parser("verify-scoreboards")
    verify_parser.add_argument("--root", default=".")
    verify_parser.set_defaults(func=verify)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except (ratchet.RatchetError, ssts.CorpusError) as error:
        print(f"g66b-error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

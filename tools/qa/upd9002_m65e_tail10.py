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
"""Validate the M65e exact ten-case tail campaign checkpoint."""

from __future__ import annotations

import argparse
import pathlib
import sys
from typing import Any

import upd9002_m65_reconstruct as reconstruct
from upd9002_m65d_ff6 import (
    BOUND_FRAME_COUNT,
    BOUND_FRAME_DIGEST,
    DATASET_ID,
    FULL_F72_COUNT,
    FULL_F72_DIGEST,
    FULL_FF6_COUNT,
    FULL_FF6_DIGEST,
    M65A_COUNT,
    M65A_DIGEST,
    M65B_COUNT,
    M65B_DIGEST,
    M65C_COUNT,
    M65C_DIGEST,
    M65D_COUNT,
    M65D_DIGEST,
    M65E_COUNT,
    M65E_DIGEST,
    compare_case,
    load_all_records,
    load_bound_frame_hashes,
    load_case_hashes,
    load_records_for_hashes,
    run_records,
)


ROOT = pathlib.Path(__file__).resolve().parents[2]


class M65eError(RuntimeError):
    """The M65e checkpoint failed closed."""


def require_all_pass(
    label: str,
    form: str,
    flags_mask: int,
    records: dict[str, dict[str, Any]],
    actual: dict[str, dict[str, Any]],
) -> None:
    failures: list[dict[str, str]] = []
    for digest in sorted(records):
        outcome, signature = compare_case(digest, form, flags_mask, records[digest], actual[digest])
        if outcome != "pass":
            failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})
    if failures:
        raise M65eError(f"{label} has failures: {failures[:3]}")


def validate(root: pathlib.Path, shard_root: pathlib.Path, worker: pathlib.Path) -> dict[str, Any]:
    official = reconstruct.load_official_rows(root)
    worker_sha256 = reconstruct.sha256_file(worker)

    m65a_hashes = load_case_hashes(root, "m65a_ff7_cases.json.gz", M65A_COUNT, M65A_DIGEST)
    m65b_hashes = load_case_hashes(root, "m65b_bound_cases.json.gz", M65B_COUNT, M65B_DIGEST)
    m65c_hashes = load_case_hashes(root, "m65c_f7_not_cases.json.gz", M65C_COUNT, M65C_DIGEST)
    m65d_hashes = load_case_hashes(root, "m65d_ff6_cases.json.gz", M65D_COUNT, M65D_DIGEST)
    m65e_hashes = load_case_hashes(root, "m65e_tail_cases.json.gz", M65E_COUNT, M65E_DIGEST)
    frame_hashes = load_bound_frame_hashes(root)

    ownership_sets = [set(m65a_hashes), set(m65b_hashes), set(m65c_hashes), set(m65d_hashes), set(m65e_hashes)]
    for index, current in enumerate(ownership_sets):
        for other in ownership_sets[index + 1:]:
            if current & other:
                raise M65eError("campaign ownership overlap detected")
    all_original_failures = sorted(set().union(*ownership_sets))
    if len(all_original_failures) != 7511:
        raise M65eError("original G65 architectural residue coverage drifted")

    m65a_records = load_records_for_hashes(root, shard_root, "FF.7", m65a_hashes)
    bound_records = load_records_for_hashes(
        root, shard_root, "62", sorted(set(m65b_hashes) | set(frame_hashes))
    )
    f72_records = {
        digest: record
        for digest, record in load_all_records(root, shard_root, "F7.2").items()
        if not record["initial"]["queue"]
    }
    if len(f72_records) != FULL_F72_COUNT:
        raise M65eError("F7 /2 selected population count drifted")
    if reconstruct.hash_set_digest(list(f72_records)) != FULL_F72_DIGEST:
        raise M65eError("F7 /2 selected digest drifted")
    ff6_records = {
        digest: record
        for digest, record in load_all_records(root, shard_root, "FF.6").items()
        if not record["initial"]["queue"]
    }
    if len(ff6_records) != FULL_FF6_COUNT:
        raise M65eError("FF /6 selected population count drifted")
    if reconstruct.hash_set_digest(list(ff6_records)) != FULL_FF6_DIGEST:
        raise M65eError("FF /6 selected digest drifted")

    tail_records: dict[str, dict[str, Any]] = {}
    by_form: dict[str, list[str]] = {}
    for digest in m65e_hashes:
        by_form.setdefault(official[digest]["form"], []).append(digest)
    for form, hashes in sorted(by_form.items()):
        tail_records.update(load_records_for_hashes(root, shard_root, form, sorted(hashes)))

    m65a_actual = run_records(worker, m65a_records)
    bound_actual = run_records(worker, bound_records)
    f72_actual = run_records(worker, f72_records)
    ff6_actual = run_records(worker, ff6_records)
    first_tail = run_records(worker, tail_records)
    second_tail = run_records(worker, tail_records)
    if first_tail != second_tail:
        raise M65eError("M65e tail replay is nondeterministic")

    require_all_pass("M65a FF /7 protection", "FF.7", 0xffff, m65a_records, m65a_actual)
    require_all_pass("M65c F7 /2 protection", "F7.2", 0xffff, f72_records, f72_actual)
    require_all_pass("M65d FF /6 protection", "FF.6", 0xffff, ff6_records, ff6_actual)

    m65b_failures: list[dict[str, str]] = []
    for digest in m65b_hashes:
        outcome, signature = compare_case(
            digest, "62", int(official[digest]["flags_mask"], 16),
            bound_records[digest], bound_actual[digest]
        )
        if outcome != "pass":
            m65b_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})
    if m65b_failures:
        raise M65eError(f"M65b protection changed: {m65b_failures[:3]}")

    frame_failures: list[dict[str, str]] = []
    for digest in frame_hashes:
        outcome, signature = compare_case(digest, "62", 0xffff, bound_records[digest], bound_actual[digest])
        if outcome != "pass":
            frame_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})
    if frame_failures:
        raise M65eError(f"BOUND frame protection changed: {frame_failures[:3]}")

    m65e_failures: list[dict[str, str]] = []
    for digest in m65e_hashes:
        form = official[digest]["form"]
        outcome, signature = compare_case(
            digest, form, int(official[digest]["flags_mask"], 16),
            tail_records[digest], first_tail[digest]
        )
        if outcome != "pass":
            m65e_failures.append(
                {
                    "case_hash": digest,
                    "form": form,
                    "outcome": outcome,
                    "signature": signature or "",
                }
            )
    if m65e_failures:
        raise M65eError(f"M65e tail still has failures: {m65e_failures[:3]}")

    return {
        "milestone": "M65e",
        "campaign_branch": "topic/m65-residue-campaign",
        "campaign_base_gate": "G65",
        "campaign_base_sha": "efd96b7e46717e7ee56e086f7d27ba42b04b49d3",
        "campaign_predecessor_sha": "ef44acbf5183ac5a8233ac007b07de72fd61eae8",
        "worker_sha256": worker_sha256,
        "dataset_id": DATASET_ID,
        "m65e": {
            "selector": "exact ten-case tail",
            "owned_count": M65E_COUNT,
            "owned_hash_set_sha256": M65E_DIGEST,
            "pass": M65E_COUNT,
            "fail": 0,
            "timeout": 0,
            "crash": 0,
            "pass_set_sha256": M65E_DIGEST,
        },
        "original_g65_architectural_residue": {
            "count": 7511,
            "pass": 7511,
            "fail": 0,
            "hash_set_sha256": reconstruct.hash_set_digest(all_original_failures),
        },
        "protected_m65a": {
            "selector": "FF /7",
            "count": M65A_COUNT,
            "hash_set_sha256": M65A_DIGEST,
            "pass": M65A_COUNT,
            "fail": 0,
        },
        "protected_m65b": {
            "selector": "62 BOUND",
            "count": M65B_COUNT,
            "hash_set_sha256": M65B_DIGEST,
            "pass": M65B_COUNT,
            "fail": 0,
        },
        "protected_bound_frame_only": {
            "selector": "62 BOUND former frame-only",
            "count": BOUND_FRAME_COUNT,
            "hash_set_sha256": BOUND_FRAME_DIGEST,
            "pass": BOUND_FRAME_COUNT,
            "fail": 0,
        },
        "protected_m65c": {
            "selector": "F7 /2 word NOT r/m16",
            "owned_count": M65C_COUNT,
            "owned_hash_set_sha256": M65C_DIGEST,
            "full_selected_count": FULL_F72_COUNT,
            "full_hash_set_sha256": FULL_F72_DIGEST,
            "pass": FULL_F72_COUNT,
            "fail": 0,
        },
        "protected_m65d": {
            "selector": "FF /6",
            "owned_count": M65D_COUNT,
            "owned_hash_set_sha256": M65D_DIGEST,
            "full_selected_count": FULL_FF6_COUNT,
            "full_hash_set_sha256": FULL_FF6_DIGEST,
            "pass": FULL_FF6_COUNT,
            "fail": 0,
        },
        "campaign_arithmetic": {
            "g65_architectural_full_pass": 1467083,
            "g65_architectural_full_fail": 7511,
            "cumulative_newly_passing": 7511,
            "expected_remaining_original_g65_failures": 0,
            "expected_architectural_full_pass_if_no_dependent_changes": 1474594,
            "expected_architectural_full_fail_if_no_dependent_changes": 0,
            "expected_architectural_full_applicable": 1474594,
        },
        "newly_passing": {
            "count": M65E_COUNT,
            "hash_set_sha256": M65E_DIGEST,
        },
        "newly_failing": [],
        "target_policy_changed": False,
        "selected_applicable_changed": False,
        "deterministic_replay": "byte_identical_normalized_tail_results",
    }


def write_summary(root: pathlib.Path, summary: dict[str, Any]) -> pathlib.Path:
    path = root / "tests/ssts/campaigns/g65m/checkpoints/m65e_replay_summary.json"
    reconstruct.write_json(path, summary)
    return path


def selftest() -> None:
    try:
        reconstruct.hash_set_digest(["duplicate", "duplicate"])
    except Exception:
        print("m65e tail10 selftest: pass")
        return
    raise M65eError("duplicate hash selftest did not fail")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("selftest")
    verify_parser = sub.add_parser("verify")
    verify_parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    verify_parser.add_argument("--shard-root", type=pathlib.Path, required=True)
    verify_parser.add_argument("--worker", type=pathlib.Path, required=True)
    verify_parser.add_argument("--write-summary", action="store_true")
    args = parser.parse_args()
    try:
        if args.command == "selftest":
            selftest()
        elif args.command == "verify":
            summary = validate(args.root.resolve(), args.shard_root.resolve(), args.worker.resolve())
            if args.write_summary:
                path = write_summary(args.root.resolve(), summary)
                print(f"m65e tail10 verify: pass summary={path}")
            else:
                print("m65e tail10 verify: pass")
    except M65eError as exc:
        print(f"m65e tail10 verify: FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

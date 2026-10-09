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
"""Validate the M65c F7 /2 word NOT campaign checkpoint."""

from __future__ import annotations

import argparse
import gzip
import json
import pathlib
import sys
from typing import Any

import upd9002_m65_reconstruct as reconstruct
import upd9002_ssts as ssts


ROOT = pathlib.Path(__file__).resolve().parents[2]
DATASET_ID = reconstruct.DATASET_ID
M65A_COUNT = 5000
M65A_DIGEST = "6028d5dcd4b6a3dcded2aaf69fb186e502f7f5a4d094180572f802c86240039a"
M65B_COUNT = 1244
M65B_DIGEST = "2fd0e1053b264042031c657ebf55796858e8ff2405509b3cc1d17ace71ae4f0d"
M65C_COUNT = 1113
M65C_DIGEST = "69bf316c8a0751f7aed67504d0ea606fd2530e8d254b2b4e73ead66ccbc30ccc"
M65D_COUNT = 144
M65D_DIGEST = "ce1bc644ee5a5bc73ae872440ad4446cb0dbccbad626ba93372082fe7add9076"
M65E_COUNT = 10
M65E_DIGEST = "7b228418bf0391884381514282e60ea9ccaf3af8c0f1f7f5a1b038a24de230a1"
BOUND_FRAME_COUNT = 3565
BOUND_FRAME_DIGEST = "15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746"
FULL_F72_COUNT = 5000


class M65cError(RuntimeError):
    """The M65c checkpoint failed closed."""


def read_gzip_json(path: pathlib.Path) -> Any:
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        return json.load(stream)


def load_case_hashes(root: pathlib.Path, filename: str, count: int, digest: str) -> list[str]:
    rows = read_gzip_json(root / "tests/ssts/campaigns/g65m/reconstruction" / filename)
    hashes = [row["case_hash"] for row in rows]
    if len(hashes) != count:
        raise M65cError(f"{filename}: count drifted")
    actual_digest = reconstruct.hash_set_digest(hashes)
    if actual_digest != digest:
        raise M65cError(f"{filename}: digest drifted {actual_digest}")
    return sorted(hashes)


def load_bound_frame_hashes(root: pathlib.Path) -> list[str]:
    rows = read_gzip_json(root / "tests/ssts/evidence/g60d/synchronous_frame_cases.json.gz")["rows"]
    hashes = sorted(
        row["case_hash"]
        for row in rows
        if row["form"] == "62"
        and row["architectural_outcome"] == "pass"
        and row["expected_event"]["active"]
    )
    if len(hashes) != BOUND_FRAME_COUNT:
        raise M65cError("BOUND frame-only count drifted")
    actual_digest = reconstruct.hash_set_digest(hashes)
    if actual_digest != BOUND_FRAME_DIGEST:
        raise M65cError(f"BOUND frame-only digest drifted {actual_digest}")
    return hashes


def verify_shard_identity(root: pathlib.Path, shard_root: pathlib.Path, form: str) -> pathlib.Path:
    manifest = reconstruct.read_json(root / "tests/ssts/v20_dataset_manifest.json")
    expected = {
        pathlib.PurePosixPath(item["path"]).name: item
        for item in manifest["files"]
    }
    path = shard_root / f"{form}.json.gz"
    entry = expected.get(path.name)
    if entry is None or not path.is_file():
        raise M65cError(f"{form}: corpus shard missing from manifest or filesystem")
    if path.stat().st_size != entry["size"] or reconstruct.sha256_file(path) != entry["sha256"]:
        raise M65cError(f"{form}: corpus shard identity mismatch")
    return path


def load_all_records(
    root: pathlib.Path, shard_root: pathlib.Path, form: str
) -> dict[str, dict[str, Any]]:
    path = verify_shard_identity(root, shard_root, form)
    records: dict[str, dict[str, Any]] = {}
    for record in read_gzip_json(path):
        digest = reconstruct.sha256_bytes(reconstruct.canonical_bytes(record))
        if digest in records:
            raise M65cError(f"{form}: duplicate corpus record digest {digest}")
        records[digest] = record
    return records


def load_records_for_hashes(
    root: pathlib.Path, shard_root: pathlib.Path, form: str, hashes: list[str]
) -> dict[str, dict[str, Any]]:
    wanted = set(hashes)
    records = load_all_records(root, shard_root, form)
    result = {digest: record for digest, record in records.items() if digest in wanted}
    if set(result) != wanted:
        raise M65cError(f"{form}: selected corpus records are incomplete")
    return result


def compare_case(
    record_hash: str,
    form: str,
    flags_mask: int,
    record: dict[str, Any],
    actual: dict[str, Any],
) -> tuple[str, str | None]:
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
    if failure is None:
        return outcome, None
    return outcome, failure["signature_sha256"]


def run_records(worker: pathlib.Path, records: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    ordered = [records[digest] for digest in sorted(records)]
    contained = ssts.run_worker_contained(worker, ordered, timeout=120.0)
    if len(contained) != len(ordered):
        raise M65cError("worker result count mismatch")
    result: dict[str, dict[str, Any]] = {}
    for record, (status, actual) in zip(ordered, contained):
        digest = reconstruct.sha256_bytes(reconstruct.canonical_bytes(record))
        if status != "ok" or actual is None:
            raise M65cError(f"{digest}: worker returned {status}")
        result[digest] = actual
    return result


def validate(root: pathlib.Path, shard_root: pathlib.Path, worker: pathlib.Path) -> dict[str, Any]:
    official = reconstruct.load_official_rows(root)
    worker_sha256 = reconstruct.sha256_file(worker)

    m65a_hashes = load_case_hashes(root, "m65a_ff7_cases.json.gz", M65A_COUNT, M65A_DIGEST)
    m65b_hashes = load_case_hashes(root, "m65b_bound_cases.json.gz", M65B_COUNT, M65B_DIGEST)
    m65c_hashes = load_case_hashes(root, "m65c_f7_not_cases.json.gz", M65C_COUNT, M65C_DIGEST)
    m65d_hashes = load_case_hashes(root, "m65d_ff6_cases.json.gz", M65D_COUNT, M65D_DIGEST)
    m65e_hashes = load_case_hashes(root, "m65e_tail_cases.json.gz", M65E_COUNT, M65E_DIGEST)
    frame_hashes = load_bound_frame_hashes(root)

    if set(m65c_hashes) & set(m65a_hashes + m65b_hashes + m65d_hashes + m65e_hashes):
        raise M65cError("M65c ownership overlaps another campaign task")

    f72_records = {
        digest: record
        for digest, record in load_all_records(root, shard_root, "F7.2").items()
        if not record["initial"]["queue"]
    }
    if len(f72_records) != FULL_F72_COUNT:
        raise M65cError("F7 /2 selected population count drifted")
    full_f72_digest = reconstruct.hash_set_digest(list(f72_records))
    if not set(m65c_hashes).issubset(f72_records):
        raise M65cError("M65c owned hashes are not a subset of F7 /2 selected population")

    m65a_records = load_records_for_hashes(root, shard_root, "FF.7", m65a_hashes)
    m65d_records = load_records_for_hashes(root, shard_root, "FF.6", m65d_hashes)
    bound_records = load_records_for_hashes(
        root, shard_root, "62", sorted(set(m65b_hashes) | set(frame_hashes))
    )

    tail_records: dict[str, dict[str, Any]] = {}
    by_form: dict[str, list[str]] = {}
    for digest in m65e_hashes:
        by_form.setdefault(official[digest]["form"], []).append(digest)
    for form, hashes in sorted(by_form.items()):
        tail_records.update(load_records_for_hashes(root, shard_root, form, sorted(hashes)))

    first_f72 = run_records(worker, f72_records)
    second_f72 = run_records(worker, f72_records)
    if first_f72 != second_f72:
        raise M65cError("F7 /2 full-population replay is nondeterministic")
    m65a_actual = run_records(worker, m65a_records)
    bound_actual = run_records(worker, bound_records)
    m65d_actual = run_records(worker, m65d_records)
    tail_actual = run_records(worker, tail_records)

    full_f72_failures: list[dict[str, str]] = []
    m65c_failures: list[dict[str, str]] = []
    predecessor_pass_regressions: list[dict[str, str]] = []
    for digest in sorted(f72_records):
        outcome, signature = compare_case(digest, "F7.2", 0xffff, f72_records[digest], first_f72[digest])
        if outcome != "pass":
            entry = {"case_hash": digest, "outcome": outcome, "signature": signature or ""}
            full_f72_failures.append(entry)
            if digest in m65c_hashes:
                m65c_failures.append(entry)
            else:
                predecessor_pass_regressions.append(entry)

    m65a_failures: list[dict[str, str]] = []
    for digest in m65a_hashes:
        outcome, signature = compare_case(
            digest, "FF.7", int(official[digest]["flags_mask"], 16),
            m65a_records[digest], m65a_actual[digest]
        )
        if outcome != "pass":
            m65a_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})

    m65b_failures: list[dict[str, str]] = []
    for digest in m65b_hashes:
        outcome, signature = compare_case(
            digest, "62", int(official[digest]["flags_mask"], 16),
            bound_records[digest], bound_actual[digest]
        )
        if outcome != "pass":
            m65b_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})

    frame_failures: list[dict[str, str]] = []
    for digest in frame_hashes:
        outcome, signature = compare_case(digest, "62", 0xffff, bound_records[digest], bound_actual[digest])
        if outcome != "pass":
            frame_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})

    m65d_signature_mismatches: list[dict[str, str]] = []
    for digest in m65d_hashes:
        outcome, signature = compare_case(
            digest, "FF.6", int(official[digest]["flags_mask"], 16),
            m65d_records[digest], m65d_actual[digest]
        )
        if outcome != "semantic_failure" or signature != official[digest]["signature_sha256"]:
            m65d_signature_mismatches.append(
                {
                    "case_hash": digest,
                    "outcome": outcome,
                    "signature": signature or "",
                    "official": official[digest]["signature_sha256"],
                }
            )

    tail_signature_mismatches: list[dict[str, str]] = []
    for digest in m65e_hashes:
        form = official[digest]["form"]
        outcome, signature = compare_case(
            digest, form, int(official[digest]["flags_mask"], 16),
            tail_records[digest], tail_actual[digest]
        )
        if outcome != "semantic_failure" or signature != official[digest]["signature_sha256"]:
            tail_signature_mismatches.append(
                {
                    "case_hash": digest,
                    "form": form,
                    "outcome": outcome,
                    "signature": signature or "",
                    "official": official[digest]["signature_sha256"],
                }
            )

    if m65c_failures:
        raise M65cError(f"M65c still has failures: {m65c_failures[:3]}")
    if predecessor_pass_regressions:
        raise M65cError(f"F7 /2 predecessor pass regressed: {predecessor_pass_regressions[:3]}")
    if full_f72_failures:
        raise M65cError(f"F7 /2 full population has failures: {full_f72_failures[:3]}")
    if m65a_failures:
        raise M65cError(f"M65a protection changed: {m65a_failures[:3]}")
    if m65b_failures:
        raise M65cError(f"M65b protection changed: {m65b_failures[:3]}")
    if frame_failures:
        raise M65cError(f"BOUND frame protection changed: {frame_failures[:3]}")
    if m65d_signature_mismatches:
        raise M65cError(f"M65d guard changed: {m65d_signature_mismatches[:3]}")
    if tail_signature_mismatches:
        raise M65cError(f"M65e tail guard changed: {tail_signature_mismatches[:3]}")

    return {
        "milestone": "M65c",
        "campaign_branch": "topic/m65-residue-campaign",
        "campaign_base_gate": "G65",
        "campaign_base_sha": "efd96b7e46717e7ee56e086f7d27ba42b04b49d3",
        "campaign_predecessor_sha": "e5ff4fda663156836d327314df28dd48c2006668",
        "worker_sha256": worker_sha256,
        "dataset_id": DATASET_ID,
        "m65c": {
            "selector": "F7 /2 word NOT r/m16",
            "owned_count": M65C_COUNT,
            "owned_hash_set_sha256": M65C_DIGEST,
            "pass": M65C_COUNT,
            "fail": 0,
            "timeout": 0,
            "crash": 0,
            "pass_set_sha256": M65C_DIGEST,
        },
        "full_f7_2": {
            "selected_count": FULL_F72_COUNT,
            "hash_set_sha256": full_f72_digest,
            "pass": FULL_F72_COUNT,
            "fail": 0,
            "predecessor_passes_preserved": FULL_F72_COUNT - M65C_COUNT,
            "owned_failures_newly_passing": M65C_COUNT,
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
        "protected_m65d_guard": {
            "selector": "FF /6 SP alias",
            "count": M65D_COUNT,
            "hash_set_sha256": M65D_DIGEST,
            "official_failures_preserved": M65D_COUNT,
        },
        "protected_m65e_tail_guard": {
            "selector": "exact ten-case tail",
            "count": M65E_COUNT,
            "hash_set_sha256": M65E_DIGEST,
            "official_failures_preserved": M65E_COUNT,
        },
        "campaign_arithmetic": {
            "g65_architectural_full_pass": 1467083,
            "g65_architectural_full_fail": 7511,
            "cumulative_newly_passing": 7357,
            "expected_remaining_original_g65_failures": 154,
            "expected_remaining_m65d_failures": M65D_COUNT,
            "expected_remaining_m65e_failures": M65E_COUNT,
            "expected_architectural_full_pass_if_no_dependent_changes": 1474440,
            "expected_architectural_full_fail_if_no_dependent_changes": 154,
            "expected_architectural_full_applicable": 1474594,
        },
        "newly_passing": {
            "count": M65C_COUNT,
            "hash_set_sha256": M65C_DIGEST,
        },
        "newly_failing": [],
        "target_policy_changed": False,
        "selected_applicable_changed": False,
        "deterministic_replay": "byte_identical_normalized_f7_2_results",
    }


def write_summary(root: pathlib.Path, summary: dict[str, Any]) -> pathlib.Path:
    path = root / "tests/ssts/campaigns/g65m/checkpoints/m65c_replay_summary.json"
    reconstruct.write_json(path, summary)
    return path


def selftest() -> None:
    try:
        reconstruct.hash_set_digest(["duplicate", "duplicate"])
    except Exception:
        print("m65c f72 selftest: pass")
        return
    raise M65cError("duplicate hash selftest did not fail")


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
                print(f"m65c f72 verify: pass summary={path}")
            else:
                print("m65c f72 verify: pass")
    except M65cError as exc:
        print(f"m65c f72 verify: FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

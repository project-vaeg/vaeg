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
"""Validate the M65b BOUND campaign checkpoint."""

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
M65D_COUNT = 144
M65D_DIGEST = "ce1bc644ee5a5bc73ae872440ad4446cb0dbccbad626ba93372082fe7add9076"
BOUND_FRAME_COUNT = 3565
BOUND_FRAME_DIGEST = "15862f179608f8745f76bb3565197106ae6f63cba6c3363dd307fb29e6bbd746"


class M65bError(RuntimeError):
    """The M65b checkpoint failed closed."""


def read_gzip_json(path: pathlib.Path) -> Any:
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        return json.load(stream)


def load_case_hashes(root: pathlib.Path, filename: str, count: int, digest: str) -> list[str]:
    rows = read_gzip_json(root / "tests/ssts/campaigns/g65m/reconstruction" / filename)
    hashes = [row["case_hash"] for row in rows]
    if len(hashes) != count:
        raise M65bError(f"{filename}: count drifted")
    actual_digest = reconstruct.hash_set_digest(hashes)
    if actual_digest != digest:
        raise M65bError(f"{filename}: digest drifted {actual_digest}")
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
        raise M65bError("BOUND frame-only count drifted")
    actual_digest = reconstruct.hash_set_digest(hashes)
    if actual_digest != BOUND_FRAME_DIGEST:
        raise M65bError(f"BOUND frame-only digest drifted {actual_digest}")
    return hashes


def load_records_for_hashes(
    root: pathlib.Path, shard_root: pathlib.Path, form: str, hashes: list[str]
) -> dict[str, dict[str, Any]]:
    manifest = reconstruct.read_json(root / "tests/ssts/v20_dataset_manifest.json")
    expected = {
        pathlib.PurePosixPath(item["path"]).name: item
        for item in manifest["files"]
    }
    path = shard_root / f"{form}.json.gz"
    entry = expected.get(path.name)
    if entry is None or not path.is_file():
        raise M65bError(f"{form}: corpus shard missing from manifest or filesystem")
    if path.stat().st_size != entry["size"] or reconstruct.sha256_file(path) != entry["sha256"]:
        raise M65bError(f"{form}: corpus shard identity mismatch")
    wanted = set(hashes)
    result: dict[str, dict[str, Any]] = {}
    for record in read_gzip_json(path):
        digest = reconstruct.sha256_bytes(reconstruct.canonical_bytes(record))
        if digest in wanted:
            result[digest] = record
    if set(result) != wanted:
        raise M65bError(f"{form}: selected corpus records are incomplete")
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
        raise M65bError("worker result count mismatch")
    result: dict[str, dict[str, Any]] = {}
    for record, (status, actual) in zip(ordered, contained):
        digest = reconstruct.sha256_bytes(reconstruct.canonical_bytes(record))
        if status != "ok" or actual is None:
            raise M65bError(f"{digest}: worker returned {status}")
        result[digest] = actual
    return result


def validate(root: pathlib.Path, shard_root: pathlib.Path, worker: pathlib.Path) -> dict[str, Any]:
    official = reconstruct.load_official_rows(root)
    worker_sha256 = reconstruct.sha256_file(worker)
    m65a_hashes = load_case_hashes(root, "m65a_ff7_cases.json.gz", M65A_COUNT, M65A_DIGEST)
    m65b_hashes = load_case_hashes(root, "m65b_bound_cases.json.gz", M65B_COUNT, M65B_DIGEST)
    m65d_hashes = load_case_hashes(root, "m65d_ff6_cases.json.gz", M65D_COUNT, M65D_DIGEST)
    frame_hashes = load_bound_frame_hashes(root)

    m65a_records = load_records_for_hashes(root, shard_root, "FF.7", m65a_hashes)
    m65d_records = load_records_for_hashes(root, shard_root, "FF.6", m65d_hashes)
    bound_records = load_records_for_hashes(
        root, shard_root, "62", sorted(set(m65b_hashes) | set(frame_hashes))
    )

    first_bound = run_records(worker, bound_records)
    second_bound = run_records(worker, bound_records)
    if first_bound != second_bound:
        raise M65bError("BOUND replay is nondeterministic")
    m65a_actual = run_records(worker, m65a_records)
    m65d_actual = run_records(worker, m65d_records)

    m65b_failures: list[dict[str, str]] = []
    for digest in m65b_hashes:
        outcome, signature = compare_case(
            digest, "62", int(official[digest]["flags_mask"], 16),
            bound_records[digest], first_bound[digest]
        )
        if outcome != "pass":
            m65b_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})

    frame_failures: list[dict[str, str]] = []
    for digest in frame_hashes:
        outcome, signature = compare_case(
            digest, "62", 0xffff, bound_records[digest], first_bound[digest]
        )
        if outcome != "pass":
            frame_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})

    m65a_failures: list[dict[str, str]] = []
    for digest in m65a_hashes:
        outcome, signature = compare_case(
            digest, "FF.7", int(official[digest]["flags_mask"], 16),
            m65a_records[digest], m65a_actual[digest]
        )
        if outcome != "pass":
            m65a_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})

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

    if m65b_failures:
        raise M65bError(f"M65b still has failures: {m65b_failures[:3]}")
    if frame_failures:
        raise M65bError(f"BOUND frame protection changed: {frame_failures[:3]}")
    if m65a_failures:
        raise M65bError(f"M65a protection changed: {m65a_failures[:3]}")
    if m65d_signature_mismatches:
        raise M65bError(f"M65d guard changed: {m65d_signature_mismatches[:3]}")

    return {
        "milestone": "M65b",
        "campaign_branch": "topic/m65-residue-campaign",
        "campaign_base_gate": "G65",
        "campaign_base_sha": "efd96b7e46717e7ee56e086f7d27ba42b04b49d3",
        "campaign_predecessor_sha": "057489a98aac5f976b82530916d15c73541036a5",
        "worker_sha256": worker_sha256,
        "dataset_id": DATASET_ID,
        "m65b": {
            "selector": "62 BOUND",
            "count": M65B_COUNT,
            "hash_set_sha256": M65B_DIGEST,
            "pass": M65B_COUNT,
            "fail": 0,
            "timeout": 0,
            "crash": 0,
            "pass_set_sha256": M65B_DIGEST,
        },
        "protected_m65a": {
            "selector": "FF /7",
            "count": M65A_COUNT,
            "hash_set_sha256": M65A_DIGEST,
            "pass": M65A_COUNT,
            "fail": 0,
        },
        "protected_m65d_guard": {
            "selector": "FF /6 SP alias",
            "count": M65D_COUNT,
            "hash_set_sha256": M65D_DIGEST,
            "official_failures_preserved": M65D_COUNT,
        },
        "protected_bound_frame_only": {
            "selector": "62 BOUND former frame-only",
            "count": BOUND_FRAME_COUNT,
            "hash_set_sha256": BOUND_FRAME_DIGEST,
            "pass": BOUND_FRAME_COUNT,
            "fail": 0,
        },
        "newly_passing": {
            "count": M65B_COUNT,
            "hash_set_sha256": M65B_DIGEST,
        },
        "newly_failing": [],
        "target_policy_changed": False,
        "selected_applicable_changed": False,
        "deterministic_replay": "byte_identical_normalized_bound_results",
    }


def write_summary(root: pathlib.Path, summary: dict[str, Any]) -> pathlib.Path:
    path = root / "tests/ssts/campaigns/g65m/checkpoints/m65b_replay_summary.json"
    reconstruct.write_json(path, summary)
    return path


def selftest() -> None:
    try:
        reconstruct.hash_set_digest(["duplicate", "duplicate"])
    except Exception:
        print("m65b bound selftest: pass")
        return
    raise M65bError("duplicate hash selftest did not fail")


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
                print(f"m65b bound verify: pass summary={path}")
            else:
                print("m65b bound verify: pass")
    except M65bError as exc:
        print(f"m65b bound verify: FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

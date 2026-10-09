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
"""Validate the M65d FF /6 campaign checkpoint."""

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
FULL_F72_DIGEST = "ff7c9f1988e9e9b1f73309501059c84fdbaf545604f942bfd533aed8ab987df6"
FULL_FF6_COUNT = 5000
FULL_FF6_DIGEST = "a2b73b7e4b6e53dc95214b0a384ce4f7549c58c0364dd1b30ab22be5fc27b67a"


class M65dError(RuntimeError):
    """The M65d checkpoint failed closed."""


def read_gzip_json(path: pathlib.Path) -> Any:
    with gzip.open(path, "rt", encoding="utf-8") as stream:
        return json.load(stream)


def load_case_hashes(root: pathlib.Path, filename: str, count: int, digest: str) -> list[str]:
    rows = read_gzip_json(root / "tests/ssts/campaigns/g65m/reconstruction" / filename)
    hashes = [row["case_hash"] for row in rows]
    if len(hashes) != count:
        raise M65dError(f"{filename}: count drifted")
    actual_digest = reconstruct.hash_set_digest(hashes)
    if actual_digest != digest:
        raise M65dError(f"{filename}: digest drifted {actual_digest}")
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
        raise M65dError("BOUND frame-only count drifted")
    actual_digest = reconstruct.hash_set_digest(hashes)
    if actual_digest != BOUND_FRAME_DIGEST:
        raise M65dError(f"BOUND frame-only digest drifted {actual_digest}")
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
        raise M65dError(f"{form}: corpus shard missing from manifest or filesystem")
    if path.stat().st_size != entry["size"] or reconstruct.sha256_file(path) != entry["sha256"]:
        raise M65dError(f"{form}: corpus shard identity mismatch")
    return path


def load_all_records(
    root: pathlib.Path, shard_root: pathlib.Path, form: str
) -> dict[str, dict[str, Any]]:
    path = verify_shard_identity(root, shard_root, form)
    records: dict[str, dict[str, Any]] = {}
    for record in read_gzip_json(path):
        digest = reconstruct.sha256_bytes(reconstruct.canonical_bytes(record))
        if digest in records:
            raise M65dError(f"{form}: duplicate corpus record digest {digest}")
        records[digest] = record
    return records


def load_records_for_hashes(
    root: pathlib.Path, shard_root: pathlib.Path, form: str, hashes: list[str]
) -> dict[str, dict[str, Any]]:
    wanted = set(hashes)
    records = load_all_records(root, shard_root, form)
    result = {digest: record for digest, record in records.items() if digest in wanted}
    if set(result) != wanted:
        raise M65dError(f"{form}: selected corpus records are incomplete")
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
        raise M65dError("worker result count mismatch")
    result: dict[str, dict[str, Any]] = {}
    for record, (status, actual) in zip(ordered, contained):
        digest = reconstruct.sha256_bytes(reconstruct.canonical_bytes(record))
        if status != "ok" or actual is None:
            raise M65dError(f"{digest}: worker returned {status}")
        result[digest] = actual
    return result


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
        raise M65dError(f"{label} has failures: {failures[:3]}")


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
                raise M65dError("campaign ownership overlap detected")

    ff6_records = {
        digest: record
        for digest, record in load_all_records(root, shard_root, "FF.6").items()
        if not record["initial"]["queue"]
    }
    if len(ff6_records) != FULL_FF6_COUNT:
        raise M65dError("FF /6 selected population count drifted")
    full_ff6_digest = reconstruct.hash_set_digest(list(ff6_records))
    if full_ff6_digest != FULL_FF6_DIGEST:
        raise M65dError(f"FF /6 selected digest drifted {full_ff6_digest}")
    if not set(m65d_hashes).issubset(ff6_records):
        raise M65dError("M65d owned hashes are not a subset of FF /6 selected population")

    f72_records = {
        digest: record
        for digest, record in load_all_records(root, shard_root, "F7.2").items()
        if not record["initial"]["queue"]
    }
    if len(f72_records) != FULL_F72_COUNT:
        raise M65dError("F7 /2 selected population count drifted")
    full_f72_digest = reconstruct.hash_set_digest(list(f72_records))
    if full_f72_digest != FULL_F72_DIGEST:
        raise M65dError(f"F7 /2 selected digest drifted {full_f72_digest}")

    m65a_records = load_records_for_hashes(root, shard_root, "FF.7", m65a_hashes)
    bound_records = load_records_for_hashes(
        root, shard_root, "62", sorted(set(m65b_hashes) | set(frame_hashes))
    )

    tail_records: dict[str, dict[str, Any]] = {}
    by_form: dict[str, list[str]] = {}
    for digest in m65e_hashes:
        by_form.setdefault(official[digest]["form"], []).append(digest)
    for form, hashes in sorted(by_form.items()):
        tail_records.update(load_records_for_hashes(root, shard_root, form, sorted(hashes)))

    first_ff6 = run_records(worker, ff6_records)
    second_ff6 = run_records(worker, ff6_records)
    if first_ff6 != second_ff6:
        raise M65dError("FF /6 full-population replay is nondeterministic")
    m65a_actual = run_records(worker, m65a_records)
    bound_actual = run_records(worker, bound_records)
    f72_actual = run_records(worker, f72_records)
    tail_actual = run_records(worker, tail_records)

    require_all_pass("FF /6 full population", "FF.6", 0xffff, ff6_records, first_ff6)
    require_all_pass("M65a FF /7 protection", "FF.7", 0xffff, m65a_records, m65a_actual)
    require_all_pass("M65c F7 /2 protection", "F7.2", 0xffff, f72_records, f72_actual)

    m65b_failures: list[dict[str, str]] = []
    for digest in m65b_hashes:
        outcome, signature = compare_case(
            digest, "62", int(official[digest]["flags_mask"], 16),
            bound_records[digest], bound_actual[digest]
        )
        if outcome != "pass":
            m65b_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})
    if m65b_failures:
        raise M65dError(f"M65b protection changed: {m65b_failures[:3]}")

    frame_failures: list[dict[str, str]] = []
    for digest in frame_hashes:
        outcome, signature = compare_case(digest, "62", 0xffff, bound_records[digest], bound_actual[digest])
        if outcome != "pass":
            frame_failures.append({"case_hash": digest, "outcome": outcome, "signature": signature or ""})
    if frame_failures:
        raise M65dError(f"BOUND frame protection changed: {frame_failures[:3]}")

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
    if tail_signature_mismatches:
        raise M65dError(f"M65e tail guard changed: {tail_signature_mismatches[:3]}")

    return {
        "milestone": "M65d",
        "campaign_branch": "topic/m65-residue-campaign",
        "campaign_base_gate": "G65",
        "campaign_base_sha": "efd96b7e46717e7ee56e086f7d27ba42b04b49d3",
        "campaign_predecessor_sha": "ef7d88938944532606c46bf1d6032ccdfd635c6a",
        "worker_sha256": worker_sha256,
        "dataset_id": DATASET_ID,
        "m65d": {
            "selector": "FF /6",
            "owned_count": M65D_COUNT,
            "owned_hash_set_sha256": M65D_DIGEST,
            "pass": M65D_COUNT,
            "fail": 0,
            "timeout": 0,
            "crash": 0,
            "pass_set_sha256": M65D_DIGEST,
        },
        "full_ff_6": {
            "selected_count": FULL_FF6_COUNT,
            "hash_set_sha256": full_ff6_digest,
            "pass": FULL_FF6_COUNT,
            "fail": 0,
            "predecessor_passes_preserved": FULL_FF6_COUNT - M65D_COUNT,
            "owned_failures_newly_passing": M65D_COUNT,
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
            "full_hash_set_sha256": full_f72_digest,
            "pass": FULL_F72_COUNT,
            "fail": 0,
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
            "cumulative_newly_passing": 7501,
            "expected_remaining_original_g65_failures": 10,
            "expected_remaining_m65e_failures": M65E_COUNT,
            "expected_architectural_full_pass_if_no_dependent_changes": 1474584,
            "expected_architectural_full_fail_if_no_dependent_changes": 10,
            "expected_architectural_full_applicable": 1474594,
        },
        "newly_passing": {
            "count": M65D_COUNT,
            "hash_set_sha256": M65D_DIGEST,
        },
        "newly_failing": [],
        "target_policy_changed": False,
        "selected_applicable_changed": False,
        "deterministic_replay": "byte_identical_normalized_ff_6_results",
    }


def write_summary(root: pathlib.Path, summary: dict[str, Any]) -> pathlib.Path:
    path = root / "tests/ssts/campaigns/g65m/checkpoints/m65d_replay_summary.json"
    reconstruct.write_json(path, summary)
    return path


def selftest() -> None:
    try:
        reconstruct.hash_set_digest(["duplicate", "duplicate"])
    except Exception:
        print("m65d ff6 selftest: pass")
        return
    raise M65dError("duplicate hash selftest did not fail")


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
                print(f"m65d ff6 verify: pass summary={path}")
            else:
                print("m65d ff6 verify: pass")
    except M65dError as exc:
        print(f"m65d ff6 verify: FAIL: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

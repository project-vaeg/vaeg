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
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

"""Run hosted G65m architectural CI against committed campaign evidence."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys
from typing import Any

import upd9002_m64_expanded as m64
import upd9002_ssts as ssts


CAMPAIGN_MANIFEST = pathlib.Path("tests/ssts/campaigns/g65m/manifest.json")
ARCHITECTURAL_CI_SCOREBOARD = pathlib.Path(
    "tests/ssts/scoreboard/g65m_architectural_ci.json"
)
G65M_GATE = "G65m"
G65_BASE_SHA = "efd96b7e46717e7ee56e086f7d27ba42b04b49d3"


class G65mCIError(RuntimeError):
    """A hosted G65m campaign CI failure."""


def require_checkout(root: pathlib.Path, checkout_sha: str) -> None:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0 or completed.stdout.strip() != checkout_sha:
        raise G65mCIError("configured checkout SHA does not match HEAD")


def artifact_for(manifest: dict[str, Any], path: pathlib.Path) -> dict[str, Any]:
    normalized = path.as_posix()
    for artifact in manifest.get("artifacts", []):
        if artifact.get("path") == normalized:
            return artifact
    raise G65mCIError(f"campaign manifest does not record {normalized}")


def verify_campaign_identity(root: pathlib.Path) -> dict[str, Any]:
    manifest = m64.read_json(root / CAMPAIGN_MANIFEST)
    if (
        manifest.get("candidate_gate") != G65M_GATE
        or manifest.get("approved_base_gate") != "G65"
        or manifest.get("approved_base_sha") != G65_BASE_SHA
        or manifest.get("milestone") != "M65m"
    ):
        raise G65mCIError("committed G65m campaign identity differs")

    policy = m64.read_json(root / m64.TARGET_POLICY_PATH)
    if (
        manifest.get("target_policy_id") != policy.get("target_policy_id")
        or manifest.get("target_policy_sha256")
        != policy.get("target_policy_sha256")
    ):
        raise G65mCIError("committed G65m target policy identity differs")

    scoreboard_artifact = artifact_for(manifest, ARCHITECTURAL_CI_SCOREBOARD)
    if (
        scoreboard_artifact.get("sha256")
        != m64.sha256_file(root / ARCHITECTURAL_CI_SCOREBOARD)
    ):
        raise G65mCIError("committed G65m architectural CI scoreboard differs")

    committed = m64.read_json(root / ARCHITECTURAL_CI_SCOREBOARD)
    expected = manifest.get("profile_results", {}).get("architectural_ci")
    if expected != {
        "selected": committed.get("selected"),
        "applicable": committed.get("applicable"),
        "pass": committed.get("pass"),
        "fail": committed.get("fail"),
        "timeout": committed.get("timeouts"),
        "crash": committed.get("crashes"),
    }:
        raise G65mCIError("G65m manifest and architectural CI scoreboard differ")
    return manifest


def nonzero_counts(value: dict[str, int]) -> dict[str, int]:
    return {key: count for key, count in value.items() if count}


def result_fail_count(result_counts: dict[str, int]) -> int:
    return sum(
        result_counts.get(kind, 0)
        for kind in ("semantic_failure", "timeout", "crash")
    )


def verify_raw_against_committed(
    raw: dict[str, Any], committed: dict[str, Any]
) -> None:
    if (
        raw.get("schema") != "vaeg-upd9002-ssts-result-v1"
        or raw.get("dataset_id") != committed.get("dataset_id")
        or raw.get("profile") != "ci"
    ):
        raise G65mCIError("raw architectural CI identity differs")

    result_counts = raw.get("result_counts", {})
    if (
        raw.get("selected_records") != committed.get("selected")
        or raw.get("executed_records") != committed.get("executed")
        or result_counts.get("pass", 0) != committed.get("pass")
        or result_fail_count(result_counts) != committed.get("fail")
        or result_counts.get("timeout", 0) != committed.get("timeouts")
        or result_counts.get("crash", 0) != committed.get("crashes")
    ):
        raise G65mCIError("raw architectural CI result counts differ")

    if nonzero_counts(raw.get("classification_counts", {})) != nonzero_counts(
        committed.get("classification_counts", {})
    ):
        raise G65mCIError("raw architectural CI classification counts differ")

    if (
        raw.get("failure_signature_index_sha256")
        != committed.get("failure_signature_index_sha256")
        or raw.get("failure_signature_count") != committed.get("fail")
        or bool(raw.get("failure_signature_files")) != bool(
            committed.get("failure_shards")
        )
    ):
        raise G65mCIError("raw architectural CI failure signatures differ")

    raw_forms = {row["form"]: row for row in raw.get("per_form", [])}
    for record in committed.get("records", []):
        raw_record = raw_forms.get(record["form"])
        if raw_record is None:
            raise G65mCIError(f"raw architectural CI form missing: {record['form']}")
        class_counts = raw_record.get("classification_counts", {})
        form_results = raw_record.get("result_counts", {})
        form_fail = result_fail_count(form_results)
        if record["classification"] == "applicable":
            expected_selected = class_counts.get("applicable", 0)
            if (
                record.get("selected") != expected_selected
                or record.get("executed")
                != form_results.get("pass", 0) + form_fail
                or record.get("pass") != form_results.get("pass", 0)
                or record.get("fail") != form_fail
            ):
                raise G65mCIError(
                    f"raw architectural CI applicable form differs: {record['form']}"
                )
        else:
            expected_selected = class_counts.get(record["classification"], 0)
            if (
                record.get("selected") != expected_selected
                or record.get("executed") != 0
                or record.get("pass") != 0
                or record.get("fail") != 0
            ):
                raise G65mCIError(
                    f"raw architectural CI skipped form differs: {record['form']}"
                )

    if committed.get("fail") == 0 and (
        committed.get("failure_hash_set_sha256")
        != "4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945"
        or committed.get("failure_shards")
    ):
        raise G65mCIError("committed G65m architectural CI is not zero-failure")


def run(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    checkout_sha: str,
    output_root: pathlib.Path,
) -> None:
    require_checkout(root, checkout_sha)
    manifest = verify_campaign_identity(root)
    policy = m64.read_json(root / m64.TARGET_POLICY_PATH)

    output_root.mkdir(parents=True, exist_ok=True)
    raw = output_root / "g65m_architectural_ci.json"
    raw_failures = output_root / "g65m_architectural_ci_raw_failures"
    dataset_manifest = ssts.load_manifest(root / m64.DATASET_MANIFEST_PATH)
    with m64.support_map(root, "g64") as support_map:
        profile = ssts.run_profile(
            dataset_root,
            dataset_manifest,
            support_map,
            worker,
            "ci",
            300.0,
            "defined",
        )
    ssts.externalize_failure_signatures(profile, raw_failures)
    m64.write_json(raw, profile)

    committed = m64.read_json(root / ARCHITECTURAL_CI_SCOREBOARD)
    verify_raw_against_committed(profile, committed)

    print(
        "g65m-ci: architectural CI exact "
        f"selected={committed['selected']} executed={committed['executed']} "
        f"pass={committed['pass']} fail={committed['fail']} "
        f"timeout={committed['timeouts']} crash={committed['crashes']} "
        f"policy={committed['target_policy_id']}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--dataset-root", type=pathlib.Path, required=True)
    parser.add_argument("--worker", type=pathlib.Path, required=True)
    parser.add_argument("--evaluated-sha", required=True)
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    arguments = parser.parse_args()
    try:
        run(
            arguments.root.resolve(),
            arguments.dataset_root.resolve(),
            arguments.worker.resolve(),
            arguments.evaluated_sha,
            arguments.output_root.resolve(),
        )
    except (
        G65mCIError,
        m64.M64Error,
        ssts.CorpusError,
        OSError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"g65m-ci-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

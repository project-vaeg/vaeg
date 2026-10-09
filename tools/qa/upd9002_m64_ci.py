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
"""Run hosted M64 architectural CI against committed G64 evidence."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys
import tempfile

import upd9002_m60e_ci as m60e_ci
import upd9002_m64_expanded as m64
import upd9002_ssts as ssts


class M64CIError(RuntimeError):
    """A hosted M64 gate failure."""


def require_checkout(root: pathlib.Path, checkout_sha: str) -> None:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0 or completed.stdout.strip() != checkout_sha:
        raise M64CIError("configured checkout SHA does not match HEAD")


def run(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    checkout_sha: str,
    output_root: pathlib.Path,
) -> None:
    require_checkout(root, checkout_sha)
    evidence = m64.read_json(root / m64.EVIDENCE_ROOT / "manifest.json")
    result_manifest = m64.read_json(root / m64.RESULT_MANIFEST_PATH)
    policy = m64.read_json(root / m64.TARGET_POLICY_PATH)
    semantic_sha = evidence.get("evaluated_sha")
    if (
        evidence.get("candidate_gate") != m64.CANDIDATE_GATE
        or evidence.get("milestone") != m64.MILESTONE
        or result_manifest.get("candidate_gate") != m64.CANDIDATE_GATE
        or result_manifest.get("evaluated_sha") != semantic_sha
        or evidence.get("target_policy_after_id")
        != policy.get("target_policy_id")
        or result_manifest.get("target_policy_sha256")
        != policy.get("target_policy_sha256")
        or evidence.get("brkem_coverage") != m64.BRKEM_COVERAGE
    ):
        raise M64CIError("committed G64 evidence identity differs")
    derived_policy = m64.generate_target_policy(
        root, dataset_root, semantic_sha
    )
    if m64.canonical_bytes(derived_policy) != m64.canonical_bytes(policy):
        raise M64CIError("committed G64 target policy is not reproducible")

    output_root.mkdir(parents=True, exist_ok=True)
    raw = output_root / "g64_architectural_ci.json"
    raw_failures = output_root / "g64_architectural_ci_raw_failures"
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

    with tempfile.TemporaryDirectory(prefix="vaeg-m64-ci-") as name:
        candidate, _ = m64.generate_scoreboard(
            root,
            pathlib.Path(name),
            dataset_root,
            raw,
            "architectural_ci",
            semantic_sha,
            policy,
        )
    committed = m64.read_json(root / m64.SCOREBOARD_PATHS["architectural_ci"])
    if m60e_ci.stable_scoreboard_identity(
        candidate
    ) != m60e_ci.stable_scoreboard_identity(committed):
        raise M64CIError("architectural CI differs from committed G64 evidence")
    print(
        "m64-ci: architectural CI exact "
        f"selected={candidate['selected']} executed={candidate['executed']} "
        f"pass={candidate['pass']} fail={candidate['fail']} "
        f"timeout={candidate['timeouts']} crash={candidate['crashes']} "
        f"policy={candidate['target_policy_id']}"
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
        M64CIError,
        m64.M64Error,
        ssts.CorpusError,
        OSError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"m64-ci-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

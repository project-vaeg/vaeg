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

"""Run the hosted M60e architectural-CI profile against committed evidence."""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
import tempfile
from typing import Any

import upd9002_m60b_authority as m60b
import upd9002_m60e_iret as m60e


class M60eCIError(RuntimeError):
    """Raised when hosted M60e behavior differs from committed evidence."""


def read_json(path: pathlib.Path) -> Any:
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def stable_scoreboard_identity(value: dict[str, Any]) -> dict[str, Any]:
    return {
        "applicable": value["applicable"],
        "applicable_hash_set_sha256": value["applicable_hash_set_sha256"],
        "classification_counts": value["classification_counts"],
        "comparison_contract_id": value["comparison_contract_id"],
        "comparison_contract_sha256": value["comparison_contract_sha256"],
        "crashes": value["crashes"],
        "dataset_id": value["dataset_id"],
        "executed": value["executed"],
        "fail": value["fail"],
        "failure_hash_set_sha256": value["failure_hash_set_sha256"],
        "failure_shards": [
            {
                "canonical_sha256": row["canonical_sha256"],
                "failure_count": row["failure_count"],
                "path": row["path"],
            }
            for row in value["failure_shards"]
        ],
        "failure_sidecar_canonical_set_sha256": value[
            "failure_sidecar_canonical_set_sha256"
        ],
        "failure_signature_index_sha256": value[
            "failure_signature_index_sha256"
        ],
        "mismatch_classes": value["mismatch_classes"],
        "pass": value["pass"],
        "pass_hash_set_sha256": value["pass_hash_set_sha256"],
        "profile": value["profile"],
        "records": value["records"],
        "scope": value["scope"],
        "scoreboard_digest": value["scoreboard_digest"],
        "selected": value["selected"],
        "selected_hash_set_sha256": value["selected_hash_set_sha256"],
        "target_policy_id": value["target_policy_id"],
        "target_policy_sha256": value["target_policy_sha256"],
        "termination_classes": value["termination_classes"],
        "timeouts": value["timeouts"],
    }


def require_head(root: pathlib.Path, expected: str) -> None:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0 or completed.stdout.strip() != expected:
        raise M60eCIError("evaluated SHA is not the checked-out HEAD")


def run(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    evaluated_sha: str,
    output_root: pathlib.Path,
    raw_summary: pathlib.Path | None = None,
) -> None:
    require_head(root, evaluated_sha)
    manifest = read_json(root / "tests/ssts/evidence/g60e/manifest.json")
    if (
        manifest.get("candidate_gate") != "G60e"
        or manifest.get("evaluated_sha") != m60e.read_json(
            root / "tests/ssts/scoreboard/g60e_architectural_ci.json"
        ).get("evaluated_sha")
    ):
        raise M60eCIError("committed G60e evidence identity differs")

    output_root.mkdir(parents=True, exist_ok=True)
    if raw_summary is None:
        raw = output_root / "g60e_architectural_ci.json"
        failures = output_root / "g60e_architectural_ci_failures"
        m60b.run_profile(
            root,
            dataset_root,
            worker,
            "ci",
            "architectural",
            "g60b",
            raw,
            failures,
        )
    else:
        raw = raw_summary
    with tempfile.TemporaryDirectory(prefix="vaeg-m60e-ci-") as name:
        temporary = pathlib.Path(name)
        candidate, _ = m60e.generate_scoreboard(
            root,
            temporary,
            dataset_root,
            raw,
            "architectural_ci",
            manifest["evaluated_sha"],
        )
    committed = read_json(
        root / "tests/ssts/scoreboard/g60e_architectural_ci.json"
    )
    if stable_scoreboard_identity(candidate) != stable_scoreboard_identity(
        committed
    ):
        raise M60eCIError("architectural CI differs from committed G60e evidence")
    print(
        "m60e-ci: architectural CI exact "
        f"selected={candidate['selected']} executed={candidate['executed']} "
        f"pass={candidate['pass']} fail={candidate['fail']} "
        f"timeout={candidate['timeouts']} crash={candidate['crashes']}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--dataset-root", type=pathlib.Path, required=True)
    parser.add_argument("--worker", type=pathlib.Path, required=True)
    parser.add_argument("--evaluated-sha", required=True)
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    parser.add_argument("--raw-summary", type=pathlib.Path)
    arguments = parser.parse_args()
    try:
        run(
            arguments.root.resolve(),
            arguments.dataset_root.resolve(),
            arguments.worker.resolve(),
            arguments.evaluated_sha,
            arguments.output_root.resolve(),
            (
                arguments.raw_summary.resolve()
                if arguments.raw_summary is not None
                else None
            ),
        )
    except (
        M60eCIError,
        m60b.M60bError,
        m60e.M60eError,
        OSError,
        UnicodeError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"m60e-ci-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

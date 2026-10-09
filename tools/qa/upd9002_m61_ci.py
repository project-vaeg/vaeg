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
"""Run hosted M61 architectural CI against committed G61 evidence."""

from __future__ import annotations

import argparse
import pathlib
import subprocess
import sys
import tempfile

import upd9002_m60b_authority as m60b
import upd9002_m60e_ci as m60e_ci
import upd9002_m61_mov_imm as m61


class M61CIError(RuntimeError):
    """A hosted M61 gate failure."""


def run(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    checkout_sha: str,
    output_root: pathlib.Path,
) -> None:
    completed = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=root,
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode != 0 or completed.stdout.strip() != checkout_sha:
        raise M61CIError("configured checkout SHA does not match HEAD")
    manifest = m61.read_json(root / "tests/ssts/evidence/g61/manifest.json")
    if manifest.get("candidate_gate") != "G61":
        raise M61CIError("committed G61 manifest is missing")
    output_root.mkdir(parents=True, exist_ok=True)
    raw = output_root / "g61_architectural_ci.json"
    failures = output_root / "g61_architectural_ci_failures"
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
    with tempfile.TemporaryDirectory(prefix="vaeg-m61-ci-") as name:
        candidate, _ = m61.candidate_scoreboard(
            root,
            pathlib.Path(name),
            dataset_root,
            raw,
            "architectural_ci",
            manifest["evaluated_sha"],
        )
    committed = m61.read_json(
        root / "tests/ssts/scoreboard/g61_architectural_ci.json"
    )
    if m60e_ci.stable_scoreboard_identity(
        candidate
    ) != m60e_ci.stable_scoreboard_identity(committed):
        raise M61CIError("architectural CI differs from committed G61 evidence")
    print(
        "m61-ci: architectural CI exact "
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
        M61CIError,
        m61.M61Error,
        m60b.M60bError,
        OSError,
        ValueError,
        KeyError,
        TypeError,
    ) as error:
        print(f"m61-ci-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

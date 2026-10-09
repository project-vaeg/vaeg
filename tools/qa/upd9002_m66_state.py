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
"""Inventory and validate M66 state-format compatibility cleanup."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
from typing import Any


sys.dont_write_bytecode = True

SEARCH_TERMS = (
    "cpu286",
    "CPU286",
    "i286",
    "I286",
    "i286c",
    "I286C",
    "state286",
    "STATE286",
    "286 state",
    "legacy CPU state",
    "legacy processor state",
)

ACTIVE_STATE_PATHS = {
    "cpu/upd9002/upd9002_state.c",
    "cpu/upd9002/upd9002_state.h",
    "cpu/upd9002/cpucore.h",
    "machine/statsave.c",
    "machine/statsave.tbl",
}

CURRENT_TEST_PATHS = {
    "tests/upd9002/abi.c",
    "tests/upd9002/abi_g41.txt",
    "tests/upd9002/fixtures.c",
    "tests/upd9002/state_boundary.c",
    "tests/upd9002/state_fixtures_m42.txt",
    "tests/upd9002/state_payload_probe.c",
    "tests/upd9002/statsave_boundary.c",
}

HISTORICAL_PREFIXES = (
    "docs/agents/reports/",
    "tests/ssts/",
    "tools/qa/golden/",
)


class M66StateError(RuntimeError):
    """M66 state-format verification failed."""


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def run_git(root: pathlib.Path, args: list[str]) -> bytes:
    return subprocess.run(
        ["git", *args],
        cwd=root,
        check=True,
        capture_output=True,
    ).stdout


def tracked_paths(root: pathlib.Path, tree: str | None) -> list[str]:
    if tree is None:
        return sorted(
            path
            for path in run_git(root, ["ls-files", "-z"]).decode("utf-8").split("\0")
            if path
        )
    return sorted(
        path
        for path in run_git(root, ["ls-tree", "-r", "--name-only", "-z", tree])
        .decode("utf-8")
        .split("\0")
        if path
    )


def read_path(root: pathlib.Path, path: str, tree: str | None) -> bytes | None:
    try:
        if tree is None:
            return (root / path).read_bytes()
        return run_git(root, ["show", f"{tree}:{path}"])
    except (OSError, subprocess.CalledProcessError):
        return None


def classify(path: str, line: str, term: str) -> str:
    if path in ACTIVE_STATE_PATHS:
        if path == "machine/statsave.c" and "statflag_index_equals" in line:
            return "negative_test"
        if term in {
            "cpu286",
            "CPU286",
            "state286",
            "STATE286",
            "286 state",
            "legacy CPU state",
            "legacy processor state",
        }:
            return "active_cpu286_state_compat"
        return "current_upd9002_state"
    if path in CURRENT_TEST_PATHS:
        if "CPU286" in line and (
            "write_section_index" in line or "compare_section_pair" in line
        ):
            return "negative_test"
        return "test_fixture"
    if path == "tests/upd9002/protected_state_inventory_m47.json":
        return "historical_documentation"
    if path.startswith(HISTORICAL_PREFIXES):
        return "historical_documentation"
    if term in {"i286", "I286", "i286c", "I286C"} and path.startswith("cpu/upd9002/"):
        return "current_upd9002_state"
    if "286" in term:
        return "unrelated_numeric_286"
    return "historical_documentation"


def disposition(classification: str) -> str:
    return {
        "active_cpu286_state_compat": "remove_or_migrate_to_upd9002_state_format",
        "current_upd9002_state": "leave_for_m66b_identity_cleanup",
        "historical_documentation": "preserve_with_history",
        "test_fixture": "update_current_fixture_or_reclassify_negative",
        "negative_test": "preserve_as_fail_closed_rejection_evidence",
        "third_party_provenance": "preserve",
        "license_or_copyright": "preserve",
        "unrelated_numeric_286": "preserve",
    }[classification]


def inventory(root: pathlib.Path, tree: str | None, phase: str) -> dict[str, Any]:
    occurrences: list[dict[str, Any]] = []
    scanned: list[dict[str, Any]] = []
    for path in tracked_paths(root, tree):
        data = read_path(root, path, tree)
        if data is None or b"\0" in data:
            continue
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError:
            continue
        scanned.append(
            {
                "path": path,
                "sha256": sha256_bytes(data),
                "bytes": len(data),
            }
        )
        for line_number, line in enumerate(text.splitlines(), start=1):
            for term in SEARCH_TERMS:
                start = 0
                while True:
                    column = line.find(term, start)
                    if column < 0:
                        break
                    classification = classify(path, line, term)
                    occurrences.append(
                        {
                            "path": path,
                            "line": line_number,
                            "column": column + 1,
                            "term": term,
                            "classification": classification,
                            "disposition": disposition(classification),
                            "line_sha256": sha256_bytes(line.encode("utf-8")),
                        }
                    )
                    start = column + max(1, len(term))
    counts: dict[str, int] = {}
    for row in occurrences:
        counts[row["classification"]] = counts.get(row["classification"], 0) + 1
    return {
        "schema": "vaeg-upd9002-m66-state-inventory-v1",
        "phase": phase,
        "tree": tree or "worktree",
        "search_terms": list(SEARCH_TERMS),
        "scanned_paths": scanned,
        "occurrences": occurrences,
        "classification_counts": dict(sorted(counts.items())),
    }


def write_json(path: pathlib.Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    data = json.dumps(payload, indent=2, sort_keys=True) + "\n"
    path.write_text(data, encoding="utf-8")


def verify_m66a(root: pathlib.Path) -> dict[str, Any]:
    tbl = (root / "machine/statsave.tbl").read_text(encoding="utf-8")
    statsave = (root / "machine/statsave.c").read_text(encoding="utf-8")
    state_h = (root / "cpu/upd9002/upd9002_state.h").read_text(encoding="utf-8")
    state_c = (root / "cpu/upd9002/upd9002_state.c").read_text(encoding="utf-8")

    failures: list[str] = []
    if "UPD9002_STATE_SECTION" not in tbl or "UPD9002_STATE_VERSION" not in tbl:
        failures.append("current uPD9002 state table entry is missing")
    if '{"CPU286"' in tbl:
        failures.append("statsave still writes the obsolete CPU286 section")
    if "Cpu286StateCompat" in state_h + state_c:
        failures.append("obsolete Cpu286StateCompat type remains in state API")
    if "UPD9002_CPU286_PAYLOAD_SIZE" in state_h + state_c:
        failures.append("obsolete CPU286 payload-size macro remains")
    if "UPD9002_STATE_ERROR_LEGACY_MARKER" not in statsave:
        failures.append("legacy section marker rejection is missing")
    if "flagcheck_legacy_cpu_state" not in statsave:
        failures.append("legacy section check path is missing")
    if "flagload_legacy_cpu_state" not in statsave:
        failures.append("predecessor transitional migration path is missing")

    after = inventory(root, None, "after")
    active = [
        row
        for row in after["occurrences"]
        if row["classification"] == "active_cpu286_state_compat"
    ]
    if active:
        failures.append(f"active CPU286 state compatibility occurrences remain: {len(active)}")
    if failures:
        raise M66StateError("; ".join(failures))
    return {
        "schema": "vaeg-upd9002-m66a-state-verification-v1",
        "current_state_section": "UPD9CPU",
        "current_state_version": 1,
        "active_cpu286_state_compat_occurrences": 0,
        "legacy_section_detection": "fail_closed_or_predecessor_marker_migration",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=".")
    sub = parser.add_subparsers(dest="command", required=True)
    inv = sub.add_parser("inventory")
    inv.add_argument("--tree")
    inv.add_argument("--phase", required=True)
    inv.add_argument("--output", required=True)
    verify = sub.add_parser("verify-m66a")
    verify.add_argument("--output")
    args = parser.parse_args()

    root = pathlib.Path(args.root).resolve()
    try:
        if args.command == "inventory":
            write_json(
                pathlib.Path(args.output),
                inventory(root, args.tree, args.phase),
            )
        elif args.command == "verify-m66a":
            result = verify_m66a(root)
            if args.output:
                write_json(pathlib.Path(args.output), result)
            print("m66a-state-verify: ok")
        return 0
    except M66StateError as exc:
        print(f"m66a-state-verify-error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

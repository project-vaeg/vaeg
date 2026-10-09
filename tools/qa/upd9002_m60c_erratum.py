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
"""Validate the prospective M60c historical-label correction."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys
from typing import Any


G60B_SHA = "4e5d74d0d9f675df2342353b8bfdbb2e5cded768"
G60C_SHA = "e425e55fc17117000ba5178a796de4444d897234"
G43_TRANSITION_SHA256 = (
    "95559fa2a42a80710e850a9308202780a6fd4dad42ae20644c308bd0a72be092"
)
G60B_POLICY_FILE_SHA256 = (
    "b8d43fd743f205149a54a280c8350bb88e129e4b3d648385f81a203d2cef0814"
)

HISTORICAL_SETS: dict[str, tuple[int, str]] = {
    "g60a_6e_retired_failure": (
        0,
        "4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945",
    ),
    "g60a_6f_retired_failure": (
        641,
        "03f8ea83c510e67e27cc60a9455322f0cd899eb88287835080d2f9e98a0fa1f2",
    ),
    "g43_unchanged_signature": (
        417,
        "7240eff77e38a2ca67cf94d6cec13c4ddec1f2e122cf62cbb7318ee39c82be2e",
    ),
    "g43_changed_signature": (
        224,
        "f70b2e4e614cc677a883bc8d9ceb349f7a9bff32f185b253d893e6aea904a814",
    ),
    "g43_fixture_fix_pass": (
        1204,
        "c8de1415733c5bad2ba85d667d56f5d04631d19379ce16f85e641792e7644322",
    ),
}


class ErratumError(ValueError):
    """Raised when prospective documentation regresses the corrected labels."""


def sha256_file(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_master_text(text: str) -> None:
    required = (
        "Historical-label correction established by G60b",
        "`6E=0` and `6F=641`",
        "unchanged-signature and",
        "changed-signature subsets of the same 641-case G43 OUTS transition",
        "they were not\nper-opcode failure counts",
        "Exact content-addressed G60a/G60b hash sets govern all later accounting",
    )
    for phrase in required:
        if phrase not in text:
            raise ErratumError(f"prospective master lacks required wording: {phrase}")
    for _, digest in HISTORICAL_SETS.values():
        if text.count(digest) != 1:
            raise ErratumError(f"prospective master digest is missing or duplicated: {digest}")
    bad_label = re.compile(
        r"(?im)^.*(?:opcode(?:-form)?|per-opcode).*(?:6E\s*=\s*417|6F\s*=\s*224).*$"
    )
    if bad_label.search(text):
        raise ErratumError("prospective documentation labels 417/224 as opcode counts")


def validate_policy(value: Any) -> None:
    if not isinstance(value, dict):
        raise ErratumError("G60b target policy is malformed")
    history = value.get("historical_g43_reconciliation")
    if not isinstance(history, dict):
        raise ErratumError("G60b historical reconciliation is missing")
    fields = {
        "g60a_6e_retired_failure": (
            "g60a_6e_retired_failure_count",
            "g60a_6e_retired_failure_hash_set_sha256",
        ),
        "g60a_6f_retired_failure": (
            "g60a_6f_retired_failure_count",
            "g60a_6f_retired_failure_hash_set_sha256",
        ),
        "g43_unchanged_signature": (
            "g43_remaining_outs_unchanged_signature_count",
            "g43_remaining_outs_unchanged_signature_hash_set_sha256",
        ),
        "g43_changed_signature": (
            "g43_remaining_outs_changed_signature_count",
            "g43_remaining_outs_changed_signature_hash_set_sha256",
        ),
        "g43_fixture_fix_pass": (
            "g43_fixture_fix_pass_count",
            "g43_fixture_fix_pass_hash_set_sha256",
        ),
    }
    for name, (count_field, digest_field) in fields.items():
        expected_count, expected_digest = HISTORICAL_SETS[name]
        if history.get(count_field) != expected_count:
            raise ErratumError(f"{name}: count differs")
        if history.get(digest_field) != expected_digest:
            raise ErratumError(f"{name}: digest differs")


def validate_roadmap(text: str) -> None:
    expected = f"**G60b passed at `{G60B_SHA}`**"
    if text.count(expected) != 1:
        raise ErratumError("ROADMAP does not record the unique approved G60b SHA")
    approved = f"**G60c passed at `{G60C_SHA}`**"
    in_progress = "G60c in progress; candidate not yet approved"
    if (
        "| M60c |" not in text
        or (text.count(approved) != 1 and text.count(in_progress) != 1)
    ):
        raise ErratumError(
            "ROADMAP does not identify the unique in-progress or approved G60c gate"
        )


def verify_static(root: pathlib.Path) -> None:
    master = root / "docs/agents/UPD9002_SEMANTICS_MIGRATION.md"
    roadmap = root / "docs/agents/ROADMAP.md"
    policy = root / "tests/ssts/target_policy/g60b.json"
    transition = root / "tests/ssts/baseline/v20_native_g43_transition.json"
    validate_master_text(master.read_text(encoding="utf-8"))
    validate_roadmap(roadmap.read_text(encoding="utf-8"))
    validate_policy(json.loads(policy.read_text(encoding="utf-8")))
    if sha256_file(policy) != G60B_POLICY_FILE_SHA256:
        raise ErratumError("approved G60b target-policy artifact changed")
    if sha256_file(transition) != G43_TRANSITION_SHA256:
        raise ErratumError("immutable G43 transition artifact changed")
    print(
        "m60c-erratum-static: G60a 6E=0, G60a 6F=641, "
        "417/224 signature subsets, and protected evidence verified"
    )


def expect_rejected(action: Any, label: str) -> None:
    try:
        action()
    except ErratumError:
        return
    raise AssertionError(f"negative test was accepted: {label}")


def selftest() -> None:
    good = """
Historical-label correction established by G60b: at G60a, the exact retired
failure population was `6E=0` and `6F=641`. The values 417 and 224 identify,
respectively, unchanged-signature and
changed-signature subsets of the same 641-case G43 OUTS transition; they were not
per-opcode failure counts.
Exact content-addressed G60a/G60b hash sets govern all later accounting.
""" + "\n".join(digest for _, digest in HISTORICAL_SETS.values())
    validate_master_text(good)
    roadmap_prefix = (
        f"| M60b | task | scope | **G60b passed at `{G60B_SHA}`** |\n"
        "| M60c | task | scope | "
    )
    validate_roadmap(
        roadmap_prefix + f"**G60c passed at `{G60C_SHA}`** |\n"
    )
    validate_roadmap(
        roadmap_prefix + "G60c in progress; candidate not yet approved |\n"
    )
    expect_rejected(
        lambda: validate_roadmap(
            roadmap_prefix + "**G60c passed at `" + "0" * 40 + "`** |\n"
        ),
        "wrong approved G60c SHA",
    )
    expect_rejected(
        lambda: validate_master_text(good.replace("`6E=0`", "`6E=417`")),
        "wrong 6E opcode count",
    )
    expect_rejected(
        lambda: validate_master_text(
            good
            + "\nOpcode-form failures were 6E=417 and 6F=224.\n"
        ),
        "417/224 relabelled as opcode counts",
    )
    for _, digest in HISTORICAL_SETS.values():
        expect_rejected(
            lambda digest=digest: validate_master_text(
                good.replace(digest, "0" * 64)
            ),
            f"incorrect digest {digest}",
        )
    print(
        "m60c-erratum-selftest: 3 positive and 8 fail-closed checks passed"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("selftest")
    static = subparsers.add_parser("verify-static")
    static.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    arguments = parser.parse_args()
    try:
        if arguments.command == "selftest":
            selftest()
        else:
            verify_static(arguments.root.resolve())
    except (ErratumError, OSError, json.JSONDecodeError) as error:
        print(f"m60c-erratum-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

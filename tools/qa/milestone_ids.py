#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""Strict milestone identifiers and the M58 numeric-only tooling audit."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
from collections.abc import Iterable


class MilestoneIdError(ValueError):
    """A milestone identifier is absent, malformed, or ambiguous."""


# A suffix is one lowercase letter optionally followed by decimal digits.
# This admits the campaign's M60a and the planned M62b1 while rejecting
# ambiguous multi-letter or uppercase spellings.
CORE_PATTERN = r"(?:0|[1-9][0-9]*(?:[a-z](?:[1-9][0-9]*)?)?)"
CORE_RE = re.compile(rf"^{CORE_PATTERN}$")
MILESTONE_RE = re.compile(rf"^[Mm]({CORE_PATTERN})$")
GATE_RE = re.compile(rf"^G({CORE_PATTERN})$")
TASK_RE = re.compile(rf"^M({CORE_PATTERN})_([a-z0-9]+(?:_[a-z0-9]+)*)\.md$")
REPORT_RE = re.compile(rf"^m({CORE_PATTERN})_([a-z0-9]+(?:_[a-z0-9]+)*)\.md$")
BRANCH_RE = re.compile(rf"^topic/m({CORE_PATTERN})-([a-z0-9]+(?:-[a-z0-9]+)*)$")
COMMIT_PREFIX_RE = re.compile(rf"^M({CORE_PATTERN}):(?: |$)")


def parse_core(value: str) -> str:
    """Return the canonical lowercase core, rejecting non-canonical input."""
    match = CORE_RE.fullmatch(value)
    if match is None:
        raise MilestoneIdError(f"invalid milestone core: {value!r}")
    return value


def parse_milestone(value: str) -> str:
    """Parse M58, m58, M60a, or m60a and return the lowercase core."""
    match = MILESTONE_RE.fullmatch(value)
    if match is None:
        raise MilestoneIdError(f"invalid milestone identifier: {value!r}")
    return match.group(1)


def parse_gate(value: str) -> str:
    """Parse a canonical gate such as G58 or G60a."""
    match = GATE_RE.fullmatch(value)
    if match is None:
        raise MilestoneIdError(f"invalid gate identifier: {value!r}")
    return match.group(1)


def parse_task_name(value: str) -> tuple[str, str]:
    """Parse a canonical task basename."""
    match = TASK_RE.fullmatch(value)
    if match is None:
        raise MilestoneIdError(f"invalid task basename: {value!r}")
    return match.group(1), match.group(2)


def parse_report_name(value: str) -> tuple[str, str]:
    """Parse a canonical report basename."""
    match = REPORT_RE.fullmatch(value)
    if match is None:
        raise MilestoneIdError(f"invalid report basename: {value!r}")
    return match.group(1), match.group(2)


def parse_branch(value: str) -> tuple[str, str]:
    """Parse a canonical topic branch."""
    match = BRANCH_RE.fullmatch(value)
    if match is None:
        raise MilestoneIdError(f"invalid topic branch: {value!r}")
    return match.group(1), match.group(2)


def parse_commit_prefix(value: str) -> str:
    """Parse the milestone prefix at the start of a commit subject."""
    match = COMMIT_PREFIX_RE.match(value)
    if match is None:
        raise MilestoneIdError(f"invalid commit prefix: {value!r}")
    return match.group(1)


def gate_belongs_to_milestone(milestone_core: str, gate_core: str) -> bool:
    """Accept a milestone gate or one of its lettered child gates."""
    if gate_core == milestone_core:
        return True
    return (
        gate_core.startswith(milestone_core)
        and len(gate_core) > len(milestone_core)
        and gate_core[len(milestone_core)].islower()
    )


def artifact_stem(gate: str, profile: str, scope: str) -> str:
    """Build a deterministic artifact stem from strict identifiers."""
    core = parse_gate(gate)
    if profile not in {"architectural", "fingerprint"}:
        raise MilestoneIdError(f"invalid profile: {profile!r}")
    if scope not in {"ci", "full"}:
        raise MilestoneIdError(f"invalid scope: {scope!r}")
    if profile == "fingerprint" and scope != "full":
        raise MilestoneIdError("the fingerprint profile has only full scope")
    return f"g{core}_{profile}_{scope}"


def discover(root: pathlib.Path) -> tuple[list[pathlib.Path], list[pathlib.Path]]:
    """Discover and validate task and report names without numeric-only globs."""
    task_dir = root / "docs/agents/tasks"
    report_dir = root / "docs/agents/reports"
    tasks = sorted(task_dir.glob("M*.md"))
    reports = sorted(report_dir.glob("m*.md"))
    for path in tasks:
        parse_task_name(path.name)
    for path in reports:
        parse_report_name(path.name)
    return tasks, reports


def validate_roadmap(root: pathlib.Path) -> int:
    """Validate milestone, task, and gate IDs in the declarative ROADMAP table."""
    path = root / "docs/agents/ROADMAP.md"
    rows = 0
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), 1
    ):
        if not line.startswith("| M"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) != 4:
            raise MilestoneIdError(f"ROADMAP:{line_number}: malformed table row")
        core = parse_milestone(cells[0])
        task = pathlib.PurePosixPath(cells[1])
        if task.parent.as_posix() != "tasks":
            raise MilestoneIdError(f"ROADMAP:{line_number}: malformed task path")
        task_core, _ = parse_task_name(task.name)
        if task_core != core:
            raise MilestoneIdError(f"ROADMAP:{line_number}: task ID mismatch")
        gates = re.findall(r"\bG(" + CORE_PATTERN + r")\b", cells[3])
        if core == "0":
            if gates:
                raise MilestoneIdError(f"ROADMAP:{line_number}: unexpected M0 gate")
        elif not gates or not all(
            gate_belongs_to_milestone(core, gate) for gate in gates
        ):
            raise MilestoneIdError(f"ROADMAP:{line_number}: gate ID mismatch")
        rows += 1
    if rows == 0:
        raise MilestoneIdError("ROADMAP contains no milestone rows")
    return rows


NUMERIC_ONLY_PATTERNS = (
    re.compile(r"M\\?\[?0-9\]?\+"),
    re.compile(r"G\\?\[?0-9\]?\+"),
    re.compile(r"M\\\\d\+"),
    re.compile(r"G\\\\d\+"),
)


def audit_numeric_only_assumptions(root: pathlib.Path) -> list[str]:
    """Report active tooling lines that appear to require integer-only IDs."""
    findings: list[str] = []
    candidates = [root / "CMakeLists.txt"]
    for directory in (root / "tools", root / ".github/workflows"):
        if directory.is_dir():
            candidates.extend(
                path
                for path in directory.rglob("*")
                if path.is_file()
                and path.suffix in {".py", ".sh", ".cmake", ".yml", ".yaml"}
            )
    this_file = pathlib.Path(__file__).resolve()
    for path in sorted(set(candidates)):
        if not path.is_file() or path.resolve() == this_file:
            continue
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            continue
        for line_number, line in enumerate(lines, 1):
            if any(pattern.search(line) for pattern in NUMERIC_ONLY_PATTERNS):
                findings.append(
                    f"{path.relative_to(root).as_posix()}:{line_number}:{line.strip()}"
                )
    return findings


def expect_rejection(function, value: str) -> None:
    try:
        function(value)
    except MilestoneIdError:
        return
    raise AssertionError(f"malformed identifier was accepted: {value!r}")


def selftest() -> None:
    assert parse_milestone("M58") == "58"
    assert parse_milestone("m60a") == "60a"
    assert parse_milestone("M62b1") == "62b1"
    assert parse_milestone("M0") == "0"
    assert parse_gate("G60a") == "60a"
    assert parse_task_name("M60a_example_task.md") == ("60a", "example_task")
    assert parse_report_name("m60a_example_report.md") == ("60a", "example_report")
    assert parse_branch("topic/m60a-example-task") == ("60a", "example-task")
    assert parse_commit_prefix("M60a: implement strict parsing") == "60a"
    assert artifact_stem("G58", "architectural", "ci") == "g58_architectural_ci"
    assert artifact_stem("G60a", "fingerprint", "full") == "g60a_fingerprint_full"

    for value in ("M060a", "M60A", "M60aa", "M60a01", "M", "M00", "60a"):
        expect_rejection(parse_milestone, value)
    for value in ("g60a", "G060a", "G60A", "G60aa", "G"):
        expect_rejection(parse_gate, value)
    for value in (
        "M60a-example.md",
        "m60a_example.md",
        "M060a_example.md",
        "M60A_example.md",
        "M60a_Example.md",
    ):
        expect_rejection(parse_task_name, value)
    for value in (
        "M60a_example.md",
        "m060a_example.md",
        "m60A_example.md",
        "m60a-example.md",
    ):
        expect_rejection(parse_report_name, value)
    for value in (
        "topic/M60a-example",
        "topic/m060a-example",
        "topic/m60A-example",
        "topic/m60a_example",
        "topic/m60a-Example",
    ):
        expect_rejection(parse_branch, value)
    for value in ("m60a:", "M060a:", "M60A:", "M60aa:", "M60a :"):
        expect_rejection(parse_commit_prefix, value)

    try:
        artifact_stem("G60a", "fingerprint", "ci")
    except MilestoneIdError:
        pass
    else:
        raise AssertionError("fingerprint CI artifact was accepted")
    print("milestone-id-selftest: 48 strict acceptance/rejection checks passed")


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--audit", action="store_true")
    parser.add_argument("--discover", action="store_true")
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    root = arguments.root.resolve()
    try:
        if arguments.selftest:
            selftest()
        if arguments.discover:
            tasks, reports = discover(root)
            roadmap_rows = validate_roadmap(root)
            print(
                f"milestone-id-discovery: tasks={len(tasks)} "
                f"reports={len(reports)} roadmap_rows={roadmap_rows}"
            )
        if arguments.audit:
            findings = audit_numeric_only_assumptions(root)
            for finding in findings:
                print(f"numeric-only-milestone-assumption: {finding}")
            if findings:
                raise MilestoneIdError(
                    f"{len(findings)} numeric-only tooling assumption(s) found"
                )
            print(
                "milestone-id-audit: ROADMAP is declarative; task/report discovery, "
                "gate, branch, commit-prefix, artifact, and CI tooling accept "
                "strict canonical lettered identifiers"
            )
        if not (arguments.selftest or arguments.audit or arguments.discover):
            raise MilestoneIdError("select --selftest, --audit, or --discover")
    except (MilestoneIdError, OSError) as error:
        print(f"milestone-id-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

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
"""Inventory and verify retired active i286/CPU286 identity for M66b."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import pathlib
import re
import subprocess
import sys
from collections import Counter
from typing import Any, Dict, Iterable, List, Mapping, Sequence, Set


sys.dont_write_bytecode = True


class IdentityError(RuntimeError):
    """A fail-closed M66b identity verification error."""


SEARCH_TERMS = (
    "i286",
    "I286",
    "i286c",
    "I286C",
    "cpu286",
    "CPU286",
    "cpu_286",
    "CPU_286",
    "i286_",
    "I286_",
    "_286",
    "286 core",
    "286 CPU",
    "80286",
)

ACTIVE_CLASSES = {
    "active_identity_remove",
    "compatibility_alias_remove",
}

PRESERVED_CLASSES = {
    "historical_record_preserve",
    "negative_test_preserve",
    "third_party_provenance_preserve",
    "license_or_copyright_preserve",
    "architecture_reference_preserve",
    "unrelated_numeric_286",
}

SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cpp",
    ".h",
    ".hpp",
    ".mcr",
    ".cmake",
    ".tbl",
    ".py",
    ".md",
    ".txt",
    ".json",
    ".csv",
    ".yml",
    ".yaml",
    ".sh",
}

ACTIVE_SOURCE_PREFIXES = (
    "cpu/upd9002/",
    "tests/upd9002/",
    "sdl2/",
    "tools/qa/",
)

ACTIVE_ROOT_FILES = {
    "CMakeLists.txt",
    "machine/statsave.c",
    "machine/statsave.h",
    "machine/statsave.tbl",
}

HISTORICAL_PREFIXES = (
    "docs/agents/reports/",
    "tests/ssts/",
    "tools/qa/golden/",
)

FROZEN_REFERENCE_PREFIXES = (
    "win9x/",
    "i286x/",
    "cpuxva/",
    "hlp/",
)

NEGATIVE_OR_HISTORICAL_FIXTURES = {
    "tests/upd9002/fixtures.c",
    "tests/upd9002/state_payload_probe.c",
    "tests/upd9002/statsave_boundary.c",
    "tests/upd9002/state_fixtures_m42.txt",
    "tests/upd9002/protected_state_inventory_m47.json",
}

NEGATIVE_QA_PATHS = {
    "tools/qa/upd9002_m66_identity.py",
    "tools/qa/upd9002_m66_state.py",
    "tools/qa/upd9002_rename.py",
}

HISTORICAL_QA_PATHS = {
    "tools/qa/upd9002_dispatch_normalization.py",
    "tools/qa/upd9002_m61_mov_imm.py",
    "tools/qa/upd9002_native_invariant.py",
    "tools/qa/upd9002_protected_deletion.py",
    "tools/qa/upd9002_protected_reachability.py",
    "tools/qa/upd9002_rep0f_analysis.py",
    "tools/qa/upd9002_rep0f_transition.py",
    "tools/qa/upd9002_state_matrix.py",
}

SELF_REFERENTIAL_OUTPUT_PATHS = {
    "tests/ssts/campaigns/g66b/active_identity_allowlist.json",
    "tests/ssts/campaigns/g66b/active_identity_allowlist.json.gz",
    "tests/ssts/campaigns/g66b/identity_inventory_after.json",
    "tests/ssts/campaigns/g66b/identity_inventory_after.json.gz",
}

TASK_DOCUMENTATION_PREFIX = "docs/agents/tasks/"

PROVENANCE_RE = re.compile(
    r"(Copyright|provenance|frozen reference|behavior archaeology|"
    r"Engine for Pentium|Studio Milmake|historical)",
    re.IGNORECASE,
)

COMPAT_ALIAS_RE = re.compile(
    r"\b(?:typedef\s+.*\bI286|I286(?:STAT|CORE|EXT|REG8|REG16|DTR)\b|"
    r"#\s*define\s+I286_|extern\s+I286CORE\b|VAEG_M44_RAW_I286STAT\b)"
)

ACTIVE_SYMBOL_RE = re.compile(
    r"\b(?:i286core|i286c_[A-Za-z0-9_]*|i286_[A-Za-z0-9_]*|"
    r"I286_[A-Za-z0-9_]*|I286(?:STAT|CORE|EXT|REG8|REG16|DTR)|"
    r"CPU286|cpu286)\b"
)


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_text(text: str) -> str:
    return sha256_bytes(text.encode("utf-8"))


def git_files(root: pathlib.Path) -> List[str]:
    completed = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=root,
        check=True,
        capture_output=True,
    )
    return sorted(path for path in completed.stdout.decode("utf-8").split("\0") if path)


def read_text(root: pathlib.Path, relative: str) -> str | None:
    try:
        data = (root / relative).read_bytes()
    except OSError as exc:
        raise IdentityError(f"cannot read {relative}: {exc}") from exc
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return None


def is_active_surface(path: str) -> bool:
    if path in ACTIVE_ROOT_FILES:
        return True
    return path.startswith(ACTIVE_SOURCE_PREFIXES)


def is_historical_surface(path: str) -> bool:
    return path.startswith(HISTORICAL_PREFIXES)


def is_frozen_reference(path: str) -> bool:
    return path.startswith(FROZEN_REFERENCE_PREFIXES)


def term_matches(line: str) -> List[str]:
    return [term for term in SEARCH_TERMS if term in line]


def classify(path: str, line: str, terms: Sequence[str]) -> tuple[str, str]:
    suffix = pathlib.PurePosixPath(path).suffix.lower()
    line_l = line.lower()

    if path in NEGATIVE_OR_HISTORICAL_FIXTURES:
        return (
            "negative_test_preserve",
            "exact historical state/fixture payload retained for regression evidence",
        )
    if path in NEGATIVE_QA_PATHS:
        return (
            "negative_test_preserve",
            "fail-closed QA validator intentionally names retired identity",
        )
    if path in HISTORICAL_QA_PATHS:
        return (
            "historical_record_preserve",
            "QA validator preserves exact historical evidence or predecessor identity",
        )
    if is_frozen_reference(path):
        return (
            "third_party_provenance_preserve",
            "frozen reference tier is not active M66b production code",
        )
    if is_historical_surface(path):
        return (
            "historical_record_preserve",
            "approved report, evidence, scoreboard, or historical QA artifact",
        )
    if path.startswith(TASK_DOCUMENTATION_PREFIX):
        return (
            "historical_record_preserve",
            "canonical task prose documents the retired identity being removed",
        )
    if "copyright" in line_l or PROVENANCE_RE.search(line):
        return (
            "third_party_provenance_preserve",
            "source provenance text must remain historically accurate",
        )
    if any(term in {"80286", "286 CPU", "286 core"} for term in terms):
        if not ACTIVE_SYMBOL_RE.search(line):
            return (
                "architecture_reference_preserve",
                "architectural comparison text, not active uPD9002 ownership",
            )
    if any(term in {"_286"} for term in terms) and not ACTIVE_SYMBOL_RE.search(line):
        return (
            "unrelated_numeric_286",
            "numeric 286 occurrence is not an active CPU ownership identifier",
        )
    if suffix in SOURCE_SUFFIXES and is_active_surface(path):
        if COMPAT_ALIAS_RE.search(line):
            return (
                "compatibility_alias_remove",
                "active alias or compatibility spelling must be removed by M66b",
            )
        return (
            "active_identity_remove",
            "active source, build, test, runtime, or tooling identity must use uPD9002",
        )
    return (
        "historical_record_preserve",
        "non-active retained text outside current uPD9002 production surfaces",
    )


def inventory(root: pathlib.Path) -> Dict[str, Any]:
    files = git_files(root)
    occurrences: List[Dict[str, Any]] = []
    scanned_paths: List[str] = []
    skipped_binary: List[str] = []
    skipped_self_referential_outputs: List[str] = []
    for path in files:
        if path in SELF_REFERENTIAL_OUTPUT_PATHS:
            skipped_self_referential_outputs.append(path)
            continue
        suffix = pathlib.PurePosixPath(path).suffix.lower()
        if suffix and suffix not in SOURCE_SUFFIXES:
            continue
        text = read_text(root, path)
        if text is None:
            skipped_binary.append(path)
            continue
        scanned_paths.append(path)
        for number, line in enumerate(text.splitlines(), start=1):
            terms = term_matches(line)
            if not terms:
                continue
            classification, reason = classify(path, line, terms)
            occurrences.append(
                {
                    "path": path,
                    "line": number,
                    "terms": sorted(set(terms)),
                    "classification": classification,
                    "reason": reason,
                    "line_sha256": sha256_text(line),
                    "content": line,
                }
            )

    counts = Counter(row["classification"] for row in occurrences)
    active_by_path: Counter[str] = Counter(
        row["path"] for row in occurrences
        if row["classification"] in ACTIVE_CLASSES
    )
    return {
        "schema": "upd9002-m66b-identity-inventory-v1",
        "search_terms": list(SEARCH_TERMS),
        "scanned_path_count": len(scanned_paths),
        "scanned_paths_sha256": sha256_text("\n".join(scanned_paths)),
        "skipped_binary_paths": skipped_binary,
        "skipped_self_referential_outputs": skipped_self_referential_outputs,
        "occurrence_count": len(occurrences),
        "classification_counts": dict(sorted(counts.items())),
        "active_occurrence_count": sum(counts[name] for name in ACTIVE_CLASSES),
        "active_paths": dict(sorted(active_by_path.items())),
        "occurrences": occurrences,
    }


def stable_json(data: Mapping[str, Any]) -> str:
    return json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True) + "\n"


def write_json(path: pathlib.Path, data: Mapping[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    text = stable_json(data).encode("utf-8")
    if path.suffix == ".gz":
        path.write_bytes(gzip.compress(text, compresslevel=9, mtime=0))
    else:
        path.write_bytes(text)


def load_json(path: pathlib.Path) -> Dict[str, Any]:
    try:
        if path.suffix == ".gz":
            return json.loads(gzip.decompress(path.read_bytes()).decode("utf-8"))
        data = json.loads(path.read_text(encoding="utf-8"))
        compressed = data.get("compressed_artifact_path")
        if isinstance(compressed, str):
            compressed_path = pathlib.Path(compressed)
            if not compressed_path.is_absolute():
                compressed_path = path.parent / compressed_path
            return load_json(compressed_path)
        return data
    except (OSError, json.JSONDecodeError) as exc:
        raise IdentityError(f"cannot load {path}: {exc}") from exc


def make_allowlist(inventory_data: Mapping[str, Any]) -> Dict[str, Any]:
    entries = []
    for row in inventory_data["occurrences"]:
        if row["classification"] not in PRESERVED_CLASSES:
            continue
        entries.append(
            {
                "path": row["path"],
                "line": row["line"],
                "terms": row["terms"],
                "classification": row["classification"],
                "reason": row["reason"],
                "line_sha256": row["line_sha256"],
            }
        )
    digest_source = stable_json({"entries": entries})
    return {
        "schema": "upd9002-m66b-identity-allowlist-v1",
        "entry_count": len(entries),
        "entry_digest": sha256_text(digest_source),
        "entries": entries,
    }


def verify_allowlist(root: pathlib.Path, allowlist: Mapping[str, Any]) -> List[str]:
    failures: List[str] = []
    line_cache: Dict[str, List[str] | None] = {}
    for entry in allowlist.get("entries", []):
        if entry["classification"] in ACTIVE_CLASSES:
            failures.append(
                f"active class cannot be allowlisted: {entry['path']}:{entry['line']}"
            )
            continue
        path = str(entry["path"])
        if path not in line_cache:
            text = read_text(root, path)
            line_cache[path] = None if text is None else text.splitlines()
        lines = line_cache[path]
        if lines is None:
            failures.append(f"allowlisted path is not UTF-8 text: {path}")
            continue
        line_index = int(entry["line"]) - 1
        if line_index < 0 or line_index >= len(lines):
            failures.append(f"allowlisted line missing: {path}:{entry['line']}")
            continue
        line = lines[line_index]
        if sha256_text(line) != entry["line_sha256"]:
            failures.append(f"allowlisted line digest mismatch: {path}:{entry['line']}")
            continue
        for term in entry["terms"]:
            if term not in line:
                failures.append(f"allowlisted term missing: {path}:{entry['line']} {term}")
    return failures


def verify_identity(root: pathlib.Path, inventory_data: Mapping[str, Any],
                    allowlist: Mapping[str, Any]) -> None:
    current = inventory(root)
    failures: List[str] = []

    if current["active_occurrence_count"]:
        active = [
            f"{row['path']}:{row['line']} {','.join(row['terms'])}: {row['content']}"
            for row in current["occurrences"]
            if row["classification"] in ACTIVE_CLASSES
        ]
        failures.append(
            "active retired identity occurrences remain:\n  " + "\n  ".join(active[:200])
        )
        if len(active) > 200:
            failures.append(f"active occurrence output truncated at 200 of {len(active)}")

    recorded = stable_json(inventory_data)
    regenerated = stable_json(current)
    if sha256_text(recorded) != sha256_text(regenerated):
        failures.append("recorded identity inventory is not the deterministic current inventory")

    failures.extend(verify_allowlist(root, allowlist))

    files = git_files(root)
    stale_paths = [
        path for path in files
        if path.startswith("cpu/upd9002/")
        and any(term in path for term in ("i286", "I286", "cpu286", "CPU286"))
    ]
    if stale_paths:
        failures.append("active retired source paths remain: " + ", ".join(stale_paths))

    cmake = read_text(root, "CMakeLists.txt") or ""
    stale_cmake = [token for token in (
        "cpu/upd9002/i286c_",
        "cpu/upd9002/i286c.",
        "i286c/",
    ) if token in cmake]
    if stale_cmake:
        failures.append("active CMake old identity remains: " + ", ".join(stale_cmake))

    if failures:
        raise IdentityError("\n".join(failures))


def run_selftest() -> None:
    sample = {
        "schema": "sample",
        "occurrences": [
            {
                "path": "docs/agents/reports/old.md",
                "line": 1,
                "terms": ["i286"],
                "classification": "historical_record_preserve",
                "reason": "sample",
                "line_sha256": sha256_text("i286"),
                "content": "i286",
            }
        ],
    }
    allowlist = make_allowlist(sample)
    if allowlist["entry_count"] != 1:
        raise IdentityError("selftest allowlist count failed")
    cls, _ = classify("cpu/upd9002/i286c.h", "extern I286CORE i286core;", ["I286", "i286"])
    if cls != "compatibility_alias_remove":
        raise IdentityError("selftest active alias classification failed")
    cls, _ = classify("docs/agents/reports/m49.md", "i286c history", ["i286c", "i286"])
    if cls != "historical_record_preserve":
        raise IdentityError("selftest historical classification failed")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)

    inventory_parser = sub.add_parser("inventory")
    inventory_parser.add_argument("--output", required=True, type=pathlib.Path)
    inventory_parser.add_argument("--root", default=".", type=pathlib.Path)

    allow_parser = sub.add_parser("allowlist")
    allow_parser.add_argument("--inventory", required=True, type=pathlib.Path)
    allow_parser.add_argument("--output", required=True, type=pathlib.Path)

    verify_parser = sub.add_parser("verify")
    verify_parser.add_argument("--inventory", required=True, type=pathlib.Path)
    verify_parser.add_argument("--allowlist", required=True, type=pathlib.Path)
    verify_parser.add_argument("--root", default=".", type=pathlib.Path)

    sub.add_parser("selftest")

    args = parser.parse_args(argv)
    try:
        if args.command == "inventory":
            write_json(args.output, inventory(args.root.resolve()))
        elif args.command == "allowlist":
            write_json(args.output, make_allowlist(load_json(args.inventory)))
        elif args.command == "verify":
            verify_identity(
                args.root.resolve(),
                load_json(args.inventory),
                load_json(args.allowlist),
            )
        elif args.command == "selftest":
            run_selftest()
        else:
            raise IdentityError(f"unknown command: {args.command}")
    except IdentityError as exc:
        print(f"upd9002_m66_identity: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

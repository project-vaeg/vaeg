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
"""Build and enforce immutable uPD9002 SST scoreboard epochs."""

from __future__ import annotations

import argparse
import copy
import gzip
import hashlib
import json
import pathlib
import re
import struct
import subprocess
import sys
import tempfile
import zlib
from collections import Counter, defaultdict
from collections.abc import Callable, Iterable, Iterator
from typing import Any

import upd9002_ssts as ssts


APPROVED_PREDECESSOR_GATE = "G57"
APPROVED_PREDECESSOR_SHA = "72322d5c9b8e40e4a988312aebe163a8190e2aa5"
EPOCH_GATE = "G58"
IMMUTABLE_M43_PREDECESSOR_GATE = "G57"
IMMUTABLE_M43_PREDECESSOR_SHA = (
    "72322d5c9b8e40e4a988312aebe163a8190e2aa5"
)
TOP_LEVEL_CLASSIFICATIONS = (
    "applicable",
    "known_target_gap",
    "expected_target_divergence",
    "unsupported_fixture",
    "upstream_nonblocking",
)
GAP_KINDS = (
    "documented_silicon_absent",
    "implementation_missing",
    "target_support_unverified",
)
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
FORM_RE = re.compile(
    r"^(?:[0-9A-F]{2}|[0-9A-F]{2}\.[0-7]|0F[0-9A-F]{2}|0F[0-9A-F]{2}\.[0-7])$"
)


class RatchetError(ValueError):
    """An epoch, registry, or transition failed closed validation."""


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, ensure_ascii=True, separators=(",", ":"), sort_keys=True
    ).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_sha256(value: Any, field: str) -> str:
    if not isinstance(value, str) or SHA256_RE.fullmatch(value) is None:
        raise RatchetError(f"{field}: expected lowercase SHA-256")
    return value


def require_sha1(value: Any, field: str) -> str:
    if not isinstance(value, str) or SHA1_RE.fullmatch(value) is None:
        raise RatchetError(f"{field}: expected lowercase SHA-1")
    return value


def require_sha(value: Any, field: str) -> str:
    if not isinstance(value, str) or SHA1_RE.fullmatch(value) is None:
        raise RatchetError(f"{field}: expected 40-hex commit SHA")
    return value


def require_count(value: Any, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise RatchetError(f"{field}: expected non-negative integer")
    return value


def read_json(path: pathlib.Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, UnicodeDecodeError) as error:
        raise RatchetError(f"{path}: invalid UTF-8 JSON: {error}") from error


def write_json(path: pathlib.Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(canonical_bytes(value) + b"\n")


def deterministic_gzip_bytes(value: Any) -> bytes:
    payload = canonical_bytes(value) + b"\n"
    compressor = zlib.compressobj(
        level=9,
        method=zlib.DEFLATED,
        wbits=-zlib.MAX_WBITS,
        memLevel=8,
        strategy=zlib.Z_DEFAULT_STRATEGY,
    )
    deflate = compressor.compress(payload) + compressor.flush(zlib.Z_FINISH)
    header = b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff"
    trailer = struct.pack(
        "<II", zlib.crc32(payload) & 0xFFFFFFFF, len(payload) & 0xFFFFFFFF
    )
    return header + deflate + trailer


def write_deterministic_gzip(path: pathlib.Path, value: Any) -> tuple[str, str]:
    path.parent.mkdir(parents=True, exist_ok=True)
    compressed = deterministic_gzip_bytes(value)
    path.write_bytes(compressed)
    return sha256_bytes(compressed), sha256_bytes(canonical_bytes(value) + b"\n")


def read_deterministic_gzip(path: pathlib.Path) -> Any:
    compressed = path.read_bytes()
    header = b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff"
    if len(compressed) < len(header) + 8 or compressed[: len(header)] != header:
        raise RatchetError(f"{path}: gzip metadata is not canonical")
    try:
        decompressor = zlib.decompressobj(-zlib.MAX_WBITS)
        payload = decompressor.decompress(compressed[len(header) : -8])
        payload += decompressor.flush()
        if (
            not decompressor.eof
            or decompressor.unconsumed_tail
            or decompressor.unused_data
        ):
            raise RatchetError(f"{path}: gzip must contain one complete member")
        expected_crc32, expected_size = struct.unpack("<II", compressed[-8:])
        if expected_crc32 != zlib.crc32(payload) & 0xFFFFFFFF:
            raise RatchetError(f"{path}: gzip CRC-32 mismatch")
        if expected_size != len(payload) & 0xFFFFFFFF:
            raise RatchetError(f"{path}: gzip size mismatch")
        value = json.loads(payload.decode("utf-8"))
    except (json.JSONDecodeError, UnicodeDecodeError, zlib.error) as error:
        raise RatchetError(f"{path}: invalid deterministic JSON gzip: {error}") from error
    if payload != canonical_bytes(value) + b"\n":
        raise RatchetError(f"{path}: gzip payload is not canonical JSON")
    return value


def hash_set_digest(values: Iterable[str]) -> str:
    ordered = sorted(values)
    if len(ordered) != len(set(ordered)):
        raise RatchetError("hash set contains duplicate values")
    for index, value in enumerate(ordered):
        require_sha256(value, f"hash_set[{index}]")
    return sha256_bytes(canonical_bytes(ordered))


def upstream_hash_set_digest(values: Iterable[str]) -> str:
    ordered = sorted(values)
    if len(ordered) != len(set(ordered)):
        raise RatchetError("upstream hash set contains duplicate values")
    for index, value in enumerate(ordered):
        require_sha1(value, f"upstream_hash_set[{index}]")
    return sha256_bytes(canonical_bytes(ordered))


def load_contract(path: pathlib.Path) -> tuple[dict[str, Any], str]:
    value = read_json(path)
    expected = {
        "blocking",
        "bus_timing",
        "comparison_contract_id",
        "copyright",
        "cycles",
        "final_flags",
        "final_general_registers",
        "final_instruction_pointer",
        "final_io_events",
        "final_ram",
        "final_segment_registers",
        "license",
        "prefetch",
        "schema",
        "schema_version",
        "termination",
    }
    if not isinstance(value, dict) or set(value) != expected:
        raise RatchetError(f"{path}: unknown comparison-contract schema")
    if (
        value["schema"] != "vaeg-upd9002-ssts-comparison-contract-v1"
        or value["schema_version"] != 1
    ):
        raise RatchetError(f"{path}: unsupported comparison-contract version")
    if value["comparison_contract_id"] not in {
        "upd9002-v20-architectural-v1",
        "upd9002-v20-fingerprint-v1",
    }:
        raise RatchetError(f"{path}: unknown comparison-contract ID")
    profile = value["comparison_contract_id"].split("-")[-2]
    expected_flags = (
        "metadata-masked-defined-bits"
        if profile == "architectural"
        else "all-16-bits"
    )
    if value["final_flags"] != expected_flags:
        raise RatchetError(f"{path}: FLAGS contract does not match profile")
    if value["blocking"] != (profile == "architectural"):
        raise RatchetError(f"{path}: blocking policy does not match profile")
    if any(value[field] != "excluded" for field in ("cycles", "prefetch", "bus_timing")):
        raise RatchetError(f"{path}: timing or prefetch entered the comparison contract")
    return value, sha256_file(path)


def verify_immutable_file_entries(
    root: pathlib.Path, files: Any
) -> list[str]:
    if not isinstance(files, list) or not files:
        raise RatchetError("immutable manifest must contain files")
    paths: list[str] = []
    for index, item in enumerate(files):
        if not isinstance(item, dict) or set(item) != {"path", "sha256", "size"}:
            raise RatchetError(f"immutable files[{index}]: unknown schema")
        relative = item["path"]
        if (
            not isinstance(relative, str)
            or not relative.startswith("tests/ssts/baseline/")
            or ".." in pathlib.PurePosixPath(relative).parts
        ):
            raise RatchetError(f"immutable files[{index}]: unsafe path")
        require_sha256(item["sha256"], f"immutable files[{index}].sha256")
        require_count(item["size"], f"immutable files[{index}].size")
        path = root / relative
        if not path.is_file():
            raise RatchetError(f"immutable M43 evidence is missing: {relative}")
        if path.stat().st_size != item["size"] or sha256_file(path) != item["sha256"]:
            raise RatchetError(f"immutable M43 evidence was modified: {relative}")
        paths.append(relative)
    if paths != sorted(paths) or len(paths) != len(set(paths)):
        raise RatchetError("immutable manifest paths are not unique and ordered")
    return paths


def verify_immutable_m43(
    root: pathlib.Path, manifest_path: pathlib.Path
) -> dict[str, Any]:
    manifest = read_json(manifest_path)
    expected = {
        "approved_predecessor_gate",
        "approved_predecessor_sha",
        "copyright",
        "dataset_id",
        "files",
        "license",
        "profiles",
        "schema",
        "schema_version",
    }
    if not isinstance(manifest, dict) or set(manifest) != expected:
        raise RatchetError(f"{manifest_path}: unknown immutable-manifest schema")
    if (
        manifest["schema"] != "vaeg-upd9002-ssts-immutable-epoch-v1"
        or manifest["schema_version"] != 1
    ):
        raise RatchetError(f"{manifest_path}: unsupported immutable-manifest version")
    if (
        manifest["approved_predecessor_gate"]
        != IMMUTABLE_M43_PREDECESSOR_GATE
    ):
        raise RatchetError("immutable manifest has wrong approved predecessor gate")
    if (
        manifest["approved_predecessor_sha"]
        != IMMUTABLE_M43_PREDECESSOR_SHA
    ):
        raise RatchetError("immutable manifest has wrong approved predecessor SHA")
    verify_immutable_file_entries(root, manifest["files"])
    profiles = manifest["profiles"]
    if not isinstance(profiles, dict) or set(profiles) != {"ci", "full"}:
        raise RatchetError("immutable manifest must describe CI and full profiles")
    for scope, profile in profiles.items():
        required = {
            "applicable_hash_set_sha256",
            "classification_hash_sets",
            "comparison_contract_id",
            "comparison_contract_sha256",
            "failure_hash_set_sha256",
            "failure_index_sha256",
            "failure_sidecar_canonical_set_sha256",
            "failure_sidecar_raw_set_sha256",
            "pass_hash_set_sha256",
            "selected_hash_set_sha256",
            "summary_path",
            "summary_sha256",
            "upstream_classification_hash_sets",
        }
        if not isinstance(profile, dict) or set(profile) != required:
            raise RatchetError(f"immutable profile {scope}: unknown schema")
        for field in required - {
            "classification_hash_sets",
            "comparison_contract_id",
            "summary_path",
            "upstream_classification_hash_sets",
        }:
            require_sha256(profile[field], f"immutable profile {scope}.{field}")
        for set_field in (
            "classification_hash_sets",
            "upstream_classification_hash_sets",
        ):
            set_digests = profile[set_field]
            if (
                not isinstance(set_digests, dict)
                or set(set_digests) != set(TOP_LEVEL_CLASSIFICATIONS)
            ):
                raise RatchetError(
                    f"immutable profile {scope}: incomplete {set_field}"
                )
            for classification, digest in set_digests.items():
                require_sha256(
                    digest,
                    f"immutable profile {scope}.{set_field}.{classification}",
                )
        if profile["comparison_contract_id"] != "upd9002-v20-architectural-v1":
            raise RatchetError(f"immutable profile {scope}: wrong contract ID")
        summary_path = root / profile["summary_path"]
        summary = read_json(summary_path)
        if sha256_file(summary_path) != profile["summary_sha256"]:
            raise RatchetError(f"immutable profile {scope}: summary digest mismatch")
        if summary.get("profile") != scope:
            raise RatchetError(f"immutable profile {scope}: summary scope mismatch")
        if summary.get("failure_signature_index_sha256") != profile["failure_index_sha256"]:
            raise RatchetError(f"immutable profile {scope}: failure index mismatch")
        try:
            ssts.verify_failure_files(summary_path, summary)
        except ssts.CorpusError as error:
            raise RatchetError(f"immutable profile {scope}: {error}") from error
        canonical_items = [
            {
                "path": item["path"],
                "sha256": item["canonical_sha256"],
                "count": item["failure_count"],
            }
            for item in summary["failure_signature_files"]
        ]
        raw_items = [
            {
                "path": item["path"],
                "sha256": sha256_file(summary_path.parent / item["path"]),
                "count": item["failure_count"],
            }
            for item in summary["failure_signature_files"]
        ]
        if sha256_bytes(canonical_bytes(canonical_items)) != profile[
            "failure_sidecar_canonical_set_sha256"
        ]:
            raise RatchetError(f"immutable profile {scope}: sidecar content-set mismatch")
        if sha256_bytes(canonical_bytes(raw_items)) != profile[
            "failure_sidecar_raw_set_sha256"
        ]:
            raise RatchetError(f"immutable profile {scope}: sidecar byte-set mismatch")
    return manifest


def selector_digest(selector: Any) -> str:
    if not isinstance(selector, dict) or not selector:
        raise RatchetError("selector must be a non-empty object")
    for key, value in selector.items():
        if not isinstance(key, str) or not key:
            raise RatchetError("selector key must be non-empty text")
        lowered = key.lower()
        if any(word in lowered for word in ("pass", "fail", "outcome", "result", "mismatch")):
            raise RatchetError("outcome-based structural selector")
        if value == "*" or (isinstance(value, str) and value.lower() in {"all", "any"}):
            raise RatchetError("open-ended selector")
    return sha256_bytes(canonical_bytes(selector))


def validate_content_registry(value: Any, registry_kind: str) -> dict[str, dict[str, Any]]:
    expected_schema = f"vaeg-upd9002-ssts-{registry_kind}-v1"
    required_top = {
        "copyright",
        "dataset_id",
        "entries",
        "license",
        "schema",
        "schema_version",
    }
    if not isinstance(value, dict) or set(value) != required_top:
        raise RatchetError(f"{registry_kind}: unknown registry schema")
    if value["schema"] != expected_schema or value["schema_version"] != 1:
        raise RatchetError(f"{registry_kind}: unsupported registry version")
    entries = value["entries"]
    if not isinstance(entries, list):
        raise RatchetError(f"{registry_kind}: entries must be an array")
    by_selector: dict[str, dict[str, Any]] = {}
    owned_hashes: set[str] = set()
    previous_key = ""
    for index, entry in enumerate(entries):
        required_entry = {
            "evidence",
            "first_introduced_milestone",
            "reason",
            "resolved_count",
            "resolved_test_hashes",
            "resolved_test_hashes_sha256",
            "review_status",
            "selector",
            "selector_sha256",
        }
        if not isinstance(entry, dict) or set(entry) != required_entry:
            raise RatchetError(f"{registry_kind}[{index}]: unknown entry schema")
        digest = selector_digest(entry["selector"])
        if entry["selector_sha256"] != digest:
            raise RatchetError(f"{registry_kind}[{index}]: selector mismatch")
        hashes = entry["resolved_test_hashes"]
        if not isinstance(hashes, list) or not hashes:
            raise RatchetError(f"{registry_kind}[{index}]: open-ended hash set")
        if hashes != sorted(hashes) or len(hashes) != len(set(hashes)):
            raise RatchetError(f"{registry_kind}[{index}]: hashes are not unique and ordered")
        for item in hashes:
            require_sha1(item, f"{registry_kind}[{index}].resolved_test_hashes")
            if item in owned_hashes:
                raise RatchetError(f"{registry_kind}: overlapping registry ownership")
            owned_hashes.add(item)
        if entry["resolved_count"] != len(hashes):
            raise RatchetError(f"{registry_kind}[{index}]: resolved-count mismatch")
        if entry["resolved_test_hashes_sha256"] != upstream_hash_set_digest(hashes):
            raise RatchetError(f"{registry_kind}[{index}]: sorted-hash-digest mismatch")
        if not isinstance(entry["reason"], str) or not entry["reason"]:
            raise RatchetError(f"{registry_kind}[{index}]: missing reason")
        if not isinstance(entry["evidence"], dict) or not entry["evidence"]:
            raise RatchetError(f"{registry_kind}[{index}]: missing evidence")
        if not isinstance(entry["first_introduced_milestone"], str):
            raise RatchetError(f"{registry_kind}[{index}]: missing milestone")
        if entry["review_status"] not in {"pending", "approved", "rejected"}:
            raise RatchetError(f"{registry_kind}[{index}]: invalid review status")
        if digest <= previous_key:
            raise RatchetError(f"{registry_kind}: entries are not deterministically ordered")
        previous_key = digest
        by_selector[digest] = entry
    return by_selector


def validate_taxonomy(
    taxonomy: Any,
    known_gaps: Any,
    hardware_pending: Any,
    known_gaps_sha256: str | None = None,
) -> tuple[dict[str, str], Counter[str], Counter[str]]:
    required_top = {
        "annotations",
        "copyright",
        "dataset_id",
        "license",
        "schema",
        "schema_version",
        "source_known_gaps_path",
        "source_known_gaps_sha256",
    }
    if not isinstance(taxonomy, dict) or set(taxonomy) != required_top:
        raise RatchetError("gap taxonomy: unknown schema")
    if (
        taxonomy["schema"] != "vaeg-upd9002-ssts-gap-taxonomy-v1"
        or taxonomy["schema_version"] != 1
    ):
        raise RatchetError("gap taxonomy: unsupported version")
    source_digest = (
        known_gaps_sha256
        if known_gaps_sha256 is not None
        else sha256_bytes(canonical_bytes(known_gaps) + b"\n")
    )
    require_sha256(source_digest, "gap taxonomy source digest")
    if taxonomy["source_known_gaps_sha256"] != source_digest:
        raise RatchetError("gap taxonomy: source known-gap digest mismatch")
    if taxonomy["dataset_id"] != known_gaps.get("dataset_id"):
        raise RatchetError("gap taxonomy: dataset identity mismatch")
    rules = known_gaps.get("rules")
    if not isinstance(rules, list):
        raise RatchetError("gap taxonomy: source rules are missing")
    source: dict[str, dict[str, Any]] = {}
    for rule in rules:
        digest = selector_digest(rule.get("selector"))
        if digest in source:
            raise RatchetError("gap taxonomy: overlapping source selectors")
        source[digest] = rule
    annotations = taxonomy["annotations"]
    if not isinstance(annotations, list):
        raise RatchetError("gap taxonomy: annotations must be an array")
    kinds: dict[str, str] = {}
    rule_counts: Counter[str] = Counter()
    hash_counts: Counter[str] = Counter()
    previous = ""
    for index, annotation in enumerate(annotations):
        required = {
            "gap_kind",
            "resolved_count",
            "resolved_test_hashes_sha256",
            "selector_sha256",
        }
        if not isinstance(annotation, dict) or set(annotation) != required:
            raise RatchetError(f"gap taxonomy[{index}]: unknown annotation schema")
        digest = require_sha256(
            annotation["selector_sha256"], f"gap taxonomy[{index}].selector_sha256"
        )
        if digest <= previous:
            raise RatchetError("gap taxonomy: annotations are not deterministically ordered")
        previous = digest
        if digest not in source:
            raise RatchetError(f"gap taxonomy[{index}]: selector mismatch")
        kind = annotation["gap_kind"]
        if kind not in GAP_KINDS:
            raise RatchetError(f"gap taxonomy[{index}]: unknown gap_kind")
        rule = source[digest]
        if annotation["resolved_count"] != rule.get("resolved_count"):
            raise RatchetError(f"gap taxonomy[{index}]: resolved-count mismatch")
        if annotation["resolved_test_hashes_sha256"] != rule.get(
            "resolved_test_hashes_sha256"
        ):
            raise RatchetError(f"gap taxonomy[{index}]: resolved-hash mismatch")
        kinds[digest] = kind
        rule_counts[kind] += 1
        hash_counts[kind] += annotation["resolved_count"]
    if set(kinds) != set(source):
        missing = sorted(set(source) - set(kinds))
        raise RatchetError(f"gap taxonomy: missing gap_kind for {len(missing)} rule(s)")

    pending = validate_content_registry(hardware_pending, "hardware-pending")
    for digest, kind in kinds.items():
        if kind != "target_support_unverified":
            continue
        if digest not in pending:
            raise RatchetError("gap taxonomy: missing hardware-pending coverage")
        rule = source[digest]
        entry = pending[digest]
        if entry["selector"] != rule["selector"]:
            raise RatchetError("gap taxonomy: hardware-pending selector mismatch")
        if entry["resolved_test_hashes"] != rule["resolved_test_hashes"]:
            raise RatchetError("gap taxonomy: hardware-pending resolved-hash mismatch")
        if entry["resolved_count"] != rule["resolved_count"]:
            raise RatchetError("gap taxonomy: hardware-pending resolved-count mismatch")
        if entry["resolved_test_hashes_sha256"] != rule[
            "resolved_test_hashes_sha256"
        ]:
            raise RatchetError("gap taxonomy: hardware-pending sorted-hash-digest mismatch")
    unexpected_pending = set(pending) - {
        digest for digest, kind in kinds.items() if kind == "target_support_unverified"
    }
    if unexpected_pending:
        raise RatchetError("hardware-pending registry has unmatched ownership")
    return kinds, rule_counts, hash_counts


def parse_form(form: str) -> tuple[str, str]:
    if not isinstance(form, str) or FORM_RE.fullmatch(form) is None:
        raise RatchetError(f"malformed opcode form: {form!r}")
    if form.startswith("0F") and len(form) >= 4:
        suffix = form[2:].lower()
        return "0f", suffix
    if "." in form:
        opcode, subform = form.split(".", 1)
        return opcode.lower(), subform
    return form.lower(), "-"


def form_from_parts(opcode: str, subform: str) -> str:
    if not isinstance(opcode, str) or re.fullmatch(r"[0-9a-f]{2}", opcode) is None:
        raise RatchetError(f"malformed opcode: {opcode!r}")
    if opcode == "0f":
        if not isinstance(subform, str) or re.fullmatch(
            r"[0-9a-f]{2}(?:\.[0-7])?", subform
        ) is None:
            raise RatchetError(f"malformed 0f subform: {subform!r}")
        return ("0F" + subform).upper()
    if subform == "-":
        return opcode.upper()
    if not isinstance(subform, str) or re.fullmatch(r"[0-7]", subform) is None:
        raise RatchetError(f"malformed subform: {subform!r}")
    return f"{opcode}.{subform}".upper()


def enumerate_profiles(
    dataset_root: pathlib.Path,
    manifest: dict[str, Any],
    predecessor_support_map: pathlib.Path,
    candidate_support_map: pathlib.Path,
    scope: str,
) -> dict[str, Any]:
    if scope not in {"ci", "full"}:
        raise RatchetError(f"unknown scope: {scope}")
    ssts.verify_fast(dataset_root, manifest)
    metadata = json.loads(
        (dataset_root / ssts.SUITE_PATH / "metadata.json").read_text(encoding="utf-8")
    )
    ssts.validate_metadata(metadata)
    before_support = ssts.load_support_map(predecessor_support_map)
    after_support = ssts.load_support_map(candidate_support_map)
    before_sets: dict[str, list[str]] = {
        classification: [] for classification in TOP_LEVEL_CLASSIFICATIONS
    }
    after_sets: dict[str, list[str]] = {
        classification: [] for classification in TOP_LEVEL_CLASSIFICATIONS
    }
    upstream_sets: dict[str, list[str]] = {
        classification: [] for classification in TOP_LEVEL_CLASSIFICATIONS
    }
    after_form_counts: dict[str, Counter[str]] = defaultdict(Counter)
    before_form_counts: dict[str, Counter[str]] = defaultdict(Counter)
    changes: list[dict[str, str]] = []
    selected_count = 0
    for path in ssts.corpus_files(dataset_root):
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            records = json.load(stream)
        form = path.name.removesuffix(".json.gz").upper()
        for raw_record in ssts.profile_records(records, scope):
            record = ssts.validate_record(raw_record, f"{path.name}:{raw_record.get('idx')}")
            before = ssts.classify_record(form, record, metadata, before_support)[
                "classification"
            ]
            after = ssts.classify_record(form, record, metadata, after_support)[
                "classification"
            ]
            if before not in TOP_LEVEL_CLASSIFICATIONS or after not in TOP_LEVEL_CLASSIFICATIONS:
                raise RatchetError(f"{form}:{record['hash']}: unknown classification")
            if before == "unsupported_fixture" or after == "unsupported_fixture":
                raise RatchetError(
                    f"{form}:{record['hash']}: unsupported fixture entered selected profile"
                )
            record_hash = sha256_bytes(canonical_bytes(record))
            before_sets[before].append(record_hash)
            after_sets[after].append(record_hash)
            upstream_sets[after].append(record["hash"])
            before_form_counts[form][before] += 1
            after_form_counts[form][after] += 1
            selected_count += 1
            if before != after:
                changes.append(
                    {
                        "record_hash": record_hash,
                        "upstream_test_hash": record["hash"],
                        "form": form,
                        "before": before,
                        "after": after,
                    }
                )
    changes.sort(key=lambda item: item["record_hash"])
    before_digests = {
        classification: hash_set_digest(values)
        for classification, values in before_sets.items()
    }
    after_digests = {
        classification: hash_set_digest(values)
        for classification, values in after_sets.items()
    }
    upstream_digests = {
        classification: upstream_hash_set_digest(values)
        for classification, values in upstream_sets.items()
    }
    selected_before = [
        value for classification in TOP_LEVEL_CLASSIFICATIONS for value in before_sets[classification]
    ]
    selected_after = [
        value for classification in TOP_LEVEL_CLASSIFICATIONS for value in after_sets[classification]
    ]
    selected_before_digest = hash_set_digest(selected_before)
    selected_after_digest = hash_set_digest(selected_after)
    if selected_before_digest != selected_after_digest:
        raise RatchetError("selected hash set changed between support maps")
    return {
        "selected_count": selected_count,
        "selected_hash_set_sha256": selected_after_digest,
        "before_sets": before_sets,
        "after_sets": after_sets,
        "before_set_digests": before_digests,
        "after_set_digests": after_digests,
        "upstream_set_digests": upstream_digests,
        "before_form_counts": before_form_counts,
        "after_form_counts": after_form_counts,
        "classification_changes": changes,
    }


def load_failure_records(summary_path: pathlib.Path) -> dict[str, dict[str, Any]]:
    summary = read_json(summary_path)
    try:
        ssts.verify_failure_files(summary_path, summary)
    except ssts.CorpusError as error:
        raise RatchetError(f"{summary_path}: {error}") from error
    failures: dict[str, dict[str, Any]] = {}
    for file_entry in summary.get("failure_signature_files", []):
        path = summary_path.parent / file_entry["path"]
        with gzip.open(path, "rt", encoding="utf-8") as stream:
            payload = json.load(stream)
        for failure in payload["failure_signatures"]:
            content = failure.get("content")
            record_hash = content.get("record_hash") if isinstance(content, dict) else None
            require_sha256(record_hash, f"{path}: record_hash")
            if record_hash in failures:
                raise RatchetError(f"{summary_path}: duplicate failure record hash")
            failures[record_hash] = failure
    if len(failures) != summary.get("failure_signature_count"):
        raise RatchetError(f"{summary_path}: failure count mismatch")
    index = sorted(
        (
            {
                "record_hash": record_hash,
                "signature_sha256": failure["signature_sha256"],
            }
            for record_hash, failure in failures.items()
        ),
        key=lambda item: item["record_hash"],
    )
    if sha256_bytes(canonical_bytes(index)) != summary.get(
        "failure_signature_index_sha256"
    ):
        raise RatchetError(f"{summary_path}: failure index digest mismatch")
    return failures


def failure_entry(failure: dict[str, Any]) -> dict[str, Any]:
    signature = require_sha256(failure.get("signature_sha256"), "failure signature")
    content = failure.get("content")
    if not isinstance(content, dict):
        raise RatchetError("failure content is missing")
    form = content.get("opcode_form")
    parse_form(form)
    record_hash = require_sha256(content.get("record_hash"), "failure record hash")
    upstream_hash = require_sha1(
        content.get("upstream_test_hash"), "failure upstream test hash"
    )
    mismatch_classes = content.get("mismatch_kinds")
    if (
        not isinstance(mismatch_classes, list)
        or mismatch_classes != sorted(set(mismatch_classes))
        or not all(isinstance(item, str) and item for item in mismatch_classes)
    ):
        raise RatchetError(f"{record_hash}: malformed mismatch classes")
    actual_state = content.get("actual_state")
    if actual_state is None:
        actual_termination = content.get("timeout_crash_status")
    elif isinstance(actual_state, dict):
        actual_termination = actual_state.get("termination")
    else:
        raise RatchetError(f"{record_hash}: malformed actual state")
    expected_state = content.get("expected_state")
    if not isinstance(expected_state, dict):
        raise RatchetError(f"{record_hash}: malformed expected state")
    expected_termination = expected_state.get("termination")
    if not isinstance(actual_termination, str) or not actual_termination:
        raise RatchetError(f"{record_hash}: missing actual termination")
    if not isinstance(expected_termination, str) or not expected_termination:
        raise RatchetError(f"{record_hash}: missing expected termination")
    flags = content.get("masked_flags_comparison")
    if not isinstance(flags, dict) or set(flags) != {"actual", "expected", "mask"}:
        raise RatchetError(f"{record_hash}: malformed FLAGS evidence")
    return {
        "actual_termination": actual_termination,
        "expected_termination": expected_termination,
        "flags_mask": flags["mask"],
        "form": form,
        "mismatch_classes": mismatch_classes,
        "record_hash": record_hash,
        "signature_sha256": signature,
        "upstream_test_hash": upstream_hash,
    }


def build_scoreboard_rows(
    raw_summary: dict[str, Any],
    form_counts: dict[str, Counter[str]],
    failures: dict[str, dict[str, Any]],
) -> list[dict[str, Any]]:
    raw_forms = raw_summary.get("per_form")
    if not isinstance(raw_forms, list):
        raise RatchetError("raw summary is missing per-form results")
    raw_by_form: dict[str, dict[str, Any]] = {}
    for item in raw_forms:
        if not isinstance(item, dict) or set(item) != {
            "classification_counts",
            "form",
            "result_counts",
            "selected_count",
        }:
            raise RatchetError("raw summary has malformed per-form result")
        form = item["form"]
        parse_form(form)
        if form in raw_by_form:
            raise RatchetError("raw summary contains duplicate form")
        raw_by_form[form] = item
    if set(raw_by_form) != set(form_counts):
        raise RatchetError("raw summary structural forms differ from classification")

    mismatch_by_form: dict[str, Counter[str]] = defaultdict(Counter)
    termination_by_form: dict[str, Counter[str]] = defaultdict(Counter)
    for failure in failures.values():
        item = failure_entry(failure)
        for mismatch in item["mismatch_classes"]:
            mismatch_by_form[item["form"]][mismatch] += 1
        termination_by_form[item["form"]][item["actual_termination"]] += 1

    rows = []
    for form in sorted(form_counts):
        raw = raw_by_form[form]
        expected_counts = dict(sorted(form_counts[form].items()))
        if raw["classification_counts"] != expected_counts:
            raise RatchetError(f"{form}: raw classification counts differ")
        selected_count = sum(form_counts[form].values())
        if raw["selected_count"] != selected_count:
            raise RatchetError(f"{form}: selected count differs")
        result_counts = raw["result_counts"]
        if not isinstance(result_counts, dict):
            raise RatchetError(f"{form}: malformed result counts")
        applicable = form_counts[form].get("applicable", 0)
        passed = require_count(result_counts.get("pass", 0), f"{form}.pass")
        failed = sum(
            require_count(result_counts.get(kind, 0), f"{form}.{kind}")
            for kind in ("semantic_failure", "timeout", "crash")
        )
        skipped = require_count(result_counts.get("skip", 0), f"{form}.skip")
        if passed + failed != applicable:
            raise RatchetError(f"{form}: applicable result-count inconsistency")
        if skipped != selected_count - applicable:
            raise RatchetError(f"{form}: non-applicable skip-count inconsistency")
        opcode, subform = parse_form(form)
        for classification in TOP_LEVEL_CLASSIFICATIONS:
            selected = form_counts[form].get(classification, 0)
            if selected == 0:
                continue
            is_applicable = classification == "applicable"
            rows.append(
                {
                    "classification": classification,
                    "executed": selected if is_applicable else 0,
                    "fail": failed if is_applicable else 0,
                    "form": form,
                    "mismatch_classes": (
                        dict(sorted(mismatch_by_form[form].items()))
                        if is_applicable
                        else {}
                    ),
                    "opcode": opcode,
                    "pass": passed if is_applicable else 0,
                    "selected": selected,
                    "subform": subform,
                    "termination_classes": (
                        dict(sorted(termination_by_form[form].items()))
                        if is_applicable
                        else {}
                    ),
                }
            )
    rows.sort(key=lambda item: (item["form"], item["classification"]))
    return rows


def validate_scoreboard_row(row: Any, field: str) -> None:
    required = {
        "classification",
        "executed",
        "fail",
        "form",
        "mismatch_classes",
        "opcode",
        "pass",
        "selected",
        "subform",
        "termination_classes",
    }
    if not isinstance(row, dict) or set(row) != required:
        raise RatchetError(f"{field}: unknown structural record schema")
    reconstructed = form_from_parts(row["opcode"], row["subform"])
    if row["form"] != reconstructed:
        raise RatchetError(f"{field}: opcode/subform does not match form")
    if row["classification"] not in TOP_LEVEL_CLASSIFICATIONS:
        raise RatchetError(f"{field}: unknown classification")
    selected = require_count(row["selected"], f"{field}.selected")
    executed = require_count(row["executed"], f"{field}.executed")
    passed = require_count(row["pass"], f"{field}.pass")
    failed = require_count(row["fail"], f"{field}.fail")
    if row["classification"] == "applicable":
        if executed != selected or passed + failed != executed:
            raise RatchetError(f"{field}: selected/executed/pass/fail inconsistency")
    elif executed or passed or failed:
        raise RatchetError(f"{field}: non-applicable record counted as pass or fail")
    for name in ("termination_classes", "mismatch_classes"):
        values = row[name]
        if (
            not isinstance(values, dict)
            or list(values) != sorted(values)
            or not all(isinstance(key, str) and key for key in values)
        ):
            raise RatchetError(f"{field}.{name}: malformed counter")
        for key, value in values.items():
            require_count(value, f"{field}.{name}.{key}")
            if value > failed:
                raise RatchetError(f"{field}.{name}: class count exceeds failures")
        if name == "termination_classes" and sum(values.values()) != failed:
            raise RatchetError(f"{field}.{name}: does not cover failed records")


def validate_scoreboard(value: Any) -> None:
    required = {
        "applicable",
        "applicable_hash_set_sha256",
        "approved_predecessor_gate",
        "approved_predecessor_sha",
        "blocking",
        "classification_counts",
        "classification_hash_sets",
        "comparison_contract_id",
        "comparison_contract_sha256",
        "crashes",
        "dataset_id",
        "epoch_gate",
        "evaluated_sha",
        "executed",
        "fail",
        "failure_hash_set_sha256",
        "failure_shards",
        "failure_sidecar_canonical_set_sha256",
        "failure_sidecar_raw_set_sha256",
        "failure_signature_index_sha256",
        "immutable_m43_ci_failure_index_sha256",
        "immutable_m43_ci_summary_sha256",
        "immutable_m43_full_failure_index_sha256",
        "immutable_m43_full_summary_sha256",
        "mismatch_classes",
        "pass",
        "pass_hash_set_sha256",
        "profile",
        "raw_result_summary_sha256",
        "records",
        "schema",
        "schema_version",
        "scope",
        "scoreboard_digest",
        "selected",
        "selected_hash_set_sha256",
        "termination_classes",
        "timeouts",
    }
    if not isinstance(value, dict) or set(value) != required:
        unknown = sorted(set(value) - required) if isinstance(value, dict) else []
        missing = sorted(required - set(value)) if isinstance(value, dict) else []
        raise RatchetError(f"scoreboard: unknown schema; missing={missing} extra={unknown}")
    if (
        value["schema"] != "vaeg-upd9002-ssts-scoreboard-v1"
        or value["schema_version"] != 1
    ):
        raise RatchetError("scoreboard: unsupported schema version")
    if value["epoch_gate"] != EPOCH_GATE:
        raise RatchetError("scoreboard: wrong epoch gate")
    if value["approved_predecessor_gate"] != APPROVED_PREDECESSOR_GATE:
        raise RatchetError("scoreboard: wrong predecessor gate")
    if value["approved_predecessor_sha"] != APPROVED_PREDECESSOR_SHA:
        raise RatchetError("scoreboard: wrong predecessor SHA")
    require_sha(value["evaluated_sha"], "scoreboard.evaluated_sha")
    if value["profile"] not in {"architectural", "fingerprint"}:
        raise RatchetError("scoreboard: unknown profile")
    if value["scope"] not in {"ci", "full"}:
        raise RatchetError("scoreboard: unknown scope")
    if value["profile"] == "fingerprint" and value["scope"] != "full":
        raise RatchetError("scoreboard: fingerprint profile must use full scope")
    if value["blocking"] != (value["profile"] == "architectural"):
        raise RatchetError("scoreboard: blocking policy mismatch")
    expected_contract = f"upd9002-v20-{value['profile']}-v1"
    if value["comparison_contract_id"] != expected_contract:
        raise RatchetError("scoreboard: contract ID/profile mismatch")
    digest_fields = (
        "applicable_hash_set_sha256",
        "comparison_contract_sha256",
        "failure_hash_set_sha256",
        "failure_sidecar_canonical_set_sha256",
        "failure_sidecar_raw_set_sha256",
        "failure_signature_index_sha256",
        "immutable_m43_ci_failure_index_sha256",
        "immutable_m43_ci_summary_sha256",
        "immutable_m43_full_failure_index_sha256",
        "immutable_m43_full_summary_sha256",
        "pass_hash_set_sha256",
        "raw_result_summary_sha256",
        "scoreboard_digest",
        "selected_hash_set_sha256",
    )
    for field in digest_fields:
        require_sha256(value[field], f"scoreboard.{field}")
    rows = value["records"]
    if not isinstance(rows, list):
        raise RatchetError("scoreboard: records must be an array")
    for index, row in enumerate(rows):
        validate_scoreboard_row(row, f"scoreboard.records[{index}]")
    keys = [(row["form"], row["classification"]) for row in rows]
    if keys != sorted(keys) or len(keys) != len(set(keys)):
        raise RatchetError("scoreboard: structural records are nondeterministically ordered")
    if value["scoreboard_digest"] != sha256_bytes(canonical_bytes(rows)):
        raise RatchetError("scoreboard: structural digest mismatch")
    selected = sum(row["selected"] for row in rows)
    applicable = sum(
        row["selected"] for row in rows if row["classification"] == "applicable"
    )
    executed = sum(row["executed"] for row in rows)
    passed = sum(row["pass"] for row in rows)
    failed = sum(row["fail"] for row in rows)
    expected_totals = {
        "selected": selected,
        "applicable": applicable,
        "executed": executed,
        "pass": passed,
        "fail": failed,
    }
    for field, expected_value in expected_totals.items():
        if require_count(value[field], f"scoreboard.{field}") != expected_value:
            raise RatchetError(f"scoreboard: {field} total inconsistency")
    if applicable != executed or passed + failed != executed:
        raise RatchetError("scoreboard: selected/executed/pass/fail total inconsistency")
    for field in ("timeouts", "crashes"):
        require_count(value[field], f"scoreboard.{field}")
    classes = value["classification_counts"]
    class_sets = value["classification_hash_sets"]
    if (
        not isinstance(classes, dict)
        or set(classes) != set(TOP_LEVEL_CLASSIFICATIONS)
        or not isinstance(class_sets, dict)
        or set(class_sets) != set(TOP_LEVEL_CLASSIFICATIONS)
    ):
        raise RatchetError("scoreboard: classification categories are incomplete or unordered")
    row_classes = Counter()
    for row in rows:
        row_classes[row["classification"]] += row["selected"]
    for classification in TOP_LEVEL_CLASSIFICATIONS:
        if require_count(classes[classification], classification) != row_classes[
            classification
        ]:
            raise RatchetError("scoreboard: classification count inconsistency")
        require_sha256(class_sets[classification], f"classification {classification}")
    for counter_name in ("termination_classes", "mismatch_classes"):
        counter = value[counter_name]
        if not isinstance(counter, dict) or list(counter) != sorted(counter):
            raise RatchetError(f"scoreboard: malformed {counter_name}")
        for key, count in counter.items():
            if not isinstance(key, str) or not key:
                raise RatchetError(f"scoreboard: malformed {counter_name} key")
            require_count(count, f"scoreboard.{counter_name}.{key}")
    shards = value["failure_shards"]
    if not isinstance(shards, list):
        raise RatchetError("scoreboard: failure_shards must be an array")
    shard_paths = []
    shard_count = 0
    for index, shard in enumerate(shards):
        if not isinstance(shard, dict) or set(shard) != {
            "canonical_sha256",
            "failure_count",
            "path",
            "sha256",
        }:
            raise RatchetError(f"scoreboard.failure_shards[{index}]: unknown schema")
        if not isinstance(shard["path"], str) or not shard["path"].endswith(".json.gz"):
            raise RatchetError("scoreboard: malformed failure shard path")
        require_sha256(shard["sha256"], "failure shard sha256")
        require_sha256(shard["canonical_sha256"], "failure shard canonical sha256")
        shard_count += require_count(shard["failure_count"], "failure shard count")
        shard_paths.append(shard["path"])
    if shard_paths != sorted(shard_paths) or len(shard_paths) != len(set(shard_paths)):
        raise RatchetError("scoreboard: failure shards are nondeterministically ordered")
    if shard_count != failed:
        raise RatchetError("scoreboard: failure shard count inconsistency")


def write_failure_shards(
    failures: dict[str, dict[str, Any]],
    profile: str,
    scope: str,
    dataset_id: str,
    output_directory: pathlib.Path,
) -> tuple[list[dict[str, Any]], str, str, str]:
    if output_directory.exists() and any(output_directory.iterdir()):
        raise RatchetError(f"failure directory is not empty: {output_directory}")
    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for failure in failures.values():
        entry = failure_entry(failure)
        group = entry["form"][0].lower()
        groups[group].append(entry)
    output_directory.mkdir(parents=True, exist_ok=True)
    shard_entries = []
    signature_index = []
    for group, entries in sorted(groups.items()):
        entries.sort(key=lambda item: item["record_hash"])
        for entry in entries:
            signature_index.append(
                {
                    "record_hash": entry["record_hash"],
                    "signature_sha256": entry["signature_sha256"],
                }
            )
        payload = {
            "dataset_id": dataset_id,
            "failure_count": len(entries),
            "failures": entries,
            "group": group,
            "profile": profile,
            "schema": "vaeg-upd9002-ssts-scoreboard-failures-v1",
            "schema_version": 1,
            "scope": scope,
        }
        path = output_directory / f"{group}.json.gz"
        raw_digest, canonical_digest = write_deterministic_gzip(path, payload)
        shard_entries.append(
            {
                "canonical_sha256": canonical_digest,
                "failure_count": len(entries),
                "path": f"{output_directory.name}/{path.name}",
                "sha256": raw_digest,
            }
        )
    signature_index.sort(key=lambda item: item["record_hash"])
    canonical_set = sha256_bytes(
        canonical_bytes(
            [
                {
                    "count": item["failure_count"],
                    "path": item["path"],
                    "sha256": item["canonical_sha256"],
                }
                for item in shard_entries
            ]
        )
    )
    raw_set = sha256_bytes(
        canonical_bytes(
            [
                {
                    "count": item["failure_count"],
                    "path": item["path"],
                    "sha256": item["sha256"],
                }
                for item in shard_entries
            ]
        )
    )
    return (
        shard_entries,
        sha256_bytes(canonical_bytes(signature_index)),
        canonical_set,
        raw_set,
    )


def write_changed_failure_shards(
    changed_hashes: Iterable[str],
    predecessor_failures: dict[str, dict[str, Any]],
    candidate_failures: dict[str, dict[str, Any]],
    profile: str,
    scope: str,
    dataset_id: str,
    output_directory: pathlib.Path,
) -> list[dict[str, Any]]:
    if output_directory.exists() and any(output_directory.iterdir()):
        raise RatchetError(
            f"changed-failure directory is not empty: {output_directory}"
        )
    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record_hash in sorted(changed_hashes):
        before = predecessor_failures.get(record_hash)
        after = candidate_failures.get(record_hash)
        if before is None or after is None:
            raise RatchetError(
                f"{record_hash}: changed failure is absent from one side"
            )
        if before["form"] != after["form"]:
            raise RatchetError(f"{record_hash}: changed failure form differs")
        if before["upstream_test_hash"] != after["upstream_test_hash"]:
            raise RatchetError(
                f"{record_hash}: changed failure upstream identity differs"
            )
        groups[after["form"][0].lower()].append(
            {
                "after": after,
                "before": before,
                "record_hash": record_hash,
            }
        )
    output_directory.mkdir(parents=True, exist_ok=True)
    shard_entries = []
    for group, entries in sorted(groups.items()):
        entries.sort(key=lambda item: item["record_hash"])
        payload = {
            "changed_failure_count": len(entries),
            "changed_failures": entries,
            "dataset_id": dataset_id,
            "group": group,
            "profile": profile,
            "schema": "vaeg-upd9002-ssts-changed-failures-v1",
            "schema_version": 1,
            "scope": scope,
        }
        path = output_directory / f"{group}.json.gz"
        raw_digest, canonical_digest = write_deterministic_gzip(path, payload)
        shard_entries.append(
            {
                "canonical_sha256": canonical_digest,
                "changed_failure_count": len(entries),
                "path": f"{output_directory.name}/{path.name}",
                "sha256": raw_digest,
            }
        )
    return shard_entries


def generate_scoreboard(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    manifest_path: pathlib.Path,
    predecessor_support_map: pathlib.Path,
    candidate_support_map: pathlib.Path,
    raw_summary_path: pathlib.Path,
    contract_path: pathlib.Path,
    g43_manifest_path: pathlib.Path,
    taxonomy_path: pathlib.Path,
    hardware_pending_path: pathlib.Path,
    approved_divergences_path: pathlib.Path,
    profile: str,
    scope: str,
    evaluated_sha: str,
    output_path: pathlib.Path,
    failure_directory: pathlib.Path,
) -> dict[str, Any]:
    require_sha(evaluated_sha, "evaluated_sha")
    if evaluated_sha == APPROVED_PREDECESSOR_SHA:
        raise RatchetError("current-worktree self-comparison is forbidden")
    if profile not in {"architectural", "fingerprint"}:
        raise RatchetError("unknown profile")
    if scope not in {"ci", "full"} or (profile == "fingerprint" and scope != "full"):
        raise RatchetError("invalid profile scope")
    immutable = verify_immutable_m43(root, g43_manifest_path)
    contract, contract_digest = load_contract(contract_path)
    if contract["comparison_contract_id"] != f"upd9002-v20-{profile}-v1":
        raise RatchetError("comparison contract/profile mismatch")
    if contract["blocking"] != (profile == "architectural"):
        raise RatchetError("comparison contract/blocking mismatch")
    manifest = ssts.load_manifest(manifest_path)
    if manifest["dataset_id"] != immutable["dataset_id"]:
        raise RatchetError("dataset identity differs from immutable M43 evidence")
    known_gaps_path = root / "tests/ssts/baseline/upd9002_v20_known_gaps.json"
    taxonomy = read_json(taxonomy_path)
    known_gaps = read_json(known_gaps_path)
    hardware_pending = read_json(hardware_pending_path)
    _, taxonomy_rule_counts, taxonomy_hash_counts = validate_taxonomy(
        taxonomy, known_gaps, hardware_pending, sha256_file(known_gaps_path)
    )
    validate_content_registry(
        read_json(approved_divergences_path), "approved-target-divergences"
    )
    enumeration = enumerate_profiles(
        dataset_root,
        manifest,
        predecessor_support_map,
        candidate_support_map,
        scope,
    )
    raw_summary = read_json(raw_summary_path)
    if raw_summary.get("schema") != "vaeg-upd9002-ssts-result-v1":
        raise RatchetError("raw summary has unknown schema")
    if raw_summary.get("dataset_id") != manifest["dataset_id"]:
        raise RatchetError("raw summary dataset identity mismatch")
    if raw_summary.get("profile") != scope:
        raise RatchetError("raw summary scope mismatch")
    if profile == "architectural":
        if "flags_comparison" in raw_summary:
            raise RatchetError("architectural raw summary uses fingerprint FLAGS")
    elif raw_summary.get("flags_comparison") != "all16":
        raise RatchetError("fingerprint raw summary does not compare all FLAGS bits")
    failures = load_failure_records(raw_summary_path)
    applicable_hashes = enumeration["after_sets"]["applicable"]
    applicable_set = set(applicable_hashes)
    failure_set = set(failures)
    if not failure_set <= applicable_set:
        raise RatchetError("failure hash lies outside the applicable denominator")
    pass_hashes = [item for item in applicable_hashes if item not in failure_set]
    rows = build_scoreboard_rows(
        raw_summary, enumeration["after_form_counts"], failures
    )
    failure_shards, failure_index, canonical_sidecars, raw_sidecars = (
        write_failure_shards(
            failures,
            profile,
            scope,
            manifest["dataset_id"],
            failure_directory,
        )
    )
    if failure_index != raw_summary["failure_signature_index_sha256"]:
        raise RatchetError("generated failure index differs from raw result")
    classification_counts = {
        key: len(enumeration["after_sets"][key])
        for key in TOP_LEVEL_CLASSIFICATIONS
    }
    if sum(classification_counts.values()) != enumeration["selected_count"]:
        raise RatchetError("classification counts do not cover selected records")
    if {
        key: value
        for key, value in raw_summary["classification_counts"].items()
        if value
    } != {key: value for key, value in classification_counts.items() if value}:
        raise RatchetError("raw summary classification population mismatch")
    result_counts = raw_summary["result_counts"]
    passed = require_count(result_counts.get("pass", 0), "raw pass")
    failed = sum(
        require_count(result_counts.get(kind, 0), f"raw {kind}")
        for kind in ("semantic_failure", "timeout", "crash")
    )
    if passed != len(pass_hashes) or failed != len(failures):
        raise RatchetError("raw pass/failure population mismatch")
    mismatch_classes: Counter[str] = Counter()
    for failure in failures.values():
        mismatch_classes.update(failure_entry(failure)["mismatch_classes"])
    m43_ci = immutable["profiles"]["ci"]
    m43_full = immutable["profiles"]["full"]
    summary = {
        "applicable": len(applicable_hashes),
        "applicable_hash_set_sha256": hash_set_digest(applicable_hashes),
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "blocking": profile == "architectural",
        "classification_counts": classification_counts,
        "classification_hash_sets": enumeration["after_set_digests"],
        "comparison_contract_id": contract["comparison_contract_id"],
        "comparison_contract_sha256": contract_digest,
        "crashes": require_count(result_counts.get("crash", 0), "raw crashes"),
        "dataset_id": manifest["dataset_id"],
        "epoch_gate": EPOCH_GATE,
        "evaluated_sha": evaluated_sha,
        "executed": raw_summary["executed_records"],
        "fail": failed,
        "failure_hash_set_sha256": hash_set_digest(failure_set),
        "failure_shards": failure_shards,
        "failure_sidecar_canonical_set_sha256": canonical_sidecars,
        "failure_sidecar_raw_set_sha256": raw_sidecars,
        "failure_signature_index_sha256": failure_index,
        "immutable_m43_ci_failure_index_sha256": m43_ci["failure_index_sha256"],
        "immutable_m43_ci_summary_sha256": m43_ci["summary_sha256"],
        "immutable_m43_full_failure_index_sha256": m43_full[
            "failure_index_sha256"
        ],
        "immutable_m43_full_summary_sha256": m43_full["summary_sha256"],
        "mismatch_classes": dict(sorted(mismatch_classes.items())),
        "pass": passed,
        "pass_hash_set_sha256": hash_set_digest(pass_hashes),
        "profile": profile,
        "raw_result_summary_sha256": sha256_file(raw_summary_path),
        "records": rows,
        "schema": "vaeg-upd9002-ssts-scoreboard-v1",
        "schema_version": 1,
        "scope": scope,
        "scoreboard_digest": sha256_bytes(canonical_bytes(rows)),
        "selected": enumeration["selected_count"],
        "selected_hash_set_sha256": enumeration["selected_hash_set_sha256"],
        "termination_classes": raw_summary["termination_counts"],
        "timeouts": require_count(result_counts.get("timeout", 0), "raw timeouts"),
    }
    validate_scoreboard(summary)
    if summary["applicable_hash_set_sha256"] != immutable["profiles"][scope][
        "applicable_hash_set_sha256"
    ]:
        raise RatchetError("applicable hash set differs from immutable M43 evidence")
    if summary["selected_hash_set_sha256"] != immutable["profiles"][scope][
        "selected_hash_set_sha256"
    ]:
        raise RatchetError("selected hash set differs from immutable M43 evidence")
    # The counts are returned to callers for reporting without making them
    # part of the stable scoreboard schema.
    summary_for_write = copy.deepcopy(summary)
    write_json(output_path, summary_for_write)
    print(
        "ssts-scoreboard: "
        f"profile={profile} scope={scope} selected={summary['selected']} "
        f"applicable={summary['applicable']} pass={summary['pass']} "
        f"fail={summary['fail']} taxonomy_rules="
        f"{dict(sorted(taxonomy_rule_counts.items()))} taxonomy_hashes="
        f"{dict(sorted(taxonomy_hash_counts.items()))}"
    )
    return summary


def load_scoreboard_failures(
    summary_path: pathlib.Path, summary: dict[str, Any]
) -> dict[str, dict[str, Any]]:
    failures: dict[str, dict[str, Any]] = {}
    canonical_items = []
    raw_items = []
    for shard in summary["failure_shards"]:
        path = summary_path.parent / shard["path"]
        if not path.is_file():
            raise RatchetError(f"scoreboard failure shard is missing: {path}")
        if sha256_file(path) != shard["sha256"]:
            raise RatchetError(f"scoreboard failure shard byte digest differs: {path}")
        payload = read_deterministic_gzip(path)
        required = {
            "dataset_id",
            "failure_count",
            "failures",
            "group",
            "profile",
            "schema",
            "schema_version",
            "scope",
        }
        if not isinstance(payload, dict) or set(payload) != required:
            raise RatchetError(f"{path}: unknown failure-shard schema")
        if (
            payload["schema"] != "vaeg-upd9002-ssts-scoreboard-failures-v1"
            or payload["schema_version"] != 1
        ):
            raise RatchetError(f"{path}: unsupported failure-shard version")
        if (
            payload["dataset_id"] != summary["dataset_id"]
            or payload["profile"] != summary["profile"]
            or payload["scope"] != summary["scope"]
        ):
            raise RatchetError(f"{path}: failure-shard identity mismatch")
        entries = payload["failures"]
        if (
            not isinstance(entries, list)
            or payload["failure_count"] != len(entries)
            or shard["failure_count"] != len(entries)
        ):
            raise RatchetError(f"{path}: failure-shard count mismatch")
        hashes = []
        for index, entry in enumerate(entries):
            required_entry = {
                "actual_termination",
                "expected_termination",
                "flags_mask",
                "form",
                "mismatch_classes",
                "record_hash",
                "signature_sha256",
                "upstream_test_hash",
            }
            if not isinstance(entry, dict) or set(entry) != required_entry:
                raise RatchetError(f"{path}:{index}: unknown failure entry schema")
            record_hash = require_sha256(
                entry["record_hash"], f"{path}:{index}.record_hash"
            )
            require_sha256(
                entry["signature_sha256"], f"{path}:{index}.signature_sha256"
            )
            require_sha1(
                entry["upstream_test_hash"], f"{path}:{index}.upstream_test_hash"
            )
            parse_form(entry["form"])
            if entry["form"][0].lower() != payload["group"]:
                raise RatchetError(f"{path}:{index}: failure group mismatch")
            if (
                not isinstance(entry["mismatch_classes"], list)
                or entry["mismatch_classes"]
                != sorted(set(entry["mismatch_classes"]))
            ):
                raise RatchetError(f"{path}:{index}: mismatch classes are not canonical")
            if record_hash in failures:
                raise RatchetError("scoreboard failure shards overlap")
            failures[record_hash] = entry
            hashes.append(record_hash)
        if hashes != sorted(hashes):
            raise RatchetError(f"{path}: failure entries are nondeterministically ordered")
        canonical_digest = sha256_bytes(canonical_bytes(payload) + b"\n")
        if canonical_digest != shard["canonical_sha256"]:
            raise RatchetError(f"{path}: canonical digest mismatch")
        canonical_items.append(
            {
                "count": shard["failure_count"],
                "path": shard["path"],
                "sha256": shard["canonical_sha256"],
            }
        )
        raw_items.append(
            {
                "count": shard["failure_count"],
                "path": shard["path"],
                "sha256": shard["sha256"],
            }
        )
    if len(failures) != summary["fail"]:
        raise RatchetError("scoreboard failure shards do not cover all failures")
    if sha256_bytes(canonical_bytes(canonical_items)) != summary[
        "failure_sidecar_canonical_set_sha256"
    ]:
        raise RatchetError("scoreboard failure canonical-set digest mismatch")
    if sha256_bytes(canonical_bytes(raw_items)) != summary[
        "failure_sidecar_raw_set_sha256"
    ]:
        raise RatchetError("scoreboard failure raw-set digest mismatch")
    index = [
        {
            "record_hash": record_hash,
            "signature_sha256": failures[record_hash]["signature_sha256"],
        }
        for record_hash in sorted(failures)
    ]
    if sha256_bytes(canonical_bytes(index)) != summary[
        "failure_signature_index_sha256"
    ]:
        raise RatchetError("scoreboard failure index digest mismatch")
    if hash_set_digest(failures) != summary["failure_hash_set_sha256"]:
        raise RatchetError("scoreboard failure hash-set digest mismatch")
    return failures


def taxonomy_hash_kinds(
    taxonomy: dict[str, Any], known_gaps: dict[str, Any]
) -> dict[str, str]:
    kinds = {
        item["selector_sha256"]: item["gap_kind"]
        for item in taxonomy["annotations"]
    }
    result: dict[str, str] = {}
    for rule in known_gaps["rules"]:
        kind = kinds[selector_digest(rule["selector"])]
        for upstream_hash in rule["resolved_test_hashes"]:
            if upstream_hash in result:
                raise RatchetError("known-gap rules have overlapping hash ownership")
            result[upstream_hash] = kind
    return result


def divergence_hash_registry(
    registry: dict[str, dict[str, Any]]
) -> dict[str, dict[str, Any]]:
    result = {}
    for entry in registry.values():
        for upstream_hash in entry["resolved_test_hashes"]:
            if upstream_hash in result:
                raise RatchetError("approved divergences overlap")
            result[upstream_hash] = entry
    return result


def govern_classification_changes(
    changes: list[dict[str, Any]],
    predecessor_failures: set[str],
    predecessor_passes: set[str],
    candidate_passes: set[str],
    gap_kinds: dict[str, str],
    approved_divergences: dict[str, dict[str, Any]],
) -> None:
    for change in changes:
        before = change.get("before")
        after = change.get("after")
        record_hash = change.get("record_hash")
        upstream_hash = change.get("upstream_test_hash")
        if change.get("outcome_based_split"):
            raise RatchetError("outcome-based structural split")
        if before == "known_target_gap" and after == "applicable":
            if gap_kinds.get(upstream_hash) != "implementation_missing":
                raise RatchetError("invalid known-gap to applicable transition")
            if record_hash not in candidate_passes:
                raise RatchetError("newly applicable record does not pass")
            continue
        if before == "applicable" and after == "expected_target_divergence":
            if record_hash in predecessor_passes:
                raise RatchetError(
                    "previously passing applicable record moved to divergence"
                )
            if record_hash not in predecessor_failures:
                raise RatchetError("divergence did not fail in predecessor")
            entry = approved_divergences.get(upstream_hash)
            if entry is None or entry.get("review_status") != "approved":
                raise RatchetError("unapproved classification transition")
            continue
        if before == "applicable" and after in {
            "known_target_gap",
            "unsupported_fixture",
            "upstream_nonblocking",
        }:
            raise RatchetError("applicable record moved to a gap or nonblocking category")
        raise RatchetError(f"invalid classification transition: {before} -> {after}")


def enforce_ratchet_policy(
    policy: dict[str, Any],
    allow_changed_failure_signatures: bool = False,
) -> None:
    predecessor_sha = policy.get("predecessor_sha")
    if predecessor_sha is None:
        raise RatchetError("omitted predecessor SHA")
    if predecessor_sha != APPROVED_PREDECESSOR_SHA:
        raise RatchetError("wrong predecessor SHA")
    if not policy.get("predecessor_immutable", False):
        raise RatchetError("mutable-golden self-approval")
    evaluated_sha = require_sha(policy.get("evaluated_sha"), "evaluated_sha")
    if evaluated_sha == predecessor_sha:
        raise RatchetError("current-worktree self-comparison")
    if policy.get("dataset_before") != policy.get("dataset_after"):
        raise RatchetError("dataset identity mismatch")
    if policy.get("contract_before") != policy.get("contract_after"):
        raise RatchetError("comparison-contract identity mismatch")
    if policy.get("selected_before") != policy.get("selected_after"):
        raise RatchetError("selected hash-set mismatch")
    if (
        policy.get("applicable_before") != policy.get("applicable_after")
        and not policy.get("classification_changes")
    ):
        raise RatchetError("applicable hash-set mismatch without approved transition")
    before_failures = set(policy.get("failure_hashes_before", set()))
    after_failures = set(policy.get("failure_hashes_after", set()))
    newly_failing = after_failures - before_failures
    if newly_failing:
        raise RatchetError(f"new failure hash: {sorted(newly_failing)[0]}")
    signatures_before = policy.get("signatures_before", {})
    signatures_after = policy.get("signatures_after", {})
    changed = [
        record_hash
        for record_hash in sorted(before_failures & after_failures)
        if signatures_before.get(record_hash) != signatures_after.get(record_hash)
    ]
    if changed and not allow_changed_failure_signatures:
        raise RatchetError(f"changed failure signature: {changed[0]}")
    before_passes = policy.get("form_passes_before", {})
    after_passes = policy.get("form_passes_after", {})
    for form, before_count in before_passes.items():
        if after_passes.get(form, -1) < before_count:
            raise RatchetError(f"per-form pass-count decrease: {form}")
    if policy.get("timeouts", 0):
        raise RatchetError("timeout count is nonzero")
    if policy.get("crashes", 0):
        raise RatchetError("crash count is nonzero")
    govern_classification_changes(
        policy.get("classification_changes", []),
        before_failures,
        set(policy.get("pass_hashes_before", set())),
        set(policy.get("pass_hashes_after", set())),
        policy.get("gap_kinds", {}),
        policy.get("approved_divergences", {}),
    )


def validate_transition(value: Any) -> None:
    required = {
        "applicable_hash_set_after_sha256",
        "applicable_hash_set_before_sha256",
        "before_gate",
        "before_sha",
        "changed_failure_count",
        "changed_failure_shards",
        "classification_changes",
        "comparison_contract_id",
        "comparison_contract_sha256",
        "dataset_id",
        "epoch_gate",
        "evaluated_sha",
        "newly_failing",
        "newly_passing",
        "profile",
        "schema",
        "schema_version",
        "scope",
        "scoreboard_after_digest",
        "scoreboard_before_digest",
        "selected_hash_set_sha256",
    }
    if not isinstance(value, dict) or set(value) != required:
        raise RatchetError("transition: unknown schema")
    if (
        value["schema"] != "vaeg-upd9002-ssts-transition-v1"
        or value["schema_version"] != 1
    ):
        raise RatchetError("transition: unsupported schema version")
    if (
        value["before_gate"] != APPROVED_PREDECESSOR_GATE
        or value["before_sha"] != APPROVED_PREDECESSOR_SHA
        or value["epoch_gate"] != EPOCH_GATE
    ):
        raise RatchetError("transition: gate identity mismatch")
    require_sha(value["evaluated_sha"], "transition.evaluated_sha")
    if value["profile"] != "architectural" or value["scope"] not in {"ci", "full"}:
        raise RatchetError("transition: invalid blocking profile")
    for field in (
        "applicable_hash_set_after_sha256",
        "applicable_hash_set_before_sha256",
        "comparison_contract_sha256",
        "scoreboard_after_digest",
        "scoreboard_before_digest",
        "selected_hash_set_sha256",
    ):
        require_sha256(value[field], f"transition.{field}")
    for field in ("newly_failing", "newly_passing"):
        hashes = value[field]
        if not isinstance(hashes, list) or hashes != sorted(set(hashes)):
            raise RatchetError(f"transition: malformed or nondeterministic {field}")
        for item in hashes:
            require_sha256(item, f"transition.{field}")
    changes = value["classification_changes"]
    if not isinstance(changes, list):
        raise RatchetError("transition: classification_changes must be an array")
    change_keys = [item.get("record_hash") for item in changes if isinstance(item, dict)]
    if (
        len(change_keys) != len(changes)
        or change_keys != sorted(set(change_keys))
    ):
        raise RatchetError("transition: malformed classification changes")
    shards = value["changed_failure_shards"]
    if not isinstance(shards, list):
        raise RatchetError("transition: changed_failure_shards must be an array")
    total = 0
    paths = []
    for shard in shards:
        if not isinstance(shard, dict) or set(shard) != {
            "canonical_sha256",
            "changed_failure_count",
            "path",
            "sha256",
        }:
            raise RatchetError("transition: malformed changed-failure shard")
        total += require_count(
            shard["changed_failure_count"], "changed failure shard count"
        )
        require_sha256(shard["canonical_sha256"], "changed shard canonical digest")
        require_sha256(shard["sha256"], "changed shard raw digest")
        if (
            not isinstance(shard["path"], str)
            or not shard["path"].endswith(".json.gz")
            or ".." in pathlib.PurePosixPath(shard["path"]).parts
        ):
            raise RatchetError("transition: malformed changed-failure path")
        paths.append(shard["path"])
    if paths != sorted(paths) or len(paths) != len(set(paths)):
        raise RatchetError("transition: changed-failure shards are nondeterministic")
    if require_count(
        value["changed_failure_count"], "transition.changed_failure_count"
    ) != total:
        raise RatchetError("transition: changed-failure count mismatch")


def validate_normalized_failure_entry(value: Any, field: str) -> None:
    required = {
        "actual_termination",
        "expected_termination",
        "flags_mask",
        "form",
        "mismatch_classes",
        "record_hash",
        "signature_sha256",
        "upstream_test_hash",
    }
    if not isinstance(value, dict) or set(value) != required:
        raise RatchetError(f"{field}: unknown failure entry schema")
    require_sha256(value["record_hash"], f"{field}.record_hash")
    require_sha256(value["signature_sha256"], f"{field}.signature_sha256")
    require_sha1(value["upstream_test_hash"], f"{field}.upstream_test_hash")
    parse_form(value["form"])
    if (
        not isinstance(value["flags_mask"], str)
        or re.fullmatch(r"[0-9a-f]{4}", value["flags_mask"]) is None
    ):
        raise RatchetError(f"{field}: malformed FLAGS mask")
    if (
        not isinstance(value["mismatch_classes"], list)
        or value["mismatch_classes"]
        != sorted(set(value["mismatch_classes"]))
        or not all(
            isinstance(item, str) and item
            for item in value["mismatch_classes"]
        )
    ):
        raise RatchetError(f"{field}: malformed mismatch classes")
    for name in ("actual_termination", "expected_termination"):
        if not isinstance(value[name], str) or not value[name]:
            raise RatchetError(f"{field}: malformed {name}")


def load_transition_changed_failures(
    transition_path: pathlib.Path,
    transition: dict[str, Any],
) -> dict[str, dict[str, Any]]:
    changed: dict[str, dict[str, Any]] = {}
    for shard_index, shard in enumerate(transition["changed_failure_shards"]):
        path = transition_path.parent / shard["path"]
        if not path.is_file():
            raise RatchetError(f"changed-failure shard is missing: {path}")
        if sha256_file(path) != shard["sha256"]:
            raise RatchetError(f"{path}: changed-failure byte digest differs")
        payload = read_deterministic_gzip(path)
        required = {
            "changed_failure_count",
            "changed_failures",
            "dataset_id",
            "group",
            "profile",
            "schema",
            "schema_version",
            "scope",
        }
        if not isinstance(payload, dict) or set(payload) != required:
            raise RatchetError(f"{path}: unknown changed-failure schema")
        if (
            payload["schema"] != "vaeg-upd9002-ssts-changed-failures-v1"
            or payload["schema_version"] != 1
        ):
            raise RatchetError(f"{path}: unsupported changed-failure version")
        if (
            payload["dataset_id"] != transition["dataset_id"]
            or payload["profile"] != transition["profile"]
            or payload["scope"] != transition["scope"]
        ):
            raise RatchetError(f"{path}: changed-failure identity mismatch")
        group = payload["group"]
        if (
            not isinstance(group, str)
            or re.fullmatch(r"[0-9a-f]", group) is None
        ):
            raise RatchetError(f"{path}: malformed changed-failure group")
        entries = payload["changed_failures"]
        if (
            not isinstance(entries, list)
            or payload["changed_failure_count"] != len(entries)
            or shard["changed_failure_count"] != len(entries)
        ):
            raise RatchetError(f"{path}: changed-failure count mismatch")
        hashes = []
        for entry_index, entry in enumerate(entries):
            field = f"{path}:{entry_index}"
            if not isinstance(entry, dict) or set(entry) != {
                "after",
                "before",
                "record_hash",
            }:
                raise RatchetError(f"{field}: unknown changed-failure entry")
            record_hash = require_sha256(
                entry["record_hash"], f"{field}.record_hash"
            )
            validate_normalized_failure_entry(
                entry["before"], f"{field}.before"
            )
            validate_normalized_failure_entry(
                entry["after"], f"{field}.after"
            )
            if (
                entry["before"]["record_hash"] != record_hash
                or entry["after"]["record_hash"] != record_hash
                or entry["before"]["form"] != entry["after"]["form"]
                or entry["before"]["upstream_test_hash"]
                != entry["after"]["upstream_test_hash"]
                or entry["before"]["signature_sha256"]
                == entry["after"]["signature_sha256"]
            ):
                raise RatchetError(f"{field}: changed-failure sides mismatch")
            if entry["after"]["form"][0].lower() != group:
                raise RatchetError(f"{field}: changed-failure group mismatch")
            if record_hash in changed:
                raise RatchetError("changed-failure shards overlap")
            changed[record_hash] = entry
            hashes.append(record_hash)
        if hashes != sorted(hashes):
            raise RatchetError(
                f"{path}: changed failures are nondeterministically ordered"
            )
        if (
            sha256_bytes(canonical_bytes(payload) + b"\n")
            != shard["canonical_sha256"]
        ):
            raise RatchetError(f"{path}: changed-failure canonical digest differs")
    if len(changed) != transition["changed_failure_count"]:
        raise RatchetError("changed-failure shards do not cover the transition")
    return changed


def build_transition(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    manifest_path: pathlib.Path,
    predecessor_support_map: pathlib.Path,
    candidate_support_map: pathlib.Path,
    candidate_path: pathlib.Path,
    g43_manifest_path: pathlib.Path,
    taxonomy_path: pathlib.Path,
    hardware_pending_path: pathlib.Path,
    approved_divergences_path: pathlib.Path,
    predecessor_sha: str | None,
    output_path: pathlib.Path,
    changed_failure_directory: pathlib.Path | None = None,
) -> dict[str, Any]:
    immutable = verify_immutable_m43(root, g43_manifest_path)
    candidate = read_json(candidate_path)
    validate_scoreboard(candidate)
    if candidate_path.read_bytes() != canonical_bytes(candidate) + b"\n":
        raise RatchetError("candidate scoreboard is not canonically serialized")
    if candidate["profile"] != "architectural":
        raise RatchetError("ratchet applies only to the blocking architectural profile")
    scope = candidate["scope"]
    if candidate["approved_predecessor_sha"] != predecessor_sha:
        raise RatchetError("candidate and supplied predecessor SHA differ")
    candidate_failures = load_scoreboard_failures(candidate_path, candidate)
    manifest = ssts.load_manifest(manifest_path)
    enumeration = enumerate_profiles(
        dataset_root,
        manifest,
        predecessor_support_map,
        candidate_support_map,
        scope,
    )
    immutable_profile = immutable["profiles"][scope]
    if enumeration["selected_hash_set_sha256"] != immutable_profile[
        "selected_hash_set_sha256"
    ]:
        raise RatchetError("selected hash set differs from immutable predecessor")
    for classification in TOP_LEVEL_CLASSIFICATIONS:
        if enumeration["before_set_digests"][classification] != immutable_profile[
            "classification_hash_sets"
        ][classification]:
            raise RatchetError(
                f"predecessor classification set differs: {classification}"
            )
        if enumeration["after_set_digests"][classification] != candidate[
            "classification_hash_sets"
        ][classification]:
            raise RatchetError(
                f"candidate classification set differs: {classification}"
            )
    predecessor_summary_path = root / immutable_profile["summary_path"]
    predecessor_summary = read_json(predecessor_summary_path)
    if sha256_file(predecessor_summary_path) != immutable_profile["summary_sha256"]:
        raise RatchetError("mutable-golden self-approval")
    predecessor_failures_raw = load_failure_records(predecessor_summary_path)
    predecessor_failures = {
        record_hash: failure_entry(failure)
        for record_hash, failure in predecessor_failures_raw.items()
    }
    before_rows = build_scoreboard_rows(
        predecessor_summary,
        enumeration["before_form_counts"],
        predecessor_failures_raw,
    )
    before_failure_set = set(predecessor_failures)
    after_failure_set = set(candidate_failures)
    before_applicable = set(enumeration["before_sets"]["applicable"])
    after_applicable = set(enumeration["after_sets"]["applicable"])
    before_pass_set = before_applicable - before_failure_set
    after_pass_set = after_applicable - after_failure_set
    before_form_passes = {
        row["form"]: row["pass"]
        for row in before_rows
        if row["classification"] == "applicable"
    }
    after_form_passes = {
        row["form"]: row["pass"]
        for row in candidate["records"]
        if row["classification"] == "applicable"
    }
    taxonomy = read_json(taxonomy_path)
    known_gaps = read_json(
        root / "tests/ssts/baseline/upd9002_v20_known_gaps.json"
    )
    hardware_pending = read_json(hardware_pending_path)
    validate_taxonomy(
        taxonomy,
        known_gaps,
        hardware_pending,
        sha256_file(root / "tests/ssts/baseline/upd9002_v20_known_gaps.json"),
    )
    divergence_registry = validate_content_registry(
        read_json(approved_divergences_path), "approved-target-divergences"
    )
    gap_kinds = taxonomy_hash_kinds(taxonomy, known_gaps)
    approved_by_hash = divergence_hash_registry(divergence_registry)
    policy = {
        "applicable_after": candidate["applicable_hash_set_sha256"],
        "applicable_before": immutable_profile["applicable_hash_set_sha256"],
        "approved_divergences": approved_by_hash,
        "classification_changes": enumeration["classification_changes"],
        "contract_after": (
            candidate["comparison_contract_id"],
            candidate["comparison_contract_sha256"],
        ),
        "contract_before": (
            immutable_profile["comparison_contract_id"],
            immutable_profile["comparison_contract_sha256"],
        ),
        "crashes": candidate["crashes"],
        "dataset_after": candidate["dataset_id"],
        "dataset_before": immutable["dataset_id"],
        "evaluated_sha": candidate["evaluated_sha"],
        "failure_hashes_after": after_failure_set,
        "failure_hashes_before": before_failure_set,
        "form_passes_after": after_form_passes,
        "form_passes_before": before_form_passes,
        "gap_kinds": gap_kinds,
        "pass_hashes_after": after_pass_set,
        "pass_hashes_before": before_pass_set,
        "predecessor_immutable": True,
        "predecessor_sha": predecessor_sha,
        "selected_after": candidate["selected_hash_set_sha256"],
        "selected_before": immutable_profile["selected_hash_set_sha256"],
        "signatures_after": {
            key: value["signature_sha256"]
            for key, value in candidate_failures.items()
        },
        "signatures_before": {
            key: value["signature_sha256"]
            for key, value in predecessor_failures.items()
        },
        "timeouts": candidate["timeouts"],
    }
    changed = [
        record_hash
        for record_hash in sorted(before_failure_set & after_failure_set)
        if predecessor_failures[record_hash]["signature_sha256"]
        != candidate_failures[record_hash]["signature_sha256"]
    ]
    enforce_ratchet_policy(
        policy,
        allow_changed_failure_signatures=(
            changed_failure_directory is not None
        ),
    )
    if changed and changed_failure_directory is None:
        raise RatchetError("changed failures require explicit human review")
    changed_failure_shards = (
        write_changed_failure_shards(
            changed,
            predecessor_failures,
            candidate_failures,
            candidate["profile"],
            scope,
            candidate["dataset_id"],
            changed_failure_directory,
        )
        if changed_failure_directory is not None
        else []
    )
    transition = {
        "applicable_hash_set_after_sha256": candidate[
            "applicable_hash_set_sha256"
        ],
        "applicable_hash_set_before_sha256": immutable_profile[
            "applicable_hash_set_sha256"
        ],
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": predecessor_sha,
        "changed_failure_count": len(changed),
        "changed_failure_shards": changed_failure_shards,
        "classification_changes": enumeration["classification_changes"],
        "comparison_contract_id": candidate["comparison_contract_id"],
        "comparison_contract_sha256": candidate["comparison_contract_sha256"],
        "dataset_id": candidate["dataset_id"],
        "epoch_gate": EPOCH_GATE,
        "evaluated_sha": candidate["evaluated_sha"],
        "newly_failing": sorted(after_failure_set - before_failure_set),
        "newly_passing": sorted(before_failure_set - after_failure_set),
        "profile": "architectural",
        "schema": "vaeg-upd9002-ssts-transition-v1",
        "schema_version": 1,
        "scope": scope,
        "scoreboard_after_digest": candidate["scoreboard_digest"],
        "scoreboard_before_digest": sha256_bytes(canonical_bytes(before_rows)),
        "selected_hash_set_sha256": candidate["selected_hash_set_sha256"],
    }
    validate_transition(transition)
    write_json(output_path, transition)
    print(
        "ssts-ratchet: "
        f"scope={scope} newly_passing={len(transition['newly_passing'])} "
        f"newly_failing=0 changed_failures={len(changed)} classification_changes="
        f"{len(transition['classification_changes'])}"
    )
    return transition


def immutable_sidecar_digests(
    summary_path: pathlib.Path, summary: dict[str, Any]
) -> tuple[str, str]:
    canonical_items = [
        {
            "count": item["failure_count"],
            "path": item["path"],
            "sha256": item["canonical_sha256"],
        }
        for item in summary["failure_signature_files"]
    ]
    raw_items = [
        {
            "count": item["failure_count"],
            "path": item["path"],
            "sha256": sha256_file(summary_path.parent / item["path"]),
        }
        for item in summary["failure_signature_files"]
    ]
    return (
        sha256_bytes(canonical_bytes(canonical_items)),
        sha256_bytes(canonical_bytes(raw_items)),
    )


def create_g43_manifest(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    manifest_path: pathlib.Path,
    predecessor_support_map: pathlib.Path,
    candidate_support_map: pathlib.Path,
    architectural_contract_path: pathlib.Path,
) -> dict[str, Any]:
    manifest = ssts.load_manifest(manifest_path)
    contract, contract_digest = load_contract(architectural_contract_path)
    if contract["comparison_contract_id"] != "upd9002-v20-architectural-v1":
        raise RatchetError("G43 manifest requires the architectural contract")
    paths = [
        root / "tests/ssts/baseline/upd9002_v20_known_gaps.json",
        root / "tests/ssts/baseline/v20_native_g43_transition.json",
        root / "tests/ssts/baseline/v20_native_ci.json",
        *sorted(
            (root / "tests/ssts/baseline/v20_native_ci_failures").glob("*.json.gz")
        ),
        root / "tests/ssts/baseline/v20_native_full.json",
        *sorted(
            (root / "tests/ssts/baseline/v20_native_full_failures").glob("*.json.gz")
        ),
    ]
    files = [
        {
            "path": path.relative_to(root).as_posix(),
            "sha256": sha256_file(path),
            "size": path.stat().st_size,
        }
        for path in sorted(paths)
    ]
    profiles = {}
    for scope in ("ci", "full"):
        enumeration = enumerate_profiles(
            dataset_root,
            manifest,
            predecessor_support_map,
            candidate_support_map,
            scope,
        )
        if enumeration["classification_changes"]:
            raise RatchetError(
                f"{scope}: M43 and candidate support maps classify records differently"
            )
        summary_path = root / f"tests/ssts/baseline/v20_native_{scope}.json"
        summary = read_json(summary_path)
        failures = load_failure_records(summary_path)
        applicable = enumeration["before_sets"]["applicable"]
        failure_hashes = set(failures)
        if not failure_hashes <= set(applicable):
            raise RatchetError(f"{scope}: immutable failure outside applicable set")
        passes = [item for item in applicable if item not in failure_hashes]
        canonical_sidecars, raw_sidecars = immutable_sidecar_digests(
            summary_path, summary
        )
        profiles[scope] = {
            "applicable_hash_set_sha256": hash_set_digest(applicable),
            "classification_hash_sets": enumeration["before_set_digests"],
            "comparison_contract_id": contract["comparison_contract_id"],
            "comparison_contract_sha256": contract_digest,
            "failure_hash_set_sha256": hash_set_digest(failure_hashes),
            "failure_index_sha256": summary["failure_signature_index_sha256"],
            "failure_sidecar_canonical_set_sha256": canonical_sidecars,
            "failure_sidecar_raw_set_sha256": raw_sidecars,
            "pass_hash_set_sha256": hash_set_digest(passes),
            "selected_hash_set_sha256": enumeration["selected_hash_set_sha256"],
            "summary_path": summary_path.relative_to(root).as_posix(),
            "summary_sha256": sha256_file(summary_path),
            "upstream_classification_hash_sets": enumeration[
                "upstream_set_digests"
            ],
        }
    return {
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": manifest["dataset_id"],
        "files": files,
        "license": "BSD-2-Clause",
        "profiles": profiles,
        "schema": "vaeg-upd9002-ssts-immutable-epoch-v1",
        "schema_version": 1,
    }


def create_gap_taxonomy(root: pathlib.Path) -> dict[str, Any]:
    path = root / "tests/ssts/baseline/upd9002_v20_known_gaps.json"
    known_gaps = read_json(path)
    annotations = []
    for rule in known_gaps["rules"]:
        form = rule["selector"]["metadata_form"]
        # Opcode 63 is the 80286 ARPL slot and is documented absent from the
        # V20/V30-class target. Every other current gap selects an unimplemented
        # V20/V30 instruction or repeat-prefix form.
        kind = "documented_silicon_absent" if form == "63" else "implementation_missing"
        annotations.append(
            {
                "gap_kind": kind,
                "resolved_count": rule["resolved_count"],
                "resolved_test_hashes_sha256": rule[
                    "resolved_test_hashes_sha256"
                ],
                "selector_sha256": selector_digest(rule["selector"]),
            }
        )
    annotations.sort(key=lambda item: item["selector_sha256"])
    return {
        "annotations": annotations,
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": known_gaps["dataset_id"],
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-ssts-gap-taxonomy-v1",
        "schema_version": 1,
        "source_known_gaps_path": path.relative_to(root).as_posix(),
        "source_known_gaps_sha256": sha256_file(path),
    }


def verify_static(root: pathlib.Path) -> None:
    g43_path = root / "tests/ssts/epochs/g43/manifest.json"
    immutable = verify_immutable_m43(root, g43_path)
    architectural, architectural_digest = load_contract(
        root / "tests/ssts/contracts/upd9002_architectural_v1.json"
    )
    load_contract(root / "tests/ssts/contracts/upd9002_fingerprint_v1.json")
    for scope in ("ci", "full"):
        if (
            immutable["profiles"][scope]["comparison_contract_id"]
            != architectural["comparison_contract_id"]
            or immutable["profiles"][scope]["comparison_contract_sha256"]
            != architectural_digest
        ):
            raise RatchetError("immutable M43 architectural contract identity differs")
    known_gaps = read_json(
        root / "tests/ssts/baseline/upd9002_v20_known_gaps.json"
    )
    taxonomy = read_json(root / "tests/ssts/gap_taxonomy.json")
    hardware = read_json(root / "tests/ssts/hardware_pending.json")
    validate_taxonomy(
        taxonomy,
        known_gaps,
        hardware,
        sha256_file(root / "tests/ssts/baseline/upd9002_v20_known_gaps.json"),
    )
    validate_content_registry(
        read_json(root / "tests/ssts/approved_target_divergences.json"),
        "approved-target-divergences",
    )
    scoreboards = (
        root / "tests/ssts/scoreboard/g58_architectural_ci.json",
        root / "tests/ssts/scoreboard/g58_architectural_full.json",
        root / "tests/ssts/scoreboard/g58_fingerprint_full.json",
    )
    present = [path.is_file() for path in scoreboards]
    if any(present) and not all(present):
        raise RatchetError("G58 scoreboard artifact family is incomplete")
    evaluated_shas = set()
    for path in scoreboards:
        if not path.is_file():
            continue
        value = read_json(path)
        if path.read_bytes() != canonical_bytes(value) + b"\n":
            raise RatchetError(f"{path}: scoreboard serialization is not canonical")
        validate_scoreboard(value)
        load_scoreboard_failures(path, value)
        evaluated_shas.add(value["evaluated_sha"])
    if len(evaluated_shas) > 1:
        raise RatchetError("G58 scoreboards name different evaluated SHAs")
    transition_paths = (
        root / "tests/ssts/transitions/g58_architectural_ci_from_g57.json",
        root / "tests/ssts/transitions/g58_architectural_full_from_g57.json",
    )
    transition_present = [path.is_file() for path in transition_paths]
    if any(transition_present) and not all(transition_present):
        raise RatchetError("G58 transition artifact family is incomplete")
    for path in transition_paths:
        if not path.is_file():
            continue
        value = read_json(path)
        if path.read_bytes() != canonical_bytes(value) + b"\n":
            raise RatchetError(f"{path}: transition serialization is not canonical")
        validate_transition(value)
        load_transition_changed_failures(path, value)
        if evaluated_shas and value["evaluated_sha"] not in evaluated_shas:
            raise RatchetError("transition and scoreboard evaluated SHAs differ")
    print(
        "ssts-ratchet-static: immutable M43, contracts, taxonomy, registries, "
        f"scoreboards={sum(present)}, transitions={sum(transition_present)} passed"
    )


def empty_registry(dataset_id: str, kind: str) -> dict[str, Any]:
    return {
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": dataset_id,
        "entries": [],
        "license": "BSD-2-Clause",
        "schema": f"vaeg-upd9002-ssts-{kind}-v1",
        "schema_version": 1,
    }


def synthetic_scoreboard() -> dict[str, Any]:
    digest = sha256_bytes(b"synthetic")
    empty_digest = hash_set_digest([])
    rows = [
        {
            "classification": "known_target_gap",
            "executed": 0,
            "fail": 0,
            "form": "63",
            "mismatch_classes": {},
            "opcode": "63",
            "pass": 0,
            "selected": 1,
            "subform": "-",
            "termination_classes": {},
        },
        {
            "classification": "applicable",
            "executed": 1,
            "fail": 0,
            "form": "F7.7",
            "mismatch_classes": {},
            "opcode": "f7",
            "pass": 1,
            "selected": 1,
            "subform": "7",
            "termination_classes": {},
        },
    ]
    return {
        "applicable": 1,
        "applicable_hash_set_sha256": digest,
        "approved_predecessor_gate": APPROVED_PREDECESSOR_GATE,
        "approved_predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "blocking": True,
        "classification_counts": {
            "applicable": 1,
            "known_target_gap": 1,
            "expected_target_divergence": 0,
            "unsupported_fixture": 0,
            "upstream_nonblocking": 0,
        },
        "classification_hash_sets": {
            classification: digest for classification in TOP_LEVEL_CLASSIFICATIONS
        },
        "comparison_contract_id": "upd9002-v20-architectural-v1",
        "comparison_contract_sha256": digest,
        "crashes": 0,
        "dataset_id": "synthetic-dataset",
        "epoch_gate": EPOCH_GATE,
        "evaluated_sha": "1" * 40,
        "executed": 1,
        "fail": 0,
        "failure_hash_set_sha256": empty_digest,
        "failure_shards": [],
        "failure_sidecar_canonical_set_sha256": digest,
        "failure_sidecar_raw_set_sha256": digest,
        "failure_signature_index_sha256": digest,
        "immutable_m43_ci_failure_index_sha256": digest,
        "immutable_m43_ci_summary_sha256": digest,
        "immutable_m43_full_failure_index_sha256": digest,
        "immutable_m43_full_summary_sha256": digest,
        "mismatch_classes": {},
        "pass": 1,
        "pass_hash_set_sha256": digest,
        "profile": "architectural",
        "raw_result_summary_sha256": digest,
        "records": rows,
        "schema": "vaeg-upd9002-ssts-scoreboard-v1",
        "schema_version": 1,
        "scope": "ci",
        "scoreboard_digest": sha256_bytes(canonical_bytes(rows)),
        "selected": 2,
        "selected_hash_set_sha256": digest,
        "termination_classes": {"normal": 1},
        "timeouts": 0,
    }


def synthetic_transition() -> dict[str, Any]:
    digest = sha256_bytes(b"synthetic")
    return {
        "applicable_hash_set_after_sha256": digest,
        "applicable_hash_set_before_sha256": digest,
        "before_gate": APPROVED_PREDECESSOR_GATE,
        "before_sha": APPROVED_PREDECESSOR_SHA,
        "changed_failure_count": 0,
        "changed_failure_shards": [],
        "classification_changes": [],
        "comparison_contract_id": "upd9002-v20-architectural-v1",
        "comparison_contract_sha256": digest,
        "dataset_id": "synthetic-dataset",
        "epoch_gate": EPOCH_GATE,
        "evaluated_sha": "1" * 40,
        "newly_failing": [],
        "newly_passing": [],
        "profile": "architectural",
        "schema": "vaeg-upd9002-ssts-transition-v1",
        "schema_version": 1,
        "scope": "full",
        "scoreboard_after_digest": digest,
        "scoreboard_before_digest": digest,
        "selected_hash_set_sha256": digest,
    }


def synthetic_taxonomy_values() -> tuple[dict[str, Any], dict[str, Any], dict[str, Any]]:
    dataset_id = "synthetic-dataset"
    hashes = ["1" * 40, "2" * 40]
    selector = {
        "metadata_form": "63",
        "opcode": "0x63",
        "repeat_prefix": "none",
    }
    rule = {
        "evidence": {"source": "synthetic"},
        "reason": "synthetic closed rule",
        "resolved_count": len(hashes),
        "resolved_test_hashes": hashes,
        "resolved_test_hashes_sha256": upstream_hash_set_digest(hashes),
        "selector": selector,
    }
    known = {
        "dataset_id": dataset_id,
        "profile": "full",
        "resolved_record_count": len(hashes),
        "rule_count": 1,
        "rules": [rule],
        "schema": "vaeg-upd9002-ssts-known-gaps-v1",
    }
    taxonomy = {
        "annotations": [
            {
                "gap_kind": "implementation_missing",
                "resolved_count": len(hashes),
                "resolved_test_hashes_sha256": upstream_hash_set_digest(hashes),
                "selector_sha256": selector_digest(selector),
            }
        ],
        "copyright": "Copyright (c) 2026 Nakata Maho",
        "dataset_id": dataset_id,
        "license": "BSD-2-Clause",
        "schema": "vaeg-upd9002-ssts-gap-taxonomy-v1",
        "schema_version": 1,
        "source_known_gaps_path": "tests/ssts/baseline/synthetic.json",
        "source_known_gaps_sha256": sha256_bytes(canonical_bytes(known) + b"\n"),
    }
    return taxonomy, known, empty_registry(dataset_id, "hardware-pending")


def registry_entry(
    selector: dict[str, Any], hashes: list[str], milestone: str = "M58"
) -> dict[str, Any]:
    return {
        "evidence": {"source": "synthetic"},
        "first_introduced_milestone": milestone,
        "reason": "synthetic closed evidence",
        "resolved_count": len(hashes),
        "resolved_test_hashes": hashes,
        "resolved_test_hashes_sha256": upstream_hash_set_digest(hashes),
        "review_status": "approved",
        "selector": selector,
        "selector_sha256": selector_digest(selector),
    }


def expect_ratchet_error(
    name: str,
    function: Callable[[], Any],
    tests: list[str],
) -> None:
    try:
        function()
    except (RatchetError, ssts.CorpusError):
        tests.append(name)
        return
    raise AssertionError(f"negative test did not reject: {name}")


def selftest() -> None:
    tests: list[str] = []
    h1 = sha256_bytes(b"failure-one")
    h2 = sha256_bytes(b"failure-two")
    base_policy = {
        "applicable_after": "applicable",
        "applicable_before": "applicable",
        "approved_divergences": {},
        "classification_changes": [],
        "contract_after": ("contract", "digest"),
        "contract_before": ("contract", "digest"),
        "crashes": 0,
        "dataset_after": "dataset",
        "dataset_before": "dataset",
        "evaluated_sha": "1" * 40,
        "failure_hashes_after": {h1},
        "failure_hashes_before": {h1},
        "form_passes_after": {"F7.7": 10},
        "form_passes_before": {"F7.7": 10},
        "gap_kinds": {},
        "pass_hashes_after": {h2},
        "pass_hashes_before": {h2},
        "predecessor_immutable": True,
        "predecessor_sha": APPROVED_PREDECESSOR_SHA,
        "selected_after": "selected",
        "selected_before": "selected",
        "signatures_after": {h1: "a"},
        "signatures_before": {h1: "a"},
        "timeouts": 0,
    }
    enforce_ratchet_policy(base_policy)
    tests.append("valid-ratchet")

    mutations = (
        ("mutable-golden self-approval", "predecessor_immutable", False),
        ("current-worktree self-comparison", "evaluated_sha", APPROVED_PREDECESSOR_SHA),
        ("omitted predecessor SHA", "predecessor_sha", None),
        ("wrong predecessor SHA", "predecessor_sha", "2" * 40),
        ("dataset identity mismatch", "dataset_after", "other-dataset"),
        ("comparison-contract identity mismatch", "contract_after", ("other", "digest")),
        ("selected hash-set mismatch", "selected_after", "other-selected"),
        ("timeout", "timeouts", 1),
        ("crash", "crashes", 1),
    )
    for name, key, value in mutations:
        policy = copy.deepcopy(base_policy)
        policy[key] = value
        expect_ratchet_error(
            name, lambda policy=policy: enforce_ratchet_policy(policy), tests
        )

    policy = copy.deepcopy(base_policy)
    policy["failure_hashes_after"].add(h2)
    policy["signatures_after"][h2] = "b"
    expect_ratchet_error(
        "new failure hash", lambda: enforce_ratchet_policy(policy), tests
    )
    policy = copy.deepcopy(base_policy)
    policy["form_passes_after"]["F7.7"] = 9
    expect_ratchet_error(
        "per-form pass-count decrease", lambda: enforce_ratchet_policy(policy), tests
    )
    policy = copy.deepcopy(base_policy)
    policy["signatures_after"][h1] = "changed"
    expect_ratchet_error(
        "changed failure signature", lambda: enforce_ratchet_policy(policy), tests
    )

    invalid_change = {
        "after": "applicable",
        "before": "known_target_gap",
        "form": "63",
        "record_hash": h2,
        "upstream_test_hash": "1" * 40,
    }
    policy = copy.deepcopy(base_policy)
    policy["classification_changes"] = [invalid_change]
    policy["gap_kinds"] = {"1" * 40: "documented_silicon_absent"}
    expect_ratchet_error(
        "invalid classification transition",
        lambda: enforce_ratchet_policy(policy),
        tests,
    )
    policy = copy.deepcopy(base_policy)
    policy["classification_changes"] = [
        {
            "after": "expected_target_divergence",
            "before": "applicable",
            "form": "F7.7",
            "record_hash": h2,
            "upstream_test_hash": "2" * 40,
        }
    ]
    expect_ratchet_error(
        "previously passing applicable moved to divergence",
        lambda: enforce_ratchet_policy(policy),
        tests,
    )
    for destination in (
        "known_target_gap",
        "unsupported_fixture",
        "upstream_nonblocking",
    ):
        policy = copy.deepcopy(base_policy)
        policy["classification_changes"] = [
            {
                "after": destination,
                "before": "applicable",
                "form": "F7.7",
                "record_hash": h1,
                "upstream_test_hash": "1" * 40,
            }
        ]
        expect_ratchet_error(
            f"applicable moved to {destination}",
            lambda policy=policy: enforce_ratchet_policy(policy),
            tests,
        )
    policy = copy.deepcopy(base_policy)
    split = copy.deepcopy(invalid_change)
    split["outcome_based_split"] = True
    policy["classification_changes"] = [split]
    expect_ratchet_error(
        "outcome-based structural split",
        lambda: enforce_ratchet_policy(policy),
        tests,
    )

    scoreboard = synthetic_scoreboard()
    validate_scoreboard(scoreboard)
    tests.append("valid-scoreboard-schema")
    assert canonical_bytes(scoreboard) == canonical_bytes(copy.deepcopy(scoreboard))
    tests.append("canonical serialization")
    assert hash_set_digest([h2, h1]) == hash_set_digest([h1, h2])
    tests.append("deterministic digest generation")
    for name, mutate in (
        (
            "schema-version rejection",
            lambda item: item.__setitem__("schema_version", 2),
        ),
        (
            "malformed opcode/subform rejection",
            lambda item: item["records"][1].__setitem__("opcode", "F7"),
        ),
        (
            "selected/executed/pass/fail consistency",
            lambda item: item["records"][1].__setitem__("pass", 0),
        ),
        (
            "classification consistency",
            lambda item: item["classification_counts"].__setitem__("applicable", 0),
        ),
        (
            "deterministic ordering",
            lambda item: item["records"].reverse(),
        ),
    ):
        invalid = copy.deepcopy(scoreboard)
        mutate(invalid)
        expect_ratchet_error(name, lambda invalid=invalid: validate_scoreboard(invalid), tests)

    taxonomy, known, hardware = synthetic_taxonomy_values()
    validate_taxonomy(taxonomy, known, hardware)
    tests.append("valid-gap-taxonomy")
    invalid = copy.deepcopy(taxonomy)
    invalid["annotations"][0]["gap_kind"] = "unknown"
    expect_ratchet_error(
        "unknown gap_kind",
        lambda: validate_taxonomy(invalid, known, hardware),
        tests,
    )
    invalid = copy.deepcopy(taxonomy)
    invalid["annotations"] = []
    expect_ratchet_error(
        "missing gap_kind",
        lambda: validate_taxonomy(invalid, known, hardware),
        tests,
    )
    unverified = copy.deepcopy(taxonomy)
    unverified["annotations"][0]["gap_kind"] = "target_support_unverified"
    expect_ratchet_error(
        "missing hardware-pending coverage",
        lambda: validate_taxonomy(unverified, known, hardware),
        tests,
    )
    invalid = copy.deepcopy(taxonomy)
    invalid["annotations"][0]["selector_sha256"] = sha256_bytes(b"other")
    expect_ratchet_error(
        "selector mismatch between taxonomy and registry",
        lambda: validate_taxonomy(invalid, known, hardware),
        tests,
    )
    invalid = copy.deepcopy(taxonomy)
    invalid["annotations"][0]["resolved_test_hashes_sha256"] = sha256_bytes(b"other")
    expect_ratchet_error(
        "resolved-hash mismatch",
        lambda: validate_taxonomy(invalid, known, hardware),
        tests,
    )
    invalid = copy.deepcopy(taxonomy)
    invalid["annotations"][0]["resolved_count"] = 1
    expect_ratchet_error(
        "resolved-count mismatch",
        lambda: validate_taxonomy(invalid, known, hardware),
        tests,
    )

    pending = empty_registry("synthetic-dataset", "hardware-pending")
    pending_entry = registry_entry(
        known["rules"][0]["selector"], known["rules"][0]["resolved_test_hashes"]
    )
    pending_entry["resolved_test_hashes_sha256"] = sha256_bytes(b"wrong")
    pending["entries"] = [pending_entry]
    expect_ratchet_error(
        "sorted-hash-digest mismatch",
        lambda: validate_content_registry(pending, "hardware-pending"),
        tests,
    )
    open_registry = empty_registry("synthetic-dataset", "hardware-pending")
    open_selector = {"opcode": "*"}
    entry = registry_entry({"opcode": "0x63"}, ["1" * 40])
    entry["selector"] = open_selector
    entry["selector_sha256"] = sha256_bytes(canonical_bytes(open_selector))
    open_registry["entries"] = [entry]
    expect_ratchet_error(
        "open-ended selector",
        lambda: validate_content_registry(open_registry, "hardware-pending"),
        tests,
    )
    overlap = empty_registry("synthetic-dataset", "hardware-pending")
    overlap["entries"] = sorted(
        [
            registry_entry({"opcode": "0x63"}, ["1" * 40]),
            registry_entry({"opcode": "0x64"}, ["1" * 40]),
        ],
        key=lambda item: item["selector_sha256"],
    )
    expect_ratchet_error(
        "overlapping registry ownership",
        lambda: validate_content_registry(overlap, "hardware-pending"),
        tests,
    )

    transition = synthetic_transition()
    validate_transition(transition)
    tests.append("valid-transition")
    invalid_transition = copy.deepcopy(transition)
    invalid_transition["newly_passing"] = [h2, h1]
    expect_ratchet_error(
        "malformed or nondeterministic transition artifact",
        lambda: validate_transition(invalid_transition),
        tests,
    )

    with tempfile.TemporaryDirectory(prefix="vaeg-m58-selftest-") as temporary:
        temporary_path = pathlib.Path(temporary)
        first = temporary_path / "first.json.gz"
        second = temporary_path / "second.json.gz"
        payload = {"schema": "synthetic", "values": [1, 2, 3]}
        write_deterministic_gzip(first, payload)
        write_deterministic_gzip(second, payload)
        if first.read_bytes() != second.read_bytes():
            raise AssertionError("deterministic gzip generation differs")
        tests.append("deterministic gzip generation")
        canonical = canonical_bytes(payload) + b"\n"
        stored_deflate = (
            b"\x01"
            + struct.pack("<HH", len(canonical), len(canonical) ^ 0xFFFF)
            + canonical
        )
        portable = temporary_path / "portable.json.gz"
        portable.write_bytes(
            b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff"
            + stored_deflate
            + struct.pack(
                "<II",
                zlib.crc32(canonical) & 0xFFFFFFFF,
                len(canonical) & 0xFFFFFFFF,
            )
        )
        if read_deterministic_gzip(portable) != payload:
            raise AssertionError("portable deterministic gzip validation differs")
        tests.append("portable deterministic gzip validation")
        nondeterministic = temporary_path / "nondeterministic.json.gz"
        nondeterministic.write_bytes(gzip.compress(canonical, mtime=1))
        expect_ratchet_error(
            "nondeterministic gzip shard",
            lambda: read_deterministic_gzip(nondeterministic),
            tests,
        )
        evidence = temporary_path / "tests/ssts/baseline/evidence.json"
        evidence.parent.mkdir(parents=True)
        evidence.write_bytes(b"immutable\n")
        file_entry = {
            "path": "tests/ssts/baseline/evidence.json",
            "sha256": sha256_file(evidence),
            "size": evidence.stat().st_size,
        }
        verify_immutable_file_entries(temporary_path, [file_entry])
        evidence.write_bytes(b"modified\n")
        expect_ratchet_error(
            "modified immutable M43 evidence",
            lambda: verify_immutable_file_entries(temporary_path, [file_entry]),
            tests,
        )
    print(
        "ssts-ratchet-selftest: "
        f"{len(tests)} positive and fail-closed checks passed: "
        + ", ".join(tests)
    )


def ci_enforce(
    root: pathlib.Path,
    dataset_root: pathlib.Path,
    worker: pathlib.Path,
    evaluated_sha: str,
    output_root: pathlib.Path,
) -> None:
    require_sha(evaluated_sha, "evaluated_sha")
    manifest_path = root / "tests/ssts/v20_dataset_manifest.json"
    manifest = ssts.load_manifest(manifest_path)
    output_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix="g58-architectural-ci-", dir=output_root
    ) as temporary:
        directory = pathlib.Path(temporary)
        raw_summary = directory / "raw.json"
        raw_failures = directory / "raw_failures"
        result = ssts.run_profile(
            dataset_root,
            manifest,
            root / "tools/qa/golden/upd9002_support_map_m48.csv",
            worker,
            "ci",
            300.0,
            "defined",
        )
        ssts.externalize_failure_signatures(result, raw_failures)
        write_json(raw_summary, result)
        scoreboard = directory / "g58_architectural_ci.json"
        generate_scoreboard(
            root,
            dataset_root,
            manifest_path,
            root / "tools/qa/golden/upd9002_support_map_m42.csv",
            root / "tools/qa/golden/upd9002_support_map_m48.csv",
            raw_summary,
            root / "tests/ssts/contracts/upd9002_architectural_v1.json",
            root / "tests/ssts/epochs/g43/manifest.json",
            root / "tests/ssts/gap_taxonomy.json",
            root / "tests/ssts/hardware_pending.json",
            root / "tests/ssts/approved_target_divergences.json",
            "architectural",
            "ci",
            evaluated_sha,
            scoreboard,
            directory / "g58_architectural_ci_failures",
        )
        build_transition(
            root,
            dataset_root,
            manifest_path,
            root / "tools/qa/golden/upd9002_support_map_m42.csv",
            root / "tools/qa/golden/upd9002_support_map_m48.csv",
            scoreboard,
            root / "tests/ssts/epochs/g43/manifest.json",
            root / "tests/ssts/gap_taxonomy.json",
            root / "tests/ssts/hardware_pending.json",
            root / "tests/ssts/approved_target_divergences.json",
            APPROVED_PREDECESSOR_SHA,
            directory / "transition.json",
        )
    print(
        "ssts-ratchet-ci: deterministic architectural CI profile passed "
        f"against {APPROVED_PREDECESSOR_GATE} {APPROVED_PREDECESSOR_SHA}"
    )


def path_from_root(root: pathlib.Path, value: pathlib.Path) -> pathlib.Path:
    return value.resolve() if value.is_absolute() else (root / value).resolve()


def add_root_argument(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))


def add_dataset_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--dataset-root", type=pathlib.Path, required=True)
    parser.add_argument(
        "--manifest",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/v20_dataset_manifest.json"),
    )
    parser.add_argument(
        "--predecessor-support-map",
        type=pathlib.Path,
        default=pathlib.Path("tools/qa/golden/upd9002_support_map_m42.csv"),
    )
    parser.add_argument(
        "--candidate-support-map",
        type=pathlib.Path,
        default=pathlib.Path("tools/qa/golden/upd9002_support_map_m48.csv"),
    )


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("selftest", help="run positive and fail-closed tests")
    static = subparsers.add_parser(
        "verify-static", help="verify committed manifests, registries, and artifacts"
    )
    add_root_argument(static)

    emit_g43 = subparsers.add_parser(
        "emit-g43", help="create immutable G43 content references"
    )
    add_root_argument(emit_g43)
    add_dataset_arguments(emit_g43)
    emit_g43.add_argument(
        "--contract",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/contracts/upd9002_architectural_v1.json"),
    )
    emit_g43.add_argument("--output", type=pathlib.Path, required=True)

    emit_taxonomy = subparsers.add_parser(
        "emit-taxonomy", help="create complete content-addressed gap annotations"
    )
    add_root_argument(emit_taxonomy)
    emit_taxonomy.add_argument("--output", type=pathlib.Path, required=True)

    generate = subparsers.add_parser(
        "generate", help="generate one canonical G58 scoreboard family"
    )
    add_root_argument(generate)
    add_dataset_arguments(generate)
    generate.add_argument("--raw-summary", type=pathlib.Path, required=True)
    generate.add_argument("--contract", type=pathlib.Path, required=True)
    generate.add_argument(
        "--g43-manifest",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/epochs/g43/manifest.json"),
    )
    generate.add_argument(
        "--taxonomy",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/gap_taxonomy.json"),
    )
    generate.add_argument(
        "--hardware-pending",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/hardware_pending.json"),
    )
    generate.add_argument(
        "--approved-divergences",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/approved_target_divergences.json"),
    )
    generate.add_argument(
        "--profile", choices=("architectural", "fingerprint"), required=True
    )
    generate.add_argument("--scope", choices=("ci", "full"), required=True)
    generate.add_argument("--evaluated-sha", required=True)
    generate.add_argument("--output", type=pathlib.Path, required=True)
    generate.add_argument("--failure-directory", type=pathlib.Path, required=True)

    ratchet = subparsers.add_parser(
        "ratchet", help="enforce a blocking scoreboard against immutable G57"
    )
    add_root_argument(ratchet)
    add_dataset_arguments(ratchet)
    ratchet.add_argument("--candidate", type=pathlib.Path, required=True)
    ratchet.add_argument(
        "--g43-manifest",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/epochs/g43/manifest.json"),
    )
    ratchet.add_argument(
        "--taxonomy",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/gap_taxonomy.json"),
    )
    ratchet.add_argument(
        "--hardware-pending",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/hardware_pending.json"),
    )
    ratchet.add_argument(
        "--approved-divergences",
        type=pathlib.Path,
        default=pathlib.Path("tests/ssts/approved_target_divergences.json"),
    )
    ratchet.add_argument("--predecessor-sha", required=True)
    ratchet.add_argument("--output", type=pathlib.Path, required=True)

    ci = subparsers.add_parser(
        "ci-enforce", help="run and ratchet the verified architectural CI profile"
    )
    add_root_argument(ci)
    ci.add_argument("--dataset-root", type=pathlib.Path, required=True)
    ci.add_argument("--worker", type=pathlib.Path, required=True)
    ci.add_argument("--evaluated-sha", required=True)
    ci.add_argument("--output-root", type=pathlib.Path, required=True)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        if arguments.command == "selftest":
            selftest()
            return 0
        root = arguments.root.resolve()
        if arguments.command == "verify-static":
            verify_static(root)
        elif arguments.command == "emit-g43":
            value = create_g43_manifest(
                root,
                arguments.dataset_root.resolve(),
                path_from_root(root, arguments.manifest),
                path_from_root(root, arguments.predecessor_support_map),
                path_from_root(root, arguments.candidate_support_map),
                path_from_root(root, arguments.contract),
            )
            write_json(arguments.output.resolve(), value)
            print(
                "ssts-g43-manifest: "
                f"files={len(value['files'])} dataset_id={value['dataset_id']}"
            )
        elif arguments.command == "emit-taxonomy":
            value = create_gap_taxonomy(root)
            write_json(arguments.output.resolve(), value)
            counts = Counter(
                annotation["gap_kind"] for annotation in value["annotations"]
            )
            print(
                "ssts-gap-taxonomy: "
                f"rules={len(value['annotations'])} "
                f"kinds={dict(sorted(counts.items()))}"
            )
        elif arguments.command == "generate":
            generate_scoreboard(
                root,
                arguments.dataset_root.resolve(),
                path_from_root(root, arguments.manifest),
                path_from_root(root, arguments.predecessor_support_map),
                path_from_root(root, arguments.candidate_support_map),
                arguments.raw_summary.resolve(),
                path_from_root(root, arguments.contract),
                path_from_root(root, arguments.g43_manifest),
                path_from_root(root, arguments.taxonomy),
                path_from_root(root, arguments.hardware_pending),
                path_from_root(root, arguments.approved_divergences),
                arguments.profile,
                arguments.scope,
                arguments.evaluated_sha,
                arguments.output.resolve(),
                arguments.failure_directory.resolve(),
            )
        elif arguments.command == "ratchet":
            build_transition(
                root,
                arguments.dataset_root.resolve(),
                path_from_root(root, arguments.manifest),
                path_from_root(root, arguments.predecessor_support_map),
                path_from_root(root, arguments.candidate_support_map),
                arguments.candidate.resolve(),
                path_from_root(root, arguments.g43_manifest),
                path_from_root(root, arguments.taxonomy),
                path_from_root(root, arguments.hardware_pending),
                path_from_root(root, arguments.approved_divergences),
                arguments.predecessor_sha,
                arguments.output.resolve(),
            )
        elif arguments.command == "ci-enforce":
            ci_enforce(
                root,
                arguments.dataset_root.resolve(),
                arguments.worker.resolve(),
                arguments.evaluated_sha,
                arguments.output_root.resolve(),
            )
        else:
            raise AssertionError(arguments.command)
    except (OSError, RatchetError, ssts.CorpusError, subprocess.SubprocessError) as error:
        print(f"ssts-ratchet-error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

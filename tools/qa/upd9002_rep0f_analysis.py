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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
"""Generate the behavior-neutral M47 REP+0F research artifacts."""

from __future__ import annotations

import argparse
import csv
import gzip
import hashlib
import importlib.util
import io
import json
import pathlib
import re
import sys
from collections import Counter
from typing import Any, Iterable


sys.dont_write_bytecode = True


MATRIX_PATH = pathlib.Path(
    "tools/qa/golden/upd9002_rep0f_instruction_matrix_m47.csv"
)
CORPUS_PATH = pathlib.Path(
    "tests/ssts/baseline/upd9002_rep0f_corpus_m47.json"
)
STATE_PATH = pathlib.Path("tests/upd9002/protected_state_inventory_m47.json")
TRANSITION_PATH = pathlib.Path(
    "tools/qa/golden/upd9002_rep0f_transition_manifest_m47.json"
)
DOCUMENT_PATH = pathlib.Path(
    "docs/agents/research/m47_upd9002_rep0f_documents.json"
)

MATRIX_COLUMNS = (
    "case_id",
    "prefix",
    "second_byte",
    "instruction_bytes",
    "manual_mnemonic",
    "modrm_constraints",
    "initial_registers",
    "initial_flags",
    "initial_memory",
    "current_dispatch_path",
    "current_handler_or_result",
    "expected_ip_advance",
    "expected_registers",
    "expected_flags",
    "memory_and_io_effects",
    "interrupt_or_exception",
    "architectural_prefix_status",
    "evidence_source",
    "confidence",
    "ssts_metadata_form",
    "ssts_empty_prefetch_records",
    "ssts_populated_prefetch_records",
    "hardware_probe_cases",
)

MANUAL_FORMS = {
    0x10: "TEST1 r/m8,CL",
    0x11: "TEST1 r/m16,CL",
    0x12: "CLR1 r/m8,CL",
    0x13: "CLR1 r/m16,CL",
    0x14: "SET1 r/m8,CL",
    0x15: "SET1 r/m16,CL",
    0x16: "NOT1 r/m8,CL",
    0x17: "NOT1 r/m16,CL",
    0x18: "TEST1 r/m8,imm3",
    0x19: "TEST1 r/m16,imm4",
    0x1A: "CLR1 r/m8,imm3",
    0x1B: "CLR1 r/m16,imm4",
    0x1C: "SET1 r/m8,imm3",
    0x1D: "SET1 r/m16,imm4",
    0x1E: "NOT1 r/m8,imm3",
    0x1F: "NOT1 r/m16,imm4",
    0x20: "ADD4S",
    0x22: "SUB4S",
    0x26: "CMP4S",
    0x28: "ROL4 r/m8",
    0x2A: "ROR4 r/m8",
    0x31: "INS r/m8,reg8",
    0x33: "EXT reg8,r/m8",
    0x39: "INS r/m8,imm4",
    0x3B: "EXT reg8,imm4",
}

CTS0 = ("SLDT", "STR", "LLDT", "LTR", "VERR", "VERW", "VERR", "VERW")
CTS1 = ("SGDT", "SIDT", "LGDT", "LIDT", "SMSW", "SMSW", "LMSW", "LMSW")

PROTECTED_FIELDS = (
    ("GDTR", 64, 6),
    ("MSW", 70, 2),
    ("IDTR", 72, 6),
    ("LDTR", 78, 2),
    ("LDTRC", 80, 6),
    ("TR", 86, 2),
    ("TRC", 88, 6),
)


class AnalysisError(RuntimeError):
    """A source, schema, or deterministic-output invariant failed."""


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def canonical_json(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True)
            + "\n").encode("utf-8")


def read_json(path: pathlib.Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise AnalysisError("cannot read {}: {}".format(path, error)) from error


def verify_document_manifest(root: pathlib.Path,
                             document_root: pathlib.Path | None = None) -> None:
    value = require_keys(
        read_json(root / DOCUMENT_PATH),
        {"copyright", "decision_status", "documents", "license",
         "repository_evidence", "schema"},
        str(DOCUMENT_PATH),
    )
    if value["schema"] != "vaeg-upd9002-rep0f-documents-m47-v1":
        raise AnalysisError("document manifest schema changed")
    if value["decision_status"] != "unresolved_pending_pc88va_hardware_probe":
        raise AnalysisError("document evidence must remain unresolved in M47")
    documents = value["documents"]
    if not isinstance(documents, list) or len(documents) != 4:
        raise AnalysisError("document manifest must contain exactly four identities")
    required = {
        "availability", "describes", "edition", "file_size", "finding",
        "local_filename", "relevant_sections", "sha256", "source",
        "source_url", "title", "unambiguously_resolves_rep0f",
    }
    acquired = 0
    upd9002_specific = 0
    for index, raw in enumerate(documents):
        document = require_keys(raw, required, "document[{}]".format(index))
        if document["unambiguously_resolves_rep0f"] is not False:
            raise AnalysisError("M47 documentary evidence cannot select a rule")
        if "uPD9002" in document["describes"]:
            upd9002_specific += 1
        if document["availability"] == "acquired":
            acquired += 1
            if (not re.fullmatch(r"[0-9a-f]{64}", document["sha256"] or "")
                    or not isinstance(document["file_size"], int)
                    or not document["relevant_sections"]):
                raise AnalysisError("acquired document identity is incomplete")
            if document_root is not None:
                path = document_root / document["local_filename"]
                if not path.is_file():
                    raise AnalysisError("missing acquired document {}".format(path))
                if path.stat().st_size != document["file_size"]:
                    raise AnalysisError("document size changed: {}".format(path))
                if sha256_file(path) != document["sha256"]:
                    raise AnalysisError("document digest changed: {}".format(path))
        elif document["availability"] == "bibliographic_record_only":
            if any((document["sha256"], document["file_size"],
                    document["local_filename"], document["relevant_sections"])):
                raise AnalysisError("unavailable document contains invented content identity")
        else:
            raise AnalysisError("unknown document availability")
    if acquired != 3 or upd9002_specific != 1:
        raise AnalysisError("document scope count changed")
    print("rep0f-analysis: documents acquired=3 upd9002-content=0 decision=unresolved")


def read_csv_rows(path: pathlib.Path, expected: tuple[str, ...]) -> list[dict[str, str]]:
    try:
        with path.open("r", encoding="utf-8", newline="") as stream:
            reader = csv.DictReader(stream)
            if tuple(reader.fieldnames or ()) != expected:
                raise AnalysisError(
                    "{}: unexpected columns {}".format(path, reader.fieldnames)
                )
            rows = list(reader)
    except OSError as error:
        raise AnalysisError("cannot read {}: {}".format(path, error)) from error
    return rows


def load_ssts_module(root: pathlib.Path) -> Any:
    path = root / "tools/qa/upd9002_ssts.py"
    specification = importlib.util.spec_from_file_location("upd9002_ssts", path)
    if specification is None or specification.loader is None:
        raise AnalysisError("cannot import {}".format(path))
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def load_support(root: pathlib.Path) -> dict[int, dict[str, str]]:
    rows = read_csv_rows(
        root / "tools/qa/golden/upd9002_support_map_m42.csv",
        ("mode", "opcode", "subopcode", "target", "classification", "basis"),
    )
    support: dict[int, dict[str, str]] = {}
    for row in rows:
        if row["mode"] != "v30op_0f":
            continue
        try:
            second = int(row["subopcode"], 16)
        except ValueError as error:
            raise AnalysisError("invalid 0F subopcode {}".format(row)) from error
        if second in support:
            raise AnalysisError("duplicate 0F support row {:02x}".format(second))
        support[second] = row
    if set(support) != set(range(256)):
        missing = sorted(set(range(256)) - set(support))
        extra = sorted(set(support) - set(range(256)))
        raise AnalysisError("0F support map is not complete: missing={} extra={}".format(
            missing, extra
        ))
    return support


def verify_active_roots(root: pathlib.Path) -> None:
    graph = read_csv_rows(
        root / "tools/qa/golden/upd9002_final_dispatch_graph.csv",
        ("table", "slot", "entry_kind", "target"),
    )
    actual = {
        (row["table"], row["slot"]): row["target"]
        for row in graph
        if row["table"] in {"v30op", "v30op_repe", "v30op_repne"}
        and row["slot"] == "0x0f"
    }
    expected = {
        ("v30op", "0x0f"): "v30_ope0x0f",
        ("v30op_repe", "0x0f"): "i286c_cts",
        ("v30op_repne", "0x0f"): "i286c_cts",
    }
    if actual != expected:
        raise AnalysisError("accepted REP+0F root edges changed: {}".format(actual))


def parse_table_symbols(source: str, name: str) -> tuple[str, ...]:
    match = re.search(
        r"static\s+const\s+I286OP_0F\s+" + re.escape(name)
        + r"\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.DOTALL,
    )
    if match is None:
        raise AnalysisError("cannot parse {}".format(name))
    body = re.sub(r"/\*.*?\*/|//[^\n]*", "", match.group("body"), flags=re.DOTALL)
    symbols = tuple(item.strip().lstrip("_").upper()
                    for item in body.split(",") if item.strip())
    if len(symbols) != 8 or any(not re.fullmatch(r"[A-Z0-9_]+", item)
                                for item in symbols):
        raise AnalysisError("{} has unexpected syntax: {}".format(name, symbols))
    return symbols


def verify_legacy_decoder(root: pathlib.Path) -> None:
    path = root / "i286c/i286c_0f.c"
    source = path.read_text(encoding="utf-8")
    if parse_table_symbols(source, "cts0_table") != CTS0:
        raise AnalysisError("cts0_table changed")
    if parse_table_symbols(source, "cts1_table") != CTS1:
        raise AnalysisError("cts1_table changed")
    required = {
        "if (op == 0)": 1,
        "if (!(I286_MSW & MSW_PE))": 1,
        "cts0_table[(op2 >> 3) & 7](op2);": 1,
        "else if (op == 1)": 1,
        "cts1_table[(op2 >> 3) & 7](op2);": 1,
        "else if (op == 5)": 1,
        "_loadall286();": 1,
        "INT_NUM(6, ip - 1);": 2,
    }
    for text, count in required.items():
        if source.count(text) != count:
            raise AnalysisError("legacy decoder evidence changed: {!r}".format(text))


def load_ssts_forms(root: pathlib.Path) -> dict[int, dict[str, Any]]:
    manifest = read_json(root / "tests/ssts/v20_dataset_manifest.json")
    full = read_json(root / "tests/ssts/baseline/v20_native_full.json")
    testsets = {item["form"]: item for item in manifest["opcode_testsets"]}
    profiles = {item["form"]: item for item in full["per_form"]}
    forms: dict[int, dict[str, Any]] = {}
    for second in range(256):
        form = "0F{:02X}".format(second)
        testset = testsets.get(form)
        profile = profiles.get(form)
        if (testset is None) != (profile is None):
            raise AnalysisError("partial M43 form evidence for {}".format(form))
        if testset is not None:
            forms[second] = {
                "form": form,
                "empty": testset["empty_queue_count"],
                "prefetched": testset["prefetched_count"],
                "classification_counts": profile["classification_counts"],
                "result_counts": profile["result_counts"],
            }
    if set(forms) != set(MANUAL_FORMS):
        raise AnalysisError(
            "M43 0F form set differs from the NEC Group3 form set: {}".format(
                sorted(forms)
            )
        )
    return forms


def operand_constraints(second: int, target: str) -> str:
    if second in {0x20, 0x22, 0x26}:
        return "no ModR/M; implicit IX/IY and CL block operands"
    if second in MANUAL_FORMS:
        suffix = "; immediate byte follows ModR/M" if second in range(0x18, 0x20) else ""
        return "ModR/M register or memory form{}".format(suffix)
    if target == "v30_reserved_0x0f":
        return "none consumed by current reserved handler"
    raise AnalysisError("unclassified V30 operand form {:02x} {}".format(second, target))


def probe_cases(prefix: str, second: int) -> str:
    keys: list[str] = []
    if second == 0x10:
        keys.append({"none": "r001", "f2": "r002", "f3": "r003"}[prefix])
    if second == 0x01:
        keys.append({"none": "r004", "f2": "r005", "f3": "r006"}[prefix])
        if prefix == "f2":
            keys.append("r008")
    if second == 0x00 and prefix == "f2":
        keys.append("r007")
    return ";".join(keys)


def build_matrix(root: pathlib.Path) -> bytes:
    support = load_support(root)
    verify_active_roots(root)
    verify_legacy_decoder(root)
    ssts_forms = load_ssts_forms(root)
    rows: list[dict[str, str]] = []
    for prefix in ("none", "f2", "f3"):
        prefix_byte = {"none": "", "f2": "f2", "f3": "f3"}[prefix]
        for second in range(256):
            target = support[second]["target"]
            ssts = ssts_forms.get(second)
            instruction = "{}0f{:02x}".format(prefix_byte, second)
            row = {column: "" for column in MATRIX_COLUMNS}
            row.update({
                "case_id": "{}-0f-{:02x}".format(prefix, second),
                "prefix": prefix,
                "second_byte": "0x{:02x}".format(second),
                "instruction_bytes": instruction,
                "manual_mnemonic": MANUAL_FORMS.get(second, "undefined Group3 code"),
                "initial_registers": "case-specific; probe defaults AX=0011 CX=0001",
                "initial_flags": "case-specific; probe default FLAGS=0002 before TF",
                "initial_memory": "case-specific; probe guard initialized to 5aa5",
                "architectural_prefix_status": (
                    "not applicable" if prefix == "none"
                    else "unresolved: candidate outcomes A, B, C, or D"
                ),
                "confidence": "high for current VAEG path; unresolved for uPD9002/V52",
                "ssts_metadata_form": "" if ssts is None else ssts["form"],
                "ssts_empty_prefetch_records": (
                    "0" if prefix != "none" or ssts is None else str(ssts["empty"])
                ),
                "ssts_populated_prefetch_records": (
                    "0" if prefix != "none" or ssts is None else str(ssts["prefetched"])
                ),
                "hardware_probe_cases": probe_cases(prefix, second),
            })
            if prefix == "none":
                row.update({
                    "modrm_constraints": operand_constraints(second, target),
                    "current_dispatch_path": "v30op[0x0f] -> v30_ope0x0f -> v30ope0x0f_table",
                    "current_handler_or_result": target,
                    "expected_ip_advance": (
                        "2" if target == "v30_reserved_0x0f"
                        else "handler- and operand-dependent"
                    ),
                    "expected_registers": (
                        "unchanged" if target == "v30_reserved_0x0f"
                        else "defined by the selected V30 Group3 handler and operands"
                    ),
                    "expected_flags": (
                        "unchanged" if target == "v30_reserved_0x0f"
                        else "defined by the selected V30 Group3 handler"
                    ),
                    "memory_and_io_effects": (
                        "none" if target == "v30_reserved_0x0f"
                        else "handler- and operand-dependent; no direct I/O handler"
                    ),
                    "interrupt_or_exception": "none in current reserved path; handler-dependent otherwise",
                    "evidence_source": "M42 graph/support map; i286c/v30patch.c; NEC U11301EJ5V0UMJ1 Appendix C",
                })
            else:
                row["current_dispatch_path"] = (
                    "v30op[0x{}] -> v30_{} -> v30op_{}[0x0f] -> i286c_cts"
                    .format(prefix, "repne" if prefix == "f2" else "repe",
                            "repne" if prefix == "f2" else "repe")
                )
                row["evidence_source"] = (
                    "M42 graph/provenance; i286c/v30patch.c; i286c/i286c_0f.c; "
                    "NEC manuals do not define uPD9002 REP+0F"
                )
                if second == 0x00:
                    row.update({
                        "modrm_constraints": "MSW_PE=0: ModR/M not consumed; MSW_PE=1: /0..7 -> " + "/".join(CTS0),
                        "current_handler_or_result": "INT 6 when MSW_PE=0; cts0_table when MSW_PE=1",
                        "expected_ip_advance": "3 before INT 6 when MSW_PE=0; 4 plus EA bytes when MSW_PE=1",
                        "expected_registers": "conditional on cts0_table selector/system handler",
                        "expected_flags": "legacy-handler-dependent",
                        "memory_and_io_effects": "selector reads/writes possible only when MSW_PE=1",
                        "interrupt_or_exception": "INT 6 when MSW_PE=0; selector exceptions possible when PE=1",
                    })
                elif second == 0x01:
                    row.update({
                        "modrm_constraints": "/0..7 -> " + "/".join(CTS1),
                        "current_handler_or_result": "cts1_table",
                        "expected_ip_advance": "4 plus EA displacement bytes",
                        "expected_registers": "SMSW/LMSW may read or change AX/MSW; other entries table-dependent",
                        "expected_flags": "unchanged by LMSW; otherwise legacy-handler-dependent",
                        "memory_and_io_effects": "descriptor-table memory read/write possible; no direct I/O",
                        "interrupt_or_exception": "INT 6 for invalid register-only descriptor-table forms",
                    })
                elif second == 0x05:
                    row.update({
                        "modrm_constraints": "no ModR/M; fixed LOADALL286 image at physical 0804h..",
                        "current_handler_or_result": "_loadall286",
                        "expected_ip_advance": "loaded from physical 081ah, not sequential",
                        "expected_registers": "legacy LOADALL286 image replaces protected and general state",
                        "expected_flags": "loaded from physical 0818h",
                        "memory_and_io_effects": "reads legacy LOADALL286 image; no direct I/O",
                        "interrupt_or_exception": "none explicitly; unsafe/unrecoverable probe omitted",
                    })
                else:
                    row.update({
                        "modrm_constraints": "no following operand consumed before current INT 6",
                        "current_handler_or_result": "INT 6 from i286c_cts",
                        "expected_ip_advance": "3 before INT 6 restart address handling",
                        "expected_registers": "unchanged before exception entry",
                        "expected_flags": "exception-entry effects only",
                        "memory_and_io_effects": "interrupt stack/vector effects only",
                        "interrupt_or_exception": "INT 6",
                    })
            rows.append(row)
    if len(rows) != 768:
        raise AnalysisError("instruction matrix does not contain 768 rows")
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=MATRIX_COLUMNS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue().encode("utf-8")


def parse_fixture_line(line: str, line_number: int) -> tuple[str, bytes]:
    fields = line.rstrip("\n").split(",")
    if not fields or not fields[0]:
        raise AnalysisError("fixture line {} has no scenario".format(line_number))
    values: dict[str, str] = {}
    for field in fields[1:]:
        if "=" not in field:
            raise AnalysisError("fixture line {} malformed field".format(line_number))
        key, value = field.split("=", 1)
        if key in values:
            raise AnalysisError("fixture line {} duplicate {}".format(line_number, key))
        values[key] = value
    if values.get("cpu286_size") != "112" or "cpu286" not in values:
        raise AnalysisError("fixture line {} lacks 112-byte CPU286".format(line_number))
    try:
        payload = bytes.fromhex(values["cpu286"])
    except ValueError as error:
        raise AnalysisError("fixture line {} invalid CPU286 hex".format(line_number)) from error
    if len(payload) != 112:
        raise AnalysisError("fixture line {} CPU286 length changed".format(line_number))
    return fields[0], payload


def protected_values(payload: bytes) -> dict[str, str]:
    return {
        name: payload[offset:offset + size].hex()
        for name, offset, size in PROTECTED_FIELDS
    }


def build_state_inventory(root: pathlib.Path) -> bytes:
    fixture_path = root / "tests/upd9002/state_fixtures_m42.txt"
    scenarios = []
    for line_number, line in enumerate(
            fixture_path.read_text(encoding="utf-8").splitlines(True), 1):
        scenario, payload = parse_fixture_line(line, line_number)
        values = protected_values(payload)
        scenarios.append({
            "scenario": scenario,
            "source": "committed M42 fixture and generated M42-M46 scenario",
            "cpu286_sha256": sha256_bytes(payload),
            "protected_fields": values,
            "has_protected_residue": any(int(value, 16) != 0 for value in values.values()),
        })
    if [item["scenario"] for item in scenarios] != [
            "reset", "executed-3", "cpu-shut-request"]:
        raise AnalysisError("accepted fixture scenario set changed")
    boundary = (root / "tests/upd9002/state_boundary.c").read_text(encoding="utf-8")
    required_assignments = {
        "GDTR.limit": "0xabcd",
        "GDTR.base": "0x1357",
        "GDTR.base24": "0x24",
        "GDTR.reserved": "0x68",
        "TRC.base24": "0x42",
        "TRC.reserved": "0x86",
    }
    for field, value in required_assignments.items():
        pattern = r"state->" + re.escape(field) + r"\s*=\s*" + re.escape(value) + r";"
        if len(re.findall(pattern, boundary)) != 1:
            raise AnalysisError("M44 valid-import residue evidence changed: {}".format(field))
    value = {
        "schema": "vaeg-upd9002-protected-state-inventory-m47-v1",
        "cpu286_payload_size": 112,
        "fields": [
            {"name": name, "offset": offset, "size": size}
            for name, offset, size in PROTECTED_FIELDS
        ],
        "accepted_scenarios": scenarios,
        "accepted_test_generated_import": {
            "source": "tests/upd9002/state_boundary.c:make_noncanonical",
            "payload_committed": False,
            "protected_residue": required_assignments,
            "accepted_by_current_import": True,
            "opaque_roundtrip_verified": True,
        },
        "user_visible_saved_state_corpus": {
            "provided": False,
            "states_scanned": 0,
            "private_identities_recorded": False,
        },
        "summary": {
            "committed_or_matrix_scenarios": len(scenarios),
            "scenarios_with_protected_residue": sum(
                1 for item in scenarios if item["has_protected_residue"]
            ),
            "test_generated_valid_imports_with_residue": 1,
        },
    }
    return canonical_json(value)


def artifact_sha(root: pathlib.Path, relative: str) -> str:
    return sha256_file(root / relative)


def build_transition(root: pathlib.Path, corpus: dict[str, Any]) -> bytes:
    graph_rows = read_csv_rows(
        root / "tools/qa/golden/upd9002_final_dispatch_graph.csv",
        ("table", "slot", "entry_kind", "target"),
    )
    provenance_rows = read_csv_rows(
        root / "tools/qa/golden/upd9002_dispatch_provenance_m42.csv",
        ("root", "slot", "base", "base_target", "operation", "final_target"),
    )
    support_rows = read_csv_rows(
        root / "tools/qa/golden/upd9002_support_map_m42.csv",
        ("mode", "opcode", "subopcode", "target", "classification", "basis"),
    )
    roots = {"v30op_repe", "v30op_repne"}
    graph = [row for row in graph_rows if row["table"] in roots and row["slot"] == "0x0f"]
    provenance = [row for row in provenance_rows if row["root"] in roots and row["slot"] == "0x0f"]
    support = [row for row in support_rows if row["mode"] in roots and row["opcode"] == "0x0f"]
    if len(graph) != 2 or len(provenance) != 2 or len(support) != 2:
        raise AnalysisError("REP+0F transition row inventory is incomplete")
    if any(row["target"] != "i286c_cts" for row in graph + support):
        raise AnalysisError("current REP+0F targets changed")
    affected = corpus["candidate_outcome_affected_record_hashes"]
    prefix_matrix_cases = [
        "{}-0f-{:02x}".format(prefix, second)
        for prefix in ("f2", "f3")
        for second in range(256)
    ]
    if any(affected[outcome] for outcome in affected):
        raise AnalysisError("M43 REP+0F affected-record set is no longer empty")
    value = {
        "schema": "vaeg-upd9002-rep0f-transition-manifest-m47-v1",
        "status": "prospective-not-authorized",
        "g47_decision": "unresolved pending PC-88VA hardware results",
        "accepted_artifact_sha256": {
            "final_dispatch_graph": artifact_sha(root, "tools/qa/golden/upd9002_final_dispatch_graph.csv"),
            "construction_provenance": artifact_sha(root, "tools/qa/golden/upd9002_dispatch_provenance_m42.csv"),
            "support_map": artifact_sha(root, "tools/qa/golden/upd9002_support_map_m42.csv"),
            "known_gaps": artifact_sha(root, "tests/ssts/baseline/upd9002_v20_known_gaps.json"),
            "ci_baseline": artifact_sha(root, "tests/ssts/baseline/v20_native_ci.json"),
            "full_baseline": artifact_sha(root, "tests/ssts/baseline/v20_native_full.json"),
            "state_fixtures": artifact_sha(root, "tests/upd9002/state_fixtures_m42.txt"),
        },
        "current_rows": {
            "final_dispatch": graph,
            "provenance": provenance,
            "support_map": support,
        },
        "complete_prospective_scope": {
            "instruction_matrix_case_ids": prefix_matrix_cases,
            "instruction_matrix_case_count": len(prefix_matrix_cases),
            "m42_final_dispatch_rows": len(graph),
            "m42_provenance_rows": len(provenance),
            "m42_support_map_rows": len(support),
            "m43_applicable_record_hashes": [],
            "m43_known_gap_record_hashes": [],
            "m43_semantic_failure_record_hashes": [],
            "m43_failure_signatures": [],
            "reason_m43_is_empty": (
                "the pinned corpus has zero decoded F2/F3+0F records; "
                "105 adjacent byte sequences are operands, immediates, or displacements"
            ),
        },
        "candidate_A_prefix_ignored": {
            "final_dispatch_replacements": [
                {**row, "target": "v30_ope0x0f"} for row in graph
            ],
            "provenance_replacements": [
                {
                    **row,
                    "operation": "patch",
                    "final_target": "v30_ope0x0f",
                    "transition_note": "exact constructor representation requires G47 approval",
                }
                for row in provenance
            ],
            "support_map_replacements": [
                {
                    **row,
                    "target": "v30_ope0x0f",
                    "basis": "REP-prefix-ignored-second-byte-resolved",
                    "transition_note": "classification must resolve the following 0F second byte",
                }
                for row in support
            ],
            "instruction_matrix_case_ids": prefix_matrix_cases,
            "m43_record_hashes": affected["A_prefix_ignored"],
            "m43_failure_signatures": [],
        },
        "candidate_B_target_specific": {
            "exact_rows": None,
            "reason": "uPD9002-specific operation has not been established",
            "rows_requiring_resolution": prefix_matrix_cases,
            "m43_record_hashes": affected["B_target_specific"],
            "m43_failure_signatures": [],
        },
        "candidate_C_reserved_or_undefined": {
            "exact_rows": None,
            "reason": "exception/no-op/restart semantics have not been established",
            "rows_requiring_resolution": prefix_matrix_cases,
            "m43_record_hashes": affected["C_reserved_or_undefined"],
            "m43_failure_signatures": [],
        },
        "candidate_D_second_byte_dependent": {
            "exact_rows": None,
            "reason": "per-second-byte behavior awaits hardware evidence",
            "rows_requiring_resolution": prefix_matrix_cases,
            "m43_record_hashes": affected["D_second_byte_dependent"],
            "m43_failure_signatures": [],
        },
        "state_transition": {
            "committed_reset_executed3_cpu_shut_fixtures": "must remain byte-identical",
            "cpu_shut_flags_0000": "must remain byte-identical",
            "states_with_protected_residue": "policy unresolved; no rejection or migration authorized",
            "g41_m44_matrix": "all three accepted scenarios have zero protected residue",
        },
        "m47_changes_accepted_baselines": False,
    }
    return canonical_json(value)


def require_keys(value: Any, keys: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != keys:
        raise AnalysisError("{}: unexpected keys".format(where))
    return value


def build_corpus(root: pathlib.Path, dataset_root: pathlib.Path) -> bytes:
    ssts = load_ssts_module(root)
    manifest_path = root / "tests/ssts/v20_dataset_manifest.json"
    manifest = ssts.load_manifest(manifest_path)
    ssts.verify_fast(dataset_root, manifest)
    metadata = read_json(dataset_root / "v1_native/metadata.json")
    prefix_entries = {}
    for form in ("F2", "F3"):
        entry = metadata["opcodes"].get(form)
        if not isinstance(entry, dict) or entry.get("status") != "prefix":
            raise AnalysisError("{} metadata is not a prefix".format(form))
        prefix_entries[form] = entry
    manifest_paths = {item["path"] for item in manifest["files"]}
    prefix_shards = sorted(path for path in manifest_paths
                            if pathlib.PurePosixPath(path).name in {"F2.json.gz", "F3.json.gz"})
    adjacent_hits = []
    decoded_hits = []
    scanned = 0
    queue_counts: Counter[str] = Counter()
    form_counts: Counter[str] = Counter()
    for path in ssts.corpus_files(dataset_root):
        relative = path.relative_to(dataset_root).as_posix()
        form = path.name[:-8]
        try:
            with gzip.open(path, "rt", encoding="utf-8") as stream:
                records = json.load(stream)
        except (OSError, json.JSONDecodeError) as error:
            raise AnalysisError("{}: {}".format(relative, error)) from error
        if not isinstance(records, list):
            raise AnalysisError("{}: shard root is not a list".format(relative))
        for index, raw in enumerate(records):
            record = ssts.validate_record(raw, "{}:{}".format(relative, index))
            scanned += 1
            instruction = record["bytes"]
            positions = [
                position for position in range(len(instruction) - 1)
                if instruction[position] in {0xF2, 0xF3}
                and instruction[position + 1] == 0x0F
            ]
            if not positions:
                continue
            queue = "populated" if record["initial"]["queue"] else "empty"
            digest = ssts.sha256_bytes(ssts.canonical_bytes(record))
            item = {
                "metadata_form": form,
                "record_hash": digest,
                "upstream_test_hash": record["hash"],
                "instruction_bytes": "".join("{:02x}".format(byte) for byte in instruction),
                "rep0f_positions": positions,
                "prefetch": queue,
                "initial": record["initial"],
                "expected_final": record["final"],
            }
            adjacent_hits.append(item)
            position = 0
            while (position < len(instruction)
                   and instruction[position] in
                   (set(ssts.SEGMENT_PREFIXES) | set(ssts.REPEAT_PREFIXES)
                    | set(ssts.IGNORED_PREFIXES))):
                if (instruction[position] in {0xF2, 0xF3}
                        and position + 1 < len(instruction)
                        and instruction[position + 1] == 0x0F):
                    decoded_hits.append(item)
                    queue_counts[queue] += 1
                    form_counts[form] += 1
                    break
                position += 1
    if scanned != manifest["counts"]["records"]:
        raise AnalysisError("scanned {} records, expected {}".format(
            scanned, manifest["counts"]["records"]
        ))
    adjacent_hits.sort(key=lambda item: item["record_hash"])
    decoded_hits.sort(key=lambda item: item["record_hash"])
    hit_hashes = {item["record_hash"] for item in decoded_hits}
    baseline_intersections = {}
    for profile in ("ci", "full"):
        summary = root / "tests/ssts/baseline/v20_native_{}.json".format(profile)
        _, failures = ssts.load_failures(summary)
        baseline_intersections[profile] = sorted(hit_hashes & set(failures))
    candidate_hashes = {
        "A_prefix_ignored": sorted(hit_hashes),
        "B_target_specific": sorted(hit_hashes),
        "C_reserved_or_undefined": sorted(hit_hashes),
        "D_second_byte_dependent": sorted(hit_hashes),
    }
    ci = read_json(root / "tests/ssts/baseline/v20_native_ci.json")
    full = read_json(root / "tests/ssts/baseline/v20_native_full.json")
    known = read_json(root / "tests/ssts/baseline/upd9002_v20_known_gaps.json")
    value = {
        "schema": "vaeg-upd9002-rep0f-corpus-m47-v1",
        "dataset_id": manifest["dataset_id"],
        "dataset_records_scanned": scanned,
        "metadata_prefix_entries": prefix_entries,
        "prefix_opcode_shards": prefix_shards,
        "adjacent_byte_sequence_record_count": len(adjacent_hits),
        "adjacent_byte_sequence_records": adjacent_hits,
        "decoded_rep0f_record_count": len(decoded_hits),
        "empty_prefetch_rep0f_records": queue_counts["empty"],
        "populated_prefetch_rep0f_records": queue_counts["populated"],
        "metadata_form_counts": dict(sorted(form_counts.items())),
        "decoded_rep0f_records": decoded_hits,
        "comparison_with_current_vaeg": (
            "no applicable records; the pinned V20 corpus supplies no F2/F3+0F oracle"
            if not decoded_hits else "see per-record expected_final values"
        ),
        "candidate_outcome_affected_record_hashes": candidate_hashes,
        "semantic_failure_intersections": baseline_intersections,
        "accepted_baseline_identity": {
            "known_gap_selectors": known["rule_count"],
            "known_gap_record_hashes": known["resolved_record_count"],
            "ci_executed": ci["executed_records"],
            "ci_failure_signature_index_sha256": ci["failure_signature_index_sha256"],
            "full_executed": full["executed_records"],
            "full_failure_signature_index_sha256": full["failure_signature_index_sha256"],
        },
        "architectural_scope": (
            "V20 evidence only; even a nonempty result would not by itself prove uPD9002 behavior"
        ),
    }
    return canonical_json(value)


def compare_or_write(path: pathlib.Path, data: bytes, write: bool) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        print("rep0f-analysis: wrote {} sha256={}".format(path, sha256_bytes(data)))
        return
    try:
        actual = path.read_bytes()
    except OSError as error:
        raise AnalysisError("cannot read golden {}: {}".format(path, error)) from error
    if actual != data:
        raise AnalysisError("{} is not byte-identical to regeneration".format(path))
    print("rep0f-analysis: checked {} sha256={}".format(path, sha256_bytes(data)))


def static_artifacts(root: pathlib.Path, write: bool) -> None:
    verify_document_manifest(root)
    corpus = read_json(root / CORPUS_PATH)
    expected_corpus_keys = {
        "accepted_baseline_identity",
        "adjacent_byte_sequence_record_count",
        "adjacent_byte_sequence_records",
        "architectural_scope",
        "candidate_outcome_affected_record_hashes",
        "comparison_with_current_vaeg",
        "dataset_id",
        "dataset_records_scanned",
        "decoded_rep0f_record_count",
        "decoded_rep0f_records",
        "empty_prefetch_rep0f_records",
        "metadata_form_counts",
        "metadata_prefix_entries",
        "populated_prefetch_rep0f_records",
        "prefix_opcode_shards",
        "schema",
        "semantic_failure_intersections",
    }
    require_keys(corpus, expected_corpus_keys, str(CORPUS_PATH))
    compare_or_write(root / MATRIX_PATH, build_matrix(root), write)
    compare_or_write(root / STATE_PATH, build_state_inventory(root), write)
    compare_or_write(root / TRANSITION_PATH, build_transition(root, corpus), write)


def selftest(root: pathlib.Path) -> None:
    matrix = build_matrix(root)
    lines = matrix.decode("utf-8").splitlines()
    if len(lines) != 769:
        raise AnalysisError("selftest matrix line count changed")
    if not any(line.startswith("f2-0f-01,") for line in lines):
        raise AnalysisError("selftest lacks F2 0F 01")
    fixture = (
        "synthetic,cpu286_size=112,cpu286=" + ("00" * 112)
    )
    scenario, payload = parse_fixture_line(fixture, 1)
    if scenario != "synthetic":
        raise AnalysisError("selftest fixture scenario changed")
    if any(int(value, 16) for value in protected_values(payload).values()):
        raise AnalysisError("selftest zero protected state parsed nonzero")
    print("rep0f-analysis: selftest matrix=768 fixture-parser=pass fail-closed=pass")


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command",
        choices=("generate", "generate-static", "check", "check-static",
                 "selftest", "verify-documents"),
    )
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--dataset-root", type=pathlib.Path)
    parser.add_argument("--document-root", type=pathlib.Path)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    root = arguments.root.resolve()
    try:
        if arguments.command in {"generate", "check"}:
            if arguments.dataset_root is None:
                raise AnalysisError("--dataset-root is required")
            corpus_data = build_corpus(root, arguments.dataset_root.resolve())
            compare_or_write(root / CORPUS_PATH, corpus_data,
                             arguments.command == "generate")
            static_artifacts(root, arguments.command == "generate")
        elif arguments.command == "check-static":
            static_artifacts(root, False)
        elif arguments.command == "generate-static":
            static_artifacts(root, True)
        elif arguments.command == "selftest":
            selftest(root)
        elif arguments.command == "verify-documents":
            if arguments.document_root is None:
                raise AnalysisError("--document-root is required")
            verify_document_manifest(root, arguments.document_root.resolve())
        else:
            raise AssertionError(arguments.command)
    except (AnalysisError, OSError, KeyError, TypeError, ValueError) as error:
        print("rep0f-analysis: FAIL: {}".format(error), file=sys.stderr)
        return 1
    print("rep0f-analysis: PASS command={}".format(arguments.command))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

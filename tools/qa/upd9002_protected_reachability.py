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
"""Generate the fail-closed M49 NP2 80286 reachability inventory."""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import io
import pathlib
import re
import sys
from collections import Counter
from typing import Dict, Iterable, List, Mapping, MutableMapping, Sequence, Tuple


sys.dont_write_bytecode = True


class ReachabilityError(RuntimeError):
    """A source or inventory invariant could not be proved."""


INVENTORY_PATH = pathlib.Path(
    "tools/qa/golden/upd9002_286_reachability_m49.csv"
)

COLUMNS = (
    "candidate_id",
    "symbol_or_field",
    "kind",
    "defining_file",
    "disposition",
    "final_dispatch_reachable",
    "secondary_dispatch_reachable",
    "directly_called",
    "address_taken",
    "macro_referenced",
    "active_state_read",
    "active_state_write",
    "active_state_address_taken",
    "serialized_offset",
    "serialized_size",
    "import_dependency",
    "export_dependency",
    "reset_dependency",
    "cpu_shut_dependency",
    "diagnostic_stop_dependency",
    "io_dependency",
    "test_dependency",
    "frozen_reference_dependency",
    "proposed_deletion_group",
    "evidence",
)

DISPOSITIONS = {
    "active_native_required",
    "fail_closed_diagnostic_required",
    "cpu_shut_compatibility_required",
    "serialized_compatibility_only",
    "test_evidence_only",
    "frozen_reference_only",
    "unreachable_286_candidate",
    "shared_helper",
    "unresolved",
}

PROPOSED_GROUPS = {
    "M50-PM-ARPL",
    "M50-PM-CTS-SYSTEM",
    "M50-PM-MOV-SEG-EA",
}

CTS0 = ("_sldt", "_str", "_lldt", "_ltr", "_verr", "_verw",
        "_verr", "_verw")
CTS1 = ("_sgdt", "_sidt", "_lgdt", "_lidt", "_smsw", "_smsw",
        "_lmsw", "_lmsw")
SYSTEM_HANDLERS = tuple(dict.fromkeys(CTS0 + CTS1)) + ("_loadall286",)

SEGSELECT_CALLERS = (
    "_pop_es",
    "_pop_ss",
    "_pop_ds",
    "_mov_seg_ea",
    "_call_far",
    "_les_r16_ea",
    "_lds_r16_ea",
    "_ret_far_data16",
    "_ret_far",
    "_jmp_far",
    "_call_far_ea16",
    "_jmp_far_ea16",
)

STATE_FIELDS = (
    ("GDTR", 64, 6),
    ("MSW", 70, 2),
    ("IDTR", 72, 6),
    ("LDTR", 78, 2),
    ("LDTRC", 80, 6),
    ("TR", 86, 2),
    ("TRC", 88, 6),
)

PROTECTED_TOKEN_COUNTS = {
    "i286c_cts": 5,
    "cts0_table": 2,
    "cts1_table": 2,
    "_sldt": 2,
    "_str": 2,
    "_lldt": 2,
    "_ltr": 2,
    "_verr": 3,
    "_verw": 3,
    "_sgdt": 2,
    "_sidt": 2,
    "_lgdt": 2,
    "_lidt": 2,
    "_smsw": 3,
    "_lmsw": 3,
    "_loadall286": 2,
    "_arpl": 4,
    "_mov_seg_ea": 4,
    "i286c_selector": 5,
}

M48_ARTIFACTS = {
    "graph": pathlib.Path(
        "tools/qa/golden/upd9002_final_dispatch_graph_m48.csv"),
    "provenance": pathlib.Path(
        "tools/qa/golden/upd9002_dispatch_provenance_m48.csv"),
    "support": pathlib.Path(
        "tools/qa/golden/upd9002_support_map_m48.csv"),
}


def read_text(root: pathlib.Path, relative: str) -> str:
    try:
        return (root / relative).read_text(encoding="utf-8")
    except OSError as error:
        raise ReachabilityError(
            "cannot read required source {}: {}".format(relative, error)
        ) from error


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def extract_braced(text: str, start: int) -> str:
    depth = 0
    for position in range(start, len(text)):
        character = text[position]
        if character == "{":
            depth += 1
        elif character == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:position]
        if depth < 0:
            break
    raise ReachabilityError("unclosed source brace at offset {}".format(start))


def extract_named_array(text: str, name: str) -> Tuple[str, ...]:
    source = strip_comments(text)
    match = re.search(r"\b{}\s*\[\s*\]\s*=\s*\{{".format(
        re.escape(name)), source)
    if match is None:
        raise ReachabilityError("array {} is missing".format(name))
    start = source.find("{", match.start())
    body = extract_braced(source, start)
    fields = [field.strip() for field in body.split(",")]
    if fields and not fields[-1]:
        fields.pop()
    entries = tuple(fields)
    if not entries or any(not field for field in entries):
        raise ReachabilityError("array {} has an empty entry".format(name))
    return entries


def extract_function_bodies(
        text: str, allowed_duplicates: Tuple[str, ...] = ()) -> Dict[str, str]:
    source = strip_comments(text)
    pattern = re.compile(
        r"(?:^|\n)\s*(?:I286FN|I286_F6|I286_0F|I286EXT|"
        r"(?:static\s+)?(?:void|UINT32|BOOL|int))\s+"
        r"([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*\{",
        re.M,
    )
    bodies: Dict[str, str] = {}
    for match in pattern.finditer(source):
        name = match.group(1)
        start = source.find("{", match.start())
        body = extract_braced(source, start)
        if name in bodies:
            if name not in allowed_duplicates:
                raise ReachabilityError("duplicate function definition: " + name)
            bodies[name] += "\n" + body
        else:
            bodies[name] = body
    return bodies


def load_dispatch_module(root: pathlib.Path):
    path = root / "tools/qa/upd9002_dispatch.py"
    spec = importlib.util.spec_from_file_location(
        "upd9002_dispatch_m49", path)
    if spec is None or spec.loader is None:
        raise ReachabilityError("cannot load upd9002_dispatch.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def verify_dispatch(root: pathlib.Path) -> None:
    module = load_dispatch_module(root)
    graph, provenance, _harness, support = module.generate(root)
    generated = {
        "graph": graph,
        "provenance": provenance,
        "support": support,
    }
    for name, relative in M48_ARTIFACTS.items():
        expected = read_text(root, str(relative))
        if generated[name] != expected:
            raise ReachabilityError(
                "post-M48 {} differs from source regeneration".format(name))

    graph_rows = list(csv.DictReader(io.StringIO(graph)))
    protected = set(SYSTEM_HANDLERS) | {
        "i286c_cts", "cts0_table", "cts1_table"
    }
    leaked = [row for row in graph_rows if row["target"] in protected]
    if leaked:
        raise ReachabilityError(
            "protected target remains in post-M48 final graph: {}".format(
                leaked))
    expected_edges = {
        ("v30op_repne", "0x0f", "v30_repne_0f_diagnostic_stop"),
        ("v30op_repe", "0x0f", "v30_repe_0f_diagnostic_stop"),
        ("v30op", "0x0f", "v30_ope0x0f"),
    }
    actual_edges = {
        (row["table"], row["slot"], row["target"])
        for row in graph_rows
        if (row["table"], row["slot"]) in {
            ("v30op_repne", "0x0f"),
            ("v30op_repe", "0x0f"),
            ("v30op", "0x0f"),
        }
    }
    if actual_edges != expected_edges:
        raise ReachabilityError(
            "REP/native 0F final edges drifted: {}".format(
                sorted(actual_edges)))


def verify_protected_source(root: pathlib.Path) -> None:
    source_0f = read_text(root, "i286c/i286c_0f.c")
    functions = extract_function_bodies(source_0f)
    expected_functions = set(SYSTEM_HANDLERS) | {"i286c_cts"}
    actual_functions = set(functions)
    if actual_functions != expected_functions:
        raise ReachabilityError(
            "protected handler set changed: expected={} actual={}".format(
                sorted(expected_functions), sorted(actual_functions)))
    if extract_named_array(source_0f, "cts0_table") != CTS0:
        raise ReachabilityError("cts0_table changed")
    if extract_named_array(source_0f, "cts1_table") != CTS1:
        raise ReachabilityError("cts1_table changed")
    dispatcher = functions["i286c_cts"]
    required = (
        "cts0_table[(op2 >> 3) & 7](op2)",
        "cts1_table[(op2 >> 3) & 7](op2)",
        "_loadall286()",
        "if (!(I286_MSW & MSW_PE))",
    )
    missing = [value for value in required if value not in dispatcher]
    if missing:
        raise ReachabilityError(
            "i286c_cts dependency changed: " + ", ".join(missing))

    sources = {
        "CMakeLists.txt": read_text(root, "CMakeLists.txt"),
        "i286c/cpucore.h": read_text(root, "i286c/cpucore.h"),
        "i286c/i286c.h": read_text(root, "i286c/i286c.h"),
        "i286c/i286c.mcr": read_text(root, "i286c/i286c.mcr"),
        "i286c/i286c_0f.c": source_0f,
        "i286c/i286c_ea.c": read_text(root, "i286c/i286c_ea.c"),
        "i286c/i286c_fe.c": read_text(root, "i286c/i286c_fe.c"),
        "i286c/i286c_mn.c": read_text(root, "i286c/i286c_mn.c"),
        "i286c/v30patch.c": read_text(root, "i286c/v30patch.c"),
        "i286c/i286c.c": read_text(root, "i286c/i286c.c"),
        "i286c/upd9002_state.c": read_text(root, "i286c/upd9002_state.c"),
        "io/cpuio.c": read_text(root, "io/cpuio.c"),
        "machine/pccore.c": read_text(root, "machine/pccore.c"),
        "machine/statsave.c": read_text(root, "machine/statsave.c"),
        "machine/statsave.tbl": read_text(root, "machine/statsave.tbl"),
    }
    joined = "\n".join(strip_comments(text) for text in sources.values())
    for token, expected in PROTECTED_TOKEN_COUNTS.items():
        actual = len(re.findall(r"\b{}\b".format(re.escape(token)), joined))
        if actual != expected:
            raise ReachabilityError(
                "{} production reference count changed: expected={} actual={}".
                format(token, expected, actual))

    cmake = sources["CMakeLists.txt"]
    if len(re.findall(r"\bi286c/i286c_0f\.c\b", cmake)) != 1:
        raise ReachabilityError("i286c_0f.c CMake ownership changed")

    mn_arrays = {
        name: extract_named_array(sources["i286c/i286c_mn.c"], name)
        for name in ("i286op", "i286op_repe", "i286op_repne")
    }
    for name, entries in mn_arrays.items():
        for slot, target in ((0x0F, "i286c_cts"), (0x63, "_arpl"),
                             (0x8E, "_mov_seg_ea")):
            if len(entries) != 256 or entries[slot] != target:
                raise ReachabilityError(
                    "{}[0x{:02x}] no longer targets {}".format(
                        name, slot, target))


def verify_segselect(root: pathlib.Path) -> None:
    mcr = strip_comments(read_text(root, "i286c/i286c.mcr"))
    expected_macro = (
        "#define\tSEGSELECT(c)\t"
        "((I286_MSW & MSW_PE)?i286c_selector(c):((c) << 4))"
    )
    if mcr.count(expected_macro) != 1:
        raise ReachabilityError("SEGSELECT definition changed")

    bodies: Dict[str, str] = {}
    for relative in ("i286c/i286c_mn.c", "i286c/i286c_fe.c"):
        allowed = ("_pusha", "_popa") if relative.endswith("i286c_mn.c") else ()
        for name, body in extract_function_bodies(
                read_text(root, relative), allowed).items():
            if name in bodies:
                raise ReachabilityError("duplicate active function: " + name)
            bodies[name] = body
    actual = sorted(
        name for name, body in bodies.items() if "SEGSELECT(" in body)
    if tuple(actual) != tuple(sorted(SEGSELECT_CALLERS)):
        raise ReachabilityError(
            "SEGSELECT caller set changed: expected={} actual={}".format(
                sorted(SEGSELECT_CALLERS), actual))
    for name in SEGSELECT_CALLERS:
        if bodies[name].count("SEGSELECT(") != 1:
            raise ReachabilityError(
                "SEGSELECT multiplicity changed in {}".format(name))


def verify_state(root: pathlib.Path) -> None:
    header = strip_comments(read_text(root, "i286c/cpucore.h"))
    state_source = strip_comments(read_text(root, "i286c/upd9002_state.c"))
    io_source = strip_comments(read_text(root, "io/cpuio.c"))
    core_source = strip_comments(read_text(root, "i286c/i286c.c"))
    abi = read_text(root, "tests/upd9002/abi_g41.txt")

    for field, offset, _size in STATE_FIELDS:
        declaration = {
            "MSW": r"UINT16\s+MSW\s*;",
            "LDTR": r"UINT16\s+LDTR\s*;",
            "TR": r"UINT16\s+TR\s*;",
        }.get(field, r"I286DTR\s+{}\s*;".format(field))
        if len(re.findall(declaration, header)) != 2:
            raise ReachabilityError(
                "serialized/runtime declaration drifted for " + field)
        assertion = "offsetof(Cpu286StateCompat, {}) == {}".format(
            field, offset)
        if state_source.count(assertion) != 1:
            raise ReachabilityError("offset assertion changed for " + field)
        abi_line = "cpu286.offset.{}={}".format(field, offset)
        if abi.count(abi_line) != 1:
            raise ReachabilityError("ABI offset changed for " + field)

    required_state_patterns = (
        "memcpy(state, runtime, padding)",
        "memcpy((UINT8 *)state + tail, (const UINT8 *)runtime + tail",
        "memcpy(runtime, state, padding)",
        "memcpy((UINT8 *)runtime + tail, (const UINT8 *)state + tail",
        "if (state.MSW & MSW_PE)",
        "cpu286_compat_image = next_image",
        "i286core.s = next_runtime",
        "memset(&cpu286_compat_image, 0, sizeof(cpu286_compat_image))",
        "offsetof(Cpu286StateCompat, cpu_type)",
    )
    missing = [value for value in required_state_patterns
               if value not in state_source]
    if missing:
        raise ReachabilityError(
            "state ownership pattern changed: " + ", ".join(missing))
    if io_source.count("CPU_MSW & 1") != 2:
        raise ReachabilityError("CPU_MSW I/O read set changed")
    if core_source.count("ZeroMemory(&i286core.s, sizeof(i286core.s))") != 1:
        raise ReachabilityError("normal-reset state transformation changed")
    if core_source.count(
            "ZeroMemory(&i286core.s, offsetof(I286STAT, cpu_type))") != 1:
        raise ReachabilityError("CPU_SHUT state transformation changed")


def verify_diagnostic(root: pathlib.Path) -> None:
    dispatch = strip_comments(read_text(root, "i286c/v30patch.c"))
    pccore = strip_comments(read_text(root, "machine/pccore.c"))
    state = strip_comments(read_text(root, "i286c/upd9002_state.c"))
    test = strip_comments(
        read_text(root, "tests/upd9002/rep0f_diagnostic_stop.c"))
    requirements = {
        "REPNE patch": dispatch.count(
            "{0x0f, v30_repne_0f_diagnostic_stop}") == 1,
        "REPE patch": dispatch.count(
            "{0x0f, v30_repe_0f_diagnostic_stop}") == 1,
        "runtime rollback": dispatch.count("i286core.s = state_before") == 1,
        "scheduler barriers": pccore.count(
            "upd9002_diagnostic_pending()") == 2,
        "PE preflight": state.count("state.MSW & MSW_PE") == 1,
        "all second bytes": test.count("second < 256") == 1,
        "522 total": (
            "segment_prefixes[] = {0x26, 0x2e, 0x36, 0x3e}" in test and
            re.search(r"prefix_index\s*=\s*0;\s*prefix_index\s*<\s*2",
                      test) is not None and
            re.search(r"second\s*=\s*0;\s*second\s*<\s*256", test)
            is not None and
            "segment_index < NELEMENTS(segment_prefixes)" in test and
            "cases += 2" in test and 2 * (256 + 4) + 2 == 522),
        "runtime atomicity": "memcmp(&state_before, &i286core.s" in test,
        "memory atomicity": "hash_before != memory_hash()" in test,
    }
    missing = sorted(name for name, present in requirements.items()
                     if not present)
    if missing:
        raise ReachabilityError(
            "M48 diagnostic invariant changed: " + ", ".join(missing))


def candidate(candidate_id: str, symbol: str, kind: str, defining_file: str,
              disposition: str, evidence: str, **values: str) -> Dict[str, str]:
    row = {column: "no" for column in COLUMNS}
    row.update({
        "candidate_id": candidate_id,
        "symbol_or_field": symbol,
        "kind": kind,
        "defining_file": defining_file,
        "disposition": disposition,
        "serialized_offset": "-",
        "serialized_size": "-",
        "proposed_deletion_group": "-",
        "evidence": evidence,
    })
    row.update(values)
    return row


def build_candidates() -> List[Dict[str, str]]:
    rows: List[Dict[str, str]] = []

    file_rows = (
        ("file.cmake", "CMakeLists.txt", "active_native_required",
         "explicit active and test source ownership"),
        ("file.cpucore_h", "i286c/cpucore.h", "shared_helper",
         "native macros plus runtime and serialized compatibility layouts"),
        ("file.i286c_h", "i286c/i286c.h", "shared_helper",
         "native helper declarations plus protected aliases"),
        ("file.i286c_mcr", "i286c/i286c.mcr", "shared_helper",
         "native execution macros and conditional SEGSELECT"),
        ("file.i286c_c", "i286c/i286c.c", "active_native_required",
         "native lifecycle, real-mode interrupt, and CPU_SHUT entry points"),
        ("file.i286c_0f", "i286c/i286c_0f.c",
         "unreachable_286_candidate",
         "all definitions are outside the post-M48 final graph"),
        ("file.i286c_ea", "i286c/i286c_ea.c", "shared_helper",
         "active effective-address code shares the selector translation unit"),
        ("file.i286c_mn", "i286c/i286c_mn.c", "shared_helper",
         "active base handlers share overwritten protected candidates"),
        ("file.i286c_fe", "i286c/i286c_fe.c", "shared_helper",
         "active FF-group handlers contain SEGSELECT calls"),
        ("file.v30patch", "i286c/v30patch.c", "active_native_required",
         "sole constructor, active step, and diagnostic root handlers"),
        ("file.upd9002_diagnostic", "i286c/upd9002_diagnostic.c",
         "fail_closed_diagnostic_required", "M48 production safety latch"),
        ("file.upd9002_state", "i286c/upd9002_state.c",
         "active_native_required", "transactional state adapter and PE rejection"),
        ("file.cpuio", "io/cpuio.c", "active_native_required",
         "active reset-request I/O reads CPU_MSW"),
        ("file.statsave", "machine/statsave.c", "active_native_required",
         "CPU286 preflight, load, and save callbacks"),
        ("file.frozen_i286x", "i286x/", "frozen_reference_only",
         "unsupported frozen assembly/reference implementation"),
        ("file.test_rep0f", "tests/upd9002/rep0f_diagnostic_stop.c",
         "test_evidence_only", "522-case atomic diagnostic-stop evidence"),
        ("file.test_state", "tests/upd9002/state_boundary.c",
         "test_evidence_only", "PE rejection and opaque-residue evidence"),
        ("file.test_statsave", "tests/upd9002/statsave_boundary.c",
         "test_evidence_only", "whole-machine preflight atomicity evidence"),
        ("file.tool_dispatch", "tools/qa/upd9002_dispatch.py",
         "test_evidence_only", "source graph and provenance parser"),
        ("file.tool_transition", "tools/qa/upd9002_rep0f_transition.py",
         "test_evidence_only", "immutable M42 to accepted M48 transition proof"),
    )
    for candidate_id, path, disposition, evidence in file_rows:
        group = "-"
        if candidate_id == "file.i286c_0f":
            group = "M50-PM-CTS-SYSTEM"
        rows.append(candidate(
            candidate_id, path, "source_or_evidence_file", path, disposition,
            evidence,
            proposed_deletion_group=group,
            test_dependency="yes" if disposition == "test_evidence_only" else "no",
            frozen_reference_dependency=(
                "yes" if disposition == "frozen_reference_only" else "no")))

    for root, size in (("v30op", 256), ("v30op_repne", 256),
                       ("v30op_repe", 256), ("v30op_repc", 256),
                       ("v30ope0xf6_table", 8),
                       ("v30ope0xf7_table", 8)):
        rows.append(candidate(
            "root." + root, root, "runtime_dispatch_root",
            "i286c/v30patch.c", "active_native_required",
            "post-M48 final root has {} immutable entries".format(size),
            final_dispatch_reachable="yes", address_taken="yes",
            test_dependency="yes"))

    for prefix, root, handler in (
            ("f2", "v30op_repne", "v30_repne_0f_diagnostic_stop"),
            ("f3", "v30op_repe", "v30_repe_0f_diagnostic_stop")):
        rows.append(candidate(
            "diagnostic.entry." + prefix, root + "[0x0f]",
            "final_dispatch_entry", "i286c/v30patch.c",
            "fail_closed_diagnostic_required",
            "M48 final graph routes {} 0F to {}".format(prefix.upper(), handler),
            final_dispatch_reachable="yes", directly_called="indirect",
            address_taken="yes", diagnostic_stop_dependency="yes",
            test_dependency="yes"))
        rows.append(candidate(
            "diagnostic.handler." + prefix, handler, "function",
            "i286c/v30patch.c", "fail_closed_diagnostic_required",
            "raises the {} REP+0F diagnostic using original CS:IP".format(
                prefix.upper()),
            final_dispatch_reachable="yes", directly_called="indirect",
            address_taken="yes", diagnostic_stop_dependency="yes",
            test_dependency="yes"))

    for name, evidence in (
            ("upd9002_diagnostic_raise_rep0f", "latches reason, prefix, CS, and IP"),
            ("upd9002_diagnostic_pending", "blocks step and scheduler progress"),
            ("upd9002_diagnostic_get", "delivers the deterministic frontend error"),
            ("v30c_step", "restores complete runtime state and omits DMA on stop"),
            ("pccore_exec", "checks the latch before and after CPU execution")):
        defining = "i286c/upd9002_diagnostic.c"
        disposition = "fail_closed_diagnostic_required"
        if name == "v30c_step":
            defining = "i286c/v30patch.c"
            disposition = "active_native_required"
        elif name == "pccore_exec":
            defining = "machine/pccore.c"
            disposition = "active_native_required"
        rows.append(candidate(
            "diagnostic.function." + name, name, "function", defining,
            disposition, evidence, directly_called="yes",
            diagnostic_stop_dependency="yes", test_dependency="yes"))

    group = "M50-PM-CTS-SYSTEM"
    rows.append(candidate(
        "cts.declaration", "i286c_cts declaration", "declaration",
        "i286c/i286c.h", "unreachable_286_candidate",
        "only three base-table initializer references remain",
        proposed_deletion_group=group, test_dependency="yes"))
    rows.append(candidate(
        "cts.dispatcher", "i286c_cts", "function", "i286c/i286c_0f.c",
        "unreachable_286_candidate",
        "no post-M48 final edge or direct caller; address occurs in three overwritten base slots",
        directly_called="no", address_taken="yes",
        proposed_deletion_group=group, test_dependency="evidence-only"))
    for table, targets in (("cts0_table", CTS0), ("cts1_table", CTS1)):
        rows.append(candidate(
            "cts.table." + table, table, "secondary_dispatch_table",
            "i286c/i286c_0f.c", "unreachable_286_candidate",
            "only i286c_cts indexes this table; no post-M48 secondary edge",
            secondary_dispatch_reachable="no", directly_called="indirect",
            address_taken="yes", proposed_deletion_group=group,
            test_dependency="evidence-only"))
        for slot, target in enumerate(targets):
            rows.append(candidate(
                "cts.entry.{}.{:02x}".format(table, slot),
                "{}[0x{:02x}] -> {}".format(table, slot, target),
                "secondary_dispatch_entry", "i286c/i286c_0f.c",
                "unreachable_286_candidate",
                "function pointer is reachable only through the unreachable CTS dispatcher",
                secondary_dispatch_reachable="no", address_taken="yes",
                proposed_deletion_group=group, test_dependency="evidence-only"))

    table_membership = {
        "_sldt": "cts0_table[0]",
        "_str": "cts0_table[1]",
        "_lldt": "cts0_table[2]",
        "_ltr": "cts0_table[3]",
        "_verr": "cts0_table[4,6]",
        "_verw": "cts0_table[5,7]",
        "_sgdt": "cts1_table[0]",
        "_sidt": "cts1_table[1]",
        "_lgdt": "cts1_table[2]",
        "_lidt": "cts1_table[3]",
        "_smsw": "cts1_table[4,5]",
        "_lmsw": "cts1_table[6,7]",
        "_loadall286": "direct i286c_cts second-byte 05 call",
    }
    for name in SYSTEM_HANDLERS:
        rows.append(candidate(
            "cts.handler." + name.lstrip("_"), name, "function",
            "i286c/i286c_0f.c", "unreachable_286_candidate",
            table_membership[name] + "; absent from post-M48 final graph",
            secondary_dispatch_reachable="no", directly_called=(
                "yes" if name == "_loadall286" else "indirect"),
            address_taken=("no" if name == "_loadall286" else "yes"),
            active_state_read="legacy-only", active_state_write="legacy-only",
            proposed_deletion_group=group, test_dependency="evidence-only"))

    for table in ("i286op", "i286op_repe", "i286op_repne"):
        rows.append(candidate(
            "cts.base." + table, table + "[0x0f] -> i286c_cts",
            "constructor_base_entry", "i286c/i286c_mn.c",
            "unreachable_286_candidate",
            "copied during construction then necessarily overwritten before immutability snapshot",
            final_dispatch_reachable="no", address_taken="yes",
            proposed_deletion_group=group, test_dependency="evidence-only"))

    for symbol, kind in (
            ("I286_0F", "macro"), ("I286OP_0F", "typedef"),
            ("I286_IDTR", "macro"), ("I286_LDTR", "macro"),
            ("I286_TR", "macro"), ("I286_TRC", "macro")):
        rows.append(candidate(
            "cts.header." + symbol.lower(), symbol, kind, "i286c/i286c.h",
            "unreachable_286_candidate",
            "used only by the unreachable i286c_0f translation unit",
            macro_referenced="yes" if kind == "macro" else "no",
            proposed_deletion_group=group))
    rows.append(candidate(
        "cts.build.cmake", "VAEG_CORE_SOURCES:i286c/i286c_0f.c",
        "build_edge", "CMakeLists.txt", "unreachable_286_candidate",
        "production build membership is required only by the three base-table references",
        proposed_deletion_group=group, test_dependency="yes"))

    for group_id, symbol, slot, evidence in (
            ("M50-PM-ARPL", "_arpl", 0x63,
             "80286 ARPL-shaped handler always overwritten by v30_reserved"),
            ("M50-PM-MOV-SEG-EA", "_mov_seg_ea", 0x8E,
             "286 SEGSELECT segment-load handler always overwritten by v30mov_seg_ea")):
        rows.append(candidate(
            "overwritten.function." + symbol.lstrip("_"), symbol, "function",
            "i286c/i286c_mn.c", "unreachable_286_candidate", evidence,
            address_taken="yes", active_state_read=(
                "legacy-only" if symbol == "_mov_seg_ea" else "no"),
            active_state_write=(
                "legacy-only" if symbol == "_mov_seg_ea" else "no"),
            proposed_deletion_group=group_id, test_dependency="evidence-only"))
        for table in ("i286op", "i286op_repe", "i286op_repne"):
            rows.append(candidate(
                "overwritten.base.{}.{}".format(symbol.lstrip("_"), table),
                "{}[0x{:02x}] -> {}".format(table, slot, symbol),
                "constructor_base_entry", "i286c/i286c_mn.c",
                "unreachable_286_candidate",
                "constructor patch replaces this address before the final graph is exposed",
                final_dispatch_reachable="no", address_taken="yes",
                proposed_deletion_group=group_id,
                test_dependency="evidence-only"))

    rows.append(candidate(
        "selector.function", "i286c_selector", "function",
        "i286c/i286c_ea.c", "shared_helper",
        "two protected direct callers plus SEGSELECT-expanded calls in active native handlers",
        directly_called="yes", active_state_read="yes",
        active_state_address_taken="yes", test_dependency="evidence-only"))
    rows.append(candidate(
        "selector.macro", "SEGSELECT", "macro", "i286c/i286c.mcr",
        "shared_helper",
        "reads MSW_PE and selects i286c_selector or real-mode selector<<4",
        directly_called="yes", macro_referenced="yes",
        active_state_read="yes"))
    for name in SEGSELECT_CALLERS:
        if name == "_mov_seg_ea":
            continue
        defining = "i286c/i286c_fe.c" if name in {
            "_call_far_ea16", "_jmp_far_ea16"} else "i286c/i286c_mn.c"
        rows.append(candidate(
            "selector.caller." + name.lstrip("_"), name, "function",
            defining, "active_native_required",
            "post-M48 final graph reaches this real-mode segment/control-transfer handler",
            final_dispatch_reachable="yes", directly_called="indirect",
            macro_referenced="yes", active_state_read="yes"))

    for name, defining, evidence in (
            ("i286c_intnum", "i286c/i286c.c",
             "shared native exception helper uses the real-mode IVT"),
            ("i286c_interrupt", "i286c/i286c.c",
             "active device interrupt entry uses the real-mode IVT"),
            ("v30_iret", "i286c/v30patch.c",
             "post-M48 final graph return path restores a real-mode CS base")):
        rows.append(candidate(
            "native.interrupt." + name, name, "function", defining,
            "active_native_required", evidence, final_dispatch_reachable=(
                "yes" if name == "v30_iret" else "indirect-lifecycle"),
            directly_called="yes"))

    macro_rows = (
        ("I286_GDTR", "shared_helper", "selector and protected system handlers"),
        ("I286_LDTRC", "shared_helper", "selector and protected system handlers"),
        ("I286_MSW", "shared_helper", "SEGSELECT and protected system handlers"),
        ("CPU_MSW", "active_native_required", "active cpuio_of0 read"),
        ("MSW_PE", "fail_closed_diagnostic_required",
         "state preflight mask and SEGSELECT predicate"),
    )
    for symbol, disposition, evidence in macro_rows:
        defining = "i286c/cpucore.h" if symbol in {"CPU_MSW", "MSW_PE"} \
            else "i286c/i286c.h"
        rows.append(candidate(
            "macro." + symbol.lower(), symbol, "macro_or_constant", defining,
            disposition, evidence, macro_referenced="yes",
            active_state_read="yes", diagnostic_stop_dependency=(
                "yes" if symbol == "MSW_PE" else "no"),
            io_dependency="yes" if symbol == "CPU_MSW" else "no"))

    rows.append(candidate(
        "type.i286dtr", "I286DTR", "type", "i286c/cpucore.h",
        "serialized_compatibility_only",
        "six-byte descriptor-table/cache shape is embedded in both state contracts",
        serialized_size="6", import_dependency="yes", export_dependency="yes",
        reset_dependency="yes", cpu_shut_dependency="yes"))

    for field, offset, size in STATE_FIELDS:
        compat_disposition = (
            "fail_closed_diagnostic_required" if field == "MSW"
            else "serialized_compatibility_only")
        rows.append(candidate(
            "state.compat." + field.lower(), "Cpu286StateCompat." + field,
            "serialized_field", "i286c/cpucore.h", compat_disposition,
            "literal CPU286 byte position retained by Cpu286CompatImage",
            serialized_offset=str(offset), serialized_size=str(size),
            import_dependency="yes", export_dependency="yes",
            reset_dependency="yes", cpu_shut_dependency="yes",
            diagnostic_stop_dependency="yes" if field == "MSW" else "no",
            test_dependency="yes"))

        if field == "MSW":
            runtime_disposition = "shared_helper"
            runtime_evidence = (
                "read by active SEGSELECT and CPU_MSW I/O; legacy writes are unreachable")
        elif field in {"GDTR", "LDTRC"}:
            runtime_disposition = "shared_helper"
            runtime_evidence = "address-taken and read by i286c_selector"
        else:
            runtime_disposition = "serialized_compatibility_only"
            runtime_evidence = (
                "no post-M48 instruction reader; adapter still imports and overlays this range")
        rows.append(candidate(
            "state.runtime." + field.lower(), "Upd9002RuntimeState." + field,
            "runtime_field", "i286c/cpucore.h", runtime_disposition,
            runtime_evidence, active_state_read="yes",
            active_state_write="yes", active_state_address_taken=(
                "yes" if field in {"GDTR", "LDTRC", "IDTR", "TRC"} else "no"),
            serialized_offset=str(offset), serialized_size=str(size),
            import_dependency="yes", export_dependency="yes",
            reset_dependency="yes", cpu_shut_dependency="yes",
            diagnostic_stop_dependency="yes" if field == "MSW" else "no",
            io_dependency="yes" if field == "MSW" else "no",
            test_dependency="yes"))

    lifecycle_rows = (
        ("upd9002_state_validate", "fail_closed_diagnostic_required",
         "reads serialized MSW.PE and cpu_type before commit"),
        ("upd9002_state_import", "active_native_required",
         "constructs temporary runtime/image then commits atomically"),
        ("upd9002_state_export", "active_native_required",
         "starts from opaque image and overlays runtime-owned ranges"),
        ("upd9002_state_reset", "active_native_required",
         "canonicalizes the compatibility image on normal reset"),
        ("upd9002_state_shut", "cpu_shut_compatibility_required",
         "zeros compatibility bytes below cpu_type for CPU_SHUT"),
        ("i286c_reset", "active_native_required",
         "zeros runtime and establishes V30 FLAGS f002"),
        ("i286c_shut", "cpu_shut_compatibility_required",
         "preserves 286-style init and FLAGS 0000"),
        ("cpuio_of0", "active_native_required",
         "reads CPU_MSW, records R/P mode, and requests reset"),
    )
    for name, disposition, evidence in lifecycle_rows:
        if name.startswith("upd9002_state"):
            defining = "i286c/upd9002_state.c"
        elif name.startswith("i286c_"):
            defining = "i286c/i286c.c"
        else:
            defining = "io/cpuio.c"
        rows.append(candidate(
            "lifecycle." + name, name, "function", defining, disposition,
            evidence, directly_called="yes", active_state_read="yes",
            active_state_write="yes", import_dependency=(
                "yes" if name in {"upd9002_state_validate", "upd9002_state_import"}
                else "no"),
            export_dependency=(
                "yes" if name == "upd9002_state_export" else "no"),
            reset_dependency=(
                "yes" if name in {"upd9002_state_reset", "i286c_reset", "cpuio_of0"}
                else "no"),
            cpu_shut_dependency=(
                "yes" if name in {"upd9002_state_shut", "i286c_shut"}
                else "no"),
            diagnostic_stop_dependency=(
                "yes" if name == "upd9002_state_validate" else "no"),
            io_dependency="yes" if name == "cpuio_of0" else "no",
            test_dependency="yes"))

    return rows


def validate_candidates(rows: Sequence[Mapping[str, str]]) -> None:
    identifiers = [row["candidate_id"] for row in rows]
    duplicates = sorted(name for name, count in Counter(identifiers).items()
                        if count != 1)
    if duplicates:
        raise ReachabilityError(
            "duplicate candidate identifiers: " + ", ".join(duplicates))
    for row in rows:
        if set(row) != set(COLUMNS):
            raise ReachabilityError(
                "candidate {} has wrong columns".format(row.get("candidate_id")))
        if row["disposition"] not in DISPOSITIONS:
            raise ReachabilityError(
                "unknown disposition for {}: {}".format(
                    row["candidate_id"], row["disposition"]))
        if row["disposition"] == "unresolved":
            raise ReachabilityError(
                "inventory generation is unresolved at " + row["candidate_id"])
        group = row["proposed_deletion_group"]
        if group != "-":
            if group not in PROPOSED_GROUPS:
                raise ReachabilityError("unknown proposed group: " + group)
            if row["disposition"] != "unreachable_286_candidate":
                raise ReachabilityError(
                    "non-unreachable member proposed for deletion: {}".format(
                        row["candidate_id"]))
        for value in row.values():
            if re.search(r"(?:^|[ ;])/(?:tmp|home|mnt)/|[A-Za-z]:\\", value):
                raise ReachabilityError(
                    "host-dependent path in candidate {}".format(
                        row["candidate_id"]))
    found_groups = {row["proposed_deletion_group"] for row in rows
                    if row["proposed_deletion_group"] != "-"}
    if found_groups != PROPOSED_GROUPS:
        raise ReachabilityError(
            "proposed group set changed: {}".format(sorted(found_groups)))

    required_symbols = set(SYSTEM_HANDLERS) | {
        "i286c_cts", "cts0_table", "cts1_table", "i286c_selector",
        "SEGSELECT", "_arpl", "_mov_seg_ea",
    }
    symbols = {row["symbol_or_field"] for row in rows}
    missing = sorted(required_symbols - symbols)
    if missing:
        raise ReachabilityError(
            "known protected candidates omitted: " + ", ".join(missing))
    for field, _offset, _size in STATE_FIELDS:
        for prefix in ("Cpu286StateCompat.", "Upd9002RuntimeState."):
            if prefix + field not in symbols:
                raise ReachabilityError(
                    "state candidate omitted: " + prefix + field)


def write_csv(rows: Iterable[Mapping[str, str]]) -> bytes:
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=COLUMNS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(sorted(rows, key=lambda row: row["candidate_id"]))
    return output.getvalue().encode("utf-8")


def compare_or_write(path: pathlib.Path, content: bytes, write: bool) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(content)
        return
    try:
        existing = path.read_bytes()
    except OSError as error:
        raise ReachabilityError(
            "cannot read committed inventory {}: {}".format(path, error)) from error
    if existing != content:
        raise ReachabilityError("committed inventory differs from regeneration")


def internal_selftest() -> None:
    sample = "static const I286OP_0F table[] = {one, two};"
    if extract_named_array(sample, "table") != ("one", "two"):
        raise ReachabilityError("array parser selftest failed")
    trailing = "static const I286OP_0F table[] = {one, two,};"
    if extract_named_array(trailing, "table") != ("one", "two"):
        raise ReachabilityError("trailing-comma array selftest failed")
    try:
        extract_named_array(
            "static const I286OP_0F table[] = {one,,two};", "table")
    except ReachabilityError:
        pass
    else:
        raise ReachabilityError("empty array slot selftest failed")

    rows = build_candidates()
    duplicate = [dict(row) for row in rows]
    duplicate.append(dict(rows[0]))
    try:
        validate_candidates(duplicate)
    except ReachabilityError:
        pass
    else:
        raise ReachabilityError("duplicate candidate selftest failed")

    invalid_group = [dict(row) for row in rows]
    for row in invalid_group:
        if row["disposition"] == "shared_helper":
            row["proposed_deletion_group"] = "M50-PM-CTS-SYSTEM"
            break
    try:
        validate_candidates(invalid_group)
    except ReachabilityError:
        pass
    else:
        raise ReachabilityError("non-candidate group selftest failed")


def verify(root: pathlib.Path, write: bool, selftest: bool) -> Tuple[int, str]:
    verify_dispatch(root)
    verify_protected_source(root)
    verify_segselect(root)
    verify_state(root)
    verify_diagnostic(root)
    rows = build_candidates()
    validate_candidates(rows)
    content = write_csv(rows)
    compare_or_write(root / INVENTORY_PATH, content, write)
    if selftest:
        internal_selftest()
    return len(rows), hashlib.sha256(content).hexdigest()


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    return parser.parse_args(argv)


def main(argv: Sequence[str] = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        count, digest = verify(arguments.root.resolve(), arguments.write,
                               arguments.selftest)
    except (ReachabilityError, OSError, ValueError, KeyError, TypeError) as error:
        print("upd9002-protected-reachability: FAIL: {}".format(error),
              file=sys.stderr)
        return 1
    action = "wrote" if arguments.write else "checked"
    print("upd9002-protected-reachability: {} candidates={} sha256={}".format(
        action, count, digest))
    if arguments.selftest:
        print("upd9002-protected-reachability: fail-closed selftest passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

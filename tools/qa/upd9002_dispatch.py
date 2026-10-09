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
"""Generate the source-level uPD9002 dispatch graph and M42 provenance."""

from __future__ import annotations

import argparse
import csv
import io
import re
import sys
from pathlib import Path


class DispatchError(RuntimeError):
    """A fail-closed source interpretation error."""


POINTER_TYPES = {
    "UPD9002OP",
    "UPD9002OP8XREG8",
    "UPD9002OP8XEXT8",
    "UPD9002OP8XREG16",
    "UPD9002OP8XEXT16",
    "UPD9002OPSFTR8",
    "UPD9002OPSFTE8",
    "UPD9002OPSFTR16",
    "UPD9002OPSFTE16",
    "UPD9002OPSFTR8CL",
    "UPD9002OPSFTE8CL",
    "UPD9002OPSFTR16CL",
    "UPD9002OPSFTE16CL",
    "UPD9002OPF6",
}

EXPECTED_ARRAY_SIZES = {
    "upd9002op": 256,
    "upd9002op_repne": 256,
    "upd9002op_repe": 256,
    "upd9002op_repnc": 256,
    "upd9002op_repc": 256,
    "upd9002_ope0x0f_table": 64,
    "c_op8xreg8_table": 8,
    "c_op8xext8_table": 8,
    "c_op8xreg16_table": 8,
    "c_op8xext16_table": 8,
    "sft_r8_table": 8,
    "sft_e8_table": 8,
    "sft_r16_table": 8,
    "sft_e16_table": 8,
    "sft_r8cl_table": 8,
    "sft_e8cl_table": 8,
    "sft_r16cl_table": 8,
    "sft_e16cl_table": 8,
    "c_ope0xf6_table": 8,
    "c_ope0xf7_table": 8,
    "c_ope0xfe_table": 2,
    "c_ope0xff_table": 8,
}

ROOTS = (
    "upd9002op",
    "upd9002op_repne",
    "upd9002op_repe",
    "upd9002op_repnc",
    "upd9002op_repc",
    "c_ope0xf6_table",
    "c_ope0xf7_table",
)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def matching(text: str, start: int, opening: str, closing: str) -> int:
    depth = 0
    for position in range(start, len(text)):
        if text[position] == opening:
            depth += 1
        elif text[position] == closing:
            depth -= 1
            if depth == 0:
                return position
    raise DispatchError(f"unclosed {opening} at source offset {start}")


def split_top_level(text: str) -> list[str]:
    fields: list[str] = []
    start = 0
    braces = parentheses = brackets = 0
    for position, character in enumerate(text):
        if character == "{":
            braces += 1
        elif character == "}":
            braces -= 1
        elif character == "(":
            parentheses += 1
        elif character == ")":
            parentheses -= 1
        elif character == "[":
            brackets += 1
        elif character == "]":
            brackets -= 1
        elif character == "," and not (braces or parentheses or brackets):
            field = text[start:position].strip()
            if not field:
                raise DispatchError("empty initializer entry")
            fields.append(field)
            start = position + 1
        if min(braces, parentheses, brackets) < 0:
            raise DispatchError("unbalanced initializer")
    tail = text[start:].strip()
    if tail:
        fields.append(tail)
    if braces or parentheses or brackets:
        raise DispatchError("unbalanced initializer")
    return fields


def parse_arrays(sources: dict[str, str]) -> dict[str, list[str]]:
    arrays: dict[str, list[str]] = {}
    type_pattern = "|".join(sorted(POINTER_TYPES, key=len, reverse=True))
    pattern = re.compile(
        rf"\b(?:static\s+)?const\s+(?:{type_pattern})\s+"
        rf"(?P<name>[A-Za-z_]\w*)\s*\[[^\]]*\]\s*=\s*\{{"
    )
    for source_name in sorted(sources):
        text = strip_comments(sources[source_name])
        for match in pattern.finditer(text):
            name = match.group("name")
            if name in arrays:
                raise DispatchError(f"duplicate array definition: {name}")
            end = matching(text, match.end() - 1, "{", "}")
            entries = split_top_level(text[match.end():end])
            normalized: list[str] = []
            for entry in entries:
                entry = entry.strip()
                if not re.fullmatch(r"[A-Za-z_]\w*", entry):
                    raise DispatchError(
                        f"unparsed initializer in {name}: {entry!r}")
                normalized.append(entry)
            arrays[name] = normalized
    for name, expected in EXPECTED_ARRAY_SIZES.items():
        if name not in arrays:
            raise DispatchError(f"required table was not parsed: {name}")
        if len(arrays[name]) != expected:
            raise DispatchError(
                f"cardinality mismatch for {name}: expected {expected}, "
                f"got {len(arrays[name])}")
    return arrays


def extract_function_bodies(sources: dict[str, str]) -> dict[str, str]:
    functions: dict[str, str] = {}
    pattern = re.compile(
        r"(?:UPD9002FN|UPD9002_8X|UPD9002_SFT|UPD9002_F6|UPD9002EXT|"
        r"static\s+(?:void|REG8|REG16|UINT|UINT32)|void\s+CPUCALL)\s+"
        r"(?P<name>[A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{"
    )
    for source_name in sorted(sources):
        text = strip_comments(sources[source_name])
        for declaration in re.finditer(
            r"(?:UPD9002FN|UPD9002_8X|UPD9002_SFT|UPD9002_F6|UPD9002EXT)\s+"
            r"(?P<name>[A-Za-z_]\w*)\s*\(", text
        ):
            functions.setdefault(declaration.group("name"), "")
        for declaration in re.finditer(
            r"\bUPD9002_CARRY_REPEAT_WRAPPER\s*\(\s*"
            r"(?P<name>[A-Za-z_]\w*)\s*,", text
        ):
            functions.setdefault(declaration.group("name"), "")
        for match in pattern.finditer(text):
            name = match.group("name")
            end = matching(text, match.end() - 1, "{", "}")
            body = text[match.end():end]
            # A few handlers have architecture-selected bodies with the same
            # source-level identity.  Dispatch identity is the identifier, and
            # the union is conservative for recursively finding table edges.
            if name in functions and functions[name] != body:
                functions[name] += "\n" + body
            else:
                functions[name] = body
    return functions


def construct_roots(
    arrays: dict[str, list[str]], _source: str | None = None
) -> tuple[dict[str, list[str]], list[list[str]]]:
    roots: dict[str, list[str]] = {}
    provenance: list[list[str]] = []
    for root in ROOTS:
        if root not in arrays:
            raise DispatchError(f"required folded root was not parsed: {root}")
        roots[root] = list(arrays[root])
        for slot, target in enumerate(roots[root]):
            provenance.append([
                root, f"0x{slot:02x}", root, target,
                "folded-static", target,
            ])
    return roots, provenance

def called_tables(body: str, known_tables: set[str]) -> set[str]:
    calls = set(re.findall(r"\b([A-Za-z_]\w*)\s*\[[^;\n]*?\]\s*\(", body))
    unknown = {name for name in calls if name not in known_tables}
    if unknown:
        raise DispatchError(
            "unknown function-pointer table edge: " + ", ".join(sorted(unknown))
        )
    return calls


def final_graph(
    roots: dict[str, list[str]], arrays: dict[str, list[str]],
    functions: dict[str, str]
) -> list[list[str]]:
    known_tables = set(arrays) | set(roots)
    rows: list[list[str]] = []
    edge_rows: set[tuple[str, str, str, str]] = set()
    pending: list[str] = list(ROOTS)
    visited: set[str] = set()
    while pending:
        table = pending.pop(0)
        if table in visited:
            continue
        visited.add(table)
        entries = roots.get(table, arrays.get(table))
        if entries is None:
            raise DispatchError(f"unknown reachable table: {table}")
        if table in {
            "upd9002op", "upd9002op_repne", "upd9002op_repe",
            "upd9002op_repnc", "upd9002op_repc"
        }:
            expected = 256
        elif table in {"c_ope0xf6_table", "c_ope0xf7_table"}:
            expected = 8
        else:
            expected = EXPECTED_ARRAY_SIZES.get(table)
        if expected is None or len(entries) != expected:
            raise DispatchError(f"unexpected reachable-table cardinality: {table}")
        for slot, target in enumerate(entries):
            if target not in functions:
                raise DispatchError(f"unknown handler target {target} in {table}")
            rows.append([table, f"0x{slot:02x}", "handler", target])
            for secondary in sorted(called_tables(functions[target], known_tables)):
                edge_rows.add((f"handler:{target}", "-", "secondary-table", secondary))
                if secondary not in visited and secondary not in pending:
                    pending.append(secondary)
    rows.extend([list(row) for row in edge_rows])
    rows.sort(key=lambda row: tuple(row))
    if len(rows) != len({tuple(row) for row in rows}):
        raise DispatchError("non-deterministic or duplicate graph row")
    return rows


def write_csv(path: Path | None, header: list[str], rows: list[list[str]]) -> str:
    output = io.StringIO(newline="")
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(header)
    writer.writerows(rows)
    text = output.getvalue()
    if path is not None:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8", newline="") as handle:
            handle.write(text)
    return text


def harness_rows(provenance: list[list[str]], arrays: dict[str, list[str]]) -> list[list[str]]:
    rows: list[list[str]] = []
    prefixes = {
        "upd9002op": "",
        "upd9002op_repne": "f2",
        "upd9002op_repe": "f3",
        "upd9002op_repnc": "64",
        "upd9002op_repc": "65",
    }
    folded_root_slots = {
        "upd9002op": {
            0x0f, 0x26, 0x27, 0x2e, 0x2f, 0x36, 0x37, 0x3e,
            0x3f, 0x54, 0x63, 0x64, 0x65, 0x66, 0x67, 0x8e,
            0x9c, 0x9d, 0xc0, 0xc1, 0xcf, 0xd2, 0xd3, 0xd4,
            0xd5, 0xd6, 0xe2, 0xf2, 0xf3, 0xf6, 0xf7,
        },
        "upd9002op_repne": {
            0x0f, 0x26, 0x2e, 0x36, 0x3e, 0x54, 0x63, 0x64,
            0x65, 0x66, 0x67, 0x8e, 0x9c, 0x9d, 0xc0, 0xc1,
            0xcf, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf2, 0xf3,
            0xf6, 0xf7,
        },
        "upd9002op_repe": {
            0x0f, 0x26, 0x2e, 0x36, 0x3e, 0x54, 0x63, 0x64,
            0x65, 0x66, 0x67, 0x8e, 0x9c, 0x9d, 0xc0, 0xc1,
            0xcf, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf2, 0xf3,
            0xf6, 0xf7,
        },
        "upd9002op_repnc": {
            0x26, 0x2e, 0x36, 0x3e, 0x64, 0x65, 0xf2, 0xf3,
            0xa4, 0xa5, 0xa6, 0xa7, 0xaa, 0xab, 0xac, 0xad,
            0xae, 0xaf,
        },
        "upd9002op_repc": {
            0x26, 0x2e, 0x36, 0x3e, 0x64, 0x65, 0xf2, 0xf3,
            0xa4, 0xa5, 0xa6, 0xa7, 0xaa, 0xab, 0xac, 0xad,
            0xae, 0xaf,
        },
    }
    for root, slot_text, _base, _base_target, _operation, target in provenance:
        slot = int(slot_text, 16)
        if root in prefixes and slot in folded_root_slots[root]:
            program = prefixes[root] + f"{slot:02x}" + "c0000000000000"
            coverage = "folded-root"
        elif root == "c_ope0xf6_table" and slot in {6, 7}:
            program = "f6" + ("f0" if slot == 6 else "f8") + "00000000"
            coverage = "folded-modrm"
        elif root == "c_ope0xf7_table" and slot in {6, 7}:
            program = "f7" + ("f0" if slot == 6 else "f8") + "00000000"
            coverage = "folded-modrm"
        else:
            continue
        rows.append([
            f"folded-{root}-{slot:02x}", root, slot_text, target,
            program, "1", coverage,
        ])
    for slot, target in enumerate(arrays["upd9002_ope0x0f_table"]):
        rows.append([
            f"native-0f-{slot:02x}", "upd9002_ope0x0f_table", f"0x{slot:02x}",
            target, f"0f{slot:02x}c0000000000000", "1", "native-secondary",
        ])
    rows.extend([
        ["div8-normal", "c_ope0xf6_table", "0x06", "_div_ea8",
         "f6f300000000", "1", "boundary"],
        ["div8-fault", "c_ope0xf6_table", "0x06", "_div_ea8",
         "f6f000000000", "1", "fault"],
        ["idiv16-normal", "c_ope0xf7_table", "0x07", "_idiv_ea16",
         "f7fb00000000", "1", "boundary"],
        ["io-out", "upd9002op", "0xe6", "_out_data8_al", "e67f00000000", "1", "io"],
        ["memory-write", "upd9002op", "0xa2", "_mov_mem_al", "a20030000000", "1", "memory"],
        ["overflow-step", "upd9002op", "0x90", "_nop", "900000000000", "1", "overflow"],
    ])
    rows.sort(key=lambda row: (row[0].startswith("folded-"), row[0]))
    return rows

def load_sources(root: Path) -> dict[str, str]:
    paths = sorted((root / "cpu/upd9002").glob("*.c"))
    if not paths:
        raise DispatchError("no uPD9002 core C sources found")
    return {path.name: path.read_text(encoding="utf-8") for path in paths}


def support_rows(roots: dict[str, list[str]], arrays: dict[str, list[str]]) -> list[list[str]]:
    rows: list[list[str]] = []
    for mode in ("upd9002op", "upd9002op_repne", "upd9002op_repe",
                 "upd9002op_repnc", "upd9002op_repc"):
        for opcode, target in enumerate(roots[mode]):
            status = "known_target_gap" if "reserved" in target else "implemented"
            rows.append([
                mode, f"0x{opcode:02x}", "-", target, status,
                "folded-root-target",
            ])
    for second in range(256):
        target = (arrays["upd9002_ope0x0f_table"][second]
                  if second < 64 else "_reserved_0x0f")
        status = "known_target_gap" if "reserved" in target else "implemented"
        rows.append([
            "upd9002op_0f", "0x0f", f"0x{second:02x}", target, status,
            "second-byte-resolved",
        ])
    for mode in ("c_ope0xf6_table", "c_ope0xf7_table"):
        for group, target in enumerate(roots[mode]):
            rows.append([
                mode, "0xf6" if "f6" in mode else "0xf7", f"/{group}",
                target, "implemented", "modrm-group-resolved",
            ])
    rows.sort(key=lambda row: tuple(row))
    return rows

def generate(root: Path) -> tuple[str, str, str, str]:
    sources = load_sources(root)
    arrays = parse_arrays(sources)
    roots, provenance = construct_roots(arrays)
    functions = extract_function_bodies(sources)
    graph = final_graph(roots, arrays, functions)
    graph_text = write_csv(None, ["table", "slot", "entry_kind", "target"], graph)
    provenance_text = write_csv(
        None,
        ["root", "slot", "base", "base_target", "operation", "final_target"],
        provenance,
    )
    harness_text = write_csv(
        None,
        ["case_id", "table", "slot", "target", "program_hex", "steps", "coverage"],
        harness_rows(provenance, arrays),
    )
    support_text = write_csv(
        None,
        ["mode", "opcode", "subopcode", "target", "classification", "basis"],
        support_rows(roots, arrays),
    )
    return graph_text, provenance_text, harness_text, support_text


def internal_selftest() -> None:
    sample = "const UPD9002OP upd9002op[] = {one, two};"
    old = EXPECTED_ARRAY_SIZES.copy()
    try:
        EXPECTED_ARRAY_SIZES.clear()
        EXPECTED_ARRAY_SIZES["upd9002op"] = 2
        parsed = parse_arrays({"sample.c": sample})
        assert parsed["upd9002op"] == ["one", "two"]
        try:
            parse_arrays({"sample.c": "const UPD9002OP upd9002op[] = {one,};"})
        except DispatchError:
            pass
        else:
            raise AssertionError("missing slot was accepted")
        try:
            parse_arrays({"sample.c": "const UPD9002OP upd9002op[] = {one, two, three};"})
        except DispatchError:
            pass
        else:
            raise AssertionError("cardinality mismatch was accepted")
        try:
            split_top_level("one,,two")
        except DispatchError:
            pass
        else:
            raise AssertionError("empty initializer was accepted")
        try:
            called_tables("unknown_table[x]();", {"known_table"})
        except DispatchError:
            pass
        else:
            raise AssertionError("unknown edge was accepted")
        try:
            construct_roots({"upd9002op": ["one", "two"]})
        except DispatchError:
            pass
        else:
            raise AssertionError("incomplete folded roots were accepted")
    finally:
        EXPECTED_ARRAY_SIZES.clear()
        EXPECTED_ARRAY_SIZES.update(old)


def compare_or_write(path: Path, content: str, write: bool) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8", newline="") as handle:
            handle.write(content)
        return
    if not path.exists():
        raise DispatchError(f"golden is absent: {path}")
    existing = path.read_text(encoding="utf-8")
    if existing != content:
        raise DispatchError(f"generated output differs from golden: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    arguments = parser.parse_args()
    try:
        if arguments.selftest:
            internal_selftest()
        graph, provenance, harness, support = generate(arguments.root)
        compare_or_write(
            arguments.root / "tools/qa/golden/upd9002_final_dispatch_graph.csv",
            graph, arguments.write)
        compare_or_write(
            arguments.root / "tools/qa/golden/upd9002_dispatch_provenance_m42.csv",
            provenance, arguments.write)
        compare_or_write(
            arguments.root / "tests/upd9002/harness_manifest.csv",
            harness, arguments.write)
        compare_or_write(
            arguments.root / "tools/qa/golden/upd9002_support_map_m42.csv",
            support, arguments.write)
    except (DispatchError, OSError) as error:
        print(f"upd9002-dispatch: {error}", file=sys.stderr)
        return 1
    print("upd9002-dispatch: graph, provenance, and harness manifest verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

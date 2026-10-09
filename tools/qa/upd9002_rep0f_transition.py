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
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

"""Verify the REP+0F diagnostic-stop transition after folded dispatch."""

from __future__ import annotations

import argparse
import copy
import csv
import hashlib
import importlib.util
import io
import pathlib
import sys
from typing import Iterable, Sequence


sys.dont_write_bytecode = True


class TransitionError(RuntimeError):
    """A fail-closed REP+0F transition verification error."""


M48_IDENTITIES = {
    "tools/qa/golden/upd9002_final_dispatch_graph_m48.csv":
        "fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6",
    "tools/qa/golden/upd9002_dispatch_provenance_m48.csv":
        "128698af06c4e4e98183e4ec0151b7025f427c4f812f95d9012f41417461027a",
    "tools/qa/golden/upd9002_support_map_m48.csv":
        "21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279",
    "tools/qa/golden/upd9002_rep0f_transition_manifest_m48.json":
        "4f3fefe8cbfb20a03364a80a0b917e475d3d545cab8eda6bee8a22c66e2147ee",
}

CURRENT_OUTPUTS = {
    "tools/qa/golden/upd9002_final_dispatch_graph.csv": 1513,
    "tools/qa/golden/upd9002_dispatch_provenance_m42.csv": 1296,
    "tools/qa/golden/upd9002_support_map_m42.csv": 1552,
    "tests/upd9002/harness_manifest.csv": 193,
}

REQUIRED_GRAPH_ROWS = {
    ("upd9002op_repe", "0x0f", "handler", "_repe_0f_diagnostic_stop"),
    ("upd9002op_repne", "0x0f", "handler", "_repne_0f_diagnostic_stop"),
}

FORBIDDEN_GRAPH_TARGETS = {
    "i286c_cts",
    "cts0_table",
    "cts1_table",
    "_sldt",
    "_str",
    "_lldt",
    "_ltr",
    "_verr",
    "_verw",
    "_sgdt",
    "_sidt",
    "_lgdt",
    "_lidt",
    "_smsw",
    "_lmsw",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_dispatch_module(root: pathlib.Path):
    path = root / "tools/qa/upd9002_dispatch.py"
    spec = importlib.util.spec_from_file_location("upd9002_dispatch_m71", path)
    if spec is None or spec.loader is None:
        raise TransitionError("cannot load upd9002_dispatch.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def rows(text: str) -> set[tuple[str, ...]]:
    return {tuple(row) for row in csv.reader(io.StringIO(text))}


def read_text(root: pathlib.Path, relative: str) -> str:
    return (root / relative).read_text(encoding="utf-8")


def verify_m48_identities(root: pathlib.Path) -> None:
    for relative, expected in M48_IDENTITIES.items():
        actual = sha256((root / relative).read_bytes())
        if actual != expected:
            raise TransitionError(
                "M48 artifact identity changed: {} expected={} actual={}".
                format(relative, expected, actual))


def verify_current_generated_outputs(root: pathlib.Path) -> tuple[str, str, str, str]:
    module = load_dispatch_module(root)
    graph, provenance, harness, support = module.generate(root)
    generated = {
        "tools/qa/golden/upd9002_final_dispatch_graph.csv": graph,
        "tools/qa/golden/upd9002_dispatch_provenance_m42.csv": provenance,
        "tools/qa/golden/upd9002_support_map_m42.csv": support,
        "tests/upd9002/harness_manifest.csv": harness,
    }
    for relative, content in generated.items():
        path = root / relative
        if path.read_text(encoding="utf-8") != content:
            raise TransitionError(
                "current folded dispatch artifact differs: {}".format(relative))
        data_rows = rows(content)
        header_adjusted = len(data_rows) - 1
        if header_adjusted != CURRENT_OUTPUTS[relative]:
            raise TransitionError(
                "current folded dispatch row count changed: {} expected={} actual={}".
                format(relative, CURRENT_OUTPUTS[relative], header_adjusted))
    graph_rows = rows(graph)
    missing = sorted(REQUIRED_GRAPH_ROWS - graph_rows)
    if missing:
        raise TransitionError("REP+0F folded graph rows missing: {}".format(missing))
    forbidden = sorted(
        row for row in graph_rows
        if any(target in row for target in FORBIDDEN_GRAPH_TARGETS)
    )
    if forbidden:
        raise TransitionError(
            "protected-mode 0F rows remain reachable: {}".format(forbidden))
    return graph, provenance, harness, support


def verify_source_policy(root: pathlib.Path) -> None:
    mn = read_text(root, "cpu/upd9002/upd9002_mn.c")
    core = read_text(root, "cpu/upd9002/upd9002_core.c")
    state = read_text(root, "cpu/upd9002/upd9002_state.c")
    pccore = read_text(root, "machine/pccore.c")
    test = read_text(root, "tests/upd9002/rep0f_diagnostic_stop.c")
    requirements = {
        "REPE folded root": "upd9002op_repe[0x0f] != _repe_0f_diagnostic_stop" in mn,
        "REPNE folded root": "upd9002op_repne[0x0f] != _repne_0f_diagnostic_stop" in mn,
        "complete state restore": "upd9002_core_context.s = state_before;" in core,
        "DMA bypass": "upd9002_diagnostic_pending()" in core,
        "scheduler stop": pccore.count("upd9002_diagnostic_pending()") == 2,
        "MSW.PE preflight": state.count("state.MSW & MSW_PE") == 1,
        "512-case loop": "second < 256" in test,
        "state atomic test": "memcmp(&state_before, &upd9002_core_context.s" in test,
        "memory atomic test": "hash_before != memory_hash()" in test,
    }
    missing = [name for name, present in requirements.items() if not present]
    if missing:
        raise TransitionError("source policy evidence missing: " + ", ".join(missing))


def verify(root: pathlib.Path) -> None:
    verify_m48_identities(root)
    verify_current_generated_outputs(root)
    verify_source_policy(root)


def selftest(root: pathlib.Path) -> None:
    verify(root)
    original = REQUIRED_GRAPH_ROWS.copy()
    try:
        REQUIRED_GRAPH_ROWS.clear()
        REQUIRED_GRAPH_ROWS.add(
            ("upd9002op_repe", "0x0f", "handler", "i286c_cts"))
        try:
            verify_current_generated_outputs(root)
        except TransitionError:
            pass
        else:
            raise TransitionError("selftest accepted missing diagnostic stop")
    finally:
        REQUIRED_GRAPH_ROWS.clear()
        REQUIRED_GRAPH_ROWS.update(original)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--selftest", action="store_true")
    arguments = parser.parse_args(sys.argv[1:] if argv is None else argv)
    try:
        root = arguments.root.resolve()
        if arguments.selftest:
            selftest(root)
        else:
            verify(root)
    except (TransitionError, OSError, UnicodeError, ValueError) as error:
        print("rep0f-transition: FAIL: {}".format(error), file=sys.stderr)
        return 1
    print("rep0f-transition: PASS folded REP+0F diagnostic-stop graph verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

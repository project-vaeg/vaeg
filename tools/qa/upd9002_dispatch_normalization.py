#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
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

"""Verify the folded uPD9002 dispatch tables after M71."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys

import upd9002_dispatch


ROOTS = {
    "upd9002op": ("UPD9002OP", 256),
    "upd9002op_repne": ("UPD9002OP", 256),
    "upd9002op_repe": ("UPD9002OP", 256),
    "upd9002op_repnc": ("UPD9002OP", 256),
    "upd9002op_repc": ("UPD9002OP", 256),
    "c_ope0xf6_table": ("UPD9002OPF6", 8),
    "c_ope0xf7_table": ("UPD9002OPF6", 8),
}

REMOVED_FILES = (
    "cpu/upd9002/upd9002_dispatch.c",
    "cpu/upd9002/upd9002_dispatch.h",
)


class NormalizationError(RuntimeError):
    """A fail-closed M71 dispatch normalization verification error."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise NormalizationError(message)


def read_text(root: Path, relative: str) -> str:
    path = root / relative
    require(path.is_file(), "missing required file: {}".format(relative))
    return path.read_text(encoding="utf-8")


def function_body(text: str, signature: str) -> str:
    start = text.find(signature)
    require(start >= 0, "missing function: {}".format(signature))
    brace = text.find("{", start + len(signature))
    require(brace >= 0, "missing function body: {}".format(signature))
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:index]
    raise NormalizationError("unterminated function: {}".format(signature))


def tracked_production_sources(root: Path) -> list[str]:
    result = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z", "*.c", "*.h", "*.cpp"],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    require(result.returncode == 0, "git ls-files failed")
    excluded = ("docs/", "hlp/", "i286x/", "tests/", "win9x/")
    sources: list[str] = []
    for item in result.stdout.split(b"\0"):
        if not item:
            continue
        relative = item.decode("utf-8")
        if relative.startswith(excluded):
            continue
        if not (root / relative).exists():
            continue
        sources.append(relative)
    return sources


def check_removed_dispatch_files(root: Path) -> None:
    for relative in REMOVED_FILES:
        require(not (root / relative).exists(),
                "retired dispatch file still exists: {}".format(relative))
    cmake = read_text(root, "CMakeLists.txt")
    require("cpu/upd9002/upd9002_dispatch.c" not in cmake,
            "retired dispatch source remains in CMake core sources")


def check_constructor_absent(root: Path) -> None:
    references: dict[str, int] = {}
    for relative in tracked_production_sources(root):
        count = len(re.findall(
            r"\bupd9002_dispatch_initialize\s*\(",
            read_text(root, relative)))
        if count:
            references[relative] = count
    require(references == {},
            "retired constructor references remain: {}".format(references))


def check_folded_roots(root: Path) -> None:
    core = read_text(root, "cpu/upd9002/upd9002_core.c")
    mn = read_text(root, "cpu/upd9002/upd9002_mn.c")
    f6 = read_text(root, "cpu/upd9002/upd9002_f6.c")
    sources = upd9002_dispatch.load_sources(root)
    arrays = upd9002_dispatch.parse_arrays(sources)
    for name, (pointer_type, expected) in ROOTS.items():
        require(name in arrays, "folded root definition missing: {}".format(name))
        require(len(arrays[name]) == expected,
                "folded root cardinality changed for {}: {}".format(
                    name, len(arrays[name])))
    combined = mn + "\n" + f6
    require("static UPD9002OP v30op" not in combined,
            "retired mutable v30op root remains")
    require("V30PATCH" not in combined,
            "retired dispatch patch structure remains")
    require("V30PATCHING" not in combined,
            "retired dispatch patch macro remains")
    require("_div_ea8" in f6 and "_idiv_ea8" in f6,
            "canonical F6 byte division handlers are missing")
    require("_div_ea16" in f6 and "_idiv_ea16" in f6,
            "canonical F7 word division handlers are missing")
    step = function_body(core, "void upd9002_core_step(void)")
    require("upd9002op[opcode]();" in step,
            "core step does not dispatch through folded canonical root")
    require("upd9002_dispatch_test_verify" in mn,
            "M46 dispatch QA seam was not retained after folding")


def check_test_lifecycle(root: Path) -> None:
    dedicated = read_text(root, "tests/upd9002/dispatch_normalization.c")
    require(dedicated.count("upd9002_core_initialize();") == 1,
            "dedicated QA initialization count changed")
    require(dedicated.count("upd9002_core_reset();") == 1,
            "dedicated QA reset count changed")
    require("upd9002_dispatch_initialize" not in dedicated,
            "dedicated QA still calls the retired constructor")
    require("upd9002_dispatch_test_construction_count() != 0" in dedicated,
            "dedicated QA does not enforce removed constructor count")
    require("upd9002_dispatch_test_rejected_count() != 0" in dedicated,
            "dedicated QA does not enforce removed re-entry count")
    selftest = read_text(root, "sdl2/selftest.c")
    require(selftest.count("upd9002_dispatch_normalization_verify_live()") == 2,
            "selftest/state-load verification count changed")
    main = read_text(root, "sdl2/np2.c")
    require(main.count("--upd9002-m46-dispatch-qa") == 1,
            "dedicated QA entry point changed")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    arguments = parser.parse_args()
    root = arguments.root.resolve()
    try:
        check_removed_dispatch_files(root)
        check_constructor_absent(root)
        check_folded_roots(root)
        check_test_lifecycle(root)
    except (NormalizationError, OSError, UnicodeError, ValueError) as error:
        print("upd9002-dispatch-normalization-static: FAIL: {}".format(error),
              file=sys.stderr)
        return 1
    print("upd9002-dispatch-normalization-static: folded canonical roots verified")
    print("upd9002-dispatch-normalization-static: "
          "retired dispatch source/header/constructor absent")
    print("upd9002-dispatch-normalization-static: roots="
          "upd9002op:256,upd9002op_repne:256,upd9002op_repe:256,"
          "upd9002op_repnc:256,upd9002op_repc:256,"
          "c_ope0xf6_table:8,c_ope0xf7_table:8")
    return 0


if __name__ == "__main__":
    sys.exit(main())

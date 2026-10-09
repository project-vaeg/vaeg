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
"""Fail closed on retired active uPD9002 ownership names after M51."""

from __future__ import annotations

import argparse
import pathlib
import re
import subprocess
import sys
from typing import Dict, Iterable, List, Mapping, Sequence, Set, Tuple


sys.dont_write_bytecode = True


class RenameError(RuntimeError):
    """A fail-closed M51 rename verification error."""


REQUIRED_PATHS = (
    "cpu/upd9002/upd9002_core.c",
    "io/upd9002_regs.c",
    "io/upd9002_regs.h",
    "memoryva/memoryva.h",
)

RETIRED_PATHS = (
    "iova/upd9002.c",
    "iova/upd9002.h",
    "cpuxva/memoryva.h",
    "cpuxva/memoryva.x86",
)

RETIRED_CORE_APIS = (
    "i286c_initialize",
    "i286c_deinitialize",
    "i286c_reset",
    "i286c_shut",
    "i286c_setextsize",
    "i286c_setemm",
    "i286c_interrupt",
    "v30c_step",
    "v30cinit",
)

REQUIRED_CORE_APIS = (
    "upd9002_core_initialize",
    "upd9002_core_deinitialize",
    "upd9002_core_reset",
    "upd9002_core_shut",
    "upd9002_core_set_ext_size",
    "upd9002_core_set_emm",
    "upd9002_core_interrupt",
    "upd9002_core_step",
)

M71_RETIRED_PATHS = (
    "cpu/upd9002/upd9002_dispatch.c",
    "cpu/upd9002/upd9002_dispatch.h",
)

M71_RETIRED_DISPATCH_TOKENS = (
    "upd9002_dispatch_initialize",
    "V30PATCH",
    "V30PATCHING",
    "v30op",
    "v30op_repe",
    "v30op_repne",
    "v30op_repc",
    "v30op_repnc",
    "v30ope0xf6_table",
    "v30ope0xf7_table",
)

# M51 allowed a small graph-bound internal compatibility exception list.
# M66b removes those active inherited identifiers; the final exception set is
# deliberately empty.
APPROVED_INTERNAL_EXCEPTIONS: Tuple[str, ...] = ()

EXCEPTION_PATHS: Mapping[str, Set[str]] = {}

ACTIVE_SOURCE_SUFFIXES = {
    ".c", ".cc", ".cpp", ".h", ".hpp", ".mcr", ".rc", ".tbl",
}
def git_files(root: pathlib.Path) -> List[str]:
    completed = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard",
         "-z"],
        cwd=root,
        check=True,
        capture_output=True,
    )
    return sorted(
        path for path in completed.stdout.decode("utf-8").split("\0")
        if path
    )


def read_text(root: pathlib.Path, relative: str) -> str:
    try:
        return (root / relative).read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise RenameError(f"cannot read {relative}: {exc}") from exc


def active_sources(files: Iterable[str]) -> Iterable[str]:
    for relative in files:
        if pathlib.PurePosixPath(relative).suffix.lower() in ACTIVE_SOURCE_SUFFIXES:
            yield relative


def exact_word_matches(
    root: pathlib.Path, files: Iterable[str], names: Sequence[str]
) -> Dict[str, Set[str]]:
    patterns = {
        name: re.compile(r"(?<![A-Za-z0-9_])" + re.escape(name)
                         + r"(?![A-Za-z0-9_])")
        for name in names
    }
    found = {name: set() for name in names}
    for relative in files:
        text = read_text(root, relative)
        for name, pattern in patterns.items():
            if pattern.search(text):
                found[name].add(relative)
    return found


def require_fragments(text: str, fragments: Sequence[str], source: str) -> None:
    for fragment in fragments:
        if fragment not in text:
            raise RenameError(f"{source}: required spelling missing: {fragment}")


def verify(root: pathlib.Path) -> None:
    files = git_files(root)
    file_set = set(files)

    stale_directory = sorted(path for path in files if path.startswith("i286c/"))
    if stale_directory:
        raise RenameError("retired active i286c/ paths: "
                          + ", ".join(stale_directory))

    missing = sorted(path for path in REQUIRED_PATHS if path not in file_set)
    if missing:
        raise RenameError("required M51 paths missing: " + ", ".join(missing))

    stale_paths = sorted(path for path in RETIRED_PATHS + M71_RETIRED_PATHS
                         if path in file_set and (root / path).exists())
    if stale_paths:
        raise RenameError("retired M51 paths remain: " + ", ".join(stale_paths))

    source_files = [
        path for path in active_sources(files) if (root / path).exists()
    ]
    retired = exact_word_matches(root, source_files, RETIRED_CORE_APIS)
    retired_hits = [
        f"{name}: {', '.join(sorted(paths))}"
        for name, paths in retired.items() if paths
    ]
    if retired_hits:
        raise RenameError("retired public core API matches:\n  "
                          + "\n  ".join(retired_hits))

    cmake = read_text(root, "CMakeLists.txt")
    require_fragments(cmake, (
        "${CMAKE_CURRENT_SOURCE_DIR}/cpu/upd9002",
        "${CMAKE_CURRENT_SOURCE_DIR}/cpu",
        "cpu/upd9002/upd9002_core.c",
        "io/upd9002_regs.c",
    ), "CMakeLists.txt")
    if "cpu/upd9002/upd9002_dispatch.c" in cmake:
        raise RenameError(
            "CMakeLists.txt: retired dispatch source remains after M71")
    obsolete_cmake = (
        "${CMAKE_CURRENT_SOURCE_DIR}/i286c",
        "${CMAKE_CURRENT_SOURCE_DIR}/cpuxva",
        "i286c/i286c.c",
        "i286c/v30patch.c",
        "iova/upd9002.c",
    )
    for token in obsolete_cmake:
        if token in cmake:
            raise RenameError(f"CMakeLists.txt: obsolete entry remains: {token}")

    core_header = read_text(root, "cpu/upd9002/cpucore.h")
    core_source = read_text(root, "cpu/upd9002/upd9002_core.c")
    declaration_text = core_header
    definition_text = core_source
    for name in REQUIRED_CORE_APIS:
        word = re.compile(r"(?<![A-Za-z0-9_])" + re.escape(name)
                          + r"(?![A-Za-z0-9_])")
        if not word.search(declaration_text):
            raise RenameError(f"public declaration missing: {name}")
        if not word.search(definition_text):
            raise RenameError(f"public definition missing: {name}")
    active_text = "\n".join(read_text(root, relative)
                            for relative in source_files
                            if relative.startswith("cpu/upd9002/"))
    for name in M71_RETIRED_DISPATCH_TOKENS:
        word = re.compile(r"(?<![A-Za-z0-9_])" + re.escape(name)
                          + r"(?![A-Za-z0-9_])")
        if word.search(active_text):
            raise RenameError(f"retired dispatch token remains: {name}")

    register_header = read_text(root, "io/upd9002_regs.h")
    register_source = read_text(root, "io/upd9002_regs.c")
    require_fragments(register_header, (
        "UPD9002_REGS",
        "upd9002_regs",
        "upd9002_regs_reset",
        "upd9002_regs_bind",
    ), "io/upd9002_regs.h")
    require_fragments(register_source, (
        "UPD9002_REGS upd9002_regs",
        "void upd9002_regs_reset(void)",
        "void upd9002_regs_bind(void)",
    ), "io/upd9002_regs.c")
    retired_register_patterns = {
        "old register include": re.compile(r'[<\"]upd9002\.h[>\"]'),
        "old register reset API": re.compile(r"\bupd9002_reset\b"),
        "old register bind API": re.compile(r"\bupd9002_bind\b"),
        "old register type": re.compile(r"\b_UPD9002\b"),
        "generic register global": re.compile(
            r"(?:&\s*upd9002\b|sizeof\s*\(\s*upd9002\b|"
            r"\bupd9002\s*(?:\.|->)\s*(?:tcks|dmy)\b)"
        ),
    }
    register_hits: List[str] = []
    for relative in source_files:
        text = read_text(root, relative)
        for label, pattern in retired_register_patterns.items():
            if pattern.search(text):
                register_hits.append(f"{label}: {relative}")
    if register_hits:
        raise RenameError("retired register-model matches:\n  "
                          + "\n  ".join(sorted(register_hits)))

    exception_files = [
        path for path in source_files if path.startswith("cpu/upd9002/")
    ]
    retired_internal = exact_word_matches(root, exception_files, (
        "i286c_rep_insb",
        "i286c_rep_insw",
        "i286c_rep_outsb",
        "i286c_rep_outsw",
        "i286c_rep_movsb",
        "i286c_rep_movsw",
        "i286c_rep_lodsb",
        "i286c_rep_lodsw",
        "i286c_rep_stosb",
        "i286c_rep_stosw",
        "i286c_repe_cmpsb",
        "i286c_repne_cmpsb",
        "i286c_repe_cmpsw",
        "i286c_repne_cmpsw",
        "i286c_repe_scasb",
        "i286c_repne_scasb",
        "i286c_repe_scasw",
        "i286c_repne_scasw",
    ))
    retained = [
        f"{name}: {', '.join(sorted(paths))}"
        for name, paths in retired_internal.items() if paths
    ]
    if retained:
        raise RenameError("retired internal compatibility identifiers remain:\n  "
                          + "\n  ".join(retained))

    statsave = read_text(root, "machine/statsave.tbl")
    if statsave.count('{"UPD9002"') != 1:
        raise RenameError("statsave.tbl: UPD9002 section tag is not unique")
    if "&upd9002_regs" not in statsave or "sizeof(upd9002_regs)" not in statsave:
        raise RenameError("statsave.tbl: UPD9002 section does not use upd9002_regs")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    args = parser.parse_args()
    try:
        root = args.root.resolve(strict=True)
        verify(root)
    except (RenameError, subprocess.CalledProcessError) as exc:
        print(f"upd9002 rename check: FAIL: {exc}", file=sys.stderr)
        return 1
    print("upd9002 rename check: PASS")
    print("  retired public core APIs: absent")
    print("  retired active paths: absent")
    print("  register-model owner: upd9002_regs")
    print("  approved internal historical exceptions: 0")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

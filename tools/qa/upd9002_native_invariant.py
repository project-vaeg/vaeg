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

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys


FROZEN_OR_HISTORICAL_PREFIXES = (
    "cpuxva/",
    "docs/",
    "hlp/",
    "i286x/",
    "win9x/",
    "tests/ssts/",
)
SCANNED_SUFFIXES = {
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".h",
    ".json",
    ".mcr",
    ".ps1",
    ".py",
    ".sh",
    ".yml",
    ".yaml",
}
BANNED_ACTIVE_TOKENS = (
    "USE_I286C",
    "i286x_step",
    "v30x_step",
    "i286c_step",
    "CPU_TYPE",
    "SINGLESTEPONLY",
    "CPU_EXEC",
    "CPU_EXECV30",
)
EXPECTED_PRESETS = {
    "linux-debug",
    "linux-release",
    "linux-ci-gcc",
    "linux-ci-clang",
    "linux-asan",
    "linux-ci-asan",
    "mingw-release",
    "mingw-cross",
    "mingw-ci",
    "mingw-static-dependencies",
    "macos-release",
    "macos-macports",
    "macos-asan",
    "macos-ci",
}


class InvariantError(Exception):
    pass


def require(condition, message):
    if not condition:
        raise InvariantError(message)


def read_text(root, relative):
    path = root / relative
    require(path.is_file(), "missing required file: {}".format(relative))
    return path.read_text(encoding="utf-8")


def function_body(text, signature):
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
    raise InvariantError("unterminated function: {}".format(signature))


def cmake_call(text, signature):
    start = text.find(signature)
    require(start >= 0, "missing CMake call: {}".format(signature))
    opening = text.find("(", start)
    require(opening >= 0, "missing CMake call body: {}".format(signature))
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return text[opening + 1:index]
    raise InvariantError("unterminated CMake call: {}".format(signature))


def tracked_active_files(root):
    result = subprocess.run(
        ["git", "-C", str(root), "ls-files", "--cached", "--others",
         "--exclude-standard", "-z"],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    require(result.returncode == 0, "git ls-files failed")
    for raw in result.stdout.split(b"\0"):
        if not raw:
            continue
        relative = raw.decode("utf-8")
        if relative == "tools/qa/upd9002_native_invariant.py":
            continue
        if relative.startswith(FROZEN_OR_HISTORICAL_PREFIXES):
            continue
        if not (root / relative).exists():
            continue
        path = Path(relative)
        if (path.name != "CMakeLists.txt") and (path.suffix not in SCANNED_SUFFIXES):
            continue
        yield relative


def check_forbidden_tokens(root):
    findings = []
    for relative in tracked_active_files(root):
        text = read_text(root, relative)
        for line_number, line in enumerate(text.splitlines(), 1):
            for token in BANNED_ACTIVE_TOKENS:
                if re.search(r"\b{}\b".format(re.escape(token)), line):
                    findings.append("{}:{}:{}".format(relative, line_number, token))
    require(not findings, "forbidden active selector references: {}".format(
        ", ".join(findings)))


def check_presets_and_core(root):
    presets = json.loads(read_text(root, "CMakePresets.json"))
    configured = {
        preset["name"] for preset in presets.get("configurePresets", [])
    }
    require(configured == EXPECTED_PRESETS,
            "supported preset set changed: {}".format(
                ",".join(sorted(configured))))

    cmake = read_text(root, "CMakeLists.txt")
    sources = cmake_call(cmake, "set(VAEG_CORE_SOURCES")
    require("cpu/upd9002/upd9002_core.c" in sources,
            "uPD9002 core implementation is missing")
    require("cpu/upd9002/upd9002_dispatch.c" not in sources,
            "retired uPD9002 dispatch source remains")
    require("i286x/" not in sources, "frozen assembly core entered active sources")
    return len(configured)


def check_native_lifecycle(root):
    core = read_text(root, "cpu/upd9002/upd9002_core.c")
    initialize = function_body(core, "void upd9002_core_initialize(void)")
    reset = function_body(core, "void upd9002_core_reset(void)")
    shut = function_body(core, "void upd9002_core_shut(void)")
    scheduler = function_body(read_text(root, "machine/pccore.c"),
                              "void pccore_exec(BOOL draw)")

    require("upd9002_core_context.s.cpu_type = CPUTYPE_V30;" in initialize,
            "initialization does not establish the V30 compatibility byte")
    require("upd9002_core_context.s.cpu_type = CPUTYPE_V30;" in reset,
            "reset does not establish the V30 compatibility byte")
    require(reset.count("upd9002_core_initreg();") == 1,
            "reset does not select the native register initializer exactly once")
    require("upd9002_initreg();" not in reset,
            "normal reset still selects the 286-style initializer")
    require(shut.count("upd9002_initreg();") == 1,
            "CPU_SHUT no longer preserves the 286-style initializer")
    require("upd9002_core_initreg();" not in shut,
            "CPU_SHUT upper-FLAGS anomaly was normalized")
    require("ADR-0012" in shut and "not an 80286 execution mode" in shut,
            "CPU_SHUT exception is not linked to the ownership ADR")
    require(scheduler.count("upd9002_core_step();") == 1,
            "scheduler does not call upd9002_core_step exactly once")
    require("CPU_EXEC" not in scheduler,
            "block executor remains reachable from the scheduler")

    require("void upd9002_legacy_block_executor(void)" not in core,
            "retired block executor remains after M46")
    header = read_text(root, "cpu/upd9002/cpucore.h")
    require("void upd9002_legacy_block_executor(void)" not in header,
            "i286c block-executor declaration remains after M46")
    require("void v30c(void)" not in header,
            "v30c block-executor declaration remains after M46")


def check_cpu_type_reference_map(root):
    allowed = {
        "cpu/upd9002/cpucore.h",
        "cpu/upd9002/upd9002_core.c",
        "cpu/upd9002/upd9002_state.c",
        "cpu/upd9002/upd9002_state.h",
    }
    counts = {}
    for relative in tracked_active_files(root):
        if relative.startswith("tests/") or relative.startswith("tools/"):
            continue
        text = read_text(root, relative)
        count = len(re.findall(r"\bcpu_type\b", text))
        if not count:
            continue
        require(relative in allowed,
                "unexpected production cpu_type reference: {}".format(relative))
        counts[relative] = count

    require(set(counts) == allowed,
            "cpu_type reference map is incomplete: {}".format(
                ",".join(sorted(counts))))

    header_lines = [line.strip() for line in
                    read_text(root, "cpu/upd9002/cpucore.h").splitlines()
                    if re.search(r"\bcpu_type\b", line)]
    require(header_lines == ["UINT8 cpu_type;", "UINT8 cpu_type;"],
            "cpucore cpu_type declarations changed")

    for line in read_text(root, "cpu/upd9002/upd9002_core.c").splitlines():
        if not re.search(r"\bcpu_type\b", line):
            continue
        stripped = line.strip()
        require((stripped == "upd9002_core_context.s.cpu_type = CPUTYPE_V30;") or
                ("offsetof(Upd9002RuntimeState, cpu_type)" in stripped),
                "cpu_type became production control: {}".format(stripped))

    adapter = read_text(root, "cpu/upd9002/upd9002_state.c")
    require(adapter.count("if (state.cpu_type != CPUTYPE_V30)") == 1,
            "state adapter V30 validation changed")
    return counts


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    arguments = parser.parse_args()
    root = arguments.root.resolve()

    try:
        preset_count = check_presets_and_core(root)
        check_forbidden_tokens(root)
        check_native_lifecycle(root)
        reference_counts = check_cpu_type_reference_map(root)
    except (InvariantError, OSError, UnicodeError, ValueError) as error:
        print("upd9002-native-invariant: FAIL: {}".format(error),
              file=sys.stderr)
        return 1

    reference_text = ",".join(
        "{}={}".format(path, reference_counts[path])
        for path in sorted(reference_counts)
    )
    print("upd9002-native-invariant: presets={} core=upd9002 selectors=absent".format(
        preset_count))
    print("upd9002-native-invariant: reset=upd9002_core_initreg "
          "step=upd9002_core_step "
          "shutdown=upd9002_initreg")
    print("upd9002-native-invariant: cpu_type={} control=state-validation-only".format(
        reference_text))
    print("upd9002-native-invariant: block-executors=absent cpu-exec-macros=absent")
    return 0


if __name__ == "__main__":
    sys.exit(main())

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
"""Build and verify the standalone PC-88VA REP+0F DOS probes."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys
from typing import Any, Iterable


class ProbeError(RuntimeError):
    """The probe manifest, tool, or output failed a closed check."""


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_manifest(path: pathlib.Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ProbeError("cannot read {}: {}".format(path, error)) from error
    expected = {
        "cases", "copyright", "license", "loadall286", "reference_builder",
        "schema", "source_sha256",
    }
    if not isinstance(value, dict) or set(value) != expected:
        raise ProbeError("manifest keys changed")
    if value["schema"] != "vaeg-pc88va-rep0f-probe-m47-v1":
        raise ProbeError("manifest schema changed")
    cases = value["cases"]
    if not isinstance(cases, list) or len(cases) != 8:
        raise ProbeError("exactly eight probe cases are required")
    required = {
        "binary", "case_id", "define", "instruction", "purpose",
        "reference_sha256", "risk_control",
    }
    for number, raw in enumerate(cases, 1):
        if not isinstance(raw, dict) or set(raw) != required:
            raise ProbeError("case {} keys changed".format(number))
        if (raw["define"] != number
                or raw["case_id"] != "r{:03d}".format(number)
                or raw["binary"] != "r0f{:03d}.com".format(number)
                or not re.fullmatch(r"[0-9a-f]{64}", raw["reference_sha256"])):
            raise ProbeError("case {} identity changed".format(number))
    return value


def build(root: pathlib.Path, output: pathlib.Path, check: bool) -> None:
    source = root / "rep0f_probe.asm"
    manifest = load_manifest(root / "probe_manifest.json")
    if sha256_file(source) != manifest["source_sha256"]:
        raise ProbeError("assembly source digest differs from manifest")
    try:
        version = subprocess.run(
            ["nasm", "-v"], check=True, capture_output=True, text=True
        ).stdout.strip()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ProbeError("NASM is required: {}".format(error)) from error
    output.mkdir(parents=True, exist_ok=True)
    for case in manifest["cases"]:
        binary = output / case["binary"]
        command = [
            "nasm", "-f", "bin", "-DCASE_ID={}".format(case["define"]),
            "-o", str(binary), str(source),
        ]
        try:
            subprocess.run(command, check=True)
        except (OSError, subprocess.CalledProcessError) as error:
            raise ProbeError("build failed for {}: {}".format(
                case["case_id"], error
            )) from error
        digest = sha256_file(binary)
        status = "verified" if digest == case["reference_sha256"] else "different"
        print("{} {} bytes={} sha256={} {}".format(
            case["case_id"], binary, binary.stat().st_size, digest, status
        ))
        if check and status != "verified":
            raise ProbeError("{} digest changed under {}".format(
                case["case_id"], version
            ))
    print("pc88va-rep0f-probe: PASS builder={!r} cases=8 check={}".format(
        version, check
    ))


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("build", "check"))
    parser.add_argument("--root", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parent)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    return parser.parse_args(argv)


def main(argv: Iterable[str] | None = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        build(arguments.root.resolve(), arguments.output.resolve(),
              arguments.command == "check")
    except (ProbeError, OSError, TypeError, ValueError) as error:
        print("pc88va-rep0f-probe: FAIL: {}".format(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

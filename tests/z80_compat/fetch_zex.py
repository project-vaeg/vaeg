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
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

"""Acquire and verify the external ZEX conformance inputs."""

import argparse
import hashlib
from pathlib import Path
import tempfile
from typing import Optional
import urllib.request


BASE_SHA = "e3926769a790fab0af1c34a5540e317f8d4f0ddc"
BASE_URL = (
    "https://raw.githubusercontent.com/suzukiplan/z80/"
    f"{BASE_SHA}/test-ex"
)
ARTIFACTS = (
    (
        "zexdoc.cim",
        "zexdoc.cim",
        "b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90",
    ),
    (
        "zexall.cim",
        "zexall.cim",
        "fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0",
    ),
    (
        "zexdoc.src",
        "zexdoc.src",
        "0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3",
    ),
    (
        "zexall.src",
        "zexall.src",
        "a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c",
    ),
    (
        "LICENSE.txt",
        "zex-license.txt",
        "e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052",
    ),
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_source(source_dir: Optional[Path], source_name: str) -> bytes:
    if source_dir is not None:
        path = source_dir / source_name
        try:
            return path.read_bytes()
        except OSError as error:
            raise SystemExit(f"cannot read offline ZEX input {path}: {error}")
    url = f"{BASE_URL}/{source_name}"
    print(f"fetch {url}")
    try:
        with urllib.request.urlopen(url, timeout=60) as response:
            return response.read()
    except OSError as error:
        raise SystemExit(f"cannot fetch {url}: {error}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Acquire the pinned ZEX inputs and verify every SHA-256"
    )
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--source-dir",
        type=Path,
        help="offline directory containing the five upstream test-ex files",
    )
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    for source_name, output_name, expected in ARTIFACTS:
        output_path = args.output_dir / output_name
        if output_path.exists():
            actual = sha256(output_path.read_bytes())
            if actual != expected:
                raise SystemExit(
                    f"hash mismatch for cached {output_path}: "
                    f"expected {expected}, got {actual}"
                )
            print(f"verified cache {output_name} {actual}")
            continue

        data = read_source(args.source_dir, source_name)
        actual = sha256(data)
        if actual != expected:
            raise SystemExit(
                f"hash mismatch for {source_name}: "
                f"expected {expected}, got {actual}"
            )
        with tempfile.NamedTemporaryFile(
            dir=args.output_dir, prefix=f".{output_name}.", delete=False
        ) as temporary:
            temporary.write(data)
            temporary_path = Path(temporary.name)
        temporary_path.replace(output_path)
        print(f"verified {output_name} {actual}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Validate the optional librashader payload in a staged VAEG package."""

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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from __future__ import annotations

import argparse
import hashlib
import pathlib
import sys
import tarfile
import zipfile


EXPECTED = {
    "assets/shaders/crt/vaeg_crt_default.slangp":
        "6965dca041d248013a1b52f353da5e19ddc4fae9d0efa379c9eb344af5a5e31f",
    "assets/shaders/crt/shaders/vaeg-screen-size.slang":
        "9a521d7ecf3ead998a5039d33d144903c5764793eba5415dc98ac4ae7ec5c361",
    "assets/shaders/crt/shaders/crt-lottes-fast.slang":
        "576eddc662ac4f77909c0c14dbd5a16ac4164e50c67527fff634316f4441c482",
    "assets/shaders/crt/shaders/vaeg-crt-aa.slang":
        "7cef376e3f8d4c76112842b155cf642b8a32eb4b89921dd026ba55f07390b9ee",
    "assets/shaders/crt/shaders/vaeg-scanline-aa.inc":
        "6461786e3d74abd56a4be789018224bd41ff834d0d9a40c3d4f67e2bf471c408",
    "assets/shaders/crt/licenses/crt-default-license.txt":
        "6b36a9fe4618402e929fb3403d4724d1b707934f2d1db8483fbf0ebfbccb26bc",
    "assets/shaders/crt/licenses/crt-default-provenance.md":
        "6e536ecceac77f736de33aa4730c321d3ec8c0c4edf10ca12f32ed25e7f9aa77",
    "licenses/librashader-MPL-2.0.txt":
        "69c15395f33bc9ce8e1d8b6cef42b7e49cdec4c6f5233d4b9cfc4bfa335f97f9",
    "licenses/librashader-headers-MIT.txt":
        "f2b103e6d0dbff9ea3cebe848f3b10c099215231a3d5edc99fa1fa2b9bba13a3",
    "licenses/THIRD_PARTY_NOTICES.md": None,
}
RUNTIME_NAMES = {"linux": "librashader.so", "macos": "librashader.dylib",
                 "windows": "librashader.dll"}
FORBIDDEN_PARTS = ("crt-geom", "crt-royale", "mega-bezel", "slang-shaders")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def validate(files: dict[str, bytes], platform: str) -> list[str]:
    errors: list[str] = []
    for name, expected in EXPECTED.items():
        if name not in files:
            errors.append(f"missing required file: {name}")
        elif expected is not None and sha256(files[name]) != expected:
            errors.append(f"hash mismatch: {name}")
    for name in files:
        lowered = name.lower()
        if any(part in lowered for part in FORBIDDEN_PARTS):
            errors.append(f"prohibited shader payload: {name}")
    runtime = RUNTIME_NAMES[platform]
    for name in files:
        basename = name.rsplit("/", 1)[-1]
        if basename.startswith("librashader.") and basename != runtime:
            errors.append(f"wrong-platform librashader runtime: {name}")
    notice = files.get("licenses/THIRD_PARTY_NOTICES.md")
    if notice is not None and b"librashader" not in notice:
        errors.append("third-party notice does not mention librashader")
    return errors


def read_input(path: pathlib.Path) -> dict[str, bytes]:
    if path.is_dir():
        return {str(item.relative_to(path)): item.read_bytes()
                for item in path.rglob("*") if item.is_file()}
    if path.suffix == ".zip":
        with zipfile.ZipFile(path) as archive:
            return {name.rstrip("/").split("/", 1)[-1]: archive.read(name)
                    for name in archive.namelist()
                    if not name.endswith("/") and "/" in name}
    if path.name.endswith((".tar.gz", ".tgz", ".tar.xz", ".tar")):
        if path.name.endswith((".tar.gz", ".tgz")):
            mode = "r:gz"
        elif path.name.endswith(".tar.xz"):
            mode = "r:xz"
        else:
            mode = "r:"
        with tarfile.open(path, mode) as archive:
            return {member.name.split("/", 1)[-1]: archive.extractfile(member).read()
                    for member in archive.getmembers()
                    if member.isfile() and "/" in member.name}
    raise ValueError(f"unsupported package input: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, type=pathlib.Path)
    parser.add_argument("--platform", required=True, choices=sorted(RUNTIME_NAMES))
    args = parser.parse_args()
    try:
        files = read_input(args.input)
        errors = validate(files, args.platform)
    except (OSError, ValueError, tarfile.TarError, zipfile.BadZipFile) as error:
        print(f"PACKAGE_READ_ERROR: {error}", file=sys.stderr)
        return 2
    if errors:
        for error in errors:
            print(f"PACKAGE_INVALID: {error}", file=sys.stderr)
        return 1
    print(f"librashader package payload valid: {args.input}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

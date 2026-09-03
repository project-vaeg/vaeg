#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import hashlib
import pathlib
import tarfile
import zipfile


EXPECTED_SHA256 = {
    "shaders/crt/vaeg_crt_lottes_fast.slangp":
        "8489d84dce043c5f68b7b6c7b7e3721bcad82b9b3498a8320f00fba7d6135835",
    "shaders/crt/crt-lottes-fast.slang":
        "576eddc662ac4f77909c0c14dbd5a16ac4164e50c67527fff634316f4441c482",
}
DENIED_MARKERS = ("crt-geom", "crt_royale", "crt-royale", "mega-bezel",
                  "mega_bezel")
REQUIRED_FILES = (
    "licenses/LIBRASHADER_MPL-2.0.txt",
    "licenses/LIBRASHADER_SOURCE.txt",
    "licenses/THIRD_PARTY_NOTICES.md",
    "shaders/crt/licenses/CRT_LOTTES_FAST_LICENSE.txt",
    "shaders/crt/licenses/CRT_LOTTES_FAST_PROVENANCE.md",
    *EXPECTED_SHA256,
)


def canonical(name):
    parts = pathlib.PurePosixPath(name).parts
    if parts and parts[0] not in (".", ".."):
        return "/".join(parts[1:]) if len(parts) > 1 else parts[0]
    return "/".join(parts)


def read_package(path):
    if path.is_dir():
        files = {}
        for child in path.rglob("*"):
            if child.is_file():
                files[child.relative_to(path).as_posix()] = child.read_bytes()
        return files
    if tarfile.is_tarfile(path):
        with tarfile.open(path, "r:*") as archive:
            return {
                canonical(member.name): archive.extractfile(member).read()
                for member in archive.getmembers()
                if member.isfile()
            }
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            return {
                canonical(name): archive.read(name)
                for name in archive.namelist()
                if not name.endswith("/")
            }
    raise ValueError(f"unsupported package: {path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("package", type=pathlib.Path)
    parser.add_argument("--platform", choices=("linux", "macos", "windows"),
                        required=True)
    args = parser.parse_args()

    files = read_package(args.package)
    missing = [name for name in REQUIRED_FILES if name not in files]
    if missing:
        raise SystemExit("missing package files: " + ", ".join(missing))

    runtime_name = {
        "linux": "librashader.so",
        "macos": "librashader.dylib",
        "windows": "librashader.dll",
    }[args.platform]
    runtimes = sorted(name for name in files
                      if pathlib.PurePosixPath(name).name.startswith("librashader."))
    if runtimes != [runtime_name]:
        raise SystemExit(f"unexpected librashader runtimes: {runtimes}")

    for name, expected in EXPECTED_SHA256.items():
        actual = hashlib.sha256(files[name]).hexdigest()
        if actual != expected:
            raise SystemExit(f"unexpected shader digest for {name}: {actual}")
    for name, data in files.items():
        lower_name = name.lower()
        if any(marker in lower_name for marker in DENIED_MARKERS):
            raise SystemExit(f"denied shader path: {name}")
        if pathlib.PurePosixPath(name).suffix.lower() in (".slang", ".slangp"):
            lower_text = data.decode("utf-8").lower()
            if "gpl" in lower_text:
                raise SystemExit(f"GPL marker in shader: {name}")
    print(f"librashader package inspection passed: {args.package}")


if __name__ == "__main__":
    main()

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

"""Audit source and runtime archives for forbidden migration artifacts."""

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import tarfile
from typing import List, Tuple
import zipfile


PROHIBITED_NAMES = {
    "zexdoc.cim",
    "zexall.cim",
    "zexdoc.src",
    "zexall.src",
    "zex-license.txt",
}
PROHIBITED_HASHES = {
    "b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90",
    "fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0",
    "0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3",
    "a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c",
    "e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052",
}
DELETED_LEGACY_PATHS = {
    "cpucva/types.h",
    "cpucva/z80.h",
    "cpucva/z80if.h",
    "cpucva/z80c.h",
    "cpucva/z80c.cpp",
    "cpucva/z80diag.h",
    "cpucva/z80diag.cpp",
}
DELETED_LEGACY_HASHES = {
    "aff5219486b4264bf9e75c8994a1e6714cb3a93f59136f8e8cf19bd2557973e7",
    "eb8e12d76dc824ea993eab7a238aae5cc1ce9b2d7458188a033db93037e1a073",
    "28e187571b5da870c91ee8511b2f2c4de624c42cbe087ab617bb084b368b747d",
    "4e84d24bb22f41de50c73815547e6bdca245d6c51cbbe0eee32f7d76fdaaec5d",
    "34eb3b6accb856efee356b3cc6ac61fb24847874df4f31475db511ae638b30d8",
    "f6711f349f5c5febe05d23c47ee38c6314c46ba4b2d42894493f5d77a53298b1",
    "b4db3d4c5e4466fce62326c445b0ea86955335945e1e406afb09960cd75385ef",
}
PRIVATE_ASSET_SUFFIXES = {
    ".d88", ".d77", ".fdi", ".xdf", ".hdm", ".hdi", ".thd",
    ".nhd", ".rom", ".sav",
}
APPROVED_EXTERNAL_ROOTS = {
    "imgui", "librashader", "softfloat", "suzukiplan-z80", "ymfm", "zex"
}
# ADR-0015: the source archive may carry exactly these upstream ZEX files at
# exactly these paths. Release archives never may.
SOURCE_ARCHIVE_ZEX_FILES = {
    "external/zex/zexdoc.src":
        "0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3",
    "external/zex/zexall.src":
        "a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c",
    "external/zex/LICENSE.txt":
        "e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052",
}
MODE_RELEASE = "release"
MODE_SOURCE = "source"
APPROVED_PATCHES = {
    "docs/agents/reports/m35_suzukiplan_irq_extension.patch",
    "docs/agents/reports/m101_suzukiplan_adc_sbc_carry.patch",
    "docs/agents/reports/m101_suzukiplan_adc_sbc_test.patch",
    "docs/agents/reports/m101_suzukiplan_flag_profile.patch",
    "docs/agents/reports/m101_suzukiplan_daa_cpl_scf_ccf.patch",
    "docs/agents/reports/m102_suzukiplan_undefined_opcodes.patch",
    "docs/agents/reports/m102_suzukiplan_ddcb_register_forms.patch",
    "docs/agents/reports/m102_upstream_upd9002_daa_cpl_scf_ccf.patch",
    "docs/agents/reports/m102_upstream_upd9002_ddcb_register_forms.patch",
    "docs/agents/reports/m102_upstream_undefined_opcodes.patch",
    "docs/agents/reports/m102_m35_irq_extension_on_1_11.patch",
    "docs/agents/reports/m102_upd9002_undefined_ed_semantics.patch",
    "tools/cpmva/patches/cpm22-64k.patch",
    "tools/cpmva/zex/zexall-fileout.patch",
    "tools/cpmva/zex/zex13s.patch",
    "tools/cpmva/zex/zexed.patch",
    "tools/cpmva/zex/zexiy.patch",
    "tools/cpmva/zex/zexund.patch",
    "tools/cpmva/zex/zexdoc-fileout.patch",
}


def is_recorded_source_zex(normalized: str, digest: str) -> bool:
    for recorded_path, recorded_digest in SOURCE_ARCHIVE_ZEX_FILES.items():
        if normalized == recorded_path or normalized.endswith("/" + recorded_path):
            return digest == recorded_digest
    return False


def inspect_member(
    name: str, data: bytes, violations: List[str], mode: str = MODE_RELEASE
) -> None:
    normalized = name.replace("\\", "/").lstrip("./")
    path = PurePosixPath(normalized)
    basename = path.name.lower()
    digest = hashlib.sha256(data).hexdigest()
    recorded_zex = mode == MODE_SOURCE and is_recorded_source_zex(normalized, digest)
    if basename in PROHIBITED_NAMES and not recorded_zex:
        violations.append(f"ZEX_NAME prohibited ZEX artifact name: {name}")
    if digest in PROHIBITED_HASHES and not recorded_zex:
        violations.append(f"ZEX_CONTENT prohibited ZEX artifact content: {name} ({digest})")
    for legacy_path in DELETED_LEGACY_PATHS:
        if normalized == legacy_path or normalized.endswith("/" + legacy_path):
            violations.append(f"LEGACY_PATH deleted legacy Z80 path: {name}")
    if digest in DELETED_LEGACY_HASHES:
        violations.append(f"LEGACY_CONTENT copied legacy Z80 content: {name} ({digest})")
    if path.suffix.lower() in PRIVATE_ASSET_SUFFIXES:
        violations.append(f"PRIVATE_MEDIA private-media filename class: {name}")
    parts = path.parts
    if "external" in parts:
        index = parts.index("external")
        if index + 1 >= len(parts) or parts[index + 1] not in APPROVED_EXTERNAL_ROOTS:
            violations.append(f"EXTERNAL_ROOT unrecorded external source root: {name}")
    if basename.endswith((".orig", ".rej")):
        violations.append(f"PATCH_WORK patch-work artifact: {name}")
    if basename.endswith(".patch") and not any(
        normalized == approved_patch
        or normalized.endswith("/" + approved_patch)
        for approved_patch in APPROVED_PATCHES
    ):
        violations.append(f"PATCH_UNAPPROVED unapproved patch artifact: {name}")


def inspect_tar(path: Path, mode: str = MODE_RELEASE) -> Tuple[int, List[str]]:
    count = 0
    violations: List[str] = []
    with tarfile.open(path, "r:*") as archive:
        for member in archive.getmembers():
            if not member.isfile():
                continue
            extracted = archive.extractfile(member)
            if extracted is None:
                continue
            inspect_member(member.name, extracted.read(), violations, mode)
            count += 1
    return count, violations


def inspect_zip(path: Path, mode: str = MODE_RELEASE) -> Tuple[int, List[str]]:
    count = 0
    violations: List[str] = []
    with zipfile.ZipFile(path) as archive:
        for member in archive.infolist():
            if member.is_dir():
                continue
            inspect_member(member.filename, archive.read(member), violations, mode)
            count += 1
    return count, violations


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Reject deleted legacy, fetched ZEX, and private artifacts"
    )
    parser.add_argument(
        "--mode",
        choices=(MODE_RELEASE, MODE_SOURCE),
        default=MODE_RELEASE,
        help="release (default) rejects all ZEX material; source accepts only "
        "the ADR-0015 recorded external/zex files",
    )
    parser.add_argument("archives", type=Path, nargs="+")
    args = parser.parse_args()

    failed = False
    for path in args.archives:
        try:
            if zipfile.is_zipfile(path):
                count, violations = inspect_zip(path, args.mode)
            elif tarfile.is_tarfile(path):
                count, violations = inspect_tar(path, args.mode)
            else:
                raise ValueError("not a supported tar or zip archive")
        except (OSError, ValueError, tarfile.TarError, zipfile.BadZipFile) as error:
            print(f"FAIL {path}: {error}")
            failed = True
            continue
        if violations:
            print(f"FAIL {path}:")
            for violation in violations:
                print(f"  {violation}")
            failed = True
        else:
            print(f"PASS {path} ({args.mode}): checked {count} files; no forbidden artifacts")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())

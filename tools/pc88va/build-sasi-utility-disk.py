#!/usr/bin/env python3
"""Build a boot-layout 40 MB PC-88VA SASI HDI development disk.

The input is a PC-Engine 1.05 or 1.1 system D88.  The generated HDI uses the
40 MB SASI geometry implemented by fdd/sxsi.c and places the four boot system
files in the conventional first clusters.  Every COM and EXE found anywhere
on the source D88 is copied into ``\\BIN`` (except the boot ``PCENGINE.COM``,
which remains at the root).

When a complete development D88 is supplied with ``--payload-d88``, its
regular files and directories are transplanted as well, the matching source
disk's COM/EXE files are added to ``\\BIN``, and the documented
CONFIG.SYS/AUTOEXEC.BAT are regenerated.  An LSI C-86 trial archive can be
supplied with ``--lsic-archive`` and its extracted tree with ``--lsic-tree``;
the archive is retained under ``\\ARCHIVE`` and the runnable compiler tree is
installed under ``\\LSIC86``.  Without that option the builder creates the
small system-plus-BIN image used for layout tests.  A verified CP/M program
EXEcutor archive and the preserved CP/M tools/source disks (plus an optional
development disk) can be supplied with the ``--cpm-*`` options; they are
installed under ``\\CPM\\BIN``, ``\\CPM\\TOOLS``,
``\\CPM\\SOURCE``, and ``\\CPM\\DEV`` and the emulator directory is added to
``PATH``.

The optional ``--supplemental-tree`` is the normalized output of
``stage-development-tools.sh``.  Its files are merged into the image and an
optional ``--supplemental-manifest`` is checked before allocation.  The full
SASI profile includes the UNIX-like tools below ``\\UNIX`` and appends that
directory to ``PATH``.

The optional PC-88VA SCSI/MO reference packages from PC88.gr.jp can be
supplied with the ``--mo-*`` options.  SCHD 1.55T is installed under
``\\SYS`` and its manuals under ``\\DOC``; the VA128MO and STEST manuals are
kept with the other documentation, while the STEST utilities are installed
under ``\\BIN``.  VAEG does not support MO/removable-media operation; these
packages are retained for hardware reference only.  The original archives
are retained under ``\\ARCHIVE``.  Package bytes are checksum-verified and
the builder never formats or modifies removable media.

The generated ``CONFIG.SYS`` uses SCSI target ID 0 and the fixed-160MB policy
by default.  ``--scsi-id`` selects another target, while ``--scsi-profile``
can select ``fixed``, ``mo-128mb``, or ``mo-160mb``.  MO profiles add the
documented ``-X -D1`` SCHD policy and install a manual setup note; VBUFF,
SCFORM, SFORM, and SETDMA are never run by the builder.  ``--docs-lang``
selects the installed operator-note language (``en``, ``ja``, or ``both``;
the default is ``both``).

The optional free JWasm DOS package can be supplied with
``--jwasm-archive``.  Its real-mode MASM-compatible assembler
(``JWASMR.EXE``), readme, and license are installed under ``\\BIN`` and
``\\DOC``; the original archive is retained under ``\\ARCHIVE``.

This is a host-side image builder.  It does not require a running emulator or
DOSBox; the generated image contains the source PC-Engine IPL and the FAT12
layout used by the PC-88VA SASI formatter.  Real-machine boot validation
remains a separate gate.
"""

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

from __future__ import annotations

import argparse
import hashlib
import io
import importlib.util
import os
import re
import struct
import tempfile
import zipfile
from datetime import datetime
from pathlib import Path, PurePosixPath


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
PCENGINE_DISK_PATH = Path(__file__).with_name("pcengine_disk.py")
CPM_INSTALLER_PATH = REPOSITORY_ROOT / "tools" / "cpmva" / "install_cpmva.py"

HEADER_SIZE = 4096
SECTOR_SIZE = 256
SECTORS = 33
SURFACES = 8
CYLINDERS = 615
DATA_SIZE = SECTOR_SIZE * SECTORS * SURFACES * CYLINDERS
IMAGE_SIZE = HEADER_SIZE + DATA_SIZE

# HDFORM's 40 MB SASI layout, expressed in 256-byte physical blocks.
BOOT_BLOCKS = 4                 # one 1024-byte IPL sector
FAT_BLOCKS = 16                 # one 4096-byte FAT copy
# HDFORM reserves 96 KiB for the root directory (3,072 32-byte entries).
# The large root area is why the first data cluster begins at 01B400h in a
# formatted 40 MB SASI image.
ROOT_BLOCKS = 384
FAT1_OFFSET = HEADER_SIZE + BOOT_BLOCKS * SECTOR_SIZE
FAT2_OFFSET = FAT1_OFFSET + FAT_BLOCKS * SECTOR_SIZE
ROOT_OFFSET = FAT2_OFFSET + FAT_BLOCKS * SECTOR_SIZE
DATA_OFFSET = ROOT_OFFSET + ROOT_BLOCKS * SECTOR_SIZE
CLUSTER_SIZE = 16 * 1024
CLUSTER_COUNT = (IMAGE_SIZE - DATA_OFFSET) // CLUSTER_SIZE
ROOT_ENTRIES = (ROOT_BLOCKS * SECTOR_SIZE) // 32
FAT_SIZE = FAT_BLOCKS * SECTOR_SIZE

SYSTEM_FILES = ("ENGINEIO.SYS", "PCENGINE.SYS", "ADVGBIOS.SYS", "PCENGINE.COM")
SYSTEM_ATTRIBUTES = {
    "ENGINEIO.SYS": 0x27,
    "PCENGINE.SYS": 0x27,
    "ADVGBIOS.SYS": 0x27,
    "PCENGINE.COM": 0x21,
}

CONFIG_LINES = (
    "FILES   = 20",
    "BUFFERS = 30",
    r"DEVICE = A:\SYS\EMMVA01.SYS",
    r"DEVICE = A:\SYS\SQEMM98.SYS",
    r"DEVICE = A:\SYS\EMMVA02.SYS",
    r"DEVICE = A:\SYS\PCPLUS.SYS",
    r"DEVICE = A:\SYS\BMSDRVA.SYS /P",
    r"DEVICE = A:\SYS\SCHD.SYS -I0",
    r"DEVICE = A:\SYS\HOSTFAT.SYS",
    r"DEVICE = A:\SYS\PCEPAT.SYS",
    r"DEVICE = A:\SYS\JFPPAT.SYS",
    r"DEVICE = A:\SYS\RESET.SYS",
    r"DEVICE = A:\SYS\TSCLVA.SYS",
    r"DEVICE = A:\SYS\MSE352B.COM /A /B",
    r"DEVICE = A:\SYS\RDBMS.SYS -P1D0 -S2",
    r"DEVICE = A:\SYS\RDEMS.SYS -P128 -A",
    r"DEVICE = A:\SYS\RDPCM.SYS",
)

SCSI_PROFILES = ("fixed", "fixed-160mb", "mo-128mb", "mo-160mb")
SCSI_DOC_LANGUAGES = ("en", "ja", "both")


def validate_scsi_profile(scsi_id: int, scsi_profile: str) -> None:
    """Validate the target selector used by the generated guest config."""
    if not isinstance(scsi_id, int) or not 0 <= scsi_id <= 7:
        raise BuildError("SCSI ID must be an integer from 0 through 7")
    if scsi_profile not in SCSI_PROFILES:
        raise BuildError(
            "SCSI profile must be one of: " + ", ".join(SCSI_PROFILES))


def validate_scsi_doc_language(language: str) -> None:
    """Validate the language set selected for installed setup notes."""
    if language not in SCSI_DOC_LANGUAGES:
        raise BuildError(
            "SCSI document language must be one of: "
            + ", ".join(SCSI_DOC_LANGUAGES))


def scsi_setup_lines(scsi_id: int, scsi_profile: str,
                     language: str = "en") -> tuple[str, ...]:
    """Return non-destructive operator notes for an SCSI target profile."""
    validate_scsi_profile(scsi_id, scsi_profile)
    validate_scsi_doc_language(language)
    if scsi_profile == "fixed":
        return ()
    if scsi_profile == "fixed-160mb":
        medium = "160MB FIXED SCSI TARGET"
        removable = "NO"
        schd = f"DEVICE = A:\\SYS\\SCHD.SYS -I{scsi_id}"
        vbuff = "VBUFF -D1 -B12"
        caution = "160MB fixed-target support still requires real hardware validation."
        caution_ja = "160MB固定SCSIターゲットは実機検証が必要です。"
    elif scsi_profile == "mo-128mb":
        medium = "128MB REMOVABLE MO"
        removable = "YES"
        schd = f"DEVICE = A:\\SYS\\SCHD.SYS -I{scsi_id} -X -D1"
        vbuff = "VBUFF -D1 -B11"
        caution = "MO/removable-media operation is unsupported in VAEG; hardware-only reference."
        caution_ja = "MOやリムーバブル媒体はVAEG未サポートです。実機用の参考資料です。"
    else:
        medium = "160MB REMOVABLE MO"
        removable = "YES"
        schd = f"DEVICE = A:\\SYS\\SCHD.SYS -I{scsi_id} -X -D1"
        vbuff = "VBUFF -D1 -B12"
        caution = "MO/removable-media operation is unsupported in VAEG; hardware-only reference."
        caution_ja = "MOやリムーバブル媒体はVAEG未サポートです。実機用の参考資料です。"
    scform = ("SCFORM /SS"
              if scsi_profile in ("fixed-160mb", "mo-160mb")
              else "SCFORM /S")
    if language == "ja":
        return (
            "PC-88VA SCSI ターゲット設定",
            f"プロファイル: {medium}",
            f"SCSI ID: {scsi_id}",
            f"リムーバブル媒体: {removable}",
            ("サポート状態: VAEGでは未サポート（実機用参考資料）"
             if removable == "YES" else "サポート状態: VAEG固定ディスク経路"),
            f"検証: {caution_ja}",
            "",
            "CONFIG.SYS の SCHD 行:",
            schd,
            "",
            "手動手順（自動実行しません）:",
            f"1. PC-Engine システムディスクで {vbuff} を実行",
            "2. VBUFF がIPLを書き換えたら再起動",
            f"3. 対象確認とバックアップ後に {scform} を実行",
            "4. MO媒体の操作はVAEG未サポート。実機でのみ手動実行",
            "5. SETDMA.COM は必要時に PCPLUS 読み込み後に手動実行",
            "",
            "生成されたSASIディスクは40MBの起動・サポートディスクです。",
            "この設定は外部SCSIターゲット用のゲスト環境を準備します。",
        )
    return (
        "PC-88VA SCSI TARGET PROFILE",
        f"PROFILE: {medium}",
        f"SCSI ID: {scsi_id}",
        f"REMOVABLE POLICY: {removable}",
        ("SUPPORT STATUS: UNSUPPORTED IN VAEG (HARDWARE REFERENCE ONLY)"
         if removable == "YES" else "SUPPORT STATUS: VAEG FIXED-DISK PATH"),
        f"VALIDATION: {caution}",
        "",
        "CONFIG.SYS SCHD LINE:",
        schd,
        "",
        "OPERATOR STEPS (NOT RUN AUTOMATICALLY):",
        f"1. On the PC-Engine system disk, run: {vbuff}",
        "2. Reboot after VBUFF changes the IPL buffer size.",
        f"3. Run {scform} only after confirming the target and backing it up.",
        "4. MO operation is unsupported in VAEG; use SFORM only on validated hardware.",
        "5. SETDMA.COM is optional; run it manually after PCPLUS is loaded.",
        "",
        "The generated SASI disk is still a 40 MB boot/support disk.",
        "This profile prepares the guest software for an external SCSI target.",
    )


def config_lines(two_hc_enabled: bool = False, scsi_id: int = 0,
                 scsi_profile: str = "fixed-160mb") -> tuple[str, ...]:
    """Return the generated VA load order and selected SCHD target policy."""
    validate_scsi_profile(scsi_id, scsi_profile)
    lines = list(CONFIG_LINES)
    schd_index = lines.index(r"DEVICE = A:\SYS\SCHD.SYS -I0")
    schd_suffix = ""
    if scsi_profile in ("mo-128mb", "mo-160mb"):
        schd_suffix = " -X -D1"
    lines[schd_index] = (
        rf"DEVICE = A:\SYS\SCHD.SYS -I{scsi_id}{schd_suffix}")
    if two_hc_enabled:
        index = lines.index(r"DEVICE = A:\SYS\MSE352B.COM /A /B")
        lines.insert(index, r"DEVICE = A:\BIN\2HCDRV.COM")
    return tuple(lines)
AUTOEXEC_LINES = (
    r"PATH A:\BIN",
    r"SET TEEN=A:\BIN\TEEN.DEF",
    r"SET TMP=A:\TMP",
    r"SET COMSPEC=A:\PCENGINE.COM",
    r"A:\BIN\CLK.COM",
)
LSIC_AUTOEXEC_LINES = (
    r"PATH A:\BIN;A:\LSIC86\BIN",
    r"SET LSIC86=A:\LSIC86",
    r"SET INCLUDE=A:\LSIC86\INCLUDE",
    r"SET LIB=A:\LSIC86\LIB",
)


def autoexec_lines(lsic_enabled: bool, cpm_enabled: bool,
                   unix_tools_enabled: bool) -> tuple[str, ...]:
    path_entries = [r"A:\BIN"]
    if lsic_enabled:
        path_entries.append(r"A:\LSIC86\BIN")
    if cpm_enabled:
        path_entries.append(r"A:\CPM\BIN")
    if unix_tools_enabled:
        path_entries.append(r"A:\UNIX\BIN")
    lines = ["PATH " + ";".join(path_entries)]
    if lsic_enabled:
        lines.extend(LSIC_AUTOEXEC_LINES[1:])
    if cpm_enabled:
        lines.append(r"SET CPM=A:\CPM")
    lines.extend(AUTOEXEC_LINES[1:])
    return tuple(lines)


def text_file(lines: tuple[str, ...], encoding: str = "ascii") -> bytes:
    return ("\r\n".join(lines) + "\r\n").encode(encoding)


class BuildError(Exception):
    """A deterministic input or layout error."""


def load_pcengine_module():
    spec = importlib.util.spec_from_file_location(
        "pc88va_sasi_pcengine_disk", PCENGINE_DISK_PATH)
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {PCENGINE_DISK_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_cpm_module():
    """Load the repository CP/M D88 reader without invoking its CLI."""
    spec = importlib.util.spec_from_file_location(
        "pc88va_sasi_cpm_installer", CPM_INSTALLER_PATH)
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {CPM_INSTALLER_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def d88_file(disk, components: list[str], module) -> bytes:
    directory = disk.root
    for index, component in enumerate(components):
        offset, exists = module.find_entry(directory, module.short_name(component))
        if not exists:
            raise BuildError("source D88 is missing " + "/".join(components))
        entry = bytes(directory[offset:offset + 32])
        cluster = struct.unpack_from("<H", entry, 26)[0]
        if index + 1 < len(components):
            if not entry[11] & 0x10:
                raise BuildError("D88 path component is not a directory: " + component)
            directory_bytes = bytearray()
            for item in disk.cluster_chain(cluster):
                directory_bytes.extend(disk.read_cluster(item))
            directory = directory_bytes
            continue
        if entry[11] & 0x10:
            raise BuildError("D88 path names a directory: " + "/".join(components))
        size = struct.unpack_from("<I", entry, 28)[0]
        payload = bytearray()
        for item in disk.cluster_chain(cluster):
            payload.extend(disk.read_cluster(item))
        return bytes(payload[:size])
    raise BuildError("D88 path does not name a file")


def sorted_directory_entries(directory, module):
    """Return active DOS directory entries in deterministic name order."""
    return sorted(
        module.iter_entries(directory),
        key=lambda item: (module.display_name(item[1][:11]).upper(), item[0]))


def collect_executables(disk, module, directory=None, prefix=()):
    if directory is None:
        directory = disk.root
    files = []
    for _, entry in sorted_directory_entries(directory, module):
        raw_name = entry[:11]
        if entry[11] & 0x08:
            continue
        name = module.display_name(raw_name)
        if name in (".", ".."):
            continue
        path = prefix + (name,)
        if entry[11] & 0x10:
            cluster = struct.unpack_from("<H", entry, 26)[0]
            contents = bytearray()
            for item in disk.cluster_chain(cluster):
                contents.extend(disk.read_cluster(item))
            files.extend(collect_executables(disk, module, contents, path))
            continue
        if name.upper().endswith((".COM", ".EXE")):
            files.append((path, d88_file(disk, list(path), module)))
    return files


def collect_payload_files(disk, module, directory=None, prefix=()):
    """Return all regular files and directories from a PC-Engine D88.

    The result is intentionally independent of the HDI allocator.  Each
    record contains a relative 8.3 path, file bytes, and the source FAT
    attributes so a complete development D88 can be transplanted into the
    larger SASI filesystem without preserving its cluster numbers.
    """
    if directory is None:
        directory = disk.root
    files = []
    directories = []
    for _, entry in sorted_directory_entries(directory, module):
        if entry[11] & 0x08:
            continue
        name = module.display_name(entry[:11])
        if name in (".", ".."):
            continue
        path = prefix + (name,)
        cluster = struct.unpack_from("<H", entry, 26)[0]
        if entry[11] & 0x10:
            contents = bytearray()
            for item in disk.cluster_chain(cluster):
                contents.extend(disk.read_cluster(item))
            child_files, child_directories = collect_payload_files(
                disk, module, contents, path)
            directories.append(path)
            files.extend(child_files)
            directories.extend(child_directories)
            continue
        size = struct.unpack_from("<I", entry, 28)[0]
        contents = bytearray()
        for item in disk.cluster_chain(cluster):
            contents.extend(disk.read_cluster(item))
        files.append((path, bytes(contents[:size]), entry[11]))
    return files, directories


def fat_set(fat: bytearray, cluster: int, value: int) -> None:
    offset = cluster + cluster // 2
    value &= 0xFFF
    if offset + 1 >= len(fat):
        raise BuildError(f"FAT cluster is outside the image: {cluster}")
    if cluster & 1:
        fat[offset] = (fat[offset] & 0x0F) | ((value & 0x0F) << 4)
        fat[offset + 1] = (value >> 4) & 0xFF
    else:
        fat[offset] = value & 0xFF
        fat[offset + 1] = (fat[offset + 1] & 0xF0) | ((value >> 8) & 0x0F)


def fat_get(fat: bytes, cluster: int) -> int:
    offset = cluster + cluster // 2
    pair = fat[offset] | (fat[offset + 1] << 8)
    if cluster & 1:
        pair >>= 4
    return pair & 0xFFF


def new_directory_tree() -> dict[str, object]:
    """Return the mutable representation used by HdiBuilder directory trees."""
    return {"files": [], "directories": {}}


def ensure_directory(tree: dict[str, object], path: tuple[str, ...]) -> dict[str, object]:
    """Create path's directory nodes and return the final node."""
    node = tree
    for component in path:
        directories = node["directories"]
        key = component.upper()
        child = directories.get(key)
        if child is None:
            child = {"name": component, "tree": new_directory_tree()}
            directories[key] = child
        node = child["tree"]
    return node


def add_tree_file(tree: dict[str, object], path: tuple[str, ...],
                  contents: bytes, attributes: int) -> None:
    """Insert one regular file into a directory tree, rejecting collisions."""
    if len(path) < 2:
        raise BuildError("directory tree file must have a parent directory")
    node = ensure_directory(tree, path[:-1])
    files = node["files"]
    filename = path[-1]
    if any(item[0].upper() == filename.upper() for item in files):
        raise BuildError("duplicate directory entry: " + "/".join(path))
    files.append((filename, contents, attributes))


def upsert_tree_file(tree: dict[str, object], path: tuple[str, ...],
                     contents: bytes, attributes: int) -> None:
    """Install a file while allowing an identical payload entry to be reused.

    A complete development D88 already contains SCHD's driver and manuals.
    Package installation verifies those bytes rather than creating duplicate
    DOS directory entries.  A different file at the same path is a
    deterministic error so stale payload data cannot be silently replaced.
    """
    if len(path) < 2:
        raise BuildError("directory tree file must have a parent directory")
    node = ensure_directory(tree, path[:-1])
    files = node["files"]
    filename = path[-1]
    for index, item in enumerate(files):
        if item[0].upper() != filename.upper():
            continue
        if item[1] != contents:
            raise BuildError("package file conflicts with payload: " +
                             "/".join(path))
        files[index] = (item[0], contents, attributes)
        return
    files.append((filename, contents, attributes))


def replace_tree_file(tree: dict[str, object], path: tuple[str, ...],
                      contents: bytes, attributes: int) -> None:
    """Install an authoritative staged file, replacing an older copy."""
    if len(path) < 2:
        raise BuildError("directory tree file must have a parent directory")
    node = ensure_directory(tree, path[:-1])
    files = node["files"]
    filename = path[-1]
    for index, item in enumerate(files):
        if item[0].upper() == filename.upper():
            files[index] = (item[0], contents, attributes)
            return
    files.append((filename, contents, attributes))


def collect_host_tree(root: Path, module) -> list[tuple[tuple[str, ...], bytes]]:
    """Collect a checked, relative tree extracted from a host archive."""
    if not root.is_dir():
        raise BuildError(f"extracted package tree is not a directory: {root}")
    records = []
    for path in sorted(root.rglob("*"), key=lambda item: item.as_posix().upper()):
        if path.is_symlink() or not path.is_file():
            continue
        relative = path.relative_to(root)
        components = tuple(part.upper() for part in relative.parts)
        if not components or any(component in (".", "..") for component in components):
            raise BuildError("invalid extracted package path: " + str(relative))
        for component in components:
            try:
                module.short_name(component)
            except module.DiskError as error:
                raise BuildError(
                    f"invalid 8.3 path component {component}: {relative}") from error
        records.append((components, path.read_bytes()))
    if not records:
        raise BuildError("extracted package tree is empty")
    return records


def validate_stage_manifest(manifest: Path, records: list[tuple[tuple[str, ...], bytes]],
                            profile: str) -> None:
    """Verify that a normalized staging tree matches its producer manifest."""
    try:
        lines = manifest.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise BuildError(f"cannot read staging manifest: {manifest}") from error
    expected = {}
    for line_number, line in enumerate(lines, 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) != 4:
            raise BuildError(
                f"invalid staging manifest line {line_number}: {manifest}")
        observed_profile, relative, digest, size_text = fields
        if observed_profile != profile:
            raise BuildError(
                f"staging manifest profile mismatch on line {line_number}: "
                f"expected {profile}, got {observed_profile}")
        try:
            size = int(size_text, 10)
        except ValueError as error:
            raise BuildError(
                f"invalid staging manifest size on line {line_number}: {manifest}") from error
        if not relative or relative.upper() in expected:
            raise BuildError(
                f"duplicate staging manifest path on line {line_number}: {relative}")
        if len(digest) != 64 or any(character not in "0123456789abcdef" for character in digest.lower()):
            raise BuildError(
                f"invalid staging manifest SHA-256 on line {line_number}: {relative}")
        expected[relative.upper()] = (digest.lower(), size)
    actual = {}
    for path, contents in records:
        relative = "/".join(path).upper()
        actual[relative] = (hashlib.sha256(contents).hexdigest(), len(contents))
    if expected != actual:
        missing = sorted(set(expected) - set(actual))
        extra = sorted(set(actual) - set(expected))
        changed = sorted(path for path in set(expected) & set(actual)
                         if expected[path] != actual[path])
        details = []
        if missing:
            details.append("missing=" + ",".join(missing[:4]))
        if extra:
            details.append("extra=" + ",".join(extra[:4]))
        if changed:
            details.append("changed=" + ",".join(changed[:4]))
        raise BuildError("staging manifest does not match tree (" +
                         "; ".join(details) + ")")


MO_PACKAGE_SPECS = {
    "schd": {
        "archive_name": "SCHD155T.LZH",
        "sha256": "87aebcf7c9bc9c6170a40d0e6ddcce5afdcbb1fa55f1fdeeec815458f7ef065f",
        "files": {
            "SCHD.SYS": ("SYS",),
            "SCHD.DOC": ("DOC",),
            "SCHD.LOG": ("DOC",),
            "SCHD.TXT": ("DOC",),
        },
    },
    "va128mo": {
        "archive_name": "VA128MO.LZH",
        "sha256": "1dc8f366fb56e1761051e9b0c1e8950999ebb6df10ddf1bb91251e2557728a36",
        "files": {"VA128MO.DOC": ("DOC",)},
    },
    "stest": {
        "archive_name": "STEST115.LZH",
        "sha256": "6ae981b0010df20a510f85165567add33032241854b147ed47937a59953010bc",
        "files": {
            "STEST.EXE": ("BIN",),
            "STESTX.COM": ("BIN",),
            "STEST.BAT": ("BIN",),
            "STEST115.DOC": ("DOC",),
            "COMMAND.DOC": ("DOC",),
            "UTILITY.DOC": ("DOC",),
        },
    },
}


def collect_mo_package(archive: Path, tree: Path, module, package: str):
    """Verify one PC-88VA MO package and map files to the HDI tree."""
    spec = MO_PACKAGE_SPECS[package]
    try:
        archive_contents = archive.read_bytes()
    except OSError as error:
        raise BuildError(f"cannot read {package} archive: {archive}") from error
    observed_hash = hashlib.sha256(archive_contents).hexdigest()
    if observed_hash != spec["sha256"]:
        raise BuildError(
            f"{package} archive SHA-256 mismatch: {archive} ({observed_hash})")
    records = collect_host_tree(tree, module)
    by_name = {}
    for relative, contents in records:
        if len(relative) != 1:
            raise BuildError(f"unexpected nested {package} package path: " +
                             "/".join(relative))
        name = relative[0].upper()
        if name in by_name:
            raise BuildError(f"duplicate {package} package file: {name}")
        by_name[name] = contents
    expected = spec["files"]
    missing = sorted(set(expected) - set(by_name))
    unexpected = sorted(set(by_name) - set(expected))
    if missing:
        raise BuildError(f"{package} package is missing: {', '.join(missing)}")
    if unexpected:
        raise BuildError(
            f"{package} package contains unexpected files: {', '.join(unexpected)}")
    mapped = []
    for name, prefix in expected.items():
        mapped.append((prefix + (name,), by_name[name]))
    return archive_contents, mapped


def install_mo_packages(tree: dict[str, object], packages, installed_files) -> None:
    """Add verified MO packages and their archives to a mutable DOS tree."""
    def record(path, contents):
        if not any(item[0] == path for item in installed_files):
            installed_files.append((path, contents))

    for archive_name, archive_contents, records in packages:
        archive_path = ("ARCHIVE", archive_name)
        upsert_tree_file(tree, archive_path, archive_contents, 0x20)
        record(archive_path, archive_contents)
        for path, contents in records:
            attributes = 0x27 if path == ("SYS", "SCHD.SYS") else 0x20
            upsert_tree_file(tree, path, contents, attributes)
            record(path, contents)


def install_scsi_profile_doc(tree: dict[str, object], installed_files,
                             scsi_id: int, scsi_profile: str,
                             docs_lang: str = "both") -> None:
    """Install selected manual-only SCSI/MO setup notes."""
    if scsi_profile == "fixed":
        return
    validate_scsi_doc_language(docs_lang)
    basename = {
        "fixed-160mb": "SCSI160.TXT",
        "mo-128mb": "MO128.TXT",
        "mo-160mb": "MO160.TXT",
    }[scsi_profile]
    stem = basename[:-4]
    languages = ("en", "ja") if docs_lang == "both" else (docs_lang,)
    for language in languages:
        suffix = "E" if language == "en" else "J"
        path = ("DOC", f"{stem}{suffix}.TXT")
        encoding = "ascii" if language == "en" else "cp932"
        contents = text_file(
            scsi_setup_lines(scsi_id, scsi_profile, language), encoding)
        upsert_tree_file(tree, path, contents, 0x20)
        if not any(item[0] == path for item in installed_files):
            installed_files.append((path, contents))


def collect_jwasm_package(archive_contents: bytes, module):
    """Extract the real-mode JWasm tool and its provenance documents."""
    try:
        archive = zipfile.ZipFile(io.BytesIO(archive_contents))
    except (OSError, zipfile.BadZipFile) as error:
        raise BuildError("cannot open JWasm DOS package") from error
    with archive:
        by_name = {}
        for member in archive.infolist():
            if member.is_dir():
                continue
            parts = PurePosixPath(member.filename).parts
            if len(parts) != 1:
                continue
            name = parts[0].upper()
            if name in {"JWASMR.EXE", "README.TXT", "LICENSE.TXT"}:
                if name in by_name:
                    raise BuildError(f"duplicate JWasm package file: {name}")
                by_name[name] = archive.read(member)
    expected = {
        "JWASMR.EXE": ("BIN",),
        "README.TXT": ("DOC",),
        "LICENSE.TXT": ("DOC",),
    }
    missing = sorted(set(expected) - set(by_name))
    if missing:
        raise BuildError("JWasm package is missing: " + ", ".join(missing))
    records = []
    for name, prefix in expected.items():
        output_name = {
            "JWASMR.EXE": "JWASMR.EXE",
            "README.TXT": "JWASM.TXT",
            "LICENSE.TXT": "JWASM.LIC",
        }[name]
        try:
            module.short_name(output_name)
        except module.DiskError as error:
            raise BuildError(f"invalid JWasm output name: {output_name}") from error
        records.append((prefix + (output_name,), by_name[name]))
    return records


def parse_legacy_cpm_raw(raw: bytes, module) -> dict[str, bytes]:
    """Read the older CP/MVA one-16-KiB-extent-per-entry images.

    The current CP/MVA writer uses EXM=1 and is parsed by ``parse_cpm_raw``.
    The preserved source/development disks predate that writer and encode each
    16 KiB extent in its own directory entry.  They are read-only inputs here,
    so accepting that historical layout lets the builder preserve their files
    without changing the CP/M disk generator or emulator.
    """
    if len(raw) != module.CPM_RAW_SIZE:
        raise BuildError("CP/M raw image is not exactly 327680 bytes")
    files: dict[str, list[tuple[int, bytes]]] = {}
    allocated: set[int] = set()
    for index in range(module.CPM_DIRECTORY_ENTRIES):
        begin = module.CPM_DIRECTORY_OFFSET + index * 32
        entry = raw[begin:begin + 32]
        if entry[0] == 0xE5:
            continue
        if entry[0] != 0:
            raise BuildError("CP/M legacy disk uses an unsupported user area")
        if entry[13] != 0 or entry[14] != 0:
            raise BuildError("CP/M legacy disk has invalid extent fields")
        name = entry[1:9].decode("ascii").rstrip()
        extension = entry[9:12].decode("ascii").rstrip()
        display = name + (f".{extension}" if extension else "")
        extent = entry[12]
        records = entry[15]
        if records > module.CPM_RECORDS_PER_SUBEXTENT:
            raise BuildError(f"CP/M legacy extent is too large: {display}")
        block_count = (records * module.CPM_RECORD_SIZE +
                       module.CPM_BLOCK_SIZE - 1) // module.CPM_BLOCK_SIZE
        if block_count > 8:
            raise BuildError(f"CP/M legacy extent has too many blocks: {display}")
        blocks = list(entry[16:16 + block_count])
        if any(block < 2 or block > module.CPM_MAX_BLOCK for block in blocks):
            raise BuildError(f"CP/M legacy extent references an invalid block: {display}")
        if allocated.intersection(blocks):
            raise BuildError(f"CP/M legacy extent reuses a block: {display}")
        allocated.update(blocks)
        content = bytearray()
        for block in blocks:
            offset = module.CPM_DIRECTORY_OFFSET + block * module.CPM_BLOCK_SIZE
            content.extend(raw[offset:offset + module.CPM_BLOCK_SIZE])
        content = content[:records * module.CPM_RECORD_SIZE]
        entries = files.setdefault(display, [])
        if any(previous_extent == extent for previous_extent, _ in entries):
            raise BuildError(f"CP/M legacy disk repeats extent: {display}")
        entries.append((extent, bytes(content)))
    result = {}
    for name, extents in files.items():
        ordered = sorted(extents)
        for expected_extent, (extent, _) in enumerate(ordered):
            if extent != expected_extent:
                raise BuildError(f"CP/M legacy disk has an extent gap: {name}")
        result[name] = b"".join(content for _, content in ordered)
    if not result:
        raise BuildError("CP/M legacy disk has no files")
    return result


def collect_cpm_disk(path: Path, label: str, cpm_module) -> list[tuple[tuple[str, ...], bytes]]:
    """Extract all files from one CP/M D88 into a namespaced DOS tree."""
    try:
        raw = cpm_module.unwrap_cpm_d88(path.read_bytes())
    except (OSError, cpm_module.InstallerError) as error:
        raise BuildError(f"cannot read CP/M D88 {path}: {error}") from error
    try:
        files = cpm_module.parse_cpm_raw(raw)
    except cpm_module.InstallerError:
        files = parse_legacy_cpm_raw(raw, cpm_module)
    return [(('CPM', label, name), contents)
            for name, contents in sorted(files.items())]


def collect_cpm_archive(path: Path, module) -> list[tuple[tuple[str, ...], bytes]]:
    """Collect the CP/M emulator archive into its runtime/source/doc areas."""
    try:
        contents = path.read_bytes()
    except OSError as error:
        raise BuildError(f"cannot read CP/M emulator archive: {path}") from error
    if len(contents) != CPM_ARCHIVE_SIZE:
        raise BuildError(
            f"CP/M emulator archive size mismatch: {len(contents)} != {CPM_ARCHIVE_SIZE}")
    observed_hash = hashlib.sha256(contents).hexdigest()
    if observed_hash != CPM_ARCHIVE_SHA256:
        raise BuildError(
            f"CP/M emulator archive SHA-256 mismatch: {path} ({observed_hash})")
    records = []
    try:
        archive = zipfile.ZipFile(path)
    except (OSError, zipfile.BadZipFile) as error:
        raise BuildError(f"cannot open CP/M emulator archive: {path}") from error
    with archive:
        for member in sorted(archive.infolist(), key=lambda item: item.filename.upper()):
            if member.is_dir():
                continue
            relative = PurePosixPath(member.filename)
            if relative.is_absolute() or any(part in ("", ".", "..")
                                             for part in relative.parts):
                raise BuildError(f"invalid CP/M emulator archive path: {member.filename}")
            if len(relative.parts) != 1:
                raise BuildError(f"unexpected nested CP/M emulator path: {member.filename}")
            name = relative.parts[0].upper()
            try:
                module.short_name(name)
            except module.DiskError as error:
                raise BuildError(f"invalid CP/M emulator 8.3 name: {name}") from error
            if name in {"CPM.EXE", "XCCP.CPM"}:
                directory = "BIN"
            elif name.endswith((".ASM", ".MAC", ".MAK")):
                directory = "SRC"
            else:
                directory = "DOC"
            records.append((('CPM', directory, name), archive.read(member)))
    if not records:
        raise BuildError("CP/M emulator archive is empty")
    return records


def add_cpm_to_tree(tree: dict[str, object], records, installed_files) -> None:
    for path, contents in records:
        add_tree_file(tree, path, contents, 0x20)
        installed_files.append((path, contents))


def add_host_tree(tree: dict[str, object], root: Path, module,
                  installed_files, manifest: Path | None = None,
                  profile: str = "sasi") -> None:
    """Merge a pre-staged DOS tree into the generated image.

    Package staging is deliberately kept outside the filesystem allocator.  A
    caller can combine files extracted by the host's archive tools and the
    allocator will still apply the same path validation as files transplanted
    from a source D88.  The staging manifest is authoritative for its named
    package paths, so an older payload copy is replaced rather than creating
    a duplicate DOS directory entry.
    """
    records = collect_host_tree(root, module)
    if manifest is not None:
        validate_stage_manifest(manifest, records, profile)
    for path, contents in records:
        if len(path) < 2:
            raise BuildError("supplemental tree file must have a directory: " +
                             "/".join(path))
        replace_tree_file(tree, path, contents, 0x20)
        for index, item in enumerate(installed_files):
            if item[0] == path:
                installed_files[index] = (path, contents)
                break
        else:
            installed_files.append((path, contents))


def fat_now() -> tuple[int, int]:
    now = datetime.now()
    year = min(max(now.year, 1980), 2107)
    date = ((year - 1980) << 9) | (now.month << 5) | now.day
    time = (now.hour << 11) | (now.minute << 5) | (now.second // 2)
    return time, date


def make_entry(name: bytes, attributes: int, cluster: int, size: int) -> bytes:
    fat_time, fat_date = fat_now()
    entry = bytearray(32)
    entry[:11] = name
    entry[11] = attributes
    struct.pack_into("<H", entry, 14, fat_time)
    struct.pack_into("<H", entry, 16, fat_date)
    struct.pack_into("<H", entry, 18, fat_date)
    struct.pack_into("<H", entry, 22, fat_time)
    struct.pack_into("<H", entry, 24, fat_date)
    struct.pack_into("<H", entry, 26, cluster)
    struct.pack_into("<I", entry, 28, size)
    return bytes(entry)


class HdiBuilder:
    def __init__(self, boot: bytes, module):
        if CLUSTER_COUNT < 1:
            raise BuildError("40 MB layout has no data clusters")
        if len(boot) > BOOT_BLOCKS * SECTOR_SIZE:
            raise BuildError("source PC-Engine IPL exceeds one 1024-byte sector")
        self.module = module
        self.image = bytearray(IMAGE_SIZE)
        self.fat = bytearray(FAT_SIZE)
        # HDFORM leaves a zero first byte in each unused root entry and uses
        # E5h for the remaining bytes.  A populated entry replaces its whole
        # 32-byte slot, so DOS still sees the first following zero as the end
        # of the directory.
        self.root = bytearray()
        for _ in range(ROOT_ENTRIES):
            self.root.extend(b"\x00" + b"\xE5" * 31)
        self.next_cluster = 2

        struct.pack_into("<I", self.image, 8, HEADER_SIZE)
        struct.pack_into("<I", self.image, 12, DATA_SIZE)
        struct.pack_into("<I", self.image, 16, SECTOR_SIZE)
        struct.pack_into("<I", self.image, 20, SECTORS)
        struct.pack_into("<I", self.image, 24, SURFACES)
        struct.pack_into("<I", self.image, 28, CYLINDERS)

        boot_sector = bytearray(BOOT_BLOCKS * SECTOR_SIZE)
        boot_sector[:len(boot)] = boot
        self.image[HEADER_SIZE:HEADER_SIZE + len(boot_sector)] = boot_sector
        self.fat[:3] = b"\xD3\xFF\xFF"

    def allocate_chain(self, contents: bytes) -> list[int]:
        count = max(1, (len(contents) + CLUSTER_SIZE - 1) // CLUSTER_SIZE)
        first = self.next_cluster
        last = first + count - 1
        if last > CLUSTER_COUNT + 1:
            raise BuildError("40 MB SASI image has insufficient free clusters")
        for cluster in range(first, last + 1):
            fat_set(self.fat, cluster, 0xFFF if cluster == last else cluster + 1)
            begin = (cluster - first) * CLUSTER_SIZE
            data = contents[begin:begin + CLUSTER_SIZE]
            offset = DATA_OFFSET + (cluster - 2) * CLUSTER_SIZE
            self.image[offset:offset + len(data)] = data
        self.next_cluster = last + 1
        return list(range(first, last + 1))

    def allocate(self, contents: bytes) -> int:
        return self.allocate_chain(contents)[0]

    def directory_slot(self, directory: bytearray, name: str) -> int:
        raw_name = self.module.short_name(name)
        for offset in range(0, len(directory), 32):
            first = directory[offset]
            if first not in (0x00, 0xE5) and \
                    directory[offset:offset + 11] == raw_name:
                raise BuildError(f"duplicate directory entry: {name}")
            if first in (0x00, 0xE5):
                return offset
        raise BuildError("directory is full")

    def add_root(self, name: str, attributes: int, contents: bytes) -> int:
        slot = self.directory_slot(self.root, name)
        first = self.allocate(contents)
        self.root[slot:slot + 32] = make_entry(
            self.module.short_name(name), attributes, first, len(contents))
        return first

    def _create_directory(self, tree: dict[str, object],
                          parent_cluster: int) -> int:
        """Create a directory tree node and return its first cluster."""
        files = tree["files"]
        directories = tree["directories"]
        required_entries = 2 + len(files) + len(directories)
        required_clusters = max(
            1, (required_entries * 32 + CLUSTER_SIZE - 1) // CLUSTER_SIZE)
        directory_clusters = self.allocate_chain(
            bytes(required_clusters * CLUSTER_SIZE))
        directory = bytearray(b"\xE5" * (required_clusters * CLUSTER_SIZE))
        directory[:32] = make_entry(
            self.module.special_directory_name("."), 0x10,
            directory_clusters[0], 0)
        directory[32:64] = make_entry(
            self.module.special_directory_name(".."), 0x10,
            parent_cluster, 0)
        for path_name, contents, attributes in sorted(
                files, key=lambda item: item[0].upper()):
            slot = self.directory_slot(directory, path_name)
            first = self.allocate(contents)
            directory[slot:slot + 32] = make_entry(
                self.module.short_name(path_name), attributes, first,
                len(contents))
        for path_name in sorted(directories.values(),
                                key=lambda item: item["name"].upper()):
            child = self._create_directory(
                path_name["tree"], directory_clusters[0])
            slot = self.directory_slot(directory, path_name["name"])
            directory[slot:slot + 32] = make_entry(
                self.module.short_name(path_name["name"]), 0x10, child, 0)
        for index, cluster in enumerate(directory_clusters):
            begin = index * CLUSTER_SIZE
            offset = DATA_OFFSET + (cluster - 2) * CLUSTER_SIZE
            self.image[offset:offset + CLUSTER_SIZE] = directory[
                begin:begin + CLUSTER_SIZE]
        return directory_clusters[0]

    def add_directory_tree(self, name: str,
                           tree: dict[str, object]) -> int:
        """Create a top-level directory with arbitrary nested subdirectories."""
        first = self._create_directory(tree, 0)
        slot = self.directory_slot(self.root, name)
        self.root[slot:slot + 32] = make_entry(
            self.module.short_name(name), 0x10, first, 0)
        return first

    def add_directory(self, name: str,
                       files: list[tuple[str, bytes, int]]) -> int:
        """Compatibility wrapper for a top-level directory without children."""
        tree = new_directory_tree()
        tree["files"].extend(files)
        return self.add_directory_tree(name, tree)

    def finish(self) -> bytes:
        self.image[FAT1_OFFSET:FAT1_OFFSET + FAT_SIZE] = self.fat
        self.image[FAT2_OFFSET:FAT2_OFFSET + FAT_SIZE] = self.fat
        self.image[ROOT_OFFSET:ROOT_OFFSET + len(self.root)] = self.root
        if self.image[FAT1_OFFSET:FAT1_OFFSET + FAT_SIZE] != \
                self.image[FAT2_OFFSET:FAT2_OFFSET + FAT_SIZE]:
            raise BuildError("FAT copies differ")
        return bytes(self.image)


LSIC_ARCHIVE_SHA256 = (
    "c8c4c49aed600fb2413cf5707ef01b2f4057de69196c3478d5226bf1b224081b"
)
JWASM_ARCHIVE_NAME = "JWASM220.ZIP"
JWASM_ARCHIVE_SHA256 = (
    "e4cab76e0cdc038e4bc284be136cbd0e5116b02a0a2a76fc4a12cad326224723"
)
CPM_ARCHIVE_SIZE = 29761
CPM_ARCHIVE_SHA256 = (
    "691e51dda202ab97b7c8c947ca7c9bf2d93d822f3e315362fcc7840199b8d6f7"
)


def build(source: Path, output: Path, variant: str,
          payload_d88: Path | None = None,
          lsic_archive: Path | None = None,
          lsic_tree: Path | None = None,
          cpm_archive: Path | None = None,
          cpm_tools_d88: Path | None = None,
          cpm_source_d88: Path | None = None,
          cpm_dev_d88: Path | None = None,
          mo_schd_archive: Path | None = None,
          mo_schd_tree: Path | None = None,
          mo_va128mo_archive: Path | None = None,
          mo_va128mo_tree: Path | None = None,
          mo_stest_archive: Path | None = None,
          mo_stest_tree: Path | None = None,
          jwasm_archive: Path | None = None,
          supplemental_tree: Path | None = None,
          supplemental_manifest: Path | None = None,
          two_hc_enabled: bool = False,
          scsi_id: int = 0,
          scsi_profile: str = "fixed-160mb",
          docs_lang: str = "both") -> dict[str, object]:
    module = load_pcengine_module()
    validate_scsi_profile(scsi_id, scsi_profile)
    validate_scsi_doc_language(docs_lang)
    if supplemental_manifest is not None and supplemental_tree is None:
        raise BuildError("--supplemental-manifest requires --supplemental-tree")
    if (lsic_archive is None) != (lsic_tree is None):
        raise BuildError("--lsic-archive and --lsic-tree must be supplied together")
    cpm_disks = (cpm_tools_d88, cpm_source_d88, cpm_dev_d88)
    if cpm_archive is None and any(path is not None for path in cpm_disks):
        raise BuildError(
            "CP/M D88 options require --cpm-archive")
    if cpm_archive is not None and (cpm_tools_d88 is None or cpm_source_d88 is None):
        raise BuildError(
            "--cpm-archive requires --cpm-tools-d88 and --cpm-source-d88")
    mo_inputs = (
        ("schd", mo_schd_archive, mo_schd_tree),
        ("va128mo", mo_va128mo_archive, mo_va128mo_tree),
        ("stest", mo_stest_archive, mo_stest_tree),
    )
    for name, archive, tree in mo_inputs:
        if (archive is None) != (tree is None):
            raise BuildError(
                f"--mo-{name}-archive and --mo-{name}-tree must be supplied together")
    lsic_contents = None
    lsic_records = []
    if lsic_archive is not None and lsic_tree is not None:
        try:
            lsic_contents = lsic_archive.read_bytes()
        except OSError as error:
            raise BuildError(f"cannot read LSI-C archive: {lsic_archive}") from error
        observed_hash = hashlib.sha256(lsic_contents).hexdigest()
        if observed_hash != LSIC_ARCHIVE_SHA256:
            raise BuildError(
                f"LSI-C archive SHA-256 mismatch: {lsic_archive} "
                f"({observed_hash})")
        lsic_records = collect_host_tree(lsic_tree, module)
    jwasm_contents = None
    jwasm_records = []
    if jwasm_archive is not None:
        try:
            jwasm_contents = jwasm_archive.read_bytes()
        except OSError as error:
            raise BuildError(f"cannot read JWasm archive: {jwasm_archive}") from error
        observed_hash = hashlib.sha256(jwasm_contents).hexdigest()
        if observed_hash != JWASM_ARCHIVE_SHA256:
            raise BuildError(
                f"JWasm archive SHA-256 mismatch: {jwasm_archive} "
                f"({observed_hash})")
        jwasm_records = collect_jwasm_package(jwasm_contents, module)
    cpm_records = []
    if cpm_archive is not None:
        cpm_module = load_cpm_module()
        cpm_records.extend(collect_cpm_archive(cpm_archive, module))
        cpm_disk_inputs = [("TOOLS", cpm_tools_d88),
                           ("SOURCE", cpm_source_d88)]
        if cpm_dev_d88 is not None:
            cpm_disk_inputs.append(("DEV", cpm_dev_d88))
        for label, path in cpm_disk_inputs:
            cpm_records.extend(collect_cpm_disk(path, label, cpm_module))
    mo_packages = []
    for name, archive, tree in mo_inputs:
        if archive is None:
            continue
        archive_contents, records = collect_mo_package(
            archive, tree, module, name)
        mo_packages.append((MO_PACKAGE_SPECS[name]["archive_name"],
                            archive_contents, records))
    try:
        # Some preserved 1.05 disks have the same system files at different
        # cluster numbers after later files were added.  Identify the release
        # by the system-file sizes, not by mutable allocation order.
        source_disk = module.PcEngineDisk(
            source.read_bytes(), require_system_files=False)
    except (OSError, module.DiskError) as error:
        raise BuildError(f"cannot read PC-Engine source D88: {error}") from error
    expected_version = "1.1" if variant == "va2" else "1.05"
    observed_sizes = {}
    for name in SYSTEM_FILES:
        offset, exists = module.find_entry(source_disk.root, module.short_name(name))
        if not exists:
            raise BuildError(f"source D88 is missing {name}")
        observed_sizes[name] = struct.unpack_from(
            "<I", source_disk.root, offset + 28)[0]
    version_by_sizes = {
        "1.05": {"ENGINEIO.SYS": 4096, "PCENGINE.SYS": 52090,
                  "ADVGBIOS.SYS": 30956, "PCENGINE.COM": 5},
        "1.1": {"ENGINEIO.SYS": 4096, "PCENGINE.SYS": 62347,
                "ADVGBIOS.SYS": 16364, "PCENGINE.COM": 5},
    }
    source_version = next(
        (version for version, sizes in version_by_sizes.items()
         if observed_sizes == sizes), None)
    if source_version != expected_version:
        raise BuildError(
            f"{variant} requires PC-Engine {expected_version}; "
            f"source system-file sizes identify {source_version or 'unknown'}")
    executables = collect_executables(source_disk, module)
    by_name = {}
    for path, contents in executables:
        name = path[-1].upper()
        if name in by_name:
            raise BuildError(
                f"duplicate executable basename {name}: "
                f"{'/'.join(by_name[name][0])} and {'/'.join(path)}")
        by_name[name] = (path, contents)

    # HDFORM installs the PC-Engine IPL from the system disk.  Reuse the
    # validated source D88 boot sector rather than the generic empty-disk IPL
    # used by the emulator's New SASI image menu.
    builder = HdiBuilder(source_disk.boot_sector, module)
    for name in SYSTEM_FILES:
        builder.add_root(name, SYSTEM_ATTRIBUTES[name],
                         d88_file(source_disk, [name], module))
    builder.add_root("CONFIG.SYS", 0x20,
                     text_file(config_lines(two_hc_enabled, scsi_id,
                                            scsi_profile)))
    builder.add_root("AUTOEXEC.BAT", 0x20,
                     text_file(autoexec_lines(lsic_contents is not None,
                                               cpm_archive is not None,
                                               supplemental_tree is not None)))

    installed_files = []
    if payload_d88 is not None:
        try:
            payload_disk = module.PcEngineDisk(
                payload_d88.read_bytes(), require_system_files=False)
        except (OSError, module.DiskError) as error:
            raise BuildError(
                f"cannot read development payload D88: {error}") from error
        payload_files, payload_directories = collect_payload_files(
            payload_disk, module)
        payload_tree = new_directory_tree()
        for path in payload_directories:
            ensure_directory(payload_tree, path)
        for path, contents, attributes in payload_files:
            name = path[-1].upper()
            if len(path) == 1:
                if name in SYSTEM_FILES:
                    continue
                if name in {"CONFIG.SYS", "AUTOEXEC.BAT"}:
                    continue
                builder.add_root(name, attributes, contents)
                installed_files.append((path, contents))
                continue
            add_tree_file(payload_tree, path, contents, attributes)
            installed_files.append((path, contents))

        # Keep the utilities from the matching PC-Engine source disk in BIN
        # even when a complete development payload is transplanted.  The boot
        # PCENGINE.COM is already installed at the root and is not duplicated.
        source_bin = sorted(
            [(path[-1], contents, 0x20) for path, contents in executables
             if path[-1].upper() != "PCENGINE.COM"],
            key=lambda item: item[0].upper())
        ensure_directory(payload_tree, ("BIN",))
        bin_tree = ensure_directory(payload_tree, ("BIN",))
        existing_bin_names = {item[0].upper() for item in bin_tree["files"]}
        for name, contents, attributes in source_bin:
            if name.upper() in existing_bin_names:
                raise BuildError(
                    f"source executable conflicts with payload BIN entry: {name}")
            add_tree_file(payload_tree, ("BIN", name), contents, attributes)
            installed_files.append((("BIN", name), contents))
            existing_bin_names.add(name.upper())
        if lsic_contents is not None:
            add_tree_file(payload_tree, ("ARCHIVE", "LSIC330C.LZH"),
                          lsic_contents, 0x20)
            installed_files.append((("ARCHIVE", "LSIC330C.LZH"), lsic_contents))
            for path, contents in lsic_records:
                install_path = ("LSIC86",) + path
                add_tree_file(payload_tree, install_path, contents, 0x20)
                installed_files.append((install_path, contents))
        add_cpm_to_tree(payload_tree, cpm_records, installed_files)
        install_mo_packages(payload_tree, mo_packages, installed_files)
        install_scsi_profile_doc(payload_tree, installed_files,
                                 scsi_id, scsi_profile, docs_lang)
        if jwasm_contents is not None:
            add_tree_file(payload_tree, ("ARCHIVE", JWASM_ARCHIVE_NAME),
                          jwasm_contents, 0x20)
            installed_files.append((("ARCHIVE", JWASM_ARCHIVE_NAME),
                                    jwasm_contents))
            for path, contents in jwasm_records:
                add_tree_file(payload_tree, path, contents, 0x20)
                installed_files.append((path, contents))
        if supplemental_tree is not None:
            add_host_tree(payload_tree, supplemental_tree, module,
                          installed_files, supplemental_manifest)
        for key in sorted(payload_tree["directories"]):
            descriptor = payload_tree["directories"][key]
            builder.add_directory_tree(descriptor["name"], descriptor["tree"])
    else:
        records = sorted(
            [(path[-1], contents, 0x20) for path, contents in executables
             if path[-1].upper() != "PCENGINE.COM"],
            key=lambda item: item[0].upper())
        if (lsic_contents is None and not cpm_records and not mo_packages
                and jwasm_contents is None and scsi_profile == "fixed"):
            builder.add_directory("BIN", records)
            installed_files.extend(
                (("BIN", name), contents) for name, contents, _ in records)
        else:
            tree = new_directory_tree()
            for name, contents, attributes in records:
                add_tree_file(tree, ("BIN", name), contents, attributes)
                installed_files.append((("BIN", name), contents))
            if lsic_contents is not None:
                add_tree_file(tree, ("ARCHIVE", "LSIC330C.LZH"),
                              lsic_contents, 0x20)
                installed_files.append((("ARCHIVE", "LSIC330C.LZH"), lsic_contents))
                for path, contents in lsic_records:
                    install_path = ("LSIC86",) + path
                    add_tree_file(tree, install_path, contents, 0x20)
                    installed_files.append((install_path, contents))
            add_cpm_to_tree(tree, cpm_records, installed_files)
            install_mo_packages(tree, mo_packages, installed_files)
            install_scsi_profile_doc(tree, installed_files,
                                     scsi_id, scsi_profile, docs_lang)
            if jwasm_contents is not None:
                add_tree_file(tree, ("ARCHIVE", JWASM_ARCHIVE_NAME),
                              jwasm_contents, 0x20)
                installed_files.append((("ARCHIVE", JWASM_ARCHIVE_NAME),
                                        jwasm_contents))
                for path, contents in jwasm_records:
                    add_tree_file(tree, path, contents, 0x20)
                    installed_files.append((path, contents))
            if supplemental_tree is not None:
                add_host_tree(tree, supplemental_tree, module, installed_files,
                              supplemental_manifest)
            for key in sorted(tree["directories"]):
                descriptor = tree["directories"][key]
                builder.add_directory_tree(descriptor["name"], descriptor["tree"])
    image = builder.finish()
    if output.exists():
        raise BuildError(f"output already exists: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
                dir=output.parent, prefix=output.name + ".tmp.", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(image)
        os.chmod(temporary, 0o644)
        os.replace(temporary, output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {
        "variant": variant,
        "source": str(source),
        "output": str(output),
        "system_version": source_version,
        "scsi_id": scsi_id,
        "scsi_profile": scsi_profile,
        "docs_lang": docs_lang,
        "geometry": {
            "header_bytes": HEADER_SIZE,
            "sector_bytes": SECTOR_SIZE,
            "sectors": SECTORS,
            "surfaces": SURFACES,
            "cylinders": CYLINDERS,
            "cluster_bytes": CLUSTER_SIZE,
            "clusters": CLUSTER_COUNT,
            "image_bytes": len(image),
        },
        "payload_d88": str(payload_d88) if payload_d88 is not None else None,
        "lsic_archive": str(lsic_archive) if lsic_archive is not None else None,
        "lsic_tree": str(lsic_tree) if lsic_tree is not None else None,
        "cpm_archive": str(cpm_archive) if cpm_archive is not None else None,
        "cpm_disks": {
            "tools": str(cpm_tools_d88) if cpm_tools_d88 is not None else None,
            "source": str(cpm_source_d88) if cpm_source_d88 is not None else None,
            "dev": str(cpm_dev_d88) if cpm_dev_d88 is not None else None,
        },
        "jwasm_archive": str(jwasm_archive) if jwasm_archive is not None else None,
        "supplemental_tree": (str(supplemental_tree)
                              if supplemental_tree is not None else None),
        "supplemental_manifest": (str(supplemental_manifest)
                                   if supplemental_manifest is not None else None),
        "installed_files": [
            {"source": "/".join(path), "name": path[-1].upper(),
             "bytes": len(contents),
             "sha256": hashlib.sha256(contents).hexdigest()}
            for path, contents in installed_files
        ],
        "sha256": hashlib.sha256(image).hexdigest(),
    }


def default_source(variant: str) -> Path:
    if variant == "va2":
        return REPOSITORY_ROOT / "docs" / "disks" / "PC-Engine 1.1.d88"
    return REPOSITORY_ROOT / "docs" / "disks" / "PC-Engine 1.05.d88"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Build a boot-layout 40 MB PC-88VA SASI development HDI")
    parser.add_argument("--variant", required=True, choices=("va", "va2"),
                        help="VA uses PC-Engine 1.05; VA2 uses PC-Engine 1.1")
    parser.add_argument("--source", type=Path,
                        help="source PC-Engine D88 (default is the repository docs path)")
    parser.add_argument("--payload-d88", type=Path,
                        help="complete development D88 to transplant into the HDI")
    parser.add_argument("--lsic-archive", type=Path,
                        help="verified LSIC330C.LZH to install under ARCHIVE")
    parser.add_argument("--lsic-tree", type=Path,
                        help="directory extracted from --lsic-archive to install under LSIC86")
    parser.add_argument("--cpm-archive", type=Path,
                        help="verified CP/M program EXEcutor cpm08.zip")
    parser.add_argument("--cpm-tools-d88", type=Path,
                        help="CP/M tools/games D88 to install under CPM\\TOOLS")
    parser.add_argument("--cpm-source-d88", type=Path,
                        help="CP/M source/documentation D88 to install under CPM\\SOURCE")
    parser.add_argument("--cpm-dev-d88", type=Path,
                        help="CP/M development D88 to install under CPM\\DEV")
    parser.add_argument("--mo-schd-archive", type=Path,
                        help="verified SCHD155T.LZH archive")
    parser.add_argument("--mo-schd-tree", type=Path,
                        help="tree extracted from SCHD155T.LZH")
    parser.add_argument("--mo-va128mo-archive", type=Path,
                        help="verified VA128MO.LZH archive")
    parser.add_argument("--mo-va128mo-tree", type=Path,
                        help="tree extracted from VA128MO.LZH")
    parser.add_argument("--mo-stest-archive", type=Path,
                        help="verified STEST115.LZH archive")
    parser.add_argument("--mo-stest-tree", type=Path,
                        help="tree extracted from STEST115.LZH")
    parser.add_argument("--jwasm-archive", type=Path,
                        help="verified JWasm DOS package (JWasm_v220_dos.zip)")
    parser.add_argument("--supplemental-tree", type=Path,
                        help="pre-staged DOS tree to merge into the HDI")
    parser.add_argument("--supplemental-manifest", type=Path,
                        help="manifest produced for the supplemental tree")
    parser.add_argument("--enable-2hc", action="store_true",
                        help="load the SASI-only 2HCDRV.COM device in CONFIG.SYS")
    parser.add_argument("--scsi-id", type=int, default=0, choices=range(8),
                        help="SCSI target ID for SCHD (default: 0)")
    parser.add_argument("--scsi-profile", choices=SCSI_PROFILES,
                        default="fixed-160mb",
                        help="fixed-160mb (default), fixed, mo-128mb, or "
                             "mo-160mb")
    parser.add_argument("--docs-lang", choices=SCSI_DOC_LANGUAGES,
                        default="both",
                        help="SCSI setup-note language: en, ja, or both")
    parser.add_argument("--output", required=True, type=Path,
                        help="new 40 MB SASI HDI path (must not already exist)")
    args = parser.parse_args(argv)
    source = (args.source or default_source(args.variant)).resolve()
    payload_d88 = args.payload_d88.resolve() if args.payload_d88 else None
    lsic_archive = args.lsic_archive.resolve() if args.lsic_archive else None
    lsic_tree = args.lsic_tree.resolve() if args.lsic_tree else None
    cpm_archive = args.cpm_archive.resolve() if args.cpm_archive else None
    cpm_tools_d88 = args.cpm_tools_d88.resolve() if args.cpm_tools_d88 else None
    cpm_source_d88 = args.cpm_source_d88.resolve() if args.cpm_source_d88 else None
    cpm_dev_d88 = args.cpm_dev_d88.resolve() if args.cpm_dev_d88 else None
    mo_paths = {}
    for name in ("schd", "va128mo", "stest"):
        archive = getattr(args, f"mo_{name}_archive")
        tree = getattr(args, f"mo_{name}_tree")
        mo_paths[name] = (archive.resolve() if archive else None,
                          tree.resolve() if tree else None)
    jwasm_archive = args.jwasm_archive.resolve() if args.jwasm_archive else None
    supplemental_tree = (args.supplemental_tree.resolve()
                         if args.supplemental_tree else None)
    supplemental_manifest = (args.supplemental_manifest.resolve()
                             if args.supplemental_manifest else None)
    output = args.output.resolve()
    if not source.is_file():
        parser.error(f"source D88 not found: {source}")
    if payload_d88 is not None and not payload_d88.is_file():
        parser.error(f"payload D88 not found: {payload_d88}")
    if (lsic_archive is None) != (lsic_tree is None):
        parser.error("--lsic-archive and --lsic-tree must be supplied together")
    if lsic_archive is not None and not lsic_archive.is_file():
        parser.error(f"LSI-C archive not found: {lsic_archive}")
    if lsic_tree is not None and not lsic_tree.is_dir():
        parser.error(f"LSI-C extracted tree not found: {lsic_tree}")
    cpm_paths = (cpm_tools_d88, cpm_source_d88, cpm_dev_d88)
    if cpm_archive is None and any(path is not None for path in cpm_paths):
        parser.error(
            "CP/M D88 options require --cpm-archive")
    if cpm_archive is not None and (cpm_tools_d88 is None or cpm_source_d88 is None):
        parser.error(
            "--cpm-archive requires --cpm-tools-d88 and --cpm-source-d88")
    for label, path in (("CP/M archive", cpm_archive),
                        ("CP/M tools D88", cpm_tools_d88),
                        ("CP/M source D88", cpm_source_d88),
                        ("CP/M development D88", cpm_dev_d88)):
        if path is not None and not path.is_file():
            parser.error(f"{label} not found: {path}")
    for name, (archive, tree) in mo_paths.items():
        if (archive is None) != (tree is None):
            parser.error(
                f"--mo-{name}-archive and --mo-{name}-tree must be supplied together")
        if archive is not None and not archive.is_file():
            parser.error(f"MO {name} archive not found: {archive}")
        if tree is not None and not tree.is_dir():
            parser.error(f"MO {name} extracted tree not found: {tree}")
    if jwasm_archive is not None and not jwasm_archive.is_file():
        parser.error(f"JWasm archive not found: {jwasm_archive}")
    if supplemental_tree is not None and not supplemental_tree.is_dir():
        parser.error(f"supplemental tree not found: {supplemental_tree}")
    if supplemental_manifest is not None and not supplemental_manifest.is_file():
        parser.error(f"supplemental manifest not found: {supplemental_manifest}")
    try:
        result = build(source, output, args.variant, payload_d88,
                       lsic_archive, lsic_tree, cpm_archive,
                       cpm_tools_d88, cpm_source_d88, cpm_dev_d88,
                       mo_paths["schd"][0], mo_paths["schd"][1],
                       mo_paths["va128mo"][0], mo_paths["va128mo"][1],
                       mo_paths["stest"][0], mo_paths["stest"][1],
                       jwasm_archive, supplemental_tree, supplemental_manifest,
                       args.enable_2hc, args.scsi_id, args.scsi_profile,
                       args.docs_lang)
    except (BuildError, OSError) as error:
        parser.exit(1, f"error: {error}\n")
    print("Created boot-layout 40 MB SASI development disk")
    print(f"variant: {result['variant']}")
    print(f"SCSI target: ID {result['scsi_id']} ({result['scsi_profile']})")
    print(f"SCSI setup notes: {result['docs_lang']}")
    print(f"output: {result['output']}")
    print(f"size: {result['geometry']['image_bytes']} bytes")
    print(f"SHA-256: {result['sha256']}")
    print(f"installed files: {len(result['installed_files'])}")
    for item in result["installed_files"]:
        print(f"  {item['name']}: {item['bytes']} bytes ({item['source']})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

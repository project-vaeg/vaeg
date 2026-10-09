#!/usr/bin/env python3
#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from urllib.error import URLError
from urllib.parse import urljoin
from urllib.request import HTTPRedirectHandler, Request, build_opener


SCRIPT_VERSION = "1.1"
BOOT_OUTPUT_NAME = "pcengine-boot-cpmva.d88"
TOOLS_OUTPUT_NAME = "cpmva-tools.d88"
SOURCE_OUTPUT_NAME = "cpmva-source.d88"
DEVELOPMENT_OUTPUT_NAME = "cpmva-dev.d88"
TEST_OUTPUT_NAME = "cpmva-zexall-test.d88"
TEST_2D_OUTPUT_NAME = "cpmva-zexall-test.2d.d88"
MANIFEST_OUTPUT_NAME = "cpmva-build-manifest.json"
REPORT_OUTPUT_NAME = "cpmva-install-report.txt"
USER_AGENT = f"VAEG-CPMVA-installer/{SCRIPT_VERSION}"
CPM22_REGION_SIZE = 0x1600
CPM_BIOS_SIZE = 0x0600
CPM_SYS_SIZE = 0x1C00
CPM_RAW_SIZE = 327680
CPM_DIRECTORY_OFFSET = 0x4000
CPM_DIRECTORY_SIZE = 0x1000
CPM_BLOCK_SIZE = 2048
CPM_MAX_BLOCK = 0x97
CPM_DIRECTORY_ENTRIES = 128
CPM_RECORD_SIZE = 128
# CPMVA's CPMBIOS.MAC uses EXM=1. One directory entry therefore describes
# two 128-record (16 KiB) sub-extents, up to 256 records (32 KiB) in total.
# Keep this derived from the DPB's EXM value so the writer and reader cannot
# silently fall back to the one-entry-per-16-KiB layout.
CPM_EXTENT_MASK = 0x01
CPM_SUBEXTENTS_PER_ENTRY = CPM_EXTENT_MASK + 1
CPM_RECORDS_PER_SUBEXTENT = 128
CPM_RECORDS_PER_ENTRY = CPM_SUBEXTENTS_PER_ENTRY * CPM_RECORDS_PER_SUBEXTENT
CPM_BLOCKS_PER_ENTRY = (CPM_RECORDS_PER_ENTRY * CPM_RECORD_SIZE) // CPM_BLOCK_SIZE
PCENGINE_SECTOR_SIZE = 1024
MAX_MEMBER_SIZE = 4 * 1024 * 1024

GAME_BINARY_MAPPING = {
    "FTM.COM": "vt100-games/HDimage/u0/FTM.COM",
    "ROBOTS.COM": "vt100-games/HDimage/u0/ROBOTS.COM",
    "BACKGMMN.COM": "vt100-games/HDimage/u0/BACKGMMN.COM",
    "CPMTRIS.COM": "vt100-games/HDimage/u0/CPMTRIS.COM",
    "MAZEZAM.COM": "vt100-games/HDimage/u0/MAZEZAM.COM",
}

GAME_SOURCE_MAPPING = {
    "FTM.C": "vt100-games/FindThatMine/ftm.c",
    "FTM.DOC": "vt100-games/HDimage/u0/FTM.DOC",
    "GPL3.TXT": "vt100-games/FindThatMine/gpl-3.0.txt",
    "ROBOTS.C": "vt100-games/Robots/robots.c",
    "ROBOTS.DOC": "vt100-games/HDimage/u0/ROBOTS.DOC",
    "ROBOTS.TXT": "vt100-games/Robots/robots.txt",
    "COPYING.TXT": "vt100-games/Robots/copying.txt",
    "BACKGMMN.C": "vt100-games/Backgammon/backgmmn.c",
    "GAMEPLAN.C": "vt100-games/Backgammon/gameplan.c",
    "GAMEPLAN.HDR": "vt100-games/Backgammon/gameplan.hdr",
    "MYLIB2.C": "vt100-games/Backgammon/mylib2.c",
    "BACKGMMN.DOC": "vt100-games/HDimage/u0/BACKGMMN.DOC",
    "CPMTRIS.Z": "vt100-games/cpmtris/cpmtris.z",
    "CONIO.Z": "vt100-games/cpmtris/conio.z",
    "RAND.Z": "vt100-games/cpmtris/rand.z",
    "CPMTRIS.RD": "vt100-games/cpmtris/README.org",
    "CPMTRIS.DOC": "vt100-games/HDimage/u0/CPMTRIS.DOC",
    "ZMAC.DOC": "vt100-games/cpmtris/zmac/zmac.doc",
    "ZMAC.CPY": "vt100-games/cpmtris/zmac/COPYRIGHT",
    "MAZEZAM.C": "vt100-games/MazezaM/mazezam.c",
    "MAZEZAM.DOC": "vt100-games/HDimage/u0/MAZEZAM.DOC",
    "MAZEZAM.MAK": "vt100-games/MazezaM/Makefile",
    "CONIO.H": "vt100-games/FindThatMine/mescc/conio.h",
    "CTYPE.H": "vt100-games/FindThatMine/mescc/ctype.h",
    "MESCC.H": "vt100-games/FindThatMine/mescc/mescc.h",
    "SPRINTF.H": "vt100-games/FindThatMine/mescc/sprintf.h",
    "STRING.H": "vt100-games/FindThatMine/mescc/string.h",
    "XPRINTF.H": "vt100-games/FindThatMine/mescc/xprintf.h",
    "PRINTF.H": "vt100-games/Robots/mescc/printf.h",
}

MESCC_BINARY_MAPPING = {
    "CC.COM": "vt100-games/FindThatMine/mescc/cc.com",
    "CCOPT.COM": "vt100-games/FindThatMine/mescc/CCOPT.COM",
    "ZSM.COM": "vt100-games/FindThatMine/mescc/ZSM.COM",
    "HEXTOCOM.COM": "vt100-games/FindThatMine/mescc/HEXTOCOM.COM",
}

BDSC_MAPPING = {
    "CC.COM": "bdsc160/CC.COM",
    "CC2.COM": "bdsc160/CC2.COM",
    "CCONFIG.COM": "bdsc160/CCONFIG.COM",
    "CLIB.COM": "bdsc160/CLIB.COM",
    "CLINK.COM": "bdsc160/CLINK.COM",
    "DEFF.CRL": "bdsc160/DEFF.CRL",
    "DEFF2.CRL": "bdsc160/DEFF2.CRL",
    "C.CCC": "bdsc160/C.CCC",
    "C.SUB": "bdsc160/C.SUB",
    "BDSRDM.TXT": "bdsc160/-READ.ME",
    "BDSFILES.DOC": "bdsc160/FILES.DOC",
    "BDSCIO.H": "extra/BDSCIO.H",
    "BDSINFO.TXT": "source/Readme.txt",
    "BDSARCH.TXT": "README.txt",
}

GAME_LICENSES = {
    "FTM.COM": {
        "license": "GPL-3.0-only",
        "evidence": ["GPL3.TXT", "FTM.DOC"],
        "source_wording": "GNU GPL; the archive includes the GPL version 3 text.",
    },
    "ROBOTS.COM": {
        "license": "GPL-2.0-only",
        "evidence": ["COPYING.TXT", "ROBOTS.DOC"],
        "source_wording": "GPL Version 2.",
    },
    "BACKGMMN.COM": {
        "license": "Public domain",
        "evidence": ["BACKGMMN.DOC"],
        "source_wording": "The project README and original read.me state public domain.",
    },
    "CPMTRIS.COM": {
        "license": "GPL-2.0-or-later",
        "evidence": ["CPMTRIS.RD", "CPMTRIS.DOC"],
        "source_wording": "GPL version 2 or later.",
    },
    "MAZEZAM.COM": {
        "license": "GPL (version unspecified)",
        "evidence": ["MAZEZAM.DOC", "MAZEZAM.C"],
        "source_wording": "The project README says licensed under the GPL without a version.",
    },
}


class InstallerError(Exception):
    def __init__(self, code: str, message: str):
        super().__init__(f"{code}: {message}")
        self.code = code
        self.message = message


def fail(code: str, message: str) -> None:
    raise InstallerError(code, message)


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_path(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = tempfile.NamedTemporaryFile(
        dir=path.parent, prefix=f".{path.name}.", suffix=".part", delete=False
    )
    temporary_name = Path(temporary.name)
    try:
        with temporary:
            temporary.write(data)
            temporary.flush()
            os.fsync(temporary.fileno())
        os.replace(temporary_name, path)
    finally:
        if temporary_name.exists():
            temporary_name.unlink()


def load_lock(path: Path) -> dict:
    try:
        lock = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        fail("LOCK_READ", f"cannot read {path}: {error}")
    if lock.get("schema_version") != 1:
        fail("LOCK_SCHEMA", "unsupported sources.lock.json schema")
    sources = lock.get("sources")
    if not isinstance(sources, dict):
        fail("LOCK_SCHEMA", "sources must be an object")
    for name, source in sources.items():
        if not isinstance(source, dict):
            fail("LOCK_SCHEMA", f"source {name} is not an object")
        digest = source.get("sha256")
        size = source.get("size")
        if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
            fail("LOCK_DIGEST", f"source {name} has no pinned SHA-256")
        if not isinstance(size, int) or size <= 0:
            fail("LOCK_SIZE", f"source {name} has no positive pinned size")
        if "placeholder" in json.dumps(source).lower():
            fail("LOCK_PLACEHOLDER", f"source {name} contains a placeholder")
    assembler = lock.get("assembler")
    if not isinstance(assembler, dict) or assembler.get("version") != "1.8":
        fail("LOCK_ASSEMBLER", "z80asm 1.8 must be pinned")
    if not re.fullmatch(r"[0-9a-f]{64}", str(assembler.get("source_sha256"))) or not (
        isinstance(assembler.get("source_size"), int) and assembler["source_size"] > 0
    ):
        fail("LOCK_ASSEMBLER", "z80asm 1.8 source digest and size must be pinned")
    return lock


def validate_member_name(name: str) -> str:
    normalized = name.replace("\\", "/")
    path = PurePosixPath(normalized)
    if not normalized or "\x00" in normalized or path.is_absolute():
        fail("ARCHIVE_PATH", f"unsafe archive member {name!r}")
    if any(part in ("", ".", "..") for part in path.parts):
        fail("ARCHIVE_PATH", f"unsafe archive member {name!r}")
    return str(path)


def validate_extracted_tree(root: Path, max_members: int, max_bytes: int) -> dict[str, bytes]:
    files = []
    total = 0
    for path in root.rglob("*"):
        if path.is_symlink():
            fail("ARCHIVE_SYMLINK", f"symlink extracted from archive: {path}")
        if not path.is_file():
            continue
        real = path.resolve()
        if root.resolve() not in real.parents:
            fail("ARCHIVE_ESCAPE", f"extracted path escaped temporary root: {path}")
        relative = validate_member_name(path.relative_to(root).as_posix())
        size = path.stat().st_size
        if size > MAX_MEMBER_SIZE:
            fail("ARCHIVE_MEMBER_SIZE", f"member is too large: {relative}")
        total += size
        if total > max_bytes:
            fail("ARCHIVE_TOTAL_SIZE", "extracted archive exceeds the size limit")
        files.append((relative, path))
    if len(files) > max_members:
        fail("ARCHIVE_MEMBER_COUNT", "archive contains too many members")
    result: dict[str, bytes] = {}
    for relative, path in files:
        if relative in result:
            fail("ARCHIVE_DUPLICATE", f"duplicate normalized member {relative}")
        result[relative] = path.read_bytes()
    return result


def lha_is_lhasa(executable: str) -> bool:
    """Tell lhasa (Debian/Ubuntu "lhasa", often installed as "lha") from lha-1.14i."""
    try:
        probe = subprocess.run(
            [executable], capture_output=True, text=True, errors="replace", check=False, timeout=20
        )
    except (OSError, subprocess.SubprocessError):
        return False
    return "lhasa" in (probe.stdout + probe.stderr).lower()


def restore_upper_case_names(root: Path) -> None:
    """Undo lhasa's lower-casing of MS-DOS member names.

    lhasa lower-cases names stored by MS-DOS archivers, while lha-1.14i and
    unar keep them as stored (upper case for CPMVA.LZH).
    """
    for current, directories, files in os.walk(root, topdown=False):
        for name in files + directories:
            upper = name.upper()
            if upper == name:
                continue
            source = Path(current) / name
            target = Path(current) / upper
            if target.exists():
                fail("ARCHIVE_DUPLICATE", f"case-folded member collides: {upper}")
            source.rename(target)


def safe_extract_archive(path: Path, work: Path, lock: dict) -> dict[str, bytes]:
    import tarfile
    import zipfile

    limits = lock["limits"]
    destination = work / "extract"
    destination.mkdir(parents=True, exist_ok=True)
    suffix = path.name.lower()
    if suffix.endswith(".zip"):
        try:
            with zipfile.ZipFile(path) as archive:
                infos = archive.infolist()
                if len(infos) > limits["max_archive_members"]:
                    fail("ARCHIVE_MEMBER_COUNT", "ZIP contains too many members")
                names = set()
                total = 0
                for info in infos:
                    name = validate_member_name(info.filename)
                    if name in names:
                        fail("ARCHIVE_DUPLICATE", f"duplicate normalized member {name}")
                    names.add(name)
                    mode = (info.external_attr >> 16) & 0o170000
                    if mode == 0o120000:
                        fail("ARCHIVE_SYMLINK", f"ZIP symlink is not accepted: {name}")
                    total += info.file_size
                    if info.file_size > MAX_MEMBER_SIZE or total > limits["max_archive_bytes"]:
                        fail("ARCHIVE_SIZE", "ZIP exceeds the configured size limit")
                    if info.is_dir():
                        continue
                    target = destination / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(archive.read(info))
        except zipfile.BadZipFile as error:
            fail("ARCHIVE_FORMAT", f"invalid ZIP archive: {error}")
    elif suffix.endswith((".tar", ".tar.gz", ".tgz", ".tar.xz", ".txz", ".tar.bz2")):
        try:
            with tarfile.open(path, "r:*") as archive:
                members = archive.getmembers()
                if len(members) > limits["max_archive_members"]:
                    fail("ARCHIVE_MEMBER_COUNT", "TAR contains too many members")
                names = set()
                total = 0
                for member in members:
                    name = validate_member_name(member.name)
                    if name in names:
                        fail("ARCHIVE_DUPLICATE", f"duplicate normalized member {name}")
                    names.add(name)
                    if member.issym() or member.islnk() or not (member.isfile() or member.isdir()):
                        fail("ARCHIVE_LINK", f"unsupported TAR member {name}")
                    total += member.size
                    if member.size > MAX_MEMBER_SIZE or total > limits["max_archive_bytes"]:
                        fail("ARCHIVE_SIZE", "TAR exceeds the configured size limit")
                    if member.isfile():
                        target = destination / name
                        target.parent.mkdir(parents=True, exist_ok=True)
                        source = archive.extractfile(member)
                        if source is None:
                            fail("ARCHIVE_FORMAT", f"cannot read TAR member {name}")
                        target.write_bytes(source.read())
        except tarfile.TarError as error:
            fail("ARCHIVE_FORMAT", f"invalid TAR archive: {error}")
    elif suffix.endswith(".lzh"):
        executable = shutil.which("lha") or shutil.which("lhasa")
        if executable:
            lhasa = lha_is_lhasa(executable)
            listing = subprocess.run(
                [executable, "l", str(path)],
                check=False,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            if listing.returncode != 0:
                fail("ARCHIVE_FORMAT", listing.stderr.strip() or "lha cannot list archive")
            names = set()
            for line in listing.stdout.splitlines():
                if "[generic]" not in line:
                    continue
                fields = line.split()
                if len(fields) < 7:
                    fail("ARCHIVE_FORMAT", "cannot parse lha member listing")
                name = validate_member_name(fields[-1])
                if name in names:
                    fail("ARCHIVE_DUPLICATE", f"duplicate normalized member {name}")
                names.add(name)
                try:
                    size = int(fields[1])
                except ValueError:
                    fail("ARCHIVE_FORMAT", "cannot parse lha member size")
                if size > MAX_MEMBER_SIZE:
                    fail("ARCHIVE_MEMBER_SIZE", f"member is too large: {name}")
            if len(names) > limits["max_archive_members"]:
                fail("ARCHIVE_MEMBER_COUNT", "LZH contains too many members")
            # lha-1.14i takes "x -w=DIR"; lhasa only accepts "xw=DIR".
            command = (
                [executable, f"xw={destination}", str(path)]
                if lhasa
                else [executable, "x", f"-w={destination}", str(path)]
            )
            extracted = subprocess.run(
                command,
                check=False,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            if extracted.returncode != 0:
                fail("ARCHIVE_FORMAT", extracted.stderr.strip() or "lha extraction failed")
            if lhasa:
                restore_upper_case_names(destination)
        else:
            executable = shutil.which("unar")
            if not executable:
                fail(
                    "ARCHIVE_TOOL",
                    "CPMVA.LZH requires lha or unar; install one or use an extracted cache",
                )
            extracted = subprocess.run(
                [executable, "-o", str(destination), "-f", str(path)],
                check=False,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            if extracted.returncode != 0:
                fail("ARCHIVE_FORMAT", extracted.stderr.strip() or "unar extraction failed")
    else:
        fail("ARCHIVE_FORMAT", f"unsupported archive type: {path.name}")
    return validate_extracted_tree(
        destination, limits["max_archive_members"], limits["max_archive_bytes"]
    )


class LimitedRedirectHandler(HTTPRedirectHandler):
    def __init__(self, maximum: int):
        super().__init__()
        self.maximum = maximum
        self.count = 0

    def redirect_request(self, request, fp, code, msg, headers, new_url):
        self.count += 1
        if self.count > self.maximum:
            fail("NETWORK_REDIRECT", "redirect limit exceeded")
        return super().redirect_request(request, fp, code, msg, headers, new_url)


def download_bytes(url: str, limit: int, lock: dict) -> tuple[bytes, str]:
    handler = LimitedRedirectHandler(lock["limits"]["max_redirects"])
    opener = build_opener(handler)
    request = Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with opener.open(request, timeout=20) as response:
            length = response.headers.get("Content-Length")
            if length is not None and int(length) > limit:
                fail("NETWORK_SIZE", f"download exceeds {limit} bytes")
            data = bytearray()
            while True:
                block = response.read(65536)
                if not block:
                    break
                data.extend(block)
                if len(data) > limit:
                    fail("NETWORK_SIZE", f"download exceeds {limit} bytes")
            return bytes(data), response.geturl()
    except (OSError, URLError, ValueError) as error:
        fail("NETWORK", f"cannot download {url}: {error}")


def resolve_download_url(spec: dict, lock: dict) -> str:
    discovery = spec.get("discovery_url")
    if not discovery:
        return spec["resolved_url"]
    candidates = [discovery]
    if discovery.startswith("https://") and spec.get("http_fallback_url"):
        candidates.append(discovery.replace("https://", "http://", 1))
    for page_url in candidates:
        try:
            page, final = download_bytes(
                page_url, min(lock["limits"]["max_download_bytes"], 1024 * 1024), lock
            )
        except InstallerError:
            continue
        html = page.decode("utf-8", errors="replace")
        links = re.findall(r"""href\s*=\s*["']([^"']+)["']""", html, flags=re.I)
        for link in links:
            absolute = urljoin(final, link)
            if "cpmva" in absolute.lower() and "lzh" in absolute.lower():
                return absolute
            if "cpm2-asm.zip" in absolute.lower():
                return absolute
    return spec["resolved_url"]


def verify_file(path: Path, spec: dict) -> None:
    if not path.is_file():
        fail("SOURCE_MISSING", f"source file is not a regular file: {path}")
    actual_size = path.stat().st_size
    actual_hash = sha256_path(path)
    if actual_size != spec["size"]:
        fail(
            "SOURCE_SIZE",
            f"{path.name}: expected {spec['size']} bytes, got {actual_size}",
        )
    if actual_hash != spec["sha256"]:
        fail(
            "SOURCE_DIGEST",
            f"{path.name}: expected {spec['sha256']}, got {actual_hash}",
        )


def fetch_locked_source(
    key: str,
    spec: dict,
    override: Path | None,
    cache_dir: Path,
    lock: dict,
    offline: bool,
) -> tuple[Path, str, bool]:
    extension = {"lzh": ".lzh", "zip": ".zip", "tar.gz": ".tar.gz"}.get(
        spec.get("archive_type"), ".bin"
    )
    destination = cache_dir / "sources" / f"{key}-{spec['sha256']}{extension}"
    if override is not None:
        verify_file(override, spec)
        return override, "local override", True
    if destination.exists():
        verify_file(destination, spec)
        metadata_path = destination.with_name(destination.name + ".json")
        resolved = spec["resolved_url"]
        if metadata_path.is_file():
            try:
                metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
                if metadata.get("sha256") == spec["sha256"] and isinstance(metadata.get("resolved_url"), str):
                    resolved = metadata["resolved_url"]
            except (OSError, ValueError):
                pass
        return destination, resolved, False
    if offline:
        fail("OFFLINE_MISS", f"locked source is not cached: {key}")
    url = resolve_download_url(spec, lock)
    urls = [url]
    fallback = spec.get("http_fallback_url")
    if fallback and fallback not in urls:
        urls.append(fallback)
    last_error = None
    for candidate in urls:
        try:
            data, final_url = download_bytes(
                candidate, lock["limits"]["max_download_bytes"], lock
            )
            if len(data) != spec["size"] or sha256_bytes(data) != spec["sha256"]:
                fail("SOURCE_DIGEST", f"locked digest mismatch for {candidate}")
            atomic_write(destination, data)
            atomic_write(
                destination.with_name(destination.name + ".json"),
                (json.dumps({"sha256": spec["sha256"], "resolved_url": final_url}, sort_keys=True) + "\n").encode("utf-8"),
            )
            verify_file(destination, spec)
            return destination, final_url, False
        except InstallerError as error:
            last_error = error
            if candidate.startswith("http://") and not fallback:
                break
    if last_error:
        raise last_error
    fail("NETWORK", f"cannot download source {key}")


def fetch_locked_text(
    key: str,
    spec: dict,
    cache_dir: Path,
    lock: dict,
    offline: bool,
) -> tuple[bytes, str]:
    destination = cache_dir / "licenses" / f"{key}-{spec['sha256']}.txt"
    if destination.exists():
        data = destination.read_bytes()
        if len(data) != spec["size"] or sha256_bytes(data) != spec["sha256"]:
            fail("LICENSE_CACHE", f"cached license is corrupt: {destination}")
        metadata_path = destination.with_name(destination.name + ".json")
        resolved = spec["resolved_url"]
        if metadata_path.is_file():
            try:
                metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
                if metadata.get("sha256") == spec["sha256"] and isinstance(metadata.get("resolved_url"), str):
                    resolved = metadata["resolved_url"]
            except (OSError, ValueError):
                pass
        return data, resolved
    if offline:
        fail("OFFLINE_MISS", f"locked license is not cached: {key}")
    urls = [spec["resolved_url"]]
    if spec.get("http_fallback_url"):
        urls.append(spec["http_fallback_url"])
    last_error = None
    for candidate in urls:
        try:
            data, final_url = download_bytes(
                candidate, lock["limits"]["max_download_bytes"], lock
            )
            if len(data) != spec["size"] or sha256_bytes(data) != spec["sha256"]:
                fail("LICENSE_DIGEST", f"locked license mismatch for {candidate}")
            atomic_write(destination, data)
            atomic_write(
                destination.with_name(destination.name + ".json"),
                (json.dumps({"sha256": spec["sha256"], "resolved_url": final_url}, sort_keys=True) + "\n").encode("utf-8"),
            )
            return data, final_url
        except InstallerError as error:
            last_error = error
    if last_error:
        raise last_error
    fail("NETWORK", f"cannot download license {key}")


ASSEMBLER_SOURCES = ("z80asm.c", "expressions.c", "gnulib/getopt.c", "gnulib/getopt1.c")


def assembler_source_spec(lock: dict) -> dict:
    assembler = lock["assembler"]
    return {
        "resolved_url": assembler["source_url"],
        "sha256": assembler["source_sha256"],
        "size": assembler["source_size"],
        "archive_type": "tar.gz",
    }


def fetch_assembler_source(lock: dict, cache_dir: Path, offline: bool) -> Path:
    path, _, _ = fetch_locked_source(
        "z80asm", assembler_source_spec(lock), None, cache_dir, lock, offline
    )
    return path


def assembler_version_ok(assembler: str, version: str) -> bool:
    try:
        result = subprocess.run(
            [assembler, "--version"], capture_output=True, text=True, check=False, timeout=20
        )
    except (OSError, subprocess.SubprocessError):
        return False
    return result.returncode == 0 and f"version {version}" in result.stdout


def build_assembler(archive: Path, work: Path, output: Path, version: str) -> Path:
    """Compile z80asm from its locked source tarball with the host C compiler."""
    import tarfile

    source_root = work / "z80asm-source"
    wanted = {f"z80asm-{version}/{name}" for name in ASSEMBLER_SOURCES}
    wanted |= {f"z80asm-{version}/z80asm.h", f"z80asm-{version}/gnulib/getopt.h",
               f"z80asm-{version}/gnulib/gettext.h"}
    try:
        with tarfile.open(archive, "r:gz") as tar:
            for member in tar.getmembers():
                if member.name not in wanted:
                    continue
                if not member.isfile():
                    fail("ASSEMBLER_SOURCE", f"not a regular file: {member.name}")
                name = validate_member_name(member.name)
                extracted = tar.extractfile(member)
                if extracted is None:
                    fail("ASSEMBLER_SOURCE", f"cannot read {member.name}")
                target = source_root / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(extracted.read())
    except (OSError, tarfile.TarError) as error:
        fail("ASSEMBLER_SOURCE", f"cannot read {archive}: {error}")
    base = source_root / f"z80asm-{version}"
    missing = [name for name in wanted if not (source_root / name).is_file()]
    if missing:
        fail("ASSEMBLER_SOURCE", f"missing source members: {', '.join(sorted(missing))}")
    compiler = os.environ.get("CC") or next(
        (found for found in (shutil.which(name) for name in ("cc", "gcc", "clang")) if found), None
    )
    if not compiler:
        fail("ASSEMBLER_COMPILER", "a C compiler (cc, gcc or clang, or $CC) is required to build z80asm")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    command = [
        compiler, "-O2", "-I", str(base / "gnulib"), f'-DVERSION="{version}"',
        *[str(base / name) for name in ASSEMBLER_SOURCES], "-o", str(temporary),
    ]
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=False)
    except OSError as error:
        fail("ASSEMBLER_BUILD", f"cannot run {compiler}: {error}")
    built = temporary if temporary.is_file() else temporary.with_name(temporary.name + ".exe")
    if result.returncode != 0 or not built.is_file():
        fail("ASSEMBLER_BUILD", (result.stdout + result.stderr)[-4000:])
    os.replace(built, output)
    if not assembler_version_ok(str(output), version):
        fail("ASSEMBLER_VERSION", f"the built {output.name} does not report version {version}")
    return output


def ensure_assembler(
    explicit: str | None, lock: dict, cache_dir: Path, offline: bool, work: Path
) -> tuple[str, dict]:
    """Return a z80asm 1.8 executable and how it was obtained.

    An explicit --assembler or VAEG_Z80ASM is used as given. Otherwise a
    z80asm 1.8 in PATH is used; if there is none (another program named
    z80asm does not count), the locked source is downloaded, verified, and
    compiled once into the cache.
    """
    version = lock["assembler"]["version"]
    chosen = explicit or os.environ.get("VAEG_Z80ASM")
    if chosen:
        return chosen, {"origin": "explicit"}
    in_path = shutil.which("z80asm")
    if in_path and assembler_version_ok(in_path, version):
        return in_path, {"origin": "path"}
    digest = lock["assembler"]["source_sha256"]
    suffix = ".exe" if os.name == "nt" else ""
    output = cache_dir / "tools" / f"z80asm-{version}-{digest[:12]}" / f"z80asm{suffix}"
    if not (output.is_file() and assembler_version_ok(str(output), version)):
        archive = fetch_assembler_source(lock, cache_dir, offline)
        build_assembler(archive, work, output, version)
    return str(output), {"origin": "built from locked source", "source_sha256": digest}


def apply_unified_patch(source: bytes, patch: bytes) -> bytes:
    newline = b"\r\n" if b"\r\n" in source else b"\n"
    source_lines = source.decode("ascii").splitlines()
    patch_lines = patch.decode("utf-8").splitlines()
    hunks = []
    index = 0
    while index < len(patch_lines):
        if patch_lines[index].startswith("@@ "):
            match = re.match(
                r"@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@",
                patch_lines[index],
            )
            if not match:
                fail("PATCH_FORMAT", "invalid unified diff hunk header")
            old_start = int(match.group(1))
            old_count = int(match.group(2) or "1")
            index += 1
            body = []
            while index < len(patch_lines) and not patch_lines[index].startswith("@@ "):
                line = patch_lines[index]
                if line and line[0] in " +-":
                    body.append(line)
                else:
                    fail("PATCH_FORMAT", "invalid unified diff body")
                index += 1
            old_lines = [line[1:] for line in body if line[0] in " -"]
            new_lines = [line[1:] for line in body if line[0] in " +"]
            if len(old_lines) != old_count:
                fail("PATCH_FORMAT", "unified diff old-line count mismatch")
            hunks.append((old_start, old_lines, new_lines))
        else:
            index += 1
    if not hunks:
        fail("PATCH_FORMAT", "patch contains no hunks")
    offset = 0
    for old_start, old_lines, new_lines in hunks:
        position = old_start - 1 + offset
        if source_lines[position : position + len(old_lines)] != old_lines:
            fail("PATCH_CONTEXT", "CPM22.Z80 does not match the locked patch context")
        source_lines[position : position + len(old_lines)] = new_lines
        offset += len(new_lines) - len(old_lines)
    return newline.join(line.encode("ascii") for line in source_lines) + newline


def assemble_cpm22(
    source: bytes, patch: bytes, assembler: str, expected_version: str, work: Path
) -> tuple[bytes, dict, str]:
    text = source.decode("ascii")
    if "MEM\tEQU\t62" not in text or "ADD\tA,M" not in text:
        fail("PATCH_ALREADY_APPLIED", "CPM22.Z80 is already patched or unexpected")
    patched = apply_unified_patch(source, patch)
    if b"MEM: EQU 64" not in patched:
        fail("PATCH_VERIFY", "64K MEM setting was not produced")
    source_path = work / "CPM22.Z80"
    binary_path = work / "cpm22-full.bin"
    work.mkdir(parents=True, exist_ok=True)
    source_path.write_bytes(patched)
    try:
        version = subprocess.run(
            [assembler, "--version"], capture_output=True, text=True, check=False
        )
        if version.returncode != 0 or expected_version not in version.stdout:
            fail("ASSEMBLER_VERSION", f"expected {expected_version} from {assembler}")
        build = subprocess.run(
            [assembler, "-l", "-L", "-o", str(binary_path), str(source_path)],
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError as error:
        fail("ASSEMBLER_FAILED", f"cannot execute {assembler}: {error}")
    diagnostic = (build.stdout or "") + "\n" + (build.stderr or "")
    if build.returncode != 0 or re.search(r": error:", diagnostic, re.I):
        fail("ASSEMBLER_FAILED", diagnostic[-4000:])
    if re.search(r": warning:", diagnostic, re.I):
        fail("ASSEMBLER_WARNING", diagnostic[-4000:])
    if not binary_path.is_file():
        fail("ASSEMBLER_OUTPUT", "assembler produced no binary")
    symbols = {}
    for line in diagnostic.splitlines():
        match = re.match(r"([A-Za-z0-9_$]+):\s+equ\s+\$([0-9a-f]+)", line, re.I)
        if match:
            symbols[match.group(1).upper()] = int(match.group(2), 16)
    required = {"CBASE": 0xE400, "PATTRN2": 0xEC00, "BOOT": 0xFA00}
    for name, expected in required.items():
        if symbols.get(name) != expected:
            fail("ASSEMBLER_SYMBOL", f"{name} origin is not {expected:04X}h")
    image = binary_path.read_bytes()
    if len(image) < CPM22_REGION_SIZE:
        fail("ASSEMBLER_SIZE", "assembler output is shorter than the CCP+BDOS region")
    if image[CPM22_REGION_SIZE : CPM22_REGION_SIZE + 3] != b"\xC3\x00\x00":
        fail("ASSEMBLER_OVERLAP", "BIOS jump table does not begin at FA00h")
    region = image[:CPM22_REGION_SIZE]
    if len(region) != CPM22_REGION_SIZE:
        fail("ASSEMBLER_SIZE", "extracted CCP+BDOS region is not 0x1600 bytes")
    return region, symbols, diagnostic


def compose_cpm_sys(ccp_bdos: bytes, bios: bytes, mksys: bytes) -> tuple[bytes, bool]:
    if len(ccp_bdos) != CPM22_REGION_SIZE:
        fail("CPM_SYS_INPUT", "CCP+BDOS input is not 0x1600 bytes")
    if len(bios) != CPM_BIOS_SIZE:
        fail("CPM_BIOS_SIZE", "CPMBIOS.COM is not 0x0600 bytes")
    if b"SYSOFS=10" not in mksys or b"262-CPM*4" not in mksys:
        fail("MKSYS_UNVERIFIED", "MKSYS.BAS does not match the documented composition")
    result = ccp_bdos + bios
    if len(result) != CPM_SYS_SIZE:
        fail("CPM_SYS_SIZE", "CPM.SYS is not exactly 0x1C00 bytes")
    if result[-2:] != b"VA":
        fail("CPM_BIOS_SIGNATURE", "CPMBIOS.COM does not end with ASCII VA")
    return result, True


def validate_d88_structure(data: bytes, expected_type: int | None = None) -> None:
    if len(data) < 0x2B0:
        fail("D88_SIZE", "D88 image is shorter than its header")
    if struct.unpack_from("<I", data, 0x1C)[0] != len(data):
        fail("D88_SIZE", "D88 header size does not match the file")
    if expected_type is not None and data[0x1B] != expected_type:
        fail("D88_TYPE", f"expected D88 type {expected_type:02X}h")
    offsets = struct.unpack_from("<164I", data, 0x20)
    nonzero = [(index, value) for index, value in enumerate(offsets) if value]
    if not nonzero:
        fail("D88_TRACKS", "D88 contains no tracks")
    if any(value < 0x2B0 or value >= len(data) for _, value in nonzero):
        fail("D88_OFFSET", "D88 track offset is outside the image")
    values = [value for _, value in nonzero]
    if values != sorted(values) or len(values) != len(set(values)):
        fail("D88_OVERLAP", "D88 track offsets are not strictly increasing")
    required_tracks = 160 if expected_type == 0x20 else 80 if expected_type in (0x00, 0x10) else None
    if required_tracks is not None:
        if any(offsets[index] == 0 for index in range(required_tracks)):
            fail("D88_TRACKS", f"D88 is missing one of {required_tracks} required tracks")
        if expected_type in (0x00, 0x10) and any(offsets[index] for index in range(required_tracks, 164)):
            fail("D88_TRACKS", f"unexpected D88 tracks beyond {required_tracks} required tracks")
    for position, (track_index, start) in enumerate(nonzero):
        end = nonzero[position + 1][1] if position + 1 < len(nonzero) else len(data)
        if end <= start or start + 16 > end:
            fail("D88_TRACK", f"invalid D88 track extent at {track_index}")
        count = struct.unpack_from("<H", data, start + 4)[0]
        if count <= 0:
            fail("D88_TRACK", f"track {track_index} has no sectors")
        cursor = start
        seen = set()
        for _ in range(count):
            if cursor + 16 > end:
                fail("D88_TRACK", f"sector header escapes track {track_index}")
            cylinder, head, record, size_code = struct.unpack_from("<BBBB", data, cursor)
            stored_size = struct.unpack_from("<H", data, cursor + 14)[0]
            if stored_size <= 0 or cursor + 16 + stored_size > end:
                fail("D88_SECTOR", f"sector data escapes track {track_index}")
            key = (cylinder, head, record)
            if key in seen:
                fail("D88_DUPLICATE", f"duplicate sector {key}")
            seen.add(key)
            cursor += 16 + stored_size
        if cursor != end:
            fail("D88_TRACK", f"unused or overlapping bytes in track {track_index}")


def import_pcengine_disk():
    tool_path = Path(__file__).resolve().parents[1] / "pc88va"
    sys.path.insert(0, str(tool_path))
    try:
        from pcengine_disk import (
            DiskError,
            PcEngineDisk,
            add_file,
            display_name,
            find_entry,
            iter_entries,
            short_name,
        )
    except ImportError as error:
        fail("REPOSITORY_TOOL", f"cannot import pcengine_disk.py: {error}")
    return DiskError, PcEngineDisk, add_file, display_name, find_entry, iter_entries, short_name


def fat_file_bytes(disk, name: str, helpers) -> bytes:
    _, _, _, _, find_entry, _, short_name = helpers
    offset, exists = find_entry(disk.root, short_name(name))
    if not exists:
        fail("FAT_FILE", f"file not found after installation: {name}")
    cluster = struct.unpack_from("<H", disk.root, offset + 26)[0]
    size = struct.unpack_from("<I", disk.root, offset + 28)[0]
    if size == 0:
        return b""
    contents = bytearray()
    for item in disk.cluster_chain(cluster):
        contents.extend(disk.read_cluster(item))
    return bytes(contents[:size])


def choose_backup_name(disk, helpers) -> str:
    _, _, _, _, find_entry, _, short_name = helpers
    for candidate in ("AUTOEXEC.BAK", "AUTOEXE.BK1", "AUTOEXE.BK2", "AUTOEXE.BK3"):
        _, exists = find_entry(disk.root, short_name(candidate))
        if not exists:
            return candidate
    fail("AUTOSTART_BACKUP", "no recoverable AUTOEXEC backup name is available")


def prepare_native_boot_image(
    source_data: bytes, payload: dict[str, bytes], autostart: bool, work: Path
) -> tuple[bytes, dict]:
    validate_d88_structure(source_data, expected_type=0x20)
    helpers = import_pcengine_disk()
    _, PcEngineDisk, add_file, _, find_entry, _, short_name = helpers
    try:
        disk = PcEngineDisk(source_data)
    except Exception as error:
        fail("BOOT_DISK", str(error))
    if disk.free_bytes() <= 0:
        fail("BOOT_SPACE", "boot disk has no free FAT12 space")
    pending = dict(payload)
    if autostart:
        offset, exists = find_entry(disk.root, short_name("AUTOEXEC.BAT"))
        if not exists:
            fail("AUTOSTART_FILE", "AUTOEXEC.BAT is not present on the boot disk")
        original = fat_file_bytes(disk, "AUTOEXEC.BAT", helpers)
        newline = b"\r\n" if b"\r\n" in original else b"\n"
        marker_start = b"REM >>> VAEG CPMVA START >>>"
        marker_end = b"REM <<< VAEG CPMVA END <<<"
        if marker_start not in original or marker_end not in original:
            backup = choose_backup_name(disk, helpers)
            pending[backup] = original
            block = newline.join(
                (marker_start, b"CALL CPMVA.BAT", marker_end, b"")
            )
            pending["AUTOEXEC.BAT"] = original.rstrip(b"\r\n") + newline + block
    required_clusters = sum(
        (len(data) + PCENGINE_SECTOR_SIZE - 1) // PCENGINE_SECTOR_SIZE
        for data in pending.values()
    )
    if required_clusters * PCENGINE_SECTOR_SIZE > disk.free_bytes():
        fail("BOOT_SPACE", "boot disk does not have enough FAT12 free space")
    payload_dir = work / "payload"
    payload_dir.mkdir(parents=True, exist_ok=True)
    for name, data in sorted(pending.items()):
        path = payload_dir / name
        path.write_bytes(data)
        add_file(disk, disk.root, path)
    disk.flush()
    result = bytes(disk.image)
    validate_d88_structure(result, expected_type=0x20)
    verify_disk = PcEngineDisk(result)
    for name, data in pending.items():
        if fat_file_bytes(verify_disk, name, helpers) != data:
            fail("FAT_ROUNDTRIP", f"inserted file does not round-trip: {name}")
    return result, {"inserted_files": sorted(pending), "inserted_bytes": sum(map(len, pending.values()))}


def prepare_imgtool_boot_image(source_data: bytes, payload: dict[str, bytes], work: Path, autostart: bool = False) -> tuple[bytes, dict]:
    if autostart:
        fail("IMGTOOL_AUTOSTART", "--autostart requires the native image backend")
    executable = shutil.which("imgtool")
    if not executable:
        fail("IMGTOOL_MISSING", "imgtool backend requested but imgtool is not installed")
    validate_d88_structure(source_data, expected_type=0x20)
    image = work / "imgtool-boot.d88"
    image.write_bytes(source_data)
    for name, data in sorted(payload.items()):
        source = work / f"imgtool-{name}"
        source.write_bytes(data)
        result = subprocess.run(
            [executable, "put", "d88_fat", str(image), str(source), name],
            capture_output=True, text=True, check=False,
        )
        if result.returncode != 0:
            fail("IMGTOOL_FAILED", result.stderr.strip() or result.stdout.strip())
    return image.read_bytes(), {"inserted_files": sorted(payload), "inserted_bytes": sum(map(len, payload.values()))}


def cpm_name(name: str) -> tuple[bytes, bytes]:
    upper = name.upper()
    if upper.count(".") > 1:
        fail("CPM_NAME", f"not an 8.3 CP/M name: {name}")
    base, _, extension = upper.partition(".")
    allowed = re.compile(r"^[A-Z0-9$#@!%&'(){}_^~]+$")
    if not base or len(base) > 8 or len(extension) > 3:
        fail("CPM_NAME", f"not an 8.3 CP/M name: {name}")
    if not allowed.fullmatch(base) or (extension and not allowed.fullmatch(extension)):
        fail("CPM_NAME", f"unsupported CP/M character in {name}")
    return base.ljust(8).encode("ascii"), extension.ljust(3).encode("ascii")


def build_cpm_raw(files: dict[str, bytes]) -> tuple[bytes, dict]:
    raw = bytearray(CPM_RAW_SIZE)
    raw[CPM_DIRECTORY_OFFSET : CPM_DIRECTORY_OFFSET + CPM_DIRECTORY_SIZE] = b"\xE5" * CPM_DIRECTORY_SIZE
    next_block = 2
    directory_index = 0
    used_blocks = set()
    records = {}
    for name in sorted(files):
        base, extension = cpm_name(name)
        data = files[name]
        if len(data) % CPM_RECORD_SIZE:
            fail("CPM_RECORD_SIZE", f"{name} is not an exact 128-byte record length")
        if CPM_BLOCK_SIZE % CPM_RECORD_SIZE:
            fail("CPM_GEOMETRY", "CP/M block size is not a multiple of the record size")
        total_records = len(data) // CPM_RECORD_SIZE
        entry_count = max(
            1,
            (total_records + CPM_RECORDS_PER_ENTRY - 1) // CPM_RECORDS_PER_ENTRY,
        )
        for entry_number in range(entry_count):
            if directory_index >= CPM_DIRECTORY_ENTRIES:
                fail("CPM_DIRECTORY", "CP/M directory is full")
            first_record = entry_number * CPM_RECORDS_PER_ENTRY
            entry_records = min(
                CPM_RECORDS_PER_ENTRY,
                max(0, total_records - first_record),
            )
            begin = first_record * CPM_RECORD_SIZE
            chunk = data[begin : begin + entry_records * CPM_RECORD_SIZE]
            subextent = 1 if entry_records > CPM_RECORDS_PER_SUBEXTENT else 0
            extent = entry_number * CPM_SUBEXTENTS_PER_ENTRY + subextent
            record_count = entry_records - subextent * CPM_RECORDS_PER_SUBEXTENT
            if record_count > CPM_RECORDS_PER_SUBEXTENT:
                fail("CPM_EXTENT", "CP/M directory entry record count is out of range")
            if extent > 0x1FFF:
                fail("CPM_EXTENT", "CP/M extent number is out of range")
            block_count = (len(chunk) + CPM_BLOCK_SIZE - 1) // CPM_BLOCK_SIZE
            if block_count > CPM_BLOCKS_PER_ENTRY:
                fail("CPM_EXTENT", "CP/M directory entry needs too many allocation blocks")
            blocks = list(range(next_block, next_block + block_count))
            next_block += block_count
            if blocks and blocks[-1] > CPM_MAX_BLOCK:
                fail("CPM_SPACE", "CP/M disk allocation blocks are exhausted")
            if len(used_blocks.intersection(blocks)):
                fail("CPM_ALLOCATOR", "CP/M block allocated twice")
            used_blocks.update(blocks)
            entry = bytearray(32)
            entry[0] = 0
            entry[1:9] = base
            entry[9:12] = extension
            entry[12] = extent & 0x1F
            entry[13] = 0
            entry[14] = (extent >> 5) & 0xFF
            entry[15] = record_count & 0xFF
            for index, block in enumerate(blocks):
                entry[16 + index] = block
                block_offset = CPM_DIRECTORY_OFFSET + block * CPM_BLOCK_SIZE
                part = chunk[index * CPM_BLOCK_SIZE : (index + 1) * CPM_BLOCK_SIZE]
                raw[block_offset : block_offset + len(part)] = part
            entry_offset = CPM_DIRECTORY_OFFSET + directory_index * 32
            raw[entry_offset : entry_offset + 32] = entry
            directory_index += 1
            records.setdefault(name, []).append(
                {
                    "extent": extent,
                    "entry_records": entry_records,
                    "blocks": blocks,
                }
            )
    return bytes(raw), {"directory_entries": directory_index, "allocated_blocks": sorted(used_blocks)}


def parse_cpm_raw(raw: bytes) -> dict[str, bytes]:
    if len(raw) != CPM_RAW_SIZE:
        fail("CPM_RAW_SIZE", "CP/M raw image is not exactly 327680 bytes")
    if raw[CPM_DIRECTORY_OFFSET : CPM_DIRECTORY_OFFSET + CPM_DIRECTORY_SIZE].count(0xE5) == 0:
        fail("CPM_DIRECTORY", "CP/M directory has no unused entries")
    files: dict[str, list[tuple[int, int, bytes]]] = {}
    allocated = set()
    for index in range(CPM_DIRECTORY_ENTRIES):
        entry = raw[CPM_DIRECTORY_OFFSET + index * 32 : CPM_DIRECTORY_OFFSET + (index + 1) * 32]
        if entry[0] == 0xE5:
            continue
        if entry[0] != 0:
            fail("CPM_USER", "only CP/M user area 0 is supported")
        if entry[12] & 0xE0 or entry[13] != 0:
            fail("CPM_EXTENT", "CP/M directory entry has invalid extent fields")
        name = entry[1:9].decode("ascii").rstrip()
        extension = entry[9:12].decode("ascii").rstrip()
        display = name + (f".{extension}" if extension else "")
        extent = entry[12] | (entry[14] << 5)
        group = extent // CPM_SUBEXTENTS_PER_ENTRY
        subextent = extent & CPM_EXTENT_MASK
        records = subextent * CPM_RECORDS_PER_SUBEXTENT + entry[15]
        if records > CPM_RECORDS_PER_ENTRY:
            fail("CPM_EXTENT", f"{display} has too many records in one directory entry")
        block_count = (records * CPM_RECORD_SIZE + CPM_BLOCK_SIZE - 1) // CPM_BLOCK_SIZE
        if block_count > CPM_BLOCKS_PER_ENTRY:
            fail("CPM_EXTENT", f"{display} has too many allocation blocks in one directory entry")
        blocks = [entry[16 + block] for block in range(block_count)]
        if any(block < 2 or block > CPM_MAX_BLOCK for block in blocks):
            fail("CPM_ALLOCATOR", f"{display} references a reserved or invalid block")
        if allocated.intersection(blocks):
            fail("CPM_ALLOCATOR", f"{display} references a block twice")
        allocated.update(blocks)
        content = bytearray()
        for block in blocks:
            offset = CPM_DIRECTORY_OFFSET + block * CPM_BLOCK_SIZE
            content.extend(raw[offset : offset + CPM_BLOCK_SIZE])
        content = content[: records * CPM_RECORD_SIZE]
        entries = files.setdefault(display, [])
        if any(previous_group == group for previous_group, _, _ in entries):
            fail("CPM_EXTENT_DUPLICATE", f"{display} has duplicate logical extent {group}")
        entries.append((group, records, bytes(content)))
    result = {}
    for name, extents in files.items():
        ordered = sorted(extents)
        for expected_group, (group, _, _) in enumerate(ordered):
            if group != expected_group:
                fail("CPM_EXTENT_GAP", f"{name} has a missing logical extent before {group}")
        result[name] = b"".join(data for _, _, data in ordered)
    return result


def wrap_cpm_d88(raw: bytes) -> bytes:
    if len(raw) != CPM_RAW_SIZE:
        fail("CPM_RAW_SIZE", "cannot wrap a non-327680-byte CP/M image")
    header = bytearray(0x2B0)
    header[0:17] = b"CPMVA-TOOLS".ljust(17, b"\0")
    header[0x1A] = 0
    # CPMVA's PC-88VA BIOS selects the 2DD-compatible controller mode
    # before issuing its 2D-format access sequence. Using a 2D D88
    # container would enable the emulator's double-step track transform
    # and make the CP/M directory inaccessible. A 2DD container keeps
    # the direct C/H mapping used by this BIOS while retaining the
    # 256-byte, 16-sector-per-side geometry.
    header[0x1B] = 0x10
    cursor = 0x2B0
    track_offsets = []
    records = []
    for cylinder in range(40):
        for head in range(2):
            track_offsets.append(cursor)
            track = bytearray()
            for record in range(1, 17):
                offset = (cylinder * 2 + head) * 16 * 256 + (record - 1) * 256
                sector = raw[offset : offset + 256]
                if len(sector) != 256:
                    fail("CPM_D88", "raw sector mapping is incomplete")
                controller_cylinder = cylinder
                track.extend(
                    struct.pack(
                        "<BBBBHBBBB3sBH",
                        controller_cylinder, head, record, 1, 16, 0, 0, 0, 0,
                        b"\0\0\0", 0, 256
                    )
                )
                track.extend(sector)
            records.append(bytes(track))
            cursor += len(track)
    for index, offset in enumerate(track_offsets):
        struct.pack_into("<I", header, 0x20 + index * 4, offset)
    image = bytes(header) + b"".join(records)
    image = bytearray(image)
    struct.pack_into("<I", image, 0x1C, len(image))
    return bytes(image)


def media_2d_copy(image: bytes) -> bytes:
    """Copy of an emulator-container D88 with the media byte set to 2D (0x00).

    The 2DD media byte of wrap_cpm_d88 confuses real-disk writing tools,
    which expect 0x00 for the 2D-320 geometry; only that byte differs.
    """
    copy = bytearray(image)
    copy[0x1B] = 0x00
    return bytes(copy)


def unwrap_cpm_d88(image: bytes) -> bytes:
    validate_d88_structure(image, expected_type=0x10)
    offsets = struct.unpack_from("<164I", image, 0x20)
    result = bytearray()
    expected_index = 0
    for cylinder in range(40):
        for head in range(2):
            start = offsets[expected_index]
            cursor = start
            count = struct.unpack_from("<H", image, start + 4)[0]
            sectors = []
            for _ in range(count):
                c, h, r, n = struct.unpack_from("<BBBB", image, cursor)
                if (c, h, r, n) != (cylinder, head, len(sectors) + 1, 1):
                    fail("CPM_D88", "unexpected C/H/R/N order in CP/M D88")
                size = struct.unpack_from("<H", image, cursor + 14)[0]
                sectors.append(image[cursor + 16 : cursor + 16 + size])
                cursor += 16 + size
            if len(sectors) != 16:
                fail("CPM_D88", "CP/M D88 track does not have 16 sectors")
            result.extend(b"".join(sectors))
            expected_index += 1
    if len(result) != CPM_RAW_SIZE:
        fail("CPM_RAW_SIZE", "CP/M D88 did not reconstruct 327680 bytes")
    return bytes(result)


def build_tools_disk(files: dict[str, bytes]) -> bytes:
    raw, metadata = build_cpm_raw(files)
    image = wrap_cpm_d88(raw)
    roundtrip = unwrap_cpm_d88(image)
    if roundtrip != raw or parse_cpm_raw(roundtrip) != files:
        fail("CPM_ROUNDTRIP", "generated CP/M disk failed byte-identical validation")
    return image


def pad_cpm_records(files: dict[str, bytes], fill: bytes = b"\x1a") -> dict[str, bytes]:
    """Pad files to CP/M's 128-byte record boundary."""
    if len(fill) != 1:
        fail("CPM_RECORD_SIZE", "CP/M padding must be one byte")
    return {
        name: data + (fill * ((128 - len(data) % 128) % 128))
        for name, data in files.items()
    }


def make_generated_files(members: dict[str, bytes], cpm_sys: bytes) -> dict[str, bytes]:
    generated = {
        "CPMVA.EXE": members["CPMVA.EXE"],
        "CPM.SYS": cpm_sys,
        "CPMVA.BAT": b"@ECHO OFF\r\nCPMVA.EXE\r\n",
        "CPMVA.TXT": (
            b"CP/MVA installation\r\n"
            b"\r\n"
            b"1. Boot this disk with PC-Engine.\r\n"
            b"2. Run CPMVA.\r\n"
            b"3. When CPMVA asks for a CP/M disk in FD1, swap FD1 to "
            b"the generated CPMVA-TOOLS.D88.\r\n"
            b"4. Press a key.\r\n"
            b"5. Use EXIT to return to PC-Engine.\r\n"
            b"6. Optional: use cpmva-tools.d88 for games and CP/M utilities.\r\n"
            b"7. Use cpmva-source.d88 for source and license documents.\r\n"
            b"8. Use cpmva-dev.d88 for BDS C development tools.\r\n"
        ),
    }
    if "RDCPM.EXE" in members:
        generated["RDCPM.EXE"] = members["RDCPM.EXE"]
    return generated


def archive_member_map(
    archive_path: Path,
    lock: dict,
    work: Path,
    source_key: str,
    mapping: dict[str, str],
    label: str,
) -> dict[str, bytes]:
    members = safe_extract_archive(archive_path, work, lock)
    expected = set(lock["sources"][source_key].get("expected_members", []))
    missing_expected = sorted(expected - set(members))
    if missing_expected:
        fail("ARCHIVE_MEMBERS", f"{label} archive is missing {missing_expected}")
    missing = sorted(set(mapping.values()) - set(members))
    if missing:
        fail("ARCHIVE_MEMBERS", f"{label} archive is missing {missing}")
    return {destination: members[source] for destination, source in mapping.items()}


def source_member_map(archive_path: Path, lock: dict, work: Path) -> dict[str, bytes]:
    members = safe_extract_archive(archive_path, work, lock)
    expected = set(lock["sources"]["cpmva"]["expected_members"])
    basenames = [Path(name).name for name in members]
    if len(basenames) != len(set(basenames)):
        fail("ARCHIVE_DUPLICATE", "archive contains duplicate basenames")
    actual = set(basenames)
    if not expected.issubset(actual):
        fail("ARCHIVE_MEMBERS", f"CPMVA archive is missing {sorted(expected - actual)}")
    return {Path(name).name: data for name, data in members.items()}


def write_manifest(path: Path, manifest: dict) -> None:
    data = json.dumps(manifest, indent=2, sort_keys=True, ensure_ascii=True) + "\n"
    atomic_write(path, data.encode("utf-8"))


def load_zexbuild():
    path = Path(__file__).parent / "zex" / "zexbuild.py"
    spec = importlib.util.spec_from_file_location("vaeg_cpmva_zexbuild", path)
    if spec is None or spec.loader is None:
        fail("TEST_PROGRAMS", f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build_test_programs(assembler: str) -> dict[str, bytes]:
    """Build the ZEX exercisers and M101 probe programs for the tools disk."""
    zexbuild = load_zexbuild()
    try:
        return zexbuild.build_programs(assembler)
    except zexbuild.BuildError as error:
        fail(error.code, str(error))


def make_report(manifest: dict) -> str:
    lines = [
        "VAEG CP/MVA installation report",
        "================================",
        f"Installer version: {manifest['installer_version']}",
        f"Image backend: {manifest['tool_versions']['image_backend']}",
        "",
        "Sources:",
    ]
    for key, source in manifest["sources"].items():
        lines.append(f"- {key}: {source['resolved_url']}")
        lines.append(f"  size={source['size']} sha256={source['sha256']}")
    lines.extend(["", "Test programs on the tools disk:"])
    for name, item in manifest.get("test_programs", {}).get("files", {}).items():
        lines.append(f"- {name}: {item['size']} bytes sha256={item['sha256']}")
    lines.extend(["", "Generated files:"])
    for name, item in manifest["generated_files"].items():
        lines.append(f"- {name}: {item['size']} bytes sha256={item['sha256']}")
    lines.extend(
        [
            "",
            "Boot disk:",
            f"- input sha256: {manifest['boot_disk']['input_sha256']}",
            f"- output sha256: {manifest['boot_disk']['output_sha256']}",
            f"- original unchanged: {manifest['boot_disk']['input_unchanged']}",
            "",
            "License acceptance:",
            f"- CP/M: {manifest['license_acceptance']['cpm_license']}",
            f"- CPMVA: {manifest['license_acceptance']['cpmva_license']}",
            f"- Games: {manifest['license_acceptance']['games_license']}",
            f"- BDS C: {manifest['license_acceptance']['bdsc_license']}",
            "",
            "The generated images are local user artifacts. VAEG does not",
            "redistribute CPM.SYS, CPMVA files, or generated disk images.",
        ]
    )
    return "\n".join(lines) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build a reproducible CP/MVA installation from a user-supplied PC-Engine D88"
    )
    parser.add_argument("--boot-disk", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--cache-dir", type=Path)
    parser.add_argument("--offline", action="store_true")
    parser.add_argument("--download-only", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--keep-work", action="store_true")
    parser.add_argument("--accept-cpm-license", action="store_true")
    parser.add_argument("--accept-cpmva-license", action="store_true")
    parser.add_argument("--accept-games-license", action="store_true")
    parser.add_argument("--accept-bdsc-license", action="store_true")
    parser.add_argument("--cpmva-archive", type=Path)
    parser.add_argument("--cpm22-archive", type=Path)
    parser.add_argument("--vt100-games-archive", type=Path)
    parser.add_argument("--bdsc-archive", type=Path)
    parser.add_argument("--image-backend", choices=("auto", "native", "imgtool"), default="auto")
    parser.add_argument("--autostart", action="store_true")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--vaeg-binary", type=Path)
    parser.add_argument("--assembler", type=str, help="override the z80asm executable path")
    return parser.parse_args()


def require_acceptance(args: argparse.Namespace) -> None:
    accepted = (
        args.accept_cpm_license
        and args.accept_cpmva_license
        and args.accept_games_license
        and args.accept_bdsc_license
    )
    if accepted:
        return
    if not sys.stdin.isatty():
        fail(
            "LICENSE_REQUIRED",
            "non-interactive execution requires --accept-cpm-license, --accept-cpmva-license, --accept-games-license, and --accept-bdsc-license",
        )
    notices = [
        "CP/M permission notice: the current text grants a nonexclusive right to use, distribute, modify, enhance, and otherwise make CP/M and its derivatives available.",
        "CPMVA notice: CPMVA is an historical PC-88VA distribution by Makichan; the archive README/DOC text is preserved as provenance and no broader license is inferred.",
        "Games notice: the bundle contains GPL-3.0, GPL-2.0, GPL-version-unspecified, and public-domain programs. Exact notices are preserved on the source disk.",
        "BDS C notice: the author states that BDS C and its source, binaries, utilities, and documentation are public domain.",
    ]
    print("\n".join(notices))
    prompts = [
        ("accept_cpm_license", "Accept the CP/M permission text? [y/N] "),
        ("accept_cpmva_license", "Accept the CPMVA provenance terms? [y/N] "),
        ("accept_games_license", "Accept the bundled game license notices? [y/N] "),
        ("accept_bdsc_license", "Accept the BDS C public-domain notice? [y/N] "),
    ]
    for attribute, prompt in prompts:
        if not getattr(args, attribute):
            answer = input(prompt).strip().lower()
            if answer not in ("y", "yes"):
                fail("LICENSE_REFUSED", f"{attribute} was not accepted")
            setattr(args, attribute, True)


def choose_backend(requested: str) -> str:
    if requested == "auto":
        return "native"
    return requested


def main() -> int:
    args = parse_args()
    try:
        require_acceptance(args)
        lock = load_lock(Path(__file__).with_name("sources.lock.json"))
        if not args.boot_disk.is_file():
            fail("INPUT_MISSING", f"boot disk does not exist: {args.boot_disk}")
        original_data = args.boot_disk.read_bytes()
        input_digest = sha256_bytes(original_data)
        validate_d88_structure(original_data, expected_type=0x20)
        backend = choose_backend(args.image_backend)
        targets = (
            args.output_dir / BOOT_OUTPUT_NAME,
            args.output_dir / TOOLS_OUTPUT_NAME,
            args.output_dir / SOURCE_OUTPUT_NAME,
            args.output_dir / DEVELOPMENT_OUTPUT_NAME,
            args.output_dir / TEST_OUTPUT_NAME,
            args.output_dir / TEST_2D_OUTPUT_NAME,
            args.output_dir / MANIFEST_OUTPUT_NAME,
            args.output_dir / REPORT_OUTPUT_NAME,
        )
        if not args.dry_run and not args.download_only and not args.verify_only:
            existing = [str(path) for path in targets if path.exists()]
            if existing and not args.force:
                fail("OUTPUT_EXISTS", f"outputs exist; use --force: {', '.join(existing)}")
        if args.dry_run:
            print(f"Would validate input boot disk: {args.boot_disk}")
            print(
                "Would fetch and verify locked CPMVA, CP/M, vt100-games, BDS C, and "
                "(unless one is supplied or in PATH) z80asm 1.8 sources into: "
                f"{args.cache_dir or '~/.cache/vaeg/cpmva'}"
            )
            print(
                f"Would build {BOOT_OUTPUT_NAME}, {TOOLS_OUTPUT_NAME}, "
                f"{SOURCE_OUTPUT_NAME}, {DEVELOPMENT_OUTPUT_NAME}, {TEST_OUTPUT_NAME}, "
                "and the manifest"
            )
            print(f"Would use image backend: {backend}")
            return 0

        cache_dir = args.cache_dir or Path.home() / ".cache" / "vaeg" / "cpmva"
        cache_dir.mkdir(parents=True, exist_ok=True)
        cpmva_path, cpmva_url, cpmva_override = fetch_locked_source(
            "cpmva", lock["sources"]["cpmva"], args.cpmva_archive, cache_dir, lock, args.offline
        )
        cpm22_path, cpm22_url, cpm22_override = fetch_locked_source(
            "cpm22_asm", lock["sources"]["cpm22_asm"], args.cpm22_archive, cache_dir, lock, args.offline
        )
        games_path, games_url, games_override = fetch_locked_source(
            "vt100_games", lock["sources"]["vt100_games"], args.vt100_games_archive, cache_dir, lock, args.offline
        )
        bdsc_path, bdsc_url, bdsc_override = fetch_locked_source(
            "bdsc", lock["sources"]["bdsc"], args.bdsc_archive, cache_dir, lock, args.offline
        )
        license_data, license_url = fetch_locked_text(
            "cpm_license", lock["sources"]["cpm_license"], cache_dir, lock, args.offline
        )
        if not (args.assembler or os.environ.get("VAEG_Z80ASM")):
            # Cache the assembler source with the other inputs so that a later
            # --offline run can build it.
            fetch_assembler_source(lock, cache_dir, args.offline)
        work_root = cache_dir / "work"
        work_root.mkdir(parents=True, exist_ok=True)
        if args.keep_work:
            work = work_root / f"run-{input_digest[:12]}"
            work.mkdir(parents=True, exist_ok=True)
            cleanup = False
        else:
            temporary = tempfile.TemporaryDirectory(prefix="cpmva-", dir=work_root)
            work = Path(temporary.name)
            cleanup = True
        try:
            members = source_member_map(cpmva_path, lock, work / "cpmva")
            cpm22_members = safe_extract_archive(cpm22_path, work / "cpm22", lock)
            cpm22_source = cpm22_members.get("CPM22.Z80") or cpm22_members.get("cpm22.z80")
            if cpm22_source is None:
                fail("CPM_SOURCE_MEMBER", "CPM22.Z80 is missing from the CP/M archive")
            games_files = archive_member_map(
                games_path, lock, work / "games", "vt100_games", GAME_BINARY_MAPPING, "vt100-games"
            )
            game_source_files = archive_member_map(
                games_path, lock, work / "game-sources", "vt100_games", GAME_SOURCE_MAPPING, "vt100-games"
            )
            mescc_files = archive_member_map(
                games_path, lock, work / "mescc", "vt100_games", MESCC_BINARY_MAPPING, "vt100-games"
            )
            bdsc_files = archive_member_map(
                bdsc_path, lock, work / "bdsc", "bdsc", BDSC_MAPPING, "BDS C"
            )
            readme_digest = sha256_bytes(members["README.DOC"])
            atomic_write(
                cache_dir / "licenses" / f"cpmva-readme-{sha256_path(cpmva_path)}.txt",
                members["README.DOC"],
            )
            atomic_write(
                cache_dir / "licenses" / f"bdsc-readme-{sha256_path(bdsc_path)}.txt",
                bdsc_files["BDSRDM.TXT"],
            )
            if args.download_only:
                print("Sources downloaded, digests verified, and expected members validated.")
                return 0
            if args.vaeg_binary is not None and not args.vaeg_binary.is_file():
                fail("VAEG_BINARY", f"VAEG binary does not exist: {args.vaeg_binary}")
            assembler_path, assembler_origin = ensure_assembler(
                args.assembler, lock, cache_dir, args.offline, work
            )
            ccp_bdos, symbols, assembler_diagnostic = assemble_cpm22(
                cpm22_source,
                (Path(__file__).parent / "patches" / "cpm22-64k.patch").read_bytes(),
                assembler_path,
                lock["assembler"]["version"],
                work / "assembler",
            )
            cpm_sys, mksys_equivalent = compose_cpm_sys(
                ccp_bdos, members["CPMBIOS.COM"], members["MKSYS.BAS"]
            )
            generated = make_generated_files(members, cpm_sys)
            test_programs = build_test_programs(assembler_path)
            tools_files = {
                **{name: members[name] for name in ("EXIT.COM", "FCONV.COM", "DO.COM") if name in members},
                **games_files,
                **mescc_files,
            }
            if set(tools_files) != {"EXIT.COM", "FCONV.COM", "DO.COM", *GAME_BINARY_MAPPING, *MESCC_BINARY_MAPPING}:
                fail("CPM_TOOLS", "CP/M tools or game members are incomplete")
            if set(tools_files) & set(test_programs):
                fail("TEST_PROGRAMS", "test program names collide with tools disk members")
            tools_files.update(test_programs)
            tools_disk_files = pad_cpm_records(tools_files, b"\x00")
            source_disk_files = pad_cpm_records(game_source_files, b"\x1a")
            development_disk_files = pad_cpm_records(bdsc_files, b"\x00")
            # The test disk holds only the M101 programs, leaving room for
            # their output files (DAADUMP writes about 130 KiB).
            test_disk_files = pad_cpm_records(test_programs, b"\x00")
            if args.verify_only:
                print("Input disk and all locked CPMVA, CP/M, game, and BDS C sources verified.")
                return 0
            if backend == "native":
                boot_data, image_metadata = prepare_native_boot_image(
                    original_data, generated, args.autostart, work / "boot"
                )
            else:
                boot_data, image_metadata = prepare_imgtool_boot_image(
                    original_data, generated, work / "boot", args.autostart
                )
            if sha256_bytes(boot_data) == input_digest:
                fail("OUTPUT_UNCHANGED", "boot output is byte-identical to the input")
            tools_data = build_tools_disk(tools_disk_files)
            source_data = build_tools_disk(source_disk_files)
            development_data = build_tools_disk(development_disk_files)
            test_data = build_tools_disk(test_disk_files)
            output_dir = args.output_dir
            output_dir.mkdir(parents=True, exist_ok=True)
            boot_output = output_dir / BOOT_OUTPUT_NAME
            tools_output = output_dir / TOOLS_OUTPUT_NAME
            source_output = output_dir / SOURCE_OUTPUT_NAME
            development_output = output_dir / DEVELOPMENT_OUTPUT_NAME
            test_output = output_dir / TEST_OUTPUT_NAME
            test_2d_output = output_dir / TEST_2D_OUTPUT_NAME
            test_2d_data = media_2d_copy(test_data)  # for real-disk writing
            manifest_output = output_dir / MANIFEST_OUTPUT_NAME
            report_output = output_dir / REPORT_OUTPUT_NAME
            for path, data in (
                (boot_output, boot_data),
                (tools_output, tools_data),
                (source_output, source_data),
                (development_output, development_data),
                (test_output, test_data),
                (test_2d_output, test_2d_data),
            ):
                if path.exists() and not args.force:
                    fail("OUTPUT_EXISTS", f"output exists: {path}")
                atomic_write(path, data)
            if sha256_bytes(args.boot_disk.read_bytes()) != input_digest:
                fail("INPUT_MODIFIED", "input boot disk changed during installation")
            manifest = {
                "schema_version": 1,
                "installer_version": SCRIPT_VERSION,
                "license_acceptance": {
                    "cpm_license": bool(args.accept_cpm_license),
                    "cpmva_license": bool(args.accept_cpmva_license),
                    "games_license": bool(args.accept_games_license),
                    "bdsc_license": bool(args.accept_bdsc_license),
                },
                "sources": {
                    "cpmva": {
                        "discovery_url": lock["sources"]["cpmva"]["discovery_url"],
                        "resolved_url": cpmva_url,
                        "size": cpmva_path.stat().st_size,
                        "sha256": sha256_path(cpmva_path),
                        "local_override": cpmva_override,
                        "expected_members": lock["sources"]["cpmva"]["expected_members"],
                    },
                    "cpm22_asm": {
                        "discovery_url": lock["sources"]["cpm22_asm"]["discovery_url"],
                        "resolved_url": cpm22_url,
                        "size": cpm22_path.stat().st_size,
                        "sha256": sha256_path(cpm22_path),
                        "local_override": cpm22_override,
                        "member": "CPM22.Z80",
                    },
                    "cpm_license": {
                        "discovery_url": lock["sources"]["cpm_license"]["discovery_url"],
                        "resolved_url": license_url,
                        "size": len(license_data),
                        "sha256": sha256_bytes(license_data),
                    },
                    "vt100_games": {
                        "discovery_url": lock["sources"]["vt100_games"]["discovery_url"],
                        "resolved_url": games_url,
                        "size": games_path.stat().st_size,
                        "sha256": sha256_path(games_path),
                        "local_override": games_override,
                        "commit": lock["sources"]["vt100_games"].get("commit"),
                        "expected_members": lock["sources"]["vt100_games"]["expected_members"],
                    },
                    "bdsc": {
                        "discovery_url": lock["sources"]["bdsc"]["discovery_url"],
                        "resolved_url": bdsc_url,
                        "size": bdsc_path.stat().st_size,
                        "sha256": sha256_path(bdsc_path),
                        "local_override": bdsc_override,
                        "expected_members": lock["sources"]["bdsc"]["expected_members"],
                    },
                },
                "provenance": {
                    "cpm_license_text_sha256": sha256_bytes(license_data),
                    "cpm_license_text_cache": f"cpm_license-{sha256_bytes(license_data)}.txt",
                    "cpmva_readme_text_sha256": readme_digest,
                    "cpmva_license_note": "The CPMVA archive has no separate license file; its README/DOC text is preserved, and no broader license is inferred.",
                    "games": GAME_LICENSES,
                    "bdsc_license": "Public domain, based on the author statement preserved in the BDS C archive and on the discovery page.",
                    "generated_artifact": "locally produced user artifact",
                    "redistribution": "VAEG does not redistribute downloaded CP/M, CPMVA, game, BDS C files, or generated images.",
                },
                "assembler": {
                    "name": lock["assembler"]["name"],
                    "version": lock["assembler"]["version"],
                    "path_basename": Path(assembler_path).name,
                    "origin": assembler_origin,
                },
                "test_programs": {
                    "sources": "external/zex (GPL-2.0-or-later) and tools/cpmva/zex",
                    "license_note": "ZEX-derived programs are GPL-2.0-or-later; they exist only on this locally generated disk.",
                    "files": {
                        name: {"size": len(data), "sha256": sha256_bytes(data)}
                        for name, data in sorted(test_programs.items())
                    },
                },
                "patches": [{
                    "path": "tools/cpmva/patches/cpm22-64k.patch",
                    "sha256": sha256_path(Path(__file__).parent / "patches" / "cpm22-64k.patch"),
                    "summary": [
                        "Set MEM to 64K.",
                        "Add z80asm-required labels to EQU directives.",
                        "Replace ADD A,M and SBC A,M with ADD A,(HL) and SBC A,(HL).",
                    ],
                }],
                "address_checks": {
                    "ccp_origin": f"{symbols['CBASE']:04X}h",
                    "bdos_origin": f"{symbols['PATTRN2']:04X}h",
                    "bios_origin": f"{symbols['BOOT']:04X}h",
                    "ccp_bdos_size": len(ccp_bdos),
                    "bios_size": len(members["CPMBIOS.COM"]),
                    "cpm_sys_size": len(cpm_sys),
                    "mksys_equivalent": mksys_equivalent,
                },
                "source_members_used": {
                    "cpmva": {
                        name: {"size": len(members[name]), "sha256": sha256_bytes(members[name])}
                        for name in sorted(set(generated) | {"EXIT.COM", "FCONV.COM", "DO.COM"})
                        if name in members
                    },
                    "vt100_games": {
                        name: {"size": len(data), "sha256": sha256_bytes(data)}
                        for name, data in sorted({**games_files, **mescc_files, **game_source_files}.items())
                    },
                    "bdsc": {
                        name: {"size": len(data), "sha256": sha256_bytes(data)}
                        for name, data in sorted(bdsc_files.items())
                    },
                },
                "generated_files": {
                    name: {"size": len(data), "sha256": sha256_bytes(data)}
                    for name, data in sorted(generated.items())
                },
                "boot_disk": {
                    "input_name": args.boot_disk.name,
                    "input_sha256": input_digest,
                    "output_name": BOOT_OUTPUT_NAME,
                    "output_sha256": sha256_bytes(boot_data),
                    "input_unchanged": sha256_bytes(args.boot_disk.read_bytes()) == input_digest,
                    "image_metadata": image_metadata,
                },
                "companion_disk": {
                    "name": TOOLS_OUTPUT_NAME,
                    "size": len(tools_data),
                    "sha256": sha256_bytes(tools_data),
                    "raw_size": CPM_RAW_SIZE,
                    "directory_offset": CPM_DIRECTORY_OFFSET,
                    "files": {
                        name: {
                            "size": len(tools_files[name]),
                            "stored_size": len(tools_disk_files[name]),
                            "sha256": sha256_bytes(tools_files[name]),
                        }
                        for name in sorted(tools_files)
                    },
                },
                "source_disk": {
                    "name": SOURCE_OUTPUT_NAME,
                    "size": len(source_data),
                    "sha256": sha256_bytes(source_data),
                    "raw_size": CPM_RAW_SIZE,
                    "directory_offset": CPM_DIRECTORY_OFFSET,
                    "files": {
                        name: {
                            "size": len(game_source_files[name]),
                            "stored_size": len(source_disk_files[name]),
                            "sha256": sha256_bytes(game_source_files[name]),
                        }
                        for name in sorted(game_source_files)
                    },
                },
                "test_disk": {
                    "name": TEST_OUTPUT_NAME,
                    "media_2d_copy": {
                        "name": TEST_2D_OUTPUT_NAME,
                        "sha256": sha256_bytes(test_2d_data),
                    },
                    "size": len(test_data),
                    "sha256": sha256_bytes(test_data),
                    "raw_size": CPM_RAW_SIZE,
                    "directory_offset": CPM_DIRECTORY_OFFSET,
                    "files": {
                        name: {
                            "size": len(test_programs[name]),
                            "stored_size": len(test_disk_files[name]),
                            "sha256": sha256_bytes(test_programs[name]),
                        }
                        for name in sorted(test_programs)
                    },
                },
                "development_disk": {
                    "name": DEVELOPMENT_OUTPUT_NAME,
                    "size": len(development_data),
                    "sha256": sha256_bytes(development_data),
                    "raw_size": CPM_RAW_SIZE,
                    "directory_offset": CPM_DIRECTORY_OFFSET,
                    "files": {
                        name: {
                            "size": len(bdsc_files[name]),
                            "stored_size": len(development_disk_files[name]),
                            "sha256": sha256_bytes(bdsc_files[name]),
                        }
                        for name in sorted(bdsc_files)
                    },
                },
                "tool_versions": {
                    "python": platform.python_version(),
                    "image_backend": backend,
                    "platform": platform.platform(),
                    "vaeg_binary": (
                        {"name": args.vaeg_binary.name, "sha256": sha256_path(args.vaeg_binary)}
                        if args.vaeg_binary is not None else None
                    ),
                },
            }
            write_manifest(manifest_output, manifest)
            atomic_write(report_output, make_report(manifest).encode("utf-8"))
            print(f"Created {boot_output}")
            print(f"Created {tools_output}")
            print(f"Created {source_output}")
            print(f"Created {development_output}")
            print(f"Created {test_output}")
            print(f"Created {test_2d_output}")
            print(f"Created {manifest_output}")
            print(f"Created {report_output}")
            return 0
        finally:
            if cleanup:
                temporary.cleanup()
    except InstallerError as error:
        print(str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

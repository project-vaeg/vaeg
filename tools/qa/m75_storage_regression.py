#!/usr/bin/env python3
"""Run the disposable M75 SASI/HOSTFAT storage regressions."""

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
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
PCENGINE_DISK_PATH = REPOSITORY_ROOT / "tools" / "pc88va" / "pcengine_disk.py"


def pcengine_disk_module():
    import importlib.util

    spec = importlib.util.spec_from_file_location(
        "m75_pcengine_disk", PCENGINE_DISK_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(
            f"could not load PC-Engine disk helper: {PCENGINE_DISK_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


SASI_HEADER_SIZE = 4096
SASI_CYLINDERS = 615
SASI_SURFACES = 8
SASI_SECTORS = 33
SASI_SECTOR_SIZE = 256
SASI_BLOCK_COUNT = SASI_CYLINDERS * SASI_SURFACES * SASI_SECTORS
SASI_DATA_SIZE = SASI_BLOCK_COUNT * SASI_SECTOR_SIZE
SASI_FILE_SIZE = SASI_HEADER_SIZE + SASI_DATA_SIZE
SASI_MARKER = b"M75 SASI REGRESSION\n"
SASI_TEST_FILE = "G75SASI.COM"
SASI_BACKUP_FILE = "G75SASB.COM"
HOSTFAT_FILE = "REGRESS.TXT"
HOSTFAT_CONTENT = "M75 STORAGE REGRESSION\n"
SCSI_TEST_FILE = "G75TEST.COM"
SCSI_BACKUP_FILE = "G75BACK.COM"
SCSI_ID0_FILE = "G75ID0.COM"
SCSI_ID0_BACKUP_FILE = "G75I0BK.COM"
SCSI_ID1_FILE = "G75ID1.COM"
SCSI_ID1_BACKUP_FILE = "G75I1BK.COM"
SCSI_FORMAT_SCREEN = ("装置初期化", "領域確保", "ACTIVE", "PC-Engine")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def directory_manifest(path: Path) -> dict[str, object]:
    entries = []
    for item in sorted(path.rglob("*")):
        relative = item.relative_to(path).as_posix()
        if item.is_dir():
            entries.append({"path": relative, "type": "directory"})
        elif item.is_file():
            entries.append({
                "path": relative,
                "type": "file",
                "size": item.stat().st_size,
                "sha256": sha256(item),
            })
        else:
            raise AssertionError(f"unexpected HOSTFAT fixture entry: {item}")
    encoded = json.dumps(entries, sort_keys=True, separators=(",", ":"))
    return {
        "entries": entries,
        "sha256": hashlib.sha256(encoded.encode("utf-8")).hexdigest(),
    }


def create_sasi_image(path: Path) -> dict[str, int | str]:
    """Create the same 40MB HDI geometry used by newdisk_hdi()."""
    header = bytearray(SASI_HEADER_SIZE)
    struct.pack_into("<I", header, 8, SASI_HEADER_SIZE)
    struct.pack_into("<I", header, 16, SASI_SECTOR_SIZE)
    struct.pack_into("<I", header, 20, SASI_SECTORS)
    struct.pack_into("<I", header, 24, SASI_SURFACES)
    struct.pack_into("<I", header, 28, SASI_CYLINDERS)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as stream:
        stream.write(header)
        stream.write(SASI_MARKER)
        stream.truncate(SASI_FILE_SIZE)
    return {
        "path": str(path),
        "header_bytes": SASI_HEADER_SIZE,
        "block_size": SASI_SECTOR_SIZE,
        "blocks": SASI_BLOCK_COUNT,
        "size": SASI_FILE_SIZE,
        "sha256": sha256(path),
    }


def create_hostfat_root(path: Path) -> dict[str, int | str | object]:
    path.mkdir(parents=True, exist_ok=True)
    (path / HOSTFAT_FILE).write_text(HOSTFAT_CONTENT, encoding="ascii")
    (path / "SUBDIR").mkdir()
    (path / "SUBDIR" / "DATA.BIN").write_bytes(bytes(range(32)))
    files = sorted(item for item in path.rglob("*") if item.is_file())
    directories = sorted(item for item in path.rglob("*") if item.is_dir())
    manifest = directory_manifest(path)
    return {
        "path": str(path),
        "files": len(files),
        "directories": len(directories),
        "source_bytes": sum(item.stat().st_size for item in files),
        "manifest": manifest,
    }


def guest_input_lines(command: str, drive: str) -> list[str]:
    drive = drive.upper()
    if not re.fullmatch(r"[A-Z]", drive):
        raise ValueError(f"invalid HOSTFAT drive letter: {drive}")
    return [
        "@wait 600",
        f"{command} {drive}:\\{HOSTFAT_FILE}",
        "@wait 120",
    ]


def delete_input_lines(drive: str) -> list[str]:
    drive = drive.upper()
    if not re.fullmatch(r"[A-Z]", drive):
        raise ValueError(f"invalid HOSTFAT drive letter: {drive}")
    return [
        "@wait 600",
        f"DEL {drive}:\\{HOSTFAT_FILE}",
        "@wait 120",
        f"DIR {drive}:",
        "@wait 120",
    ]


def write_guest_input(path: Path, lines: list[str]) -> None:
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def sasi_format_input_lines() -> list[str]:
    return [
        "@wait 900",
        "HDFORM C:",
        "@wait 600",
        "Y",
        "@wait 2400",
    ]


def sasi_create_input_lines() -> list[str]:
    return [
        "@wait 1200",
        f"COPY A:\HDFORM.COM C:\{SASI_TEST_FILE}",
        "@wait 2400",
        "DIR C:",
        "@wait 1200",
    ]


def sasi_readback_input_lines() -> list[str]:
    return [
        "@wait 1200",
        "DIR C:",
        "@wait 1800",
        f"COPY C:\{SASI_TEST_FILE} A:\{SASI_BACKUP_FILE}",
        "@wait 2400",
        "DIR A:",
        "@wait 1200",
    ]


def sasi_delete_input_lines() -> list[str]:
    return [
        "@wait 1200",
        f"DEL C:\{SASI_TEST_FILE}",
        "@wait 2400",
        "DIR C:",
        "@wait 1200",
    ]


def scsi_format_input_lines(target_id: int = 0) -> list[str]:
    if not 0 <= target_id <= 7:
        raise ValueError(f"invalid SCSI target ID: {target_id}")
    return [
        "@wait 1200",
        str(target_id),
        "@wait 3600",
        "1",
        "@wait 6000",
        "@enter",
        "@wait 6000",
        "@enter",
        "@wait 6000",
        "10",
        "@wait 6000",
        "640",
        "@wait 6000",
        "11",
        "@wait 20000",
        "9",
        "@wait 6000",
    ]


def scsi_create_input_lines() -> list[str]:
    return [
        "@wait 1200",
        f"COPY A:\\BIN\\SCFORM.COM C:\\{SCSI_TEST_FILE}",
        "@wait 2400",
        "DIR C:",
        "@wait 1200",
    ]


def scsi_readback_input_lines() -> list[str]:
    return [
        "@wait 1200",
        "DIR C:",
        "@wait 1800",
        f"COPY C:\\{SCSI_TEST_FILE} A:\\{SCSI_BACKUP_FILE}",
        "@wait 2400",
        "DIR A:",
        "@wait 1200",
    ]


def scsi_delete_input_lines() -> list[str]:
    return [
        "@wait 1200",
        f"DEL C:\\{SCSI_TEST_FILE}",
        "@wait 2400",
        "DIR C:",
        "@wait 1200",
    ]


def scsi_two_disk_create_input_lines(drive: str, name: str) -> list[str]:
    drive = drive.upper()
    if not re.fullmatch(r"[A-Z]", drive):
        raise ValueError(f"invalid SCSI drive letter: {drive}")
    return [
        "@wait 1200",
        f"COPY A:\\BIN\\SCFORM.COM {drive}:\\{name}",
        "@wait 2400",
        f"DIR {drive}:",
        "@wait 1200",
    ]


def scsi_two_disk_readback_input_lines(drive: str, name: str,
                                       backup: str) -> list[str]:
    drive = drive.upper()
    if not re.fullmatch(r"[A-Z]", drive):
        raise ValueError(f"invalid SCSI drive letter: {drive}")
    return [
        "@wait 1200",
        f"DIR {drive}:",
        "@wait 1800",
        f"COPY {drive}:\\{name} A:\\{backup}",
        "@wait 2400",
        "DIR A:",
        "@wait 1200",
    ]


def scsi_two_disk_delete_input_lines(drive: str, name: str) -> list[str]:
    drive = drive.upper()
    if not re.fullmatch(r"[A-Z]", drive):
        raise ValueError(f"invalid SCSI drive letter: {drive}")
    return [
        "@wait 1200",
        f"DEL {drive}:\\{name}",
        "@wait 2400",
        f"DIR {drive}:",
        "@wait 1200",
    ]


def d88_find_file(image: Path, path: str) -> bytes:
    """Read one file from a disposable PC-Engine 1.1 D88 image."""
    module = pcengine_disk_module()
    disk = module.PcEngineDisk(image.read_bytes(), require_system_files=False)
    components = [
        component.upper()
        for component in path.replace("\\", "/").split("/")
        if component
    ]
    directory = disk.root
    for index, component in enumerate(components):
        raw_name = module.short_name(component)
        offset, exists = module.find_entry(directory, raw_name)
        if not exists:
            raise RuntimeError(f"D88 file is missing: {path}")
        entry = bytes(directory[offset:offset + 32])
        cluster = struct.unpack_from("<H", entry, 26)[0]
        if index + 1 < len(components):
            if not entry[11] & 0x10:
                raise RuntimeError(
                    f"D88 path component is not a directory: {component}")
            directory_bytes = bytearray()
            for item in disk.cluster_chain(cluster):
                directory_bytes.extend(disk.read_cluster(item))
            directory = directory_bytes
            continue
        size = struct.unpack_from("<I", entry, 28)[0]
        payload = bytearray()
        for item in disk.cluster_chain(cluster):
            payload.extend(disk.read_cluster(item))
        return bytes(payload[:size])
    raise RuntimeError(f"D88 path does not name a file: {path}")


def make_scsi_format_disk(source: Path, destination: Path) -> None:
    """Copy a support disk and make SCFORM run from AUTOEXEC.BAT."""
    module = pcengine_disk_module()
    destination.write_bytes(source.read_bytes())
    disk = module.PcEngineDisk(
        destination.read_bytes(), require_system_files=False)
    with tempfile.TemporaryDirectory(prefix="m75-scsi-autoexec-") as temporary:
        payload = Path(temporary) / "AUTOEXEC.BAT"
        payload.write_bytes(
            b"PATH A:\\BIN\r\nSET COMSPEC=A:\\PCENGINE.COM\r\n"
            b"SCFORM\r\n")
        module.add_file(disk, disk.root, payload)
    disk.flush()
    destination.write_bytes(disk.image)


def make_scsi_two_disk(source: Path, destination: Path,
                       run_scform: bool = False) -> None:
    """Make a disposable boot disk with SCHD target IDs 0 and 1."""
    module = pcengine_disk_module()
    destination.write_bytes(source.read_bytes())
    disk = module.PcEngineDisk(
        destination.read_bytes(), require_system_files=False)
    config = d88_find_file(source, "CONFIG.SYS")
    config_lines = config.splitlines(keepends=True)
    schd0_index = None
    has_schd1 = False
    for index, line in enumerate(config_lines):
        normalized = line.decode("ascii").strip().upper()
        if normalized == r"DEVICE = A:\SCHD.SYS -I0":
            schd0_index = index
        if normalized == r"DEVICE = A:\SCHD.SYS -I1":
            has_schd1 = True
    if schd0_index is None:
        raise RuntimeError("two-disk support D88 lacks SCHD -I0")
    if not has_schd1:
        ending = b"\r\n" if b"\r\n" in config else b"\n"
        config_lines.insert(
            schd0_index + 1,
            b"DEVICE = A:\\SCHD.SYS -I1" + ending)
    config_payload = b"".join(config_lines)
    with tempfile.TemporaryDirectory(prefix="m75-scsi-two-disk-") as temporary:
        config_path = Path(temporary) / "CONFIG.SYS"
        config_path.write_bytes(config_payload)
        module.add_file(disk, disk.root, config_path)
        if run_scform:
            autoexec_path = Path(temporary) / "AUTOEXEC.BAT"
            autoexec_path.write_bytes(
                b"PATH A:\\BIN\r\nSET COMSPEC=A:\\PCENGINE.COM\r\n"
                b"SCFORM\r\n")
            module.add_file(disk, disk.root, autoexec_path)
    disk.flush()
    destination.write_bytes(disk.image)


def create_scsi_image(worker: Path, path: Path) -> dict[str, object]:
    creator = REPOSITORY_ROOT / "tools" / "create_vaeg_scsi_hdd.py"
    completed = subprocess.run(
        [sys.executable, str(creator), "--output", str(path), "--size-mib",
         "40", "--block-size", "256", "--force", "--executable", str(worker)],
        check=False, text=True, capture_output=True)
    if completed.returncode != 0:
        raise RuntimeError(
            f"SCSI image creation failed: {completed.stdout}{completed.stderr}")
    return {
        "path": str(path),
        "size": path.stat().st_size,
        "sha256": sha256(path),
        "stdout": completed.stdout.strip(),
    }


def inspect_scsi_fat(path: Path) -> dict[str, object]:
    inspector = REPOSITORY_ROOT / "tools" / "inspect_vaeg_fat.py"
    completed = subprocess.run(
        [sys.executable, str(inspector), "--image", str(path), "--json"],
        check=False, text=True, capture_output=True)
    if completed.returncode != 0:
        raise RuntimeError(
            f"SCSI FAT inspection failed: {completed.stdout}{completed.stderr}")
    result = json.loads(completed.stdout)
    selected = result.get("selected")
    fat = result.get("fat", {})
    copies = fat.get("copies", [])
    if not selected or not selected.get("valid") or len(copies) != 2:
        raise AssertionError(f"formatted SCSI FAT is invalid: {result}")
    if not fat.get("equal") or any(
            copy.get("free_entries", 0) <= 0 for copy in copies):
        raise AssertionError(
            f"formatted SCSI FAT has no positive free space: {result}")
    return result


def scsi_logical_sector(path: Path, logical_sector: int,
                        info: dict[str, object]) -> bytes:
    container = info["container"]
    selected = info["selected"]
    block_size = int(container["physical_block_size"])
    header_size = int(container["header_size"])
    partition_lba = int(selected["partition_start_physical_lba"])
    multiplier = int(selected["blocks_per_logical_sector"])
    with path.open("rb") as stream:
        stream.seek(
            header_size
            + (partition_lba + logical_sector * multiplier) * block_size)
        data = stream.read(multiplier * block_size)
    if len(data) != int(selected["bytes_per_sector"]):
        raise AssertionError(f"short logical-sector read at {logical_sector}")
    return data


def scsi_root_files(path: Path, info: dict[str, object]) -> dict[str, dict[str, int]]:
    bpb = info["selected"]
    bytes_per_sector = int(bpb["bytes_per_sector"])
    reserved = int(bpb["reserved_sectors"])
    fats = int(bpb["number_of_fats"])
    sectors_per_fat = int(bpb["sectors_per_fat"])
    root_entries = int(bpb["root_directory_entries"])
    root_sectors = int(bpb["root_dir_sectors"])
    root_start = reserved + fats * sectors_per_fat
    root = b"".join(
        scsi_logical_sector(path, root_start + index, info)
        for index in range(root_sectors))
    files = {}
    for offset in range(0, root_entries * 32, 32):
        entry = root[offset:offset + 32]
        if entry[0] == 0:
            break
        if entry[0] in (0xE5, 0x2E) or entry[11] & 0x08:
            continue
        name = entry[:8].decode("ascii", "replace").rstrip()
        extension = entry[8:11].decode("ascii", "replace").rstrip()
        display = f"{name}.{extension}" if extension else name
        files[display] = {
            "cluster": int.from_bytes(entry[26:28], "little"),
            "size": int.from_bytes(entry[28:32], "little"),
        }
    return files


def scsi_file_bytes(path: Path, info: dict[str, object],
                     name: str) -> bytes | None:
    files = scsi_root_files(path, info)
    entry = files.get(name.upper())
    if entry is None:
        return None
    bpb = info["selected"]
    reserved = int(bpb["reserved_sectors"])
    fats = int(bpb["number_of_fats"])
    sectors_per_fat = int(bpb["sectors_per_fat"])
    root_sectors = int(bpb["root_dir_sectors"])
    sectors_per_cluster = int(bpb["sectors_per_cluster"])
    data_start = reserved + fats * sectors_per_fat + root_sectors
    fat = b"".join(
        scsi_logical_sector(path, reserved + index, info)
        for index in range(sectors_per_fat))
    data = bytearray()
    cluster = entry["cluster"]
    visited = set()
    while 2 <= cluster < 0xfff8:
        if cluster in visited:
            raise AssertionError(f"SCSI FAT cluster loop for {name}")
        visited.add(cluster)
        for index in range(sectors_per_cluster):
            data.extend(
                scsi_logical_sector(
                    path,
                    data_start + (cluster - 2) * sectors_per_cluster + index,
                    info))
        cluster = int.from_bytes(fat[cluster * 2:cluster * 2 + 2], "little")
    return bytes(data[:entry["size"]])


def sasi_root_file_entry(path: Path, name: str) -> dict[str, int] | None:
    """Find a DOS 8.3 entry in the formatted SASI image.

    HDI has a 4096-byte container header and the PC-Engine SASI formatter
    keeps directory entries on 32-byte boundaries in the guest-visible data.
    This intentionally validates the directory entry only; payload equality is
    checked independently against the source D88 file and the A: readback.
    """
    base, extension = name.upper().split(".", 1)
    raw_name = base.ljust(8).encode("ascii") + extension.ljust(3).encode("ascii")
    data = path.read_bytes()
    matches = []
    for offset in range(SASI_HEADER_SIZE, len(data) - 31, 32):
        entry = data[offset:offset + 32]
        if entry[:11] != raw_name or entry[0] in (0x00, 0xE5):
            continue
        if entry[11] & 0x08:
            continue
        matches.append({
            "offset": offset,
            "cluster": int.from_bytes(entry[26:28], "little"),
            "size": int.from_bytes(entry[28:32], "little"),
        })
    if len(matches) > 1:
        raise AssertionError(f"SASI directory contains duplicate {name}")
    return matches[0] if matches else None


def fixture_selftest() -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="m75-storage-fixture-") as temporary:
        root = Path(temporary)
        sasi = create_sasi_image(root / "sasi.hdi")
        hostfat = create_hostfat_root(root / "hostfat")
        with (root / "sasi.hdi").open("rb") as stream:
            stream.seek(SASI_HEADER_SIZE)
            marker = stream.read(len(SASI_MARKER))
        if marker != SASI_MARKER:
            raise AssertionError("SASI marker was not written")
        if (root / "sasi.hdi").stat().st_size != SASI_FILE_SIZE:
            raise AssertionError("SASI image size is not deterministic")
        if hostfat["files"] != 2 or hostfat["directories"] != 1:
            raise AssertionError(f"unexpected HOSTFAT fixture: {hostfat}")
        if guest_input_lines("TYPE", "D") != [
                "@wait 600", "TYPE D:\\REGRESS.TXT", "@wait 120"]:
            raise AssertionError("read input script is not deterministic")
        if delete_input_lines("D")[-2] != "DIR D:":
            raise AssertionError("delete input script lacks post-delete DIR")
        if sasi_format_input_lines()[1] != "HDFORM C:":
            raise AssertionError("SASI format script does not target C:")
        if SASI_TEST_FILE not in sasi_create_input_lines()[1]:
            raise AssertionError("SASI create script lacks test file")
        if SASI_BACKUP_FILE not in sasi_readback_input_lines()[3]:
            raise AssertionError("SASI readback script lacks host copy")
        if SASI_TEST_FILE not in sasi_delete_input_lines()[1]:
            raise AssertionError("SASI delete script lacks test file")
        if "G75TEST.COM" not in scsi_create_input_lines()[1]:
            raise AssertionError("SCSI create script lacks test file")
        if "G75BACK.COM" not in scsi_readback_input_lines()[3]:
            raise AssertionError("SCSI readback script lacks host copy")
        if "G75TEST.COM" not in scsi_delete_input_lines()[1]:
            raise AssertionError("SCSI delete script lacks test file")
        if scsi_format_input_lines(1)[1] != "1":
            raise AssertionError("SCSI ID 1 format script is not deterministic")
        if "G75ID0.COM" not in scsi_two_disk_create_input_lines("C", SCSI_ID0_FILE)[1]:
            raise AssertionError("SCSI ID 0 create script lacks test file")
        if "G75ID1.COM" not in scsi_two_disk_create_input_lines("D", SCSI_ID1_FILE)[1]:
            raise AssertionError("SCSI ID 1 create script lacks test file")
        return {
            "sasi": sasi,
            "hostfat": hostfat,
            "sasi_format_input": sasi_format_input_lines(),
            "scsi_format_input": scsi_format_input_lines(),
        }


def validate_guest_inputs(args: argparse.Namespace,
                          require_hostfat: bool = False) -> None:
    if not Path(args.worker).is_file():
        raise FileNotFoundError(f"worker not found: {args.worker}")
    for label, value in (("support D88", args.support_d88),
                         ("ROM directory", args.roms)):
        if not Path(value).exists():
            raise FileNotFoundError(f"{label} not found: {value}")
    if require_hostfat:
        support_bytes = Path(args.support_d88).read_bytes()
        if b"HOSTFAT" not in support_bytes.upper():
            raise RuntimeError(
                "support D88 does not contain HOSTFAT.SYS; provide the "
                "PC-Engine support disk with DEVICE=HOSTFAT.SYS")
        if not re.fullmatch(r"[A-Za-z]", args.hostfat_drive):
            raise ValueError("--hostfat-drive must be one ASCII drive letter")


def run_guest(args: argparse.Namespace, output_root: Path,
              input_lines: list[str] | None = None,
              expected_screen: tuple[str, ...] = ()) -> dict[str, object]:
    output_root.mkdir(parents=True, exist_ok=True)
    harness = Path(__file__).with_name("m75_scsi_harness.py")
    sasi_path = output_root / "sasi.hdi"
    hostfat_path = output_root / "hostfat"
    screen_path = output_root / "screen.bin"
    trace_path = output_root / "trace.log"
    input_path = output_root / "headless-input.txt"
    sasi = create_sasi_image(sasi_path)
    hostfat = create_hostfat_root(hostfat_path)
    sasi_before = sasi["sha256"]
    hostfat_before = hostfat["manifest"]["sha256"]
    if input_lines is not None:
        write_guest_input(input_path, input_lines)

    worker_args = [
        "--model", args.model,
        "--roms", str(Path(args.roms).resolve()),
        "--fdd1", str(Path(args.support_d88).resolve()),
        "--fdd2", "none",
        "--sasi1", str(sasi_path),
        "--sasi2", "none",
        "--hostfat-dir", str(hostfat_path),
        "--nowait", "--mute",
    ]
    if input_lines is not None:
        worker_args.extend(["--headless-input-script", str(input_path)])
    elif args.headless_input_script:
        worker_args.extend(["--headless-input-script",
                            str(Path(args.headless_input_script).resolve())])
    command = [
        sys.executable, str(harness),
        "--worker", str(Path(args.worker).resolve()),
        "--screen-out", str(screen_path),
        "--trace-out", str(trace_path),
        "--timeout", str(args.timeout),
        "--", *worker_args,
    ]
    environment = os.environ.copy()
    environment["VAEG_SCREEN_EXIT_MS"] = str(args.exit_ms)
    completed = subprocess.run(command, check=False, text=True,
                               capture_output=True, env=environment)
    if completed.stdout.strip():
        try:
            harness_result = json.loads(completed.stdout)
        except json.JSONDecodeError as error:
            raise RuntimeError(
                f"harness did not return JSON: {completed.stdout!r}") from error
    else:
        harness_result = {"stdout": completed.stdout}
    if not trace_path.exists():
        raise RuntimeError(
            f"harness produced no trace (exit {completed.returncode}): "
            f"{completed.stderr.strip()}")
    trace = trace_path.read_text(encoding="utf-8", errors="replace")
    if not re.search(r"INFO: SCSI mount path=.*sasi\.hdi .*block_size=256",
                     trace):
        raise AssertionError("SASI mount record is missing from the trace")
    if "HOSTFAT: read-only snapshot ready: 2 files, 1 directories" not in trace:
        raise AssertionError("HOSTFAT snapshot mount record is missing")
    if completed.returncode != 0:
        raise RuntimeError(
            f"storage scenario failed with exit {completed.returncode}: "
            f"{completed.stderr.strip()}")
    if not screen_path.exists():
        raise RuntimeError("storage scenario produced no screen capture")
    screen = harness_result.get("screen", {})
    screen_lines = screen.get("lines", [])
    run_id = harness_result.get("run_id")
    if not run_id or screen.get("run_id") != run_id or run_id not in trace:
        raise AssertionError("screen and trace are not from the same run")
    if harness_result.get("termination") != "process-exit":
        raise AssertionError("scenario did not end by process exit")
    for expected in expected_screen:
        if not any(expected in line for line in screen_lines):
            raise AssertionError(
                f"screen is missing expected text {expected!r}: {screen_lines}")
    sasi_after = sha256(sasi_path)
    hostfat_after = directory_manifest(hostfat_path)["sha256"]
    if sasi_after != sasi_before:
        raise AssertionError("SASI image changed during read-only scenario")
    if hostfat_after != hostfat_before:
        raise AssertionError("HOSTFAT source directory changed during scenario")
    result = {
        "command": command,
        "output_dir": str(output_root),
        "input_script": str(input_path) if input_lines is not None else None,
        "input_lines": input_lines,
        "sasi": {**sasi, "sha256_before": sasi_before,
                 "sha256_after": sasi_after},
        "hostfat": {**hostfat, "manifest_sha256_before": hostfat_before,
                    "manifest_sha256_after": hostfat_after},
        "screen_lines": screen_lines,
        "harness": harness_result,
        "trace_sha256": sha256(trace_path),
        "screen_sha256": sha256(screen_path),
    }
    return result


def run_media_guest(args: argparse.Namespace, output_root: Path,
                     fdd_path: Path, media_option: str, media_path: Path,
                     input_lines: list[str], expected_screen: tuple[str, ...],
                     exit_ms: int,
                     additional_media: dict[str, Path] | None = None
                     ) -> dict[str, object]:
    output_root.mkdir(parents=True, exist_ok=True)
    harness = Path(__file__).with_name("m75_scsi_harness.py")
    screen_path = output_root / "screen.bin"
    trace_path = output_root / "trace.log"
    input_path = output_root / "headless-input.txt"
    write_guest_input(input_path, input_lines)
    worker_args = [
        "--model", args.model,
        "--roms", str(Path(args.roms).resolve()),
        "--fdd1", str(fdd_path.resolve()),
        "--fdd2", "none",
    ]
    media_paths = {media_option: media_path}
    if additional_media:
        media_paths.update(additional_media)
    for option in ("--sasi1", "--sasi2", "--scsi1", "--scsi2",
                   "--scsi3", "--scsi4"):
        worker_args.extend([
            option,
            str(media_paths[option].resolve()) if option in media_paths
            else "none",
        ])
    worker_args.extend([
        "--nowait", "--mute",
        "--headless-input-script", str(input_path),
    ])
    command = [
        sys.executable, str(harness),
        "--worker", str(Path(args.worker).resolve()),
        "--screen-out", str(screen_path),
        "--trace-out", str(trace_path),
        "--timeout", str(args.timeout),
        "--", *worker_args,
    ]
    environment = os.environ.copy()
    environment["VAEG_SCREEN_EXIT_MS"] = str(exit_ms)
    completed = subprocess.run(command, check=False, text=True,
                               capture_output=True, env=environment)
    if completed.stdout.strip():
        try:
            harness_result = json.loads(completed.stdout)
        except json.JSONDecodeError as error:
            raise RuntimeError(
                f"harness did not return JSON: {completed.stdout!r}") from error
    else:
        harness_result = {"stdout": completed.stdout}
    if completed.returncode != 0:
        raise RuntimeError(
            f"guest scenario failed with exit {completed.returncode}: "
            f"{completed.stderr.strip()}")
    if not trace_path.exists() or not screen_path.exists():
        raise RuntimeError("guest scenario produced no screen/trace pair")
    screen = harness_result.get("screen", {})
    screen_lines = screen.get("lines", [])
    run_id = harness_result.get("run_id")
    trace = trace_path.read_text(encoding="utf-8", errors="replace")
    if (not run_id or screen.get("run_id") != run_id
            or run_id not in trace):
        raise AssertionError("screen and trace are not from the same run")
    if harness_result.get("termination") != "process-exit":
        raise AssertionError("guest scenario did not end by process exit")
    for expected in expected_screen:
        if not any(expected in line for line in screen_lines):
            raise AssertionError(
                f"screen is missing expected text {expected!r}: {screen_lines}")
    return {
        "command": command,
        "output_dir": str(output_root),
        "input_script": str(input_path),
        "input_lines": input_lines,
        "screen_lines": screen_lines,
        "harness": harness_result,
        "trace_sha256": sha256(trace_path),
        "screen_sha256": sha256(screen_path),
    }


def run_sasi_format(args: argparse.Namespace) -> dict[str, object]:
    base = (Path(args.output_dir).resolve() / "sasi-format"
            if args.output_dir else
            Path(tempfile.mkdtemp(prefix="m75-sasi-format-")))
    base.mkdir(parents=True, exist_ok=True)
    source = Path(args.sasi_source).resolve()
    boot = base / "boot.d88"
    image = base / "sasi.hdi"
    if boot.exists() or image.exists():
        raise FileExistsError(f"SASI format output already exists: {base}")
    hform = d88_find_file(source, "HDFORM.COM")
    boot.write_bytes(source.read_bytes())
    before = create_sasi_image(image)
    result = run_media_guest(
        args, base / "guest", boot, "--sasi1", image,
        sasi_format_input_lines(),
        ("フォーマットが終わりました", "40435712 バイト", "使用可能ディスク容量"),
        args.g75_exit_ms)
    after = sha256(image)
    if after == before["sha256"]:
        raise AssertionError("HDFORM did not change the SASI image")
    if "HDFORM C:" not in result["screen_lines"]:
        raise AssertionError("SASI screen lacks the HDFORM command")
    formatted_sha = after
    if sasi_root_file_entry(image, SASI_TEST_FILE) is not None:
        raise AssertionError("fresh formatted SASI image already has test file")

    create_result = run_media_guest(
        args, base / "create", boot, "--sasi1", image,
        sasi_create_input_lines(), ("G75SASI .COM",), args.g75_io_exit_ms)
    created_sha = sha256(image)
    if created_sha == formatted_sha:
        raise AssertionError("SASI create did not change the backing image")
    created_entry = sasi_root_file_entry(image, SASI_TEST_FILE)
    if created_entry is None:
        raise AssertionError("SASI create did not create the root entry")
    if created_entry["size"] != len(hform):
        raise AssertionError(
            f"SASI created size {created_entry['size']} != {len(hform)}")
    if image.read_bytes().find(hform, SASI_HEADER_SIZE) < 0:
        raise AssertionError("SASI backing image lacks created file payload")

    readback_before = sha256(image)
    readback_result = run_media_guest(
        args, base / "readback", boot, "--sasi1", image,
        sasi_readback_input_lines(), ("G75SASB .COM",), args.g75_io_exit_ms)
    if sha256(image) != readback_before:
        raise AssertionError("SASI readback changed the backing image")
    readback = d88_find_file(boot, SASI_BACKUP_FILE)
    if readback != hform:
        raise AssertionError("SASI close/reopen readback differs from HDFORM")

    delete_result = run_media_guest(
        args, base / "delete", boot, "--sasi1", image,
        sasi_delete_input_lines(),
        (f"DEL C:\\{SASI_TEST_FILE}", "該当するファイルはありません"),
        args.g75_io_exit_ms)
    deleted_sha = sha256(image)
    if sasi_root_file_entry(image, SASI_TEST_FILE) is not None:
        raise AssertionError("SASI delete left the test root entry")
    return {
        "mode": "sasi-lifecycle",
        "source_d88": str(source),
        "hform_size": len(hform),
        "hform_sha256": hashlib.sha256(hform).hexdigest(),
        "image": {**before, "sha256_after_format": after,
                  "sha256_after_create": created_sha,
                  "sha256_after_delete": deleted_sha},
        "format": result,
        "create": create_result,
        "readback": {**readback_result, "source_equal": True},
        "delete": delete_result,
    }


def run_g75_scsi(args: argparse.Namespace) -> dict[str, object]:
    base = (Path(args.output_dir).resolve() / "g75-scsi"
            if args.output_dir else
            Path(tempfile.mkdtemp(prefix="m75-g75-scsi-")))
    base.mkdir(parents=True, exist_ok=True)
    if any((base / name).exists()
           for name in ("scsi.hdd", "format-boot.d88", "boot.d88")):
        raise FileExistsError(f"SCSI G75 output already exists: {base}")
    support = Path(args.support_d88).resolve()
    format_boot = base / "format-boot.d88"
    boot = base / "boot.d88"
    image = base / "scsi.hdd"
    make_scsi_format_disk(support, format_boot)
    created_image = create_scsi_image(Path(args.worker).resolve(), image)
    format_result = run_media_guest(
        args, base / "format", format_boot, "--scsi1", image,
        scsi_format_input_lines(), SCSI_FORMAT_SCREEN, args.g75_exit_ms)
    fat = inspect_scsi_fat(image)
    expected = d88_find_file(support, "BIN/SCFORM.COM")
    boot.write_bytes(support.read_bytes())
    files_before = scsi_root_files(image, fat)
    if scsi_file_bytes(image, fat, SCSI_TEST_FILE) is not None:
        raise AssertionError("fresh formatted SCSI image already has test file")
    create_result = run_media_guest(
        args, base / "create", boot, "--scsi1", image,
        scsi_create_input_lines(), ("G75TEST .COM",), args.g75_io_exit_ms)
    created_bytes = scsi_file_bytes(image, fat, SCSI_TEST_FILE)
    if created_bytes != expected:
        raise AssertionError("SCSI create payload differs from SCFORM.COM")
    readback_result = run_media_guest(
        args, base / "readback", boot, "--scsi1", image,
        scsi_readback_input_lines(), ("G75BACK .COM",),
        args.g75_io_exit_ms)
    back_bytes = d88_find_file(boot, SCSI_BACKUP_FILE)
    if back_bytes != expected:
        raise AssertionError("SCSI readback copy differs from SCFORM.COM")
    delete_result = run_media_guest(
        args, base / "delete", boot, "--scsi1", image,
        scsi_delete_input_lines(), ("DEL C:\\G75TEST.COM",), args.g75_io_exit_ms)
    delete_lines = delete_result["screen_lines"]
    if any("G75TEST .COM" in line for line in delete_lines):
        raise AssertionError("SCSI file remains in guest DIR after delete")
    if scsi_file_bytes(image, fat, SCSI_TEST_FILE) is not None:
        raise AssertionError("SCSI file remains in the image after delete")
    return {
        "mode": "g75-scsi",
        "support_d88": str(support),
        "source_file": "A:\BIN\SCFORM.COM",
        "source_file_sha256": hashlib.sha256(expected).hexdigest(),
        "created_image": created_image,
        "files_before": files_before,
        "format": format_result,
        "fat": {
            "fat_type": fat["selected"]["fat_type"],
            "cluster_count": fat["selected"]["cluster_count"],
            "free_clusters_fat1": fat["fat"]["copies"][0]["free_entries"],
            "free_clusters_fat2": fat["fat"]["copies"][1]["free_entries"],
        },
        "create": create_result,
        "readback_after_reopen": readback_result,
        "delete_after_reopen": delete_result,
        "image_sha256_after_delete": sha256(image),
        "remaining_files": scsi_root_files(image, fat),
        "boot_d88": str(boot),
        "image": str(image),
    }


def dos_display_name(name: str) -> str:
    base, extension = name.upper().split(".", 1)
    return f"{base:<8}.{extension}"


def run_g75_scsi_two(args: argparse.Namespace) -> dict[str, object]:
    base = (Path(args.output_dir).resolve() / "g75-scsi-two"
            if args.output_dir else
            Path(tempfile.mkdtemp(prefix="m75-g75-scsi-two-")))
    base.mkdir(parents=True, exist_ok=True)
    if any((base / name).exists() for name in
           ("scsi-id0.hdd", "scsi-id1.hdd", "format-boot.d88",
            "boot.d88")):
        raise FileExistsError(f"SCSI two-disk output already exists: {base}")
    support = Path(args.support_d88).resolve()
    format_boot = base / "format-boot.d88"
    boot = base / "boot.d88"
    image0 = base / "scsi-id0.hdd"
    image1 = base / "scsi-id1.hdd"
    make_scsi_two_disk(support, format_boot, run_scform=True)
    make_scsi_two_disk(support, boot)
    created0 = create_scsi_image(Path(args.worker).resolve(), image0)
    created1 = create_scsi_image(Path(args.worker).resolve(), image1)
    both = {"--scsi2": image1}
    format0 = run_media_guest(
        args, base / "format-id0", format_boot, "--scsi1", image0,
        scsi_format_input_lines(0), SCSI_FORMAT_SCREEN, args.g75_exit_ms,
        both)
    format1 = run_media_guest(
        args, base / "format-id1", format_boot, "--scsi1", image0,
        scsi_format_input_lines(1), SCSI_FORMAT_SCREEN, args.g75_exit_ms,
        both)
    fat0 = inspect_scsi_fat(image0)
    fat1 = inspect_scsi_fat(image1)
    expected = d88_find_file(support, "BIN/SCFORM.COM")
    files_before = {
        "C": scsi_root_files(image0, fat0),
        "D": scsi_root_files(image1, fat1),
    }
    if files_before["C"] or files_before["D"]:
        raise AssertionError("fresh two-disk SCSI images are not empty")

    disks = (
        {"id": 0, "drive": "C", "image": image0, "fat": fat0,
         "name": SCSI_ID0_FILE, "backup": SCSI_ID0_BACKUP_FILE},
        {"id": 1, "drive": "D", "image": image1, "fat": fat1,
         "name": SCSI_ID1_FILE, "backup": SCSI_ID1_BACKUP_FILE},
    )
    create_results = {}
    image_hashes = {"C": sha256(image0), "D": sha256(image1)}
    for disk in disks:
        display = dos_display_name(disk["name"])
        result = run_media_guest(
            args, base / f"create-id{disk['id']}", boot, "--scsi1", image0,
            scsi_two_disk_create_input_lines(disk["drive"], disk["name"]),
            (display,), args.g75_io_exit_ms, both)
        payload = scsi_file_bytes(disk["image"], disk["fat"], disk["name"])
        if payload != expected:
            raise AssertionError(
                f"SCSI ID {disk['id']} create payload differs from SCFORM.COM")
        other = "D" if disk["drive"] == "C" else "C"
        if sha256(image1 if other == "D" else image0) != image_hashes[other]:
            raise AssertionError(
                f"SCSI ID {disk['id']} write changed the other disk")
        image_hashes[disk["drive"]] = sha256(disk["image"])
        create_results[str(disk["id"])] = result

    readback_results = {}
    for disk in disks:
        display = dos_display_name(disk["backup"])
        result = run_media_guest(
            args, base / f"readback-id{disk['id']}", boot, "--scsi1", image0,
            scsi_two_disk_readback_input_lines(
                disk["drive"], disk["name"], disk["backup"]),
            (display,), args.g75_io_exit_ms, both)
        backup = d88_find_file(boot, disk["backup"])
        if backup != expected:
            raise AssertionError(
                f"SCSI ID {disk['id']} readback differs from SCFORM.COM")
        readback_results[str(disk["id"])] = result

    delete_results = {}
    for disk in disks:
        display = dos_display_name(disk["name"])
        result = run_media_guest(
            args, base / f"delete-id{disk['id']}", boot, "--scsi1", image0,
            scsi_two_disk_delete_input_lines(disk["drive"], disk["name"]),
            (f"DEL {disk['drive']}:\\{disk['name']}",
             "該当するファイルはありません"),
            args.g75_io_exit_ms, both)
        if any(display in line for line in result["screen_lines"]):
            raise AssertionError(
                f"SCSI ID {disk['id']} file remains in guest DIR")
        if scsi_file_bytes(disk["image"], disk["fat"], disk["name"]) is not None:
            raise AssertionError(
                f"SCSI ID {disk['id']} file remains in the image")
        delete_results[str(disk["id"])] = result

    final0 = inspect_scsi_fat(image0)
    final1 = inspect_scsi_fat(image1)
    remaining0 = scsi_root_files(image0, final0)
    remaining1 = scsi_root_files(image1, final1)
    if remaining0 or remaining1:
        raise AssertionError("two-disk SCSI root directories are not empty")
    for label, info in (("SCSI ID 0", final0), ("SCSI ID 1", final1)):
        copies = info["fat"]["copies"]
        if copies[0]["free_entries"] != copies[1]["free_entries"]:
            raise AssertionError(f"{label} FAT copies disagree after delete")
        if copies[0]["free_entries"] <= 0:
            raise AssertionError(f"{label} has no free clusters after delete")
    return {
        "mode": "g75-scsi-two",
        "support_d88": str(support),
        "config_schd": ["-I0", "-I1"],
        "source_file": "A:\\BIN\\SCFORM.COM",
        "source_file_sha256": hashlib.sha256(expected).hexdigest(),
        "created_images": {"id0": created0, "id1": created1},
        "format": {"id0": format0, "id1": format1},
        "create": create_results,
        "readback_after_reopen": readback_results,
        "delete_after_reopen": delete_results,
        "fat_after_delete": {
            "id0": {
                "fat_type": final0["selected"]["fat_type"],
                "cluster_count": final0["selected"]["cluster_count"],
                "free_clusters_fat1": final0["fat"]["copies"][0]["free_entries"],
                "free_clusters_fat2": final0["fat"]["copies"][1]["free_entries"],
            },
            "id1": {
                "fat_type": final1["selected"]["fat_type"],
                "cluster_count": final1["selected"]["cluster_count"],
                "free_clusters_fat1": final1["fat"]["copies"][0]["free_entries"],
                "free_clusters_fat2": final1["fat"]["copies"][1]["free_entries"],
            },
        },
        "remaining_files": {"id0": remaining0, "id1": remaining1},
        "boot_d88": str(boot),
        "images": {"id0": str(image0), "id1": str(image1)},
    }


def run_guest_io(args: argparse.Namespace) -> dict[str, object]:
    base = (Path(args.output_dir).resolve()
            if args.output_dir else
            Path(tempfile.mkdtemp(prefix="m75-storage-guest-io-")))
    base.mkdir(parents=True, exist_ok=True)
    read_root = base / "read"
    delete_root = base / "delete-rejected"
    if read_root.exists() or delete_root.exists():
        raise FileExistsError(
            f"guest-io output already contains read/delete directories: {base}")
    drive = args.hostfat_drive.upper()
    read = run_guest(
        args, read_root, guest_input_lines("TYPE", drive),
        (f"TYPE {drive}:\\{HOSTFAT_FILE}", HOSTFAT_CONTENT.rstrip("\n")))
    delete = run_guest(
        args, delete_root, delete_input_lines(drive),
        (f"DEL {drive}:\\{HOSTFAT_FILE}", "削除できません", HOSTFAT_FILE))
    return {
        "mode": "guest-io",
        "output_dir": str(base),
        "hostfat_drive": drive,
        "read": read,
        "delete_rejected": delete,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Run the disposable M75 SASI/HOSTFAT storage regressions.")
    parser.add_argument("--selftest", action="store_true",
                        help="validate generated fixtures and input scripts")
    parser.add_argument("--guest-io", action="store_true",
                        help="run TYPE success and DEL rejection guest scenarios")
    parser.add_argument("--sasi-format", action="store_true",
                        help="run SASI HDFORM and the create/readback/delete lifecycle")
    parser.add_argument("--g75-scsi", action="store_true",
                        help="format a blank 40MiB SCSI image and run G75 lifecycle")
    parser.add_argument("--g75-scsi-two", action="store_true",
                        help="run create/readback/delete on SCSI IDs 0 and 1")
    parser.add_argument("--full-g75", action="store_true",
                        help="run SASI HDFORM and one- and two-disk SCSI lifecycles")
    parser.add_argument("--worker")
    parser.add_argument("--support-d88")
    parser.add_argument("--sasi-source",
                        help="PC-Engine 1.1 D88 containing HDFORM.COM")
    parser.add_argument("--roms")
    parser.add_argument("--model", default="va2", choices=("va", "va2"))
    parser.add_argument("--output-dir",
                        help="retain fixtures and same-run screen/trace pairs")
    parser.add_argument("--headless-input-script")
    parser.add_argument("--hostfat-drive", default="D",
                        help="DOS drive letter assigned to HOSTFAT (default: D)")
    parser.add_argument("--timeout", type=float, default=900.0)
    parser.add_argument("--exit-ms", type=int, default=30000,
                        help="guest screen-harness exit delay (default: 30000)")
    parser.add_argument("--g75-exit-ms", type=int, default=180000,
                        help="format-run exit delay (default: 180000)")
    parser.add_argument("--g75-io-exit-ms", type=int, default=30000,
                        help="G75 file-I/O run exit delay (default: 30000)")
    args = parser.parse_args(argv)
    if args.selftest:
        print(json.dumps(fixture_selftest(), indent=2, sort_keys=True))
        return 0
    modes = sum(bool(value) for value in (
        args.guest_io, args.sasi_format, args.g75_scsi,
        args.g75_scsi_two, args.full_g75))
    if modes > 1:
        parser.error("guest regression modes are mutually exclusive")
    missing = [name for name in ("--worker", "--support-d88", "--roms")
               if getattr(args, name[2:].replace("-", "_")) is None]
    if missing:
        parser.error("guest run requires " + ", ".join(missing))
    if args.sasi_format or args.full_g75:
        if args.sasi_source is None:
            parser.error("--sasi-format/--full-g75 requires --sasi-source")
        if not Path(args.sasi_source).is_file():
            parser.error(f"SASI source D88 not found: {args.sasi_source}")
    validate_guest_inputs(args, require_hostfat=args.guest_io)
    if args.guest_io:
        if args.headless_input_script:
            parser.error("--guest-io generates its own headless input scripts")
        result = run_guest_io(args)
    elif args.sasi_format:
        result = run_sasi_format(args)
    elif args.g75_scsi:
        result = run_g75_scsi(args)
    elif args.g75_scsi_two:
        result = run_g75_scsi_two(args)
    elif args.full_g75:
        root = (Path(args.output_dir).resolve()
                if args.output_dir else
                Path(tempfile.mkdtemp(prefix="m75-full-g75-")))
        if root.exists() and any(root.iterdir()):
            raise FileExistsError(f"full G75 output already contains files: {root}")
        root.mkdir(parents=True, exist_ok=True)
        original = args.output_dir
        args.output_dir = str(root)
        sasi_result = run_sasi_format(args)
        args.output_dir = str(root)
        scsi_result = run_g75_scsi(args)
        args.output_dir = str(root)
        scsi_two_result = run_g75_scsi_two(args)
        args.output_dir = original
        result = {
            "mode": "full-g75",
            "output_dir": str(root),
            "sasi": sasi_result,
            "scsi": scsi_result,
            "scsi_two": scsi_two_result,
        }
    else:
        output_root = (Path(args.output_dir).resolve()
                       if args.output_dir else
                       Path(tempfile.mkdtemp(prefix="m75-storage-regression-")))
        result = run_guest(args, output_root)
    print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, FileExistsError, FileNotFoundError,
            RuntimeError, ValueError) as error:
        print(f"M75_STORAGE_REGRESSION_FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)

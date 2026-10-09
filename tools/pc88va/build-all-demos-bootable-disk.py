#!/usr/bin/env python3
"""Build one bootable PC-Engine D88 containing all demo distributions.

The checked-in demo distributions are non-bootable data D88 images.  This
builder extracts their files into collision-free profile directories, then
installs that payload on a user-supplied PC-Engine system disk.  The source
system image and the generated bootable D88 are local artifacts.
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
import importlib.util
import lzma
import os
import struct
import tempfile
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
PCENGINE_DISK_PATH = Path(__file__).with_name("pcengine_disk.py")
DEFAULT_DISTRIBUTION_DIR = REPOSITORY_ROOT / "demos" / "disks"
DEFAULT_DISTRIBUTIONS = (
    ("glass-orbit.d88.xz", ("GLASS",)),
    ("neon3-distribution.d88.xz", ("NEON3",)),
    ("neon4-distribution.d88.xz", ("NEON4/16", "NEON4/65536")),
    ("sgp-pseudo-sprite.d88.xz", ("SPRITE/16", "SPRITE/256", "SPRITE/65536")),
    ("sgp-wireframe.d88.xz", ("WIRE/16", "WIRE/256", "WIRE/65536")),
    ("zundamon-orbit.d88.xz", ("ZUNDAMON",)),
)


class BuildError(Exception):
    """A deterministic input or layout error."""


def load_pcengine_module():
    spec = importlib.util.spec_from_file_location(
        "pc88va_all_demos_pcengine_disk", PCENGINE_DISK_PATH)
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {PCENGINE_DISK_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def read_directory(disk, module, directory_cluster=None):
    if directory_cluster is None:
        directory = disk.root
    else:
        directory = bytearray()
        for cluster in disk.cluster_chain(directory_cluster):
            directory.extend(disk.read_cluster(cluster))
    return list(module.iter_entries(directory))


def read_file(disk, entry):
    cluster = struct.unpack_from("<H", entry, 26)[0]
    size = struct.unpack_from("<I", entry, 28)[0]
    contents = bytearray()
    for item in disk.cluster_chain(cluster):
        contents.extend(disk.read_cluster(item))
    return bytes(contents[:size])


def extract_distribution(path, target_names, payload_root, module):
    """Extract one distribution into one or more collision-free directories."""
    try:
        with lzma.open(path, "rb") as stream:
            disk = module.PcEngineDisk(stream.read(), require_system_files=False)
    except (OSError, lzma.LZMAError, module.DiskError) as error:
        raise BuildError(f"cannot read distribution {path}: {error}") from error

    root_entries = read_directory(disk, module)
    directories = []
    root_files = []
    for _, entry in root_entries:
        if entry[11] & 0x08:
            continue
        name = module.display_name(entry[:11])
        if name in (".", ".."):
            continue
        if entry[11] & 0x10:
            directories.append((name, entry))
        else:
            root_files.append((name, entry))

    if len(target_names) != max(1, len(directories)):
        raise BuildError(
            f"{path.name}: expected {len(target_names)} profile directories, "
            f"found {len(directories)}")
    if len(directories) > 1 and root_files:
        raise BuildError(f"{path.name}: mixed root files and profile directories")

    if directories:
        ordered = sorted(directories, key=lambda item: item[0].upper())
        for target_name, (_, entry) in zip(target_names, ordered):
            destination = payload_root / target_name
            destination.mkdir(parents=True, exist_ok=False)
            for _, child in read_directory(
                    disk, module, struct.unpack_from("<H", entry, 26)[0]):
                if child[11] & 0x08:
                    continue
                name = module.display_name(child[:11])
                if name in (".", ".."):
                    continue
                if child[11] & 0x10:
                    raise BuildError(
                        f"{path.name}: nested directory is unsupported: {name}")
                output = destination / name
                if output.exists():
                    raise BuildError(f"duplicate output file: {output}")
                output.write_bytes(read_file(disk, child))
    else:
        destination = payload_root / target_names[0]
        destination.mkdir(parents=True, exist_ok=False)
        for name, entry in root_files:
            output = destination / name
            if output.exists():
                raise BuildError(f"duplicate output file: {output}")
            output.write_bytes(read_file(disk, entry))


def build(source, distribution_dir, output):
    module = load_pcengine_module()
    source = Path(source)
    distribution_dir = Path(distribution_dir)
    output = Path(output)
    if output.exists():
        raise BuildError(f"output already exists: {output}")
    if not source.is_file():
        raise BuildError(f"source system D88 is not readable: {source}")
    if not distribution_dir.is_dir():
        raise BuildError(f"distribution directory is not readable: {distribution_dir}")

    with tempfile.TemporaryDirectory(prefix="vaeg-all-demos-") as temporary:
        temporary_root = Path(temporary)
        payload_root = temporary_root / "payload"
        payload_root.mkdir()
        for filename, target_names in DEFAULT_DISTRIBUTIONS:
            distribution = distribution_dir / filename
            if not distribution.is_file():
                raise BuildError(f"distribution is not readable: {distribution}")
            extract_distribution(distribution, target_names, payload_root, module)

        vanilla = temporary_root / "vanilla.d88"
        try:
            module.create_vanilla(str(source), str(vanilla))
            module.install_payload(str(vanilla), str(payload_root))
        except (OSError, module.DiskError) as error:
            raise BuildError(f"could not install demo payload: {error}") from error
        image = vanilla.read_bytes()

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
    return len(image), hashlib.sha256(image).hexdigest()


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Build one bootable PC-Engine D88 containing all demo distributions")
    parser.add_argument(
        "--source", required=True, type=Path,
        help="user-supplied PC-Engine 1.05 or 1.1 boot D88")
    parser.add_argument(
        "--distribution-dir", type=Path, default=DEFAULT_DISTRIBUTION_DIR,
        help="directory containing the checked-in demo .d88.xz distributions")
    parser.add_argument(
        "--output", required=True, type=Path,
        help="new bootable D88 path (must not already exist)")
    args = parser.parse_args(argv)
    try:
        size, digest = build(args.source.resolve(),
                             args.distribution_dir.resolve(),
                             args.output.resolve())
    except (BuildError, OSError) as error:
        parser.exit(1, f"error: {error}\n")
    print("Created bootable all-demo PC-Engine D88")
    print(f"output: {args.output.resolve()}")
    print(f"size: {size} bytes")
    print(f"SHA-256: {digest}")
    print("directories: GLASS NEON3 NEON4/16 NEON4/65536 SPRITE/16 "
          "SPRITE/256 SPRITE/65536 WIRE/16 WIRE/256 WIRE/65536 "
          "ZUNDAMON")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

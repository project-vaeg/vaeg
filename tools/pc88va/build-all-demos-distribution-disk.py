#!/usr/bin/env python3
"""Build one non-bootable PC-Engine D88 containing all demo distributions."""

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
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import lzma
import os
import tempfile
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
BOOTABLE_BUILDER = Path(__file__).with_name(
    "build-all-demos-bootable-disk.py"
)
DEFAULT_DISTRIBUTION_DIR = REPOSITORY_ROOT / "demos" / "disks"
DEFAULT_COMPRESSED_OUTPUT = DEFAULT_DISTRIBUTION_DIR / "all-demos.d88.xz"


class BuildError(Exception):
    """A deterministic input or layout error."""


def load_bootable_builder():
    spec = importlib.util.spec_from_file_location(
        "pc88va_all_demos_bootable_builder", BOOTABLE_BUILDER
    )
    if spec is None or spec.loader is None:
        raise BuildError(f"could not load {BOOTABLE_BUILDER}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write_new_file(path, contents):
    path = Path(path)
    if path.exists():
        raise BuildError(f"output already exists: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
                dir=path.parent, prefix=path.name + ".tmp.", delete=False
        ) as stream:
            temporary = Path(stream.name)
            stream.write(contents)
        os.chmod(temporary, 0o644)
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def build(source, distribution_dir, output, compressed_output):
    helper = load_bootable_builder()
    module = helper.load_pcengine_module()
    source = Path(source)
    distribution_dir = Path(distribution_dir)
    output = Path(output)
    compressed_output = Path(compressed_output)
    if not source.is_file():
        raise BuildError(f"source system D88 is not readable: {source}")
    if not distribution_dir.is_dir():
        raise BuildError(
            f"distribution directory is not readable: {distribution_dir}"
        )
    if output.exists():
        raise BuildError(f"output already exists: {output}")
    if compressed_output.exists():
        raise BuildError(
            f"compressed output already exists: {compressed_output}"
        )

    with tempfile.TemporaryDirectory(prefix="vaeg-all-demos-data-") as temporary:
        temporary_root = Path(temporary)
        payload_root = temporary_root / "payload"
        payload_root.mkdir()
        for filename, target_names in helper.DEFAULT_DISTRIBUTIONS:
            distribution = distribution_dir / filename
            if not distribution.is_file():
                raise BuildError(f"distribution is not readable: {distribution}")
            helper.extract_distribution(
                distribution, target_names, payload_root, module
            )

        data_disk = temporary_root / "data.d88"
        try:
            module.create_data_disk(str(source), str(data_disk))
            module.install_payload(str(data_disk), str(payload_root))
        except (OSError, module.DiskError) as error:
            raise BuildError(f"could not install demo payload: {error}") from error
        image = data_disk.read_bytes()

    compressed = lzma.compress(
        image, format=lzma.FORMAT_XZ, preset=lzma.PRESET_EXTREME | 9
    )
    write_new_file(output, image)
    write_new_file(compressed_output, compressed)
    return len(image), hashlib.sha256(image).hexdigest()


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Build one non-bootable PC-Engine D88 containing all demos"
    )
    parser.add_argument(
        "--source", required=True, type=Path,
        help="user-supplied PC-Engine 1.05 or 1.1 D88 template"
    )
    parser.add_argument(
        "--distribution-dir", type=Path, default=DEFAULT_DISTRIBUTION_DIR,
        help="directory containing demo .d88.xz distributions"
    )
    parser.add_argument(
        "--output", required=True, type=Path,
        help="new non-bootable D88 path (must not already exist)"
    )
    parser.add_argument(
        "--compressed-output", type=Path, default=DEFAULT_COMPRESSED_OUTPUT,
        help="compressed distribution path (must not already exist)"
    )
    args = parser.parse_args(argv)
    try:
        size, digest = build(
            args.source.resolve(), args.distribution_dir.resolve(),
            args.output.resolve(), args.compressed_output.resolve()
        )
    except (BuildError, OSError) as error:
        parser.exit(1, f"error: {error}\n")
    print("Created non-bootable all-demo PC-Engine D88")
    print(f"output: {args.output.resolve()}")
    print(f"compressed: {args.compressed_output.resolve()}")
    print(f"size: {size} bytes")
    print(f"SHA-256: {digest}")
    print("directories: GLASS NEON3 NEON4/16 NEON4/65536 SPRITE/16 "
          "SPRITE/256 SPRITE/65536 WIRE/16 WIRE/256 WIRE/65536 "
          "ZUNDAMON")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

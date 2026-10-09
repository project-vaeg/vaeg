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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Build one private 96x128 VA8 atlas from a PSD composite."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from typing import NoReturn

import convert_zundamon_orbit_va8 as va8_converter
import generate_zundamon_orbit_scales as scaler
import pack_zundamon_orbit_atlas as packer


SOURCE_WIDTH = 96
SOURCE_HEIGHT = 128
ANCHOR_X = 48
# Center the full-body sprite inside the accepted 64-phase orbit bounds.
ANCHOR_Y = 64
RGB_BYTES = SOURCE_WIDTH * SOURCE_HEIGHT * 3
BACKGROUND = (0, 0, 0)


class PsdAtlasError(Exception):
    """A stable PSD-to-atlas conversion failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise PsdAtlasError(code, detail)


def read_psd_rgb(input_file: Path, convert_command: str) -> bytes:
    if not input_file.is_file() or input_file.is_symlink():
        fail("M98AB_PSD_INPUT", "PSD input must be a regular file")
    executable = shutil.which(convert_command)
    if executable is None:
        fail("M98AB_CONVERT_MISSING", "ImageMagick convert is unavailable")
    command = [
        executable,
        f"{input_file}[0]",
        "-thumbnail", "1082x1650",
        "-background", "#000000",
        "-alpha", "background",
        "-resize", f"{SOURCE_WIDTH}x{SOURCE_HEIGHT}!",
        "-depth", "8",
        "RGB:-",
    ]
    try:
        result = subprocess.run(
            command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            check=False,
        )
    except OSError as error:
        raise PsdAtlasError(
            "M98AB_CONVERT_EXEC", "ImageMagick could not be started") from error
    if result.returncode != 0:
        fail("M98AB_PSD_DECODE", "PSD composite could not be decoded")
    if len(result.stdout) != RGB_BYTES:
        fail("M98AB_RGB_SIZE", "PSD composite did not produce 96x128 RGB data")
    return result.stdout


def convert_rgb_to_va8(rgb_pixels: bytes) -> bytes:
    if len(rgb_pixels) != RGB_BYTES:
        fail("M98AB_RGB_SIZE", "RGB input length differs")
    output = bytearray()
    for offset in range(0, len(rgb_pixels), 3):
        rgb = tuple(rgb_pixels[offset:offset + 3])
        if rgb == BACKGROUND:
            output.append(va8_converter.TRANSPARENT_VALUE)
        else:
            value, _repaired = va8_converter.convert_opaque_rgb(rgb)
            output.append(value)
    return bytes(output)


def build_atlas(rgb_pixels: bytes) -> bytes:
    va8_pixels = convert_rgb_to_va8(rgb_pixels)
    scale_set = scaler.build_scale_set(
        va8_pixels, SOURCE_WIDTH, SOURCE_HEIGHT, ANCHOR_X, ANCHOR_Y,
    )
    return packer.build_atlas(scale_set).contents


def write_atlas(input_file: Path, output_file: Path,
                convert_command: str) -> None:
    if output_file.exists():
        fail("M98AB_OUTPUT_EXISTS", "output file already exists")
    contents = build_atlas(read_psd_rgb(input_file, convert_command))
    try:
        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_bytes(contents)
    except OSError as error:
        raise PsdAtlasError(
            "M98AB_OUTPUT", "atlas output could not be written") from error


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="one PSD input file")
    parser.add_argument("output", type=Path, help="new zundamon.bin output")
    arguments = parser.parse_args(argv)
    try:
        write_atlas(arguments.input, arguments.output, "convert")
    except (PsdAtlasError, va8_converter.ConversionError,
            scaler.ScaleError, packer.PackingError) as error:
        print(error, file=sys.stderr)
        return 1
    print(
        "M98AB_IDA_ATLAS_PASS source=96x128x256 "
        f"output_bytes={arguments.output.stat().st_size}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

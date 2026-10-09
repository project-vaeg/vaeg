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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Validate a source-neutral Zundamon orbit local-input manifest."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import NoReturn


SCHEMA_ID = "vaeg-zundamon-orbit-input-v1"
SCHEMA_VERSION = 1
COPYRIGHT = "Copyright (c) 2026 Nakata Maho"
LICENSE = "BSD-2-Clause"
MAX_MANIFEST_BYTES = 65536
MAX_CROP_DIMENSION = 4096
MAX_SOURCE_COORDINATE = 65535
LOCAL_BASENAME = re.compile(r"[a-z0-9][a-z0-9._-]{0,63}\.(?:bmp|rgb)")

ROOT_KEYS = frozenset((
    "anchor",
    "copyright",
    "crop",
    "image",
    "license",
    "palette",
    "schema",
    "schema_version",
    "transparency",
))
IMAGE_KEYS = frozenset(("encoding", "path"))
PALETTE_KEYS = frozenset((
    "encoding",
    "entries",
    "path",
    "reserved_index",
    "transparent_index",
))
CROP_KEYS = frozenset(("height", "width", "x", "y"))
TRANSPARENCY_KEYS = frozenset(("background_rgb", "method"))
ANCHOR_KEYS = frozenset(("space", "x", "y"))


class ManifestError(Exception):
    """A stable, fail-closed M98c manifest validation failure."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code


def fail(code: str, detail: str) -> NoReturn:
    raise ManifestError(code, detail)


def reject_duplicate_pairs(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            fail("M98C_JSON_DUPLICATE_KEY", "duplicate object member")
        result[key] = value
    return result


def parse_manifest_bytes(contents: bytes) -> object:
    if len(contents) > MAX_MANIFEST_BYTES:
        fail("M98C_MANIFEST_SIZE", "manifest exceeds 65536 bytes")
    if contents.startswith(b"\xef\xbb\xbf"):
        fail("M98C_MANIFEST_BOM", "UTF-8 BOM is not permitted")
    try:
        text = contents.decode("utf-8")
    except UnicodeDecodeError:
        fail("M98C_MANIFEST_UTF8", "manifest is not valid UTF-8")
    try:
        return json.loads(text, object_pairs_hook=reject_duplicate_pairs)
    except ManifestError:
        raise
    except json.JSONDecodeError:
        fail("M98C_MANIFEST_JSON", "manifest is not valid JSON")


def read_manifest(input_file: Path) -> object:
    try:
        with input_file.open("rb") as stream:
            contents = stream.read(MAX_MANIFEST_BYTES + 1)
    except OSError:
        fail("M98C_MANIFEST_READ", "manifest could not be read")
    return parse_manifest_bytes(contents)


def require_object(value: object, code: str) -> dict[str, object]:
    if not isinstance(value, dict):
        fail(code, "value must be an object")
    return value


def require_exact_keys(value: dict[str, object], expected: frozenset[str], code: str) -> None:
    if frozenset(value) != expected:
        fail(code, "required member set differs")


def require_integer(value: object, minimum: int, maximum: int, code: str) -> int:
    if type(value) is not int or not minimum <= value <= maximum:
        fail(code, "integer is outside the permitted range")
    return value


def require_constant(value: object, expected: object, code: str) -> None:
    if value != expected or type(value) is not type(expected):
        fail(code, "constant value differs")


def require_local_filename(value: object, suffix: str, code: str) -> None:
    if not isinstance(value, str):
        fail(code, "path must be a string")
    if LOCAL_BASENAME.fullmatch(value) is None or not value.endswith(suffix):
        fail(code, "path must be a lowercase local basename")


def validate_manifest(value: object) -> None:
    root = require_object(value, "M98C_ROOT_TYPE")
    require_exact_keys(root, ROOT_KEYS, "M98C_ROOT_KEYS")
    require_constant(root["copyright"], COPYRIGHT, "M98C_COPYRIGHT")
    require_constant(root["license"], LICENSE, "M98C_LICENSE")
    require_constant(root["schema"], SCHEMA_ID, "M98C_SCHEMA")
    require_constant(root["schema_version"], SCHEMA_VERSION, "M98C_SCHEMA_VERSION")

    image = require_object(root["image"], "M98C_IMAGE_TYPE")
    require_exact_keys(image, IMAGE_KEYS, "M98C_IMAGE_KEYS")
    require_local_filename(image["path"], ".bmp", "M98C_IMAGE_PATH")
    require_constant(image["encoding"], "bmp32", "M98C_IMAGE_ENCODING")

    palette = require_object(root["palette"], "M98C_PALETTE_TYPE")
    require_exact_keys(palette, PALETTE_KEYS, "M98C_PALETTE_KEYS")
    require_local_filename(palette["path"], ".rgb", "M98C_PALETTE_PATH")
    require_constant(palette["encoding"], "rgb888", "M98C_PALETTE_ENCODING")
    require_constant(palette["entries"], 16, "M98C_PALETTE_ENTRIES")
    require_constant(palette["transparent_index"], 0, "M98C_TRANSPARENT_INDEX")
    require_constant(palette["reserved_index"], 15, "M98C_RESERVED_INDEX")

    crop = require_object(root["crop"], "M98C_CROP_TYPE")
    require_exact_keys(crop, CROP_KEYS, "M98C_CROP_KEYS")
    crop_x = require_integer(crop["x"], 0, MAX_SOURCE_COORDINATE, "M98C_CROP_X")
    crop_y = require_integer(crop["y"], 0, MAX_SOURCE_COORDINATE, "M98C_CROP_Y")
    crop_width = require_integer(crop["width"], 1, MAX_CROP_DIMENSION,
                                 "M98C_CROP_WIDTH")
    crop_height = require_integer(crop["height"], 1, MAX_CROP_DIMENSION,
                                  "M98C_CROP_HEIGHT")
    if crop_x + crop_width > MAX_SOURCE_COORDINATE + 1:
        fail("M98C_CROP_RANGE", "horizontal crop range exceeds the contract")
    if crop_y + crop_height > MAX_SOURCE_COORDINATE + 1:
        fail("M98C_CROP_RANGE", "vertical crop range exceeds the contract")

    transparency = require_object(root["transparency"], "M98C_TRANSPARENCY_TYPE")
    require_exact_keys(transparency, TRANSPARENCY_KEYS, "M98C_TRANSPARENCY_KEYS")
    require_constant(transparency["method"], "exact-rgb", "M98C_TRANSPARENCY_METHOD")
    background = transparency["background_rgb"]
    if not isinstance(background, list) or len(background) != 3:
        fail("M98C_BACKGROUND_RGB", "background must contain exactly three channels")
    for channel in background:
        require_integer(channel, 0, 255, "M98C_BACKGROUND_CHANNEL")

    anchor = require_object(root["anchor"], "M98C_ANCHOR_TYPE")
    require_exact_keys(anchor, ANCHOR_KEYS, "M98C_ANCHOR_KEYS")
    require_constant(anchor["space"], "crop-top-left", "M98C_ANCHOR_SPACE")
    anchor_x = require_integer(anchor["x"], 0, MAX_CROP_DIMENSION - 1,
                               "M98C_ANCHOR_X")
    anchor_y = require_integer(anchor["y"], 0, MAX_CROP_DIMENSION - 1,
                               "M98C_ANCHOR_Y")
    if anchor_x >= crop_width or anchor_y >= crop_height:
        fail("M98C_ANCHOR_BOUNDS", "anchor lies outside the crop")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True,
                        help="local input manifest to validate")
    arguments = parser.parse_args(argv)
    try:
        validate_manifest(read_manifest(arguments.input))
    except ManifestError as error:
        print(error, file=sys.stderr)
        return 1
    print("M98C_MANIFEST_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

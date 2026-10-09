#!/usr/bin/env python3
#
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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import io
import os
import tempfile
import zipfile
from pathlib import Path


FIXED_DATE_TIME = (2026, 6, 28, 2, 54, 10)


class ArchiveError(Exception):
    pass


def create_archive(source, output, comment):
    source_path = Path(source)
    output_path = Path(output)
    if not source_path.is_dir():
        raise ArchiveError("source directory does not exist")
    if output_path.exists():
        raise ArchiveError("output already exists; refusing to overwrite it")
    if not output_path.parent.is_dir():
        raise ArchiveError("output directory does not exist")

    files = sorted(path for path in source_path.rglob("*") if path.is_file())
    if not files:
        raise ArchiveError("source directory contains no files")
    if any(path.is_symlink() for path in source_path.rglob("*")):
        raise ArchiveError("symbolic links are unsupported")

    archive_bytes = io.BytesIO()
    with zipfile.ZipFile(archive_bytes, "w", compression=zipfile.ZIP_STORED) as archive:
        archive.comment = comment.encode("ascii")
        for path in files:
            relative = path.relative_to(source_path).as_posix()
            info = zipfile.ZipInfo(relative, FIXED_DATE_TIME)
            info.compress_type = zipfile.ZIP_STORED
            info.create_system = 0
            info.external_attr = 0x20
            archive.writestr(info, path.read_bytes())

    temporary_name = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=output_path.parent,
            prefix=f"{output_path.name}.tmp.",
            delete=False,
        ) as temporary:
            temporary_name = temporary.name
            temporary.write(archive_bytes.getvalue())
        os.chmod(temporary_name, 0o644)
        os.replace(temporary_name, output_path)
    finally:
        if temporary_name and os.path.exists(temporary_name):
            os.unlink(temporary_name)


def main():
    parser = argparse.ArgumentParser(
        description="Create a reproducible uncompressed ZIP archive"
    )
    parser.add_argument("--source", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--comment", default="")
    args = parser.parse_args()
    try:
        create_archive(args.source, args.output, args.comment)
    except (ArchiveError, OSError, UnicodeEncodeError) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()

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

"""Focused tests for the ADR-0015 source/release archive modes."""

import io
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))

import check_zex_archive as checker  # noqa: E402

ZEX_FILES = {
    path: (ROOT / path).read_bytes() for path in checker.SOURCE_ARCHIVE_ZEX_FILES
}
PLAIN = ("README.md", b"plain file\n")


def codes(violations):
    return sorted(violation.split(" ", 1)[0] for violation in violations)


def inspect(members, mode):
    violations = []
    for name, data in members:
        checker.inspect_member(name, data, violations, mode)
    return violations


class SourceModeTest(unittest.TestCase):
    def fixture(self):
        return [PLAIN, *ZEX_FILES.items()]

    def test_fixture_passes(self):
        self.assertEqual(inspect(self.fixture(), checker.MODE_SOURCE), [])

    def test_prefixed_archive_passes(self):
        members = [("vaeg-1/" + name, data) for name, data in self.fixture()]
        self.assertEqual(inspect(members, checker.MODE_SOURCE), [])

    def test_recorded_name_elsewhere(self):
        members = self.fixture()
        members.append(("tools/zexdoc.src", ZEX_FILES["external/zex/zexdoc.src"]))
        self.assertEqual(
            codes(inspect(members, checker.MODE_SOURCE)), ["ZEX_CONTENT", "ZEX_NAME"]
        )

    def test_modified_recorded_file(self):
        members = self.fixture()
        members[1] = (members[1][0], members[1][1] + b"; edited\n")
        self.assertEqual(codes(inspect(members, checker.MODE_SOURCE)), ["ZEX_NAME"])

    def test_stock_binary_rejected(self):
        members = self.fixture()
        members.append(("external/zex/zexdoc.cim", b"\x00"))
        self.assertEqual(codes(inspect(members, checker.MODE_SOURCE)), ["ZEX_NAME"])

    def test_unrecorded_external_root(self):
        members = self.fixture()
        members.append(("external/other/file.c", b"int x;\n"))
        self.assertEqual(
            codes(inspect(members, checker.MODE_SOURCE)), ["EXTERNAL_ROOT"]
        )


class ReleaseModeTest(unittest.TestCase):
    def test_plain_package_passes(self):
        self.assertEqual(inspect([PLAIN], checker.MODE_RELEASE), [])

    def test_recorded_source_rejected(self):
        name = "external/zex/zexdoc.src"
        violations = inspect([PLAIN, (name, ZEX_FILES[name])], checker.MODE_RELEASE)
        self.assertEqual(codes(violations), ["ZEX_CONTENT", "ZEX_NAME"])

    def test_default_mode_is_release(self):
        name = "external/zex/LICENSE.txt"
        violations = []
        checker.inspect_member(name, ZEX_FILES[name], violations)
        self.assertEqual(codes(violations), ["ZEX_CONTENT"])


class IntegrationTest(unittest.TestCase):
    def run_checker(self, mode, archive):
        return subprocess.run(
            [sys.executable, str(HERE / "check_zex_archive.py"), "--mode", mode, str(archive)],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_modes_on_one_archive(self):
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / "source.tar.gz"
            with tarfile.open(archive, "w:gz") as output:
                for name, data in [PLAIN, *ZEX_FILES.items()]:
                    info = tarfile.TarInfo(name)
                    info.size = len(data)
                    output.addfile(info, io.BytesIO(data))
            source = self.run_checker("source", archive)
            self.assertEqual(source.returncode, 0, source.stdout)
            release = self.run_checker("release", archive)
            self.assertEqual(release.returncode, 1, release.stdout)
            self.assertIn("ZEX_NAME", release.stdout)


if __name__ == "__main__":
    unittest.main()

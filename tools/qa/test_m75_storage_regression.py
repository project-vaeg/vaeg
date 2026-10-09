#!/usr/bin/env python3
"""Focused tests for disposable M75 SASI/HOSTFAT regression fixtures."""

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
# INCLUDING NEGLIGENCE OR OTHERWISE ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import importlib.util
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("m75_storage_regression.py")
SPEC = importlib.util.spec_from_file_location("m75_storage_regression", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class StorageFixtureTest(unittest.TestCase):
    def test_fixture_selftest(self):
        result = MODULE.fixture_selftest()
        self.assertEqual(result["sasi"]["block_size"], 256)
        self.assertEqual(result["sasi"]["blocks"], 162360)
        self.assertEqual(result["sasi"]["size"], 41568256)
        self.assertEqual(result["hostfat"]["files"], 2)
        self.assertEqual(result["hostfat"]["directories"], 1)

    def test_guest_input_scripts(self):
        self.assertEqual(
            MODULE.guest_input_lines("TYPE", "D"),
            ["@wait 600", "TYPE D:\\REGRESS.TXT", "@wait 120"])
        self.assertEqual(
            MODULE.delete_input_lines("D"),
            ["@wait 600", "DEL D:\\REGRESS.TXT", "@wait 120",
             "DIR D:", "@wait 120"])

    def test_sasi_header_and_marker(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "sasi.hdi"
            MODULE.create_sasi_image(path)
            data = path.read_bytes()
            self.assertEqual(len(data), MODULE.SASI_FILE_SIZE)
            self.assertEqual(
                data[MODULE.SASI_HEADER_SIZE:
                     MODULE.SASI_HEADER_SIZE + len(MODULE.SASI_MARKER)],
                MODULE.SASI_MARKER)

    def test_g75_input_scripts(self):
        self.assertEqual(MODULE.sasi_format_input_lines()[1], "HDFORM C:")
        self.assertIn(MODULE.SASI_TEST_FILE,
                      MODULE.sasi_create_input_lines()[1])
        self.assertIn(MODULE.SASI_BACKUP_FILE,
                      MODULE.sasi_readback_input_lines()[3])
        self.assertIn(MODULE.SASI_TEST_FILE,
                      MODULE.sasi_delete_input_lines()[1])
        self.assertIn("G75TEST.COM", MODULE.scsi_create_input_lines()[1])
        self.assertIn("G75BACK.COM", MODULE.scsi_readback_input_lines()[3])
        self.assertIn("G75TEST.COM", MODULE.scsi_delete_input_lines()[1])
        self.assertEqual(MODULE.scsi_format_input_lines(1)[1], "1")
        self.assertIn(
            "G75ID0.COM",
            MODULE.scsi_two_disk_create_input_lines("C", "G75ID0.COM")[1])
        self.assertIn(
            "G75ID1.COM",
            MODULE.scsi_two_disk_create_input_lines("D", "G75ID1.COM")[1])
        self.assertIn(
            "G75I1BK.COM",
            MODULE.scsi_two_disk_readback_input_lines(
                "D", "G75ID1.COM", "G75I1BK.COM")[3])

    def test_scsi_image_helpers(self):
        self.assertEqual(MODULE.SCSI_TEST_FILE, "G75TEST.COM")
        self.assertEqual(MODULE.SCSI_BACKUP_FILE, "G75BACK.COM")
        self.assertEqual(MODULE.SCSI_ID0_FILE, "G75ID0.COM")
        self.assertEqual(MODULE.SCSI_ID1_FILE, "G75ID1.COM")

if __name__ == "__main__":
    unittest.main()

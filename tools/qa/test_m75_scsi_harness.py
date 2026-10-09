#!/usr/bin/env python3
"""Focused tests for the same-run M75 screen decoder."""

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

import importlib.util
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("m75_scsi_harness.py")
SPEC = importlib.util.spec_from_file_location("m75_scsi_harness", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ScreenDecoderTest(unittest.TestCase):
    def test_ascii_cell(self):
        self.assertEqual(MODULE.decode_cell(b"A\x00", 0), ("A", 1))

    def test_jis_cell(self):
        self.assertEqual(MODULE.jis_char(0x5005), "バ")
        self.assertEqual(MODULE.jis_char(0x3421), "全")


if __name__ == "__main__":
    unittest.main()

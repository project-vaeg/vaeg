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

"""Focused fail-closed tests for the P4 CPU-versus-SGP capture comparator."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).with_name("verify-p4-backends.py")
GVRAM_SIZE = 0x40000


def write_capture(directory: Path, prefix: str, marker: str, ip: str, raw: bytes) -> None:
    registers = {
        "schema": "vaeg-registers-v1",
        "ax": marker,
        "bx": "7ace",
        "cs": "3000",
        "ds": "3000",
        "es": "3000",
        "ss": "3000",
        "sp": "f000",
        "ip": ip,
        "flags": "0200",
    }
    directory.mkdir()
    (directory / f"{prefix}.registers.tsv").write_text(
        "".join(f"{key}\t{value}\n" for key, value in registers.items()), encoding="utf-8")
    (directory / "events.tsv").write_text("event\tframe\tid\tvalue\npc\t1\t-\t1\n", encoding="utf-8")
    (directory / f"{prefix}.gvram.bin").write_bytes(raw)
    (directory / f"{prefix}.screen.bmp").write_bytes(b"fixture-screen")


class P4BackendVerifierTests(unittest.TestCase):
    def run_fixture(self, mutate: bool) -> tuple[int, dict[str, object]]:
        raw = bytes(GVRAM_SIZE)
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            cpu = root / "cpu"
            sgp = root / "sgp"
            repeat = root / "repeat"
            write_capture(cpu, "glass-p4-cpu", "4750", "0200", raw)
            sgp_raw = bytearray(raw)
            if mutate:
                sgp_raw[0x1234] = 1
            write_capture(sgp, "glass-p4-sgp", "4753", "0280", bytes(sgp_raw))
            write_capture(repeat, "glass-p4-sgp", "4753", "0280", bytes(sgp_raw))
            completed = subprocess.run(
                [sys.executable, str(SCRIPT), str(cpu), str(sgp), str(repeat)],
                check=False,
                capture_output=True,
                text=True,
            )
            return completed.returncode, json.loads(completed.stdout)

    def test_matching_fixture_passes(self) -> None:
        returncode, result = self.run_fixture(mutate=False)
        self.assertEqual(returncode, 0)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["errors"], [])

    def test_one_byte_gvram_mutation_has_its_own_error(self) -> None:
        returncode, result = self.run_fixture(mutate=True)
        self.assertEqual(returncode, 1)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["errors"], ["P4_CPU_SGP_GVRAM_MISMATCH"])


if __name__ == "__main__":
    unittest.main()

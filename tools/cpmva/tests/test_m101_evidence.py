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

"""Content checks for the committed M101 QA outputs."""

import hashlib
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
QA = ROOT / "docs" / "agents" / "reports" / "m101_zexall_qa"
sys.path.insert(0, str(ROOT / "tools" / "cpmva" / "zex"))

import probe_results as pr  # noqa: E402

RUNS = ("vaeg", "pc88va2")
FILES = {
    "flagprb.txt", "zex13s.txt", "daadump.txt", "zexdoc.txt", "zexall.txt",
    "daa.bin", "cpl.bin", "scf.bin", "ccf.bin",
}


def manifest(run):
    entries = {}
    for line in (QA / run / "manifest.sha256").read_text(encoding="ascii").splitlines():
        digest, name = line.split(None, 1)
        entries[name] = digest
    return entries


class EvidenceTest(unittest.TestCase):
    def test_manifests_match_files(self):
        for run in RUNS:
            with self.subTest(run=run):
                entries = manifest(run)
                self.assertEqual(set(entries), {f"raw/{name}" for name in FILES})
                self.assertEqual({p.name for p in (QA / run / "raw").iterdir()}, FILES)
                for name, digest in entries.items():
                    data = (QA / run / name).read_bytes()
                    self.assertEqual(hashlib.sha256(data).hexdigest(), digest, name)

    def test_outputs_are_complete_and_consistent(self):
        for run in RUNS:
            with self.subTest(run=run):
                raw = QA / run / "raw"
                probes, expected = pr.parse_flagprb(pr.read_text(raw, "FLAGPRB.TXT"))
                self.assertEqual(sorted(p.ident for p in probes), sorted(expected))
                self.assertEqual(len(pr.parse_zex(pr.read_text(raw, "ZEX13S.TXT"))), 10)
                listed = pr.parse_daadump_txt(pr.read_text(raw, "DAADUMP.TXT"))
                for name in pr.DUMP_NAMES:
                    data = pr.find_file(raw, f"{name}.BIN").read_bytes()
                    self.assertEqual(listed[name], (pr.DUMP_SIZE, pr.crc32_hex(data)))

    def test_real_machine_matches_upd9002_expectations(self):
        raw = QA / "pc88va2" / "raw"
        for suite in ("zexdoc", "zexall"):
            with self.subTest(suite=suite):
                real = [pr.group_signature(g) for g in pr.parse_zex(pr.read_text(raw, f"{suite}.txt"))]
                expected = pr.load_expectation_file(
                    ROOT / "tests" / "z80_compat" / f"upd9002_{suite}_expected.txt"
                )
                self.assertEqual(real, expected)

    def test_real_machine_flag_probes_match_upd9002_column(self):
        probes, expected = pr.parse_flagprb(pr.read_text(QA / "pc88va2" / "raw", "FLAGPRB.TXT"))
        for probe in probes:
            for field, value in expected[probe.ident]["upd9002"].items():
                self.assertEqual(pr.expected_field(probe, field), value, (probe.ident, field))


if __name__ == "__main__":
    unittest.main()

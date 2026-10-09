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

"""Tests for the M101 probe output parsers."""

from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "zex"))

import probe_results as pr  # noqa: E402

FLAGPRB = (
    "P1 IN A=FF F=00 BC=0000 DE=0000 HL=0000 OUT A=0F F=04 BC=0000 DE=0000 HL=0000\r\n"
    "P7a IN A=00 F=00 BC=FFFF DE=0000 HL=0000 OUT A=FF F=D7 BC=FFD7 DE=0000 HL=0000\r\n"
    "# P1 AND 0Fh: ZILOG A=0F F=1C; UPD9002 A=0F F=04\r\n"
    "# P7a PUSH BC/POP AF/PUSH AF/POP BC: ZILOG C=FF; UPD9002 C=D7\r\n"
)
ZEX = (
    "Z80doc instruction exerciser\r\n"
    "aluop a,nn....................  ERROR **** crc expected:48799360 found:12967d59\r\n"
    "<daa,cpl,scf,ccf>.............  OK\r\n"
    "Tests complete\r\n"
)


class ParserTest(unittest.TestCase):
    def test_flagprb(self):
        probes, expected = pr.parse_flagprb(FLAGPRB.replace("\r\n", "\n"))
        self.assertEqual([p.ident for p in probes], ["P1", "P7a"])
        self.assertEqual(probes[1].outputs["BC"], "FFD7")
        self.assertEqual(expected["P1"]["upd9002"], {"A": "0F", "F": "04"})
        self.assertEqual(pr.expected_field(probes[1], "C"), "D7")

    def test_flagprb_rejects_unknown_line(self):
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_flagprb(FLAGPRB.replace("\r\n", "\n") + "garbage\n")
        self.assertEqual(context.exception.code, "FLAGPRB_FORMAT")

    def test_zex(self):
        groups = pr.parse_zex(ZEX.replace("\r\n", "\n"))
        self.assertEqual([pr.group_signature(g) for g in groups],
                         ["ERROR 48799360 12967d59", "OK"])
        self.assertEqual(groups[1].name, "<daa,cpl,scf,ccf>")
        self.assertEqual(pr.found_crc(groups[1], "9b4ba675"), "9b4ba675")

    def test_zex_incomplete(self):
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_zex(ZEX.replace("Tests complete", "").replace("\r\n", "\n"))
        self.assertEqual(context.exception.code, "ZEX_INCOMPLETE")

    def test_dump_layout(self):
        data = bytes(range(256)) * 128
        records = pr.dump_records(data)
        self.assertEqual(records[0x0101], (1, 1, 2, 3))
        self.assertEqual(pr.f_in(63), 0xD7)
        self.assertEqual(pr.f_in(0b100001), 0x81)

    def test_dump_size(self):
        with self.assertRaises(pr.ResultError) as context:
            pr.dump_records(b"\0" * 10)
        self.assertEqual(context.exception.code, "DUMP_SIZE")

    def test_daadump_txt_and_crc(self):
        text = "DAA.BIN 32768 940FA288\nCPL.BIN 32768 C4A07F70\n"
        self.assertEqual(pr.parse_daadump_txt(text)["CPL"], (32768, "C4A07F70"))
        self.assertEqual(pr.crc32_hex(b"123456789"), "CBF43926")

    def test_read_text_stops_at_eof_marker(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "flagprb.txt"
            path.write_bytes(FLAGPRB.encode() + b"\x1a\x1a\x1a")
            self.assertEqual(pr.read_text(Path(directory), "FLAGPRB.TXT"),
                             FLAGPRB.replace("\r\n", "\n"))


ED_LINE = ("ED 4C NEXT A=A6 F=93 BC=0B0C DE=0D0E HL=2AD8 IX=2ADC IY=2AE0 SP=2B04 "
           "M=11223344 X=11223344 Y=11223344 S=A1A2D75AE127A7A8")
E2_LINE = ("ED 64 1 RST  R=0331 L=0000 A=5B F=13 BC=0150 DE=3404 HL=3C04 IX=4404 "
           "IY=4C04 SP=5404 B=88898A8B8C8D8E8F D=98999A9B9C9D9E9F H=A8A9AAABACADAEAF "
           "X=B8B9BABBBCBDBEBF Y=C8C9CACBCCCDCECF S=D8D931037917DEDF O=CFCFCFCFCFCFCFCF")
CB_LINE = "DD CB 02 40 1 A=0A F=45 BC=0B0C DE=0D0E HL=4455 IX=5178 IY=5178 M=7E"


def ed_record(line):
    return (line.ljust(126) + "\r\n").encode("ascii")


class ProbeParserTest(unittest.TestCase):
    def test_edprb_records_and_partial_file(self):
        data = ed_record(ED_LINE) + ed_record(ED_LINE.replace("ED 4C", "ED 4D")) + b"\x1a" * 128
        records = pr.parse_edprb(data)
        self.assertEqual(sorted(records), [0x4C, 0x4D])
        self.assertEqual(records[0x4C], ED_LINE)

    def test_edprb_bad_record(self):
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_edprb(ed_record("garbage"))
        self.assertEqual(context.exception.code, "EDPRB_FORMAT")

    def test_edprb_restart_files_merge(self):
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "ED00.TXT").write_bytes(ed_record(ED_LINE))
            later = ED_LINE.replace("NEXT", "RET ")
            Path(directory, "ED4C.TXT").write_bytes(ed_record(later))
            self.assertEqual(pr.load_edprb(Path(directory)), {0x4C: later})

    def test_edprb2_records_merge_by_opcode_and_set(self):
        def record(line):
            return (line.ljust(254) + "\r\n").encode("ascii")

        other = E2_LINE.replace("ED 64 1", "ED 64 2")
        data = record(E2_LINE) + record(other) + b"\x1a" * 128
        self.assertEqual(sorted(pr.parse_edprb2(data)), [(0x64, 1), (0x64, 2)])
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "E200.TXT").write_bytes(record(E2_LINE))
            later = E2_LINE.replace("RST ", "FALL")
            Path(directory, "E264.TXT").write_bytes(record(later))
            Path(directory, "S200.TXT").write_bytes(record(other))
            self.assertEqual(pr.load_edprb2(Path(directory), "E2"), {(0x64, 1): later})
            self.assertEqual(pr.load_edprb2(Path(directory), "S2"), {(0x64, 2): other})
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_edprb2(record("ED 64 1 garbage"))
        self.assertEqual(context.exception.code, "EDPRB2_FORMAT")

    def test_edprb3(self):
        line = ("X00 5AD7 0130 3404 3C04 5404 2524 3534 6564 20 -> 2800 04")
        record = (line.ljust(126) + "\r\n").encode("ascii")
        records = pr.parse_edprb3(record + b"\x1a" * 128)
        self.assertEqual(records, {0: line})
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_edprb3(("X00 garbage".ljust(126) + "\r\n").encode("ascii"))
        self.assertEqual(context.exception.code, "EDPRB3_FORMAT")

    def test_edprb4(self):
        record = ("Y00 2329 00".ljust(14) + "\r\n").encode("ascii")
        self.assertEqual(pr.parse_edprb4(record + b"\x1a" * 16), {0: "Y00 2329 00"})
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_edprb4(("Y00 bad".ljust(14) + "\r\n").encode("ascii"))
        self.assertEqual(context.exception.code, "EDPRB4_FORMAT")

    def test_altprb(self):
        line = ("X1 O AF=0042 BC=0033 DE=13ED HL=F000 IX=1111 IY=2222 SP=0360 "
                "AF'=AA45 BC'=A1A2 DE'=B1B2 HL'=C1C2")
        record = (line.ljust(126) + "\r\n").encode("ascii")
        self.assertEqual(pr.parse_altprb(record + b"\x1a" * 128), {(1, "O"): line})
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_altprb(("X1 O garbage".ljust(126) + "\r\n").encode("ascii"))
        self.assertEqual(context.exception.code, "ALTPRB_FORMAT")

    def test_inprb(self):
        text = "# INPRB M102\r\nP0C 12 34 56 78\r\nC0B0C 12 34\r\n\x1a"
        lines = pr.parse_inprb(text)
        self.assertEqual(sorted(lines), ["C0B0C", "P0C"])
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_inprb("P0C garbage")
        self.assertEqual(context.exception.code, "INPRB_FORMAT")

    def test_cbprb(self):
        lines = pr.parse_cbprb(CB_LINE + "\n")
        self.assertEqual(lines, {("DD", 0x40, 1): CB_LINE})
        with self.assertRaises(pr.ResultError) as context:
            pr.parse_cbprb("DD CB 02 40 1 nonsense\n")
        self.assertEqual(context.exception.code, "CBPRB_FORMAT")


if __name__ == "__main__":
    unittest.main()

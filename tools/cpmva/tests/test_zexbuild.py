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

"""Tests for the ZEX dialect translator and program builder.

Assembler-dependent tests run when VAEG_Z80ASM or z80asm in PATH is z80asm 1.8.
"""

import importlib.util
import os
from pathlib import Path
import shutil
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "zex" / "zexbuild.py"
spec = importlib.util.spec_from_file_location("zexbuild", SCRIPT)
zexbuild = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(zexbuild)


def find_assembler():
    candidate = os.environ.get("VAEG_Z80ASM") or shutil.which("z80asm")
    if not candidate:
        return None
    try:
        zexbuild.check_assembler(candidate)
    except zexbuild.BuildError:
        return None
    return candidate


class TranslateTest(unittest.TestCase):
    def test_tstr_expansion(self):
        text = "\ttstr\t1,2,3,4,5,6,7,8,9,10,11,12,13\t; cycles"
        self.assertEqual(
            zexbuild.translate_zmac(text).split("\n"),
            ["\tdb\t1,2,3,4", "\tdw\t5,6,7,8,9,10", "\tdb\t11", "\tdb\t12", "\tdw\t13"],
        )

    def test_tmsg_padding(self):
        self.assertEqual(
            zexbuild.translate_zmac("\ttmsg\t'abc'").split("\n"),
            ["\tdb\t'abc" + "." * 27 + "'", "\tdb\t'$'"],
        )

    def test_labels_and_operands(self):
        text = "bdos\tpush\taf\nlbl:\tand\ta,0fh\n\tadd\ta,'0'\nx\tequ\t1"
        self.assertEqual(
            zexbuild.translate_zmac(text).split("\n"),
            ["bdos:\tpush\taf", "lbl:\tand\t0fh", "\tadd\ta,'0'", "x:\tequ\t1"],
        )

    def test_macro_and_disabled_blocks_dropped(self):
        text = "m\tmacro\tx\n\tdb\tx\n\tendm\n  if\t0\n\tnop\n  endif\n\tret"
        self.assertEqual(zexbuild.translate_zmac(text), "\tret")

    def test_tstr_field_count(self):
        with self.assertRaises(zexbuild.BuildError) as context:
            zexbuild.translate_zmac("\ttstr\t1,2,3")
        self.assertEqual(context.exception.code, "ZEX_TSTR")

    def test_tmsg_length(self):
        with self.assertRaises(zexbuild.BuildError) as context:
            zexbuild.translate_zmac("\ttmsg\t'" + "x" * 31 + "'")
        self.assertEqual(context.exception.code, "ZEX_TMSG")

    def test_unsupported_conditional(self):
        with self.assertRaises(zexbuild.BuildError) as context:
            zexbuild.translate_zmac("\tif\t1\n\tnop\n\tendif")
        self.assertEqual(context.exception.code, "ZEX_DIALECT")

    def test_unterminated_block(self):
        with self.assertRaises(zexbuild.BuildError) as context:
            zexbuild.translate_zmac("  if\t0\n\tnop")
        self.assertEqual(context.exception.code, "ZEX_DIALECT")


class PatchTest(unittest.TestCase):
    SOURCE = "a\nb\nc\nd"

    def test_apply(self):
        patch = "--- x\n+++ x\n@@ -2,2 +2,3 @@\n b\n+b2\n c\n"
        self.assertEqual(zexbuild.apply_unified_patch(self.SOURCE, patch, "t"), "a\nb\nb2\nc\nd")

    def test_context_mismatch(self):
        patch = "@@ -2,1 +2,1 @@\n-z\n+y\n"
        with self.assertRaises(zexbuild.BuildError) as context:
            zexbuild.apply_unified_patch(self.SOURCE, patch, "t")
        self.assertEqual(context.exception.code, "PATCH_CONTEXT")

    def test_count_mismatch(self):
        patch = "@@ -2,2 +2,2 @@\n b\n"
        with self.assertRaises(zexbuild.BuildError) as context:
            zexbuild.apply_unified_patch(self.SOURCE, patch, "t")
        self.assertEqual(context.exception.code, "PATCH_FORMAT")


@unittest.skipUnless(find_assembler(), "z80asm 1.8 is not available")
class AssemblerTest(unittest.TestCase):
    def test_stock_rebuild_is_byte_identical(self):
        assembler = find_assembler()
        for name in ("zexdoc", "zexall"):
            with self.subTest(name=name):
                binary = zexbuild.build_stock(name, assembler)
                self.assertEqual(zexbuild.sha256_bytes(binary), zexbuild.STOCK_SHA256[name])

    def test_program_set(self):
        programs = zexbuild.build_programs(find_assembler())
        self.assertEqual(
            sorted(programs),
            sorted(["ZEXDOC.COM", "ZEXALL.COM", "ZEXDOCF.COM", "ZEXALLF.COM",
                    "ZEX13S.COM", "FLAGPRB.COM", "DAADUMP.COM",
                    "ZEXIY.COM", "ZEXUND.COM", "ZEXED.COM", "EDPRB.COM",
                    "CBPRB.COM", "EDPRB2.COM", "EDPRB2S.COM", "INPRB.COM",
                    "EDPRB3.COM", "EDPRB4.COM", "ALTPRB.COM"]),
        )
        for name, data in programs.items():
            with self.subTest(name=name):
                self.assertLess(len(data), 0xC000)

    def test_fileout_keeps_test_state(self):
        assembler = find_assembler()
        for name in ("zexdoc", "zexall"):
            with self.subTest(name=name):
                stock = zexbuild.build_stock(name, assembler)
                binary = zexbuild.build_fileout(name, assembler)
                self.assertNotEqual(binary, stock)
                self.assertEqual(binary[:0x13], stock[:0x13])
                self.assertIn(f"{name.upper()}TXT".encode(), binary.replace(b" ", b""))


if __name__ == "__main__":
    unittest.main()

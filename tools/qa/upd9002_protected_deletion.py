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
"""Verify the exact G49-approved M50 protected-mode deletions."""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import io
import pathlib
import re
import sys
from collections import Counter
from typing import Dict, Iterable, List, Mapping, Sequence, Set, Tuple


sys.dont_write_bytecode = True


class DeletionError(RuntimeError):
    """A fail-closed M50 deletion verification error."""


APPROVED_GROUP_COUNTS = {
    "M50-PM-ARPL": 4,
    "M50-PM-MOV-SEG-EA": 4,
    "M50-PM-CTS-SYSTEM": 44,
}

M49_INVENTORY = "tools/qa/golden/upd9002_286_reachability_m49.csv"
M49_INVENTORY_SHA256 = (
    "f3843cd57b57af8f5baa4a180a7a30c88d628d0b12865d6a4a451a306794c15b"
)
M50_MANIFEST = "tools/qa/golden/upd9002_286_deletion_manifest_m50.csv"
M50_PROVENANCE = "tools/qa/golden/upd9002_dispatch_provenance_m50.csv"

IMMUTABLE_FILES = {
    "cpu/upd9002/upd9002_state.c":
        "72212d8a3b7bed6fcaf4a6670904187cdf268d5846195423ece5fdf3c05b318b",
    "cpu/upd9002/upd9002_state.h":
        "07d23bc255b0f931d8576b44c333d6046126c6e4173e2ea31f042cf1de491e92",
    "cpu/upd9002/upd9002_ea.c":
        "64fd640d307540d85b7d1fd6932daf49e83d99f5dfe81efe5f2035fb23f36172",
    "cpu/upd9002/upd9002_ops.mcr":
        "dbfcc5b3ce7d3f0b4df493cd494b7fe297aa932e231904ddeb4b59411cd73183",
    "tests/upd9002/state_fixtures_m42.txt":
        "c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb",
    "tools/qa/golden/upd9002_final_dispatch_graph_m48.csv":
        "fe9df28ad3d51cc55235afc3979ada890e86158b294762c15fa33c20d8a800a6",
    "tools/qa/golden/upd9002_dispatch_provenance_m48.csv":
        "128698af06c4e4e98183e4ec0151b7025f427c4f812f95d9012f41417461027a",
    "tools/qa/golden/upd9002_support_map_m48.csv":
        "21dd037c3eb11e1674805ad456ef03663f17804affbd7382c8db77291ab25279",
    "tools/qa/golden/upd9002_rep0f_transition_manifest_m48.json":
        "4f3fefe8cbfb20a03364a80a0b917e475d3d545cab8eda6bee8a22c66e2147ee",
    "tools/qa/golden/upd9002_dispatch_provenance_m50.csv":
        "30246dbd2bbf95b406a6bb05a182d16ea04f56dba48dca73842f5d06b74aae2c",
}

M51_CANONICAL_REPLACEMENTS = {
    "cpu/upd9002/cpucore.h": (
        (b"upd9002_core_initialize", b"i286c_initialize", 2),
        (b"upd9002_core_deinitialize", b"i286c_deinitialize", 2),
        (b"upd9002_core_reset", b"i286c_reset", 2),
        (b"upd9002_core_shut", b"i286c_shut", 2),
        (b"upd9002_core_set_ext_size", b"i286c_setextsize", 2),
        (b"upd9002_core_set_emm", b"i286c_setemm", 2),
        (b"upd9002_core_interrupt", b"i286c_interrupt", 2),
        (b"upd9002_core_step", b"v30c_step", 1),
    ),
    "cpu/upd9002/upd9002_core.c": (
        (b'#include\t"upd9002_dispatch.h"', b'#include\t"v30patch.h"', 1),
        (b"upd9002_core_initialize", b"i286c_initialize", 1),
        (b"upd9002_core_deinitialize", b"i286c_deinitialize", 1),
        (b"upd9002_core_reset", b"i286c_reset", 1),
        (b"upd9002_core_shut", b"i286c_shut", 1),
        (b"upd9002_core_set_ext_size", b"i286c_setextsize", 1),
        (b"upd9002_core_set_emm", b"i286c_setemm", 1),
        (b"upd9002_core_interrupt", b"i286c_interrupt", 1),
        (b"upd9002_dispatch_initialize", b"v30cinit", 1),
    ),
    "cpu/upd9002/upd9002_dispatch.c": (
        (b'#include\t"upd9002_dispatch.h"', b'#include\t"v30patch.h"', 1),
        (b"upd9002_dispatch_initialize", b"v30cinit", 1),
        (b"upd9002_core_step", b"v30c_step", 1),
    ),
    "tests/upd9002/rep0f_diagnostic_stop.c": (
        (b'#include "upd9002_dispatch.h"', b'#include "v30patch.h"', 1),
        (b"upd9002_core_reset", b"i286c_reset", 1),
        (b"upd9002_core_step", b"v30c_step", 3),
        (b"upd9002_core_initialize", b"i286c_initialize", 1),
        (b"upd9002_core_deinitialize", b"i286c_deinitialize", 2),
    ),
}

M60A_CANONICAL_REPLACEMENTS = {
    "cpu/upd9002/upd9002_core.c": (
        (
            b"static UINT16 upd9002_materialize_interrupt_saved_flags(void) {\n"
            b"\n"
            b"\treturn (UINT16)((UPD9002_FLAG & (UINT16)~O_FLAG) |\n"
            b"\t\t\t\t\t\t(UPD9002_OV ? O_FLAG : 0));\n"
            b"}\n\n",
            b"",
            1,
        ),
        (
            b"REGPUSH0(upd9002_materialize_interrupt_saved_flags())",
            b"REGPUSH0(REAL_FLAGREG)",
            1,
        ),
    ),
    "cpu/upd9002/upd9002_dispatch.c": (
        (
            b"#define V30_DMAP()\t\tdmap_i286()",
            b"#define REAL_V30FLAG\t(UINT16)((I286_FLAG & 0x7ff) + \\\n"
            b"\t\t\t\t\t\t\t\t\t\t\t(I286_OV?O_FLAG:0) + 0xf000)\n"
            b"#define V30_DMAP()\t\tdmap_i286()",
            1,
        ),
        (
            b"static UINT16 v30_materialize_pushf_image(void) {\n"
            b"\n"
            b"\treturn (UINT16)((I286_FLAG & (UINT16)~O_FLAG) |\n"
            b"\t\t\t\t\t\t(I286_OV ? O_FLAG : 0));\n"
            b"}\n\n",
            b"",
            1,
        ),
        (
            b"REGPUSH(v30_materialize_pushf_image(), 3)",
            b"REGPUSH(REAL_V30FLAG, 3)",
            1,
        ),
        (
            b"\tUINT\tflag;\n\n"
            b"\tI286_WORKCLOCK(5);\n"
            b"\tREGPOP0(flag)\n"
            b"\tflag = (flag & 0x0ed5) | 0xf002;\n"
            b"\tI286_OV = flag & O_FLAG;\n"
            b"\tI286_FLAG = flag & (UINT16)~O_FLAG;\n"
            b"\tI286_TRAP = ((flag & 0x300) == 0x300);",
            b"\tI286_WORKCLOCK(5);\n"
            b"\tREGPOP0(I286_FLAG)\n"
            b"\tI286_FLAG |= 0xf000;\n"
            b"\tI286_OV = I286_FLAG & O_FLAG;\n"
            b"\tI286_FLAG &= (0xfff ^ O_FLAG);\n"
            b"\tI286_TRAP = ((I286_FLAG & 0x300) == 0x300);",
            1,
        ),
    ),
}

M60E_CANONICAL_REPLACEMENTS = {
    "cpu/upd9002/upd9002_dispatch.c": (
        (
            b"\tflag = (flag & 0x0fd7) | 0xf002;",
            b"\tflag = (flag & 0x0fff) | 0xf002;",
            1,
        ),
    ),
}

DELETED_IDENTIFIERS = (
    "_arpl", "i286c_cts", "cts0_table", "cts1_table",
    "_sldt", "_str", "_lldt", "_ltr", "_verr", "_verw", "_sgdt",
    "_sidt", "_lgdt", "_lidt", "_smsw", "_lmsw", "_loadall286",
    "I286_0F", "I286OP_0F", "I286_IDTR", "I286_LDTR", "I286_TR",
    "I286_TRC",
)

PLACEHOLDERS = (
    ("v30op", "upd9002op", 0x0F, "_reserved", "v30_ope0x0f"),
    ("v30op_repe", "upd9002op_repe", 0x0F, "_reserved",
     "v30_repe_0f_diagnostic_stop"),
    ("v30op_repne", "upd9002op_repne", 0x0F, "_reserved",
     "v30_repne_0f_diagnostic_stop"),
    ("v30op", "upd9002op", 0x63, "_reserved", "v30_reserved"),
    ("v30op_repe", "upd9002op_repe", 0x63, "_reserved", "v30_reserved"),
    ("v30op_repne", "upd9002op_repne", 0x63, "_reserved", "v30_reserved"),
    ("v30op", "upd9002op", 0x8E, "_reserved", "v30mov_seg_ea"),
    ("v30op_repe", "upd9002op_repe", 0x8E, "_reserved",
     "v30mov_seg_ea"),
    ("v30op_repne", "upd9002op_repne", 0x8E, "_reserved",
     "v30mov_seg_ea"),
)

M62_GRAPH_REMOVED = {
    ("v30op", "0x27", "handler", "_daa"),
    ("v30op", "0x2f", "handler", "_das"),
    ("v30op", "0x37", "handler", "_aaa"),
    ("v30op", "0x3f", "handler", "_aas"),
    ("v30ope0x0f_table", "0x28", "handler", "v30_reserved_0x0f"),
}

M62_GRAPH_ADDED = {
    ("v30op", "0x27", "handler", "v30_daa"),
    ("v30op", "0x2f", "handler", "v30_das"),
    ("v30op", "0x37", "handler", "v30_aaa"),
    ("v30op", "0x3f", "handler", "v30_aas"),
    ("v30ope0x0f_table", "0x28", "handler", "v30_rol4_ea8"),
}

M62_SUPPORT_REMOVED = {
    ("v30op", "0x27", "-", "_daa", "implemented", "final-root-target"),
    ("v30op", "0x2f", "-", "_das", "implemented", "final-root-target"),
    ("v30op", "0x37", "-", "_aaa", "implemented", "final-root-target"),
    ("v30op", "0x3f", "-", "_aas", "implemented", "final-root-target"),
    (
        "v30op_0f",
        "0x0f",
        "0x28",
        "v30_reserved_0x0f",
        "known_target_gap",
        "second-byte-resolved",
    ),
}

M62_SUPPORT_ADDED = {
    ("v30op", "0x27", "-", "v30_daa", "implemented", "final-root-target"),
    ("v30op", "0x2f", "-", "v30_das", "implemented", "final-root-target"),
    ("v30op", "0x37", "-", "v30_aaa", "implemented", "final-root-target"),
    ("v30op", "0x3f", "-", "v30_aas", "implemented", "final-root-target"),
    (
        "v30op_0f",
        "0x0f",
        "0x28",
        "v30_rol4_ea8",
        "implemented",
        "second-byte-resolved",
    ),
}

M64_GRAPH_REMOVED = M62_GRAPH_REMOVED | {
    ("v30ope0x0f_table", opcode, "handler", "v30_reserved_0x0f")
    for opcode in ("0x13", "0x15", "0x16", "0x17", "0x1e", "0x1f", "0x26")
}

M64_GRAPH_ADDED = M62_GRAPH_ADDED | {
    ("v30ope0x0f_table", "0x13", "handler", "v30_clr1_ea16_cl"),
    ("v30ope0x0f_table", "0x15", "handler", "v30_set1_ea16_cl"),
    ("v30ope0x0f_table", "0x16", "handler", "v30_not1_ea8_cl"),
    ("v30ope0x0f_table", "0x17", "handler", "v30_not1_ea16_cl"),
    ("v30ope0x0f_table", "0x1e", "handler", "v30_not1_ea8_i3"),
    ("v30ope0x0f_table", "0x1f", "handler", "v30_not1_ea16_i4"),
    ("v30ope0x0f_table", "0x26", "handler", "v30_cmp4s"),
}

M64_SUPPORT_REMOVED = M62_SUPPORT_REMOVED | {
    (
        "v30op_0f",
        "0x0f",
        opcode,
        "v30_reserved_0x0f",
        "known_target_gap",
        "second-byte-resolved",
    )
    for opcode in ("0x13", "0x15", "0x16", "0x17", "0x1e", "0x1f", "0x26")
}

M64_SUPPORT_ADDED = M62_SUPPORT_ADDED | {
    ("v30op_0f", "0x0f", "0x13", "v30_clr1_ea16_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x15", "v30_set1_ea16_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x16", "v30_not1_ea8_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x17", "v30_not1_ea16_cl", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x1e", "v30_not1_ea8_i3", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x1f", "v30_not1_ea16_i4", "implemented",
     "second-byte-resolved"),
    ("v30op_0f", "0x0f", "0x26", "v30_cmp4s", "implemented",
     "second-byte-resolved"),
}
G70_GRAPH_PATH = pathlib.Path("tests/ssts/campaigns/g70/dispatch_graph.csv")
G70_SUPPORT_PATH = pathlib.Path("tests/ssts/campaigns/g70/dispatch_support_map.csv")

M65A_GRAPH_REMOVED = M64_GRAPH_REMOVED | {
    ("c_ope0xff_table", "0x07", "handler", "_pop_ea16"),
}

M65A_GRAPH_ADDED = M64_GRAPH_ADDED | {
    ("c_ope0xff_table", "0x07", "handler", "_push_ff7_ea16"),
}

M66B_REP_ROWS = (
    ("v30op_repe", "0x6c", "i286c_rep_insb", "upd9002_rep_insb"),
    ("v30op_repe", "0x6d", "i286c_rep_insw", "upd9002_rep_insw"),
    ("v30op_repe", "0x6e", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repe", "0x6f", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repe", "0xa4", "i286c_rep_movsb", "upd9002_rep_movsb"),
    ("v30op_repe", "0xa5", "i286c_rep_movsw", "upd9002_rep_movsw"),
    ("v30op_repe", "0xa6", "i286c_repe_cmpsb", "upd9002_repe_cmpsb"),
    ("v30op_repe", "0xa7", "i286c_repe_cmpsw", "upd9002_repe_cmpsw"),
    ("v30op_repe", "0xaa", "i286c_rep_stosb", "upd9002_rep_stosb"),
    ("v30op_repe", "0xab", "i286c_rep_stosw", "upd9002_rep_stosw"),
    ("v30op_repe", "0xac", "i286c_rep_lodsb", "upd9002_rep_lodsb"),
    ("v30op_repe", "0xad", "i286c_rep_lodsw", "upd9002_rep_lodsw"),
    ("v30op_repe", "0xae", "i286c_repe_scasb", "upd9002_repe_scasb"),
    ("v30op_repe", "0xaf", "i286c_repe_scasw", "upd9002_repe_scasw"),
    ("v30op_repne", "0x6c", "i286c_rep_insb", "upd9002_rep_insb"),
    ("v30op_repne", "0x6d", "i286c_rep_insw", "upd9002_rep_insw"),
    ("v30op_repne", "0x6e", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repne", "0x6f", "i286c_rep_outsb", "upd9002_rep_outsb"),
    ("v30op_repne", "0xa4", "i286c_rep_movsb", "upd9002_rep_movsb"),
    ("v30op_repne", "0xa5", "i286c_rep_movsw", "upd9002_rep_movsw"),
    ("v30op_repne", "0xa6", "i286c_repne_cmpsb", "upd9002_repne_cmpsb"),
    ("v30op_repne", "0xa7", "i286c_repne_cmpsw", "upd9002_repne_cmpsw"),
    ("v30op_repne", "0xaa", "i286c_rep_stosb", "upd9002_rep_stosb"),
    ("v30op_repne", "0xab", "i286c_rep_stosw", "upd9002_rep_stosw"),
    ("v30op_repne", "0xac", "i286c_rep_lodsb", "upd9002_rep_lodsb"),
    ("v30op_repne", "0xad", "i286c_rep_lodsw", "upd9002_rep_lodsw"),
    ("v30op_repne", "0xae", "i286c_repne_scasb", "upd9002_repne_scasb"),
    ("v30op_repne", "0xaf", "i286c_repne_scasw", "upd9002_repne_scasw"),
)

M66B_GRAPH_REMOVED = M65A_GRAPH_REMOVED | {
    (table, opcode, "handler", old)
    for table, opcode, old, _new in M66B_REP_ROWS
}

M66B_GRAPH_ADDED = M65A_GRAPH_ADDED | {
    (table, opcode, "handler", new)
    for table, opcode, _old, new in M66B_REP_ROWS
}

M66B_SUPPORT_REMOVED = M64_SUPPORT_REMOVED | {
    (table, opcode, "-", old, "implemented", "final-root-target")
    for table, opcode, old, _new in M66B_REP_ROWS
}

M66B_SUPPORT_ADDED = M64_SUPPORT_ADDED | {
    (table, opcode, "-", new, "implemented", "final-root-target")
    for table, opcode, _old, new in M66B_REP_ROWS
}

M62_PROVENANCE_REMOVED = {
    ("v30op", "0x27", "i286op", "_daa", "base", "_daa"),
    ("v30op", "0x2f", "i286op", "_das", "base", "_das"),
    ("v30op", "0x37", "i286op", "_aaa", "base", "_aaa"),
    ("v30op", "0x3f", "i286op", "_aas", "base", "_aas"),
}

M62_PROVENANCE_ADDED = {
    ("v30op", "0x27", "i286op", "_daa", "patch", "v30_daa"),
    ("v30op", "0x2f", "i286op", "_das", "patch", "v30_das"),
    ("v30op", "0x37", "i286op", "_aaa", "patch", "v30_aaa"),
    ("v30op", "0x3f", "i286op", "_aas", "patch", "v30_aas"),
}

M48_HARNESS_ADDED = {
    (
        "patch-v30op_repe-0f",
        "v30op_repe",
        "0x0f",
        "v30_repe_0f_diagnostic_stop",
        "f30fc0000000000000",
        "1",
        "patched-root",
    ),
    (
        "patch-v30op_repne-0f",
        "v30op_repne",
        "0x0f",
        "v30_repne_0f_diagnostic_stop",
        "f20fc0000000000000",
        "1",
        "patched-root",
    ),
}

M62_HARNESS_REMOVED = {
    (
        "native-0f-28",
        "v30ope0x0f_table",
        "0x28",
        "v30_reserved_0x0f",
        "0f28c0000000000000",
        "1",
        "native-secondary",
    ),
}

M62_HARNESS_ADDED = {
    (
        "native-0f-28",
        "v30ope0x0f_table",
        "0x28",
        "v30_rol4_ea8",
        "0f28c0000000000000",
        "1",
        "native-secondary",
    ),
    (
        "patch-v30op-27",
        "v30op",
        "0x27",
        "v30_daa",
        "27c0000000000000",
        "1",
        "patched-root",
    ),
    (
        "patch-v30op-2f",
        "v30op",
        "0x2f",
        "v30_das",
        "2fc0000000000000",
        "1",
        "patched-root",
    ),
    (
        "patch-v30op-37",
        "v30op",
        "0x37",
        "v30_aaa",
        "37c0000000000000",
        "1",
        "patched-root",
    ),
    (
        "patch-v30op-3f",
        "v30op",
        "0x3f",
        "v30_aas",
        "3fc0000000000000",
        "1",
        "patched-root",
    ),
}

M64_NATIVE_HANDLERS = {
    "13": "v30_clr1_ea16_cl",
    "15": "v30_set1_ea16_cl",
    "16": "v30_not1_ea8_cl",
    "17": "v30_not1_ea16_cl",
    "1e": "v30_not1_ea8_i3",
    "1f": "v30_not1_ea16_i4",
    "26": "v30_cmp4s",
}

M64_HARNESS_REMOVED = M62_HARNESS_REMOVED | {
    (
        "native-0f-{}".format(opcode),
        "v30ope0x0f_table",
        "0x{}".format(opcode),
        "v30_reserved_0x0f",
        "0f{}c0000000000000".format(opcode),
        "1",
        "native-secondary",
    )
    for opcode in M64_NATIVE_HANDLERS
}

M64_HARNESS_ADDED = M62_HARNESS_ADDED | {
    (
        "native-0f-{}".format(opcode),
        "v30ope0x0f_table",
        "0x{}".format(opcode),
        handler,
        "0f{}c0000000000000".format(opcode),
        "1",
        "native-secondary",
    )
    for opcode, handler in M64_NATIVE_HANDLERS.items()
}

MANIFEST_COLUMNS = (
    "candidate_id", "symbol_or_field", "kind", "defining_file",
    "approved_group", "m50_action", "retained_replacement", "evidence",
)

Row = Tuple[str, ...]


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_bytes(root: pathlib.Path, relative: str) -> bytes:
    try:
        return (root / relative).read_bytes()
    except OSError as error:
        raise DeletionError("cannot read {}: {}".format(relative, error)) from error


def verify_immutable_files(root: pathlib.Path) -> None:
    for relative, expected in IMMUTABLE_FILES.items():
        data = read_bytes(root, relative)
        for current, accepted, expected_count in (
                M60E_CANONICAL_REPLACEMENTS.get(relative, ())):
            actual_count = data.count(current)
            if actual_count != expected_count:
                raise DeletionError(
                    "M60e semantic transition count changed: {} token={!r} "
                    "expected={} actual={}".format(
                        relative, current, expected_count, actual_count))
            data = data.replace(current, accepted)
        for current, accepted, expected_count in (
                M60A_CANONICAL_REPLACEMENTS.get(relative, ())):
            actual_count = data.count(current)
            if actual_count != expected_count:
                raise DeletionError(
                    "M60a semantic transition count changed: {} token={!r} "
                    "expected={} actual={}".format(
                        relative, current, expected_count, actual_count))
            data = data.replace(current, accepted)
        for current, accepted, expected_count in M51_CANONICAL_REPLACEMENTS.get(
                relative, ()):
            actual_count = data.count(current)
            if actual_count != expected_count:
                raise DeletionError(
                    "M51 rename count changed: {} token={!r} expected={} "
                    "actual={}".format(relative, current, expected_count,
                                       actual_count))
            data = data.replace(current, accepted)
        actual = sha256(data)
        if actual != expected:
            raise DeletionError(
                "retained artifact changed: {} expected={} actual={}".format(
                    relative, expected, actual))


def load_dispatch_module(root: pathlib.Path):
    path = root / "tools/qa/upd9002_dispatch.py"
    spec = importlib.util.spec_from_file_location("upd9002_dispatch_m50", path)
    if spec is None or spec.loader is None:
        raise DeletionError("cannot load upd9002_dispatch.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def csv_rows(text: str) -> Set[Row]:
    return {tuple(row) for row in csv.reader(io.StringIO(text))}


M66B_PROVENANCE_TABLE_CANONICAL = {
    "upd9002op": "i286op",
    "upd9002op_repe": "i286op_repe",
    "upd9002op_repne": "i286op_repne",
}

M66B_PROVENANCE_HANDLER_CANONICAL = {
    new: old for _table, _opcode, old, new in M66B_REP_ROWS
}


def canonicalize_m66b_provenance(rows: Set[Row]) -> Set[Row]:
    canonical: Set[Row] = set()
    for row in rows:
        fields = list(row)
        if len(fields) >= 3:
            fields[2] = M66B_PROVENANCE_TABLE_CANONICAL.get(
                fields[2], fields[2])
        if len(fields) >= 4:
            fields[3] = M66B_PROVENANCE_HANDLER_CANONICAL.get(
                fields[3], fields[3])
        if len(fields) >= 6:
            fields[5] = M66B_PROVENANCE_HANDLER_CANONICAL.get(
                fields[5], fields[5])
        canonical.add(tuple(fields))
    return canonical


def require_exact_difference(
        name: str, old: Set[Row], new: Set[Row],
        expected_removed: Set[Row], expected_added: Set[Row]) -> None:
    removed = old - new
    added = new - old
    if removed != expected_removed or added != expected_added:
        raise DeletionError(
            "{} transition drifted: removed={} added={}".format(
                name, sorted(removed), sorted(added)))


def load_approved_rows(root: pathlib.Path) -> List[Dict[str, str]]:
    data = read_bytes(root, M49_INVENTORY)
    if sha256(data) != M49_INVENTORY_SHA256:
        raise DeletionError("accepted M49 inventory identity changed")
    rows = list(csv.DictReader(io.StringIO(data.decode("utf-8"))))
    approved = [row for row in rows
                if row["proposed_deletion_group"] in APPROVED_GROUP_COUNTS]
    counts = Counter(row["proposed_deletion_group"] for row in approved)
    if dict(counts) != APPROVED_GROUP_COUNTS:
        raise DeletionError("approved M49 group closure changed: {}".format(counts))
    unexpected = sorted({row["proposed_deletion_group"] for row in rows
                         if row["proposed_deletion_group"] != "-"} -
                        set(APPROVED_GROUP_COUNTS))
    if unexpected:
        raise DeletionError("unapproved M49 groups appeared: {}".format(unexpected))
    return approved


def build_manifest(rows: Iterable[Mapping[str, str]]) -> List[Dict[str, str]]:
    output = []
    for row in rows:
        placeholder = row["kind"] == "constructor_base_entry"
        output.append({
            "candidate_id": row["candidate_id"],
            "symbol_or_field": row["symbol_or_field"],
            "kind": row["kind"],
            "defining_file": row["defining_file"],
            "approved_group": row["proposed_deletion_group"],
            "m50_action": ("replaced_with_reserved_placeholder"
                           if placeholder else "deleted"),
            "retained_replacement": "_reserved" if placeholder else "-",
            "evidence": ("final native patch retained; base address removed"
                         if placeholder else
                         "approved dependency-closed member absent from active source"),
        })
    output.sort(key=lambda row: row["candidate_id"])
    return output


def validate_manifest(rows: Sequence[Mapping[str, str]]) -> None:
    identifiers = [row["candidate_id"] for row in rows]
    duplicates = sorted(name for name, count in Counter(identifiers).items()
                        if count != 1)
    if duplicates:
        raise DeletionError("duplicate manifest candidates: {}".format(duplicates))
    counts = Counter(row["approved_group"] for row in rows)
    if dict(counts) != APPROVED_GROUP_COUNTS:
        raise DeletionError("manifest group closure changed: {}".format(counts))
    for row in rows:
        if set(row) != set(MANIFEST_COLUMNS):
            raise DeletionError("manifest columns changed: {}".format(
                row.get("candidate_id")))
        placeholder = row["kind"] == "constructor_base_entry"
        expected_action = ("replaced_with_reserved_placeholder"
                           if placeholder else "deleted")
        expected_replacement = "_reserved" if placeholder else "-"
        if (row["m50_action"] != expected_action or
                row["retained_replacement"] != expected_replacement):
            raise DeletionError("invalid deletion action: {}".format(
                row["candidate_id"]))
        for value in row.values():
            if re.search(r"(?:^|[ ;])/(?:tmp|home|mnt)/|[A-Za-z]:\\", value):
                raise DeletionError("host-dependent manifest value: {}".format(
                    row["candidate_id"]))


def manifest_bytes(rows: Sequence[Mapping[str, str]]) -> bytes:
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=MANIFEST_COLUMNS,
                            lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue().encode("utf-8")


def active_source_paths(root: pathlib.Path) -> List[pathlib.Path]:
    paths = [root / "CMakeLists.txt"]
    for base in (root / "cpu/upd9002", root / "tests/upd9002"):
        for path in sorted(base.iterdir()):
            if path.is_file() and path.suffix in {".c", ".h", ".mcr"}:
                paths.append(path)
    return paths


def verify_source_absence(root: pathlib.Path) -> None:
    if (root / "cpu/upd9002/i286c_0f.c").exists():
        raise DeletionError("approved CTS translation unit still exists")
    findings = []
    for path in active_source_paths(root):
        text = path.read_text(encoding="utf-8")
        relative = path.relative_to(root).as_posix()
        for symbol in DELETED_IDENTIFIERS:
            pattern = r"(?<![A-Za-z0-9_]){}(?![A-Za-z0-9_])".format(
                re.escape(symbol))
            if re.search(pattern, text):
                findings.append("{}:{}".format(relative, symbol))
    if findings:
        raise DeletionError("deleted active identifiers remain: {}".format(
            ", ".join(findings)))
    mn = (root / "cpu/upd9002/upd9002_mn.c").read_text(encoding="utf-8")
    match = re.search(r"\bUPD9002FN\s+_mov_seg_ea\s*\([^)]*\)\s*\{(.*?)\n\}",
                      mn, flags=re.S)
    if not match:
        raise DeletionError("canonical MOV segment handler is missing")
    body = match.group(1)
    forbidden = ("MSW_PE", "I286_PROTECT", "I286_TR", "I286_LDTR", "I286_GDTR")
    present = [token for token in forbidden if token in body]
    if present:
        raise DeletionError(
            "protected-mode MOV segment body reintroduced: {}".format(present))


def verify_dispatch(root: pathlib.Path, module, write: bool) -> Tuple[str, str]:
    sources = module.load_sources(root)
    arrays = module.parse_arrays(sources)
    roots, _provenance_rows = module.construct_roots(arrays)

    graph, provenance, harness, support = module.generate(root)
    if (root / G70_GRAPH_PATH).exists() and (root / G70_SUPPORT_PATH).exists():
        current_graph = read_bytes(
            root, "tools/qa/golden/upd9002_final_dispatch_graph.csv"
        ).decode("utf-8")
        current_support = read_bytes(
            root, "tools/qa/golden/upd9002_support_map_m42.csv"
        ).decode("utf-8")
        current_harness = read_bytes(
            root, "tests/upd9002/harness_manifest.csv"
        ).decode("utf-8")
        current_provenance = read_bytes(
            root, "tools/qa/golden/upd9002_dispatch_provenance_m42.csv"
        ).decode("utf-8")
        if current_graph != graph:
            raise DeletionError("current dispatch graph differs from regeneration")
        if current_support != support:
            raise DeletionError("current dispatch support map differs from regeneration")
        if current_harness != harness:
            raise DeletionError("current dispatch harness differs from regeneration")
        if current_provenance != provenance:
            raise DeletionError("current dispatch provenance differs from regeneration")
        return sha256(provenance.encode("utf-8")), sha256(graph.encode("utf-8"))
    expected_graph = read_bytes(
        root, "tools/qa/golden/upd9002_final_dispatch_graph_m48.csv")
    expected_support = read_bytes(
        root, "tools/qa/golden/upd9002_support_map_m48.csv")
    require_exact_difference(
        "post-M48 governed graph",
        csv_rows(expected_graph.decode("utf-8")),
        csv_rows(graph),
        M66B_GRAPH_REMOVED,
        M66B_GRAPH_ADDED,
    )
    require_exact_difference(
        "post-M48 governed support",
        csv_rows(expected_support.decode("utf-8")),
        csv_rows(support),
        M66B_SUPPORT_REMOVED,
        M66B_SUPPORT_ADDED,
    )

    accepted_m50_provenance = read_bytes(root, M50_PROVENANCE)
    require_exact_difference(
        "post-M50 governed provenance",
        csv_rows(accepted_m50_provenance.decode("utf-8")),
        canonicalize_m66b_provenance(csv_rows(provenance)),
        M62_PROVENANCE_REMOVED,
        M62_PROVENANCE_ADDED,
    )

    old_harness = csv_rows(read_bytes(
        root, "tests/upd9002/harness_manifest.csv").decode("utf-8"))
    live_harness = csv_rows(harness)
    accepted_m48_harness = old_harness | M48_HARNESS_ADDED
    require_exact_difference(
        "post-M48 governed harness",
        accepted_m48_harness,
        live_harness,
        M64_HARNESS_REMOVED,
        M64_HARNESS_ADDED,
    )

    return sha256(accepted_m50_provenance), sha256(graph.encode("utf-8"))


def compare_or_write(path: pathlib.Path, data: bytes, write: bool) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return
    try:
        existing = path.read_bytes()
    except OSError as error:
        raise DeletionError("cannot read generated artifact {}: {}".format(
            path, error)) from error
    if existing != data:
        raise DeletionError("generated artifact differs: {}".format(path))


def internal_selftest(rows: Sequence[Mapping[str, str]]) -> None:
    duplicate = [dict(row) for row in rows]
    duplicate.append(dict(rows[0]))
    try:
        validate_manifest(duplicate)
    except DeletionError:
        pass
    else:
        raise DeletionError("duplicate manifest candidate was accepted")

    invalid = [dict(row) for row in rows]
    invalid[0]["retained_replacement"] = "-"
    try:
        validate_manifest(invalid)
    except DeletionError:
        pass
    else:
        raise DeletionError("invalid replacement action was accepted")

    host_dependent = [dict(row) for row in rows]
    host_dependent[0]["evidence"] = "/tmp/address-dependent"
    try:
        validate_manifest(host_dependent)
    except DeletionError:
        pass
    else:
        raise DeletionError("host-dependent manifest value was accepted")


def verify(root: pathlib.Path, write: bool, selftest: bool) -> Tuple[int, str, str]:
    verify_immutable_files(root)
    approved = load_approved_rows(root)
    rows = build_manifest(approved)
    validate_manifest(rows)
    verify_source_absence(root)
    module = load_dispatch_module(root)
    provenance_digest, graph_digest = verify_dispatch(root, module, write)
    data = manifest_bytes(rows)
    compare_or_write(root / M50_MANIFEST, data, write)
    if selftest:
        internal_selftest(rows)
        module.internal_selftest()
    return len(rows), sha256(data), provenance_digest + ":" + graph_digest


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    return parser.parse_args(argv)


def main(argv: Sequence[str] = None) -> int:
    arguments = parse_args(sys.argv[1:] if argv is None else argv)
    try:
        count, manifest_digest, dispatch_digests = verify(
            arguments.root.resolve(), arguments.write, arguments.selftest)
    except (DeletionError, OSError, UnicodeError, ValueError, KeyError,
            TypeError) as error:
        print("upd9002-protected-deletion: FAIL: {}".format(error),
              file=sys.stderr)
        return 1
    print("upd9002-protected-deletion: PASS candidates={} manifest_sha256={}".format(
        count, manifest_digest))
    print("upd9002-protected-deletion: provenance:graph_sha256={}".format(
        dispatch_digests))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

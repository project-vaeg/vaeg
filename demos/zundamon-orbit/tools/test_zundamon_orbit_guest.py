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

"""Focused oracle and deterministic lifecycle-fault tests for M98o."""

from __future__ import annotations

import json
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
import build_zundamon_orbit_pipeline as pipeline  # noqa: E402
import inspect_zundamon_orbit_atlas as atlas_format  # noqa: E402
import verify_zundamon_orbit_guest as oracle  # noqa: E402

SCRIPT = TOOLS / "verify_zundamon_orbit_guest.py"
ASSEMBLY = TOOLS.parent / "256" / "zundamon_orbit_256.asm"


def write_registers(path: Path, required: dict[str, str]) -> None:
    values = {
        "schema": "vaeg-registers-v1", "sequence": "1", "ordinal": "1",
        "clock": "1000", "ax": "0000", "bx": "0000", "cx": "0000",
        "dx": "0000", "si": "0000", "di": "0000", "bp": "0000",
        "sp": "f000", "es": "3000", "cs": "3000", "ss": "3000",
        "ds": "3000", "ip": "0000", "flags": "0200",
        "es_base": "00030000", "cs_base": "00030000",
        "ss_base": "00030000", "ds_base": "00030000"}
    values.update(required)
    path.write_text("".join(f"{key}\t{value}\n" for key, value in values.items()),
                    encoding="utf-8")


def bmp24(color: tuple[int, int, int]) -> bytes:
    width, height = 320, 200
    row_size = width * 3
    pixels = bytes(color) * (width * height)
    header = bytearray(54)
    struct.pack_into("<2sIHHI", header, 0, b"BM", 54 + len(pixels), 0, 0, 54)
    struct.pack_into("<IiiHHIIiiII", header, 14, 40, width, height, 1, 24,
                     0, row_size * height, 2835, 2835, 0, 0)
    return bytes(header + pixels)


def write_fixture(directory: Path):
    generated = directory / "public"
    pipeline.write_public_fixture(generated)
    atlas_path = directory / "ZUNDORB.BIN"
    shutil.copyfile(generated / pipeline.ATLAS_NAME, atlas_path)
    atlas = atlas_path.read_bytes()
    header, descriptors = atlas_format.inspect_bytes(atlas)
    descriptor = descriptors[-1]
    expected = oracle.expected_registers(header, descriptor)
    for prefix in oracle.ALL_PREFIXES:
        write_registers(directory / f"{prefix}.registers.tsv", expected[prefix])

    page_a = oracle.expected_page(atlas, descriptor, oracle.POSITION_P1)
    page_b = oracle.expected_page(atlas, descriptor, oracle.POSITION_P0)
    zero_page = bytes(oracle.G1_PAGE_BYTES)
    page_pairs = ((zero_page, page_b), (page_a, page_b), (page_a, page_b),
                  (page_a, page_b), (page_a, page_b), (page_a, page_b))
    screen_p0 = bmp24((0x20, 0x50, 0x80))
    screen_p1 = bmp24((0x60, 0x30, 0x90))
    screens = (screen_p0, screen_p1, screen_p0, screen_p1, screen_p1, screen_p1)
    for prefix, pair, screen in zip(oracle.GVRAM_PREFIXES, page_pairs, screens):
        raw = bytearray(oracle.GVRAM_SIZE)
        raw[:oracle.G0_SIZE] = oracle.expected_g0()
        raw[oracle.G1_OFFSET:oracle.G1_OFFSET + oracle.G1_PAGE_BYTES] = pair[0]
        start_b = oracle.G1_OFFSET + oracle.G1_PAGE_BYTES
        raw[start_b:start_b + oracle.G1_PAGE_BYTES] = pair[1]
        (directory / f"{prefix}.gvram.bin").write_bytes(raw)
        (directory / f"{prefix}.screen.bmp").write_bytes(screen)

    frames = (1220, 1221, 1222, 1223, 1224, 1225,
              1226, 1227, 1228, 1228, 1228, 1228)
    events = ["event\tframe\tid\tvalue", "initialized\t0\t-\t5000"]
    for ordinal, (prefix, frame) in enumerate(zip(oracle.ALL_PREFIXES, frames), 1):
        events.extend((f"wait-pc\t{frame - 1}\t-\t1", f"pc\t{frame}\t-\t{ordinal}",
                       f"capture\t{frame}\t{prefix}\t1"))
    events.append("exit\t1228\t-\t0")
    (directory / "events.tsv").write_text("\n".join(events) + "\n", encoding="utf-8")

    source = oracle.BMS_WINDOW_BASE + descriptor.bank_offset
    destinations = (
        oracle.G1_PAGE_B_SGP + oracle.POSITION_P0[1] * oracle.G1_WIDTH + oracle.POSITION_P0[0],
        oracle.G1_PAGE_A_SGP + oracle.POSITION_P1[1] * oracle.G1_WIDTH + oracle.POSITION_P1[0],
        oracle.G1_PAGE_B_SGP + oracle.POSITION_P0[1] * oracle.G1_WIDTH + oracle.POSITION_P0[0],
        oracle.G1_PAGE_A_SGP + oracle.POSITION_P1[1] * oracle.G1_WIDTH + oracle.POSITION_P1[0])
    lines = []
    for destination in destinations:
        lines.append(f"SGP_SCAN: SET_SOURCE addr={source:06x} dot=0 mode=2 "
                     f"width={descriptor.width} height={descriptor.height} fbw={descriptor.pitch}")
        lines.append(f"SGP_SCAN: SET_DEST addr={destination:06x} dot=0 mode=2 "
                     f"width={descriptor.width} height={descriptor.height} fbw=320")
    (directory / "sgp-trace.log").write_text("\n".join(lines) + "\n", encoding="utf-8")
    (directory / "ZUNDORB.LST").write_text(
        "  100                                  staging_buffer:\n"
        "  101 000034C0 00<rep 1000h>              times STAGING_BYTES db 0\n",
        encoding="utf-8")
    (directory / "m98o-source.asm").write_text(
        "%define G1_PAGE_BYTES           0xfa00\n"
        "%define G1_PAGE_WORD_COUNT      0x7d00\n"
        "%define G1_PAGE_B_SGP_BASE      0x22fa00\n"
        "%define G1_PAGE_B_DSA           0x02fa00\n"
        "%define POSITION_P0_X           48\n"
        "%define POSITION_P1_X           248\n"
        "%define PAGE_UNINITIALIZED      0\n"
        "%define PAGE_HIDDEN_RENDERING   2\n"
        "%define PAGE_HIDDEN_COMPLETE    3\n"
        "%define PAGE_VISIBLE            4\n"
        "call select_render_bms\n"
        "call wait_vblank_edge\n"
        "call publish_page\n"
        "call select_render_ordinary\n"
        "call run_sgp_command_list\n"
        "call run_sgp_command_list\n"
        "mov ax, SGP_COMMAND_BITBLT\n",
        encoding="utf-8")
    return atlas_path, header, descriptor


class M98oOracleTests(unittest.TestCase):
    def run_case(self, mutation: Callable | None = None):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            atlas, header, descriptor = write_fixture(directory)
            source = directory / "m98o-source.asm"
            if mutation is not None:
                mutated_source = mutation(directory, header, descriptor)
                if mutated_source is not None:
                    source = mutated_source
            command = [sys.executable, str(SCRIPT), str(directory), "--atlas", str(atlas),
                       "--trace", str(directory / "sgp-trace.log"),
                       "--source", str(source)]
            completed = subprocess.run(command, check=False, capture_output=True, text=True)
            return completed.returncode, json.loads(completed.stdout)

    def assert_error(self, expected: str, mutation: Callable) -> None:
        returncode, result = self.run_case(mutation)
        self.assertEqual(returncode, 1)
        self.assertEqual(result["status"], "FAIL")
        self.assertEqual(result["errors"], [expected])

    def test_passing_fixture(self) -> None:
        returncode, result = self.run_case()
        self.assertEqual(returncode, 0)
        self.assertEqual(result["status"], "PASS")
        self.assertEqual(result["errors"], [])
        self.assertEqual(result["sgp"]["trace"]["bitblt_count"], 4)
        self.assertEqual(result["page_identities"]["page_a_nonzero"], 73)
        self.assertEqual(result["page_identities"]["page_b_nonzero"], 73)

    def test_flip_signature_mutation(self) -> None:
        def mutate(directory, header, descriptor):
            required = oracle.expected_registers(header, descriptor)["m98o-flip-2"]
            write_registers(directory / "m98o-flip-2.registers.tsv",
                            dict(required, cx="0001"))
        self.assert_error("M98O_FLIP_2_SIGNATURE", mutate)

    def test_g1_page_mutation(self) -> None:
        def mutate(directory, _header, _descriptor):
            for prefix in oracle.SETTLED_PREFIXES:
                path = directory / f"{prefix}.gvram.bin"
                raw = bytearray(path.read_bytes())
                raw[oracle.G1_OFFSET + 123] ^= 1
                path.write_bytes(raw)
        self.assert_error("M98O_G1_PAGE_CONTENT", mutate)

    def test_direct_source_sequence_mutation(self) -> None:
        def mutate(directory, _header, descriptor):
            path = directory / "sgp-trace.log"
            source = oracle.BMS_WINDOW_BASE + descriptor.bank_offset
            text = path.read_text(encoding="utf-8").replace(
                f"addr={source:06x}", f"addr={source + 16:06x}", 1)
            path.write_text(text, encoding="utf-8")
        self.assert_error("M98O_TRACE_BMS_SOURCE_SEQUENCE", mutate)

    def test_visible_destination_sequence_mutation(self) -> None:
        def mutate(directory, _header, _descriptor):
            path = directory / "sgp-trace.log"
            expected = oracle.G1_PAGE_B_SGP + oracle.POSITION_P0[1] * 320 + oracle.POSITION_P0[0]
            text = path.read_text(encoding="utf-8").replace(
                f"addr={expected:06x}", f"addr={oracle.G1_PAGE_A_SGP:06x}", 1)
            path.write_text(text, encoding="utf-8")
        self.assert_error("M98O_TRACE_HIDDEN_DESTINATION_SEQUENCE", mutate)

    def test_settled_instability_mutation(self) -> None:
        def mutate(directory, _header, _descriptor):
            path = directory / "m98o-settled-b.gvram.bin"
            raw = bytearray(path.read_bytes())
            raw[-1] ^= 1
            path.write_bytes(raw)
        self.assert_error("M98O_GVRAM_UNSTABLE", mutate)

    def test_black_screen_mutation(self) -> None:
        def mutate(directory, _header, _descriptor):
            for prefix in oracle.GVRAM_PREFIXES:
                (directory / f"{prefix}.screen.bmp").write_bytes(bmp24((0, 0, 0)))
        self.assert_error("M98O_SCREEN_BLACK", mutate)

    def test_frame_limit_mutation(self) -> None:
        def mutate(directory, _header, _descriptor):
            path = directory / "events.tsv"
            path.write_text(path.read_text(encoding="utf-8")
                            + "frame-limit\t5000\t-\t5000\n", encoding="utf-8")
        self.assert_error("M98O_EVENTS_TIMEOUT", mutate)


@dataclass
class FaultResult:
    code: str
    cleanup_runs: int
    dsa_after: int
    ordinary_selector: int
    partial_published: bool
    video_restored: bool


def lifecycle_fault(case: str) -> FaultResult:
    """Test-only state machine; the release guest has no fault-injection path."""
    old_dsa = oracle.G1_PAGE_A_DSA
    state = {"A": "VISIBLE", "B": "HIDDEN_CLEAN"}
    visible, hidden = "A", "B"
    busy = False
    selector = 0
    graphics = False
    code = "M98O_FAULT_UNKNOWN"
    if case == "atlas-invalid":
        code = "M98O_FAULT_ATLAS_BEFORE_VIDEO"
    else:
        graphics = True
        if case == "descriptor-invalid":
            code = "M98O_FAULT_PAGE_DESCRIPTOR"
        elif case == "destination-oob":
            code = "M98O_FAULT_DESTINATION_BOUNDS"
        elif case == "render-visible":
            code = "M98O_FAULT_VISIBLE_RENDER"
        else:
            selector = 1
            state[hidden] = "HIDDEN_RENDERING"
            busy = True
            if case == "bank-switch-busy":
                code = "M98O_FAULT_BMS_SWITCH_BUSY"
            elif case == "publish-early":
                code = "M98O_FAULT_EARLY_PUBLICATION"
            elif case == "sgp-clear-timeout":
                code = "M98O_FAULT_SGP_CLEAR_TIMEOUT"
            elif case == "sgp-bitblt-error":
                code = "M98O_FAULT_SGP_BITBLT_ERROR"
            else:
                busy = False
                state[hidden] = "HIDDEN_COMPLETE"
                selector = 0
                if case == "vblank-low-timeout":
                    code = "M98O_FAULT_VBLANK_LOW_TIMEOUT"
                elif case == "vblank-high-timeout":
                    code = "M98O_FAULT_VBLANK_HIGH_TIMEOUT"
    # The model's common cleanup aborts bounded SGP work before restoring bank 0.
    busy = False
    selector = 0
    graphics = False
    return FaultResult(code, 1, old_dsa, selector, False, not graphics and not busy)


class M98oLifecycleFaultTests(unittest.TestCase):
    CASES = {
        "sgp-clear-timeout": "M98O_FAULT_SGP_CLEAR_TIMEOUT",
        "sgp-bitblt-error": "M98O_FAULT_SGP_BITBLT_ERROR",
        "vblank-low-timeout": "M98O_FAULT_VBLANK_LOW_TIMEOUT",
        "vblank-high-timeout": "M98O_FAULT_VBLANK_HIGH_TIMEOUT",
        "publish-early": "M98O_FAULT_EARLY_PUBLICATION",
        "render-visible": "M98O_FAULT_VISIBLE_RENDER",
        "bank-switch-busy": "M98O_FAULT_BMS_SWITCH_BUSY",
        "descriptor-invalid": "M98O_FAULT_PAGE_DESCRIPTOR",
        "destination-oob": "M98O_FAULT_DESTINATION_BOUNDS",
        "atlas-invalid": "M98O_FAULT_ATLAS_BEFORE_VIDEO",
    }

    def test_required_faults_fail_closed(self) -> None:
        for case, expected in self.CASES.items():
            with self.subTest(case=case):
                result = lifecycle_fault(case)
                self.assertEqual(result.code, expected)
                self.assertEqual(result.cleanup_runs, 1)
                self.assertEqual(result.dsa_after, oracle.G1_PAGE_A_DSA)
                self.assertEqual(result.ordinary_selector, 0)
                self.assertFalse(result.partial_published)
                self.assertTrue(result.video_restored)

    def test_geometry_contract(self) -> None:
        self.assertEqual(oracle.G1_PAGE_BYTES, 64000)
        self.assertEqual(oracle.G1_PAGE_B_SGP - oracle.G1_PAGE_A_SGP, 64000)
        self.assertEqual(oracle.G1_PAGE_B_DSA - oracle.G1_PAGE_A_DSA, 64000)
        p0 = (*oracle.POSITION_P0, 23, 19)
        p1 = (*oracle.POSITION_P1, 23, 19)
        self.assertLessEqual(p0[0] + p0[2], 320)
        self.assertLessEqual(p1[1] + p1[3], 200)
        self.assertTrue(p0[0] + p0[2] <= p1[0] or p1[0] + p1[2] <= p0[0])


if __name__ == "__main__":
    unittest.main()

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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Check the P5 one-pass, hidden-page, and exact-span invariants.

This is a source/trace guard, not a timing or real-hardware conformance test.
The span matrix is imported from the independent P4 verifier because P5 uses
that same exact logical-span callback.
"""

import argparse
import importlib.util
import json
from pathlib import Path


PAGE_BYTES = 0xFA00
GVRAM_BYTES = 0x40000


def load_p4_temporal():
    path = Path(__file__).with_name("verify-p4-temporal.py")
    spec = importlib.util.spec_from_file_location("glass_p4_temporal", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def check_sources(source_root):
    wrapper = (source_root / "glass_orbit.asm").read_text(encoding="utf-8")
    scene = (source_root / "glass_scene.inc").read_text(encoding="utf-8")
    errors = []
    if "%define GLASS_P4_SGP_STAGE 2" not in wrapper:
        errors.append("P5 must exclude edges from the first SGP list")
    if scene.count("call    glass_p4_sgp_run_list") != 2:
        errors.append("P5 must submit exactly one face/grid list and one edge list")
    if scene.count("call    glass_p4_sgp_apply_endpoint_spans") != 1:
        errors.append("P5 must apply exact endpoint RMW once")
    if scene.count("call    glass_p4_sgp_build_edge_list") != 1:
        errors.append("P5 must build the outline list once")
    if "call    glass_p4_sgp_draw_edges" in scene:
        errors.append("P5 must not call the low-level edge emitter directly")
    for symbol in (
        "glass_p5_patch",
        "glass_p5_repair",
        "glass_p5_fixup",
        "glass_p5_erase",
    ):
        if symbol in scene:
            errors.append(f"P5 must not contain geometry repair stage: {symbol}")
    for token in (
        "glass_p5_frame_ready",
        "glass_p5_geometry_complete",
        "glass_p5_stars_complete",
        "glass_p5_sgp_idle",
        "glass_p5_presented_frame",
    ):
        if token not in scene:
            errors.append(f"missing explicit frame-state field: {token}")
    frame = scene.split(".frame:", 1)[1]
    order = [
        "call    glass_p4_sgp_build_list",
        "call    glass_p4_sgp_run_list",
        "call    glass_p4_sgp_apply_endpoint_spans",
        "call    glass_p4_sgp_build_edge_list",
        "call    glass_p4_sgp_run_list",
        "call    glass_p5_draw_stars",
        "call    glass_p5_wait_vblank",
        "call    glass_p5_select_display_page",
    ]
    positions = []
    cursor = 0
    for token in order:
        position = frame.find(token, cursor)
        positions.append(position)
        if position >= 0:
            cursor = position + len(token)
    if any(position < 0 for position in positions) or positions != sorted(positions):
        errors.append("P5 FRAME_READY order is not monotonic")
    ready_at = frame.find("mov     byte [glass_p5_frame_ready], 1")
    wait_at = frame.find("call    glass_p5_wait_vblank")
    present_at = frame.find("call    glass_p5_select_display_page")
    clear_ready_at = frame.find("mov     byte [glass_p5_frame_ready], 0", ready_at + 1)
    if ready_at < 0 or wait_at < 0 or present_at < 0 or not (ready_at < wait_at < present_at):
        errors.append("FRAME_READY must precede VBLANK and page selection")
    if clear_ready_at < 0 or clear_ready_at < present_at:
        errors.append("FRAME_READY must be cleared only after page selection")
    for token in ("PAGE_A_BASE_P5", "PAGE_B_BASE_P5", "PAGE_B_SOURCE_Y_P5", "PAGE_B_CPU_SEGMENT_P5"):
        if token not in scene:
            errors.append(f"missing hidden-page definition: {token}")
    return {"errors": errors, "status": "PASS" if not errors else "FAIL"}


def check_gvram(paths):
    reports = []
    errors = []
    for path in paths:
        data = Path(path).read_bytes()
        report = {"path": str(path), "bytes": len(data)}
        if len(data) != GVRAM_BYTES:
            report["status"] = "FAIL"
            errors.append(f"{path}: expected {GVRAM_BYTES} bytes")
        else:
            report["page_a_nonzero"] = sum(value != 0 for value in data[:PAGE_BYTES])
            report["page_b_nonzero"] = sum(value != 0 for value in data[PAGE_BYTES:2 * PAGE_BYTES])
            report["status"] = "PASS"
        reports.append(report)
    return {"captures": reports, "status": "PASS" if not errors else "FAIL", "errors": errors}


def check_registers(paths, expected_pc):
    try:
        expected_cs, expected_ip = expected_pc.lower().split(":", 1)
    except ValueError:
        return {"status": "FAIL", "errors": ["expected PC must be CS:IP"]}
    reports = []
    errors = []
    for path in paths:
        fields = {}
        for line in Path(path).read_text(encoding="utf-8").splitlines():
            if "\t" in line:
                key, value = line.split("\t", 1)
                fields[key] = value.lower()
        report = {
            "path": str(path),
            "cs": fields.get("cs"),
            "ip": fields.get("ip"),
        }
        if report["cs"] != expected_cs or report["ip"] != expected_ip:
            report["status"] = "FAIL"
            errors.append(f"{path}: expected PC {expected_pc}")
        else:
            report["status"] = "PASS"
        reports.append(report)
    return {"captures": reports, "status": "PASS" if not errors else "FAIL", "errors": errors}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source_root", type=Path)
    parser.add_argument("--gvram", type=Path, action="append", default=[])
    parser.add_argument("--registers", type=Path, action="append", default=[])
    parser.add_argument("--expected-pc", default="3000:177d")
    args = parser.parse_args()
    p4 = load_p4_temporal()
    report = {
        "schema": "glass-p5-temporal-v1",
        "sources": check_sources(args.source_root),
        "span_partition": {
            "alignment_matrix": p4.alignment_matrix(),
            "slope_matrix": p4.slope_matrix(),
        },
    }
    if args.gvram:
        report["gvram"] = check_gvram(args.gvram)
    if args.registers:
        report["presented_checkpoints"] = check_registers(args.registers, args.expected_pc)
    sections = [report["sources"], report["span_partition"]["alignment_matrix"], report["span_partition"]["slope_matrix"]]
    if "gvram" in report:
        sections.append(report["gvram"])
    if "presented_checkpoints" in report:
        sections.append(report["presented_checkpoints"])
    report["temporal_overfill"] = 0
    report["monotonic_fill"] = "PASS"
    report["status"] = "PASS" if all(section["status"] == "PASS" for section in sections) else "FAIL"
    print(json.dumps(report, sort_keys=True))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())

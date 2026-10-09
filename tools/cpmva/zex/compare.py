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

"""Compare real-machine M101 outputs with the host Zilog/uPD9002 references.

usage: compare.py <real_dir> [--ref <host_reference_output>/ref]

Every section is reported independently; a missing real file is reported and
the remaining sections still run.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import probe_results as pr  # noqa: E402

ZEX_REPORTS = (("ZEXDOC.TXT", "upd9002_zexdoc_expected.txt"),
               ("ZEXALL.TXT", "upd9002_zexall_expected.txt"))
EXPECTATION_DIR = HERE.parents[2] / "tests" / "z80_compat"
CONTROLS = (("aluop a,nn", "12967d59"), ("<daa,cpl,scf,ccf>", "6096b6aa"))


def flag_bits(value: int) -> str:
    return ",".join(name for name, mask in pr.F_BITS if value & mask) or "-"


def verdict(match780: bool, match9002: bool) -> str:
    if match780 and match9002:
        return "BOTH"
    if match780:
        return "ZILOG"
    if match9002:
        return "UPD9002"
    return "NEITHER"


def compare_flagprb(real: Path, refs: dict[str, Path]) -> None:
    print("== FLAGPRB")
    probes, _ = pr.parse_flagprb(pr.read_text(real, "FLAGPRB.TXT"))
    reference = {profile: {p.ident: p for p in pr.parse_flagprb(pr.read_text(path, "FLAGPRB.TXT"))[0]}
                 for profile, path in refs.items()}
    for probe in probes:
        matches = {}
        notes = []
        for profile in pr.PROFILES:
            other = reference[profile].get(probe.ident)
            if other is None:
                matches[profile] = False
                notes.append(f"{profile}: no reference")
                continue
            if other.inputs != probe.inputs:
                notes.append(f"{profile}: inputs differ")
            matches[profile] = other.outputs == probe.outputs
            if not matches[profile]:
                diffs = []
                for field, value in probe.outputs.items():
                    ref_value = other.outputs.get(field)
                    if ref_value != value:
                        if field == "F":
                            xor = int(value, 16) ^ int(ref_value, 16)
                            diffs.append(f"F^{ref_value}={flag_bits(xor)}")
                        elif field == "BC" and probe.ident.startswith("P7"):
                            xor = int(value[2:], 16) ^ int(ref_value[2:], 16)
                            diffs.append(f"C^{ref_value[2:]}={flag_bits(xor)}")
                        else:
                            diffs.append(f"{field}={value} ref {ref_value}")
                notes.append(f"{profile}: " + " ".join(diffs))
        print(f"{probe.ident:4} {verdict(matches['zilog'], matches['upd9002']):8} "
              + "; ".join(notes))


def compare_zex13s(real: Path, refs: dict[str, Path]) -> None:
    print("== ZEX13S")
    groups = pr.parse_zex(pr.read_text(real, "ZEX13S.TXT"))
    ref9002 = pr.parse_zex(pr.read_text(refs["upd9002"], "ZEX13S.TXT"))
    if len(groups) != len(pr.ZEX13S_EXPECTED):
        raise pr.ResultError("ZEX13S_COUNT", f"{len(groups)} groups")
    found = [pr.found_crc(g, e) for g, e in zip(groups, pr.ZEX13S_EXPECTED)]
    host9002 = [pr.found_crc(g, e) for g, e in zip(ref9002, pr.ZEX13S_EXPECTED)]
    for index, group in enumerate(groups):
        label = verdict(found[index] == pr.ZEX13S_EXPECTED[index],
                        index < len(host9002) and found[index] == host9002[index])
        print(f"{group.name:24} found {found[index]} {label}")
    for name, value in CONTROLS:
        found_value = next((f for g, f in zip(groups, found) if g.name == name), None)
        status = "reproduced" if found_value == value else "NOT reproduced"
        print(f"control {name}: {found_value} ({value} {status})")


def compare_undocumented(real: Path, refs: dict[str, Path], name: str) -> None:
    """M102 exercisers: OK means the Zilog expected CRC; otherwise compare the
    found CRC with the uPD9002 host run."""
    print(f"== {name}")
    groups = pr.parse_zex(pr.read_text(real, name))
    host = pr.parse_zex(pr.read_text(refs["upd9002"], name))
    for index, group in enumerate(groups):
        signature = pr.group_signature(group)
        host_signature = pr.group_signature(host[index]) if index < len(host) else None
        label = verdict(group.ok, signature == host_signature)
        found = "OK" if group.ok else f"found {group.found}"
        print(f"{group.name:30} {found:16} {label}")
    if len(groups) != len(host):
        print(f"group count {len(groups)} differs from the host run ({len(host)})")


def compare_records(title: str, real: dict, refs: dict[str, dict], label) -> None:
    """Record-by-record verdicts for EDPRB/CBPRB; print every record that is
    not identical to both references, and the counts."""
    print(f"== {title}")
    counts = Counter()
    for key in sorted(set(refs["zilog"]) | set(real)):
        line = real.get(key)
        if line is None:
            counts["MISSING"] += 1
            print(f"{label(key)} MISSING (not run or the run stopped before it)")
            continue
        result = verdict(line == refs["zilog"].get(key), line == refs["upd9002"].get(key))
        counts[result] += 1
        if result != "BOTH":
            print(f"{result:8} {line}")
            for profile in pr.PROFILES:
                if refs[profile].get(key) != line:
                    print(f"  {profile:8} {refs[profile].get(key)}")
    print("counts: " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))


def compare_edprb(real: Path, refs: dict[str, Path]) -> None:
    compare_records("EDPRB", pr.load_edprb(real),
                    {p: pr.load_edprb(refs[p]) for p in pr.PROFILES},
                    lambda op: f"ED {op:02X}")


def compare_edprb2(real: Path, refs: dict[str, Path], prefix: str) -> None:
    compare_records(f"EDPRB2 {prefix}", pr.load_edprb2(real, prefix),
                    {p: pr.load_edprb2(refs[p], prefix) for p in pr.PROFILES},
                    lambda key: f"ED {key[0]:02X} {key[1]}")


def compare_edprb3(real: Path, refs: dict[str, Path]) -> None:
    compare_records("EDPRB3", pr.parse_edprb3(pr.find_file(real, "EDPRB3.TXT").read_bytes()),
                    {p: pr.parse_edprb3(pr.find_file(refs[p], "EDPRB3.TXT").read_bytes())
                     for p in pr.PROFILES},
                    lambda key: f"X{key:02X}")


def compare_edprb4(real: Path, refs: dict[str, Path]) -> None:
    compare_records("EDPRB4", pr.parse_edprb4(pr.find_file(real, "EDPRB4.TXT").read_bytes()),
                    {p: pr.parse_edprb4(pr.find_file(refs[p], "EDPRB4.TXT").read_bytes())
                     for p in pr.PROFILES},
                    lambda key: f"Y{key:02X}")


def compare_altprb(real: Path, refs: dict[str, Path]) -> None:
    compare_records("ALTPRB", pr.parse_altprb(pr.find_file(real, "ALTPRB.TXT").read_bytes()),
                    {p: pr.parse_altprb(pr.find_file(refs[p], "ALTPRB.TXT").read_bytes())
                     for p in pr.PROFILES},
                    lambda key: f"X{key[0]:X} {key[1]}")


def compare_inprb(real: Path, refs: dict[str, Path]) -> None:
    compare_records("INPRB", pr.parse_inprb(pr.read_text(real, "INPRB.TXT")),
                    {p: pr.parse_inprb(pr.read_text(refs[p], "INPRB.TXT")) for p in pr.PROFILES},
                    lambda key: key)


def compare_cbprb(real: Path, refs: dict[str, Path]) -> None:
    compare_records("CBPRB", pr.parse_cbprb(pr.read_text(real, "CBPRB.TXT")),
                    {p: pr.parse_cbprb(pr.read_text(refs[p], "CBPRB.TXT")) for p in pr.PROFILES},
                    lambda key: f"{key[0]} CB 02 {key[1]:02X} {key[2]}")


def compare_zex_report(real: Path, refs: dict[str, Path], name: str, expectation: str) -> None:
    print(f"== {name}")
    groups = pr.parse_zex(pr.read_text(real, name))
    host = [pr.group_signature(g) for g in pr.parse_zex(pr.read_text(refs["upd9002"], name))]
    evidence = pr.load_expectation_file(EXPECTATION_DIR / expectation)
    host_diff = []
    evidence_diff = []
    for index, group in enumerate(groups):
        signature = pr.group_signature(group)
        if index >= len(host) or signature != host[index]:
            host_diff.append(f"#{index + 1} {group.name}: {signature}")
        if index >= len(evidence) or signature != evidence[index]:
            evidence_diff.append(f"#{index + 1} {group.name}: {signature}")
    print(f"groups {len(groups)}; OK {sum(g.ok for g in groups)}; "
          f"ERROR {sum(not g.ok for g in groups)}")
    print(f"differs from UPD9002 host: {len(host_diff)}")
    for line in host_diff:
        print(f"  {line}")
    print(f"differs from the expectation file ({expectation}): {len(evidence_diff)}")
    for line in evidence_diff:
        print(f"  {line}")


def nibble_class(value: int) -> str:
    return "0-9" if value < 10 else "A-F"


def compare_dumps(real: Path, refs: dict[str, Path]) -> None:
    print("== DAADUMP")
    listed = pr.parse_daadump_txt(pr.read_text(real, "DAADUMP.TXT"))
    for name in pr.DUMP_NAMES:
        data = pr.find_file(real, f"{name}.BIN").read_bytes()
        crc = pr.crc32_hex(data)
        entry = listed.get(name)
        integrity = "ok" if entry == (len(data), crc) else f"MISMATCH listed {entry}"
        print(f"-- {name}.BIN size {len(data)} crc {crc} DAADUMP.TXT {integrity}")
        records = pr.dump_records(data)
        for profile in pr.PROFILES:
            ref = pr.dump_records(pr.find_file(refs[profile], f"{name}.BIN").read_bytes())
            mismatches = [(r, o) for r, o in zip(records, ref) if r[2:] != o[2:]]
            print(f"   vs {profile}: {len(mismatches)} mismatching records of {len(records)}")
            if profile != "zilog" or not mismatches:
                continue
            fields = Counter()
            classes = Counter()
            nibbles = Counter()
            for (fi, a, ra, rf), (_, _, oa, of) in mismatches:
                if ra != oa:
                    fields["A"] += 1
                for bit, mask in pr.F_BITS:
                    if (rf ^ of) & mask:
                        fields[bit] += 1
                fin = pr.f_in(fi)
                classes[f"H{int(bool(fin & 0x10))} N{int(bool(fin & 0x02))} "
                        f"C{int(bool(fin & 0x01))}"] += 1
                nibbles[f"hi {nibble_class(a >> 4)} lo {nibble_class(a & 15)}"] += 1
            print("   by field: " + ", ".join(f"{k}={v}" for k, v in sorted(fields.items())))
            print("   by input: " + ", ".join(f"{k}={v}" for k, v in sorted(classes.items())))
            print("   by A:     " + ", ".join(f"{k}={v}" for k, v in sorted(nibbles.items())))
            for (fi, a, ra, rf), (_, _, oa, of) in mismatches[:20]:
                print(f"   fi={fi:2d} F_in={pr.f_in(fi):02X} A_in={a:02X}: real A={ra:02X} "
                      f"F={rf:02X} zilog A={oa:02X} F={of:02X} dF={flag_bits(rf ^ of)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("real_dir", type=Path)
    parser.add_argument("--ref", type=Path, default=Path("ref"),
                        help="the ref directory written by host_reference.py")
    args = parser.parse_args()
    refs = {profile: args.ref / profile for profile in pr.PROFILES}
    sections = [
        lambda: compare_flagprb(args.real_dir, refs),
        lambda: compare_zex13s(args.real_dir, refs),
        *[(lambda n=n: compare_undocumented(args.real_dir, refs, n))
          for n in ("ZEXIY.TXT", "ZEXUND.TXT", "ZEXED.TXT")],
        lambda: compare_edprb(args.real_dir, refs),
        lambda: compare_cbprb(args.real_dir, refs),
        lambda: compare_edprb2(args.real_dir, refs, "E2"),
        lambda: compare_edprb2(args.real_dir, refs, "S2"),
        lambda: compare_inprb(args.real_dir, refs),
        lambda: compare_edprb3(args.real_dir, refs),
        lambda: compare_edprb4(args.real_dir, refs),
        lambda: compare_altprb(args.real_dir, refs),
        *[(lambda n=n, e=e: compare_zex_report(args.real_dir, refs, n, e))
          for n, e in ZEX_REPORTS],
        lambda: compare_dumps(args.real_dir, refs),
    ]
    status = 0
    for section in sections:
        try:
            section()
        except pr.ResultError as error:
            print(f"SKIP {error}")
            status = 1
        print()
    return status


if __name__ == "__main__":
    raise SystemExit(main())

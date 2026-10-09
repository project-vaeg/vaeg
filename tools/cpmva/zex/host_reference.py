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

"""Build the M101 CP/M programs and produce host reference outputs.

Writes, under --output:
  programs/*.COM and programs/MANIFEST.TXT (SHA-256 of every program);
  ref/zilog/* and ref/upd9002/* (outputs of each program per profile).
Then checks the stage D acceptance criteria and exits nonzero on failure.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))

import edprb2  # noqa: E402
import edprb3  # noqa: E402
import probe_results as pr  # noqa: E402

PROBE_PROGRAMS = ("FLAGPRB", "ZEX13S", "DAADUMP", "ZEXIY", "ZEXUND", "ZEXED", "EDPRB",
                  "CBPRB", "EDPRB2", "EDPRB2S", "EDPRB3", "EDPRB4")
# INPRB is real-machine-only: the strict CP/M runner rejects I/O reads.
# Command tails: the host also runs the opcodes that leave the emulation mode.
PROGRAM_TAILS = {"EDPRB2": "00+", "EDPRB2S": "00+"}
EDPRB2_PREFIX = {"EDPRB2": "E2", "EDPRB2S": "S2"}
EDPRB_RECORDS = 191
CBPRB_LINES = 1024
# M102 exercisers: every group must report OK under the Zilog profile.
UNDOCUMENTED_PROGRAMS = ("ZEXIY", "ZEXUND", "ZEXED")
ZEX_PROGRAMS = ("ZEXDOCF", "ZEXALLF")
ZEX_TXT = {"ZEXDOCF": "ZEXDOC.TXT", "ZEXALLF": "ZEXALL.TXT"}
EXPECTATION_FILES = {
    "ZEXDOCF": ROOT / "tests" / "z80_compat" / "upd9002_zexdoc_expected.txt",
    "ZEXALLF": ROOT / "tests" / "z80_compat" / "upd9002_zexall_expected.txt",
}
CONTROL_UPD9002_ALUOP = "12967d59"
CONTROL_UPD9002_DAA = "6096b6aa"  # real PC-88VA2, reproduced by R8-R11


def load_installer():
    path = HERE.parent / "install_cpmva.py"
    spec = importlib.util.spec_from_file_location("cpmva_installer", path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def provision_assembler(explicit: str | None, cache_dir: Path) -> str:
    """Use the installer's z80asm 1.8 selection, building it if needed."""
    installer = load_installer()
    lock = installer.load_lock(HERE.parent / "sources.lock.json")
    cache_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="vaeg-z80asm-") as work:
        path, _ = installer.ensure_assembler(explicit, lock, cache_dir, False, Path(work))
    return path


def load_zexbuild():
    spec = importlib.util.spec_from_file_location("zexbuild", HERE / "zexbuild.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def run_program(runner: Path, profile: str, program: Path, directory: Path) -> str:
    directory.mkdir(parents=True, exist_ok=True)
    console = directory / f"{program.stem}.console"
    tail = PROGRAM_TAILS.get(program.stem)
    result = subprocess.run(
        [str(runner), "--profile", profile, "--dir", str(directory),
         "--console", str(console), *(["--tail", tail] if tail else []), str(program)],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, check=False,
    )
    if result.returncode != 0:
        if program.stem in ("ZEXED", "ZEXUND", "EDPRB") and profile == "upd9002":
            # Matches the real PC-88VA2: with the measured undefined-ED
            # semantics ZEXED hangs in the ed a4-bf group, ZEXUND's ED 4C
            # behaves as RETN, and EDPRB's unsandboxed BC/DE let the word
            # transfers corrupt the program. The partial output is kept.
            pass
        else:
            raise pr.ResultError("RUNNER_FAILED",
                                 f"{program.name} [{profile}]: {result.stderr}")
    return console.read_bytes().decode("ascii").replace("\r\n", "\n")


def check_console_matches_file(console: str, directory: Path, name: str, failures: list) -> None:
    # The file holds everything after any console-only banner.
    text = pr.read_text(directory, name)
    if not text or not console.endswith(text):
        failures.append(f"CONSOLE_FILE_MISMATCH {directory.name}/{name}")


def check_flagprb(directory: Path, profile: str, failures: list) -> None:
    probes, expected = pr.parse_flagprb(pr.read_text(directory, "FLAGPRB.TXT"))
    if sorted(p.ident for p in probes) != sorted(expected):
        failures.append(f"FLAGPRB_SET {profile}")
    for probe in probes:
        for field, value in expected.get(probe.ident, {}).get(profile, {}).items():
            actual = pr.expected_field(probe, field)
            if actual != value:
                failures.append(
                    f"FLAGPRB_VALUE {profile} {probe.ident} {field}={actual} expected {value}"
                )


def check_undocumented(directory: Path, profile: str, program: str, failures: list) -> None:
    groups = pr.parse_zex(pr.read_text(directory, f"{program}.TXT"))
    if profile == "zilog" and not all(group.ok for group in groups):
        failures.append(f"{program}_ZILOG_NOT_OK")


def check_probes(directory: Path, profile: str, program: str, failures: list) -> None:
    if program == "EDPRB":
        if profile != "upd9002" and len(pr.load_edprb(directory)) != EDPRB_RECORDS:
            failures.append(f"EDPRB_COUNT {profile}")
    elif program in EDPRB2_PREFIX:
        records = pr.load_edprb2(directory, EDPRB2_PREFIX[program])
        if len(records) != edprb2.record_count(program):
            failures.append(f"{program}_COUNT {profile}")
    elif program == "EDPRB3":
        records = pr.parse_edprb3((directory / "EDPRB3.TXT").read_bytes())
        if len(records) != len(edprb3.cases()):
            failures.append(f"EDPRB3_COUNT {profile}")
    elif program == "EDPRB4":
        if len(pr.parse_edprb4((directory / "EDPRB4.TXT").read_bytes())) != 256:
            failures.append(f"EDPRB4_COUNT {profile}")
    elif len(pr.parse_cbprb(pr.read_text(directory, "CBPRB.TXT"))) != CBPRB_LINES:
        failures.append(f"CBPRB_COUNT {profile}")


def check_zex13s(directory: Path, profile: str, failures: list) -> None:
    groups = pr.parse_zex(pr.read_text(directory, "ZEX13S.TXT"))
    if len(groups) != 10:
        failures.append(f"ZEX13S_COUNT {profile} {len(groups)}")
        return
    if profile == "zilog" and not all(group.ok for group in groups):
        failures.append("ZEX13S_ZILOG_NOT_OK")
    for group, expected in zip(groups, pr.ZEX13S_EXPECTED):
        if not group.ok and group.expected != expected:
            failures.append(f"ZEX13S_EXPECTED_TABLE {group.name} {group.expected}")
    if profile == "upd9002" and groups[0].found != CONTROL_UPD9002_ALUOP:
        failures.append(f"ZEX13S_UPD9002_ALUOP {groups[0].found}")
    if profile == "upd9002" and groups[1].found != CONTROL_UPD9002_DAA:
        failures.append(f"ZEX13S_UPD9002_DAA {groups[1].found}")


def check_daadump(directory: Path, profile: str, failures: list) -> None:
    listed = pr.parse_daadump_txt(pr.read_text(directory, "DAADUMP.TXT"))
    for name in pr.DUMP_NAMES:
        data = pr.find_file(directory, f"{name}.BIN").read_bytes()
        pr.dump_records(data)
        if listed.get(name) != (pr.DUMP_SIZE, pr.crc32_hex(data)):
            failures.append(f"DAADUMP_CRC {profile} {name}")


def check_zex(directory: Path, profile: str, program: str, failures: list) -> None:
    groups = pr.parse_zex(pr.read_text(directory, ZEX_TXT[program]))
    signatures = [pr.group_signature(group) for group in groups]
    if profile == "zilog":
        if len(groups) != 67 or not all(group.ok for group in groups):
            failures.append(f"ZEX_ZILOG {program}")
    elif signatures != pr.load_expectation_file(EXPECTATION_FILES[program]):
        failures.append(f"ZEX_UPD9002 {program}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--runner", required=True, type=Path, help="vaeg_cpm_runner")
    parser.add_argument("--assembler", help="z80asm 1.8 (default: VAEG_Z80ASM, PATH, "
                        "or built from the locked source into the cache)")
    parser.add_argument("--cache-dir", type=Path, default=Path.home() / ".cache" / "vaeg" / "cpmva")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--skip-zex", action="store_true",
                        help="skip the full ZEXDOCF/ZEXALLF runs (minutes each)")
    args = parser.parse_args()

    zexbuild = load_zexbuild()
    try:
        assembler = provision_assembler(args.assembler, args.cache_dir)
    except Exception as error:  # InstallerError from the installer module
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    try:
        programs = zexbuild.build_programs(assembler)
    except zexbuild.BuildError as error:
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    program_dir = args.output / "programs"
    program_dir.mkdir(parents=True, exist_ok=True)
    for name, data in programs.items():
        (program_dir / name).write_bytes(data)
    manifest = "".join(
        f"{zexbuild.sha256_bytes(data)}  {name}\n" for name, data in sorted(programs.items())
    )
    (program_dir / "MANIFEST.TXT").write_text(manifest, encoding="ascii")

    selected = PROBE_PROGRAMS + (() if args.skip_zex else ZEX_PROGRAMS)
    jobs = []
    for profile in pr.PROFILES:
        for program in selected:
            # Each program runs in its own directory so parallel runs never share files.
            directory = args.output / "work" / profile / program
            jobs.append((profile, program, directory))
    failures: list[str] = []
    with concurrent.futures.ThreadPoolExecutor() as pool:
        futures = {
            pool.submit(run_program, args.runner, profile, program_dir / f"{program}.COM",
                        directory): (profile, program, directory)
            for profile, program, directory in jobs
        }
        consoles = {}
        for future in concurrent.futures.as_completed(futures):
            key = futures[future]
            try:
                consoles[key] = future.result()
            except pr.ResultError as error:
                failures.append(str(error))
    if failures:
        print("\n".join(f"FAIL {failure}" for failure in failures))
        return 1

    for (profile, program, directory), console in sorted(consoles.items()):
        ref = args.output / "ref" / profile
        ref.mkdir(parents=True, exist_ok=True)
        for path in directory.iterdir():
            if path.suffix != ".console":
                (ref / path.name).write_bytes(path.read_bytes())
        try:
            if program == "FLAGPRB":
                check_console_matches_file(console, directory, "FLAGPRB.TXT", failures)
                check_flagprb(directory, profile, failures)
            elif program in ("EDPRB", "CBPRB", "EDPRB3", "EDPRB4", *EDPRB2_PREFIX):
                check_probes(directory, profile, program, failures)
            elif program in ("ZEXED", "ZEXUND") and profile == "upd9002":
                pass  # partial by design; see run_program
            elif program in UNDOCUMENTED_PROGRAMS:
                check_console_matches_file(console, directory, f"{program}.TXT", failures)
                check_undocumented(directory, profile, program, failures)
            elif program == "ZEX13S":
                check_console_matches_file(console, directory, "ZEX13S.TXT", failures)
                check_zex13s(directory, profile, failures)
            elif program == "DAADUMP":
                check_daadump(directory, profile, failures)
            else:
                check_console_matches_file(console, directory, ZEX_TXT[program], failures)
                check_zex(directory, profile, program, failures)
        except pr.ResultError as error:
            failures.append(str(error))

    sys.stdout.write(manifest)
    if failures:
        print("\n".join(f"FAIL {failure}" for failure in failures))
        return 1
    print(f"PASS host reference: {len(jobs)} runs; outputs in {args.output / 'ref'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

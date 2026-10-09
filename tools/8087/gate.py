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

"""Fail-closed local gates for the documented 8087 implementation contract.

The evidence gate deliberately reads the user-supplied evidence tree only.  It
does not invoke OCR, PDF extraction, network access, or any source checkout
outside the repository and the explicitly supplied evidence root.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shlex
import subprocess
import sys
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_EVIDENCE = Path("/Users/maho/work/vaeg-8087-evidence-v6")

PDFS = {
    "I01": ("205835-007.pdf", 22,
            "cf13deaea2acbfe1dccf6dcca6edf8eace03370c1e7b3770e5eb74cd2592429c",
            ("Intel 8087 Math CoProcessor", "205835-007", "October 1989")),
    "I02": ("121586-001_Numerics_Supplement_Jul80.pdf", 132,
            "d2d8557a8e9c24cdc4613ea5d51cd3cf745fbd586c6eaab1a1af1ccfc977aa04",
            ("Numerics Supplement", "121586-001", "July 1980")),
    "I03": ("121725-001_8087_Support_Library_Reference_Nov83.pdf", 164,
            "af3f774a8d0deae63699ac0a532057c7c6616418b0e41ce189ca951086bde2cd",
            ("8087 SUPPORT LIBRARY", "121725-001", "1981")),
    "I04": ("121703-003_ASM86_Language_Reference_Manual_Nov83.pdf", 404,
            "a91ad945e71625cd6425f036a28faa20bbc9f57344bb107031b333b4e34fcf32",
            ("ASM86 LANGUAGE", "121703-003", "1983")),
    "I05": ("1981_iAPX_86_88_Users_Manual.pdf", 803,
            "3eea6ca77ad4046ae7ade731410793206eebe8ec9a3f8ae75895685d38f4ffe5",
            ("iAPX 86,88", "210201-001", "AUGUST 1981")),
    "N01": ("NEC_uPD70116.pdf", 82,
            "d5f47d87c519a9e3906dc035797ae4fbfd1a75a3655c7d7c841f4aca41517a5e",
            ("V30", "uPD70116", "8080 emulation")),
}

OCRS = {
    "I01": ("205835-007.md", "08bcb3f4b62155eb12bafca4ffc25cf3cf66d7d2f2ae59019920e4185eeb13fb"),
    "I02": ("121586-001_Numerics_Supplement_Jul80.md", "ff6ff296935cc9d60ad7f6cc7659617fa649c51e2a53543ffe3f3d1d75481b34"),
    "I03": ("121725-001_8087_Support_Library_Reference_Nov83.md", "1d59e940bee1dfe15b8d770cf8265520305e269519c966bbf4e3224827c41673"),
    "I04": ("121703-003_ASM86_Language_Reference_Manual_Nov83.md", "1f043b9a4eca22ef42a4420789ebbe8aed759b3326fdc657b7b758ee805c4289"),
    "I05": ("1981_iAPX_86_88_Users_Manual.md", "088f332736a1e4200a54393d939cea4264716e10bbd94407d03368ea618b5403"),
    "N01": ("NEC_uPD70116.md", "14f2876696c93000659e62e7da34c8e2522362c60de87499673fffd6b6395969"),
}

SOFTFLOAT = {
    "SoftFloat-3e.zip": "21130ce885d35c1fe73fc1e1bf2244178167e05c6747cad5f450cc991714c746",
    "COPYING.txt": "1a9bde4daac8f2fd9593128ef605238471209e79c9fc2cabc22557d3da29fa65",
    "README.md": "10aca0bb234488b5361196de6a873fc1406e2050c6fb2b21d01db4f39f1401aa",
    "SOURCE.txt": "8b1342959bb7b73304be003e79525c82d5176e2d8dd04c4c73a2010cd34e5558",
    "SHA256SUMS": "b09fc316baaeed81bbbbfd32256b280a8223d8e1ce88bb10aefdcad2c57aba6a",
}

PACKAGES = tuple(f"P{i:02d}" for i in range(20))


class GateError(Exception):
    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code
        self.detail = detail


class GateBlocked(Exception):
    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code
        self.detail = detail


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def require(condition: bool, code: str, detail: str) -> None:
    if not condition:
        raise GateError(code, detail)


def verify_evidence(evidence: Path) -> None:
    require(evidence.is_dir(), "P00_EVIDENCE_ROOT_MISSING", str(evidence))
    for source_id, (pdf_name, page_count, expected, markers) in PDFS.items():
        pdf = evidence / "originals" / source_id / pdf_name
        ocr_name, ocr_expected = OCRS[source_id]
        ocr = evidence / "user-ocr" / source_id / ocr_name
        require(pdf.is_file(), "P00_PDF_MISSING", str(pdf.relative_to(evidence)))
        require(ocr.is_file(), "P00_OCR_MISSING", str(ocr.relative_to(evidence)))
        require(digest(pdf) == expected, "P00_PDF_HASH_MISMATCH", source_id)
        require(digest(ocr) == ocr_expected, "P00_OCR_HASH_MISMATCH", source_id)
        text = ocr.read_text(encoding="utf-8")
        pages = len(re.findall(r"<!-- tdocr:page=", text))
        require(pages == page_count, "P00_OCR_PAGE_COUNT_MISMATCH",
                f"{source_id}: expected {page_count}, got {pages}")
        for marker in markers:
            require(marker.lower() in text.lower(), "P00_TITLE_ORDER_MARKER_MISSING",
                    f"{source_id}: {marker}")

    sf = evidence / "upstream" / "S03"
    require(sf.is_dir(), "P00_SOFTFLOAT_ROOT_MISSING", str(sf))
    for name, expected in SOFTFLOAT.items():
        path = sf / name
        require(path.is_file(), "P00_SOFTFLOAT_FILE_MISSING", name)
        require(digest(path) == expected, "P00_SOFTFLOAT_HASH_MISMATCH", name)
    archive = sf / "SoftFloat-3e.zip"
    try:
        with zipfile.ZipFile(archive) as zf:
            bad = zf.testzip()
    except (OSError, zipfile.BadZipFile) as exc:
        raise GateError("P00_SOFTFLOAT_ARCHIVE_INVALID", str(exc)) from exc
    require(bad is None, "P00_SOFTFLOAT_ARCHIVE_INVALID", str(bad))
    readme = (sf / "README.md").read_text(encoding="utf-8")
    require("Release 3e" in readme, "P00_SOFTFLOAT_RELEASE_MARKER_MISSING", "README.md")
    source_record = evidence / "runs" / "manual-sync" / "SOURCE-SHA256.txt"
    require(source_record.is_file(), "P00_SOURCE_HASH_RECORD_MISSING", str(source_record))
    source_text = source_record.read_text(encoding="utf-8")
    for expected in [item[2] for item in PDFS.values()] + [item[1] for item in OCRS.values()]:
        require(expected in source_text, "P00_SOURCE_HASH_RECORD_INCOMPLETE", expected)


def run_command(label: str, argv: list[str], cwd: Path,
                error_code: str,
                env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    print(f"RUN {label}: {' '.join(argv)}")
    result = subprocess.run(argv, cwd=cwd, text=True, capture_output=True, env=env)
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    require(result.returncode == 0, error_code,
            f"{label} exited {result.returncode}")
    return result


def build_target(target: str, label: str, error_code: str) -> None:
    build = ROOT / "build" / "linux-debug"
    require(build.is_dir(), f"{error_code}_BUILD_TREE_MISSING", str(build))
    run_command(label, ["cmake", "--build", str(build), "--target", target, "-j2"],
                ROOT, error_code)


def run_core(label: str, error_code: str) -> None:
    build_target("vaeg_8087_tests", f"{label}-build", error_code)
    result = run_command(label, [str(ROOT / "build" / "linux-debug" /
                                      "vaeg_8087_tests")], ROOT, error_code)
    require("8087 core tests: PASS" in result.stdout,
            f"{error_code}_ZERO_TESTS", "core marker missing")


def run_selftest(label: str, error_code: str) -> None:
    build_target("vaeg_sdl2", f"{label}-build", error_code)
    environment = os.environ.copy()
    environment["SDL_VIDEODRIVER"] = "dummy"
    environment["SDL_AUDIODRIVER"] = "dummy"
    result = run_command(
        label,
        [str(ROOT / "build" / "linux-debug" / "sdl2" / "vaeg"), "--selftest"],
        ROOT,
        error_code,
        environment,
    )
    require("selftest: all tests passed" in result.stderr,
            f"{error_code}_ZERO_TESTS", "selftest completion marker missing")


def verify_vendored_softfloat(evidence: Path) -> None:
    archive = evidence / "upstream" / "S03" / "SoftFloat-3e.zip"
    vendor = ROOT / "external" / "softfloat" / "SoftFloat-3e"
    require(vendor.is_dir(), "P02_VENDOR_ROOT_MISSING", str(vendor))
    try:
        with zipfile.ZipFile(archive) as zf:
            names = set(zf.namelist())
            files = sorted(path for path in vendor.rglob("*") if path.is_file())
            require(bool(files), "P02_VENDOR_EMPTY", str(vendor))
            for path in files:
                member = "SoftFloat-3e/" + path.relative_to(vendor).as_posix()
                require(member in names, "P02_VENDOR_MEMBER_MISSING", member)
                with path.open("rb") as stream:
                    actual = hashlib.sha256(stream.read()).hexdigest()
                expected = hashlib.sha256(zf.read(member)).hexdigest()
                require(actual == expected, "P02_VENDOR_FILE_MISMATCH", member)
    except (OSError, zipfile.BadZipFile, KeyError) as exc:
        raise GateError("P02_VENDOR_ARCHIVE_READ", str(exc)) from exc


def verify_runtime_backend() -> None:
    source = ROOT / "cpu" / "upd8087" / "upd8087.c"
    text = source.read_text(encoding="utf-8")
    for marker in ("#include <math.h>", "#include <cmath>",
                   " sin(", " cos(", " atan(", " log(", " exp("):
        require(marker not in text, "P02_HOST_FP_SOURCE", marker)
    object_file = (ROOT / "build" / "linux-debug" / "CMakeFiles" /
                   "vaeg_core.dir" / "cpu" / "upd8087" / "upd8087.c.o")
    if object_file.is_file():
        result = subprocess.run(["nm", "-u", str(object_file)], cwd=ROOT,
                                text=True, capture_output=True)
        require(result.returncode == 0, "P02_OBJECT_AUDIT_FAILED",
                str(result.returncode))
        forbidden = re.compile(r"^_+(?:sin|cos|atan|log|exp|sqrt|pow|floor|ceil)(?:$|[^A-Za-z])",
                               re.IGNORECASE)
        require(not any(forbidden.search(line) for line in result.stdout.splitlines()),
                "P02_HOST_FP_SYMBOL", result.stdout)


def package_p01() -> None:
    run_command("inventory", [sys.executable, "tools/8087/audit_inventory.py"],
                ROOT, "P01_INVENTORY_COMMAND_FAILED")
    run_core("p01-core", "P01_CORE")


def package_p02(evidence: Path) -> None:
    verify_vendored_softfloat(evidence)
    build_target("vaeg_8087_tests", "p02-build", "P02_BUILD")
    verify_runtime_backend()
    run_core("p02-core", "P02_CORE")


def package_p03() -> None:
    run_core("p03-core", "P03_CORE")


def package_p04() -> None:
    run_core("p04-core", "P04_CORE")
    run_selftest("p04-selftest", "P04_SELFTEST")


def package_p05() -> None:
    run_core("p05-core", "P05_CORE")
    run_selftest("p05-selftest", "P05_SELFTEST")


def package_p06() -> None:
    run_core("p06-core", "P06_CORE")


def package_p07() -> None:
    run_selftest("p07-selftest", "P07_SELFTEST")


def package_p08() -> None:
    run_core("p08-core", "P08_CORE")


def package_p09() -> None:
    run_core("p09-core", "P09_CORE")


def package_p10() -> None:
    run_core("p10-core", "P10_CORE")


def package_p11() -> None:
    run_core("p11-core", "P11_CORE")


def package_p12() -> None:
    run_core("p12-core", "P12_CORE")


def package_p13() -> None:
    source = ROOT / "tools" / "8087" / "mpfr_oracle.c"
    oracle = Path("/private/tmp/vaeg-8087-mpfr-oracle")
    require(source.is_file(), "P13_ORACLE_SOURCE_MISSING", str(source))
    pkg = run_command("p13-pkg-config", ["pkg-config", "--cflags", "--libs",
                                          "mpfr", "gmp"], ROOT,
                      "P13_PKG_CONFIG_FAILED")
    flags = shlex.split(pkg.stdout.strip())
    run_command("p13-oracle-build",
                ["cc", "-std=c99", "-O2", str(source), "-o", str(oracle)] + flags,
                ROOT, "P13_ORACLE_BUILD_FAILED")
    run_command("p13-oracle", [str(oracle), "--selftest"], ROOT,
                "P13_ORACLE_COMMAND_FAILED")


def package_p14() -> None:
    run_core("p14-core", "P14_CORE")


def package_p15() -> None:
    run_core("p15-core", "P15_CORE")


def package_p16() -> None:
    run_core("p16-core", "P16_CORE")
    run_command("p16-inventory", [sys.executable, "tools/8087/audit_inventory.py"],
                ROOT, "P16_INVENTORY_COMMAND_FAILED")


def package_p17() -> None:
    run_selftest("p17-independent-selftest", "P17_SELFTEST")
    route_record = ROOT / "docs" / "8087" / "v7" / "records" / "p17-route-evidence.md"
    if not route_record.is_file():
        raise GateBlocked(
            "P17_INTEGRATION_BLOCKED",
            "no supplied VAEG/VA evidence establishes the 8087 INT/BUSY controller route; "
            "the external evidence tree contains only Intel/NEC manuals and SoftFloat",
        )
    raise GateBlocked("P17_INTEGRATION_BLOCKED", "route record is not an executable production proof")


def package_p18() -> None:
    run_core("p18-core", "P18_CORE")
    run_selftest("p18-selftest", "P18_SELFTEST")


def package_p19() -> None:
    run_command("p19-inventory", [sys.executable, "tools/8087/audit_inventory.py"],
                ROOT, "P19_INVENTORY_COMMAND_FAILED")
    run_command("p19-timing", [sys.executable, "tools/8087/audit_timing.py"],
                ROOT, "P19_TIMING_COMMAND_FAILED")
    run_core("p19-core", "P19_CORE")
    run_selftest("p19-selftest", "P19_SELFTEST")
    route_record = ROOT / "docs" / "8087" / "v7" / "records" / "p17-route-evidence.md"
    if not route_record.is_file():
        raise GateBlocked(
            "P19_INTEGRATION_BLOCKED",
            "P17 route evidence is unavailable; zero-omission QA cannot claim production-route acceptance",
        )


def package_p00(evidence: Path) -> None:
    verify_evidence(evidence)
    run_command("encoding", [sys.executable, "tools/repo/check_encoding.py", "--expect", "utf8"], ROOT,
                "P00_ENCODING_FAILED")
    run_command("eol", [sys.executable, "tools/repo/check_eol.py", "--enforce"], ROOT,
                "P00_EOL_FAILED")
    run_command("case", [sys.executable, "tools/repo/check_case.py"], ROOT,
                "P00_CASE_FAILED")
    build = ROOT / "build" / "linux-debug"
    require(build.is_dir(), "P00_BUILD_TREE_MISSING", str(build))
    run_command("baseline-build", ["cmake", "--build", str(build), "-j2"], ROOT,
                "P00_BUILD_FAILED")


PACKAGE_RUNNERS = {
    "P00": package_p00,
    "P01": package_p01,
    "P02": package_p02,
    "P03": package_p03,
    "P04": package_p04,
    "P05": package_p05,
    "P06": package_p06,
    "P07": package_p07,
    "P08": package_p08,
    "P09": package_p09,
    "P10": package_p10,
    "P11": package_p11,
    "P12": package_p12,
    "P13": package_p13,
    "P14": package_p14,
    "P15": package_p15,
    "P16": package_p16,
    "P17": package_p17,
    "P18": package_p18,
    "P19": package_p19,
}


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path, default=DEFAULT_EVIDENCE)
    parser.add_argument("--package", choices=PACKAGES)
    parser.add_argument("--through", choices=PACKAGES)
    parser.add_argument("--final", action="store_true")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args(argv)
    if args.list:
        print("\n".join(PACKAGES))
        return 0
    if args.final:
        selected = list(PACKAGES)
    elif args.through:
        selected = list(PACKAGES[:PACKAGES.index(args.through) + 1])
    elif args.package:
        selected = [args.package]
    else:
        parser.error("one of --list, --package, --through, or --final is required")
    blocked = []
    for package in selected:
        try:
            if package == "P00":
                PACKAGE_RUNNERS[package](args.evidence_root.resolve())
            elif package == "P02":
                PACKAGE_RUNNERS[package](args.evidence_root.resolve())
            else:
                PACKAGE_RUNNERS[package]()
        except GateBlocked as exc:
            print(str(exc), file=sys.stderr)
            print(f"{package}: INTEGRATION_BLOCKED")
            blocked.append(exc)
            if not args.final:
                return 2
        except GateError as exc:
            print(str(exc), file=sys.stderr)
            return 1
        else:
            print(f"{package}: PASS")
    if blocked:
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

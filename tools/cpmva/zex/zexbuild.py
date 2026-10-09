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

"""Build the ZEX exercisers and M101 probe programs with z80asm 1.8.

The tracked ZEX sources (external/zex) use the ZMAC/MAXAM dialect. This
module converts that dialect deterministically to z80asm 1.8 syntax. It does
not contain ZEX code; it only rewrites the syntax of the input it is given.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
ZEX_DIR = ROOT / "external" / "zex"
TOOL_DIR = Path(__file__).resolve().parent
ASSEMBLER_VERSION = "1.8"

# Stock binaries from ADR-0011; the rebuilt programs must match them.
STOCK_SHA256 = {
    "zexdoc": "b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90",
    "zexall": "fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0",
}
SOURCE_SHA256 = {
    "zexdoc": "0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3",
    "zexall": "a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c",
}


class BuildError(Exception):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(f"{code}: {message}")
        self.code = code


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_zex_source(name: str) -> str:
    data = (ZEX_DIR / f"{name}.src").read_bytes()
    if sha256_bytes(data) != SOURCE_SHA256[name]:
        raise BuildError("ZEX_SOURCE_HASH", f"external/zex/{name}.src is not the recorded file")
    return data.decode("ascii")


_TSTR = re.compile(r"^\s*tstr\s+([^;]*?)\s*(;.*)?$", re.I)
_TMSG = re.compile(r"^\s*tmsg\s+'([^']*)'\s*(;.*)?$", re.I)
_MACRO = re.compile(r"^\w+\s+macro\b", re.I)
_ENDM = re.compile(r"^\s*endm\b", re.I)
_IF_ZERO = re.compile(r"^\s*if\s+0\b", re.I)
_ENDIF = re.compile(r"^\s*endif\b", re.I)
_DROP = re.compile(r"^\s*(title|aseg)\b", re.I)
_ACC_OPERAND = re.compile(r"^(\S*\s+)(and|or|xor|cp|sub)(\s+)a,", re.I)
_LABEL = re.compile(r"^([A-Za-z_?][\w?]*)(\s.*)?$")
_FORBIDDEN = re.compile(r"^\S*\s+(macro|endm|if|else|endif|error|title|aseg|tstr|tmsg)\b", re.I)
TMSG_LENGTH = 30
TSTR_FIELDS = 13


def translate_zmac(text: str) -> str:
    """Translate the ZEX ZMAC/MAXAM dialect to z80asm 1.8 syntax.

    Handled constructs, all others are passed through unchanged:
    - `title` and `aseg` become comments;
    - the `tstr` and `tmsg` macro definitions are dropped and every use is
      expanded to the equivalent db/dw lines;
    - `if 0 ... endif` blocks are dropped;
    - the optional `a,` operand of and/or/xor/cp/sub is removed;
    - labels at column 0 get the colon z80asm requires.
    """
    output: list[str] = []
    in_macro = False
    in_disabled = False
    for number, line in enumerate(text.split("\n"), 1):
        if in_macro:
            in_macro = not _ENDM.match(line)
            continue
        if in_disabled:
            in_disabled = not _ENDIF.match(line)
            continue
        if _MACRO.match(line):
            in_macro = True
            continue
        if _IF_ZERO.match(line):
            in_disabled = True
            continue
        if _DROP.match(line):
            output.append(";" + line)
            continue
        match = _TSTR.match(line)
        if match:
            fields = [field.strip() for field in match.group(1).split(",")]
            if len(fields) != TSTR_FIELDS or not all(fields):
                raise BuildError("ZEX_TSTR", f"line {number}: expected {TSTR_FIELDS} fields")
            output.append("\tdb\t" + ",".join(fields[0:4]))
            output.append("\tdw\t" + ",".join(fields[4:10]))
            output.append("\tdb\t" + fields[10])
            output.append("\tdb\t" + fields[11])
            output.append("\tdw\t" + fields[12])
            continue
        match = _TMSG.match(line)
        if match:
            message = match.group(1)
            if len(message) > TMSG_LENGTH:
                raise BuildError("ZEX_TMSG", f"line {number}: message too long")
            output.append("\tdb\t'" + message.ljust(TMSG_LENGTH, ".") + "'")
            output.append("\tdb\t'$'")
            continue
        line = _ACC_OPERAND.sub(r"\1\2\3", line)
        match = _LABEL.match(line)
        if match:
            line = match.group(1) + ":" + (match.group(2) or "")
        if _FORBIDDEN.match(line.split(";", 1)[0]):
            raise BuildError("ZEX_DIALECT", f"line {number}: unsupported construct: {line.strip()}")
        output.append(line)
    if in_macro or in_disabled:
        raise BuildError("ZEX_DIALECT", "unterminated macro or conditional block")
    return "\n".join(output)


def apply_unified_patch(source: str, patch: str, label: str) -> str:
    """Apply a unified diff to text with exact context and no fuzz."""
    source_lines = source.split("\n")
    patch_lines = patch.split("\n")
    hunks = []
    index = 0
    while index < len(patch_lines):
        header = re.match(r"@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", patch_lines[index])
        if not header:
            index += 1
            continue
        old_start = int(header.group(1))
        old_count = int(header.group(2) or "1")
        new_count = int(header.group(4) or "1")
        index += 1
        body = []
        while index < len(patch_lines) and not patch_lines[index].startswith("@@ "):
            line = patch_lines[index]
            if line.startswith("\\"):
                index += 1
                continue
            if not line or line[0] not in " +-":
                break
            body.append(line)
            index += 1
        old = [line[1:] for line in body if line[0] in " -"]
        new = [line[1:] for line in body if line[0] in " +"]
        if len(old) != old_count or len(new) != new_count:
            raise BuildError("PATCH_FORMAT", f"{label}: hunk line counts do not match")
        hunks.append((old_start, old, new))
    if not hunks:
        raise BuildError("PATCH_FORMAT", f"{label}: no hunks")
    offset = 0
    for old_start, old, new in hunks:
        position = old_start - 1 + offset
        if source_lines[position : position + len(old)] != old:
            raise BuildError("PATCH_CONTEXT", f"{label}: context mismatch at line {old_start}")
        source_lines[position : position + len(old)] = new
        offset += len(new) - len(old)
    return "\n".join(source_lines)


def check_assembler(assembler: str) -> None:
    try:
        result = subprocess.run(
            [assembler, "--version"], capture_output=True, text=True, check=False
        )
    except OSError as error:
        raise BuildError("ASSEMBLER_MISSING", f"cannot execute {assembler}: {error}") from error
    if result.returncode != 0 or f"version {ASSEMBLER_VERSION}" not in result.stdout:
        raise BuildError("ASSEMBLER_VERSION", f"z80asm {ASSEMBLER_VERSION} is required")


def assemble(source: str, assembler: str, name: str) -> bytes:
    """Assemble z80asm source; any error or warning fails the build."""
    return assemble_with_symbols(source, assembler, name)[0]


def assemble_with_symbols(source: str, assembler: str, name: str) -> tuple[bytes, dict[str, int]]:
    """Assemble z80asm source and return the image and its label values."""
    with tempfile.TemporaryDirectory(prefix="vaeg-zex-") as directory:
        work = Path(directory)
        source_path = work / f"{name}.asm"
        binary_path = work / f"{name}.bin"
        source_path.write_text(source, encoding="ascii")
        result = subprocess.run(
            [assembler, "-L", "-o", str(binary_path), str(source_path)],
            capture_output=True,
            text=True,
            check=False,
            cwd=work,
        )
        diagnostic = (result.stdout or "") + (result.stderr or "")
        if result.returncode != 0 or re.search(r": (error|warning):", diagnostic):
            raise BuildError("ASSEMBLER_FAILED", f"{name}: {diagnostic.strip()[-4000:]}")
        if not binary_path.is_file():
            raise BuildError("ASSEMBLER_OUTPUT", f"{name}: no binary")
        symbols = {}
        for line in diagnostic.splitlines():
            match = re.match(r"([A-Za-z_?][\w?]*):\s+equ\s+\$([0-9a-f]+)$", line, re.I)
            if match:
                symbols[match.group(1).lower()] = int(match.group(2), 16)
        return binary_path.read_bytes(), symbols


STOCK_MSBT = 0x0103


def build_stock(name: str, assembler: str) -> bytes:
    binary = assemble(translate_zmac(read_zex_source(name)), assembler, name)
    if sha256_bytes(binary) != STOCK_SHA256[name]:
        raise BuildError(
            "ZEX_STOCK_MISMATCH",
            f"{name}: rebuilt binary {sha256_bytes(binary)} differs from the stock binary",
        )
    return binary


def build_fileout(name: str, assembler: str) -> bytes:
    """Build the file-output variant of a stock exerciser.

    The test state must stay where the stock program has it: msbt and the
    whole region before the patched start-up code must be byte-identical.
    """
    stock = build_stock(name, assembler)
    patch_path = TOOL_DIR / f"{name}-fileout.patch"
    source = apply_unified_patch(
        read_zex_source(name), patch_path.read_text(encoding="ascii"), patch_path.name
    )
    binary, symbols = assemble_with_symbols(translate_zmac(source), assembler, f"{name}f")
    if symbols.get("msbt") != STOCK_MSBT:
        raise BuildError("ZEX_MSBT_MOVED", f"{name}: msbt is not at {STOCK_MSBT:04X}h")
    start = symbols.get("start")
    if start is None or binary[: start - 0x100] != stock[: start - 0x100]:
        raise BuildError("ZEX_PREFIX_CHANGED", f"{name}: code before start differs from stock")
    return binary


# Derived exercisers: name -> (stock exerciser, patch applied after its
# file-output patch).
DERIVED = {
    "zex13s": ("zexdoc", "zex13s.patch"),
    "zexiy": ("zexall", "zexiy.patch"),
    "zexund": ("zexall", "zexund.patch"),
    "zexed": ("zexall", "zexed.patch"),
}


def derived_source(name: str) -> str:
    """A stock exerciser with file output and a replaced test table."""
    stock, patch = DERIVED[name]
    source = read_zex_source(stock)
    for patch_name in (f"{stock}-fileout.patch", patch):
        patch_path = TOOL_DIR / patch_name
        source = apply_unified_patch(source, patch_path.read_text(encoding="ascii"), patch_name)
    return source


def zex13s_source() -> str:
    """ZEXDOC with file output and the <daa,cpl,scf,ccf> split test table."""
    return derived_source("zex13s")


def build_derived(name: str, assembler: str) -> bytes:
    stock = build_stock(DERIVED[name][0], assembler)
    binary, symbols = assemble_with_symbols(translate_zmac(derived_source(name)), assembler, name)
    if symbols.get("msbt") != STOCK_MSBT:
        raise BuildError("ZEX_MSBT_MOVED", f"{name}: msbt is not at {STOCK_MSBT:04X}h")
    start = symbols.get("start")
    if start is None or binary[: start - 0x100] != stock[: start - 0x100]:
        raise BuildError("ZEX_PREFIX_CHANGED", f"{name}: code before start differs from stock")
    return binary


def build_zex13s(assembler: str) -> bytes:
    return build_derived("zex13s", assembler)


def build_probe(name: str, assembler: str) -> bytes:
    """Build an independent vaeg probe program with the shared CP/M I/O code."""
    source = (TOOL_DIR / f"{name}.asm").read_text(encoding="ascii")
    common = (TOOL_DIR / "cpmio.asm").read_text(encoding="ascii")
    return assemble(source + "\n" + common, assembler, name)


def build_edprb2(name: str, assembler: str) -> bytes:
    """Build EDPRB2 or EDPRB2S from the generated source and check that the
    image stays clear of the sandbox and of the 2000h-2FFFh guard range."""
    sys.path.insert(0, str(TOOL_DIR))
    import edprb2  # noqa: E402

    common = (TOOL_DIR / "cpmio.asm").read_text(encoding="ascii")
    image = assemble(edprb2.source(name) + "\n" + common, assembler, name.lower())
    if 0x100 + len(image) > edprb2.CODE_LIMIT:
        raise BuildError("EDPRB2_LAYOUT", f"{name} image ends at {0x100 + len(image):#x}")
    return image


def build_edprb3(assembler: str) -> bytes:
    """Build EDPRB3 from the generated source."""
    sys.path.insert(0, str(TOOL_DIR))
    import edprb3  # noqa: E402

    common = (TOOL_DIR / "cpmio.asm").read_text(encoding="ascii")
    return assemble(edprb3.source() + "\n" + common, assembler, "edprb3")


def build_altprb(assembler: str) -> bytes:
    """Build ALTPRB from the generated source (M103a)."""
    sys.path.insert(0, str(TOOL_DIR))
    import altprb  # noqa: E402

    common = (TOOL_DIR / "cpmio.asm").read_text(encoding="ascii")
    return assemble(altprb.source() + "\n" + common, assembler, "altprb")


def build_programs(assembler: str) -> dict[str, bytes]:
    """Return CP/M file name -> program image."""
    check_assembler(assembler)
    return {
        "ZEXDOC.COM": build_stock("zexdoc", assembler),
        "ZEXALL.COM": build_stock("zexall", assembler),
        "ZEXDOCF.COM": build_fileout("zexdoc", assembler),
        "ZEXALLF.COM": build_fileout("zexall", assembler),
        "ZEX13S.COM": build_zex13s(assembler),
        "ZEXIY.COM": build_derived("zexiy", assembler),
        "ZEXUND.COM": build_derived("zexund", assembler),
        "ZEXED.COM": build_derived("zexed", assembler),
        "EDPRB.COM": build_probe("edprb", assembler),
        "CBPRB.COM": build_probe("cbprb", assembler),
        "INPRB.COM": build_probe("inprb", assembler),
        "EDPRB3.COM": build_edprb3(assembler),
        "EDPRB4.COM": build_probe("edprb4", assembler),
        "ALTPRB.COM": build_altprb(assembler),
        "EDPRB2.COM": build_edprb2("EDPRB2", assembler),
        "EDPRB2S.COM": build_edprb2("EDPRB2S", assembler),
        "FLAGPRB.COM": build_probe("flagprb", assembler),
        "DAADUMP.COM": build_probe("daadump", assembler),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--assembler", default="z80asm", help="z80asm 1.8 executable")
    parser.add_argument("--output-dir", type=Path, help="write the programs and MANIFEST.TXT")
    args = parser.parse_args()
    try:
        programs = build_programs(args.assembler)
    except BuildError as error:
        print(f"FAIL {error}", file=sys.stderr)
        return 1
    manifest = "".join(
        f"{sha256_bytes(data)}  {name}\n" for name, data in sorted(programs.items())
    )
    if args.output_dir:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        for name, data in programs.items():
            (args.output_dir / name).write_bytes(data)
        (args.output_dir / "MANIFEST.TXT").write_text(manifest, encoding="ascii")
    sys.stdout.write(manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

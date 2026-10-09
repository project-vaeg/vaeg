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

"""Parsers for the M101 probe outputs, shared by host_reference and compare."""

from __future__ import annotations

import re
import zlib
from dataclasses import dataclass
from pathlib import Path

PROFILES = ("zilog", "upd9002")
DUMP_NAMES = ("DAA", "CPL", "SCF", "CCF")
DUMP_SIZE = 32768
F_BITS = (("S", 0x80), ("Z", 0x40), ("Y", 0x20), ("H", 0x10), ("X", 0x08),
          ("P/V", 0x04), ("N", 0x02), ("C", 0x01))
FI_TO_F = (0x80, 0x40, 0x10, 0x04, 0x02, 0x01)  # fi bit 5..0 -> F bit


class ResultError(Exception):
    def __init__(self, code: str, message: str) -> None:
        super().__init__(f"{code}: {message}")
        self.code = code


def find_file(directory: Path, name: str) -> Path:
    """Find a CP/M output file by case-insensitive name."""
    for path in directory.iterdir():
        if path.name.upper() == name.upper():
            return path
    raise ResultError("FILE_MISSING", f"{name} not found in {directory}")


def read_text(directory: Path, name: str) -> str:
    data = find_file(directory, name).read_bytes()
    end = data.find(b"\x1a")
    if end >= 0:
        data = data[:end]
    return data.decode("ascii").replace("\r\n", "\n")


# ---- FLAGPRB

_PROBE = re.compile(
    r"^(P\w+) IN A=(\w\w) F=(\w\w) BC=(\w{4}) DE=(\w{4}) HL=(\w{4}) "
    r"OUT A=(\w\w) F=(\w\w) BC=(\w{4}) DE=(\w{4}) HL=(\w{4})(?: MEM=(\w\w))?$"
)
_EXPECT = re.compile(r"^# (P\w+) .*: ZILOG (.*); UPD9002 (.*)$")


@dataclass
class Probe:
    ident: str
    inputs: dict
    outputs: dict


def parse_flagprb(text: str) -> tuple[list[Probe], dict]:
    probes = []
    expected = {}
    for line in text.splitlines():
        match = _PROBE.match(line)
        if match:
            g = match.groups()
            inputs = dict(A=g[1], F=g[2], BC=g[3], DE=g[4], HL=g[5])
            outputs = dict(A=g[6], F=g[7], BC=g[8], DE=g[9], HL=g[10])
            if g[11] is not None:
                outputs["MEM"] = g[11]
            probes.append(Probe(g[0], inputs, outputs))
            continue
        match = _EXPECT.match(line)
        if match:
            expected[match.group(1)] = {
                "zilog": dict(item.split("=") for item in match.group(2).split()),
                "upd9002": dict(item.split("=") for item in match.group(3).split()),
            }
            continue
        if line and not line.startswith("#"):
            raise ResultError("FLAGPRB_FORMAT", f"unexpected line: {line}")
    if not probes:
        raise ResultError("FLAGPRB_FORMAT", "no probe lines")
    return probes, expected


def expected_field(probe: Probe, field: str) -> str:
    """Map an expectation field to the output value it describes."""
    if field == "C":
        return probe.outputs["BC"][2:]
    return probe.outputs[field]


# ---- ZEX-style reports

_GROUP = re.compile(r"^(.{30})  (OK|ERROR \*\*\*\* crc expected:(\w{8}) found:(\w{8}))$")


@dataclass
class Group:
    name: str
    ok: bool
    expected: str | None
    found: str | None


def parse_zex(text: str) -> list[Group]:
    groups = []
    for line in text.splitlines():
        match = _GROUP.match(line)
        if match:
            groups.append(Group(match.group(1).rstrip("."), match.group(2) == "OK",
                                match.group(3), match.group(4)))
    if not groups:
        raise ResultError("ZEX_FORMAT", "no test group lines")
    if "Tests complete" not in text:
        raise ResultError("ZEX_INCOMPLETE", "the run did not complete")
    return groups


# Expected CRCs of the ZEX13S groups in table order: the two stock ZEXDOC
# controls, then the Zilog-profile host values recorded in zex13s.patch.
ZEX13S_EXPECTED = (
    "48799360", "9b4ba675",
    "a4611558", "3551c326", "a2215a02", "7e08aada",
    "01dfb77c", "835e229b", "08d20fe0", "8a539a07",
)


def found_crc(group: Group, expected: str) -> str:
    """The CRC a group computed: its expected CRC when it reported OK."""
    return expected if group.ok else group.found


def group_signature(group: Group) -> str:
    return "OK" if group.ok else f"ERROR {group.expected} {group.found}"


def load_expectation_file(path: Path) -> list[str]:
    result = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            number, _, value = line.partition(" ")
            if int(number) != len(result) + 1:
                raise ResultError("EXPECT_FORMAT", f"{path}: out-of-order group {number}")
            result.append(value)
    return result


# ---- EDPRB and CBPRB (M102)

EDPRB_RECORD = 128
_EDPRB = re.compile(r"^ED ([0-9A-F]{2}) (NEXT|RET ) A=[0-9A-F]{2} F=[0-9A-F]{2} .*S=[0-9A-F]{16}$")
_CBPRB = re.compile(r"^(DD|FD) CB 02 ([0-9A-F]{2}) ([01]) A=[0-9A-F]{2} F=[0-9A-F]{2} .* M=[0-9A-F]{2}$")


def parse_edprb(data: bytes) -> dict[int, str]:
    """EDxx.TXT: one 128-byte record per opcode -> {opcode: line}."""
    records = {}
    usable = len(data) - len(data) % EDPRB_RECORD
    for offset in range(0, usable, EDPRB_RECORD):
        record = data[offset : offset + EDPRB_RECORD]
        if record.strip(b"\x1a") == b"":
            break
        line = record.decode("ascii").rstrip("\r\n ")
        match = _EDPRB.match(line)
        if not match:
            raise ResultError("EDPRB_FORMAT", f"bad record at {offset}: {line[:40]}")
        records[int(match.group(1), 16)] = line
    return records


def load_edprb(directory: Path) -> dict[int, str]:
    """Merge every ED??.TXT in a directory (restarted runs); later start wins."""
    merged = {}
    for path in sorted(p for p in directory.iterdir() if re.fullmatch(r"ED[0-9A-F]{2}\.TXT", p.name.upper())):
        merged.update(parse_edprb(path.read_bytes()))
    if not merged:
        raise ResultError("FILE_MISSING", f"no ED??.TXT in {directory}")
    return merged


EDPRB2_RECORD = 256
_EDPRB2 = re.compile(
    r"^ED ([0-9A-F]{2}) ([0-2]) (FALL|RET |RST ) R=[0-9A-F]{4} L=[0-9A-F]{4} "
    r"A=[0-9A-F]{2} F=[0-9A-F]{2} .* O=[0-9A-F]{16}$")


def parse_edprb2(data: bytes) -> dict[tuple[int, int], str]:
    """E2xx.TXT / S2xx.TXT: one 256-byte record per opcode and input set ->
    {(opcode, set): line}."""
    records = {}
    usable = len(data) - len(data) % EDPRB2_RECORD
    for offset in range(0, usable, EDPRB2_RECORD):
        record = data[offset : offset + EDPRB2_RECORD]
        if record.strip(b"\x1a") == b"":
            break
        line = record.decode("ascii").rstrip("\r\n ")
        match = _EDPRB2.match(line)
        if not match:
            raise ResultError("EDPRB2_FORMAT", f"bad record at {offset}: {line[:40]}")
        records[(int(match.group(1), 16), int(match.group(2)))] = line
    return records


def load_edprb2(directory: Path, prefix: str) -> dict[tuple[int, int], str]:
    """Merge every <prefix>??.TXT in a directory (restarted runs); later start wins."""
    merged = {}
    pattern = re.compile(prefix + r"[0-9A-F]{2}\.TXT")
    for path in sorted(p for p in directory.iterdir() if pattern.fullmatch(p.name.upper())):
        merged.update(parse_edprb2(path.read_bytes()))
    if not merged:
        raise ResultError("FILE_MISSING", f"no {prefix}??.TXT in {directory}")
    return merged


def parse_cbprb(text: str) -> dict[tuple[str, int, int], str]:
    """CBPRB.TXT -> {(prefix, opcode, set): line}."""
    lines = {}
    for line in text.splitlines():
        if not line.strip():
            continue
        match = _CBPRB.match(line)
        if not match:
            raise ResultError("CBPRB_FORMAT", f"bad line: {line[:40]}")
        lines[(match.group(1), int(match.group(2), 16), int(match.group(3)))] = line
    return lines


# ---- DAADUMP

EDPRB3_RECORD = 128
_EDPRB3 = re.compile(r"^X([0-9A-F]{2}) ([0-9A-F]{4} ){8}[0-9A-F]{2} -> "
                     r"([0-9A-F]{4}) ([0-9A-F]{2})$")


def parse_edprb3(data: bytes) -> dict[int, str]:
    """EDPRB3.TXT -> {case index: line}."""
    records = {}
    usable = len(data) - len(data) % EDPRB3_RECORD
    for offset in range(0, usable, EDPRB3_RECORD):
        record = data[offset : offset + EDPRB3_RECORD]
        if record.strip(b"\x1a") == b"":
            break
        line = record.decode("ascii").rstrip("\r\n ")
        match = _EDPRB3.match(line)
        if not match:
            raise ResultError("EDPRB3_FORMAT", f"bad record at {offset}: {line[:40]}")
        records[int(match.group(1), 16)] = line
    return records


EDPRB4_RECORD = 16
_EDPRB4 = re.compile(r"^Y([0-9A-F]{2}) ([0-9A-F]{4}) ([0-9A-F]{2})$")


def parse_edprb4(data: bytes) -> dict[int, str]:
    """EDPRB4.TXT -> {C: line}."""
    records = {}
    usable = len(data) - len(data) % EDPRB4_RECORD
    for offset in range(0, usable, EDPRB4_RECORD):
        record = data[offset : offset + EDPRB4_RECORD]
        if record.strip(b"\x1a") == b"":
            break
        line = record.decode("ascii").rstrip("\r\n ")
        match = _EDPRB4.match(line)
        if not match:
            raise ResultError("EDPRB4_FORMAT", f"bad record at {offset}: {line[:40]}")
        records[int(match.group(1), 16)] = line
    return records


ALTPRB_RECORD = 128
_ALTPRB = re.compile(r"^X([0-9A-F]) ([IO])( (AF|BC|DE|HL|IX|IY|SP|AF'|BC'|DE'|HL')=[0-9A-F]{4}){11}$")


def parse_altprb(data: bytes) -> dict[tuple[int, str], str]:
    """ALTPRB.TXT -> {(case, 'I'|'O'): line}."""
    records = {}
    usable = len(data) - len(data) % ALTPRB_RECORD
    for offset in range(0, usable, ALTPRB_RECORD):
        record = data[offset : offset + ALTPRB_RECORD]
        if record.strip(b"\x1a") == b"":
            break
        line = record.decode("ascii").rstrip("\r\n ")
        match = _ALTPRB.match(line)
        if not match:
            raise ResultError("ALTPRB_FORMAT", f"bad record at {offset}: {line[:40]}")
        records[(int(match.group(1), 16), match.group(2))] = line
    return records


_INPRB = re.compile(r"^(P[0-9A-F]{2}( [0-9A-F]{2}){4}|C[0-9A-F]{4}( [0-9A-F]{2}){2})$")


def parse_inprb(text: str) -> dict[str, str]:
    """INPRB.TXT -> {port label: line}."""
    lines = {}
    for line in text.replace("\r\n", "\n").split("\n"):
        line = line.rstrip("\x1a")
        if not line or line.startswith("#"):
            continue
        if not _INPRB.match(line):
            raise ResultError("INPRB_FORMAT", f"bad line: {line[:40]}")
        lines[line.split()[0]] = line
    return lines


def dump_records(data: bytes) -> list[tuple[int, int, int, int]]:
    """Return (fi, a, A_out, F_out) per record."""
    if len(data) != DUMP_SIZE:
        raise ResultError("DUMP_SIZE", f"dump is {len(data)} bytes, expected {DUMP_SIZE}")
    return [(index >> 8, index & 0xFF, data[2 * index], data[2 * index + 1])
            for index in range(DUMP_SIZE // 2)]


def f_in(fi: int) -> int:
    value = 0
    for bit, mask in enumerate(reversed(FI_TO_F)):
        if fi >> bit & 1:
            value |= mask
    return value


def parse_daadump_txt(text: str) -> dict[str, tuple[int, str]]:
    result = {}
    for line in text.splitlines():
        match = re.match(r"^(\w+)\.BIN (\d+) ([0-9A-F]{8})$", line)
        if match:
            result[match.group(1)] = (int(match.group(2)), match.group(3))
    return result


def crc32_hex(data: bytes) -> str:
    return f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"

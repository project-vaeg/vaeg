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
"""Verify the bidirectional G41/M44 CPU state compatibility matrix."""

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path


HEADER_SIZE = 48
SECTION_HEADER_SIZE = 16
CPU286_SIZE = 112
UPD9002_SIZE = 16
SCENARIOS = (
    ("reset", "m44-reset.state"),
    ("executed-3", "m44-executed-3.state"),
    ("cpu-shut-request", "m44-cpu-shut-request.state"),
)


class MatrixError(Exception):
    pass


def sha256(payload):
    return hashlib.sha256(payload).hexdigest()


def parse_sections(path):
    data = path.read_bytes()
    if len(data) < HEADER_SIZE:
        raise MatrixError("{}: truncated container header".format(path))
    sections = {}
    position = HEADER_SIZE
    while position < len(data):
        if len(data) - position < SECTION_HEADER_SIZE:
            raise MatrixError("{}: truncated section header".format(path))
        raw_tag = data[position:position + 10]
        try:
            tag = raw_tag.rstrip(b"\0").decode("ascii", "strict")
        except UnicodeDecodeError as error:
            raise MatrixError("{}: non-ASCII section tag".format(path)) from error
        version, size = struct.unpack_from("<HI", data, position + 10)
        if tag in sections:
            raise MatrixError("{}: duplicate section {}".format(path, tag))
        start = position + SECTION_HEADER_SIZE
        end = start + size
        padded_end = start + ((size + 15) & ~15)
        if (end > len(data)) or (padded_end > len(data)):
            raise MatrixError("{}: truncated section {}".format(path, tag))
        if any(data[end:padded_end]):
            raise MatrixError("{}: nonzero padding for {}".format(path, tag))
        sections[tag] = (version, data[start:end])
        position = padded_end
    if position != len(data):
        raise MatrixError("{}: trailing container bytes".format(path))
    for tag, expected_size in (("CPU286", CPU286_SIZE),
                               ("UPD9002", UPD9002_SIZE)):
        if tag not in sections:
            raise MatrixError("{}: missing section {}".format(path, tag))
        version, payload = sections[tag]
        if version != 0:
            raise MatrixError("{}: {} version is not zero".format(path, tag))
        if len(payload) != expected_size:
            raise MatrixError("{}: {} size is not {}".format(
                path, tag, expected_size))
    return {tag: sections[tag][1] for tag in ("CPU286", "UPD9002")}


def parse_fixtures(path):
    fixtures = {}
    for line_number, line in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        fields = line.split(",")
        scenario = fields[0]
        values = {}
        for field in fields[1:]:
            if "=" not in field:
                raise MatrixError("{}:{}: malformed fixture field".format(
                    path, line_number))
            key, value = field.split("=", 1)
            values[key] = value
        try:
            cpu_size = int(values["cpu286_size"])
            registers_size = int(values["upd9002_size"])
            cpu = bytes.fromhex(values["cpu286"])
            registers = bytes.fromhex(values["upd9002"])
        except (KeyError, ValueError) as error:
            raise MatrixError("{}:{}: invalid fixture payload".format(
                path, line_number)) from error
        if (cpu_size != CPU286_SIZE) or (len(cpu) != CPU286_SIZE):
            raise MatrixError("{}:{}: invalid CPU286 fixture size".format(
                path, line_number))
        if (registers_size != UPD9002_SIZE) or (len(registers) != UPD9002_SIZE):
            raise MatrixError("{}:{}: invalid UPD9002 fixture size".format(
                path, line_number))
        if scenario in fixtures:
            raise MatrixError("{}: duplicate fixture {}".format(path, scenario))
        fixtures[scenario] = {"CPU286": cpu, "UPD9002": registers}
    expected = {scenario for scenario, _ in SCENARIOS}
    if set(fixtures) != expected:
        raise MatrixError("{}: fixture scenarios differ".format(path))
    return fixtures


def load_directory(directory):
    return {
        scenario: parse_sections(directory / filename)
        for scenario, filename in SCENARIOS
    }


def require_equal(left, right, description):
    for scenario, _ in SCENARIOS:
        for tag in ("CPU286", "UPD9002"):
            if left[scenario][tag] != right[scenario][tag]:
                raise MatrixError("{}: {} {} differs".format(
                    description, scenario, tag))


def verify(args):
    fixtures = parse_fixtures(args.fixtures)
    g41_generated = load_directory(args.g41_generated)
    m44_generated = load_directory(args.m44_generated)
    g41_to_m44 = load_directory(args.g41_to_m44)
    m44_to_g41 = load_directory(args.m44_to_g41)

    require_equal(g41_generated, fixtures, "G41 generation vs M42 fixture")
    require_equal(m44_generated, fixtures, "M44 generation vs M42 fixture")
    require_equal(g41_generated, m44_generated, "G41 vs M44 generation")
    require_equal(g41_generated, g41_to_m44, "G41 to M44 round trip")
    require_equal(m44_generated, m44_to_g41, "M44 to G41 round trip")

    rows = []
    for scenario, _ in SCENARIOS:
        cpu = fixtures[scenario]["CPU286"]
        registers = fixtures[scenario]["UPD9002"]
        rows.append({
            "scenario": scenario,
            "cpu286_sha256": sha256(cpu),
            "cpu286_size": len(cpu),
            "cpu_type": cpu[96],
            "upd9002_sha256": sha256(registers),
            "upd9002_size": len(registers),
        })
    result = {
        "fixture_sha256": sha256(args.fixtures.read_bytes()),
        "g41_sha": args.g41_sha,
        "m44_sha": args.m44_sha,
        "matrix": {
            "g41_generated_matches_fixture": True,
            "m44_generated_matches_fixture": True,
            "g41_to_m44_roundtrip": True,
            "m44_to_g41_roundtrip": True,
        },
        "scenarios": rows,
    }
    print(json.dumps(result, indent=2, sort_keys=True))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixtures", required=True, type=Path)
    parser.add_argument("--g41-generated", required=True, type=Path)
    parser.add_argument("--m44-generated", required=True, type=Path)
    parser.add_argument("--g41-to-m44", required=True, type=Path)
    parser.add_argument("--m44-to-g41", required=True, type=Path)
    parser.add_argument("--g41-sha", required=True)
    parser.add_argument("--m44-sha", required=True)
    args = parser.parse_args()
    try:
        verify(args)
    except (MatrixError, OSError) as error:
        print("upd9002-state-matrix: {}".format(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

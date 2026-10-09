<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98h - Freeze the atlas format and fail-closed inspector

Status: **G98h machine gate passed on 2026-08-31; 30-descriptor contract revalidated by M98j; `HOST_ATLAS_FORMAT_PASS`**

Branch: `topic/m98h-zundamon-atlas-format`

Commit prefix: `M98h:`

Gate type: **machine-verifiable public format fixture**

## Goal

Freeze `ZUNDORB.BIN` version 1 before any guest loader or production bank
packer exists, and add an independent fail-closed inspector that validates
the complete container without trusting the writer.

## Header contract

All integers are unsigned little-endian. The header is exactly 64 bytes:

| Offset | Size | Field | Version-1 value or meaning |
|---:|---:|---|---|
| `00h` | 8 | magic | `ZUNDORB` plus NUL |
| `08h` | 2 | version | 1 |
| `0ah` | 2 | header size | 64 |
| `0ch` | 4 | flags | zero |
| `10h` | 2 | pose count | exactly 1 |
| `12h` | 2 | scale count | exactly 30 |
| `14h` | 2 | descriptor size | 32 |
| `16h` | 2 | reserved | zero |
| `18h` | 4 | bank size | `00020000h` |
| `1ch` | 2 | required bank count | 1 through 30 |
| `1eh` | 2 | first bank value | exactly 1 |
| `20h` | 4 | descriptor offset | 64 |
| `24h` | 4 | descriptor bytes | 960 |
| `28h` | 4 | payload offset | 1024 |
| `2ch` | 4 | payload bytes | complete payload region |
| `30h` | 4 | file size | exact complete file size |
| `34h` | 4 | payload CRC32 | bytes from payload offset to EOF |
| `38h` | 4 | file CRC32 | complete file with this field zeroed |
| `3ch` | 4 | reserved | zero |

Guest selector value zero remains the ordinary-memory mapping. A descriptor's
guest selector is `first_bank_value + logical_bank_slot`; direct hardware
selector values are not stored per descriptor.

## Descriptor contract

Exactly 30 descriptors appear in increasing scale order. Each is 32 bytes:

| Offset | Size | Field |
|---:|---:|---|
| `00h` | 2 | width |
| `02h` | 2 | height |
| `04h` | 2 | four-byte-aligned pitch |
| `06h` | 2 | scaled anchor X |
| `08h` | 2 | scaled anchor Y |
| `0ah` | 2 | logical bank slot |
| `0ch` | 2 | flags, zero in version 1 |
| `0eh` | 2 | reserved, zero |
| `10h` | 4 | offset within the 128-KiB bank |
| `14h` | 4 | absolute file payload offset |
| `18h` | 4 | payload bytes, exactly pitch times height |
| `1ch` | 4 | frame CRC32 including row padding |

Frame and bank offsets are 16-byte aligned. A frame never crosses a bank.
File payloads are stored in descriptor order with only minimal zero alignment
padding. Row padding and inter-frame file padding are zero. Version 1 requires
canonical M98g dimensions, anchor projection, contiguous logical bank use,
nondecreasing bank slots, and nonoverlapping ranges within each bank.

## Required changes

- Add a deterministic public writer that exercises the format without owning
  M98i production packing. Assign each public scale to its own logical bank.
- Add an independent standard-library inspector with separately callable
  header, descriptor, canonical-geometry, layout, padding, frame-CRC,
  payload-CRC, and file-CRC validation layers.
- Bound files to the maximum possible 30 one-bank frames and reject symlinks,
  non-regular files, malformed or extra data, and every noncanonical field.
- Keep writer and inspector CLI output free of paths, filenames, dimensions,
  anchors, bank assignments, sizes, CRCs, payload values, and hashes.
- Add focused negative tests that begin with a passing fixture, make one
  controlled mutation, and assert the exact intended stable error code.

## Private boundary

M98h does not build, inspect, name, hash, or report any private atlas. The
public fixture contains only the M98b abstract marker. Private bank packing
and private `ZUNDORB.BIN` generation remain later local work.

## Out of scope

- Enforcing the production single-bank rule or implementing production packing.
- Changing M98g pixels, scaling, or anchors.
- Guest loading, BMS probing, SGP transfer, VAEG, or physical-machine work.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98h-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_atlas.py

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The public writer is byte-reproducible, the independent inspector accepts it,
all fixed header and descriptor values agree, and focused structure, geometry,
anchor, layout, padding, bank-boundary, overlap, CRC, file-type, overwrite,
and CLI privacy cases reach their intended results. Repository and privacy
checks pass. Report `HOST_ATLAS_FORMAT_PASS`; make no private-atlas claim.
This is a machine gate. Stop at G98h; M98i remains unassigned.

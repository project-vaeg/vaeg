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

# M98h - Zundamon atlas-format report

Evaluated predecessor: `9babd808d2f3a8cea898e9caadbfa2301d2e8726`

Status: **G98h machine gate passed on 2026-08-31; 30-descriptor contract revalidated by M98j; `HOST_ATLAS_FORMAT_PASS`**

## Result

M98h freezes the 64-byte little-endian `ZUNDORB.BIN` version-1 header and 30
fixed 32-byte descriptors. The format records one pose, 30 scales, 128-KiB
BMS bank size, required bank count, explicit first selector value, canonical
payload bounds, complete file size, and frame, payload, and file CRC32 values.
Logical bank slots are converted to guest selectors through the header;
selector zero is not consumed by the atlas.

The independent inspector validates fixed fields, canonical M98g geometry and
anchors, four-byte pitch, 16-byte file and bank alignment, complete-frame bank
containment, ordered file layout, nondecreasing and contiguous logical banks,
nonoverlapping bank ranges, zero row/file padding, exact lengths, and all CRC
layers. It rejects symlinks and non-regular or oversized input.

The public format fixture deliberately assigns one scale to each logical bank.
This proves the binary contract without implementing the M98i production
packer or making a private-atlas claim.

## Machine evidence

The M98h standard-library suite ran eight test methods and passed:

```text
OK
M98H_TEST_PASS
```

Coverage includes byte reproducibility, exact header and descriptor values,
isolated header and descriptor mutations, canonical scale and anchor failures,
file and bank layout, bank order/usage/overlap, file and row padding, frame,
payload, and file CRC failures, overwrite refusal, file type, and path-redacted
CLI success and failure.

M98b-M98g regression tests, Python compilation, public writer/inspector
workflow, repository encoding, EOL, case, diff, JSON, and M98 privacy checks
passed. No private atlas or generated binary is tracked.

## Boundary

G98h is a host format gate only. M98h makes no production-packing, private
atlas, guest, BMS, SGP, VAEG, or physical-machine claim. M98i remains
unassigned.

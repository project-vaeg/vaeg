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

# M98i - Pack complete frames into one 128 KiB BMS bank

Status: **G98i machine gate passed on 2026-08-31; single-bank contract revalidated by M98j; `HOST_BMS_PACKING_PASS`**

Branch: `topic/m98i-zundamon-bms-packing`

Commit prefix: `M98i:`

Gate type: **machine-verifiable public packing fixture**

## Goal

Pack the 30 ordered M98g scale frames into exactly one 128-KiB logical BMS
bank, then encode the result through the M98h version-1 atlas format without
splitting a frame.

## Packing contract

Process frames in increasing scale order. For each frame:

1. reject a payload larger than one 128-KiB bank;
2. round the current bank cursor up to a 16-byte boundary;
3. place the complete frame there when it fits;
4. reject the complete atlas when the next frame would require a second bank.

No look-ahead, reordering, backfilling, frame splitting, or per-frame bank
selector is permitted. Every production descriptor uses logical slot zero;
the M98h header derives the one guest selector from its fixed first-bank
value. Required bank count is exactly one.

The serialized atlas payload retains M98h's canonical compact file layout:
frames appear in descriptor order with only minimal 16-byte zero file
alignment. The unused BMS bank tail is logical placement padding and is not
serialized into the atlas file.

## Required changes

- Add a standard-library packer that accepts an in-memory validated M98g
  scale set and produces a complete M98h version-1 atlas.
- Validate the scale count, levels, dimensions, pitches, anchors, frame
  payloads, source stream layout, and zero alignment padding before packing.
- Record logical bank slot and bank offset for every frame, and independently
  pass the complete output through the M98h format inspector.
- Add an M98i packing validator that rejects a format-valid but nonminimal or
  multi-bank assignment without changing the M98h format inspector's
  contract.
- Produce a deterministic public output directory containing a packed public
  atlas and JSON metrics report. Refuse an existing output directory.
- Report useful pixel bytes, row padding, bank-frame alignment, compact file
  alignment, payload/file size, required bank count, and payload/occupied
  bytes in the generated report only.
- Keep CLI success and failure output free of paths, filenames, dimensions,
  anchors, bank assignments, sizes, CRCs, payload values, and hashes.
- Add independent-oracle and focused fail-closed tests for exact fit,
  alignment fit, one-byte overflow, multi-bank rejection, an oversized frame,
  nonminimal/corrupted plans, deterministic output, and CLI privacy.

## Private boundary

M98i uses only the public synthetic fixture and in-memory neutral test data.
It does not read or produce a maintainer-supplied atlas. The generated public
atlas and report are temporary test artifacts and are not tracked.

## Out of scope

- Connecting a maintainer-supplied manifest to the final packer.
- Changing the first guest bank selector or BMS
  bank-zero behavior.
- Guest loading, BMS probing, SGP transfer, VAEG, disk images, screenshots, or
  physical-machine work.

## Machine checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98i-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_packing.py

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

The public fixture packs reproducibly and passes both the M98h format
inspector and the M98i single-bank packing validator. Independent plan oracles
agree for exact-fit, alignment, one-byte-overflow, and multi-bank cases. Every
frame is 16-byte aligned, complete, ordered, and within the single bank;
required bank count is exactly one. Oversized frames, multi-bank atlases, and
isolated packing corruptions reach exact stable codes. Metrics reconcile,
CLI diagnostics remain path-redacted, and repository/privacy checks pass.
Report `HOST_BMS_PACKING_PASS`. This is a machine gate. Stop at G98i; M98j
remains unassigned.

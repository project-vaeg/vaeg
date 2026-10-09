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

# M98l - Stream one BMS atlas and prove a direct G1 transfer

Status: **G98l human gate passed on 2026-08-31; M98l closed**

Branch: `topic/m98l-zundamon-bms-direct-g1`

Starting commit: `2a6c3944bab1fb691261fa2f0950dc4a2faeab8c`

Evaluated candidate: `228f31eb192c2722862691067c46c4db9e4aeb95`

Result: [`../reports/m98l_zundamon_bms_direct_g1.md`](../reports/m98l_zundamon_bms_direct_g1.md)

Commit prefix: `M98l:`

Gate type: **automated VAEG evidence plus maintainer human gate**

## Goal

Extend the accepted M98k bootable 320x200 8-bpp guest into one bounded proof:

```text
public synthetic ZUNDORB.BIN on the guest filesystem
    -> one conventional-memory staging buffer of at most 4096 bytes
    -> selector 1 at the default 01D0h BMS port
    -> direct SGP source inside the 80000h-9ffffh BMS window
    -> one transparent BITBLT to Graphic 1
```

M98l combines the former M98l, M98m, and M98n scopes. M98m and M98n are
reserved and are not executed separately. M98o and later retain their existing
numbers.

## Fixed live BMS contract

- The selector is one 8-bit read/write register at configured port `01D0h`.
- Selector zero exposes ordinary main RAM at `80000h-9ffffh`.
- Selectors 1 through N expose N independent 128-KiB BMS banks.
- The clean configuration provides 128 banks (16 MiB).
- An invalid nonzero selector is retained by the selector register but its
  memory window is open bus; it does not wrap or alias a valid bank.
- Only the configured port is bound. `00ech` is a separately selected
  compatibility configuration, not a simultaneous alias.
- CPU byte/word access and SGP word access observe the same selected window.
- Reset selects zero. The guest must write zero on success and every failure
  path, and it must never change the selector while SGP is busy.

## G98l-A - Reversible mapping probe

Use the default 128-bank configuration and perform only bounded probes. Save
and guard ordinary memory on both sides of the aperture boundary. Save the
small BMS ranges touched in selectors 1, 2, and 128; prove bank-1/bank-2
independence and selector-128 validity; then select 129 and prove open-bus
behavior without changing bank 1. Restore the saved BMS bytes, select zero,
verify the ordinary-memory guards, and retain deterministic checkpoint
registers for the host oracle.

## G98l-B - Bounded atlas streaming

Generate the repository-owned public synthetic atlas through the accepted M98j
pipeline. Install it on the local boot disk as `ZUNDORB.BIN`; generated media
and the atlas remain ignored artifacts.

The guest shall:

- open the file through DOS calls while selector zero is active;
- read and validate the fixed 1024-byte header/descriptor region;
- require version 1, one pose, 30 scales, one 128-KiB bank, first selector 1,
  canonical bounds, and a payload region no larger than 128 KiB;
- use one fixed 4096-byte staging buffer and handle short reads explicitly;
- copy each payload chunk into selector 1 at the descriptor-compatible running
  bank offset, restoring selector zero before every DOS call;
- validate incremental file and payload CRC32 values;
- read the BMS range directly and validate the resident payload CRC32;
- reject trailing input; and
- poison the complete staging buffer before SGP submission.

No complete atlas may be embedded in the COM or retained in conventional
memory. The selected test cell is descriptor 30 and its geometry, pitch,
offset, and expected pixels come from the validated public atlas.

## G98l-C - Direct BMS-window SGP BITBLT

Reuse M98k's video setup, nonzero G0 checkerboard, transparent G1 semantics,
320-byte pitch, G1 page A, bounded SGP wait, VBLANK checkpoint, and exact GVRAM
capture. Clear G1 and submit one command list containing exactly one BITBLT
`0105h`. Its source is `080000h + selected bank offset` while selector 1 is
active; its centered destination is on G1. Do not copy atlas pixels to G1 with
the CPU or host.

After completion, verify that the staging buffer remains poisoned and that the
BMS payload CRC is unchanged. Select zero, verify and restore ordinary-memory
guards, publish page A, and leave the image static. A generic SGP trace must
show exactly one source descriptor in the BMS aperture and the expected G1
destination. Two settled captures must be identical and nonblack.

## Host oracle

Extend the standard-library M98 oracle so it independently parses and
validates the public atlas, the probe/load/final register checkpoints, event
chronology, SGP descriptor trace, complete indexed GVRAM, and two composed
frames. It must fail closed for wrong bank/capacity, alias behavior, load
length/CRC, staging size/poison state, source address/stride/cell offset,
destination/layer, transparency, BITBLT count, mapping restoration, an extra
atlas-sized conventional allocation, frame instability, and black output.

Focused negative tests start from a passing fixture, apply one mutation, and
assert one stable error code.

## Non-goals

- Maintainer-supplied artwork, ROM extraction, or proprietary inputs.
- A second BMS bank, EMS, XMS, caching, runtime scaling, or atlas traversal.
- Repeated drawing, page exchange, animation, controls, multiple objects, or
  performance measurement.
- Physical-machine timing or conformance claims.

## Required checks

```sh
PYTHONPYCACHEPREFIX=/tmp/vaeg-m98l-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py

NASM=/opt/local/bin/nasm \
  sh demos/zundamon-orbit/256/build.sh \
  build/generated/zundamon-orbit/m98l/ZUNDORB.COM \
  build/generated/zundamon-orbit/m98l/ZUNDORB.LST

sh demos/zundamon-orbit/build-local-d88.sh \
  <local-bootable-2hd-template> \
  build/generated/zundamon-orbit/m98l/ZUNDORB.BIN \
  build/generated/zundamon-orbit/m98l/zundamon-orbit-m98l.d88

VAEG_ZUNDAMON_MODEL=va2 sh demos/zundamon-orbit/run-vaeg.sh \
  <local-bootable-2hd-template> \
  build/macos-macports/sdl2/vaeg \
  <local-rom-directory> \
  build/generated/zundamon-orbit/m98l-run

python3 demos/zundamon-orbit/tools/verify_zundamon_orbit_guest.py \
  --atlas build/generated/zundamon-orbit/m98l-run/ZUNDORB.BIN \
  --trace build/generated/zundamon-orbit/m98l-run/sgp-trace.log \
  build/generated/zundamon-orbit/m98l-run

build/macos-macports/sdl2/vaeg --selftest
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

## Acceptance

`G98l PASS` requires `G98l-A`, `G98l-B`, and `G98l-C` to pass in the same
guest artifact and validated run family. The final report records the exact
BMS contract, atlas provenance and geometry, streaming totals, CRCs, staging
bound and poison proof, one direct BMS-source BITBLT, G1 pixel oracle, artifact
hashes, GUI/headless limitation, final worktree state, and the explicit
deferral of scaling, animation, multiple objects, real artwork, performance,
and physical-machine validation. Stop at G98l; do not start M98o.

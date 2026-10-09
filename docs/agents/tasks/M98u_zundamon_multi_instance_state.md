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

# M98u - Freeze bounded multi-instance state and depth order

Status: **G98u machine gate passed; M98u closed on 2026-09-01**

Branch: `topic/m98u-multi-instance-state`

Accepted M98t implementation:
`9440798d13bd00229b03163f98f9fee7c4caac68`

Accepted M98t report head:
`06d43348a35efb2b93db8272fba961631be146eb`

M98t human-gate audit head:
`ae5bfc9b3fa6284e97390b6b40fb04eea9a0a700`

Commit prefix: `M98u:`

Gate type: **machine-verifiable**

## Goal

Freeze the deterministic bounded state needed for 1 through 16 future public
instances without enabling multi-instance rendering. For active count `n`,
global phase `g`, and instance ID `i`, assign:

```text
phase_offset = floor(64*i/n)
phase_id = (g + phase_offset) & 63
```

Derive depth, scale, descriptor, anchor, half-open destination rectangle, BMS
selector/offset/source, payload length, and frame identity through the accepted
M98t table and atlas. Keep records in ascending instance-ID order and sort a
separate bounded byte-index array by `(signed depth_rank, instance_id)` from
far to near.

## Fixed representation

The reference capacity is exactly 16 records and 16 one-byte indices. The
guest-compatible record is exactly 50 bytes: eight byte fields, thirteen
word fields, and four double-word source/identity fields. Signed depth,
offset, anchor-target, and destination values retain explicit signed fields.
The active prefix is exactly `n`; there is no heap allocation, pointer in the
serialized state, recursive sort, or copied atlas payload.

The checked-in compact include freezes field offsets and capacities only. The
complete 16-count by 64-phase matrix remains generated and ignored. Future
guest code must generate records from `n`, `g`, the accepted 64 orbit entries,
and the 30 atlas descriptors rather than embedding 1,024 states.

## Automated evidence

Test all 1,024 `(n,g)` combinations and exactly 8,704 instance records. Verify
unique phases, balanced circular gaps, rotation covariance, descriptor/source
identity, screen/G1 bounds, HUD exclusion, one shared bank, permutation,
signed depth order, explicit equal-depth ID ties, and count-one equality with
all 64 M98t states. Generate canonical UTF-8 JSON twice and independently
validate it. Invalid counts, phases, arithmetic, offsets, phase tables,
descriptors, bounds, sources, capacities, orders, serialization, and contract
layouts must fail with stable M98u codes.

Rebuild the accepted release guest twice. It must remain exactly 32,656 bytes
with SHA-256 `b6e1bbc2a600f22ca583e256c82cccab3c1523530a0a2a7836439d4cb74d87ec`.
The 159 accepted M98t host tests, VAEG selftest, and accepted generated VA2
matrix evidence remain applicable because no guest, emulator, atlas, or
runtime input changes.

## Non-goals

M98u does not draw a second object, change clearing, add rectangle lists or
unions, add `/N` or UP/DOWN controls, update `ZUNDAMON: 1`, duplicate atlas
payloads, integrate private imagery, measure multi-instance performance, or
change any guest-visible behavior. M98v owns the full-clear multi-instance
renderer; M98w owns dirty-row interval unions; M98x owns count controls and
HUD/load telemetry.

## Gate result

The exhaustive machine gate generated all 1,024 count/global-phase states and
8,704 instance records with zero uniqueness, gap, descriptor, bounds, HUD,
source, permutation, ordering, tie, count-one, determinism, or privacy
failures. The independent validator accepted the canonical golden, and two
generations had SHA-256
`6ed8e4e4b70ed62547d0feca9847999f730e25b6e4d19e0a16c670021c5a3e52`.

All 187 demo host tests and VAEG selftest passed. Two release guest rebuilds
remained exactly 32,656 bytes and byte-identical to M98t with SHA-256
`b6e1bbc2a600f22ca583e256c82cccab3c1523530a0a2a7836439d4cb74d87ec`.
The accepted M98t A/B full/dirty, cadence/HUD, pause, and missed-slot oracle
records were revalidated without changing guest or emulator inputs.

`REAL_HW_PENDING`

`G98u PASS`

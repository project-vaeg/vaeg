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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M60c uPD9002 FPO2 and main-dispatch authority audit

M60c corrects the prospective historical labels for the G43 OUTS transition,
audits every selected SST record for primary opcodes 66 and 67, and binds a
complete monitor-disassembler dispatch/FPU audit to content-addressed
evidence. The formal result is `target_absence_proven` within this monitor
target-authority model. This is not a claim about complete uPD9002 silicon
semantics.

M60c is complete and pushed. G60c is an unapproved candidate pending human
review. No production CPU semantics or top-level classifications were
changed. M60d and later milestones have not been started.

A commit cannot contain its own SHA. The evidence commit and final candidate
are therefore the commit containing this report; the exact SHA is supplied in
the maintainer handoff and is independently available from
`origin/topic/m60c-upd9002-fpo2-audit`. The audit implementation actually
executed is fixed below as `analysis_evaluated_sha`.

## Identity and preparation

- Approved predecessor gate: `G60b`
- Exact approved G60b SHA and M60c base:
  `4e5d74d0d9f675df2342353b8bfdbb2e5cded768`
- G60b implementation/evaluated SHA:
  `23c5de2a7d28b35dd184201dee8d101607178510`
- Approved G60b CI:
  [build 30144447279](https://github.com/nakatamaho/vaeg/actions/runs/30144447279)
- Approved G60b target-policy ID:
  `upd9002-g60b-eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93`
- Approved G60b target-policy SHA-256:
  `eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93`
- Approved G60b authority-manifest SHA-256:
  `f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac`
- Initial primary-worktree branch: `main`
- Initial primary-worktree SHA:
  `39b982801ac85a6e01219d4404e79b9f06534b0f`
- Initial primary-worktree state: dirty with unrelated maintainer work
- Dedicated worktree: `/tmp/vaeg-m60c.Ff5jms/worktree`
- Dedicated worktree starting SHA:
  `4e5d74d0d9f675df2342353b8bfdbb2e5cded768`
- M60c branch: `topic/m60c-upd9002-fpo2-audit`
- Prospective-documentation erratum commit:
  `744e79467e4003c812bbb0d36b42f28e8c35a2bb`
- Audit implementation commit and `analysis_evaluated_sha`:
  `a9dd78bded5c1072f0285f00cf7759654da8b7d8`
- Evidence commit/final candidate: the commit containing this report; exact
  SHA in the maintainer handoff and remote branch ref

The primary worktree was not cleaned, reset, stashed, staged, or modified.
The dedicated worktree was created directly from the fixed approved
predecessor. The maintainer authorization, approved G60b report, local
predecessor object, and
`origin/topic/m60b-upd9002-rom-authority` all resolve to the same SHA. No
conflicting approved G60b SHA was found.

The mandatory preparation commands exited zero:

```text
git status --short
git branch --show-current
git rev-parse HEAD
git rev-parse 4e5d74d0d9f675df2342353b8bfdbb2e5cded768
git show --stat --oneline 4e5d74d0d9f675df2342353b8bfdbb2e5cded768
git rev-parse origin/topic/m60b-upd9002-rom-authority
python3 tools/qa/milestone_ids.py --selftest --audit --discover
```

Milestone discovery passed all 48 strict identifier tests and confirmed that
ROADMAP, task discovery, and
`docs/agents/tasks/M60c_upd9002_fpo2_main_dispatch_audit.md` agree on the
M60c identifier, branch, and scope.

## Environment, corpus, contracts, and policy

| Component | Recorded value |
|---|---|
| Host | `Linux-6.18.33.2-microsoft-standard-WSL2-x86_64-with-glibc2.43` |
| Distribution | Ubuntu 26.04 under WSL2 |
| Git | 2.53.0 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |
| GCC | 15.2.0 |
| Python | 3.14.4 |
| gzip command | 1.14 |
| Python gzip module | `/usr/lib/python3.14/gzip.py` |
| zlib compile/runtime | 1.3.1 / 1.3.1 |

The verified corpus was available for every required SST run:

```text
/tmp/vaeg-m57-ssts-cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
```

Dataset identity:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

| Contract | ID | SHA-256 |
|---|---|---|
| architectural | `upd9002-v20-architectural-v1` | `aa7ecb1fa7c30fc5d7e7fc742bb4e616595c3d10c7a35e561c09da419907d5d5` |
| fingerprint | `upd9002-v20-fingerprint-v1` | `47e6b4dcf8c2bba2a36f15953b9701fb306b8db7e0254c54e1fe878e2d33fb2e` |

The target-policy ID and SHA-256 before and after M60c are both the approved
G60b values above. No new target-policy epoch is required because M60c makes
no gap-kind, hardware-pending, top-level-classification, selected-set, or
applicable-set change.

Selected and applicable set identities remain:

| Scope | Selected SHA-256 | Applicable SHA-256 |
|---|---|---|
| CI | `d30dd9c864fbbaa74c661e1b829c66264f2184a8fbbb72b654b2baa825664ae6` | `5a00d3fa15d55b38630015e771163fe58aff544d5f13342beac01a8236a485a1` |
| full | `0aa3dbb24323223b3a9595a0bd7cfd5666596741157c14b60f6969318475f8f7` | `a29c0a52d0818c7797515d5fbcc680b1fecdf5b10b896e5e02afff498cf99d65` |

## Approved G60b reproduction

Before editing, a fresh tests-enabled build at the approved G60b SHA ran all
three profiles without profile skip and reproduced the complete approved
state:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash |
|---|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,179 | 8,121 | 0 | 0 |
| architectural full | 1,562,502 | 1,438,594 | 1,378,653 | 59,941 | 0 | 0 |
| fingerprint full | 1,562,502 | 1,438,594 | 1,276,215 | 162,379 | 0 | 0 |

Counts were not used as identity substitutes. Dataset, contracts, target
policy, selected/applicable sets, pass/failure sets, signature indexes,
mismatch classes, termination classes, classifications, taxonomy,
registries, scoreboards, failure shards, transitions, and protected evidence
matched G60b exactly. In particular:

```text
G60b target-policy ID:
upd9002-g60b-eb9695cbe7b06f6339a1c725983c5ea92918f81d35aea34fc79cc9aa0b09ed93

G60b authority manifest:
f14fa57e8aedb54c773e55c94d55572d3c99e00457c01c75df3507582c35f1ac

G60b full transition:
2396a03ec5b64033406f200458ef9d3d1e8bf805a551385c8bfbb07ed9493cdd

G60b artifact tree:
af1d979faa3d75019e3df6d419f6caf870c33dbb19b18a5c0d8010f33bd695c5
```

M58 ratchet selftests/static verification, M59 evidence selftests/static
verification, M60a artifact verification, and M60b authority/policy/static
verification all passed. Immutable G43/M43 and approved G58/G59/G60a/G60b
artifacts remained byte-identical.

## Prospective historical-label correction

The first M60c commit changes prospective documentation and its validator
only. It does not rewrite the completed M60b task/report or any historical
artifact. The exact corrected interpretation is:

| Population | Count | Hash-set SHA-256 |
|---|---:|---|
| G60a 6E retired failures | 0 | `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945` |
| G60a 6F retired failures | 641 | `03f8ea83c510e67e27cc60a9455322f0cd899eb88287835080d2f9e98a0fa1f2` |
| G43 unchanged-signature subset | 417 | `7240eff77e38a2ca67cf94d6cec13c4ddec1f2e122cf62cbb7318ee39c82be2e` |
| G43 changed-signature subset | 224 | `f70b2e4e614cc677a883bc8d9ceb349f7a9bff32f185b253d893e6aea904a814` |
| G43 fixture-pass population | 1,204 | `c8de1415733c5bad2ba85d667d56f5d04631d19379ce16f85e641792e7644322` |

Thus 417 and 224 are signature-transition subsets of the same 641-case G43
OUTS population, not 6E/6F opcode-form failure counts. All 1,204 G43 fixture
pass hashes remain retired V20 differential evidence. Exact content-addressed
G60a/G60b sets govern later accounting.

`upd9002_m60c_erratum.py selftest` passed one positive and seven fail-closed
tests. The static validator rejects the old opcode-count interpretation, any
wrong count or digest, and mutation of protected G43 or G60b evidence.

## Implementation and semantic boundary

The non-evidence commit chain is:

```text
744e79467e4003c812bbb0d36b42f28e8c35a2bb
M60c: correct historical OUTS population labels

a9dd78bded5c1072f0285f00cf7759654da8b7d8
M60c: add FPO2 target-authority audit
```

The evaluated commit adds only:

- prospective erratum wording and its fail-closed validator;
- deterministic ROM main/group/FPU dispatch extraction and validation;
- prefix-aware full-population 66/67 SST audit tooling;
- versioned authority schemas and canonical deterministic generation;
- positive/fail-closed tests and narrow CTest/hosted-CI integration.

The semantic diff is empty:

```text
git diff --exit-code \
  4e5d74d0d9f675df2342353b8bfdbb2e5cded768...a9dd78bded5c1072f0285f00cf7759654da8b7d8 \
  -- cpu/upd9002/
```

There are no changes to SST fixtures, comparison contracts, selected or
applicable sets, 6C-6F policy ownership, 0F28 ownership, G60b authority,
scoreboards or transitions, immutable G43/M43 evidence, or approved
G58/G59/G60a artifacts. Production decoder, dispatch, FPU, and opcode
handlers are untouched.

## Complete 66/67 SST audit

Selection decodes the primary opcode after all recognized prefixes. A
first-raw-byte-only selector is explicitly rejected. The complete
machine-readable population contains 10,000 full records and the exact 1,000
CI-record subset:

| Opcode | Full selected | CI selected | Executed | Arch pass/fail | Fingerprint pass/fail | Classification | Gap kind |
|---|---:|---:|---:|---:|---:|---|---|
| 66 | 5,000 | 500 | 0 | 0 / 0 | 0 / 0 | `upstream_nonblocking` | none |
| 67 | 5,000 | 500 | 0 | 0 / 0 | 0 / 0 | `upstream_nonblocking` | none |

Absence from a failure list was not interpreted as passing. These records are
selected but not applicable/executed under the approved policy. Upstream
metadata labels all 10,000 records architecture `v30`, status `fpu`, and
mnemonic root `fpo2`.

The support-map/dispatch tuple for each form is
`mode=v30op`, `target=v30_reserved`, `classification=known_target_gap`,
`subopcode=-`. Top-level ownership is nevertheless
`upstream_nonblocking` because the upstream `fpu` status takes precedence.
M60c preserves that ownership rather than inventing a gap kind.

| Population | Count | Record-hash SHA-256 | Upstream-hash SHA-256 |
|---|---:|---|---|
| 66 CI | 500 | `097473645186cac101e486b4aa4e4251c350cbfabb22c2de3e5cde88e26c5237` | `97eebdca07524d67df6060224d21f92b4497730ac78de97f7393cf28525c0b71` |
| 67 CI | 500 | `0f4f361b232e6b799acd8562855c1050005fbc9ceeda5a7f8eba63983d36d709` | `8deb20f385ae8c6afa61e44eee9684e9f103d7509cf3fc58cf52c8569eab1c02` |
| 66 full | 5,000 | `3363f95f044fb79587633d0958549e10ce6c92f9589cd58f934cee4d83d3e443` | `b152b91a7de513d6d04d52b7ceeca97fd4cbdfd91d1839635bcbac24f9a20c53` |
| 67 full | 5,000 | `2ffee1efb6ec206ddab14ed3ded2cb526009d9415b58fba65f4a8afdf40abb7f` | `95cbfde2144580fad8db102bd688cadae5354acfc973f1b33b9e33921a6493be` |

Combined record-hash digests are
`e118526ebc141af8ba63b993c0f1d5027eed5e3ae383a4b2c95742d368071988`
for CI and
`9619ad38620df14f4f5c1e4e34c5b811631dc92d0046d0597eb4bd3b9b06b58f`
for full.

Corpus structure is FPO2 plus ModR/M and any ModR/M-selected displacement.
Complete instruction lengths are 2 through 5 bytes. Forms are unprefixed or
carry one corpus-represented segment override; no form was silently omitted:

| Opcode | Unprefixed | 26 | 2E | 36 | 3E | Lengths 2/3/4/5 |
|---|---:|---:|---:|---:|---:|---|
| 66 | 2,482 | 664 | 610 | 654 | 590 | 1,181 / 1,756 / 1,334 / 729 |
| 67 | 2,532 | 606 | 618 | 608 | 636 | 1,210 / 1,848 / 1,265 / 677 |

The ModR/M `mod` counts for 66 are 1,246 / 1,245 / 1,274 / 1,235 and for 67
are 1,249 / 1,263 / 1,204 / 1,284 for `mod=0/1/2/3`. Every case row records
complete bytes, prefix sequence, upstream metadata, structural dispatch,
classification ownership, selection/execution status, and exact case hashes.

## ROM provenance and ordinary dispatch

M60c reuses the exact G60b-authorized out-of-tree ROM:

```text
local analysis path: /tmp/vaeg-m60b-authority/monitor.rom
role:                PC-88VA2 main varom00 monitor ROM
size:                524,288 bytes
SHA-256:             0460b58d4c5fa19cc8f9a2120bb7e65bbe7c78613552c3057a718444b76e8fee
CRC32:               98c9959a
SHA-1:               bcaea28c58816602ca1e8290f534360f1ca03fe8
mapping:             canonical address = ROM file offset
bank base:           0x60000
```

The complete copyrighted ROM remains out of tree. The maintainer-authorized
pack contains only hashes, minimal raw table/code extracts, deterministic
decoded records, and neutral analysis.

The ordinary primary table is reverified as `[0x66350, 0x664f4)`, 140
records, with parallel mnemonic range `[0x66515, 0x666fa)`. Its raw-record
digest is
`057d34124d0034e2fc89b58d796966c7d30056297e49e3f58fa0bf96299e33db`;
the mnemonic-range digest is
`18006f6526e018b562f0a7c3246d1d6445053d0b0d920d8cc034ea4be2962c00`.
Primary 66 and 67 are absent. That fact is recorded only as one input and is
not treated by itself as proof of target absence.

## Group dispatch and segment-override sanity check

The complete group table is `[0x668fd, 0x66921)`, exactly twelve three-byte
ordered `(mask, value, group)` candidates. The raw-slice digest is
`70cc706fbe9b01a3d623e15ed3392903566d9976a57ab849dd8d1e3b34fe395e`.
The complete records are:

```text
fe f6 00   ff ff 01   ff 8f 02   fe fe 03
fe f6 04   fc 80 05   fe 80 06   fc d0 07
fc d0 07   fe c0 07   fe c0 07   e7 26 08
```

Overlapping expansions are intentionally ordered candidates: the decoder
calls the record matcher before selecting a handler. The handler-pointer
range is `[0x662ff, 0x66311)` with digest
`2cd91cb1332640898d2fc76b24ea5dcc3b61e80db584cfda9d90e44dc3dee99e`.
No candidate expands to 66 or 67.

`e7 26` expands exactly to 26, 2E, 36, and 3E. Group 08 points to handler
`0x60bc`; code `[0x660bc,0x660dc)` has digest
`09d32bd8ebd7f0d9ba981eb9d1b2fc8c597079318d86aa23438e9fa76a7df45c`
and derives the segment selector from opcode bits 3-4 before emitting the
segment mnemonic and colon. This positively proves the segment-override
interpretation and validates the ordered mask expansion. It is not used as
independent evidence about 66/67.

## Reachable decoder trace

All reachable normal-disassembler alternatives are bounded:

| Range | Incoming condition | Result relevant to 66/67 |
|---|---|---|
| `[0x658f3,0x6592c)` | normal disassembler entry | fetches primary and calls primary dispatcher |
| `[0x6592c,0x6598d)` | primary byte | scans all 140 ordinary records, then falls through |
| `[0x6598d,0x659d3)` | ordinary-table miss | scans all 12 ordered group candidates, then falls through |
| `[0x659d3,0x65a3c)` | group-table miss | selects 0F table only for primary 0F; otherwise FPU path |
| `[0x65a3c,0x65b2c)` | non-0F miss | fetches following byte, scans four FPU tables, then requires D8-DF/register ModR/M |
| `[0x65b2c,0x65b79)` | FPU table search | performs 16-bit masked match and selects linked handler/mnemonic |

The complete normal-entry path digest is
`a4516c4eba558a2c14e9531d900466cdae7dfc3f60b79c2e27a99c1d8228f914`.
Path-specific raw-byte digests and neutral pseudocode are in
`decoder_paths_66_67.json`.

For 66 and 67, ordinary and group tables miss, the 0F branch is unreachable,
all four FPU tables miss, and the positive fallback range test requires a
primary opcode in D8-DF plus register ModR/M. Both encodings therefore reach
the monitor's unknown-instruction result. The alternate path consumes one
following byte while testing the FPU tables; it does not recognize 66/67 as a
prefix for a following D8-DF instruction, consume a 66/67-owned ModR/M, or
link either form to a mnemonic/handler.

## D8-DF FPU authority

The complete FPU region consists of four five-byte-record tables:

| Table | Record range | Records | Raw SHA-256 | Mnemonic range |
|---|---|---:|---|---|
| memory arithmetic | `[0x66b3c,0x66baa)` | 22 | `ae1189c97b15486e8a9ec4c47cd32212876ba60a7ee6f7f1d7bf50baa0ec54e7` | `[0x66cf4,0x66d59)` |
| memory load/store/environment | `[0x66baa,0x66beb)` | 13 | `6869ed9fb6ba48b138e4abae0f9522531b147acd01c29ec4f86f828ba0f18da8` | `[0x66d59,0x66d98)` |
| register arithmetic | `[0x66beb,0x66c5e)` | 23 | `ce92e0f91cd57858205f1e6343fbcc6792f65d9d40e48d3e8bfc38bdcace5a0f` | `[0x66d98,0x66e00)` |
| register constant/transcendental/control | `[0x66c5e,0x66cef)` | 29 | `dd88d5aceeaf656c9d8bfbc456bc072dec9c55296fc314252a732a08d6e3615e` | `[0x66e00,0x66e9a)` |

All 87 records decode to 2,069 exact opcode/ModR/M forms. Primary ownership
is exactly D8 through DF. No record or fallback path owns 66 or 67. Handler
links, mask expansion, table order, mnemonic links, and operand/ModR/M
relationships are content-addressed in the authority pack.

The monitor contains 68 distinct individual FPU mnemonics:

```text
F2XM1 FABS FADD FADDP FBLD FBSTP FCHS FCLEX FCOM FCOMP FCOMPP
FDECSTP FDISI FDIV FDIVP FDIVR FDIVRP FENI FFREE FIADD FICOM
FICOMP FIDIV FIDIVR FILD FIMUL FINCSTP FINIT FIST FISTP FISUB
FISUBR FLD FLD1 FLDCW FLDENV FLDL2E FLDL2T FLDLG2 FLDLN2 FLDPI
FLDZ FMUL FMULP FNOP FPATAN FPREM FPTAN FRNDINT FRSTOR FSAVE
FSCALE FSQRT FST FSTCW FSTENV FSTP FSTSW FSUB FSUBP FSUBR
FSUBRP FTST FXAM FXCH FXTRACT FYL2X FYL2XP1
```

Consequently, absence of generic `FPO1`, `FPO2`, or `ESC` strings is
non-evidence. Generic-string presence would likewise be insufficient.
Neither generic-string inference is used by the conclusion.

## Formal support conclusion and governance

The formal machine-readable result for both 66 and 67 is:

```text
target_absence_proven
```

The proof is positive and conjunctive: the complete ordinary table, complete
ordered group table, exclusive 0F branch, all four complete FPU tables, and
positive D8-DF fallback range jointly bound every normal monitor-disassembler
alternative and exclude 66/67.

This proves monitor-disassembler target authority for these encodings. It
does not prove complete uPD9002 silicon behavior, electrical behavior, or the
semantics of an unobserved hardware path.

Both SST populations remain `upstream_nonblocking`, with no gap kind before
or after. No `hardware_pending` entry is required because the formal result
is not `target_support_unverified`. There are no taxonomy or registry
changes, no promotion to applicable, and no top-level classification change.
The approved G60b target-policy ID remains unchanged.

The authority transition is `target_authority_audit` and records:

```text
newly_passing: []
newly_failing: []
changed_failure_count: 0
top_level_classification_changes: []
gap_kind_changes: []
hardware_pending_changes: []
```

No production implementation or removal of 66, 67, FPO1/FPO2, D8-DF, 0F28,
0F2A, BRKFEM, or BRKEM is part of M60c.

## Final evaluated profile results

The exact `a9dd78bded5c1072f0285f00cf7759654da8b7d8` worker ran all three
profiles under the approved G60b policy without profile skip:

| Profile | Selected | Applicable/executed | Pass | Fail | Timeout | Crash | Elapsed |
|---|---:|---:|---:|---:|---:|---:|---:|
| architectural CI | 180,000 | 165,300 | 157,179 | 8,121 | 0 | 0 | 206.21 s |
| architectural full | 1,562,502 | 1,438,594 | 1,378,653 | 59,941 | 0 | 0 | 443.93 s |
| fingerprint full | 1,562,502 | 1,438,594 | 1,276,215 | 162,379 | 0 | 0 | 451.29 s |

Exact result identities are unchanged:

| Profile | Pass-set SHA-256 | Failure-set SHA-256 | Signature-index SHA-256 |
|---|---|---|---|
| architectural CI | `ae83f89ed1bfe34088197012683c726d9dab5a76700bb247efcf9a7e6e0d8bff` | `04ffb31baf4f66e60f0e700ebe8713ad7b1d582352160dc02c61415619426603` | `a67c9135780ce1df35486b2c2a28fe3e309be97c30045b4761ed5a8c652a4132` |
| architectural full | `898b7e6e66fa9ea4e475bfc78db5a0cf8c3d6b109276fac6db0f68e05a1c27f2` | `9c69f518d168e1675e4d1fcf59db031dc1bce058db3fdfe9f6cb66b8d2bec91d` | `776c17c19e52cc03becc83464dfcdcb0b1590532812deac7e93c8955f0c1a473` |
| fingerprint full | `691242bb0a05324fe3be261653863db45f45fd495c83d6798d8491a3e8db42db` | `2d825411e958ec980317d0c29d06aee8f9e7337c036f1d5703014311e53c2cc4` | `84c40271c91cb8d01e3545ada60c50e35594e790ce29e9b4d609c75e1d59d3bb` |

The normalized scoreboard comparisons differed from the approved G60b
summaries only in `raw_result_summary_sha256`, which binds the newly generated
runner-wrapper file. Every contract-governed field was identical. All eight
CI, ten architectural-full, and ten fingerprint-full deterministic failure
shards had identical filenames and byte-identical content to G60b.

| Profile | Final raw-result SHA-256 | Canonical sidecar-set SHA-256 | Raw shard-set SHA-256 |
|---|---|---|---|
| architectural CI | `1b025a304eeb68f59b25419df07dcaa618a899ddb10685a196b5c8ee95662149` | `d27b42b714d7f25a289ec057ce3c278be8b78e58d1626cc3d2b359093382c6aa` | `df6d28cf97884be7105c2a64b141513685c29f0b088af1e8a2dfb8d22dae9f8e` |
| architectural full | `cdc136e7e33469a17fc8dd99d2d92899b4b6dd68681a9485ea5541dd7802c063` | `50d02072857abd48c7df66e9ea2b5a72c449b6714f8d62f8d96ed39d8c9af488` | `a2550a4beb9367e943670e02b06ba7908939ee763cbd2ae1801463b59adc03d4` |
| fingerprint full | `bc4c7c505ca2ea944235f87d028cae3f76edad59c27eebfb42902daea8f3d8ad` | `9708241508d99b37abf9cc2967ed70599a3539d54a1d1f514275bd8a03305686` | `636d5f377003956840c3be5e9ba63ec0377a022448ef1d002bdd57d83de9fd81` |

The raw wrapper digest is not used as a substitute for the canonical
scoreboard identity. The accepted gzip/path environment limitation remains
bounded: committed canonical shards reproduce byte-for-byte in the recorded
environment, while arbitrary cross-environment or raw temporary-path gzip
identity is not claimed.

Therefore `newly_passing`, `newly_failing`, and changed failure signatures
are all empty; mismatch classes and termination classes are unchanged; all
timeouts and crashes are zero.

## Generated G60c evidence

The content-addressed evidence families are:

```text
tests/ssts/authority/g60c/
tests/ssts/authority/g60c_result_manifest.json
tests/ssts/transitions/g60c_target_authority_from_g60b.json
```

Principal identities:

| Artifact | SHA-256 |
|---|---|
| authority manifest | `7c556b1edd22637744dfeef6063ec6139bf78a6e350100292dc39cdc5361c68f` |
| 66/67 case-table compressed bytes | `04d3b9d976eddfeb6f0fc8f7d92578a3eeb72b2db0ab2685a823387ca25f8a73` |
| 66/67 case-table canonical content | `d84434a4eeee407986639d281fce5f50569ec27c651d4d0f676aaf89623682a9` |
| group-dispatch raw artifact | `60beab6a24e0885cddc7411fd00411255c2edf4ce814839760685d13274b58e3` |
| group-dispatch decoded artifact | `ad9381d66c7084fd772b1fc09513d0d178d303729f0cbc4a617e399f3d046f9c` |
| decoder-path artifact | `57171e46ac507232f66dfc3dbae742254251bde698942e1c2ab89f083eb3bae8` |
| FPU raw artifact | `6401e51d7b1439ae4b81a0c028483b8a4b74050e9c53653e615ceb97530c6a8c` |
| FPU decoded artifact | `d062a7bdb04ef0a1d67724fd1e1274a25b1e1c265328d4d90b1309dac5391337` |
| FPU mnemonic artifact | `82d93749914cd0b91f7015bddc3768335470a3caf491f7e06dd893494ad5cfa3` |
| support-conclusion artifact | `de668f6c4f2696b18acb9bd6eb207698b9260ad69b6e5df3c633dc7910fb2b0f` |
| transition | `85b8b7466e39fbe761bc5993c570878070b053da38f2605edbb97eab05a20751` |
| result manifest | `a2787adb099818db5d07f1aafb3f69b31efe7d95e5bb27569095d40e4754baa3` |
| complete G60c artifact tree | `51c0bda25f87f679c795df36ac9dc925176890ded55205235999d5598c24fc79` |

The manifest enumerates every artifact path, byte count, row count, and
SHA-256. It records `analysis_evaluated_sha` as
`a9dd78bded5c1072f0285f00cf7759654da8b7d8`, never the containing evidence
commit.

Complete generation ran twice in the same Python 3.14.4 / gzip 1.14 / zlib
1.3.1 environment and produced byte-identical authority, compressed case
table, transition, and result-manifest outputs. The double-generation command
took 7.39 seconds; final generation took 3.83 seconds. Exact ROM
re-extraction verification took 0.51 seconds, and complete static verification
took 37.74 seconds.

## Validation and hosted CI

`upd9002_m60c_audit.py selftest` passed four positive and 57 fail-closed
negative tests. Rejections cover:

- wrong historical counts/digests or recurrence of the 417/224 opcode-label
  error;
- incomplete or first-byte-only 66/67 selection, wrong prefix decoding,
  duplicate/missing hashes, metadata/support ownership omissions, and count
  inconsistency;
- wrong ROM/table bounds or counts, malformed records, incomplete group
  links, unreachable paths, partial table decoding, and wrong `e7 26`
  expansion;
- incomplete FPU tables/mnemonics or handler links, duplicate ownership, and
  FPO2 conclusions without a complete 66/67 trace;
- generic-string or ordinary-table-only absence inference, failure-list
  passing inference, and cross-CPU undefined inference;
- top-level, selected/applicable, contract, unauthorized taxonomy/registry,
  hardware-pending, fixture, protected-artifact, or production-CPU mutation.

The standard native build succeeded. All 51 CTest tests passed in 808.86
seconds, including the verified external architectural CI gate; no required
profile was skipped. Repository encoding, EOL, path-case, milestone-ID, and
documentation checks passed. `git diff --check` produced no output.
After final evidence generation, the 13 M58/M59/M60a/M60b/M60c,
milestone-ID, and static-evidence CTest tests were rerun and all passed in
108.58 seconds.

The evaluated implementation hosted CI is:

[build 30147009649](https://github.com/nakatamaho/vaeg/actions/runs/30147009649)

All eight jobs succeeded: repository invariants, Linux GCC/Clang, ASan,
Windows MinGW, macOS, standalone Z80 conformance, and the verified uPD9002
architectural SST ratchet. The verified corpus was available and the
architectural CI gate did not skip.

The final evidence commit triggers the same workflow. Its successful run and
URL are supplied in the maintainer handoff because the commit cannot contain
the URL of a workflow that starts only after the commit exists.

## Complete changed-file summary

Relative to G60b, the documentation/evaluated commits contain only:

```text
.github/workflows/build.yml
CMakeLists.txt
docs/agents/ROADMAP.md
docs/agents/UPD9002_SEMANTICS_MIGRATION.md
tests/ssts/schema/fpo2-authority-audit-v1.md
tools/qa/upd9002_m60c_audit.py
tools/qa/upd9002_m60c_erratum.py
```

The final evidence commit contains only:

```text
docs/agents/reports/m60c_upd9002_fpo2_main_dispatch_audit.md
tests/ssts/authority/g60c/decoder_paths_66_67.json
tests/ssts/authority/g60c/fpu_d8_df_decoded.json
tests/ssts/authority/g60c/fpu_d8_df_raw.json
tests/ssts/authority/g60c/fpu_mnemonic_map.json
tests/ssts/authority/g60c/group_dispatch_decoded.json
tests/ssts/authority/g60c/group_dispatch_raw.json
tests/ssts/authority/g60c/historical_label_erratum.json
tests/ssts/authority/g60c/manifest.json
tests/ssts/authority/g60c/ordinary_primary_table_reference.json
tests/ssts/authority/g60c/primary_66_67_sst_audit.json
tests/ssts/authority/g60c/primary_66_67_sst_cases.json.gz
tests/ssts/authority/g60c/source_provenance.json
tests/ssts/authority/g60c/support_conclusions.json
tests/ssts/authority/g60c_result_manifest.json
tests/ssts/transitions/g60c_target_authority_from_g60b.json
```

It changes no tooling, test logic, policy logic, CPU code, fixture,
classification, or comparison contract.

## Principal commands and human review

The principal evaluated commands exited zero:

```text
cmake -S . -B build/linux-ci-gcc -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVAEG_BUILD_SDL2=OFF \
  -DVAEG_BUILD_TESTS=ON \
  -DVAEG_UPD9002_SSTS_DATASET_ROOT=<verified-corpus>
cmake --build build/linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope ci --profile architectural --policy g60b \
  --output <temporary-result> --failure-directory <temporary-sidecars>

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope full --profile architectural --policy g60b \
  --output <temporary-result> --failure-directory <temporary-sidecars>

python3 tools/qa/upd9002_m60b_authority.py run-profile \
  --root . --dataset-root <verified-corpus> \
  --worker build/linux-ci-gcc/sdl2/vaeg \
  --scope full --profile fingerprint --policy g60b \
  --output <temporary-result> --failure-directory <temporary-sidecars>

python3 tools/qa/upd9002_m60c_erratum.py selftest
python3 tools/qa/upd9002_m60c_erratum.py verify-static
python3 tools/qa/upd9002_m60c_audit.py selftest
python3 tools/qa/upd9002_m60c_audit.py regenerate-twice \
  --root . --dataset-root <verified-corpus> \
  --rom /tmp/vaeg-m60b-authority/monitor.rom \
  --evaluated-sha a9dd78bded5c1072f0285f00cf7759654da8b7d8
python3 tools/qa/upd9002_m60c_audit.py verify-authority \
  --root . --rom /tmp/vaeg-m60b-authority/monitor.rom
python3 tools/qa/upd9002_m60c_audit.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

For human review from a clean final-candidate checkout:

```text
python3 tools/qa/upd9002_m60c_erratum.py selftest
python3 tools/qa/upd9002_m60c_erratum.py verify-static
python3 tools/qa/upd9002_m60c_audit.py selftest
python3 tools/qa/upd9002_m60c_audit.py verify-authority \
  --root . --rom /tmp/vaeg-m60b-authority/monitor.rom
python3 tools/qa/upd9002_m60c_audit.py verify-static --root .
python3 tools/qa/milestone_ids.py --selftest --audit --discover
ctest --test-dir build/linux-ci-gcc --output-on-failure
git diff --exit-code \
  4e5d74d0d9f675df2342353b8bfdbb2e5cded768...a9dd78bded5c1072f0285f00cf7759654da8b7d8 \
  -- cpu/upd9002/
git diff --check
```

## Known limitations

- The positive exclusion result proves the monitor disassembler's bounded
  target authority, not complete uPD9002 silicon validation.
- No executed SST result exists for 66/67 under the approved policy; the audit
  does not describe either population as passing.
- The complete ROM remains private and out of tree. Independent regeneration
  requires the exact authorized ROM identity.
- Individual FPU mnemonics and D8-DF dispatch ownership are established, but
  M60c does not validate FPU execution semantics.
- Deterministic gzip byte identity is bounded to the recorded environment,
  consistent with the limitation accepted at G58.
- M60c neither implements nor removes 66/67. Any production cleanup prompted
  by this authority result requires a separately approved future milestone.

G60c remains unapproved pending human review. M60d is untouched.

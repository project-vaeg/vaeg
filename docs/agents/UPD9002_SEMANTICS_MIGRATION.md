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
# uPD9002 Semantics Migration Specification (v5)

Repository: `github.com/nakatamaho/vaeg`

## 1. Status

- G57 approved at `72322d5c9b8e40e4a988312aebe163a8190e2aa5`.
- G58 approved at `bc8a55c6da1082b85b794068e0d933e31fe46b13`.
- G59 approved at `e7f2325bc81310532091a8ca82914030fdb8b6ba`.
- M59 analysis/evaluated SHA is `7b4bd12aecf92e8fe8299d8b1ec5e48bbb1b61a7`.
- G60a through G61 are approved historical gates. G61 is approved at
  `829f314bb0d363ec5b6e9aa738e948b1a3adb365`.
- G62 is approved at
  `70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`.
- G68 is approved at
  `d1e0225c4edb716893fe5579283fbf0915db72b9`.
- G69 is approved at
  `680308a603b24341c5b9649657f01791b79002f7`.
- G70 is approved at
  `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`.
- G71 is approved at
  `24950894eca79e308afae8d574d43c8f393bb483`.
- G72 is approved at
  `643d9f7289d817c67f343bf01be368b546bc1438`.
- M73 is reserved for the post-M49 runtime performance regression observed
  between approved M49 and M50 checkpoints.
- M76 is reserved for uPD9002 main-CPU uPD780 emulation-mode authority after
  M75 SCSI evidence and before the I/O source-tree reorganization. This is
  separate from the FDC subsystem uPD780-compatible CPU cleanup planned later
  in the VA-only source-tree sequence.

Codex executes one milestone or lettered submilestone per session and stops at its candidate gate.

## 2. Approved historical evidence

- M50 removed protected-mode execution machinery.
- M51 established `cpu/upd9002/` ownership and names.
- G57 removed the frozen Win9x/i286x tier.
- G58 established the hash-level ratchet, architectural/fingerprint profiles, and gap taxonomy.
- G59 produced the 160,000-row evidence pack without changing production semantics.
- G43/G58/G59 artifacts remain immutable, including the historical G43 OUTS fixture correction.

The G59 architectural full profile had 84,329 applicable failures and the CI profile 10,593. M60a
may change those values semantically. M60b must use the exact approved G60a result as predecessor;
it must not assume the G59 counts remain current.

## 3. Mission

Replace remaining 286-derived native instruction semantics with evidence-supported uPD9002/V30-class
semantics, while keeping target-absent and unresolved forms outside claims of correctness.

Completion means zero failures among the current target-correct `applicable` set, not that every V20
SST record is executed and not that silicon accuracy is complete.

## 4. Profiles and denominator

### 4.1 Architectural — blocking

Compare metadata-defined final FLAGS bits, final registers/IP, SST-represented final RAM bytes, and
termination. Cycles, prefetch, and bus timing are excluded.

```text
blocking_denominator = count(current top-level classification == applicable)
pass_rate = applicable_pass / blocking_denominator
```

Non-applicable categories are reported separately and never counted as passes.

### 4.2 Fingerprint — diagnostic

Compare all 16 final FLAGS bits. This is V20-observed evidence, not a blocking uPD9002 contract.

### 4.3 Target-policy epochs

Dataset and comparison-contract identities are independent of target policy. M60b creates a new
content-addressed target-policy epoch because the applicable set changes. Every later scoreboard
must record dataset ID, comparison-contract ID, target-policy ID, and selected/applicable digests.

## 5. ROM target authority

M60b must bind maintainer-provided findings to content-addressed evidence. At minimum record ROM
SHA-256/size, address mapping, table start/end, raw bytes, deterministic decoders, group/mnemonic
mapping, string-pool search range, and independent debugger references. Do not commit copyrighted ROM
bytes without explicit authorization.

### 5.1 Complete V30-side `0F` inventory

The table at `0x66A8A` uses `(mask, value, group)` records and identifies:

- TEST1: `0F10/11/18/19`
- CLR1: `0F12/13/1A/1B`
- SET1: `0F14/15/1C/1D`
- NOT1: `0F16/17/1E/1F`
- ADD4S/SUB4S/CMP4S: `0F20/22/26`
- ROL4/ROR4: `0F28/2A`
- BRKFEM: `0FFE imm8`
- BRKEM: `0FFF imm8`

`0F31/33/39/3B` are absent from the complete table. String-pool absence is corroboration, not the
sole proof. `0F28` is a mandatory implementation-missing form. BRKFEM semantics remain pending.

### 5.2 Target-absent `6C-6F`

Every selected record structurally encoded with primary opcode `6C`, `6D`, `6E`, or `6F` must be
resolved as `known_target_gap/documented_silicon_absent` in the target-correct epoch. The correction
covers currently applicable forms and existing gap forms and is fixed structurally before execution.

The G43 fixture correction that made 1,204 V20 cases pass remains valid historical V20 evidence.
Historical-label correction established by G60b: at G60a, the exact retired failure population was
`6E=0` and `6F=641`. The values 417 and 224 identify, respectively, unchanged-signature and
changed-signature subsets of the same 641-case G43 OUTS transition population; they were not
per-opcode failure counts. The exact content-addressed sets are:

- G60a 6E retired failures: 0,
  `4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945`;
- G60a 6F retired failures: 641,
  `03f8ea83c510e67e27cc60a9455322f0cd899eb88287835080d2f9e98a0fa1f2`;
- unchanged-signature subset: 417,
  `7240eff77e38a2ca67cf94d6cec13c4ddec1f2e122cf62cbb7318ee39c82be2e`;
- changed-signature subset: 224,
  `f70b2e4e614cc677a883bc8d9ceb349f7a9bff32f185b253d893e6aea904a814`;
- G43 fixture-pass population: 1,204,
  `c8de1415733c5bad2ba85d667d56f5d04631d19379ce16f85e641792e7644322`.

Exact content-addressed G60a/G60b hash sets govern all later accounting. None of these populations
are uPD9002 semantic fixes. M60b removes exact target-absent hashes from the denominator and records
them as retired applicable pass/failure sets, never as newly passing.

### 5.3 REPC/REPNC and PREPARE/DISPOSE

Their presence is independent evidence. The absence of base opcodes `6C-6F` must not be generalized
to deletion of REPC/REPNC or PREPARE/DISPOSE.

M70 is a maintainer-approved project-target amendment for the exact
SST-present `64H`/`65H` plus primary string-opcode population recorded by
M65j and consolidated by G67:

- selector groups: 19;
- owned hashes: 5,908;
- population digest:
  `240e0bf76de968b310ad13ef53de8d044637b185e267e1cfb2540f32ab6571e5`.

For this exact population, M70 supersedes the G67
`known_target_gap/target_support_unverified` disposition with
milestone-owned implementation authority. Terminal closure requires every
owned hash to be selected, applicable, executed through production semantics,
and architecturally passing under `upd9002-v20-architectural-v1`.

This amendment does not authorize generic V20/V30 compatibility, i386 FS/GS
interpretation of `64H`/`65H`, implementation of absent corpus selectors, or
fingerprint diagnostics as architectural authority. `REPC + AE` is absent
from the canonical population and receives no M70 conformance claim.

M70 also preserves a negative-protection boundary for `64/65 + 6C-6F`. Those
pairs remain governed by the approved target-absence/reserved-behavior
authority and must not enter the string engine or INM/OUTM semantics without a
later maintainer-approved policy amendment.

## 6. FPU/FPO evidence discipline

Generic mnemonic absence is non-evidence. The monitor stores individual FPU mnemonics and D8-DF
records near `0x66B3B`. FPO2 must be investigated through the main dispatch table near `0x66900`.
The hypothesis that primary opcodes 66/67 encode FPO2 must be verified, not assumed.

M60c must inspect every 66/67 SST record's current classification, selected/executed counts,
metadata status, support-map mapping, and gap kind if any. It then traces the ROM dispatch to a
handler/group. It may correct a known-gap `gap_kind` only from positive evidence or downgrade an
unsupported absence claim to `target_support_unverified` with exact hardware-pending coverage.
It does not implement FPU semantics or change top-level classification.

## 7. Ground-truth rules

- Absence from a failure list is never proof of passing.
- Never use Intel 8086/286 documentation as target authority where SST/ROM target evidence exists.
- Never modify fixtures to improve the target score.
- Preserve expected and actual evidence separately.
- Final RAM does not prove transient order, rollback, or bus activity.
- Do not infer target absence from missing generic strings.

## 8. Classification governance

Top-level categories remain `applicable`, `known_target_gap`, `expected_target_divergence`,
`unsupported_fixture`, and `upstream_nonblocking`. Gap kinds remain
`documented_silicon_absent`, `implementation_missing`, and `target_support_unverified`.

### 8.1 One authorized target-authority correction

Ordinarily `applicable -> known_target_gap` is forbidden. M60b is explicitly authorized to perform
exactly this structural correction for primary opcodes `6C-6F`, after ROM evidence is bound:

```text
applicable -> known_target_gap
gap_kind = documented_silicon_absent
transition_kind = target_authority_correction
```

Requirements:

- exact selectors fixed before observing outcome;
- exact resolved hashes/counts/sorted-hash digests;
- no pass/fail-based partition;
- retired pass and retired failure sets reported separately;
- no other top-level transition;
- no production semantic change;
- unaffected applicable hashes satisfy no-regression.

M60b may also change `gap_kind` to `documented_silicon_absent` for existing exact gaps under
`6C-6F` and `0F31/33/39/3B`, and must preserve `0F28` as
`known_target_gap/implementation_missing`.

After G60b, `applicable -> known_target_gap` is forbidden again without a new approved master-spec
revision.

### 8.2 Ordinary transitions

- `known_target_gap/implementation_missing -> applicable` only in the implementing PR, with all
  newly applicable hashes passing;
- failing `applicable -> expected_target_divergence` only with exact target evidence and explicit
  approval;
- passing records never move to divergence;
- outcome-based splitting is forbidden.

## 9. Ratchet and artifacts

The predecessor is the exact approved gate SHA. A candidate records dataset, comparison contract,
target policy, selected/applicable sets, pass/failure sets, signatures, and terminations.

For M60b, comparison is over the intersection of predecessor/candidate applicable sets plus explicit
`retired_applicable` accounting. Retired failures are not improvements. For ordinary semantic gates,
newly failing must be empty and no unaffected per-form pass count may decrease.

Artifacts record `evaluated_sha`, never the containing evidence commit's SHA. Changed failures and
classification changes are fully enumerated in deterministic shards.

## 10. Guest-visible FLAGS

M60a is governed by its existing task and live prompt. v5 does not change it. Later work must not
reuse interrupt, PUSHF, LAHF, POPF, SAHF, or IRET conventions by analogy without evidence.

M60d is conditional: if G60a already clears synchronous frame populations and no independent frame
residual remains, M60d closes with evidence and no semantic edit. Otherwise it fixes only the proven
residual. M60e handles IRET separately.

## 11. PR and gate discipline

- One primitive/family per semantic PR and one approval gate.
- M62 is the one maintainer-approved exception: AAM, ROR4, ROL4 activation,
  BCD/ASCII adjust, and shifts share G62 only while retaining separate
  pre-edit audits, semantic commits, phase manifests, and hash ownership.
- M64 is a second explicit maintainer-approved exception: DIV/IDIV,
  ADD4S/SUB4S/CMP4S, TEST1/NOT1/CLR1/SET1, and BRKEM share G64 only while
  retaining separate phase audits, semantic commits, phase manifests, and
  exact hash ownership.
- Do not stack semantic PRs on unapproved predecessors.
- No semantic changes after evidence generation.
- Evidence-only and rename-only commits remain separate.
- Regenerate the full target-correct profile after shared changes.
- All new repository text/code is English.

## 12. Prospective order after G60a

1. M60b — ROM authority and target-policy correction.
2. M60c — main-dispatch/FPO2 audit.
3. M60d — conditional synchronous interrupt-frame residual.
4. M60e — IRET.
5. M61 — C6/C7 register-form MOV immediate; F7 `/2` remains separate.
6. M62 — one-time consolidated gate for AAM, ROR4, mandatory ROL4 activation,
   BCD/ASCII adjust, and shifts. Each phase remains independently reviewable.
7. M64 — DIV/IDIV plus the exact requested monitor-authorized ADD4S,
   SUB4S, CMP4S, and TEST1/NOT1/CLR1/SET1 families. Raw ROM
   `(mask,value,group)` records are not instruction-byte sequences. ROL4 and
   ROR4 remain protected G62 behavior; BRKFEM remains out of scope. The
   approved SST v20 metadata names BRKEM (`0FFF`) but has no corresponding
   shard, so M64 records an exact zero-case authority/coverage checkpoint and
   does not claim or invent executable BRKEM semantics.
8. M65 — serial residue campaign after approved G65. M65j decomposes all
   5,908 implementation-missing selectors, then M65a–M65m execute one at a
   time. Intermediate checkpoints are not independently approved; only G65m
   is a formal gate. G65m passed at
   `81887aae14f718d7d4d0f2a7bd3fe05d5ea80630`. M66a requires approved G65m. Enumerate all
   7,511 architectural failures and 5,908 implementation-missing hashes with
   disjoint owners. BRKEM (`0F FF imm8`) is explicitly deferred because the
   v20 metadata exists but the `0fff.json.gz` corpus shard is absent; no cases
   may be fabricated. Generated M65a-or-later tasks, M66a, M66b, and M67 do
   not start in M65.
   reserved behavior, BRKFEM, FPO2, remaining 0F forms, and prefixes.
9. M66 bundle — execute the two canonical cleanup milestones on one linear
   branch: `G65m → M66a internal checkpoint → M66b terminal bundle closure →
   G66b human approval → M67`. G65m is the only approved predecessor. M66a
   runs first and receives no independent approval. M66b starts only from the
   exact M66a checkpoint SHA and is the only terminal candidate presented for
   G66b review. M66a and M66b retain separate scope, ownership, commits,
   evidence, and reports; the combined approval protocol does not permit
   combining unrelated code changes into one commit.
10. M67 — final divergence and hardware-question consolidation after approved
   G66b. M67 starts only from G66b at
   `97f760e8da573888edf089c2875c623895a3c2c9` and records the current
   divergence-domain state in `tests/ssts/divergence/g67/registry.json`.
   Compatibility views for approved target divergences, hardware-pending
   questions, evidence backlogs, zero-coverage items, fingerprint diagnostics,
   and state-compatibility exceptions are generated from that registry rather
   than hand-edited as duplicate current sources. Conflicts discovered during
   consolidation are blockers, not permission for M67 to change target policy
   or classifications. The M65j 19-group/5,908-hash target-support-unverified
   backlog remains non-applicable, unimplemented, not officially executed, not
   claimed passing, and nonblocking only under its recorded amendment. BRKEM
   and BRKFEM zero-coverage or evidence-backlog records remain non-passing
   until separate approved evidence/corpus gates. Fingerprint residue remains
   diagnostic rather than architectural blocking. The G66b
   `CPU286 v0 + UPD9002 v0 -> UPD9CPU v1 + UPD9002 v0` migration bridge is a
   state-format exception only and does not restore broader CPU286
   compatibility. G67 passed at
   `f8f350e1aadec4b6c79c20192d14c50bd39934be`.
11. M68 — maintainer-reassigned runtime-regression hotfix. M68 restores
   canonical mapped-memory dispatch ownership for uPD9002 segmented word
   access after the approved G67 predecessor. The defect is not MOVSW
   iteration semantics; it is an independent host-memory fast path inside the
   segmented word helper that bypasses canonical VA mapped-memory dispatch.
   The segmented helper owns only segment-offset address calculation and
   `FFFFh -> 0000h` wrapping. The canonical generic memory API owns RAM,
   TVRAM, BMS and device mapping, callbacks, side effects, dirty tracking,
   and fast-path selection. Any former unapproved M68 scope is revoked and
   deferred for later reassignment. The unrelated TSP `0142H` status-port
   defect is explicitly out of scope. G68 passed at
   `d1e0225c4edb716893fe5579283fbf0915db72b9`.
12. M69 — IDP/TSP `0142H` status-bit composition correction. M69 starts from
   the approved G68 candidate and fixes only the Boolean composition of the
   status byte: stored IDP status flags are preserved and the dynamic VB bit
   is added orthogonally. It does not change IDP timing, FIFO behavior, IBF,
   OBF, ER, SC, EMEN, light-pen behavior, command latency, rendering,
   segmented memory, MOVSW, or CPU instruction semantics.

## 13. Definition of done

- Zero failures among target-correct `applicable` hashes.
- No `implementation_missing` remains.
- Every `6C-6F` form and `0F31/33/39/3B` gap is exact and evidence-backed as target absent.
- `0F28` is applicable and passing.
- Active `6C-6F` V20 handlers are not reachable or advertised as uPD9002 instructions; final
  reserved behavior is evidence-governed.
- FPO2 status is positively resolved or explicitly pending without a false absence claim.
- BRKFEM unresolved semantics remain explicit. BRKEM's zero-case SST coverage
  and unresolved executable compatibility and silicon-mode semantics remain
  explicitly separated until an approved corpus adds cases.
- Historical G43/G58/G59 artifacts remain byte-identical and the 1,204 OUTS gain is never presented
  as target progress.
- No active I286/i286c production identity remains.
- SST success is not described as complete hardware validation.

## 14. Prohibitions

- Do not change or restart M60a under v5.
- Do not revert the G43 fixture correction.
- Do not count reclassified `6C-6F` records as fixed or passing.
- Do not implement V20 INM/OUTM/INS/OUTS as uPD9002 correctness.
- Do not infer FPO2 absence from missing generic strings.
- Do not infer RETEM/CALLN absence from the V30-side `0F` table.
- Do not leave 0F28 conditional after target authority is accepted.
- Do not touch cycles, prefetch, or bus timing in semantic milestones.

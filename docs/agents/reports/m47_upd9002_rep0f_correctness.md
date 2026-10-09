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
# M47 uPD9002 REP+0F correctness evidence

Status: the behavior-neutral implementation and required local validation are
complete. The architectural decision is unresolved because no authoritative
uPD9002-specific rule or PC-88VA hardware result is available. This report does
not declare G47 passed, authorize a baseline change, or authorize M48.

The final evidence and remote SHA are reported in the handoff because this file
cannot contain the SHA of the commit that contains itself.

## Identity, corrected scope, and commits

- Branch: `topic/m47-upd9002-rep0f-correctness`
- Accepted G46 starting SHA:
  `5a9b4c1de72ce18fd7989c8db22a542ca49ede09`
- Pre-report evidence SHA:
  `60781fa62e08c1b54a6ec3f0bac4bec663093295`
- Hosted-tested report SHA:
  `13f7aac9b95e324994261324da0005e147e7c302`
- Hosted run: [build 29639368627](https://github.com/nakatamaho/vaeg/actions/runs/29639368627),
  7/7 jobs successful
- Accepted M47 pre-implementation audit SHA-256:
  `3b93bc4b9fefe3026b3dd1533a968f7f6ea2a6b6b00a9c4756d3315dbcdfa5ea`

The worktree was created cleanly at the exact approved G46 SHA. No reset,
rebase, squash, or M42--M46 history rewrite was performed.

The accepted audit disproved the original M47 premise: REPNE/REPE-prefixed 0F
is actively routed into NP2 80286 system handlers. The old inventory/deletion
sequence was therefore replaced before implementation. M47 is now evidence and
decision preparation; M48 is a separately gated correctness transition, and
the old inventory, deletion, and rename work moved to M49, M50, and M51.

Commits before this report are:

1. `60132cc` — shift protected-mode tasks to M49--M51;
2. `4e97310` — correct the roadmap after the REP+0F audit;
3. `3df75d3` — pin documentary evidence;
4. `90d75cb` — inventory REP+0F corpus and protected-state evidence;
5. `fdabd05` — record current REP+0F execution evidence;
6. `5938263` — add the PC-88VA hardware probe;
7. `1b6eada` — prepare the prospective transition manifest;
8. `60781fa` — document the unresolved correctness boundary.

## Accepted current behavior

The current behavior is preserved, not certified as correct uPD9002/V52
behavior:

```text
v30c_step
  -> v30op[0xf2] -> v30_repne -> v30op_repne[0x0f] -> i286c_cts
  -> v30op[0xf3] -> v30_repe  -> v30op_repe[0x0f]  -> i286c_cts
  -> v30op[0x0f] -> v30_ope0x0f -> v30ope0x0f_table
```

Inside `i286c_cts`:

- second byte 00 enters `cts0_table` only with MSW_PE set; otherwise INT 6;
- second byte 01 selects `cts1_table`;
- second byte 05 calls `LOADALL286`;
- every other second byte raises the legacy INT 6 path.

The tables reach SLDT, STR, LLDT, LTR, VERR, VERW, SGDT, SIDT, LGDT, LIDT,
SMSW, and LMSW. LLDT/LTR and active SEGSELECT callers can reach
`i286c_selector`. A valid state import may restore MSW_PE and descriptor state,
and `io/cpuio.c` reads CPU_MSW. Reset and CPU_SHUT transformations also own
the protected fields. These dependencies prevent treating the cluster as dead.

The focused current-behavior test records:

```text
0f 01 f0       -> IP 2002, MSW 0000
f2 0f 01 f0    -> IP 2004, MSW 0001
f3 0f 01 f0    -> IP 2004, MSW 0001
```

The unprefixed path consumes only 0F 01 as the current reserved V30 form. Both
prefixed paths consume all four bytes and execute legacy LMSW AX. This test is
explicitly labelled `current-behavior-only`.

## Documentary evidence

The deterministic identity manifest is
`docs/agents/research/m47_upd9002_rep0f_documents.json`. The three acquired
PDFs remain external and are not committed.

| Document | Exact edition/date | Scope | Relevant pages/sections | SHA-256 | Finding |
|---|---|---|---|---|---|
| [uPD70108/V20 & uPD70116/V30 User's Manual](https://bitsavers.org/components/nec/V-Series/V20_V30_Users_Manual_Oct86.pdf) | October 1986 | V20/V30 | 6-3--6-4 interrupt/prefix retention; 12-14--12-15 repeat prefixes | `8252fe5ddc0623380b7f28819009e6bb7d2b8e345ef347d9e6f2d299ce97865f` | Defines family prefix rules, not uPD9002 REP+0F |
| [16-BIT V SERIES ... INSTRUCTION](https://www.ardent-tool.com/CPU/docs/NEC/V20-V30/v_series.pdf) | U11301EJ5V0UMJ1, 5th edition, September 2000; information current February 1997 | V20/V30/V25/V35/V40/V50 | 1.3.2 p3; REP family pp124--128; Appendix C pp200/202 | `19a69547651aa65e32df33f1bf84dea095a00237f680a67b166213fcc6d96f6a` | Defines REP targets and 0F Group 3 separately; combined encoding unresolved |
| [V30MX CPU Core User's Manual](https://www.bitsavers.org/components/nec/V-Series/uPD70116mx-um_199704.pdf) | A11897EJ1V0UM00, 1st edition, April 1997 | V30MX | 3.6 p37; 5.3 p68 | `71d7c5c749d1c3839a32fa04b6405c420f3358c5e1e62dd130a7014dd54d474e` | Not a uPD9002/PC-88VA rule |
| [PC-88VA Technical Manual](https://ci.nii.ac.jp/ncid/BA48885862) | revised edition, January 1988, 454 pages, NCID BA48885862 | PC-88VA/uPD9002 | contents unavailable | not available | Bibliographic identity only; cannot be semantic evidence |

No tracked PC-88VA/uPD9002 technical or instruction manual was found. The
available NEC documents do not select candidate A, B, C, or D. An unsourced
emulator implementation was not used as architectural proof.

## Complete instruction matrix

`tools/qa/golden/upd9002_rep0f_instruction_matrix_m47.csv` contains 768 rows:
all 256 second bytes for no prefix, F2, and F3. Its SHA-256 is
`1f82a81371a664f7c83559e58e1fe8445c8e3f15c779b016787b6abdb5a0375f`.

Every row records bytes, mnemonic or undefined class, ModR/M constraints,
initial registers/FLAGS/memory, current path/result, IP, registers/FLAGS,
memory/I/O, interrupt/exception behavior, architectural prefix status,
evidence, confidence, M43 metadata/prefetch counts, and hardware case IDs.
The 25 NEC Group 3 forms represented by M43 are named individually. All other
unprefixed second bytes record the accepted V30 reserved path. For F2/F3 the
matrix separately identifies CTS0 at 00, CTS1 at 01, LOADALL286 at 05, and the
legacy INT 6 result for the other 253 values.

Current VAEG reachability has high confidence from source, M42 artifacts, and
the runtime test. The architectural prefix status of all 512 F2/F3 rows remains
`unresolved: candidate outcomes A, B, C, or D`.

## SingleStepTests V20 analysis

The analysis verified the exact M43 dataset:

```text
ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4
```

All 3,125,000 records and all 360 shards were schema-validated. There are 105
records with adjacent byte values F2/F3,0F somewhere in the encoded byte array,
but every pair is operand, immediate, or displacement data. There are:

- zero decoded F2/F3 instruction-prefix + 0F records;
- zero empty-prefetch executable REP+0F records;
- zero populated-prefetch REP+0F records;
- no F2 or F3 opcode shard, because upstream metadata classifies them as
  prefixes;
- zero applicable, known-gap, or semantic-failure hashes affected by any of
  candidates A--D;
- zero failure signatures expected to change.

The 105 adjacent records, including canonical record hash, upstream hash,
metadata form, instruction bytes, positions, prefetch class, and complete
initial/expected state, are enumerated in
`tests/ssts/baseline/upd9002_rep0f_corpus_m47.json`. Its SHA-256 is
`72c5f8f0b25f6c635e90a704598420ad46c7a3aa2a13d1c3a19fcf9ee830db1e`.
The artifact is byte-identical to an independent full regeneration.

This is an absence of a V20 oracle, not proof of uPD9002 behavior.

## PC-88VA hardware probe

The standalone probe lives under `tools/hardware/pc88va_rep0f/` and is not
connected to CMake or production. NASM emits one 8.3-compatible DOS COM file
per case. Every invocation prints its ID and exact bytes before execution,
hooks INT 1 and INT 6, executes one case, then reports completion, saved IP and
FLAGS, general/segment registers, trap number, and a 12-byte guard.

| Case | Bytes | Purpose | Size | SHA-256 |
|---|---|---|---:|---|
| r001 | `0f 10 c0` | harmless implemented V30 TEST1 control | 1944 | `00c088a9e3f997bf5c14b05bc7c4d32a577464661782e5f65ac58a7bf46bf8cd` |
| r002 | `f2 0f 10 c0` | F2 TEST1 | 1946 | `0e96f9169eeeb307e2fc876909b42f66814427b813a7db67a146654cae505a99` |
| r003 | `f3 0f 10 c0` | F3 TEST1 | 1946 | `6ac009ee97094153bd82022b66324bfcd2a60dd477ea0f08bebcee4e45e27228` |
| r004 | `0f 01 f0` | unprefixed LMSW-shaped control | 1944 | `346567984dfb5ef6151586b059353851eddb307ffea613db3aea26265a58e074` |
| r005 | `f2 0f 01 f0` | F2 current legacy LMSW route | 1946 | `e0e63a12bba753e940ec62aa89f7a344b94bcf2c1b2d971a144b7a7436a2cbdc` |
| r006 | `f3 0f 01 f0` | F3 current legacy LMSW route | 1946 | `7cfffc6dfa633c423aff832cb2e0c5eb397720e039dc9e34b4f48cae4f7713c9` |
| r007 | `f2 0f 00 c0` | representative CTS0 route | 1946 | `9a2c4de33147f7ec40feca8955c58c504624e5f431505d6f88b73d7038e03b99` |
| r008 | `f2 0f 01 06 guard16` | representative CTS1 SGDT guard write | 1948 | `94778f8b1a1471eaf235ba43cf6225cdc39253f81ea79ce721cf3d224cf6f734` |

r005/r006 set AX=0000, so an LMSW-shaped interpretation cannot set MSW_PE.
r008 can write only its private guard. Probe media stays write-protected and no
file is opened or written. One case is run per invocation and the machine is
rebooted between cases. A hang/reset is recorded before a power cycle; no state
is assumed to survive it. LOADALL286 is omitted because it can replace CS:IP,
FLAGS, descriptors, and stack from a fixed physical image and no generally
recoverable probe was justified.

NASM 3.01 reproduced all eight digests. The parser was smoke-tested with a
separately labelled emulator log. This verifies the tooling only; DOSBox and
VAEG results are not PC-88VA architectural proof.

Real-hardware result: **pending, 0/8 cases supplied**. Therefore no semantic
candidate is selected.

## Protected-state occurrence and policy analysis

`tests/upd9002/protected_state_inventory_m47.json` has SHA-256
`df9cb2a9793edae9683a0dab8b94e1e6d200fd721bb10e0ef161c1abfcfadda3`.
The audited CPU286 payload locations are:

| Field | Offset | Size | Accepted reset/executed-3/CPU_SHUT residue |
|---|---:|---:|---|
| GDTR | 64 | 6 | zero |
| MSW | 70 | 2 | zero |
| IDTR | 72 | 6 | zero |
| LDTR | 78 | 2 | zero |
| LDTRC | 80 | 6 | zero |
| TR | 86 | 2 | zero |
| TRC | 88 | 6 | zero |

All three committed/generated M42--M46 scenarios have zero protected residue.
The accepted M44 `make_noncanonical` valid-import test has nonzero GDTR limit,
base and opaque bytes plus nonzero TRC opaque bytes; import and opaque
round-trip are accepted. No user-visible save corpus was provided, so zero user
states were scanned and no private identity was recorded.

No policy is selected:

| Policy | Data-loss / silent-change risk | Compatibility and version | Matrix / CPU_SHUT | Complexity |
|---|---|---|---|---|
| Reject residue | no silent mutation, but affected saves become unusable | breaks accepted import; requires explicit compatibility/version rule | three zero-residue scenarios remain; noncanonical import changes; CPU_SHUT must remain exact | medium |
| Canonicalize | high data loss and silent semantic change | breaks G41/M44 byte-exact residue round trip; versioned migration required | canonical fixtures can remain; CPU_SHUT exempt | medium |
| Compatibility adapter | mapping loss depends on native representability | versioned mapping and new fixtures required | new cross-version matrix; CPU_SHUT untouched | high |
| Opaque-only, no runtime execution | serialized loss low, resume meaning intentionally dropped | overlay ownership and likely state policy/version change | zero-residue matrix can remain; residue tests required; CPU_SHUT exact | medium-high |

G47 must approve a policy for MSW_PE and every descriptor/cache field, not just
the two currently nonzero test values.

## Prospective M48 transition manifest

`tools/qa/golden/upd9002_rep0f_transition_manifest_m47.json` has SHA-256
`ef06ab700896773fd91e4dc076834f767a70d243e0f011cdc82d60eddd2d91f3`.
It is marked `prospective-not-authorized`, enumerates all 512 prefixed matrix
case IDs, and records the accepted artifact digests.

If G47 selects candidate A, exactly these source-level root changes are
prospective:

- final graph: `v30op_repe[0x0f]` and `v30op_repne[0x0f]` change from
  `i286c_cts` to `v30_ope0x0f`;
- provenance: the same two base rows gain explicit patch operations to
  `v30_ope0x0f`;
- support map: the same two root rows target `v30_ope0x0f`, with the second
  byte resolved by the normal 0F support map;
- all 512 prefixed matrix cases are semantically in scope;
- M43 applicable, known-gap, failure hashes, and failure signatures changed:
  zero.

Candidates B--D have no invented target. The manifest explicitly lists every
one of the 512 rows requiring resolution and records the M43 affected set as
empty. An exact M48 replacement cannot be authorized until the selected rule
defines those targets, exception/restart behavior, operands, and IP effects.

The three state fixtures, their payload sizes/tags, and CPU_SHUT FLAGS 0000
remain fixed under every candidate. State rejection/migration changes require
the separately approved policy. Historical M42/M43 artifacts are never
overwritten; any authorized M48 difference must be a new transition artifact.

## M42--M46 preservation

M42 source artifacts remain byte-identical to accepted SHA
`336227f093d37e3c60bc50333216d66668755cef`:

| Artifact | SHA-256 |
|---|---|
| final dispatch graph | `e8c8fded6e1f44ef2d74ace93ba1063d64464b217bdd4f9180bf50e86e19df3d` |
| construction provenance | `605e9af6761e3069f6c5af0ad9647ae574ab98252f7373969586cfee892cd6ba` |
| support map | `7b856f95d8c3d0356325483d188b9dde2d03a8e6cbe9b7fd0cacd004f1d09d13` |
| 156-case harness manifest | `14769ca59bf589921cbcf88dc0c357937f6da36e4fe0b78f9a46dc75b9098f2a` |
| trace baseline | `17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba` |
| state fixtures | `c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb` |
| G41 ABI fixture | `f6995d59a293a4e81fc710400fd447031016322018a1c88c9e752c1eb1889db7` |

GCC, Clang, and ASan/UBSan each passed 32/32 CTests. This includes dispatch,
156-case harness, trace, ABI, reset/executed-3/CPU_SHUT fixtures, state
boundary/atomic rejection, Z80, and ROM-less selftests.

M43 preserved the dataset ID above, 40 known-gap selectors, 68,626 exact
record hashes, and known-gap SHA-256
`11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`.
The independent CI profile reproduced:

| Profile | Executed | Pass | Semantic failure | Timeout | Crash | Signature index |
|---|---:|---:|---:|---:|---:|---|
| CI | 166,821 | 156,228 | 10,593 | 0 | 0 | `946268103309f8dc7d442fade21596b46c734f48bf0b1e9e32a18736e5e85597` |
| full empty-prefetch | 1,443,876 | 1,359,547 | 84,329 | 0 | 0 | `50087f8f6b9483ac70ce5e2dc922ab11fade51a58bea9cb72322ef85ef264ec0` |

Both `--expect` runs and sidecar directory comparisons completed successfully.
No classification or signature changed.

M44's fresh G41/M47 bidirectional matrix passed all four comparisons. CPU286
payloads remain 112 bytes and UPD9002 payloads 16 bytes. Reset, executed-3,
and CPU_SHUT hashes remain respectively:

```text
CPU286 reset       6cbd3314a586ccf9c5f1d11a17b8836f76f1deaf10814de8b61ebdf96a792f89
CPU286 executed-3  45d06e424fd332027817ca66db7ed0298e87695cf0cfed5582b7fe1348faa7fe
CPU286 CPU_SHUT    7636def12b5f25c39540a1367166394a8308064cce74a15860c62a487a946ff7
UPD9002 all        374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb
```

Invalid cpu_type, declared size 111, seven-byte truncation, opaque bytes,
transactional preflight, and atomic rejection passed. Reset FLAGS remains
f002 and CPU_SHUT FLAGS remains 0000.

M45 reports 13 supported presets, native `v30c_step`, V30-native reset,
CPU_SHUT's `i286c_initreg` exception, state-validation-only cpu_type, no block
executors, and no CPU_EXEC macros. M46 reports one `v30cinit` production call,
one successful construction, constructor-only writes, and element-wise
immutability for roots 256/256/256/256/8/8. Runtime reset/selftest/state-load
checks passed.

## Builds, production isolation, and repository checks

| Configuration | Result |
|---|---|
| GCC 15.2 `linux-debug`, `linux-release`, `linux-ci-gcc` | clean configure/build passed; GCC CTest 32/32 |
| Clang 21.1 `linux-ci-clang` | clean configure/build; CTest 32/32 |
| GCC ASan/UBSan `linux-asan`, `linux-ci-asan` | clean configure/build; CTest 32/32 with leak detection disabled; focused halt-on-UB run 4/4 |
| MinGW-w64 GCC 13 `mingw-cross` | clean configure/build; PE32+ GUI x86-64 |
| Wine 10.0 | MinGW production selftest and smoke passed |
| production isolation | tests OFF; no M47 source, define, option string, diagnostic, or symbol in Ninja graph or ELF binary |
| probe reproducibility | NASM 3.01, 8/8 digests exact; parser smoke passed |
| encoding/EOL/case | 0/0/0 findings |
| unreferenced report | 70 findings: accepted prior 69 plus the intentionally standalone, production-unlinked probe ASM |
| hosted Linux/Windows/macOS/standalone | [build 29639368627](https://github.com/nakatamaho/vaeg/actions/runs/29639368627): 7/7 jobs successful on `13f7aac9b95e324994261324da0005e147e7c302` |

The first clean FetchContent configure required network access for the pinned
zlib/libarchive/SDL2 archives; rerunning with network access succeeded. Native
Windows and macOS cannot run locally and are delegated to hosted CI.

## Exact commands and exit statuses

Every command below exited zero unless marked as the initial sandbox-network
retry. Paths under `/tmp` are disposable evidence directories.

```text
python3 tools/qa/upd9002_rep0f_analysis.py check \
  --root . --dataset-root /tmp/singlesteptests-v20-9efbd02b
python3 tools/qa/upd9002_rep0f_analysis.py check-static --root .
python3 tools/qa/upd9002_rep0f_analysis.py selftest --root .
python3 tools/qa/upd9002_rep0f_analysis.py verify-documents \
  --root . --document-root /tmp
python3 tools/hardware/pc88va_rep0f/build_probe.py check \
  --output /tmp/vaeg-m47-probe-report.h4LR8i

cmake --preset linux-debug --fresh
cmake --preset linux-release --fresh
cmake --preset linux-ci-gcc --fresh
cmake --preset linux-ci-clang --fresh
cmake --preset linux-asan --fresh
cmake --preset linux-ci-asan --fresh
cmake --preset mingw-cross --fresh
cmake --build build/linux-debug --parallel 2
cmake --build build/linux-release --parallel 2
cmake --build build/linux-ci-gcc --parallel 2
cmake --build build/linux-ci-clang --parallel 2
cmake --build build/asan --parallel 2
cmake --build build/linux-ci-asan --parallel 2
cmake --build build/mingw-cross --parallel 2

ctest --test-dir build/linux-ci-gcc --output-on-failure
ctest --test-dir build/linux-ci-clang --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=0 \
  ctest --test-dir build/linux-ci-asan --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir build/linux-ci-asan --output-on-failure \
    -R 'vaeg_upd9002_(dispatch_normalization|rep0f_current_behavior|state_boundary|state_payload_probe)$'

python3 tools/qa/upd9002_dispatch.py --root .
python3 tools/qa/upd9002_dispatch.py --root . --selftest
python3 tools/qa/upd9002_native_invariant.py --root .
python3 tools/qa/upd9002_dispatch_normalization.py --root .
build/linux-ci-gcc/sdl2/vaeg --upd9002-m46-dispatch-qa
build/linux-ci-gcc/sdl2/vaeg --upd9002-m47-rep0f-current
build/linux-ci-gcc/vaeg_upd9002_state_boundary
build/linux-ci-gcc/vaeg_upd9002_state_payload_probe
build/linux-ci-gcc/vaeg_upd9002_abi

python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg --profile ci \
  --shard-timeout 120 --output /tmp/vaeg-m47-ssts.sTtPMh/v20_native_ci.json \
  --failure-directory /tmp/vaeg-m47-ssts.sTtPMh/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/linux-ci-gcc/sdl2/vaeg --profile full \
  --shard-timeout 300 --output /tmp/vaeg-m47-ssts.sTtPMh/v20_native_full.json \
  --failure-directory /tmp/vaeg-m47-ssts.sTtPMh/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
cmp /tmp/vaeg-m47-ssts.sTtPMh/v20_native_ci.json \
  tests/ssts/baseline/v20_native_ci.json
cmp /tmp/vaeg-m47-ssts.sTtPMh/v20_native_full.json \
  tests/ssts/baseline/v20_native_full.json
diff -qr /tmp/vaeg-m47-ssts.sTtPMh/v20_native_ci_failures \
  tests/ssts/baseline/v20_native_ci_failures
diff -qr /tmp/vaeg-m47-ssts.sTtPMh/v20_native_full_failures \
  tests/ssts/baseline/v20_native_full_failures

python3 tools/qa/upd9002_state_matrix.py \
  --fixtures tests/upd9002/state_fixtures_m42.txt \
  --g41-generated /tmp/vaeg-m47-matrix.i6ajc2/g41-generated \
  --m44-generated /tmp/vaeg-m47-matrix.i6ajc2/m47-generated \
  --g41-to-m44 /tmp/vaeg-m47-matrix.i6ajc2/g41-to-m47 \
  --m44-to-g41 /tmp/vaeg-m47-matrix.i6ajc2/m47-to-g41 \
  --g41-sha dc8a72da974f0ea328613e480f1de662c28f4436 \
  --m44-sha 60781fa62e08c1b54a6ec3f0bac4bec663093295

git diff --exit-code 336227f093d37e3c60bc50333216d66668755cef -- \
  tools/qa/golden/upd9002_final_dispatch_graph.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m42.csv \
  tools/qa/golden/upd9002_support_map_m42.csv \
  tests/upd9002/harness_manifest.csv tests/upd9002/trace_baseline.txt \
  tests/upd9002/state_fixtures_m42.txt tests/upd9002/abi_g41.txt

SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --selftest
SDL_AUDIODRIVER=dummy SDL_VIDEODRIVER=dummy \
  build/linux-release/sdl2/vaeg --smoke
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m45-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --selftest
WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m45-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/mingw-cross/sdl2/vaeg.exe --smoke

python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check

git push -u origin topic/m47-upd9002-rep0f-correctness
gh run view 29639368627 --json status,conclusion,url,jobs
```

## Files changed and production boundary

Changed files are limited to:

- ROADMAP, the M42--M51 consolidation plan, corrected M47/M48 tasks, and
  shifted M49--M51 task files;
- ADR-0012's amended schedule, new ADR-0013, the document identity manifest,
  and this report;
- the deterministic M47 analysis tool and its instruction/corpus/state/
  transition evidence artifacts;
- the standalone hardware probe, build verifier, result parser, manifest, and
  documentation;
- a target-local current-behavior C test plus CMake/test-only CLI wiring.

No production CPU, dispatch, state adapter, instruction, memory, I/O, timing,
DMA, prefetch, interrupt, exception, or save-format source changed. No accepted
M42/M43 baseline, frozen-reference file, ROM/disk/binary payload, public API, or
bug-fix ledger entry changed. M47 records evidence and corrects a plan; it does
not fix guest-visible behavior.

## Deviations and unresolved risks

- No real PC-88VA result has been supplied. This is the decisive blocker.
- The identified PC-88VA Technical Manual was unavailable for content/digest
  inspection.
- No user save-state corpus was provided, so protected-residue prevalence is
  unknown.
- DOSBox smoke verifies COM output/recovery tooling only and is not target
  evidence.
- Native Windows and macOS validation is hosted rather than local.
- The initial clean FetchContent attempts failed only because sandbox DNS was
  disabled; approved network retries completed with pinned sources.
- Existing warnings and sanitizer diagnostics in untouched legacy/vendor code
  remain outside M47. Focused M47/M44/M46 sanitizer tests halt cleanly on UB.
- The standalone probe ASM appears as the one new intentional unreferenced
  source because production must never link it.

## Exact G47 decisions and checklist

The evidence currently supports only **unresolved / do not begin M48**. The
maintainer must not approve a rule from successful emulator builds alone.

```text
[ ] real PC-88VA r001 result captured and source-labelled
[ ] real PC-88VA r002 result captured and source-labelled
[ ] real PC-88VA r003 result captured and source-labelled
[ ] real PC-88VA r004 result captured and source-labelled
[ ] real PC-88VA r005 result captured and source-labelled
[ ] real PC-88VA r006 result captured and source-labelled
[ ] real PC-88VA r007 result captured and source-labelled
[ ] real PC-88VA r008 result captured and source-labelled
[ ] REP+0F rule A, B, C, or fully specified D explicitly approved
[ ] MSW_PE and all protected-residue state policy explicitly approved
[ ] exact M48 dispatch/state changes explicitly approved
[ ] exact historical M42/M43 transition scope explicitly approved
[ ] evidence explicitly declared sufficient to begin M48
[ ] clean-checkout review confirms no production M47 seam
```

Until every decision item is explicit, M48, M49, M50, and M51 remain blocked.

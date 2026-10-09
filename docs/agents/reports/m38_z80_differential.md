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
# M38 Z80 differential evidence

Date: 2026-07-15

Status: implemented; **G38 passed** after the maintainer-approved eventual
convergence test; stopped before M39

## Starting state and scope

The branch was created as `topic/m38-z80-differential` from exact approved
G37 commit `9599e5c9b91ef545e6e7dea5957681e8ce30b3a4`. The required starting
commands returned:

```text
$ git status --short
?? "PC-Engine 1.05.d88"
?? docs/modernization/PCEPAT.DOC
?? docs/tekumani/
?? his.txt
$ git rev-parse HEAD
9599e5c9b91ef545e6e7dea5957681e8ce30b3a4
$ git branch --show-current
topic/m38-z80-differential
```

All four paths remain untracked and untouched. The optional M34 report was
absent. All other required inputs were read in the requested order. No ROM,
disk image, or frozen-reference input was used.

The implementation and first documentation-closure commits are:

```text
8923817 M38: correct wrapper state exposed by differential probes
37ffe3c M38: add normalized Z80 differential harness
6d5d6e9 M38: document blocked differential gate
dc25a3e M38: record final validation evidence
bf5ca60 M38: prove FDD event convergence across slices
```

The final validation-record commit and exact ending SHA are reported in the
G38 handoff; a commit cannot embed its own immutable SHA.

## Runner architecture and trace schema

The harness uses three independent executables:

- `vaeg_z80_legacy_trace` links `cpucva/z80c.cpp` and `z80diag.cpp`;
- `vaeg_z80_new_trace` links the standalone
  `vaeg_z80_wrapper_no_functional` library;
- `vaeg_z80_trace_compare` parses both traces and reports the first normalized
  difference.

The old and new `Z80C` definitions never enter one process or translation
unit. There is no class-renaming macro. Both runners compile the same scenario
descriptions, memory patches, event scripts, seeds, and clock slices from
`trace_common.cpp`.

The `vaeg-z80-trace-v1` text schema uses LF, stable field order, fixed-width
lower-case hexadecimal, and no host pointers, timestamps, or temporary paths.
Each test header records its identifier, seed, initial program bytes, and
numeric action script. Ordered event records identify memory read/write,
masked I/O read/write, interrupt acknowledge, NMI, state save, and state load.
I/O and acknowledge events also record callback-time live and public PC.
Checkpoint records contain:

- AF, BC, DE, HL, IX, IY, SP, and architectural PC;
- alternate AF, BC, DE, and HL;
- I, complete R, R low seven bits, R bit 7, IFF1, IFF2, and IM;
- HALT, external WAIT, external IRQ, and EI inhibition;
- authoritative live PC and public mirrored PC;
- `remainclock`, `lastclock`, and cumulative consumed clocks;
- full-memory and selected device/stack memory hashes;
- checkpoint index, reason, and normalized/non-normalized status.

The comparator matches `test/reason/occurrence`, not private decoder step
count. Events from non-normalized EI checkpoints accumulate into the next
normalized checkpoint. Architectural fields, R bits 0-6, R bit 7, memory
hashes, and event order are strict except for an exact allowlist entry. Clock
and remaining-balance differences go to the cycle report. Lost, duplicated,
reordered, or permanently divergent external effects fail G38; a verified
architectural timing correction may move an otherwise identical event to a
later `Exec()` slice only when eventual convergence is proven.

## Corpus

The directed corpus has 61 scenarios and 64 normalized checkpoints. It covers
loads, arithmetic/flags, taken and untaken branches, CALL/RET/PUSH/POP,
alternate exchange, IX/IY, address wrapping, memory ordering, LDI/LDD/LDIR/
LDDR, CB/ED/DD/FD/DDCB/FDCB, `LD A,R`, RETN/RETI, immediate and `(C)` I/O,
all eight directed block-I/O forms, DI/EI, changed acknowledge data,
deasserted and persistent IRQ, IM0 `00`, `7f`, RST, multi-byte and prefix
forms, IM1, ordered IM2, HALT entry/wake, NMI, NMI priority, and WAIT resume.

The state corpus has 22 scenarios and 26 normalized checkpoints. It loads all
15 retained M34 revision-1 fixtures, then covers positive/zero/negative clock
state, HALT, external WAIT, restored asserted IRQ, and the focused EI
inhibition save/load boundary. It compares decoded/resumed behavior, not raw
new-versus-old serialized bytes, and does not ask the legacy core to load a
new EI-bit image.

The convergence corpus adds one scenario and one normalized checkpoint. The
public generated corpus uses fixed seeds `0x4d383001`, `0x4d383002`,
`0x4d383003`, and `0x4d383004`, with 128 bounded defined base-instruction
cases per seed: 512 cases/checkpoints. The trace header preserves the exact
first failing generated program if a regression occurs. Prefix and interrupt
behavior stay in the directed suite so a broad generated allowlist cannot
hide their classified differences. The longer local run uses 4,096 cases per
seed: 16,384 cases/checkpoints.

## Wrapper corrections supported by minimal probes

M38 corrected only the standalone vaeg wrapper and its focused tests. The
production core is unchanged.

| Finding | Minimal probe | Disposition |
|---|---|---|
| The third-party core increments R for the first opcode but not a following CB/ED/DD/FD M1 fetch. | `cb 00`, R=`ff`; raw IM0 `cb` plus memory `00` | Adapter tracks only a proven prefix first byte and increments R on its following M1 callback. No extra memory read occurs. |
| HALT idle reads did not increment R. | HALT followed by one 4-clock idle slice | Adapter increments R before each complete third-party HALT-idle execution. |
| RETI did not copy IFF2 to IFF1; RETN with clear IFF2 set IFF1. | `ed 4d` and `ed 45`, stack return word, both IFF2 values | Adapter restores IFF1 from IFF2 at instruction completion. |
| `LD A,I`/`LD A,R` changed only PV and left the other defined flags stale. | `ed 5f`, R=`ff`, F carry set | Adapter materializes S/Z/5/H/3/PV/N while retaining carry. |
| New synchronous NMI refreshed public PC, unlike the legacy split `inst`/mirror model. | NMI at PC `2222` | Adapter updates architectural mirror fields but retains the pre-NMI public PC until `Exec()` returns. |

The prefix/opcode tracking exists only while the core is executing a complete
instruction. No instruction or callback is active at the approved frame save
boundary, so no new revision-1 field is needed. Wrapper tests cover normal and
`Z80_NO_FUNCTIONAL` configurations. Raw and wrapper ZEX remain green.

## Evidence-backed allowlist

The comparator contains exact scenario/checkpoint sets, not opcode ranges.
Every identifier below emitted at least one accepted field or event. The
generated corpus emitted none.

| Stable ID | Minimal input and first difference | Classification and safety | FDD relevance |
|---|---|---|---|
| `vaeg-m38-halt-pc-model` | `76`; live/public PC old=opcode, new=post-HALT | Approved explicit HALT-PC translation; architectural decoded PC matches. | Representation only. |
| `vaeg-m38-halt-fetch-model` | restored HALT then one slice; old rereads opcode, new performs post-PC idle read | Internal HALT representation. Current subsystem reads are ROM/RAM/`ff`, with no memory-mapped device side effect. | No current device effect; clocks are separate. |
| `vaeg-m38-legacy-stack-write-order` | `cd 34 12`, `c5`, RST/IRQ/NMI pushes; old low then high, new high then low | Legacy `Push` writes a little-endian word after subtracting SP; new uses Z80 bus order. Final bytes/SP match. Current `Subsystem::Write8` only stores `0x4000-0x7fff` RAM and has no MMIO callbacks. | No current FDD I/O event; bus-order correction retained. |
| `vaeg-m38-legacy-skipped-branch-operand-read` | `20 7f` with Z set; old skips the displacement callback | Legacy omits a real operand read. Current Z80 memory map has no read-side-effect MMIO; architectural result matches. | None in current map. |
| `vaeg-m38-legacy-xf-exchange` | `08`, AF=`1234`, AF'=`abcd`; old result `abe5`, new `abcd` | Legacy lazy `xf` leaks across EX AF,AF'. Architectural AF is authoritative by ADR-0011. | Deliberate architectural correction. |
| `vaeg-m38-legacy-ei-fusion-deassert` | `fb 00`, asserted IRQ, slice 1, deassert, slice 4; old acknowledges, new does not | Legacy fuses EI's following instruction and makes API deassertion impossible at that boundary. New behavior is the approved level/EI contract. | Deliberate interrupt correction. |
| `vaeg-m38-legacy-interrupt-r` | NOP plus accepted IRQ, NMI, or persistent reacceptance; old omits acknowledge M1 R increment | New core increments R on interrupt/NMI acknowledge; low seven bits are recorded and bit 7 stays strict. | Architectural correction; software reading R can observe it. |
| `vaeg-m38-legacy-halt-r` | loaded HALT plus accepted IRQ; old R=`01`, new R=`02` | New counts HALT idle and interrupt-acknowledge M1 cycles. | Architectural correction. |
| `vaeg-m38-legacy-block-memory-flags` | `ed a0/a8/b0/b8`, A=`12`, data=`34`; old F=`05`, new F=`25` | Legacy omits defined/undocumented transfer-result flag materialization; new result is retained with ZEX conformance evidence. | Guest flags can differ; deliberate correction. |
| `vaeg-m38-legacy-block-io-flags` | `ed a2/aa/b2/ba/a3/ab/b3/bb`, B=`02` | Legacy updates only Z/N; new core materializes block-I/O flags and passes ZEX. | FDD code can observe flags; deliberate correction. |
| `vaeg-m38-ei-save-boundary-scheduling` | `fb 00`, save/load after slice 1 | Old has already fused NOP/IRQ; new saves `execEI`. After reload the next instruction, ack count/value, stack result, and architectural state agree apart from separately classified R/stack order. | Required production-reachable state boundary. |

EI-following I/O/acknowledge callbacks can see different stale public PC
values because the new core can return immediately after EI. In the exact
allowlisted probes the callbacks are port `0xf0` and the acknowledge port;
neither production handler consumes mirrored PC. A hypothetical EI directly
before the production SLEEP_HACK `IN 0xfe` remains a G39 private-system risk,
not an observed M38 result.

## Cycle evidence and eventual convergence

The generated [cycle report](../../modernization/z80-compat-cycle-deltas.md) lists
30 normalized checkpoints with a nonzero consumed/balance delta, including
the convergence checkpoint. The generated corpus has zero cycle-delta groups.
Examples include taken JR legacy/new `7/12`, RET `4/10`, repeated block I/O
`16/21`, DDCB `16/20`, IM1 acceptance `17/12`, and legacy HALT wake `77/12`.

The separate unallowlisted scheduling suite remains slice-exact evidence:

```text
initial: A=5a, PC=0000
bytes:   18 02 00 00 d3 f4
slices:  1, 7
first divergent checkpoint: cycle-fdd-io-scheduling/fdd-io-slice/0
legacy: PC=0006, R=02, reads 0004:d3 and 0005:f4, OUT f4:5a
new:    PC=0004, R=01, no memory or I/O event in that slice
```

The source explanation is exact: legacy charges 7 clocks for taken JR, while
the new core charges the architectural 12. Production
`Subsystem::Out(0xf4)` calls `fdcsubsys_o_dskctl`, so the difference is not
relabelled as externally inert. The reproducer remains and still exits 1 with
five differences (PC, R, live PC, public PC, and events).

The maintainer-approved `cycle-fdd-io-eventual-convergence` case keeps the
same program and `1,7` slices, then supplies identical additional `4,1`
slices. At the normalized 13-clock boundary, both runners have the same
ordered five events, including exactly one `OUT (0xf4),0x5a`. They finish
with PC=`0006`, R=`02`, AF=`5a00`, SP=`f000`, identical other architectural
and interrupt state, matching full/device memory hashes, and `lastclock=13`.
No I/O is lost, duplicated, or reordered. The only remaining differences are
consumed clocks `18/23`, balances `-5/-10`, and the earlier slice assignment.

Under the revised policy, G38 blocks only on lost, duplicated, reordered, or
permanently divergent external effects. A verified architectural timing
correction may shift a slice boundary when eventual convergence is proven.
This case passes G38 but remains a mandatory M39 private integration risk.
M39 must stop production integration if real VA/FDD testing finds transfer
failure, timeout, corrupted data, changed IRQ/DRQ sequencing, or broken
save/load behavior. No legacy per-opcode timing emulation was added.

## Validation evidence

### Native Linux

| Exact command | Result |
|---|---|
| `cmake --preset linux-ci-gcc -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m36-z80.7UocP2/zex-cache` | PASS; all five configured ZEX artifacts reverified |
| `cmake --build --preset linux-ci-gcc` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ctest --test-dir build/linux-ci-gcc --output-on-failure` | PASS, 17/17; raw ZEXDOC 72.58 s, raw ZEXALL 71.06 s, wrapper ZEXDOC 141.02 s, wrapper ZEXALL 140.75 s, differential 3/3, ROM-less selftest 1.24 s |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-gcc/sdl2/vaeg --smoke` | PASS, reduced ROM-less smoke |
| `cmake --preset linux-ci-clang && cmake --build --preset linux-ci-clang` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ctest --test-dir build/linux-ci-clang --output-on-failure` | PASS, 13/13 |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-clang/sdl2/vaeg --smoke` | PASS, reduced ROM-less smoke |
| `cmake --preset linux-ci-asan && cmake --build --preset linux-ci-asan` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan --output-on-failure` | PASS, 13/13; differential 3/3 under ASan/UBSan |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 ./build/linux-ci-asan/sdl2/vaeg --smoke` | Process PASS; reproduced the pre-existing signed-overflow/shift UBSan backlog in `tms3631c.c`, `psggenc.c`, `psggeng.c`, and `parts.c` |
| final all-suite CMake-script command with `SUITE=all`, `CASES=128` | PASS; 596 scenarios, 603 normalized checkpoints, 58 exact accepted matches |
| long CMake-script command documented in `BUILD.md`, `SUITE=generated`, `CASES=4096` | PASS; 16,384 cases/checkpoints, zero allowlist matches, about 3 seconds locally |
| direct legacy/new `--suite scheduling`, then comparator | Expected slice-exact comparator failure under the superseded policy; five preserved differences, exit 1 |

### Clock-policy continuation

| Exact command | Result |
|---|---|
| GCC build of the three trace targets, then `ctest --test-dir build/linux-ci-gcc --output-on-failure -R '^vaeg_z80_differential_(directed\|state\|generated\|convergence)$'` | PASS, 4/4 |
| direct legacy/new `--suite convergence`, then comparator | PASS; exactly one `OUT 00f4:5a`, 1 normalized checkpoint, 0 allowlist matches |
| direct legacy/new `--suite scheduling`, then comparator | Expected slice-exact evidence failure preserved; the same five differences, exit 1 |
| Clang build of the three trace targets, then the same four-test CTest regex | PASS, 4/4 |
| ASan/UBSan build of the three trace targets, then the same four-test CTest regex with `ASAN_OPTIONS=detect_leaks=0` | PASS, 4/4 |
| MinGW cross-build of the three trace targets | PASS |
| Wine legacy/new `--suite convergence`, then the Wine comparator | PASS; exactly one `OUT 00f4:5a`, 1 checkpoint, 0 allowlist matches |

### MinGW and Wine

| Exact command | Result |
|---|---|
| `cmake --preset mingw-cross && cmake --build --preset mingw-cross` | PASS, cross-compile only |
| `WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m38-wine WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wine64 build/mingw-cross/vaeg_z80_wrapper_default.exe` | PASS, all 10 grouped wrapper tests |
| same command with `vaeg_z80_wrapper_no_functional_test.exe` | PASS |
| Wine legacy/new/comparator commands for directed, state, and generated suites | PASS: 64, 26, and 512 normalized checkpoints respectively; generated uses zero allowlist entries |
| `WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m38-wine WINEPATH=... SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy wine64 build/mingw-cross/sdl2/vaeg.exe --smoke` | PASS, reduced ROM-less smoke |

Wine is execution of a cross-built Windows binary, not hosted native Windows.

### Repository, archive, provenance, and hosted CI

| Exact command or evidence | Result |
|---|---|
| `python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `python3 tools/repo/find_unreferenced.py` | PASS, 69 pre-existing paths and no M38 path |
| `git diff --check` | PASS |
| Python YAML parse of `.github/workflows/build.yml` and `.github/workflows/release.yml` | PASS |
| `git archive --format=tar.gz --output=/tmp/vaeg-m38-source-6d5d6e9.tar.gz HEAD` then archive checker | PASS, 987 files and no ZEX artifact |
| staged five-file Linux runtime archive then archive checker | PASS, 5 files and no ZEX artifact |
| SHA-256 of the four vendored files and the approved M35 patch, plus `git diff --exit-code 9599e5c... -- external/suzukiplan-z80 ...patch` | PASS; hashes unchanged and no diff |
| `git diff --exit-code 9599e5c... -- iova/subsystem.cpp` and frozen-tier name-status check | PASS, no diff |
| `ar t build/linux-ci-gcc/libvaeg_va.a \| rg 'z80'` | `z80c.cpp.o`, `z80diag.cpp.o`; no new-wrapper object |

Vendored SHA-256 values are:

```text
ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1  LICENSE.txt
59d660c57262d1166cd317877691496e0a072200f4e20e388f8d88e77ba39cda  provenance.txt
abe7cd10642c6d13ad0636ef53091cd3c4bf643ecd6f564a22707393e06728c4  test/test-interrupt-extension.cpp
88c878f0087f114eb864dc6a9e8cb98473c022e052c62937ab7a23aecbdb7106  z80.hpp
d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4  M35 patch
```

The final all-suite trace hashes are legacy
`30aedfb57fbc849561c4870ae233568f3777b174c94a83607f836a7785a95fc7`
and new
`3ee3af91d52269fde3143b97829f8a05db5080e8286a9bf5988a473a65090037`.
The focused convergence hashes are legacy
`cb48b389bb932c2d466628a06b7bf0f886d7b116d6fbf59314ea29663f2c1b2c`
and new
`127201f2a64b4e73cbbd14c4f1e4c64382686d0ef3bcd82fc397ebcdf5beeb35`.
The long generated trace hashes are legacy
`0fa6b1c03342db39d0c802b84cf38a95a3b683efe0a9eee7f9ce0f91d1ce9cd8`
and new
`d3f574db8a1d4806acabe3a604f600d917b7e4ffb1bf1f2d85c3f2c5fba0bf4f`.

[GitHub Actions run 29389442853](https://github.com/nakatamaho/vaeg/actions/runs/29389442853)
for SHA `6d5d6e9035ecadf209c0123a551409ad337d6fc3` completed successfully
with all seven jobs green: native Windows MSYS2/MinGW64, native macOS,
Ubuntu GCC, Ubuntu Clang, Ubuntu ASan/UBSan, standalone Z80 conformance, and
repository invariants. The Windows and macOS jobs each passed configure,
build, native smoke, and native unit tests. Native macOS and native Windows
were not run locally; only the hosted results are claimed as native.

[GitHub Actions run 29391037922](https://github.com/nakatamaho/vaeg/actions/runs/29391037922)
for convergence-policy SHA
`1abb3ef90b1dc9305220d46499eee82613cdd30f` also passed all seven jobs.
The new convergence CTest passed in the native Windows and macOS unit suites
and the Ubuntu GCC, Clang, and ASan/UBSan suites. The standalone job passed
raw and wrapper ZEX plus archive exclusion. Job runtimes were Windows 486
seconds, macOS 158, Ubuntu GCC 86, Ubuntu Clang 92, Ubuntu ASan/UBSan 85,
standalone conformance 517, and repository invariants 7.

## Verified facts, limitations, and hypotheses

### Verified facts

- The normal directed, state, public generated, and long generated corpora
  pass with no unresolved architectural, bus, interrupt, WAIT, HALT, or
  resumed-state difference outside the exact classifications above.
- All 15 retained revision-1 fixtures load in both runners. External WAIT,
  IRQ, clock signs, HALT, and EI inhibition resume at completed instruction
  boundaries.
- Every external test I/O event is eight-bit masked. Acknowledge events remain
  distinct and IM2 order is acknowledge, vector low, vector high.
- The standalone wrapper corrections do not require a third-party edit or a
  revision-1 extension.
- Production still compiles the legacy `cpucva/z80c.cpp`; the new wrapper is
  absent from production sources and `iova/subsystem.cpp` is unchanged.
- The scheduling reproducer changes the timing of an actual FDD control-port
  call and remains documented. Identical additional slices prove that the
  ordered external effect and final state eventually converge, so G38 passed
  under the maintainer-approved policy.

### Accepted differences

The eleven stable IDs in the allowlist table are classified with minimal
inputs and exact checkpoint sets. They are legacy defects, deliberate
architectural corrections, or externally inert representation/scheduling
differences. None is used by the generated corpus.

The FDD slice shift is not in that allowlist and is not classified as
externally inert. It is accepted only by the explicit convergence rule and is
a mandatory M39 private integration risk.

### Known limitations

- The public corpus is bounded and is not exhaustive instruction fuzzing.
- Cycle identity is deliberately not asserted. Thirty normal
  checkpoints have deltas, and the FDD scheduling case proves at least one is
  externally consequential.
- Private subsystem ROM and disk behavior is not tested or committed. G39 is
  the intended private-system comparison milestone and was not started in
  M38.
- Native hosted platform results are reported only from the pushed CI run and
  are not inferred from cross-compilation or Wine.

### G38 disposition

`cycle-fdd-io-scheduling` remains a slice-exact divergence, but it no longer
blocks G38 because `cycle-fdd-io-eventual-convergence` proves no lost,
duplicated, reordered, or permanently divergent effect. G38 passed. M39 must
stop integration on any real VA/FDD transfer failure, timeout, data
corruption, IRQ/DRQ sequencing change, or save/load regression.

### Hypotheses

- Other recorded cycle deltas may move additional FDD events between frame
  calls; the one minimal `0xf4` case proves the risk class but does not
  enumerate every affected instruction sequence.
- EI immediately before either SLEEP_HACK `IN 0xfe` address may expose the
  new intermediate public-mirror boundary. No private ROM evidence was used,
  so this remains a hypothesis for later authorized work.

M39 production selection, private differential work, disassembler work, and
all approved legacy deletions were deliberately not started.

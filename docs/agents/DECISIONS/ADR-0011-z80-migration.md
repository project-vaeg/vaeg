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
# ADR-0011: Replace the M88-derived Z80 subsystem through a gated migration

## Status

Accepted through G41. M41 implementation, private validation, release audit,
and native Linux/Windows/macOS hosted CI passed. The maintainer
approved the reproducible downstream patch and documented callback test matrix
as sufficient M35 provenance on 2026-07-15. M36 reproduced and vendored the
approved tree, G36 passed, and the maintainer explicitly authorized M37. The
standalone M37 implementation and all configured hosted-platform jobs passed.
M38 has a deterministic old/new harness. Its FDD-visible slice shift remains
documented, and the maintainer-approved eventual-convergence test proves that
the external effect is delayed but not lost, duplicated, reordered, or
permanently divergent. G38 passed under the revised clock policy. M39 added
the opt-in production build and passed its public and maintainer-private boot,
FDD, SLEEP_HACK, WAIT-wake, and state gates.

## Decision

Use `suzukiplan/z80` release `1.10.0`, commit
`e3926769a790fab0af1c34a5540e317f8d4f0ddc`, as the replacement base. The
upstream repository is `https://github.com/suzukiplan/z80`; its code is MIT
licensed, Copyright (c) 2019 Yoji Suzuki. At the inspected revision:

| Item | SHA-256 |
|---|---|
| `LICENSE.txt` | `ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1` |
| `z80.hpp` | `4d2f76f6fae1a4650d4392d2bce067384716e3206b61891855953a4edb89467e` |

MIT-licensed third-party code is acceptable. Vendored upstream or fork code
retains its MIT notice; vaeg-owned new files use BSD-2-Clause. C++17 is
permitted only for approved CPU backends and thin adapters under `cpucva/`.
No C++ STL or third-party type may cross the consumer-visible subsystem,
debugger, C bridge, or state-save interfaces. Other new emulator-core code
remains C99 unless separately approved.

Development remains upstream-first where practical. For M35, the maintainer
approved a reproducible downstream patch against the exact base above as
sufficient provenance; an immutable public upstream or fork commit is not
required. A vendored copy must never be hand-edited. Before vendoring, M36 must
apply the approved patch to the approved base, reproduce the expected tree,
and record the base SHA, patch SHA-256, and resulting tree hash in the vendored
provenance file.

## Legacy provenance and deletion boundary

The active source and compiler-dependency inventories find exactly these seven
M88/cisc-derived Z80 files:

```text
cpucva/types.h
cpucva/z80.h
cpucva/z80if.h
cpucva/z80c.h
cpucva/z80c.cpp
cpucva/z80diag.h
cpucva/z80diag.cpp
```

This was the final approved deletion list. M41 removed exactly these seven
files from current HEAD after the dependency inventory found no non-legacy
consumer. No additional active M88-derived Z80 file was found. Git history and
frozen reference consumers remain untouched.

## Consumer-visible contract

Keep the class name `Z80C` and source-compatible signatures for every method
used by the active tree: constructor/destructor, `Init`, `Exec`, `Reset`,
`IRQ`, `Wait`, `GetStatusSize`, `SaveStatus`, `LoadStatus`, and `GetReg`.
Retain `GetPC`, `SetPC`, and `NMI` as required architectural services. M40
removed the temporary `GetDiag()->Disassemble` bridge. The public `TestIntr`,
`GetWaits`, `IsIntr`, dump controls, and statistics API had no active consumer
and were removed at cutover.

Independently author the replacement contracts. Familiar names may be kept to
minimize consumer edits, but no M88 comments, layout, macro collection, or
implementation structure may be copied. Use these vaeg-owned component names:

```text
cpucva/z80_compat_bus.h
cpucva/z80_compat_registers.h
cpucva/z80_compat_state.h
cpucva/z80_compat_state.cpp
cpucva/z80_compat_cpu.h
cpucva/z80_compat_cpu.cpp
cpucva/upd780_disasm.h
cpucva/upd780_disasm.cpp
```

`z80_bus.h` owns fixed-width memory, eight-bit external I/O, clock, and clock-
counter contracts. `z80_registers.h` owns the vaeg public mirror. The codec
owns the revision-1 byte image; the runtime must not use that compiler-shaped
image as its internal state. `z80_core.*` owns `Z80C` and the third-party
adapter. `z80_disasm.*` is introduced only in M40.

The public mirror returned by `GetReg()` is refreshed when `Exec()` returns,
as today. It is intentionally stale within callbacks. `GetPC()` and state save
use authoritative live state. State save materializes architectural flags.

## Execution, bus, and interrupt contract

`Exec()` adds unsigned `now - lastclock` to the signed remaining-clock value.
If external WAIT is set, it consumes all positive credit without executing.
Otherwise it retires complete instructions while credit is positive; the final
instruction can overshoot into zero or a negative debt. It then refreshes the
public mirror and stores `lastclock`. All memory addresses wrap at 16 bits.
Every external IN, OUT, and block-I/O port is passed to vaeg as `port & 0xff`.

IRQ is an external level: assertion persists until vaeg deasserts it. The
acknowledge callback is invoked exactly once when a maskable interrupt is
actually accepted, after the current instruction and EI inhibition, and never
for assertion alone or NMI. IM1 reads and ignores the byte; IM2 uses it as the
vector low byte. IM0 treats it as the raw first opcode: it is not fetched from
memory and does not advance PC, while all operands or following prefix bytes
are fetched normally from current PC. The focused practical acceptance set is
the reset value `0x7f`, arbitrary values written by subsystem port `0xf0`,
RST-family values, and `0x00`. A maintainable normal-decoder injection supports
multi-byte and prefixed values too; test CB, ED, DD, FD, DDCB, and FDCB in M35.
No unsupported byte may silently become a no-op or fixed RST.

## Required M35 third-party extension

The inspected core's `generateIRQ()` is a one-shot latch, its IM0 path calls
`RST(interruptVector)` rather than decoding a raw opcode, and it has no
acknowledge callback. Therefore the unmodified revision cannot implement the
approved contract. M35 must add, in upstream style:

```text
setIRQLine(bool asserted)
an unsigned-eight-bit interrupt-acknowledge callback
raw first-opcode dispatch through the normal decoder at IM0 acceptance
```

The existing one-shot APIs must retain their semantics. The new level state
must be inspectable/restorable, directly or through the setter. The callback
must work with both normal `std::function` and `Z80_NO_FUNCTIONAL` function-
pointer configurations. IM0/IM1/IM2 callback timing, EI inhibition,
deassertion, persistent assertion, HALT wake, raw multi-byte/prefix decode, and
NMI non-acknowledge need focused upstream tests.

The inspected public register state already exposes the architectural
registers, I/R, IFF, IM, HALT flag, and `execEI`; no broader import/export API
is needed. Current vaeg has synchronous NMI and no pending-NMI caller or saved
condition, so no pending NMI state is required.

## M35 verified extension result

The focused extension was implemented and tested from the exact selected base
as local commit `b4a0a5a238fecc280781e6fe5719faf0eafcd667`. It adds
`setIRQLine`, `isIRQLineAsserted`, an interrupt-acknowledge callback for both
callback configurations, and normal-decoder IM0 injection. It retains the
one-shot behavior for existing users that do not opt into the new APIs. The
level line is inspectable and restorable, so M35 found no need for a broader
state API.

The MIT license blob is unchanged. M35 also corrected this ADR's former
copyright claim: this file previously said `2021-2023`, while the selected
base's `LICENSE.txt:3` and `z80.hpp:6` both say `Copyright (c) 2019 Yoji
Suzuki`. This documentary correction does not change the license decision.

The directly applicable
[M35 format patch](../reports/m35_suzukiplan_irq_extension.patch) is the
approved downstream provenance artifact. Its SHA-256 is
`d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4`.
The complete provenance, source references, and test record are in the
[M35 evidence report](../reports/m35_suzukiplan_irq_extension.md). A clean
`git am` from base `e3926769a790fab0af1c34a5540e317f8d4f0ddc` reproduced tested
commit `b4a0a5a238fecc280781e6fe5719faf0eafcd667`'s tree
`8a606eb39332a6e79b69bb62d9dedca042b923dc`; MIT verification passed.

M35 also found that the existing `test/` suite cannot be compiled wholesale
with `Z80_NO_FUNCTIONAL`: `test/test-checkreg-on-callback.cpp:88` passes a
capturing lambda to the function-pointer constructor. The exact selected base
fails identically before reaching any extension code. Focused extension tests
pass in both configurations, the complete existing `test/` suite passes in
its normal configuration, and the upstream ZEX harness passes in its declared
`Z80_NO_FUNCTIONAL` configuration. The maintainer accepted this matrix for
G35: baseline-incompatible legacy tests need not be rewritten when the
limitation is reproduced and documented.

## M36 verified vendoring and standalone conformance

M36 cloned the public upstream repository into a clean temporary checkout,
checked out base `e3926769a790fab0af1c34a5540e317f8d4f0ddc`, computed the approved
patch SHA-256, and applied it with `git am`. A plain application created a new
commit ID because Git used the current committer timestamp, but its tree was
the required `8a606eb39332a6e79b69bb62d9dedca042b923dc`. Repeating the clean
application with `git am --committer-date-is-author-date` reproduced historical
tested commit `b4a0a5a238fecc280781e6fe5719faf0eafcd667` exactly. Thus the approved
commit is reproducible when commit metadata is deterministic, while the tree
is the time-independent vendoring identity.

The only copied upstream files are `LICENSE.txt`, `z80.hpp`, and
`test/test-interrupt-extension.cpp`. Their Git blob IDs are respectively
`a4cbbf62b0edaf761ef48556c7a2e50bb3b4817f`,
`b4e1f9d357ff4076c9e4c2a9cb4b8ae64d273865`, and
`3708c77fabbfef85bd68da99fc7cef068af4ec55`. Byte comparisons against the
reproduced tree pass. `provenance.txt` is vaeg-authored provenance, not a
modified upstream source. It was initially named `VERSION`, but the first
hosted macOS build proved that this path shadows libc++'s `<version>` header on
the runner's case-insensitive filesystem: Apple Clang diagnosed the provenance
comments at the former `external/suzukiplan-z80/VERSION:3`, `:4`, and `:5` as
invalid preprocessing directives. Renaming only the vaeg-authored metadata
removed that collision without changing an upstream byte. The MIT license
remains byte-identical with SHA-256
`ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1`.

The vendored header is compiled only by standalone test targets. Four target-
local configurations cover normal callbacks, `Z80_NO_FUNCTIONAL`, the default
debug API, and release-oriented disabled debug/breakpoint/nest-check/exception
features. The focused 21-case interrupt extension runs in normal and
`Z80_NO_FUNCTIONAL` configurations. The raw core is not present in
`VAEG_CORE_SOURCES`, and the production emulator continues to compile
`cpucva/z80c.cpp` and `cpucva/z80diag.cpp`.

The vaeg-owned ZEX runner uses deterministic 64 KiB memory, loads at `0x0100`,
provides the required CP/M CALL-5 services, and enforces emulated-clock and
wall-clock limits. Its acquisition helper pins the approved base URLs and
verifies the two `.cim` files, two GPL `.src` files, and license. These external
inputs remain outside Git and release packages. Archive checks reject both
their expected names and their content hashes.

## M37 standalone wrapper and revision-1 codec result

M37 added the independently authored `z80_bus.h`, `z80_registers.h`,
`z80_legacy_state.*`, and `z80_core.*` components named above. The public
surface retains the used `Z80C` constructor, destructor, `Init`, `Exec`,
`Reset`, `IRQ`, `Wait`, status methods, `GetReg`, and placeholder `GetDiag`,
plus the approved architectural `GetPC`, `SetPC`, and `NMI`. `TestIntr`,
`GetWaits`, `IsIntr`, dump controls, and statistics were not reproduced. No
STL or third-party type crosses the consumer-visible headers.

The wrapper owns a public register mirror separate from the authoritative
third-party register state. `GetReg()` returns that mirror; `Exec()` refreshes
it only on return, while `GetPC()` and status save use the authoritative live
PC. A synthetic port callback at the production SLEEP_HACK PC proves that the
callback sees the old mirror PC while `GetPC()` sees the advanced PC. Memory,
I/O, level IRQ, acceptance acknowledge, and fine-grained clock callbacks are
adapted without a wrapper-side interrupt approximation. NMI remains the
synchronous vaeg operation and copies IFF1 to IFF2 before clearing IFF1.

The revision-1 codec is an explicit 68-byte little-endian representation. It
does not reinterpret an input buffer as runtime state. Its offsets remain AF
0, HL 4, DE 8, BC 12, IX 16, IY 20, SP 24, alternate pairs 28/32/36/40, PC 44,
I/R-low/R7/IM/IFF1/IFF2 at 48/49/50/51/52/53, IRQ 56, wait 57, `xf` 58,
revision 59, remaining clock 60, and last clock 64. All 15 retained M34 images
load. HALT load adds one to the legacy opcode PC and save subtracts one from
the authoritative post-HALT PC. Architectural AF is authoritative; load
ignores the legacy lazy-flag `xf`, and save writes deterministic F bits 3 and
5. The ordinary retained fixture therefore normalizes its historical `xf`
byte rather than reproducing uninitialized state.

Every retained fixture has wait bit 2 clear. A clear bit imports no EI
inhibition; a new boundary immediately after EI exports `execEI` in bit 2.
Reloading that image executes the required following instruction before an
asserted level IRQ is accepted. HALT, external WAIT, and EI occupy independent
bits 0, 1, and 2. The focused continuation test produces the same next
instruction, memory/I/O trace, architectural result, status image, and clock
balance before and after reload. IRQ assertion is inspectable in the image,
restored through the approved core setter, accepted as a level, and cleared
from later images only when vaeg deasserts it.

Two standalone libraries compile the wrapper with normal callbacks and
`Z80_NO_FUNCTIONAL`; both use target-local release-oriented core definitions.
The wrapper unit suite passes in both configurations. The separate wrapper
ZEX runner passes ZEXDOC and ZEXALL. These libraries are not members of
`VAEG_CORE_SOURCES`: the emulator still builds `cpucva/z80c.cpp`, and no
production selection or guest-visible path changed in M37.

## M38 differential result and passed gate

M38 uses three standalone processes: the legacy runner, the new-wrapper
runner, and a comparator. Both runners consume the same compiled scenario,
memory-patch, IRQ/NMI/WAIT, input, acknowledge, save/load, and clock-slice
descriptions. They emit `vaeg-z80-trace-v1` records with fixed lower-case
fields, full and selected memory hashes, ordered memory/I/O/acknowledge/NMI
events, callback-time live/public PC, authoritative state, clock balance, and
normalized checkpoint reasons. The comparator never assumes that an old
private step equals one new instruction; EI intermediate returns are retained
for diagnostics but compared only at the next declared normalized boundary.

The harness exposed third-party behaviors that the vaeg adapter can correct
without editing vendored bytes. The adapter now counts the second M1 fetch for
CB/ED/DD/FD prefixes, including raw IM0 prefixes, and counts HALT idle M1
cycles. It restores IFF1 from IFF2 after RETN/RETI and materializes the defined
S/Z/5/H/3/PV/N flag result after `LD A,I` and `LD A,R`. Its synchronous NMI
still updates architectural state immediately but retains the legacy stale
public PC until `Exec()` refreshes the mirror. These are instruction-boundary
adapter fields only; they are inactive at the approved frame save boundary
and are not added to revision-1 state.

The normal comparison has evidence-backed exact classifications for the
legacy HALT-PC/fetch representation, low/high stack write order, skipped
untaken-branch displacement read, lazy-`xf` exchange result, fused-EI
deassertion behavior, missing interrupt-acknowledge R increments, block
memory/I/O undocumented flags, and EI save-boundary event placement. The
complete identifiers and minimal bytes are in the
[M38 evidence report](../reports/m38_z80_differential.md). There is no broad
opcode allowlist and the generated corpus matches without any allowlist.

The slice-exact evidence remains unallowlisted. With bytes
`18 02 00 00 d3 f4`, initial A=`0x5a`, and slices `1,7`, legacy's 7-clock taken
JR reaches and writes FDD control port `0xf4` in the second `Exec()`; the new
core's architectural 12-clock JR leaves it at `0x0004` with no write. This
remains a reproducible FDD-visible slice assignment difference and is not
relabelled as externally inert.

The approved convergence continuation gives both runners the same additional
`4,1` clock slices. At 13 total clocks each trace contains the same ordered
reads and exactly one `OUT (0xf4),0x5a`; AF, BC, DE, HL, IX, IY, SP, PC,
alternate registers, I/R, interrupt state, memory hashes, device-visible
values, and `lastclock` agree. Only the classified architectural cycle count,
remaining balance, and earlier slice assignment differ.

G38 blocks on lost, duplicated, reordered, or permanently divergent external
effects. A slice-boundary shift caused by a verified architectural timing
correction may pass when eventual convergence is proven. Every such shift is
still a mandatory M39 private integration risk. M39 must stop production
integration if real VA/FDD testing finds a transfer failure, timeout,
corrupted data, changed IRQ/DRQ sequencing, or broken save/load behavior.
M38 adds no legacy per-opcode timing emulation. G38 passed and work remains
subject to the M39 private-system gate.

## M39 production integration

M39 adds one build-time CMake cache selection with exactly
`VAEG_Z80_CORE=legacy|suzukiplan`; `legacy` remains the default. The
production `vaeg_va` target compiles `cpucva/z80c.cpp` only for `legacy`, or
`cpucva/z80_compat_cpu.cpp` plus `cpucva/z80_compat_state.cpp` only for
`suzukiplan`. It never links both `Z80C` implementations, and there is no
runtime toggle. The vendored tree and approved M35 patch remain byte-unchanged.

The C subsystem seam and consumer signatures remain stable. The new selection
uses the same memory, eight-bit port, clock, level IRQ, and acceptance-time
acknowledge interfaces. Because M40 is not authorized, both selections still
compile the legacy `z80diag.cpp`; the new selection calls it through an
independent function-pointer memory bridge in a separate translation unit.
No M88 declaration, type, or disassembler object crosses the new wrapper's
consumer-visible interface.

The production state bridge now propagates `SaveStatus()` and `LoadStatus()`
failure through `machine/statsave.c`. A copied valid state with only embedded Z80
revision changed to 2 is rejected by the top-level load path. Successful load
also refreshes the subsystem's diagnostic WAIT mirror from revision-1 byte 57
bit 1; the restored CPU state remains authoritative. The status size, offsets,
HALT translation, external IRQ level, EI bit 2, and frame-call boundary are
unchanged.

The first M39 MinGW build of the suzukiplan production selection exposed a
portable-adapter defect: `iova/subsystem.cpp` still obtained the non-standard
`uint` alias transitively from the legacy header, while the independently
authored wrapper deliberately does not define that alias. The minimal
reproducer was the normal `mingw-cross` production build with
`VAEG_Z80_CORE=suzukiplan`, which failed before linking at the `Subsystem`
memory and I/O overrides. Those adapter signatures and helpers now use
`std::uint32_t`, matching `z80_bus.h`; both MinGW selections build and execute
their ROM-less integration selftests under Wine. No vendored or legacy-core
source was changed.

ROM-less tests under both production selections preserve the VA and Sorcerian
SLEEP_HACK constants. At the callback, live PC is after `IN (0xfe)` while the
public mirror still contains `0x1732` or `0x700e`; at `Exec()` return the
mirror catches up. WAIT drains clocks, ATN releases it, and execution resumes.
The focused synthetic EI-before-VA-IN case confirms the M38 scheduling
hypothesis under the new core without changing a magic constant. The tests
also require exactly one production `OUT (0xf4),0x5a` across a state boundary.

These results do not discharge the M38 timing risk. The committed private
manifest requires separate legacy/new VA/V3 boot, FDD read/write and
timing-sensitive loaders, IRQ/DRQ/`0xf4` evidence, actual VA and Sorcerian
sleep/wake paths, and legacy-to-new/new-to-new state during logical FDD
activity. Missing or reordered events, timeout, corruption, changed IRQ/DRQ,
failed sleep/wake, or altered completion after load blocks G39. The default
cannot change until the maintainer explicitly passes that gate.

The maintainer-private manifest was completed on 2026-07-15 and G39 passed.
Both production selections booted and completed OS, demo, reset, FDD
read/write, representative-loader, and timing-sensitive-loader cases. The
timing-sensitive trace had the same 21 ordered port-`0xf4` values and an exact
164-record common prefix of completed FDC commands. The longer legacy
wall-clock capture contained only additional post-convergence commands. No
loss, duplication, reordering, timeout, retry, or data mismatch was observed.
The diagnostic stream records DMA transfer metadata, IRQ, and acknowledge but
does not expose dedicated DRQ edges; no independent DRQ edge-count claim is
made.

Actual SLEEP_HACK traces preserved live/public PC `1734/1732` and `7010/700e`,
fixed-PC WAIT clock draining, ATN release, and resumed execution. Legacy-to-new
and new-to-new revision-1 states resumed ordinary and active-loader behavior.
Active external-WAIT states saved at live/public PC `7010/7010`, restored, and
woke through actual ATN under both cores. The task-permitted stable-idle
alternative was used for the private HALT/idle case; private HALT assertion is
not claimed. Tracked records use neutral asset identifiers and exclude private
filenames, absolute paths, hashes, screenshots, raw traces, saves, and media.

## M40 disassembler replacement

M40 selects an independently authored vaeg decoder rather than adding an
upstream-derived API. The approved vendored suzukiplan revision has
execution-time debug strings but no public side-effect-free decoding API.
`cpucva/upd780_disasm.h` and `cpucva/upd780_disasm.cpp` therefore implement the Z80
encoding structure directly as new BSD-2-Clause vaeg code, Copyright (c) 2026
Nakata Maho. No M88/cisc implementation or opcode table, GPL opcode table, or
other disassembler implementation/table was copied or adapted. No vendored
byte changed.

The decoder accepts a fixed-width memory-reader callback and explicit output
capacity and returns a 16-bit next PC, exact byte length, and status. STL use
is implementation-private. Both production CPU selections compile this one
decoder. `subsystem_disassemble_bounded()` is the capacity-aware C-facing seam;
the old `subsystem_disassemble()` signature remains as a 64-byte compatibility
adapter. Source inventory finds no active text parser and no interactive Z80
debugger in the portable SDL2 frontend. The frozen Win9x debugger caller is
unchanged.

Canonical output is lower case with fixed-width `0x` hexadecimal operands,
absolute wrapped relative targets, and signed IX/IY displacement syntax. Base,
CB, ED, DD, FD, DDCB, and FDCB pages include SLL, index-register halves, and
indexed-CB register-result forms. Reserved ED encodings use a deterministic
`db` representation with their actual consumed length. Redundant index
prefixes count toward length and ordered memory reads; the final DD/FD selects
IX/IY. A stream with no finite boundary is rejected after 32 consecutive
index prefixes with explicit prefix-limit status, zero length, and unchanged
next PC.

The reviewed golden data was generated from the new decoder and manually
checked against documented Z80 encodings, not against the old disassembler.
An independent test-side encoding classifier cross-checks lengths. The
deterministic exhaustive corpus contains 3,844 cases and covers all 256 base,
CB, ED, DD-following, and FD-following bytes plus all DDCB/FDCB final bytes at
five displacements, repeated prefixes, and wrap boundaries. Buffer tests cover
zero, one-byte, exact, truncated, large, null-reader, and prefix-limit output.

Production subsystem disassembly no longer calls `Z80C::GetDiag()` and no
active subsystem/debugger source includes `z80diag.h`, `z80if.h`, `z80.h`, or
`types.h`. The M39 `z80diag_bridge` is removed. M41 removed the legacy CPU and
its diagnostic object; `cpucva/upd780_disasm.cpp` is now the only production Z80
decoder.

## M41 final production design

The production `vaeg_va` target unconditionally compiles
`cpucva/z80_compat_cpu.cpp`, `cpucva/z80_compat_state.cpp`, and
`cpucva/upd780_disasm.cpp`. `VAEG_Z80_CORE` is removed; there is no compatibility
alias, hidden fallback, or runtime switch. The same standalone header,
interrupt, wrapper, revision-1, ZEX, disassembler, and M38-derived regression
tests remain permanent.

The seven approved M88/cisc-derived files are absent from current HEAD and
release inputs. Historical ADR, task, report, and migration-master references
remain as evidence and are not current build dependencies. Source and runtime
archive audits reject both the exact deleted paths and byte-identical copied
legacy source, fetched ZEX inputs, private media, and unapproved third-party
copies.

The consumer-visible class name and required methods remain unchanged. The
revision-1 image remains 68 bytes and all retained fixtures load. HALT PC
translation, external WAIT, level IRQ, EI bit 2, signed `remainclock`, and
`lastclock` retain their M37 mapping; top-level state load continues to
propagate codec failure.

## M101 downstream patches

M101 extends the vendored tree with four further downstream patches applied
on top of the M35 commit, following the same reproduce-then-copy procedure.
They are the commits of upstream pull requests
[#60](https://github.com/suzukiplan/z80/pull/60) and
[#61](https://github.com/suzukiplan/z80/pull/61), with the `test/Makefile`
conflict against M35 resolved:

| Patch | SHA-256 |
|---|---|
| [ADC/SBC HL carry fix](../reports/m101_suzukiplan_adc_sbc_carry.patch) | `233360c8763db8473604d699dbc30f5887659c84aeaf4ca3107e72e7059bd4d7` |
| [ADC/SBC HL flag test](../reports/m101_suzukiplan_adc_sbc_test.patch) | `f3829e5acce1d6e0faa22f102a8b6aa783ae793cbd39ee64da448cf3721bc67a` |
| [Flag profile](../reports/m101_suzukiplan_flag_profile.patch) | `4d5bedbb5f556b758debe95b2319323153db3c113b31bc74bca1fb103bbf3844` |
| [DAA/CPL/SCF/CCF rules](../reports/m101_suzukiplan_daa_cpl_scf_ccf.patch) | `73510c321e4dce29975298619fb34bcdd8d3e93a342888b5ec7b64a430589e83` |

`git am --committer-date-is-author-date` from
`b4a0a5a238fecc280781e6fe5719faf0eafcd667` reproduces commit
`6f34be621ae67a66d74485beca9f424f6ac314fd`, tree
`5ec540547bf9736baabe09e51d18542650d19714`. An earlier M101 revision named
the default profile `Upd780`; it was renamed because the NEC documentation
calls the uPD9002 Z80 mode itself "uPD780 mode", and because the uPD780C's
own undocumented flags are not known to match the Zilog behaviour.

- The carry fix corrects ADC/SBC HL,rr flags. The carry was folded into the
  16-bit operand before the flags were computed, so H was wrong whenever
  `rr & 0FFFh = 0FFFh` and C = 1, P/V whenever `rr & 7FFFh = 7FFFh` and
  C = 1, and C as well when rr = FFFFh.
- `Z80::FlagProfile` is per instance. `Zilog`, the default, keeps the
  existing Zilog behaviour and is used by the FDC CPU. Whether the FDC's NEC
  uPD780C matches it is unverified: the MAME 0.250 Z80 core
  (`src/devices/cpu/z80/z80.cpp`) notes that SCF/CCF take X/Y from
  `(F|A) & 28h` on SGS/Sharp/Zilog NMOS parts but from `F & A & 28h` on NEC
  NMOS parts, and that the exact NEC behaviour is still unknown.
- `Upd9002` implements rules R1–R11 of
  `docs/modernization/uPD9002-zex-results.md`, all measured on a real
  PC-88VA2 (see `docs/agents/reports/m101_zexall_qa/`), and is used by the
  uPD70008-compatible main-CPU mode.

## M102 undefined-opcode patch

The vendored core left 196 ED, 90 DD and 90 FD opcode-table entries empty.
vaeg builds it with `Z80_NO_EXCEPTION`, where executing such an opcode called
a null handler and crashed the emulator. M102 applies
[m102_suzukiplan_undefined_opcodes.patch](../reports/m102_suzukiplan_undefined_opcodes.patch)
(SHA-256 `d25676fee49329f8159b20ee6840b368f041971fd68b599a7222edd6a34489aa`,
upstream pull request [#63](https://github.com/suzukiplan/z80/pull/63)) on
top of the M101 result. With `Z80_NO_EXCEPTION` an undefined ED opcode is a
two-byte NOP, the NEG/RETN/IM duplicates act as the base instruction, and a
DD/FD prefix before an opcode without an index form is ignored. The default
configuration still throws.

M102 then adds R12/R13 of the `Upd9002` profile,
[m102_suzukiplan_ddcb_register_forms.patch](../reports/m102_suzukiplan_ddcb_register_forms.patch)
(SHA-256 `2f68c16a32405d0eb71a8697ced7ebff7ff6d5c5b5fc1289be9c4376cfb99a70`,
part of upstream pull request #61): DD/FD CB d xx BIT/RES/SET forms with a
register operand act on that register only, as measured on a real PC-88VA2.
`git am --committer-date-is-author-date` from
`b4a0a5a238fecc280781e6fe5719faf0eafcd667` with all M101 patches and both
M102 patches reproduces commit `60b725f129979c4179fa6e08b6385d20fd69f85e`,
tree `37e3d5e6130ca12d5dbf41904bd65d567817bff0`.

## M102 rebase on upstream 1.11.0

Upstream merged pull requests #60 and #61 and added its own changes (#62;
CHANGELOG version 1.11.0, master
`ab97d3fa9eedc4914f82f3d9bc2abf33984d984d`). #61 was merged at its first
commit (R1–R7) because GitHub did not pick up two later pushes, so R8–R11
and R12/R13 are offered again as
[#64](https://github.com/suzukiplan/z80/pull/64); #63 (undefined opcodes)
was rebased and remains open. The upstream changes include a WZ update in
`OP_IX4`/`OP_IY4`, so `BIT n,(IX+d)` and `BIT n,(IY+d)` take X/Y from the
effective address under the Zilog profile.

The approved base therefore moves to `ab97d3f`. The vendored tree is that
base plus, in order, the two #64 commits, the #63 commit and the M35 IRQ
extension rebased onto the base (its `test/Makefile` conflict resolved):

| Patch | SHA-256 |
|---|---|
| [R8–R11](../reports/m102_upstream_upd9002_daa_cpl_scf_ccf.patch) | `0731306a6a0b6df14777f070f0194ecba345d929f2e37e06db00422db62beb07` |
| [R12/R13](../reports/m102_upstream_upd9002_ddcb_register_forms.patch) | `8eb861d3f79508af85e8b21d5993cb065ec295b561ae6da993dd5a36188ed9dd` |
| [Undefined opcodes (#63)](../reports/m102_upstream_undefined_opcodes.patch) | `4d622e1cd327e58e45b7614e8cfd2ecd95ab01a876c2562b9ea1392cadc54ab9` |
| [M35 IRQ extension on 1.11](../reports/m102_m35_irq_extension_on_1_11.patch) | `76e50ec777c1f6117f95faa2455d5625046329e778f183f4786c0d362a7b9ce8` |
| [uPD9002 undefined ED semantics](../reports/m102_upd9002_undefined_ed_semantics.patch) | `678746bb8a7ae69f0d92cb959776839f6016bbcc70fff53d2a9dc91bc88c01cb` |

`git am --committer-date-is-author-date` from `ab97d3f` reproduces commit
`3ce76465df732e38aab5b3f31642a3a5a7351872`, tree
`b0eaa360c215f6a7ab64ad0d757a57c5339ffdc7`. Compared with the previous
vendored `z80.hpp`, the only change is the upstream WZ update. The M35 IRQ
extension stays downstream.

## Frame-boundary revision-1 state

Production save is initiated by the GUI only after `pccore_exec()` returns.
At the Z80 section callback, no `Z80C::Exec()` or Z80 callback is active and a
complete instruction boundary has been reached. Preserve that scheduler; do
not serialize instruction- or callback-internal microstate.

The same-build-family revision-1 target is the existing 68-byte image on the
verified little-endian 64-bit GCC/Clang/MinGW-family layouts. Decode and encode
it by explicit offsets. Map architectural registers, I/R, IM, IFF1/IFF2,
HALT, external WAIT, IRQ level, `remainclock`, and `lastclock`. Translate HALT
PC explicitly: the legacy core stores the HALT opcode address, while the
selected core's PC is after HALT. Legacy-to-new adds one modulo 16 bits;
new-to-revision-1 subtracts one while halted.

The selected core can return from its execution call with `execEI` set when
the slice ends immediately after EI. The legacy image has no named EI field,
but its saved `wait` byte uses only bits 0 (HALT) and 1 (external WAIT). Reserve
bit 2 in new-core revision-1 images for reachable EI inhibition. Old fixtures
have it clear; new-to-new round trips retain it. This does not promise that a
new-core image loads in an old release. M37 must prove this focused mapping on
every supported build family before integration.

The legacy implementation never returns positive `remainclock` from normal
`Exec()` or its external-WAIT branch with the current positive multiplier: it
runs or drains until the value is non-positive. Zero and negative values are
production-reachable and must round trip. This corrects the contrary
positive/zero/negative assertion in the M34 task and migration master; it does
not authorize a scheduler change. A codec should still preserve any signed
32-bit input byte-for-byte, including positive values, for defensive legacy
compatibility tests.

## ZEX acquisition

Amended by [ADR-0015](ADR-0015-zex-sources-for-cpmva.md) in M101: the
upstream `.src` files and bundled license are tracked under `external/zex/`
for user-built CP/MVA disks. Release archives remain free of ZEX material.

Use the candidate commit's `test-ex` artifacts only in dedicated conformance
CI or from an offline user cache. Fetch by immutable raw GitHub URL, verify
SHA-256, and never put the fetched artifacts in source or release archives.
Record and make available the corresponding source and license:

| File | SHA-256 |
|---|---|
| `zexdoc.cim` | `b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90` |
| `zexall.cim` | `fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0` |
| `zexdoc.src` | `0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3` |
| `zexall.src` | `a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c` |
| `test-ex/LICENSE.txt` | `e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052` |

The `.src` notices identify Frank D. Cringle (1994), modifications by J.G.
Harston (2002), and GPL-2.0-or-later. The bundled license text is GPLv3; retain
both notices in acquisition metadata. These programs are executed as separate
test inputs, not linked into vaeg. The policy is lawful and reproducible.

## Disassembler and release policy

The selected revision has execution-time debug strings but no public,
side-effect-free reusable decoder. M40 will independently author a vaeg BSD-2-
Clause disassembler from documented Z80 encodings. It must not copy M88/cisc
or GPL opcode tables.

During M39 and M40 only, explicit build selection may keep legacy and new cores
for differential and private-system evidence; legacy remains the default.
M41 has no fallback: the new core becomes the only production path, the build
switch is removed, the seven approved files are deleted, and source/release
archives are audited for their absence. Historical Git objects are out of
scope.

## M35 feasibility recommendation (historical G35 output)

**G35 PASSED; M36 WAS FEASIBLE.** The focused extension, approved downstream
patch provenance, license verification, and accepted test matrix met G35. The
maintainer subsequently authorized M36; this paragraph is retained as the
historical G35 recommendation rather than a current authorization statement.

| Area | Finding |
|---|---|
| Interrupt contract | Feasible only with the focused level-line and acceptance callback extension. |
| State import/export | Feasible from public register state plus restorable level line; no broad state API is needed. |
| Revision-1 save | Feasible at the verified frame boundary with an explicit 68-byte codec, HALT-PC translation, and reserved EI bit. |
| Practical IM0 | Feasible by routing the supplied first byte through the normal decoder; current upstream RST-only handling is insufficient. |
| Disassembler | Feasible as an independent BSD-2-Clause vaeg component in M40. |
| Licensing/provenance | MIT base plus the approved hash-verified downstream patch and BSD-2-Clause vaeg adapter are compatible and precisely reproducible. |
| Test acquisition | ZEXDOC/ZEXALL have an immutable, hash-verified GPL test-input path with source and offline cache support. |

## Consequences and unresolved risks

- The ordinary ASan/UBSan matrix passes. An additional sanitizer-plus-ZEX
  experiment is too slow for the ZEX runner's 600-second wall limit and emits
  signed-left-shift reports at immutable vendored `z80.hpp` lines 3000 and
  3885. Normal GCC, Clang, and Wine raw/wrapper ZEX pass. This combined-mode
  limitation is recorded rather than changing the approved vendor tree.
- `xf` is not initialized by legacy `Reset()` but influences saved AF and is
  serialized. Fixtures deliberately execute `XOR A` first. This is a legacy
  defect, not a behavior contract; it remains open and is not fixed by M34.
- Genuine revision-1 images are compiler-shaped. Local native GCC/Clang and
  cross-target layout probes agree, but Windows and macOS CI must execute the
  fixture target before M37 freezes per-family codecs.
- Actual private-ROM IM0 acceptance bytes were not required by the observed
  private scenarios. Optional tracing still logs them only at real acceptance;
  static code permits every byte.
- ROM-less and private M39 tests preserve SLEEP_HACK's stale callback-time
  `GetReg()->pc` mirror, both constants, clock drain, and ATN wake.
- The M34 task/master claim that positive remaining credit can be present
  after `Exec()` conflicts with current code; the verified zero/negative
  boundary governs implementation.
- The migration master says M33 is in the roadmap, but current `ROADMAP.md`
  has no M33 row. M33 commits exist in history. M34 leaves that completed work
  and identifier untouched rather than inventing an entry.

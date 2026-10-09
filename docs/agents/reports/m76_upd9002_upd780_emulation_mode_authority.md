<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M76 — uPD9002/uPD780 emulation-mode authority audit

Status: G76-approved Stage 1 implementation. The original audit was
expanded by explicit user approval to implement the uPD9002 compatible mode
as Z80, while excluding BRKEM2, full VA I/O-trap semantics, and FDC/SCSI
changes.

Evaluated branch: topic/m76-brkem-z80-emulation

Evaluated base: ec829cd02bb11202d9172ae9066d37c14c46202c

## Decision

The user-facing statement is correct: a V30 can use its 8080 emulation mode.
The correct CP/M evidence is Vector's separate MS-DOS CP/M emulator v0.8,
whose .cpv path performs a V30 probe and emits BRKEM, CALLN, and RETEM; the
earlier Vector Win32 v0.4 package was the wrong artifact. This establishes a
bounded V30 transition contract, not the VA's full uPD780/Z80 mode. At the
evaluated pre-implementation base the active main CPU had no mode latch,
compatible register state, decoder, or transition support. G76 approval then
amended this work to a bounded Z80 Stage 1 implementation. VA ROM evidence
still reaches 0F FE 90 (BRKEM2), and the exact BRKEM2 frame, mode-latch rule,
interrupt behavior, I/O boundary, and silicon-specific timing remain
unverified. Those items stay outside Stage 1 and require separate evidence.

## CP/M v0.8 identity and limits

| item | identity |
|---|---|
| Vector page | https://www.vector.co.jp/soft/dos/util/se000015.html |
| archive | https://ftp.vector.co.jp/58/06/545/cpm08.zip |
| archive SHA-256 | 691e51dda202ab97b7c8c947ca7c9bf2d93d822f3e315362fcc7840199b8d6f7 |
| CPM.ASM SHA-256 | 1172892475ed0852dc00795ef0af117c9ba120bc31d339c4b65e08631c5115f0 |
| CPM.EXE SHA-256 | d9e8ba8e5322bd037186ef11da4f980c4d5999affce3ad4201221ca721947040 |
| EM180.ASM SHA-256 | 1e4290565eb3aa57b4213c39a75ffba7f268d5cf2cb79218e401399dd7c96c3c |
| CPM08.TXT SHA-256 | 870eca29949bc026e2048153bba8c032d6cf85ae714a43c5d5b32e6cb91b25b1 |

The source and documentation are CP932 in the archive; the audit converted
them to temporary UTF-8. No downloaded binary or CP932 source is committed.
No DOS runtime is installed on this macOS worker, so this is static
source/binary evidence, not an emulator execution trace.

The task file named the separate Vector Win32 page
https://www.vector.co.jp/soft/win95/util/se378130.html. That v0.4 artifact
was checked first and has no .cpv, D5 00, BRKEM, CALLN, or RETEM path in its
audited source/binary. It is not the correct evidence package.

The operator also supplied a docs/cpmva/ corpus in the dirty working tree.
Its CPMVA source and generated artifacts cross-check the existing CPMVA
claims: CPMVA.H has the 0F FF macro, V30.MAC has ED ED and ED FD macros,
CPMBIOS.MAC is .z80 and calls CALLN, and EXIT.MAC ends with RETEM. Those
binary payloads are not copied or committed by M76 because repository policy
prohibits adding binary payloads.

## Corrected .cpv transition audit

CPM08.TXT lines 102–111 state that .cpv uses V30 8080 emulation when
possible, claims a 4–8x speedup, and falls back to software emulation on
80186 or later CPUs. Lines 211–225 identify ordinary IVT entries F1/F2/F3
for program start, BDOS, and BIOS and state that they are relocatable.

| source | observed contract |
|---|---|
| CPM.ASM:208–225 | tries .cpm, .com, then .cpv; only .cpv reaches hard path |
| CPM.ASM:228–230 | AX=0100h; D5 00 (AAD 0); JZ soft_emu. V30 is expected to produce AX=000Ah and ZF=0 |
| CPM.ASM:243–259 | saves and installs ordinary DOS IVT entries F1/F2/F3 |
| CPM.ASM:262–264 | sets BP to the emulated 8080 stack value and emits 0F FF F1 (BRKEM F1h) |
| CPM.ASM:54–60 | CALLN is ED ED imm8; RETEM is ED FD |
| CPM.ASM:386–419 | native BDOS handler is far and returns with IRET |
| CPM.ASM:584–601 | native BIOS handler consumes DS:[BP] and returns with IRET |
| CPM.ASM:675–702 | generated compatible image contains CALLN F2/F3 and RETEM |
| EM180.ASM:1603–1627 | software fallback mirrors CALLN with PUSHF, far handler, IRET, and RETEM |

This answers the user's V30 question directly: yes, the .cpv route is
designed to use the V30's 8080 emulation mode. It does not show that the
current vaeg uPD9002 core can execute it.

Known exerciser defects are recorded: restor_vct uses int_bdos_ptr again for
the BIOS restore (CPM.ASM:267–276), and unused incsp emits 3Eh rather than
8080 INC SP 33h (CPM.ASM:62–64). Neither changes the hard-path entry
contract.

## Current uPD9002 implementation boundary

| area | source evidence at evaluated base | result |
|---|---|---|
| mode state | cpu/upd9002/cpucore.h:27–44, 146–210 has native flags/MSW and fixed native register/state images, but no MD latch, compatible register file, or alternate state | absent |
| dispatch | cpu/upd9002/upd9002_core.c:184–235 calls one upd9002op table; reset initializes CPUTYPE_V30 at :155–159 and :237–247 | native-only |
| 0F FE/FF | cpu/upd9002/upd9002_mn.c:3629–3753 routes high 0F second bytes to _reserved_0x0f | no BRKEM2/BRKEM |
| ED | native ED maps to _in_ax_dx | no native-mode CALLN/RETEM |
| interrupts | upd9002_core.c:306–356 always pushes native flags/CS/IP and loads ordinary IVT | no mode-aware entry |
| save state | upd9002_state.h:32–39 fixes a 112-byte version 1; state.c:135–187 accepts V30 and rejects protected mode | no compatible state |
| trace | upd9002_trace.h:39–51 has step/event tracing but no mode, decoder, frame, or compatible-interrupt fields | later trace contract |

The separate FDD Z80 wrapper in cpucva/z80_compat_cpu.cpp is not main-CPU
compatible-mode support and must not be reused as a hidden implementation.

## VA target evidence and V20/V30 limits

- The VA1/VA2 provenance documents identify ROM 0F FE 90 at a valid
  instruction boundary, vector 90h to 1000:0000, and the following native
  resume CLI. The V1/V2 fallback reaches BRKEM2 after display and I/O-trap
  setup. This is main-CPU evidence, not FDD-Z80 evidence.
- Debug 8800 evidence contains Z80 JR, LDIR, alternate-register, and IX/IY
  code and records real PC-88VA V1/V2 execution. It is strong target evidence
  for a Z80-class compatible decoder, but private ROM/hardware execution is
  not independently rerunnable from this clean public clone.
- M65g records zero executable BRKEM corpus cases and leaves BRKEM/RETEM/CALLN
  untouched. The public M75 report concerns SCSI rather than main-CPU mode.

V20/V30 manuals and MAME are useful analogies for standard BRKEM/CALLN/RETEM
frames, MD write-enable behavior, and separate decode tables. They do not
prove BRKEM2, the VA Z80 superset, IX/IY sharing, Z80 interrupt modes,
V1/V2 I/O-port translation, or uPD9002 timing.

## Authority decision

| question | decision | action |
|---|---|---|
| V30 8080 hard-emulation path exists? | proven by CP/M v0.8 .cpv | use as transition exerciser |
| active vaeg main CPU implements Stage 1? | yes, on this branch | review Stage 2 gaps below |
| VA firmware attempts compatible entry? | yes, ROM 0F FE 90 evidence | BRKEM2 tests after mode state |
| VA compatible decoder is Z80-class? | strong target evidence plus Stage 1 Z80 execution | preserve ROM/Debug 8800 obligations |
| exact uPD9002/BRKEM2 semantics known? | no | hardware/manual flip points |
| broad generic V20/V30/Z80 implementation authorized? | no | this is bounded Z80 Stage 1 only |

Disposition: the approved Stage 1 transition boundary is implemented. The
remaining VA-specific behavior stays hardware-bounded and is not treated as
complete Z80 silicon emulation.

## Remaining implementation obligations

1. Extend the explicit MD mode latch and decoder selector only as required by
   additional proven VA behavior; keep it separate from the FDD Z80.
2. Extend compatible register aliases, alternate registers, and flags only
   where the target software or hardware evidence requires it.
3. Add further tests for the implemented BRKEM/CALLN/RETEM frame and for
   save/import while native code has suspended compatible state.
4. Keep 0F FE imm8 BRKEM2 behind an authority decision until its frame/latch
   behavior is established; do not silently alias it to BRKEM.
5. Specify compatible interrupt, NMI, RESET, HALT, nesting, and prefetch
   behavior, with synchronous CALLN and asynchronous-entry tests.
6. Resolve port 153h and compatible-mode I/O trapping without moving FDC or
   SCSI code into the CPU core.
7. Version save state and migrate old 112-byte native saves with a documented
   native-mode default.
8. Trace mode before/after, decoder, vector, saved/restored frame, interrupt
   source, and reset/HALT events.
9. Add production-path round-trip, flags/BP/register, CALLN/IRET, RETEM,
   BRKEM2, interrupt/HALT/RESET, save/import, and ROMless tests. Use .cpv as
   a transition fixture, not a Z80/VA instruction oracle.

## G76-approved Stage 1 implementation

The user approved G76 and then amended the implementation target from the
V30-style 8080 boundary to the VA's Z80-compatible mode. The implementation
uses the existing suzukiplan Z80 core through an independent main-CPU adapter
under `cpucva/`; it does not reuse the FDD CPU instance.

Implemented production boundaries:

- `0F FF imm8` enters compatible mode and pushes PSW, PS, and the
  post-instruction PC on native `SS:SP`.
- Compatible execution uses suzukiplan Z80 instructions, including tested
  `JR`, `IX`, and `IY`; the main-register aliases are synchronized at the
  existing uPD9002 boundary.
- `ED ED imm8` enters native code, and native `IRET` resumes the suspended
  compatible instance. `ED FD` restores the native frame.
- The compatible core state is persisted in the versioned 68-byte `UPD9Z80`
  save-state section.
- Native `IRET` does not use PSW bit 15 as a software marker; a separate
  runtime pending flag avoids confusing the architectural native PSW with a
  compatibility return.

The ROMless production-path regression
`vaeg_upd9002_brkem_upd70008` passed and covers BRKEM, uPD70008-compatible JR/IX/IY execution,
CALLN/IRET, RETEM, register aliases, and stack restoration. Existing uPD9002
IRET, state-boundary, state-payload, FDD-Z80 wrapper, and differential Z80
regressions also passed after the change.

Explicitly not implemented in this stage: `0F FE imm8` (`BRKEM2`), complete
VA-compatible I/O-trap behavior, silicon-specific interrupt/timing details,
and any FDC/SCSI behavior change.

## Gate

G76 approval authorizes this Stage 1 implementation. The branch must still
pass the repository checks and the standard human gate before it is merged to
`main`. Do not start M77.

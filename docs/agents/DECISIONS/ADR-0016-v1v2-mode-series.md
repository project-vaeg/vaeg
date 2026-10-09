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
# ADR-0016: The V1/V2-mode series and its parallel V3 work

## Status

Accepted by the maintainer on 2026-10-02.

## Context

The PC-88VA's V1/V2 modes run the µPD9002 in its Z80 emulation mode on the
VA chipset in PC-8801 compatibility decoding. M76 through M102 built and
validated the CPU side of that mode (the uPD70008-compatible main-CPU
adapter, flag rules R1–R11, the undocumented and undefined opcode set
R12–R19, CALLN/RETEM and the live IVT read). The machine side — mode
register, 8801-style memory banking, text CRTC, keyboard matrix, display
pipeline in compatibility mode — does not exist in vaeg yet.

This is far more work than one milestone, and it must not block unrelated
V3-mode work (SGP and display features), which the maintainer wants to run
in separate sessions at the same time.

One usable behavioural reference exists: X88000 by Manuke
(<https://quagma.sakura.ne.jp/manuke/x88000.html>), a PC-8801-series
emulator released into the public domain. It emulates a PC-8801, not a
PC-88VA in compatibility mode.

## Decision

### Two milestone series, two worktrees

- **V1/V2 series: `M103a`, `M103b`, … (lettered).** Work happens on the
  integration branch `topic/v1v2-mode`; each lettered milestone is its own
  task file, its own branch off the integration branch, its own human gate,
  and is merged to `main` as soon as its gate passes. Until the mode is
  usable, every V1/V2 code path stays behind a build-time or configuration
  feature flag so that `main` remains shippable at every merge.
- **V3 series: `M104`, `M105`, … (integer).** Ordinary one-milestone
  branches off `main`, as before. The first reserved identifier, `M104`,
  is for SGP work.
- The two series use separate worktrees and must not edit the same
  subsystem in the same milestone pair. Shared infrastructure (the register
  model, state-save sections, the main-CPU adapter) is landed on `main`
  first — `M103a` is such a milestone — and both series rebase from there.
  A series that lets `main` advance underneath it takes `main` at the start
  of its next milestone.

### Priority and gates

- **V2 first** (PC-8801mkIISR level). V1 follows once V2 is usable.
- The V1/V2 series is complete when, on a PC-88VA2 configuration in vaeg:
  1. **N88-DISK BASIC V2** boots from a user-supplied disk image and
     accepts keyboard input and disk commands, and
  2. the **PC-8801mkIISR DEMO** runs.
  Each lettered milestone defines a smaller observable step toward these
  (for example Debug 8800's banner, the `Ok` prompt, keyboard echo, the
  first graphic frame).

### How X88000 is used

- As a **behavioural reference for the PC-8801 side** (memory banking
  ports, µPD3301 text CRTC, 8255 keyboard matrix, graphic VRAM layout and
  timing), not as code to transplant. The VA's compatibility mode is the
  VA chipset with 8801 decoding (`upd9002-upd70008-mode.md` §8), so vaeg's
  implementation follows the VA's own compatibility layer and the VA ROMs,
  and uses X88000 to check what 8801 software expects.
- Where a routine is nevertheless derived from X88000, the file header
  states the origin, the X88000 version and the public-domain status, and
  the deriving milestone records the exact source file and revision here
  under "Derivations". No X88000 file is vendored under `external/` unless
  a later ADR approves one.
- The 8801-side ROMs needed by V1/V2 mode (N88-BASIC, Debug 8800, the
  sub-ROMs) are already in the VA ROM set (`upd9002-upd70008-mode.md`
  §4.4–§4.6). No additional ROM image is introduced.

### Derivations

Re-implemented from X88000 1.5.3 (public domain; source archive
`x88_1_5_3_src.tar.gz`, SHA-256
`66bc0b69d5a394e2195b5e277093eebcc045fbb3930c03637aa106394ae83cf3`,
<https://quagma.sakura.ne.jp/manuke/x88src.html>); no code copied:

| Milestone | vaeg code | X88000 source | What |
|---|---|---|---|
| M103c | `vram/maketextva.c` (`makeline_3301`) | `X88ScreenDrawer.cpp`, transparent attribute mode | µPD3301 attribute-pair walk and carry across rows |
| M103d | `memoryva/memoryva.c` (`n88_gvram_ex_read`/`_write`) | `PC88Z80Main.cpp`, `ReadMemoryGVRamEx`, `WriteMemoryGVRamEx*` | GVRAM extended access: latch, comparison read, ALU and copy modes |
| M103d | `vram/scrndrawva.c` (multiplane 1 bit/pixel merge) | `X88ScreenDrawer.cpp`, monochrome modes | Colour and reverse rule for graphics dots (the merge itself follows the VA technical manual) |
| M103e | `io/memctrlva.c` (`E2h`/`E3h`) | `PC88Z80Main.cpp`, `ReadIO_E2`/`WriteIO_E2`/`ReadIO_E3`/`WriteIO_E3` | Port numbering and bit layout of extended RAM control |

Each choice and its status is listed in
[`v1v2-mode-decisions.md`](../../modernization/v1v2-mode-decisions.md).

## Notes on the implementation (2026-10-04)

- **No feature flag.** M103b–M103f did not put the V1/V2 paths behind a
  build or configuration flag. They run only when the VA ROM selects V1/V2
  from the boot media; V3 software never enables them (port 153H bit 6 and
  the I/O trap are set only on that path; the 8214 interrupt mode is the
  reset default, which V3 software leaves by writing 158H), and each gate
  rechecks V3 boot and the bundled demo. The maintainer accepted this
  (no flag) on 2026-10-04.
- **Series gate.** N88-DISK BASIC V2 operates (M103d) and the
  PC-8801mkIISR DEMO runs (M103e); both await G103d/G103e. V1 mode has not
  been run (no V1 media).

## Consequences

- `docs/agents/ROADMAP.md` lists both series. A session reads only the
  next task file of its own series.
- `main` may contain partial V1/V2 code behind a flag. Releases do not
  advertise the mode until the series gate passes.
- Conflicts between the series are resolved by landing the shared piece on
  `main` as its own lettered or integer milestone, never by cross-merging
  topic branches.

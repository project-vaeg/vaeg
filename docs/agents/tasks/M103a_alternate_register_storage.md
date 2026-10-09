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

# M103a - Machine-owned alternate register storage

Status: **complete; G103a passed on 2026-10-03**

Series: V1/V2 mode (ADR-0016). First milestone of the series; it lands on
`main` before `topic/v1v2-mode` is created, because the V3 series depends
on the same register model.

Predecessor: G102 passed; M102 merged to `main` at
`659a01e16f5e0c7b79e314b5cbdc0a3e12cddcca`.

Branch: `topic/m103a-alt-registers` (off `main`)

Commit prefix: `M103a:`

## Background

`docs/modernization/upd9002-upd70008-mode.md` §3.1 and §11-4: the µPD9002
carries eight bytes of register storage that native mode cannot address —
`AF'`, `BC'`, `DE'`, `HL'`. A real-machine dump shows the alternate set
holding values distinct from the main set, and the V30 register file has no
room for them. The main set, `IX`/`IY` and the compatible-mode `SP` are
aliases of the native file (`AL`, `CX`, `DX`, `BX`, `SI`, `DI`, `BP`, `IP`).

In vaeg today (`cpu/upd9002_upd70008.cpp`):

- the main set is copied between the native core (`CPU_*`) and the
  suzukiplan core's `reg.pair` at `Enter()`, `Resume()` and
  `SyncToNative()` — a mirror, not an alias, but with a working sync
  discipline;
- the alternate set exists **only** inside the suzukiplan core
  (`reg.back`). It is absent from the native register model, invisible to
  the debugger and GUI register views, persisted only inside the opaque
  `UPD9Z80` compat section, and discarded whenever `Reset()` clears
  `have_compatible_state_` and the next `Enter()` calls `SetReg()`.

V1/V2 mode will run compatible-mode code for long stretches with frequent
CALLN round trips, mode transitions, resets and state saves, so the
alternate set needs a definite owner before that work starts.

A defect found while closing M102 belongs here as well: `StateLoad()`
initialises the compat core when a state is loaded before any `Enter()`,
but does not install `SetNativeVectorRead`, so the undefined ED 00–3F read
page 0 as zeros until the next mode entry.

## Scope

1. **Machine-owned alternate storage.** Add `alt_af`, `alt_bc`, `alt_de`,
   `alt_hl` (eight bytes) to the native uPD9002 state owned by
   `cpu/upd9002/` (the `UPD9CPU` state image, with a version step per the
   M85 state-save conventions and a loader that accepts the previous
   version with zeroed alternates). The adapter copies them in and out
   together with the main set at `Enter()`, `Resume()` and `SyncToNative()`;
   the suzukiplan core keeps using `reg.back` as its working copy.
2. **Lifetime policy**, written into `upd9002-upd70008-mode.md` §11 and
   implemented: the alternate set survives CALLN/RETI round trips,
   RETEM→BRKEM re-entry and adapter `Reset()`; it is cleared only by the
   machine's hardware reset path (`upd9002_state_reset`). The unmeasurable
   reset case is a policy, tagged `[DERIVED]`, not a measurement.
3. **Real-machine probe `ALTPRB`** (CP/M program under CP/MVA, built by
   `tools/cpmva/zex/zexbuild.py`, output `ALTPRB.TXT` in the probe style of
   M102): load known patterns into the alternate set and different patterns
   into the main set, perform a `CALLN 91h` V3 memory read (§7.1), then
   record both sets and F before and after. This measures whether the
   alternate set and the main set and flags survive a genuine native round
   trip (§3.1 `alt-regs` persistence, and the §3.2/§17.6 `flag-callback`
   anomaly). The CP/M host runner cannot execute CALLN; `ALTPRB` is
   real-machine-only like `INPRB`.
4. **`StateLoad` fix**: install the native vector reader on the
   load-before-enter path; add a wrapper or adapter test that loads a state
   first and executes `ED 00`. Ledger entry.
5. **Visibility**: expose the alternate set in the existing debugger
   register dump (and the GUI register view if one exists for the main
   CPU), read-only.
6. Tests: wrapper contract test for the adapter copy-in/copy-out and the
   lifetime policy (Reset preserves, hardware reset clears); state-save
   round trip including the new fields and the old-version load path.

Out of scope: the V1/V2 machine side (M103b onward), changes to the
vendored core, any change to the main-set mirror design.

## Deliverables

- `cpu/upd9002/` state image extension and `cpu/upd9002_upd70008.cpp`
  copy-in/copy-out.
- `tools/cpmva/zex/altprb.asm`, parser and `compare.py` section, installer
  program-set and disk-budget updates, `docs/cpmva-setup.md` row.
- Real-machine evidence under `docs/agents/reports/m103a_altregs_qa/`
  (same layout as `m102_zexund_qa`), with the measured persistence recorded
  in `upd9002-upd70008-mode.md` §3.1/§17 as `[MEAS]` and §15.2 `alt-regs`
  and `flag-callback` updated.
- `docs/modernization/bug-fixes.md` entries for the `StateLoad` reader
  omission and, if the probe shows one, any persistence defect.
- Task-file progress log; ROADMAP status.

## Machine checks

- `cmake --build --preset linux-ci-gcc` and `ctest` (all green).
- `tools/repo/check_case.py`, `check_encoding.py --expect utf8`,
  `check_eol.py --enforce`, `tools/qa/milestone_ids.py --audit`.
- `tools/cpmva/zex/host_reference.py` unchanged outputs for every existing
  program (ALTPRB excluded from the host run).

## Gate G103a (human)

Standard gate (clean build, V3 boot, VA demo, OS boot and simple
operations) plus:

- `ALTPRB` on the real PC-88VA2 and inside vaeg report the same alternate
  and main register values across the CALLN round trip (or the difference
  is explained and fixed in this milestone);
- a state saved while in compatible mode with distinct alternate values
  restores them, and a state file from before this milestone still loads.

## Progress

- 2026-10-02: task created (ADR-0016).
- 2026-10-03: first G103a run by the maintainer with the static Windows
  build of `33f268cc5487e4feffdde75bcb495c4657cdcf94` (`vaeg.exe` SHA-256
  `999b702aa4095d517531bda007b23a9438884e6b17c4359c48986e87532c0b8e`):
  standard gate passed; ALTPRB results returned from the real PC-88VA2 and
  from vaeg; saving at the CP/MVA `A>` prompt and loading hung and then
  crashed; loading a state saved by the M102 build failed with a disk
  error and crashed.

### Compatible-mode state save/load

- Reproduced headless on Linux at `33f268cc` and also at the M102 merge
  `659a01e16f5e0c7b79e314b5cbdc0a3e12cddcca`, so it predates M103a. A
  local diagnostic (frame-triggered save/load using the GUI's sequence,
  not committed) booted the PC-Engine 1.1 system disk with the CP/MVA
  boot disk in FD2, ran CP/MVA from B:, saved at `A>` and loaded. Saving
  and loading at the native PC-Engine prompt worked.
- At `A>` the CPU is usually inside a native BIOS handler entered by
  `CALLN`: native mode with the return-pending flag set (`F000:2B33` in
  the reproduction). Demonstrated cause: the mode and return-pending
  flag live in the two UPD9CPU padding bytes, which export/import
  deliberately skip, and the return SP is a core static that was never
  saved. After loading, the flag was clear, so the UPD9Z80 blob was not
  loaded either and the handler's IRET returned into the Z80 code as
  native code. A same-window comparison showed the handler IRET resuming
  `1FC14h` before saving but `32D75h` after loading, followed by a jump
  to `0000:108Fh` and an `OUT A2h`.
- Correction: new optional section `UPD9MODE`, version 0, 4 bytes (mode,
  return-pending flag, return SP), saved after UPD9CPU and before
  UPD9ALT/UPD9Z80; invalid combinations are rejected. A file without it
  whose UPD9Z80 payload is nonzero was saved while compatible code was
  active and cannot be resumed: `statsave_check` now refuses it with
  "saved in compatible mode by an older build; it cannot be resumed"
  instead of loading it and running Z80 code as native code. Files saved
  in native mode without the section load as before.
- Verification: the CP/MVA reproduction now lists the directory after
  loading, in the same process and in a fresh process; the M102-era state
  is refused with the message; a native-mode state without UPD9MODE
  loads. `vaeg_romless_tests` gains an end-to-end case (BRKEM, CALLN,
  save in the handler, reset, load, IRET back to compatible mode; plus
  the old-format refusal and the native old-format load), and the
  adapter self-test checks the mode-state codec. Full CTest has no
  failures (106 entries, one external SST skipped).

### ALTPRB results and the F bit-1 correction

- Run 1 outputs and the record-by-record comparison are archived in
  [`m103a_altregs_qa`](../reports/m103a_altregs_qa/README.md). On the real
  PC-88VA2 the alternate set, IX/IY/SP/DE/HL and F survive the `CALLN 91h`
  round trip; BC returns 0033h. vaeg matched except F in X2 (00h→02h) and
  X3 (C5h→C7h).
- Demonstrated cause: the native `IRET` applies
  `(flag & 0FD7h) | F002h` to every restored frame, forcing bit 1 to one
  and clearing bits 3/5, also when the frame returns to compatible mode,
  where its low byte is the compatible F.
- Correction: when the `IRET` resumes compatible mode, keep bits 1, 3 and
  5 of the frame (`(flag & 0FFFh) | F000h`); native returns are unchanged.
  Bit 1 is measured; bits 3/5 follow the same rule but were zero in every
  ALTPRB input, so they remain implementation policy.
- Verification: the adapter self-test now runs `XOR A` (F=44h) before the
  `CALLN` and requires F=44h after the `IRET`; with the old masking it
  fails with F=46h. Full CTest has no failures (106 entries, one external
  SST skipped). Re-running ALTPRB in vaeg is part of the repeated G103a.

### G103a (passed 2026-10-03)

- Standard gate passed with the static Windows build of
  `bb217138bb039fdec3f57d5bd48af69f0cd58f07` (`vaeg.exe` SHA-256
  `9292aceaa71ed0eae7f397b132bf4e80ab2aa4e07257c5f9a5588d8f7a817aa2`):
  V3 boot, VA demo, FreeDOS and a PC-Engine game checked by the maintainer.
- ALTPRB in vaeg after the F correction is byte-identical to the real
  PC-88VA2 run across all eight records (run 2 in
  [`m103a_altregs_qa`](../reports/m103a_altregs_qa/README.md); both
  `altprb.txt` SHA-256
  `8a559139d5eaeedee8082a0855e4af5c35acb96c4cd6c5db717d80f9899de30a`).
- Save/load at the CP/MVA `A>` prompt now works (step 4). A state saved by
  the M102 build is no longer resumed into a crash (step 5): it fails
  closed. Observation: for the maintainer's M102 save the GUI showed the
  generic "state load failed" rather than the "saved in compatible mode by
  an older build" text, so the tailored diagnostic can be masked by an
  earlier fail-closed check for some real M102 files; the safety property
  (no crash, load refused) holds. Not recorded as a root cause.
- Gate candidate including the run-2 evidence:
  `719f83950a5d3ce723f175dd3ac4babc9e24e759`. The behaviour-relevant source
  is `bb217138`; `719f8395` adds the run-2 ALTPRB evidence only.

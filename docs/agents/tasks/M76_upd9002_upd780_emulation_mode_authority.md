# M76 - uPD9002 uPD780 emulation-mode authority audit

M76 audits the uPD9002 main-CPU uPD780 emulation-mode evidence and decides the
safe implementation authority before the later I/O and BIOS restructuring
milestones.

Predecessor: approved G75.

Branch: `topic/m76-upd9002-upd780-emulation-mode-authority`

Commit prefix: `M76:`

Candidate gate: `G76`

Report: `docs/agents/reports/m76_upd9002_upd780_emulation_mode_authority.md`

Do not start M77. Do not merge M76 to `main` before G76 approval. Do not
declare G76 passed.

## Scope

M76 owns the authority boundary for the uPD9002 main CPU's uPD780
emulation-mode mechanism. This is separate from the FDC subsystem
uPD780-compatible CPU currently wrapped through `cpucva/z80_compat_cpu.cpp`.

M76 must:

- audit `docs/modernization/upd9002-upd70008-mode.md` and current uPD9002 mode
  state, decode, trace, interrupt, and state-save boundaries;
- use public M75 SCSI evidence and independently reproducible main-CPU mode
  evidence as inputs when available;
- audit the CP/M emulator `.cpv` hard-emulation path as a transition-mechanism
  exerciser when the source and binary identity are available;
- distinguish project-target software evidence from physical-silicon evidence;
- record the limits of V20/V30 8080-emulation-mode analogy sources;
- identify any VA ROM, BASIC, Debug 8800, or other guest software path that
  attempts to enter uPD9002 emulation mode;
- decide whether production implementation is authorized now, should be split
  into a later milestone, or remains blocked pending hardware evidence;
- define the exact state-save, trace, decoder, and test obligations for any
  later implementation.

## Non-goals

M76 must not:

- implement broad generic V20, V30, i286, i386, 8080, or Z80 compatibility;
- conflate the main uPD9002 emulation-mode mechanism with the FDC subsystem
  uPD780-compatible CPU;
- change FDC subsystem CPU behavior;
- move `iova/*`;
- implement SCSI;
- remove 98-only I/O or BIOS code;
- claim complete physical uPD9002 silicon proof while the repair/test hardware
  remains unavailable.

If M76 proves that emulation-mode implementation is required and sufficiently
bounded, it must still close with a clear authority report unless the approved
task is explicitly amended to include implementation.

## CP/M emulator transition evidence

M76 must treat the CP/M emulator `.cpv` path as evidence for the V30-style
mode-transition mechanism only, not as evidence for the VA uPD780/Z80
instruction set.

The CP/M emulator retrieval source to audit is:

```text
https://www.vector.co.jp/soft/win95/util/se378130.html
```

M76 must record the downloaded archive identity, source-file identity, binary
identity, and any redistribution or archival limitations before committing
derived evidence.

The audit must verify and record:

- whether `cpm.exe` enters the hard-emulation path only for `.cpv` programs
  and falls back to the software emulator for `.cpm` or `.com`;
- the runtime CPU probe, including the `D5 00` AAD behavior required before
  the hard path can execute;
- the exact BRKEM, CALLN, and RETEM byte sequences used by the hard path;
- the interrupt-vector table entries selected by the program and proof that
  vector numbers are ordinary IVT entries, not fixed firmware services;
- whether CALLN pushes a FLAGS, CS, IP frame on the native `SS:SP` stack while
  preserving the emulated stack pointer carried in `BP`;
- whether the native handler returns to emulation mode through `IRET`;
- whether RETEM is executed from the emulation-mode guest image and resumes
  native execution after BRKEM;
- any known exerciser defects, such as incomplete vector restoration, that
  affect repeatable testing.

The report must explicitly separate:

- transition-mechanism evidence from CP/M `.cpv`;
- VA project-target software evidence from BASIC, Debug 8800, ROM paths, or
  other VA programs;
- analogy evidence from V20/V30 documentation;
- unavailable physical-silicon evidence while repaired hardware is pending.

The CP/M emulator must not be used to justify 8080, Z80, IX/IY, alternate
register, ED-block, or VA uPD780 instruction semantics. It exercises only the
mode boundary and the minimal BRKEM/CALLN/RETEM contract unless additional
evidence is separately proven.

## Validation

Run repository invariant checks, normal builds, native tests, focused uPD9002
mode-state audits, and any trace or ROM-less probes needed to prove whether
guest software reaches the emulation-mode entry path.

The human gate must review the authority conclusion and decide whether a later
implementation milestone is authorized.

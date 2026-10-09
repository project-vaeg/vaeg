# VAEG Intel 8087 v7 implementation progress

- Implementation status: **IMPLEMENTED_UNVERIFIED / INTEGRATION_BLOCKED**
- Evidence validation: **PASS (P00 local audit)**
- Local emulator gates: **P00-P16 PASS; P18 PASS**
- Zero-omission audit: **PASS (1,597/1,597 documented slots; final P19 blocked)**
- Cross-host comparison: **NOT_RUN**
- Real VA hardware validation: **NOT_RUN**
- Silicon numerical comparison: **NOT_RUN**
- Cycle-exact CPU/NDP overlap: **OUT_OF_SCOPE**
- Integration blocker: **P17 VAEG-specific 8087 INT/BUSY route evidence is
  unavailable; P19 remains fail-closed**
- VA2 `/87` direct-8087 ASM seam: **PASS**; BASIC FAC conversion for the
  supplied arithmetic workload: **LOCAL SOFTWARE QA PASS**
- Next action: **qualify the VA2/VA3 BASIC /87 guest target, then obtain
  approved VAEG/VA route evidence and rerun P17/P19**

## 2026-09-20 local result

The tested tree remains at starting HEAD
`1544a1b8e19cc324c203a94c4e055361abb57d8b` on `main`; no commit, push, merge,
reset, clean, or history rewrite was performed. Existing unrelated dirty and
untracked paths remain preserved.

The supplied external evidence was verified before implementation. The six
PDF/OCR pairs and the exact Berkeley SoftFloat Release 3e archive are recorded
in [p00-audit.md](p00-audit.md). No OCR was rerun, and no full manual or OCR
document was copied into this repository.

Package results:

| Package | Local result | Evidence |
| --- | --- | --- |
| P00 | PASS | source/evidence and repository baseline audit |
| P01 | PASS | `memory=1368`, `register=229`, `total=1597` |
| P02 | PASS | exact SoftFloat 3e vendor/provenance and host-FP audit |
| P03-P12 | PASS | core tests and applicable production selftests, including native CPU/bus inventory coverage |
| P13 | PASS | `P13_MPFR_ORACLE PASS cases=9 precision=192/512` |
| P14-P16 | PASS | core tests; P16 inventory recheck |
| P17 | **INTEGRATION_BLOCKED** | [p17-route-blocker.md](p17-route-blocker.md); [BASIC /87 open issue](issue-basic87-va2-int-route.md) |
| P18 | PASS | configuration, reset-time clock application, savestate, GUI build, debugger path |
| P19 | **INTEGRATION_BLOCKED** | inventory/timing/core/selftest pass; route prerequisite unavailable |

The local production selftest also reports `8087 production path ok`, `8087
production inventory ok (1597 slots)`, `8087 CPU decode seams ok`, and `8087
guest interrupt route ok`. The inventory now executes every documented slot
through native uPD9002 decoding and the production bus/8087 seam. The latter
route result is a synthetic guest-handler test through the actual local V30/PIC
path; it does not replace the missing VAEG-specific evidence described by P17.

The VA2/VA3 main-ROM `/87` path is tracked as the next production guest
qualification target in [issue-basic87-va2-int-route.md](issue-basic87-va2-int-route.md).
Its guest-handler result is conditional: valid BASIC /87 execution does not
necessarily raise an unmasked 8087 exception, and a guest handler observation
still does not prove the VA2 PCB BUSY/INT wiring.

The direct ASM seam is now independently qualified in
[va2-basic87-asm-probe.md](va2-basic87-asm-probe.md): a self-authored flat
16-bit program loaded through the VA BASIC machine-language ABI executed
`FNINIT`, register arithmetic, segment-prefixed `FSTP` short/long-real stores,
`FWAIT`, and `RETF` under VA2 `/87`. The observed bytes were `00 00 00 40` for
short-real `2.0` and `00 00 00 00 00 00 00 40` for long-real `2.0`. This is
local software/guest-seam evidence only. The separate BASIC arithmetic
workload still needs an explicit N88 FAC-to-8087 format adapter test and is
not marked numerically passed.

The same probe's numbered ASCII BASIC companion was then placed on a
generated temporary QA disk and executed through `LOAD "VA2ASM.BAS"` followed
by `RUN` after `AUTOEXEC.BAT` selected `BASIC /87`. The guest returned `OK`
after both commands and printed the same short/long-real byte sequences. The
generated binary, D88, ROMs, and screenshots remain outside the repository.

## 2026-09-20 BASIC /87 FAC conversion closure

The earlier BASIC `/87` arithmetic mismatch was reproduced with a temporary
ESC trace and reduced to the `DC`/`DE` register-form dispatch in
`cpu/upd8087/upd8087.c`. The dispatcher passed the arithmetic operation and
the operand-reversal flag to `binary_stack_operation()` in the opposite order.
The ROM conversion helper uses `DC F9` (`FDIV ST(1),ST(0)`) to divide the
packed-BCD value by its decimal scaling constants; the swapped call made that
instruction execute as an add-like operation. The corrected dispatcher now
keeps the documented `DC` and `DE` operation/reversal directions.

The runtime route was then confirmed against the supplied VA2 ROM fixture:

```text
FAC packed-BCD bytes at physical 037DE: 00 00 00 00 00 00 00 00 15 00
FBLD entry: runtime E000:B85C (varom00 bank 1)
internal long-real store for A:  3FF8000000000000 (1.5)
internal short-real store for A: 3FC00000 (1.5)
FAC packed-BCD bytes for B:     00 00 00 00 00 00 00 50 22 00
internal long-real store for B:  4002000000000000 (2.25)
internal short-real store for B: 40100000 (2.25)
short-real FADD store:            40700000 (3.75)
```

The complete ASCII `X87TEST.BAS` workload was loaded from the temporary QA
disk under `BASIC /87` and displayed the expected values for ADD, MUL, DIV,
SQR, LOG, EXP, SIN, COS, TAN, and ATN, ending with `Ok`. The same ASCII source
was also saved with `SAVE "X87COPY.BAS",A`, reloaded, and run successfully.
`NEW 87` was separately exercised with `A=1.5`, `B=2.25`, and `PRINT A+B`,
which displayed `3.75`. Plain BASIC on the same VA2 ROM fixture continued to
display the matching non-8087 workload results. The generated disks, ROMs,
screenshots, and trace logs remain outside the repository.

The focused regression now covers both `DC` normal/reverse division and `DE`
pop arithmetic directions. `vaeg_8087_tests`, the production selftest, and
the native 1,597-slot inventory remain passing. This closes the local numeric
FAC/IEEE qualification for the supplied workload, but does not close the
separate VA2 PCB BUSY/INT route gate.

The final gate was run after the latest source rebuild, native inventory
coverage addition, and post-audit repairs:

```text
python3 tools/8087/gate.py --final
P01_INVENTORY rows=111 memory=1368 register=229 total=1597
P19_TIMING rows=111 slots=1597
P17_INTEGRATION_BLOCKED: no supplied VAEG/VA evidence establishes the 8087 INT/BUSY controller route; the external evidence tree contains only Intel/NEC manuals and SoftFloat
P17: INTEGRATION_BLOCKED
P19_INTEGRATION_BLOCKED: P17 route evidence is unavailable; zero-omission QA cannot claim production-route acceptance
P19: INTEGRATION_BLOCKED
exit 2
```

The key tested source/audit digests for the latest run were:

```text
b29c5289c8e622afceb8c33aeac01440cec64c9edbc623555e4988bb31f1bc07  cpu/upd8087/upd8087.c
6e32fc6900faaa0039e2ca7c6d1c3cddc694486e753d9d474dba443b77d34588  cpu/upd8087/upd8087.h
8b82f0b706c3726c2a2bd37641258bf4baef3d929995f62d1cc1318a82aab30b  tools/8087/gate.py
87b42f422ad9420b6e5745898f778de14d38765b0723baa06dde961aebaddc90  docs/8087/v7/records/opcode-inventory.tsv
e544eea688d3b013eb9e75898c5f999ed96ec9b80d10598ec181284077484863  docs/8087/v7/records/timing-table.tsv
5dddc415a07c320c31a3b54e0cb3b55e02ee97d16ecb4ca759e9e39a60b428b3  tools/8087/test_core.c
332c7ca81d6f97cb019d08d75de68e8a95e7b04472215629b672042e784d2304  sdl2/selftest.c
```

The local commands and exit codes were:

```text
python3 tools/8087/audit_inventory.py                         exit 0
python3 tools/8087/audit_timing.py                            exit 0
build/linux-debug/vaeg_8087_tests                             exit 0
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-debug/sdl2/vaeg --selftest  exit 0 (native inventory 1597/1597)
cmake -DVAEG_EXECUTABLE="$PWD/build/linux-debug/sdl2/vaeg" -DTRACE_GOLDEN="$PWD/tests/upd9002/trace_baseline.txt" -P "$PWD/tests/upd9002/run_trace_equivalence.cmake"  exit 0
cmake --build build/linux-debug -j2 && ctest --test-dir build/linux-debug --output-on-failure  exit 0 (98/98; test 70 skipped)
python3 tools/8087/gate.py --final                            exit 2 (P17/P19 contract blocker above)
```

The post-audit repairs covered the documented FPATAN operand direction, DA/DE
integer-memory operand direction, masked versus unmasked `FLD ST(i)` stack
underflow response, CPU IF masking in the synthetic interrupt-route test,
FSAVE/FSTENV post-save behavior, machine-state acknowledgement round trips,
projective-infinity comparison, FPREM sole-NaN propagation, denormal-to-unnormal
work-area conversion, unnormal arithmetic/result classes and real-store
boundaries, unmasked denormal transaction behavior, FDISI/FENI request state,
FRSTOR timing-residue preservation, C1 preservation for masked compare stack
faults, and signed `INT64_MIN` loading. The latest core test, full production
selftest, native 1,597-slot inventory, trace-equivalence check, and complete
CTest run were rerun on 2026-09-20 after the coverage addition; the core test,
production selftest, trace check, and all 98 local CTest cases exited zero. The
final gate was rerun at 2026-09-20 14:55 (+0900) on the updated source tree and
exited 2 solely for the contract-defined P17/P19 route-evidence blocker.

The latest final-gate output is retained at
`/private/tmp/vaeg-8087-gate-final-20260920-1455.out` (outside the repository). It reports
`P01_INVENTORY rows=111 memory=1368 register=229 total=1597`,
`P19_TIMING rows=111 slots=1597`, P00-P16/P18 PASS, and the same P17/P19
`INTEGRATION_BLOCKED` messages recorded above.

## 2026-09-20 regression closure

The CPU decode-seam fixture now explicitly executes native `9Bh` FWAIT/POLL
with no 8087 present and with an enabled 8087 holding an unmasked pending
fault. The fixture was placed after the bounded legacy trace window so the
existing M42/M60a trace contract remains unchanged. On the resulting source
tree:

```text
cmake --build build/linux-debug --target vaeg_sdl2 vaeg_8087_tests -j2  exit 0
build/linux-debug/vaeg_8087_tests                                  exit 0
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-debug/sdl2/vaeg --selftest  exit 0
cmake -DVAEG_EXECUTABLE="$PWD/build/linux-debug/sdl2/vaeg" -DTRACE_GOLDEN="$PWD/tests/upd9002/trace_baseline.txt" -P "$PWD/tests/upd9002/run_trace_equivalence.cmake"  exit 0
cmake --build build/linux-debug -j2 && ctest --test-dir build/linux-debug --output-on-failure  exit 0
```

CTest reports `100% tests passed, 0 tests failed out of 98`; test 70 is the
pre-existing external SST test and remains skipped. The native production
fixture additionally reports `8087 production inventory ok (1597 slots)`, and
the final gate still exits 2 only for the documented P17/P19 route-evidence
blocker recorded above.

## 2026-09-20 INT route seam extension

The provisional software route now drives the sole 8087's source-backed INT
state through the existing logical `IRQ_NDP` input (PIC2 IR6), while leaving
PIC vector selection, masking, IF arbitration, and EOI handling to the native
PIC implementation. Edge mode latches only the transition; level mode
re-presents an asserted input after EOI. The 8087 source clear remains
separate from PIC acknowledgement, and save-state restore rebinds the input
from the restored pending state without changing the serialized PIC layout.

The following focused commands ran on the current uncommitted working tree:

```text
cmake --build build/linux-debug --target vaeg_sdl2 vaeg_8087_tests -j2  exit 0
build/linux-debug/vaeg_8087_tests                                  exit 0
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/linux-debug/sdl2/vaeg --selftest  exit 0
  selftest: 8087 production path ok
  selftest: 8087 production inventory ok (1597 slots)
  selftest: 8087 CPU decode seams ok
  selftest: 8087 guest interrupt route ok
  selftest: 8087 PIC input modes ok
  selftest: statsave ok
  selftest: all tests passed
ctest --test-dir build/linux-debug --output-on-failure -R 'vaeg_(8087_core|upd9002_state_boundary|upd9002_state_payload_probe|romless_tests)$'  exit 0 (4/4)
git diff --check                                                   exit 0
python3 tools/8087/gate.py --package P17                            exit 2 (selftest passed; route evidence blocker)
python3 tools/8087/gate.py --package P19                            exit 2 (inventory/timing/core/selftest passed; route blocker)
```

Current working-tree SHA-256 values for the affected implementation/test
files are:

```text
08b52a19c883da9584d446c15f439e3b58f06a54ee403657d97c787f439aac6e  cpu/upd9002/upd9002_mn.c
7bc43153c7588ce40beb0b0ee606e405ce36828b2285c0b47355292f003124d4  io/pic.c
ff8dd6e277936ae8b90422b26e70f647703aaf73cf67a0727157b25d170e67a1  io/pic.h
88981ce4f45a087bb6b17ed77224648dc91ca026bda2229673e5c123e741e408  machine/statsave.c
2407fdf39796e080ee0d29a364a335329e5b236bfab9d2b5025f2fbd8a9d48f0  machine/statsave.tbl
823e25000013ddfbda932358196cefee843ed1b571c3c7f007624e03210d4b9a  sdl2/selftest.c
```

The PIC2 `40h`/vector `46h` and LTIM results are repository-local synthetic
route evidence only. They do not establish the VA2 PCB BUSY/INT net,
polarity, physical PIC input, or hardware acknowledgement. P17 and P19
therefore remain `INTEGRATION_BLOCKED`.

## 2026-09-20 boot-profile integration

The SDL2 `Emulate -> Boot model` menu now exposes three persistent startup
profiles:

```text
VA                 pc_model=88VA1, 8087 disabled
VA2/VA3            pc_model=88VA2, 8087 disabled
VA2/VA3 + 8087     pc_model=88VA2, 8087 enabled
```

Both VA2 profiles use the same VA2/VA3 ROM set. Selecting a profile keeps the
existing backup-memory, sound, media-preserving reset, configuration-save, and
reset-time 8087 application paths. The Configure modal remains the place for
the independently persisted 8087 clock value. This is the local software
integration handoff for [Issue #8](https://github.com/nakatamaho/vaeg/issues/8);
the issue remains open for later VA2/VA3 PCB BUSY/INT route verification.

The profile change rebuilt successfully and the existing 8087 core,
production selftest, and focused CTest set passed. It does not change the
P17/P19 classification: the physical route evidence remains unavailable.

## Scope and evidence boundary

The implementation covers the single optional 8087, exact 8 MHz default,
independent persisted clock with reset-time application, serial integer NDP
service timing, native/8080 decode separation, FPO1-only attachment, all
documented inventory forms, packed BCD, environment/save/restore, control and
exception behavior, and the five required transcendental instructions. The
local tests exercise those paths, but this record deliberately does not call
the result `SOFTWARE_VERIFIED` while the contract-defined P17 route evidence
is absent.

For every package attempt record:

- date/time
- package ID
- starting HEAD
- dirty-source digest
- exact changed production/test paths
- exact commands and exit codes
- discovered/run/pass/fail/skip test counts
- artifact hashes
- baseline delta
- source/evidence decisions
- blocker or next action
- local commit ID, if repository policy permits one

Do not write PASS before the command actually ran on the recorded source digest.

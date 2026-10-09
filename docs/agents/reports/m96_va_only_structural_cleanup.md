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

# M96 VA-only structural cleanup report

## M96 status

M96a was report-only and passed G96a. M96b removes only proven-dead residue
after the staged reachability review. M96c clarified the live VA I/O and C-bus
ownership boundaries. M96d removes the physical-address NOP side channel and
passed its focused regression test. G96d passed on the maintainer's human
check. M96e removes the unreachable simulated BIOS initializer and bootstrap
helpers, and retains only live SASI/SCSI work-area fields. No ROM, disk, font,
icon, cursor, or wave payload was modified. The
working branch is
`topic/m96-va-only-structural-cleanup`.

M96f removed proven-dead configuration fields after the designated-initializer
byte-identity gate; G96f passed. M96g then migrated runtime system-port users
to the VA latch, removed the redundant `SYSTEMPORT` state section with an
explicit version bump, and removed legacy serialized memory chunks and
constants. G96g passed on the final M96g4 candidate. G96h passed on the final
M96h candidate; M96i completed its final audit and passed G96i on the
`e311ea583397f04d9489e82897405fe139e2c3aa` candidate.
The maintainer subsequently passed the final G96 human gate on the
`74c859111951f22e86b2bd6453a5f07b040fd6da` candidate; M96 is complete.

The task file was absent at the evaluated baseline. This commit adds the
tracked task index at
[`docs/agents/tasks/M96_va_only_structural_cleanup.md`](../tasks/M96_va_only_structural_cleanup.md)
and registers M96 in `ROADMAP.md`.

## 1. Baseline confirmation

| Item | Result |
| --- | --- |
| Evaluated commit | `dfe50a1420c075040c12b96f00c315b5987a846a` |
| Branch before M96a | `main` |
| M96 branch | `topic/m96-va-only-structural-cleanup` |
| `git status --short` | Only pre-existing untracked `.codex-write-test` and maintainer-local `docs/98io/`, `docs/cpmva/`, `docs/disks/`, `docs/roms/`, `docs/tekumani/` |
| Submodules | None reported by `git submodule status` |
| Task file before M96a | Missing |
| CMake presets | `linux-debug`, `linux-release`, `linux-ci-gcc`, `linux-ci-clang`, `linux-asan`, `linux-ci-asan`, `mingw-release`, `mingw-cross`, `mingw-ci`, `macos-release`, `macos-macports`, `macos-asan`, `macos-ci` |

The local reference trees are not included in the report or source comments.
They remain outside the tracked M96 tree.

## 2. Production ownership inventory

This is a directory-level ownership map derived from the explicit CMake source
lists at the baseline. It is not a deletion verdict.

| Directory or tier | Baseline ownership |
| --- | --- |
| `common/`, `generic/` | Shared utility, text, profile, geometry, and information helpers used by active targets |
| `cpu/upd9002/` | Main uPD9002 instruction, memory, state, diagnostics, and DMA implementation; CPU adapter changes are excluded by M96 S4 |
| `cpu/upd780/`, `cpu/z80_compat_*` | uPD780 FDC-facing disassembly and shared compatibility backend used by production adapters; excluded by S4 |
| `diagnostics/` | Active host-side uPD9002 diagnostics |
| `io/` | Native VA built-in I/O, DMA, FDC, serial, timer, memory-control, SGP/TSP, and shared lifecycle callbacks |
| `cbus/` | Live PC-88VA C-bus ownership and device lifecycle for SASI, SCSI, MPU98II, and BMS; protected by S1 |
| `sound/` | VA FM/OPN/OPNA, PSG, rhythm, ADPCM, beep, sound ROM, and shared synthesis/input support |
| `fdd/` | FDD media, geometry, motor, XDF, D88, and SASI/SCSI backing support |
| `font/` | Character data, conversion, and host backing; `fontrom` remains in `mem[]` under S3 |
| `bios/` | VA ROM loader, simulated BIOS fallback, FDD bootstrap helpers, and SASI/SCSI BIOS service code; M96d/e audit only |
| `machine/` | Reset, clock, event, keyboard, state-save, timing, and machine policy |
| `memoryva/` | Authoritative VA CPU memory decoder and GVRAM mapping |
| `vram/` | Active VA raster/text/sprite/graphics conversion plus generic residue candidates under M96b2 |
| `sdl2/` | Portable host frontend, GUI, media, keyboard, selftest, pacing, and sound integration |
| `external/` | Vendored third-party ImGui, ymfm, SDL2, libarchive, zlib, and xz dependencies; not M96 cleanup scope |
| `romimage/` | Read-only ROM/resource source and generated payload inventory; protected by S2 |

## 3. Reset and bind lifecycle

### Cold reset (`machine/pccore.c`)

The observed order is:

```text
iocore_reset()
cbuscore_reset()
fmboard_reset(pccore.sound)
upd9002_memorymap_va()
iocore_build()
iocore_bind()
cbuscore_bind()
fmboard_bind()
```

`iocore_create()` occurs during machine construction and
`iocore_destroy()` during termination. `fmboard_reset(usesound)` also occurs
during state-load reconstruction before the callback tables are rebuilt.

### State-load reconstruction (`machine/statsave.c`)

The observed order is:

```text
iocore_reset()
cbuscore_reset()
fmboard_reset(pccore.sound)
... state sections are restored ...
upd9002_memorymap_va()
iocore_build()
iocore_bind()
cbuscore_bind()
fmboard_bind()
```

No ordering change is made in M96a. The three tiers are currently distinct:
`iocore` owns the canonical CPU-visible 16-bit VA map and built-in bindings;
`cbuscore` owns live C-bus device reset/bind ownership; `fmboard` owns the
selectable VA OPN/OPNA sound-board lifecycle. M96c must preserve this
distinction unless a behavior-backed alternative is demonstrated.

## 4. Dispatch inventory

| Dispatch or selector | Baseline finding | M96a classification |
| --- | --- | --- |
| `PCMODEL_VA` | Same value as `PCMODEL_VA1`; removed by the M96f vocabulary cleanup while the serialized model byte remains | `REMOVED_M96F`; `PCMODEL_VA1`/`PCMODEL_VA2` remain the live VA selectors |
| `PCMODEL_VA1` / `PCMODEL_VA2` | Live VA1/VA2 runtime selection in memory, ROM, SGP, GUI, and selftests | `RETAIN_LIVE` |
| `SUPPORT_PC98*`, `PC9821*`, `PC9801*` | No active production selector found in the baseline source search; prior removal is recorded as `ALREADY_ABSENT` (M72/M91 where applicable) | `ALREADY_ABSENT` |
| `iocore16` | Only the unreferenced `io/iocore16.tbl` artifact remains; canonical VA map is `io/iocore.c` | `DEFER_TO_M96b3` |
| `CPU_ITFBANK` | Only writer is inactive `io/necio.c`; read guard remains in `biosfunc()` and the CPU state field is serialized | `DEFER_TO_M96b3/M96g` |
| `biosfunc()` | Live NOP side-channel entry and physical simulated-BIOS cases remain | `RETAIN_LIVE`; isolated M96d/e audit |
| `BIOS_SIMULATE` | Always-defined simulated BIOS path in `bios/bios.c` | `DEFER_TO_M96e` |
| `VAEG_FIX`, `VAEG_EXT` | No active selector remains in production CMake; historical evidence is retained in reports | `ALREADY_ABSENT` / M96h evidence check |

### M96c dispatch and ownership conclusion

The M96c census found no surviving runtime selector that dispatches to an
obsolete PC-98 machine implementation. `PCMODEL_VA1`/`PCMODEL_VA2` remain the
live VA model selectors. The same-valued `PCMODEL_VA` compatibility constant is
removed by M96f without changing the serialized `PCCORE.model` byte. `CPU_ITFBANK` and
the simulated-BIOS path remain deferred to their isolated M96d/e/g stages.
The C-bus callback tier is retained under S1; it is a live hardware ownership
boundary, not a second I/O map.

M96c makes no callback or dispatch behavior change. It adds only file-level
ownership comments to `io/iocore.c` and `cbus/cbuscore.c`.

## 5. Two-reviewer debate

Both reviews inspect the same baseline and treat S1-S4 as settled facts.

| Item | Reviewer A - reduction advocate | Reviewer B - preservation advocate | Arbiter | Evidence |
| --- | --- | --- | --- | --- |
| `io/egc.*`, `cpu/upd9002/egcmem.*` | Not in production CMake; tool reports them unreferenced | Names and implementation could represent inherited graphics behavior; verify VA ownership and stale includes before deletion | `DELETE` in M96b1 | Explicit CMake lists; `find_unreferenced.py`; includes in each other only |
| `io/vramcompat.h` | No production include or symbol use | Header may be historical contract for the EGC family | `DELETE` candidate, pending M96b proof | No active include; no CMake entry |
| `vram/vram.c`, `vram/vram.h` | Generic state is only consumed by dead EGC code and stale includes | `bios.c`, `pccore.c`, and `statsave.c` include `vram.h`; prove no state symbols are used | `DELETE` in M96b2 | `rg` finds no `vramop`, `VOP_*`, or `MEMWAIT_*` in those production units |
| `vram/palettesva.c`, `vram/palettesva.h` | `palettesva.c` was the only active CMake entry and both files contain no implementation | No production include or symbol use | `DELETE` in M96b2 | Explicit `VAEG_VA_SOURCES` entry removed with the empty files |
| `io/dipsw.*`, `io/printif.*` | No CMake entry or runtime registration | DIP switch data itself is live in `NP2CFG`; VA port 0040 is owned by `sysportva.c`, not this duplicate | `DELETE` in M96b3 | Candidate files are not built; active VA state remains |
| `io/necio.*` | No production entry; sole `CPU_ITFBANK` writer is inactive | CPU state layout and legacy BIOS guard remain separate state work | `DELETE` in M96b3; retain state field | `CPU_ITFBANK` read/write census; state assertions in `upd9002_state.c` |
| `io/cpuio.*` | No production entry, but tracked M49 QA evidence reads it | Historical QA contract still names the reset-request handler | `RETAIN_COMPATIBILITY` | Do not break the protected reachability tool in M96b3 |
| `io/fdd320.*` | No production entry | M72 preservation note and no VA-authoritative 0051h evidence leave 5-inch 2D ownership unresolved | `DEFER_INSUFFICIENT_EVIDENCE` | Requires a dedicated hardware audit; do not delete by name |
| `io/iocore16.tbl` | Unreferenced | It is a historical word-port termination table, not the active map | `DELETE` in M96b3 | No production include; `io/iocore.c` owns active map |
| `oprecord.c` | Not built and guarded by undefined `SUPPORT_OPRECORD` | Out-of-tree operation-record compatibility may still be intentional | `DEFER_INSUFFICIENT_EVIDENCE` | M72 explicitly left `SUPPORT_OPRECORD` for focused audit |
| `common/wavefile.*` | Not built and guarded by undefined `SUPPORT_WAVEREC` | Recording is an optional historical interface; verify all supported definitions first | `DEFER_INSUFFICIENT_EVIDENCE` | M72 left `SUPPORT_WAVEREC` for focused audit |
| `io/np2vasup.*` | Stub is unreferenced by production CMake | Header explicitly says it is retained for out-of-tree users | `RETAIN_COMPATIBILITY` | File comment and no active selector |
| `generic/unasm*`, `cmndraw*`, `dipswbmp*`, `common/mimpidef.*` | Self-contained, not built, and unreferenced | No tracked production or tool consumer remains | `DELETE` in M96b4 | CMake and reference census |
| `sound/tms3631*`, `bios/rsbios.h` | No active CMake or symbol path | No generated/tool consumer remains | `DELETE` in M96b4 | Production and tool search has no consumers |
| `cbus/scsibios.res` | Payload is not included; `cbus/sasibios.res` is the live SASI payload | C-bus itself is live under S1; deleting one unused payload must not touch device code | `DELETE` candidate | `cbus/sasiio.c` includes `sasibios.res`, not `scsibios.res` |
| `fdd/fdd_mtr.res` | Payload is not included; active motor sound loads external WAV files | FDD motor behavior is live and must remain | `DELETE` candidate | `fdd/fdd_mtr.c` uses WAV names, not the resource |

## 6. Reachability freeze for M96b candidates

The B1-B3 evidence is committed. B4 applies the same proof to the remaining
NP2 utility and generated-payload candidates. `find_unreferenced.py` reported
57 items after B3, including protected ROM generators, guest tools, and the
candidates below. M96b adds one final row per deleted file with build,
include, symbol, runtime, hardware, and historical evidence.

| Candidate group | CMake entry at baseline | Active include/symbol evidence | Initial decision |
| --- | --- | --- | --- |
| EGC/GRCG (`io/egc.*`, `cpu/upd9002/egcmem.*`, `io/vramcompat.h`) | Absent | Only candidate-to-candidate includes; no active VA registration | `DELETE` in M96b1 |
| Generic VRAM (`vram/vram.*`) | `vram.c` and `vram.h` removed; prior includes were stale | No production use of `vramop`, `tramupdate`, `vramupdate`, `VOP_*`, or `MEMWAIT_*` outside deleted EGC code | `DELETE` in M96b2 |
| PC-98 I/O (`io/dipsw.*`, `io/necio.*`, `io/cpuio.*`, `io/fdd320.*`, `io/printif.*`, `io/iocore16.tbl`) | Absent | No runtime registration; `np2cfg.dipsw` and `CPU_ITFBANK` uses are separate live/state paths | `DELETE` or defer by per-file proof |
| NP2 utility residue | Absent | `oprecord` and `wavefile` are macro-guarded; `np2vasup` explicitly retained as stub | Defer macro-guarded items; retain compatibility stub; audit others |
| Generated legacy payloads | Absent | `scsibios.res` and `fdd_mtr.res` have no production include | `DELETE` in M96b4; no C-bus/FDD code deletion |

`romimage/` files reported by the tool are read-only and are not M96b
deletion candidates.

### 6.1 M96b1 B1 deletion proof

The B1 files were outside every production CMake source list. A complete
tracked-source search found no production include, symbol reference, callback
registration, reset/bind path, or state-save entry after the earlier portable
VA migration. The only remaining references were candidate-to-candidate
includes and historical reports. The EGC/GRCG implementation therefore has
no runtime route in the current VA product and was removed as a group.

| File | CMake absent | Include absent | Symbol absent | Runtime absent | Hardware classification | Last historical reference | Decision |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `io/egc.c` | Yes | Yes | Yes outside deleted group | Yes: no `egc_bind`/`egc_reset` registration | Obsolete unreachable legacy EGC port handler | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `io/egc.h` | Yes | Yes | Yes outside deleted group | Yes | Obsolete unreachable legacy EGC state contract | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `cpu/upd9002/egcmem.c` | Yes | Yes | Yes outside deleted group | Yes: no VA memory-map route | Obsolete unreachable legacy EGC memory engine | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `cpu/upd9002/egcmem.h` | Yes | Yes | Yes outside deleted group | Yes | Obsolete unreachable legacy EGC memory interface | `484cf94d` (`M51: move uPD9002 core sources`) | `DELETE` |
| `io/vramcompat.h` | Yes | Yes | Yes | Yes | Obsolete compatibility-only GRCG/EGC state declarations | `2fe49c94` (`M88: remove retired non-VA display backends`) | `DELETE` |

This deletion does not remove the live VA GVRAM, GDC, SGP, TSP, or C-bus
boundaries. The VA renderer remains under `vram/*va.c` and the VA memory
decoder remains under `memoryva/`.

### 6.2 M96b2 B2 deletion proof

The generic VRAM files had no production definition or consumer after the
EGC removal. `bios/bios.c`, `machine/pccore.c`, and `machine/statsave.c` had
only stale `vram.h` includes; no code in those units referenced the generic
state, wait macros, or operation flags. The two VA palette files contained no
implementation and were not included anywhere. Their one CMake source entry
was removed together with the files. Comment-only stale includes in the VA
renderer were removed as part of the same cleanup.

| File | CMake absent | Include absent | Symbol absent | Runtime absent | Hardware classification | Last historical reference | Decision |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `vram/vram.c` | Yes | Yes | Yes | Yes: no `vram_initialize` route | Obsolete generic VRAM state backing | `88399555` (`M72: remove inactive PC-9821 guarded code`) | `DELETE` |
| `vram/vram.h` | Yes | Yes after removing three stale includes | Yes: no `VRAM_T`, `VOP_*`, or `MEMWAIT_*` consumer | Yes | Obsolete generic VRAM/EGC compatibility declarations | `e5941339` (`M92: format active emulator core sources`) | `DELETE` |
| `vram/palettesva.c` | Entry removed from `VAEG_VA_SOURCES` | Yes | Yes: comment only | Yes | Empty obsolete VA palette translation residue | `71a4a3bd` (`M89: merge VA source directories`) | `DELETE` |
| `vram/palettesva.h` | N/A | Yes | Yes: comment only | Yes | Empty obsolete VA palette header residue | `71a4a3bd` (`M89: merge VA source directories`) | `DELETE` |

### 6.3 M96b3 B3 deletion and preservation proof

The deleted B3 files have no production CMake entry, include edge, runtime
registration, or active symbol consumer. `io/dipsw.c` is not the live
configuration storage: `np2cfg.dipsw[]` remains in `NP2CFG` and its VA
consumers remain untouched. The old printer file duplicated the 0040H port
but the active VA owner is `io/sysportva.c`; deleting the unbuilt duplicate
does not remove the VA system-port implementation. `io/necio.c` was the only
writer of `CPU_ITFBANK`, but the serialized CPU field and its BIOS guard are
not removed here.

Two candidates are deliberately retained. `io/cpuio.c` is read by the
tracked M49 protected-reachability tool and golden evidence, so deleting it
would break a compatibility/evidence contract even though it is absent from
the production build. `io/fdd320.c` remains unresolved: M72 recorded that
5-inch 2D behavior might belong to the PC-88 side, and the available
VA-authoritative references inspected in M96 do not establish the 0051H
interface. It is deferred rather than deleted by its historical name.

| File | CMake absent | Include absent | Symbol/runtime result | Hardware classification | Last historical reference | Decision |
| --- | --- | --- | --- | --- | --- | --- |
| `io/dipsw.c` | Yes | Yes | No `dipsw_w8`/`dipsw_r8` registration | Obsolete unbuilt legacy DIP port adapter; `NP2CFG.dipsw[]` is live separately | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `io/dipsw.h` | Yes | Yes | No declaration consumer | Obsolete companion declaration | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `io/necio.c` | Yes | Yes | Sole `CPU_ITFBANK` writer is not in production | Obsolete unbuilt ITF-bank selector; CPU state is retained | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `io/necio.h` | Yes | Yes | No declaration consumer or live `NECIO` object | Obsolete companion declaration | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `io/printif.c` | Yes | Yes | No `printif_bind` registration; VA 0040H is owned by `sysportva.c` | Obsolete duplicate printer adapter | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `io/printif.h` | Yes | Yes | No declaration consumer | Obsolete companion declaration | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `io/iocore16.tbl` | No production entry | No include | No symbol consumer | Obsolete generic word-port table; active VA map is `io/iocore.c` | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `io/cpuio.c` | Yes | Referenced by tracked M49 QA tool | Protected tool/evidence consumer remains | Legacy compatibility/evidence contract | `b04c6203` (`M86: update machine core references`) | `RETAIN_COMPATIBILITY` |
| `io/cpuio.h` | Yes | No production include; paired with retained QA source | Protected historical interface companion | Legacy compatibility/evidence contract | `2baf50de` (`M4: lowercase all tracked paths`) | `RETAIN_COMPATIBILITY` |
| `io/fdd320.c` | Yes | No production include | No live route found | Hardware ownership unresolved; prior M72 preservation note | `b04c6203` (`M86: update machine core references`) | `DEFER_INSUFFICIENT_EVIDENCE` |
| `io/fdd320.h` | Yes | No production include | No live route found | Hardware ownership unresolved; companion header | `2baf50de` (`M4: lowercase all tracked paths`) | `DEFER_INSUFFICIENT_EVIDENCE` |

### 6.4 M96b4 B4 deletion and preservation proof

The B4 utility sources are not in any production CMake source list and have
no production include or symbol consumer. `oprecord.*` and `common/wavefile.*`
are deliberately retained: active files include their headers unconditionally,
and their APIs remain a compatibility surface when the corresponding optional
macros are enabled by an out-of-tree build. `io/np2vasup.*` is also retained
because its source comment explicitly documents that out-of-tree contract.

The two deleted `.res` files are generated payloads, not live device code.
The active C-bus SASI path includes `sasibios.res`; no production unit includes
`scsibios.res`. The active FDD motor implementation loads external WAV files
(`seek.wav`, `seek1.wav`, `headon.wav`, and `headoff.wav`) and does not include
`fdd_mtr.res`.

| File | CMake absent | Include absent | Symbol/runtime result | Hardware classification | Last historical reference | Decision |
| --- | --- | --- | --- | --- | --- | --- |
| `generic/unasm.c` | Yes | Yes | Self-contained `unasm()` only | Obsolete unbuilt disassembler utility | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/unasm.h` | Yes | Yes | No declaration consumer | Obsolete companion declaration | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `generic/unasmdef.tbl` | N/A | Only deleted `unasm.c` | No external consumer | Obsolete disassembler table | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/unasmfpu.tbl` | N/A | Only deleted `unasm.c` | No external consumer | Obsolete disassembler table | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/unasmop.tbl` | N/A | Only deleted `unasm.c` | No external consumer | Obsolete disassembler table | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/unasmop3.tbl` | N/A | Only deleted `unasm.c` | No external consumer | Obsolete disassembler table | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/unasmop8.tbl` | N/A | Only deleted `unasm.c` | No external consumer | Obsolete disassembler table | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/unasmstr.tbl` | N/A | Only deleted `unasm.c` | No external consumer | Obsolete disassembler table | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/cmndraw.c` | Yes | Yes | Self-contained host bitmap helpers only | Obsolete unbuilt generic renderer | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/cmndraw.h` | Yes | Yes | No declaration consumer | Obsolete companion declaration | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `generic/dipswbmp.c` | Yes | Only deleted `dipswbmp.h` | No `dipswbmp_*` consumer | Obsolete unbuilt legacy option bitmap resource | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `generic/dipswbmp.h` | Yes | Yes | No declaration consumer | Obsolete companion declaration | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `generic/dipswbmp.res` | N/A | Only deleted `dipswbmp.c` | No external consumer | Obsolete unbuilt option bitmap payload | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `common/mimpidef.c` | Yes | Only deleted `mimpidef.h` | No `mimpidef_load` consumer | Obsolete unbuilt MIMPI definition parser | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `common/mimpidef.h` | Yes | Yes | No declaration consumer | Obsolete companion declaration | `2baf50de` (`M4: lowercase all tracked paths`) | `DELETE` |
| `sound/tms3631.h` | Yes | Yes | No `tms3631_*` consumer | Obsolete unbuilt TMS3631 sound path | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `sound/tms3631c.c` | Yes | Only deleted TMS header | No runtime registration | Obsolete unbuilt TMS3631 implementation | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `sound/tms3631g.c` | Yes | Only deleted TMS header | No runtime registration | Obsolete unbuilt TMS3631 implementation | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `bios/rsbios.h` | Yes | Yes | No `RSBIOS` consumer | Obsolete unbuilt serial BIOS layout | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `cbus/scsibios.res` | Yes | Yes | `sasibios.res` is live; this payload has no consumer | Obsolete duplicate C-bus SCSI payload; C-bus code remains live | `b04c6203` (`M86: update machine core references`) | `DELETE` |
| `fdd/fdd_mtr.res` | Yes | Yes | `fdd_mtr.c` loads external WAV files | Obsolete embedded motor-sound payload; FDD motor code remains live | `b04c6203` (`M86: update machine core references`) | `DELETE` |

## 7. Retained legacy-looking names

These names are not deletion evidence. Their baseline disposition is:

| Name | Why it remains or is deferred |
| --- | --- |
| `mpu98ii` | Live PC-88VA C-bus MPU98II device selected through `cbuscore` |
| `fontpc98`, `fontv98` | Font conversion/backing paths in the active VA font subsystem; require behavior evidence before removal |
| `sasiio`, `scsiio`, `scsicmd` | Live PC-88VA C-bus storage devices under S1 |
| `np2sysp` | Active shared system-policy/I/O support in the production map; name is historical, path is live |
| `cbus` | Live PC-88VA expansion-bus lifecycle and ownership tier under S1 |

## 8. `mem[]` and state-save baseline

M96a does not change storage or state format. The reset path currently clears
main/HMA and legacy compatibility ranges, and also clears a font range at
`FONT_ADRS`. The exact live layout and state-section sizes are deferred to
M96e/M96g after producer/consumer evidence. In particular, `fontrom` remains
inside `mem[]` and no `mem[]` bound is changed here.

## 11. Comment evidence map

M96c adds only file-level ownership comments. They cite this tracked section;
no source comment cites the maintainer-local hardware-document directories.

| Source file | Comment subject | Behavioral statement | Hardware reference | Evidence class |
| --- | --- | --- | --- | --- |
| `io/iocore.c` | File ownership and lifecycle comment | The canonical VA CPU-visible 16-bit I/O map owns built-in bindings; C-bus devices register into the same map and reset/build/bind order is deliberate | Current source lifecycle in `machine/pccore.c` and `machine/statsave.c`; maintainer-settled S1 C-bus boundary | `emulator-policy` / `hardware-documented` |
| `cbus/cbuscore.c` | C-bus ownership comment | C-bus owns reset/bind lifecycle for live SASI, SCSI, MPU98II, and BMS devices; it is not a second CPU I/O map or PC-9801 residue | Current callback tables and `machine/pccore.c`; maintainer-settled S1 | `hardware-documented` / `emulator-policy` |
| `tests/upd9002/m96d_nop.c` | ROM1 test backing comment | The focused test populates the ROM1 backing selected for the VA F0000H-FFFFFH window rather than flat `mem[]` storage | `memoryva/memoryva.c`: ROM1 handler for the F0000H region | `hardware-documented` |
| `bios/biosmem.h` | Remaining work-area offsets | Active SASI/SCSI service state and memory-switch synchronization use these emulator-owned offsets; former simulated bootstrap offsets are absent | Current consumers in `bios/sxsibios.c` and `machine/pccore.c`; no untracked hardware path is claimed | `emulator-policy` |
| `machine/pccore.h` | Configuration and serialized-model comments | `NP2CFG` values are reset-time configuration; `PCCORE.model` is retained as a serialized compatibility byte while `model_va` selects the active VA model | Current reset and save-state declarations in `machine/pccore.c` and `machine/statsave.c` | `emulator-policy` |
| `io/sysportva.c` | System-port ownership | Runtime serial/beeper consumers and the VA port handlers use the VA system-port latch; the obsolete generic shadow and serialized section are removed in M96g | `io/sysportva.c`, `io/serial.c`, `sound/beepc.c`, and `machine/statsave.tbl`; VA system-port behavior summarized in `docs/modernization/pc88va-boot-sequence.md` | `emulator-policy` / `hardware-documented` |
| `cpu/upd9002/memory.h` | `mem[]` ownership comment | `memoryva` owns guest-visible VA decoding; `mem[]` additionally contains host font backing at `FONT_ADRS` | `cpu/upd9002/memory.h`, `font/font.h`, and live references catalogued in M96g §14.2 | `emulator-policy` |
| `cpu/upd9002/memory.c` | Allocation comment | The host allocation covers main/HMA storage and the font backing range | `cpu/upd9002/memory.c`, `font/font.h`, and M96g §14.2 | `emulator-policy` |
| `cpu/upd9002/upd9002_mn.c` | NOP opcode comment | Opcode `90h` is the plain uPD9002 NOP after the simulated-BIOS side channel was removed | `cpu/upd9002/upd9002_mn.c` opcode implementation and M96d decision | `emulator-policy` |
| `machine/pccore.h` | Configuration/model comments | DIP values are reset-latched configuration; configuration fields are reset-time state; the serialized model byte is retained while the active VA selector is separate | `machine/pccore.c`, `machine/statsave.c`, and M96f §13 | `emulator-policy` |
| `sound/ymfmbridge.h` | YMFM state API comment | The exposed state API serializes the complete OPN/OPNA bridge state used by the sound path | `sdl2/ymfmbridge.cpp` and M96g §14.7 | `emulator-policy` |
| `tests/upd9002/m96d_nop.c` | NOP regression setup comments | The regression places a NOP in the VA ROM1 backing for the historical hook address and selects the normal ROM/default path | `memoryva/memoryva.c` ROM1 mapping and M96d §9 | `emulator-policy` / `hardware-documented` |

## 9. M96d simulated-BIOS NOP-hook audit

Before M96d, the only production caller of `biosfunc()` was the `_nop()`
opcode handler in `cpu/upd9002/upd9002_mn.c`. The handler derived a physical
address from the post-fetch instruction pointer and dispatched every NOP in
the `0xf8000`-`0xfffff` range through the simulated BIOS path. M96d makes
opcode `90h` a plain NOP and preserves its existing `UPD9002_WORKCLOCK(3)`
cost. The segment-base reloads that existed only to recover from that side
channel are removed with the dispatch.

| Function or address | Baseline callers / cases | Callers after NOP-hook removal | Final decision |
| --- | --- | --- | --- |
| `biosfunc()` | `_nop()` was the only production caller; declaration and definition remain | No production caller from the CPU instruction path | Retain pending the complete simulated-BIOS audit in M96e |
| `BIOS_BASE + BIOSOFST_ITF` (`fd80:0080`) | `biosfunc()` switch case calls `bios_itfcall()` | No NOP caller; direct helper reachability is an M96e question | Retain pending M96e |
| `BIOS_BASE + BIOSOFST_INIT` (`fd80:0084`) | `biosfunc()` switch case calls `bios_memclear()`, `bios_vectorset()`, and `bios_reinitbyswitch()` | No NOP caller; direct helper reachability is an M96e question | Retain pending M96e |
| `BIOS_BASE + BIOSOFST_WAIT` (`fd80:00b4`) | `biosfunc()` switch case calls `biosboot_wait()` | No NOP caller; FDD wait reachability is an M96e question | Retain pending M96e |
| Physical `0xfffe8` | `biosfunc()` case calls `biosboot_load()` and subtracts 2000 clocks | No CPU NOP route | Retain pending M96e |
| Physical `0xfffec` | `biosfunc()` case calls `biosboot_load()` and subtracts 2000 clocks | No CPU NOP route | Retain pending M96e |

The focused test `--upd9002-m96d-nop` places a NOP in the VA ROM1 backing at
the `0xfffe8` window address, selects the ROM/default bootstrap path without
private ROM data, and verifies that one step consumes the normal three-clock
NOP cost, advances IP by one, and leaves all segment bases unchanged. This
test does not assert that a supported guest previously exercised the hook.
No supported-workload behavior difference has been observed; M96d therefore
records a removed latent simulated-BIOS NOP hook rather than a demonstrated
guest-visible bug.

## 14. M96e simulated-BIOS producer/consumer audit

The M96d change removes the only production caller of `biosfunc()`: the
physical-address side channel in the uPD9002 NOP handler. A complete tracked
source search at the M96e starting point found no other caller of
`biosfunc()`, `bios_itfcall()`, `bios_itfprepare()`, `bios_memclear()`,
`bios_vectorset()`, `bios_reinitbyswitch()`, `setbiosseed()`, or
`bios_initialize()` other than the reset call listed below. The reset call was
removed in M96e1 after this audit; native VA ROM setup remains in
`romva_initialize()`.

### 14.1 `bios_initialize()` producer/consumer map

| Produced range or value | Producer | Production consumer after M96d | CPU-visible through native VA decoder | Decision |
| --- | --- | --- | --- | --- |
| `mem + 0xe8000` (`bios.rom`, `nosyscode`, checksum seed) | `bios_initialize()` | None; no host reader found | No; VA `E0000h-EEFFFh` is backed by `rom0mem`, not flat `mem[]` | Remove simulated initializer |
| `mem + 0xfd800` (`biosfd80`, FDD format tables, key table) | `bios_initialize()` | None after `biosfunc()` removal | No; VA ROM1 is backed by `rom1mem` | Remove simulated initializer; retain generated resources as separate backlog |
| `mem + 0xfffe8` / `0xfffec` bootstrap stubs | `bios_initialize()` | None after `biosfunc()` removal | No; the VA `F0000h-FFFFFh` window reads `rom1mem` | Remove simulated initializer |
| `mem + ITF_ADRS` (`0x1f8000`) and `0x1c0000` shadow | `bios_initialize()` | None; no production reader found | No native CPU route; these are raw host `mem[]` storage | Remove simulated initializer |
| `mem + 0x1e8000` BIOS shadow | `bios_initialize()` | None; no production reader found | No native CPU route | Remove simulated initializer |
| `pccore.rom |= PCROM_BIOS` | `bios_initialize()` | None after removing the old `bios.rom` path | N/A | Remove with simulated initializer |

`machine/pccore.c` now initializes the CPU-visible native VA ROMs only through
`romva_initialize()`. `memoryva/memoryva.c` maps the F0000H-FFFFFH window to
`rom1mem` and the lower VA ROM window to `rom0mem`; it does not expose the
simulated BIOS bytes written to flat `mem[]`. Direct host reads of the audited
ranges are absent from production source. This distinguishes guest-visible
ROM backing from emulator-only flat-memory storage and avoids treating host
storage as a hardware memory map.

### 14.2 Simulated-BIOS helper reachability

| Function | Baseline callers | Callers after M96d | M96e decision |
| --- | --- | --- | --- |
| `bios_initialize()` | `machine/pccore.c:pccore_reset()` | Same single reset caller before M96e1 | Remove call and simulated initializer |
| `biosfunc()` | uPD9002 `_nop()` only | None | Remove definition with simulated BIOS C dispatch |
| `bios_itfcall()` | `biosfunc()` ITF case | None | Remove with `bios.c` |
| `bios_itfprepare()` | `bios_itfcall()` | None | Remove with `bios.c` |
| `bios_memclear()` | `bios_itfcall()` and INIT case | None | Remove with `bios.c` |
| `bios_vectorset()` | `bios_itfcall()` and INIT case | None | Remove with `bios.c` |
| `bios_reinitbyswitch()` | `bios_itfcall()` and INIT case | None | Remove with `bios.c` |
| `setbiosseed()` | `bios_initialize()` | None after reset call removal | Remove with `bios.c` |
| `msw_default[]` | `bios_itfcall()` | None | Remove with `bios.c` |
| `iodata[]` / `neccheck` | `bios_initialize()` / `bios_itfprepare()` | None | Remove with `bios.c` |

The `CPU_ITFBANK` guard in the removed `biosfunc()` is not used as evidence
for deleting the serialized CPU field; that state decision remains in M96g.

### 14.3 `biosboot.c` function-by-function disposition

At the M96e starting point all callers below were inside `bios.c`. There were
no callers from FDD, SASI, SCSI, reset, state-load, or host frontend code.
After M96e1 removed the dispatch that referenced it, the file was deleted in
M96e2.

| Function | Baseline production callers | Callers after M96d / M96e1 | Decision |
| --- | --- | --- | --- |
| `biosboot_fdd_equip()` | `bios_reinitbyswitch()`, `boot_fd()`, `boot_fd1()` through `bios.c` | None | Deleted in M96e2 |
| `biosboot_load()` | `biosfunc()` cases `0xfffe8` and `0xfffec` | None | Deleted in M96e2 |
| `biosboot_wait()` | `biosfunc()` case `BIOSOFST_WAIT` | None | Deleted in M96e2 |

`biosboot.c` is not the live FDD/SASI/SCSI device implementation. Those paths
remain under `io/`, `fdd/`, `cbus/`, and `bios/sxsibios.c` as applicable.

### 14.4 Live BIOS work-area consumers

The simulated bootstrap work-area header is reduced only after the dispatch
deletion. The following definitions have live production consumers and are
retained:

| Definition | Consumer | Classification |
| --- | --- | --- |
| `MEMW_DISK_EQUIP` (`0x055c`) | `bios/sxsibios.c` SASI initialization | Emulator representation of guest disk-equipment state |
| `MEMB_DISK_EQUIPS` (`0x0482`) | `bios/sxsibios.c` SCSI initialization | Emulator representation of guest disk-equipment state |
| `MEMX_MSW` (`0xa3fe2`) | `machine/pccore.c:pccore_cfgupdate()` | VA memory-switch synchronization state; source authority remains unresolved in this report |

All other `biosmem.h` definitions are referenced only by the removed
simulated BIOS/bootstrap path or by the M96d test's historical selector. The
test now uses a local named offset so that dead simulated-BIOS definitions do
not remain in the production header.

### 14.5 Read-only `romimage/` inventory

`romimage/` is protected by S2. This inventory records provenance and live
consumers only; every action is `READ_ONLY`.

| Generated or source payload | Included by | Generator/source | Live consumer | M96 action |
| --- | --- | --- | --- | --- |
| `romimage/bios/biosfd80.asm` | No active CMake translation unit | Historical simulated BIOS source | None after M96e | `READ_ONLY` |
| `romimage/bios/biosmain.x86` | No active CMake translation unit | Historical BIOS image source | None after M96e | `READ_ONLY` |
| `romimage/itf.asm`, `romimage/itf.mk`, `romimage/itfd.mk` | External ROM-generation workflow | VA ITF ROM source | Native ROM build workflow, outside active CMake | `READ_ONLY` |
| `romimage/hddboot.asm`, `romimage/idebios.asm` | External ROM-generation workflow | VA storage bootstrap sources | Native ROM build workflow, outside active CMake | `READ_ONLY` |
| `romimage/sasibios.asm`, `romimage/scsibios.asm` | External ROM-generation workflow | C-bus storage BIOS sources | ROM-generation workflow; emulator C-bus remains live | `READ_ONLY` |

No `romimage/` file is edited, deleted, regenerated, or replaced by M96e.

### 14.6 M96e3 work-area result

`bios/biosmem.h` now contains only `MEMB_DISK_EQUIPS`, `MEMW_DISK_EQUIP`,
and `MEMX_MSW`, plus the endian-safe access macros required by the two live
consumers. The removed definitions were referenced only by the deleted
simulated BIOS/bootstrap implementation or by the focused M96d test. The
test's historical MSW5 selector is a local constant and is not part of the
production work-area header.

## 10. Validation at baseline

| Check | Result |
| --- | --- |
| `git diff --check` | PASS |
| `python3 tools/repo/check_encoding.py --expect utf8` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `python3 tools/repo/clang_format.py` | FAIL on four pre-existing M95 UI lines in `sdl2/np2.c` and `sdl2/scrnmng.c`; no M96 source changed |
| `python3 tools/repo/find_unreferenced.py --report` | PASS, report-only: 488 sources, 417 reached, 71 unreferenced |
| `cmake --preset linux-debug` | PASS |
| `cmake --build --preset linux-debug -j4` | PASS, 179/179 |
| `build/linux-debug/sdl2/vaeg --selftest` | PASS, all selftests passed |
| `ctest --preset linux-debug` | SKIP: no test preset named `linux-debug` |
| `ctest --test-dir build/linux-debug` | SKIP: no tests registered in this non-test build |
| `CCACHE_DISABLE=1 cmake --fresh --preset mingw-cross` | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` | PASS, 725/725; warnings are pre-existing |

The MinGW build used the repository-supported `mingw-cross` preset. No
source-changing M96 work was included in either build.

M96b submilestone validation so far:

| Submilestone | Linux configure/build | Linux selftest | MinGW cross-build | Notes |
| --- | --- | --- | --- | --- |
| M96b1 | PASS | PASS | PASS | EGC/GRCG residue removed |
| M96b2 | PASS | PASS | PASS | Generic VRAM residue and stale includes removed |
| M96b3 | PASS | PASS | PASS (`ninja: no work to do`) | Legacy I/O residue removed; `cpuio` and `fdd320` retained/deferred |
| M96b4 | PASS | PASS | PASS (`ninja: no work to do`) | Utility/resource residue removed; optional `oprecord`/wave recording surfaces retained |
| M96c | PASS | PASS | PASS | Ownership comments only; callback order and dispatch behavior unchanged |
| M96d | PASS | PASS | PASS | Test-enabled build; `ctest --test-dir build/m96d-linux` 84/84 passed with one fixture-dependent skip; focused physical-address regression test passes |
| M96e1 | PASS | PASS | PASS | Removed `bios_initialize()` and `bios/bios.c`; native VA ROM initialization remains in `romva_initialize()` |
| M96e2 | PASS | PASS | PASS | Removed `biosboot.c` and `bios.h` after function-by-function zero-caller proof |
| M96e3 | PASS | PASS | PASS | Reduced `biosmem.h` to three live consumers; no simulated bootstrap offsets remain |

M96e final validation was evaluated at commit `613a8a8` before this
report-only recording commit: `cmake --build --preset linux-debug -j4` and
the SDL dummy selftest passed; `build/m96e-linux/sdl2/vaeg --upd9002-m96d-nop`
passed; `ctest --test-dir build/m96e-linux --output-on-failure` reported 83
PASS and one fixture-dependent SKIP (`vaeg_upd9002_ssts_ci_external`); and
`CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` passed. The MinGW
artifact was `build/mingw-cross/sdl2/vaeg.exe` with SHA-256
`8f52ee29298865769094400b9d2be993a322891605c8672395e7904d068dfd2a`.
Encoding, EOL, case, focused clang-format, milestone-ID, diff-check, and
unreferenced-source validators passed. The unreferenced report contains 453
sources, 413 reached, and 40 unreferenced; retained candidates and protected
`romimage/` sources are classified above.

## 12. Corrections against earlier reports

M96f verified the two corrections previously registered by M96b6. The M88
report's statement that the removed display settings remain as positional
configuration padding is no longer current: those `NP2CFG` members had no
production readers or writers and are removed by M96f2. The M84a report's
statement that `snd26opt`, `snd86opt`, and `vol_pcm` are named padding inside
`PCCORE` is also incorrect. They were members of `NP2CFG`, persisted through
the name-keyed `INITBL` table in `sdl2/ini.c`, and were not part of the raw
`PCCORE` save-state section. Both corrections are recorded here without
rewriting the historical reports.

## 13. M96f configuration evidence

### 13.1 M96f1 designated initializer gate

The positional `NP2CFG` initializer in `machine/pccore.c` was converted to
designated fields before any member deletion. The baseline and converted
initializer each name all 59 `NP2CFG` declarations. `PCCORE` was not
converted. A compiler object dump located the `np2cfg` symbol in `__data` and
extracted its 12,388-byte image before and after the conversion:

| Check | Result |
| --- | --- |
| `NP2CFG` byte count | 12,388 before / 12,388 after |
| Before image SHA-256 | `5762154eb4875b2ee1af4ac4a5736c5f09096833a47dbb606dba1cc0d57b0956` |
| After image SHA-256 | `5762154eb4875b2ee1af4ac4a5736c5f09096833a47dbb606dba1cc0d57b0956` |
| Byte comparison | identical (`cmp` exit 0) |
| Configuration round-trip | existing `ini_read`/`ini_write` selftest: `selftest: ini ok` |
| `--selftest` | all tests passed, exit 0 |

The conversion changes no `INITBL` entry or target address; the configuration
round-trip therefore exercises the same name-keyed fields and produces the
same serialized settings. The machine-verifiable gate G96f1 passed on the
M96f1 candidate.

### 13.2 M96f2 removed configuration residue

The following members had no production CMake consumer, include consumer,
symbol read/write, GUI/CLI entry, `INITBL` entry, or save-state dependency.
Their only source references were the declaration and initializer (with
`PCMODEL_VA` additionally appearing in the old generic model byte setup).

| Field | Proof of no live consumer | Decision |
| --- | --- | --- |
| `uPD72020` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `DISPSYNC` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `RASTER` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `realpal` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `LCD_MODE` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `skipline` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `skiplight` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `wait[6]` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `grcg` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `color16` | declaration/initializer only; no `INITBL`, GUI, CLI, or runtime reference | removed |
| `snd26opt` | declaration/initializer only; no sound, GUI, CLI, or `INITBL` reference | removed |
| `snd86opt` | declaration/initializer only; no sound, GUI, CLI, or `INITBL` reference | removed |
| `vol_pcm` | declaration/initializer only; no sound, GUI, CLI, or `INITBL` reference | removed |
| `PROTECTMEM` | declaration/initializer only; no memory, GUI, CLI, or `INITBL` reference | removed |
| `hdrvacc` | declaration/initializer only; no storage, GUI, CLI, or `INITBL` reference | removed |
| `hdrvroot` | declaration/initializer only; no storage, GUI, CLI, or `INITBL` reference | removed |
| `PCMODEL_VA` | same-valued generic selector; replaced by `PCMODEL_VA1` while preserving the serialized byte | removed |

The active VA selector remains `PCMODEL_VA1`/`PCMODEL_VA2` in
`PCCORE.model_va`; the serialized `PCCORE.model` byte remains in place and is
documented as a compatibility byte. `NP2CFG` is configuration state, not a
raw save-state section, so removing these members does not change the
`PCCORE` binary state layout.

### 13.3 M96f validation

The M96f source-changing candidate was rebuilt after the field removal. The
test-enabled Linux build used a separate `build/m96f-linux` directory so the
repository `linux-debug` preset remained unchanged. The MinGW build used the
repository `mingw-cross` preset with ccache disabled.

| Check | Result |
| --- | --- |
| `cmake -S . -B build/m96f-linux -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF` | PASS |
| `cmake --build build/m96f-linux -j4` | PASS |
| `ctest --test-dir build/m96f-linux --output-on-failure` | 83 PASS, 1 fixture-dependent SKIP |
| `build/m96f-linux/sdl2/vaeg --selftest` with dummy SDL drivers | PASS, all tests passed |
| `CCACHE_DISABLE=1 cmake --preset mingw-cross` | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` | PASS |
| MinGW artifact | `build/mingw-cross/sdl2/vaeg.exe`, SHA-256 `15b7a734756db5ce8ebb79345a4cc25ecf12ecaa6edeb9d1557bf3f1aa7226ba` |
| `git diff --check`, encoding, EOL, case | PASS |
| Focused `clang-format-mp-22` check for `machine/pccore.c` and `machine/pccore.h` | PASS |
| Full `tools/repo/clang_format.py` check | FAIL on unrelated pre-existing `sdl2/np2.c` and `sdl2/scrnmng.c` lines |

The full formatter failure is inherited from the baseline and is not in the
M96f diff. The report-only unreferenced-source scan remains informational; its
candidate list includes protected ROM sources and retained demo material.

## 14. M96g system-port and memory audit

### 14.1 System-port ownership (G1)

The baseline has one VA-visible system-port implementation and one redundant
one-byte shadow. `io/sysportva.c` binds the VA ports and owns `sysportva.c`;
`io/sysport.c` owns no guest port and only resets the shadow used by the
serialized `SYSTEMPORT` section. The only runtime reads of the shadow were in
`io/serial.c`, `sound/beepc.c`, and `machine/debugsub.c`; the only writes were
the VA `01CDh`/`01CFh` handlers in `io/sysportva.c`. No production path calls
`systemport_bind()`, and `systemport_reset()` is only present in the iocore
reset table.

| Consumer or lifecycle point | Before M96g1 | M96g1 result | Evidence |
| --- | --- | --- | --- |
| RS-232C receive/transmit interrupt gating | Reads `sysport.c` bits 0 and 2 | Reads `sysportva.c` bits 0 and 2 | `io/serial.c` and VA `01CDh`/`01CFh` handlers |
| Beeper gate | Reads `sysport.c` bit 3 | Reads `sysportva.c` bit 3 | `sound/beepc.c` and VA `01CDh`/`01CFh` handlers |
| Debug status dump | Prints `sysport.c` | Prints `sysportva.c` | `machine/debugsub.c` |
| Reset | `systemport_reset()` writes `sysport.c=F9h`, then `systemportva_reset()` writes VA state | Generic reset callback is no longer a runtime dependency; `systemportva_reset()` restores `F9h`, mirrors the compatibility byte, and refreshes the beeper gate | `io/iocore.c`, `io/sysportva.c` |
| Guest writes | VA handlers write VA state and mirror the shadow | Unchanged while the old state section is retained | `io/sysportva.c` |

M96g1 retained the one-byte `sysport` object and its mirrored writes so that
the pre-M96g serialized section remained byte-compatible while runtime
ownership was migrated. M96g2 removes that shadow and the `SYSTEMPORT` section
together, with an explicit state-version change. The reset callback sequence
therefore retains the VA reset at its original position while dropping only
the unused generic callback. Bind order is unchanged.

### 14.2 Memory and state-save baseline (G3-G6)

The `MEMORY` section currently declares `0x130000` bytes. `flagsave_mem()`
and `flagload_mem()` transfer `0x110000` bytes of main/HMA backing followed
by two legacy chunks: `mem + VRAM1_B` (`0x18000` bytes) and `mem + VRAM1_E`
(`0x8000` bytes). No production reader consumes either legacy chunk after
M96e; they are only serialized residue. M96g3 will remove those transfers
and the reset clears that only serve those chunks.

The live host-backed `mem[]` ranges are:

| Use | Start | Size | Exclusive end | Live after M96e |
| --- | ---: | ---: | ---: | --- |
| VA CPU main RAM + HMA backing | `0x000000` | `0x110000` | `0x110000` | yes |
| Font backing via `FONTMEMORYBIND` / `fontrom` | `0x110000` | `0x084000` | `0x194000` | yes; required by S3 |
| Legacy serialized VRAM1_B chunk | `0x1A8000` | `0x018000` | `0x1C0000` | no; removed by M96g3 |
| Legacy serialized VRAM1_E chunk | `0x1E0000` | `0x008000` | `0x1E8000` | no; removed by M96g3 |

The current allocation is `0x200000`; M96g3 must not reduce it below
`0x194000` while `fontrom` remains an alias inside `mem[]`. CPU-visible VA
ROM/GVRAM are owned by `memoryva` and are not counted as `mem[]` host font
backing. `FONT_ADRS` remains live; `VRAM*`, `VRAMADDRMASKEX`, `VRAM_STEP`, and
`ITF_ADRS` become removable only after the legacy transfers and all other
references are deleted.

The old header was `NP2STATUS_VERSION=0x080608`; M96g2 uses policy B
(explicit rejection) and bumps it to `0x080609`. M96g4 changes the FMBOARD
payload and therefore bumps the current header to `0x08060a`; the FMBOARD
section version is now `1`. The old `SYSTEMPORT` section is absent from the
new format. M96g3 removes the two legacy `MEMORY` chunks; the resulting
`MEMORY` size is `0x110000`. No pre-M96g state is silently partially loaded;
the focused state test mutates the header version and verifies rejection
before any state is applied.

### 14.3 M96g1 machine validation

The M96g1 runtime-consumer migration was built from the M96f candidate and
passed the emulator selftest. No save-state bytes or protected payloads were
changed in this substage.

| Check | Result |
| --- | --- |
| `cmake --build build/m96f-linux -j4` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m96f-linux/sdl2/vaeg --selftest` | PASS |
| `git diff --check` | PASS |

The M96g human gate remains pending until the complete state/memory change is
validated; this substage does not claim G96g.

### 14.4 M96g2 state-format validation

M96g2 removes the generic `sysport` source and header, removes its CMake and
formatter-manifest entries, removes the `SYSTEMPORT` section, and increments
the state header from `0x080608` to `0x080609`. The `SYSPORTVA` section remains
the sole serialized system-port state. The focused state-boundary test now
hashes the `0x110000`-byte active memory section and rejects a pre-M96g header
version without mutating the captured machine snapshot.

### 14.5 M96g3 memory cleanup

M96g3 removes the two obsolete reset clears and the unused `VRAM*`,
`VRAMADDRMASKEX`, `VRAM_STEP`, and `ITF_ADRS` constants. The active state
payload is `0x110000` bytes. The allocated `mem[]` bound remains `0x200000`
intentionally: the proven live high range is the font backing through
exclusive address `0x194000`, and leaving allocation headroom avoids changing
the flat-memory/test seam without a separate proof. No production reference
to the removed constants remains.

| Check | Result |
| --- | --- |
| `cmake --build build/m96f-linux -j4` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m96f-linux/sdl2/vaeg --selftest` | PASS |
| `python3 tools/repo/check_encoding.py --expect utf8` | PASS |
| `python3 tools/repo/check_eol.py --enforce` | PASS |
| `python3 tools/repo/check_case.py` | PASS |
| Focused `clang-format-mp-22` check for changed C files | PASS |

### 14.6 M96g candidate validation

The complete M96g source candidate, including the OPN/OPNA state correction,
was validated before this documentation update. Linux CTest passed; the only
skipped test requires external CI assets. The full formatter check still
reports the two unrelated pre-existing `sdl2/np2.c` and `sdl2/scrnmng.c`
violations recorded by M96f.

| Check | Result |
| --- | --- |
| `ctest --test-dir build/m96f-linux --output-on-failure` | 83 PASS, 1 SKIP, 0 FAIL |
| `CCACHE_DISABLE=1 cmake --preset mingw-cross` | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` | PASS |
| MinGW artifact | `build/mingw-cross/sdl2/vaeg.exe` |
| MinGW SHA-256 | `7d2a475550deedda4337c143aa2cc18fadef7c93eba46f26391b309f551bda57` |
| `python3 tools/repo/find_unreferenced.py --report` | PASS; informational candidates remain, including protected `romimage/` sources |
| Full `python3 tools/repo/clang_format.py` check | FAIL only on inherited `sdl2/np2.c` and `sdl2/scrnmng.c` lines |

### 14.7 M96g4 OPN/OPNA state correction

The FMBOARD section previously saved only the register image and did not
serialize the active YMFM engine. Its flag selection also omitted the VA OPN
board. M96g4 adds the complete YMFM bridge state for both VA OPN and VA OPNA,
restores it after `fmboard_bind()` has rebuilt the device map, and rejects the
older FMBOARD payload through section version `1` and
`NP2STATUS_VERSION=0x08060a`.

The existing save/load/save stability assertion now covers the serialized
YMFM state bytes: the Linux selftest passed after the change. The human gate
must still confirm audible OPN and OPNA continuation after loading a state.

## 14.8. M96h dead branches and identity cleanup

M96h was evaluated after the explicit G96g pass. The audit searched the
tracked source tree, `CMakeLists.txt`, `CMakePresets.json`, `cmake/`,
`.github/`, generated CMake/Ninja files for the Linux Debug, M96f test, and
MinGW cross builds, and the verbose per-target `DEFINES` emitted in those
build files. No supported repository build supplies `CPUSTRUC_MEMWAIT`,
`USEIPTRACE`, or `SUPPORT_OPRECORD`; `MEMOPTIMIZE` is actively defined by the
portable frontend. This does not make the three undefined macros all
removable: one has live guarded compatibility hooks, while the other two are
proven dead in the current product.

### 14.8.1 H1 macro inventory and decisions

| Macro | Definitions in supported builds | Production references | Arbiter decision | M96h action |
| --- | --- | --- | --- | --- |
| `CPUSTRUC_MEMWAIT` | None in tracked build/configuration files or generated `DEFINES` | Two guarded field/macro blocks in `cpu/upd9002/cpucore.h`; one historical readme entry | `DELETE` | Removed the guarded fields, access macros, and stale readme entry |
| `USEIPTRACE` | None in tracked build/configuration files or generated `DEFINES` | Dead declaration in `machine/pccore.h` and trace storage/recording blocks in `machine/pccore.c` | `DELETE` | Removed the dead IP-trace blocks and the unused Windows SCSI declaration |
| `SUPPORT_OPRECORD` | None in supported builds | Guarded operation-record hooks in `machine/statsave.*`, `fdd/diskdrv.c`, `io/mouseifva.c`, `io/serial.c`, and `oprecord.*` | `DEFER_INSUFFICIENT_EVIDENCE` | Retained; M72 explicitly deferred this out-of-tree/state-save compatibility surface |
| `MEMOPTIMIZE` | Active definition `sdl2/compiler.h:189` (`2`) | CPU fast-path branches in `cpu/upd9002/*` and generated operation macros | `RETAIN_LIVE` | Unchanged; this is an active optimization policy, not dead residue |

`TRACE` remains a separate supported diagnostic macro. M96h removes only the
`USEIPTRACE`-dependent storage and recording path; it does not alter the
normal `TRACE` counters or the supported VAEG integration trace option.
Historical SSTS fixture JSON may still contain old source snapshots; those
fixtures are provenance data, not compiled definitions.

### 14.8.2 H1 two-reviewer audit

| Item | Reviewer A - reduction advocate | Reviewer B - preservation advocate | Arbiter | Evidence |
| --- | --- | --- | --- | --- |
| `CPUSTRUC_MEMWAIT` | No definition, active include, or consumer exists; guarded struct members are absent from production layout | An out-of-tree build could have enabled it historically, so prove every supported CMake/Ninja definition is absent before deleting | `DELETE` | Tracked-source and build-definition search; Linux/MinGW/M96f `DEFINES`; `cpucore.h` was the only code owner |
| `USEIPTRACE` | No definition or caller exists; the `pccore` blocks are compile-dead and the SCSI declaration has no call site | Trace code is diagnostic and could be useful, but no supported build enables it and the remaining `TRACE` path is independent | `DELETE` | Tracked-source/build search; `rg` call census; normal `TRACE` blocks retained |
| `SUPPORT_OPRECORD` | Undefined in the product build and appears removable | M72 documented operation-record state/device hooks as an out-of-tree compatibility surface requiring a focused audit | `DEFER_INSUFFICIENT_EVIDENCE` | M72 report/task; guarded references remain in state and device code |
| `MEMOPTIMIZE` | Several branches look legacy and could be simplified | `sdl2/compiler.h` actively defines level 2 and CPU code consumes it | `RETAIN_LIVE` | Active definition plus compiled CPU references; no removal |
| `NP2VER_CORE` and selector comments | User-visible output still exposed stale Neko Project II identity | Save-state/header names and legal provenance are compatibility/history and must remain; only active identity should change | `DELETE` active macro/comments, `RETAIN` format/history | `np2ver.h`, CLI, About dialog, `np2info`; `VA-EG` state signature and `NP2FHDR` untouched |

### 14.8.3 H2 VAEG identity result

`np2ver.h` no longer defines the obsolete `NP2VER_CORE "ver.0.80"` value or
the commented platform-selector list. `VAEGREL_CORE` remains the build-date
release identity. The historical `np2version` symbol is retained to avoid a
broad `np2*` rename, but its value now comes from `VAEGREL_CORE`, so the
runtime information panel reports the VAEG release rather than an NP2 core
version. CLI help/version output now prints one VAEG release string, and the
About dialog identifies the `PC-88VA emulator core` without the stale Neko
Project II version claim. The `VA-EG` save-state signature and `NP2FHDR`
layout are unchanged.

### 14.8.4 H3 comment scope

Only files already changed by M96h were touched. The removed trace block also
removed its `Shinra` and `@@@@@@` diary markers. No repository-wide comment
translation was attempted, and no source comment cites maintainer-local
hardware-document paths. Existing legacy-provenance documentation remains
unchanged because it describes licensing/history rather than active emulator
identity.

### 14.8.5 M96h source commits and validation

| Commit | Concern | Result |
| --- | --- | --- |
| `2835907` | Remove proven-dead `CPUSTRUC_MEMWAIT` and `USEIPTRACE` branches; remove stale diary markers and readme flag entry | Source/build validation passed |
| `d524c44` | Remove stale NP2 core identity from active version/header/UI paths while retaining VAEG release information | Source/build validation passed |

The M96f test build was rebuilt after both source commits. The build emitted
only pre-existing warnings (including the known `fontrom` `sizeof` warning);
no M96h warning or error was introduced. The full repository formatter still
reports the inherited `sdl2/np2.c` formatting issue outside the M96h lines.
The M96h audit also corrected the M96 row in `ROADMAP.md`: the strict
milestone validator requires the row's canonical gate `G96`, while the staged
sub-gate history is recorded in this report. No source or behavior change is
associated with that documentation-only correction.

| Check | Result |
| --- | --- |
| `cmake --build build/m96f-linux -j4` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/m96f-linux/sdl2/vaeg --selftest` from a temporary directory | PASS, exit 0; all tests passed |
| `CCACHE_DISABLE=1 cmake --preset mingw-cross` | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` | PASS |
| MinGW artifact | `build/mingw-cross/sdl2/vaeg.exe` |
| MinGW SHA-256 | `151eb94844205fbbecff82d2e16dd450f86f143d383cbbcbc7ebc6638ae69da9` |
| `ctest --test-dir build/m96f-linux --output-on-failure` | 83 PASS, 1 fixture-dependent SKIP, 0 FAIL |
| `build/m96f-linux/sdl2/vaeg --version` | `88VA Eternal Grafx Rel.20260818`; no legacy NP2 version suffix |
| `python3 tools/repo/check_encoding.py --expect utf8` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null git diff --check` | PASS |
| `python3 tools/repo/find_unreferenced.py --report` | PASS; 451 sources, 411 reached, 40 unreferenced; candidates remain classified/backlog or protected |
| Focused `clang-format-mp-22` on all M96h-changed C/C++ files | PASS |
| `python3 tools/repo/clang_format.py` | FAIL only on inherited pre-existing lines in `sdl2/np2.c` and `sdl2/scrnmng.c` |
| `vaeg_milestone_id_selftest` | PASS after the `ROADMAP.md` gate-row correction |

## 14.9. M96i final structural audit

M96i was evaluated against the M96 baseline commit
`dfe50a1420c075040c12b96f00c315b5987a846a` and the pre-audit candidate
`12dcc1d80aa0db708763d0f626dfdf897788cf4a`. This is a report-only audit;
the source and build inputs used for the M96h validation were unchanged.

### 14.9.1. I1 reachability census

`python3 tools/repo/find_unreferenced.py --report` reports 451 sources, 411
reached, and 40 unreferenced at the M96h candidate. The remaining list is not
a new deletion verdict. It consists of the already classified compatibility
and demo material (`io/cpuio.*`, `io/np2vasup.*`, `oprecord.c`, and the SGP
milestone sources), protected/read-only ROM generators and payloads under
`romimage/`, and the standalone hardware probe under
`tools/hardware/pc88va_rep0f/`, plus `bios/fdfmt.h` and
`common/wavefile.c` retained for the backlog. Every item is therefore either
covered by the M96b reviewer decision, protected by S2, or explicitly
deferred; none is silently promoted to `DELETE` by this final census.

### 14.9.2. I2 protected payload audit

The evaluated diff has no path under `romimage/` and no modified ROM, disk,
font, icon, cursor, or wave payload. `git diff --name-status ... -- romimage`
is empty. The only generated payload change in the M96 history is the
approved deletion of `cbus/scsibios.res` in M96b4; it is not under the S2
protected `romimage/` tree and is not C-bus device support. The final audit
therefore leaves every protected payload unchanged.

### 14.9.3. I3 C-bus boundary

The live C-bus boundary remains intact. `cbus/cbuscore.c` still owns reset and
bind callbacks for SASI, SCSI, MPU98II, and BMS, and those devices register
their CPU-visible ports through the canonical `io/iocore.c` map. The callback
order is unchanged from M96c for both cold reset and state-load reconstruction:

| Path | Reset order | Bind order | M96i result |
| --- | --- | --- | --- |
| Cold reset | `iocore_reset()` -> `cbuscore_reset()` -> `fmboard_reset()` -> `upd9002_memorymap_va()` -> `iocore_build()` -> `iocore_bind()` -> `cbuscore_bind()` -> `fmboard_bind()` | As shown | `RETAIN_HARDWARE_BOUNDARY` |
| State-load reconstruction | `iocore_reset()` -> `cbuscore_reset()` -> `fmboard_reset()` -> section restore -> `upd9002_memorymap_va()` -> `iocore_build()` -> `iocore_bind()` -> `cbuscore_bind()` -> `fmboard_bind()` | As shown | `RETAIN_HARDWARE_BOUNDARY` |

No C-bus source or device was removed in M96i. The retained names `sasiio`,
`scsiio`, `scsicmd`, `mpu98ii`, and `cbus` remain live VA ownership names;
they are not PC-9801 residue.

### 14.9.4. I4 final dispatch table

| Dispatch or selector | Baseline status | Final status | VA reason |
| --- | --- | --- | --- |
| `PCMODEL_VA1` / `PCMODEL_VA2` | Live model selection | `RETAIN_LIVE` | VA1/VA2 ROM and reset behavior remain selectable |
| `CPU_ITFBANK` | CPU context field; old writer was `io/necio.c` | `RETAIN_COMPATIBILITY` | State/context layout is preserved; no production writer remains |
| `SUPPORT_OPRECORD` | Guarded out-of-tree/state hooks | `DEFER_INSUFFICIENT_EVIDENCE` | M72 compatibility surface was not audited in M96 |
| `MEMOPTIMIZE` | Active portable CPU optimization | `RETAIN_LIVE` | Defined by `sdl2/compiler.h` and consumed by compiled CPU code |
| `cbuscore` callbacks | Live VA device lifecycle tier | `RETAIN_HARDWARE_BOUNDARY` | Owns C-bus devices; it is not a CPU-model selector |
| `biosfunc()` and physical-address NOP hook | Simulated-BIOS side channel | `DELETE` | M96d/e removed the hook and unreachable simulated BIOS C path |
| `PCMODEL_VA`, `CPUSTRUC_MEMWAIT`, `USEIPTRACE` | Obsolete/dead selectors or branches | `ALREADY_ABSENT` | Removed by M96f/M96h after build-definition and reachability evidence |
| `SUPPORT_PC98*`, `PC9821*`, `PC9801*`, `iocore16` runtime selectors | No active production selector | `ALREADY_ABSENT` / `READ_ONLY` | Only historical docs or the unreferenced table artifact remain |
| `romimage/` generators and payloads | Read-only source/payload tree | `READ_ONLY_PROTECTED` | S2 prohibits M96 edits |

### 14.9.5. I5 comment/source reconciliation

The comment map in section 11 now covers every behavior-bearing comment added
by M96 to a changed source or test file. Comments that describe host storage,
state serialization, or compatibility policy are explicitly classified as
`emulator-policy`; no source comment cites the maintainer-local
`docs/tekumani/` or ROM-disassembly paths. Copyright headers and untouched
historical comments are not treated as behavioral evidence entries.

### 14.9.6. M96i conclusion

M96i found no additional safe source deletion or dispatch flattening. The VA
I/O map, live C-bus ownership tier, VA1/VA2 selector, and protected ROM
workflow are all explicit and preserved. The remaining unreferenced list is
classified backlog/compatibility/protected material rather than an unreviewed
cleanup target. The maintainer passed G96i on the `e311ea5` candidate. M96
still has no completion claim until the final `G96` is explicitly passed.

## 15. Human gates

| Gate | Evaluated commit | Result | Maintainer statement |
| --- | --- | --- | --- |
| G96a | `fd1214e3584b5cc21e1076f6f1ce0f956de72cc8` | **PASS** | Maintainer: human gate passed |
| G96b | `78ac500` | **PASS** | Maintainer: human gate passed |
| G96c | `f31fb45` | **PASS** | Maintainer: human gate passed |
| G96d | `11038588b491ca8e250df9ced8ccf821494def28` | **PASS** | Maintainer: human gate passed |
| G96e | `613a8a8` | **PASS** | Maintainer: human gate passed |
| G96f | `75d088a` | **PASS** | Maintainer: human gate passed; SCSI support-disk path checked |
| G96g | `6374cb1` | **PASS** | Maintainer: G96g passed |
| G96h | `fc8afbd2fa46e5df6ba3163590cd2ff39f626a80` | **PASS** | Maintainer: G96h human gate passed |
| G96i | `e311ea583397f04d9489e82897405fe139e2c3aa` | **PASS** | Maintainer: G96i human gate passed |
| G96 | `74c859111951f22e86b2bd6453a5f07b040fd6da` | **PASS** | Maintainer: G96 human gate passed |

M96b completed after the maintainer recorded G96b as passed. M96c completed
after G96c. The maintainer then passed G96d for the M96d candidate and G96e
for the M96e candidate. G96f, G96g, G96h, G96i, and final G96 were each
recorded against their evaluated candidates. M96 is closed.

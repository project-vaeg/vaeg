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

# M91: native V3 VA single-path report

## Status

M91 implementation is complete on `topic/m91-va-single-path` at
`b20da74a4fab80877b7449f82e766ade6782615a`. Automated validation passes.
The maintainer passed G91 on 2026-08-15 against candidate
`66fbf20f6f5a8fbe107884ca1223acc53d352cbd`; M91 is closed.

## Evidence boundary

The maintainer-provided `docs/tekumani` and `docs/98io` trees are local,
read-only reference material and are not copied into this branch. Tekumani is
the authority for built-in PC-88VA functions. The PC-98 material is used only
to identify inherited implementations and same-number semantic collisions.
A shared port number is not evidence that two devices have the same behavior.

## Implemented single path

| Area | M91 result |
| --- | --- |
| I/O dispatch | `io/iocore.c` now owns one 16-bit VA map. The `iomode_va` selector and separate common/VA attach APIs are gone. Word I/O dispatches the low byte and then the adjacent high byte, including a `00FFH` to `0100H` page crossing. |
| Mixed port bindings | Removed inherited PC-98 aliases from DMA, FDC, serial, PIC, PIT, calendar/system-port, SASI, SCSI, BMS, EMS, NP2SYSP, and sound-extension registration. VA and separately owned expansion routes remain. |
| Memory | Public CPU and DMA access always enters `memoryva`. The `memmode_va` selector is gone. Explicit raw main-RAM helpers are used only by VA map entries. Direct instruction-memory bypass thresholds are disabled so mapped TVRAM and BMS windows cannot be skipped. |
| Test isolation | CPU-only semantic tests may explicitly request a flat 20-bit fixture through a test-build-only seam. Production builds contain no alternate machine-mode selector. The M68 mapped-memory test continues to exercise the VA decoder. |
| CGROM | The V3 `014CH`-`014FH` route in `io/cgromva.c` is canonical. The inherited PC-98 CGROM window was removed. |
| Initialization | Reset and bind tables now initialize only the VA/shared active device set. Inactive inherited sources remain outside CMake unless deletion was explicitly approved. |
| State save | Retired PC-98 ARTIC and NECIO sections and callbacks are no longer registered. |

## Tekumani and PC-98 comparison

- Tekumani assigns V3 Kanji CG access to `014CH`-`014FH`: a hardware
  character code, raster/left-right selection, and font read or user-defined
  glyph write access. Comments in `io/cgromva.c` now describe those fields and
  distinguish documented behavior from the existing font-image layout.
- Tekumani assigns `0160H`-`016FH` to the VA DMA controller. Comments identify
  the implemented registers, unimplemented device-control pair, mode bits,
  and the current base/current register limitation.
- Tekumani specifies a 600 ms FDD motor wait. The existing 505 ms emulator
  value is retained and explicitly labelled as an approximation; M91 does not
  change timing.
- Tekumani documents keyboard command classes at `0197H`. The implementation
  comment records both the documented class bits and the VA ROM command values
  actually accepted by the model.
- `docs/98io/io_tstmp.txt` identifies the inherited ARTIC timestamp/wait
  device at `005CH`, `005EH`, and `005FH`. Tekumani assigns those addresses to
  VA1/V2 GVRAM selection/status with different semantics. M91 therefore
  removes ARTIC without inventing the deferred VA1/V2 behavior.

## Deletion and retention boundary

The only physically deleted sources are the four files explicitly approved by
the maintainer:

- `io/artic.c`
- `io/artic.h`
- `io/cgrom.c`
- `io/cgrom.h`

The following inactive PC-98 closure is retained in the source tree but is no
longer part of the portable CMake build: `cpu/upd9002/egcmem.c`,
`io/cpuio.c`, `io/dipsw.c`, `io/egc.c`, `io/fdd320.c`, `io/necio.c`,
`io/np2vasup.c`, `io/printif.c`, and `vram/vram.c`. Their physical deletion
requires separate maintainer approval.

## Comment audit

Comments changed by M91 are English and describe native VA behavior rather
than translating inherited PC-98 labels. They explicitly mark documented
semantics, observed ROM behavior, emulator approximations, and unimplemented
features. A Japanese-character census over every modified C, C++, header, and
macro source returned no matches. Repository encoding, EOL, and case checks
also report zero violations. Unrelated untouched sources are outside this
comment-only census.

## Validation

| Check | Result |
| --- | --- |
| `git diff --check` | PASS |
| `python3 tools/repo/check_encoding.py --expect utf8` | PASS, 0 violations |
| `python3 tools/repo/check_eol.py --enforce` | PASS, 0 violations |
| `python3 tools/repo/check_case.py` | PASS, 0 findings |
| `python3 tools/qa/upd9002_protected_deletion.py --root .` | PASS |
| `python3 tools/qa/upd9002_native_invariant.py --root .` | PASS; production selectors absent |
| `python3 tools/qa/m75_scsi_controller.py --root .` | PASS |
| `cmake --build --preset macos-macports -j4` | PASS |
| `build/macos-macports/sdl2/vaeg --selftest` | PASS, all selftests |
| M61, M62, M65c, M68 focused CPU tests | PASS |
| `ctest --test-dir build/m91-tests --output-on-failure` | PASS, 0 failures among 83 registered tests; 1 external SST corpus test explicitly skipped because no corpus was supplied |
| `CCACHE_DISABLE=1 cmake --preset mingw-cross` and `cmake --build --preset mingw-cross -j4` | PASS |

The macOS executable SHA-256 is
`fadfa1f4b776d86e607eef5824e1fd46227ad1efdb7ae8550d0601b3f8cfdbe9`.
The MinGW executable SHA-256 is
`0d1e4038593c5b72ceb7a8a403ec5f3763544b41b710a09bd13e26417b184cd3`.
Warnings printed by these builds are pre-existing warning classes; no M91
compile or link failure remains.

## Gate

The maintainer explicitly reported G91 passed on 2026-08-15 against candidate
`66fbf20f6f5a8fbe107884ca1223acc53d352cbd`. The approval covers the required
clean-checkout native V3 boot, bundled VA demo, OS boot, and simple FDD,
storage, keyboard, display, sound, and state-save operations. M91 is closed;
this documentation-only closure does not claim additional automated testing.

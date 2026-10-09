<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# Legacy common-BIOS hook reachability on PC-88VA

## Scope and result

This audit examines the former `bios09.c`, `bios0c.c`, `bios12.c`, and
`bios13.c` handlers and the retained `biosboot.c` helpers in the VA-only product.
It combines static call-graph inspection, the PC-88VA hardware descriptions in
Tekumani, the PC-98 interface descriptions in `docs/98io/`, and a bounded boot
of `docs/disks/pcengine110-bootonly.d88`.

The evaluated source baseline was
[`080aff95722e89b1c0973f32e52b7afe56d54440`](https://github.com/nakatamaho/vaeg/commit/080aff95722e89b1c0973f32e52b7afe56d54440).
The result is:

- remove `bios09.c`, `bios0c.c`, `bios12.c`, and `bios13.c` and their C
  dispatch cases; they implement the simulated PC-98 common-BIOS path and are
  not reached by the VA ROM or PC-Engine boot;
- retain `biosboot.c`; its public INT 1Bh dispatcher was already removed in M81,
  but the remaining file provides emulator-internal FDD equipment,
  bootstrap-load, and completion-wait helpers that still have direct callers.

The interrupt numbers alone are not a removal test. Tekumani assigns INT 09h
to the VA keyboard, INT 0Ch to VA RS-232C, and INT 13h to the VA D765 FDC.
The removed C handlers are obsolete because their *implementation and entry
route* belong to the simulated common BIOS, not because those interrupt
numbers are absent on VA.

## Runtime layers

The relevant execution layers are:

1. [`machine/pccore.c`](../../machine/pccore.c) initializes the simulated
   common BIOS and then the native VA ROM mapping.
2. [`cpu/upd9002/upd9002_mn.c`](../../cpu/upd9002/upd9002_mn.c) calls
   `biosfunc()` only when the main CPU executes a NOP in physical
   `F8000h-FFFFFh`.
3. [`bios/bios.c`](../../bios/bios.c) formerly recognized fixed simulated-BIOS
   NOP addresses `FD80:0088`, `FD80:008C`, `FD80:0090`, and `FD80:0094` and
   called the four C handlers.
4. [`romimage/bios/biosfd80.asm`](../../romimage/bios/biosfd80.asm) contains
   the corresponding fixed-offset stubs and an old common-BIOS vector table.
5. During a supported VA boot, the native ROM installs its own IVT targets.
   PC-Engine later replaces the keyboard and serial targets with RAM handlers.

After removal, the assembly labels at the fixed offsets remain layout
reservations in the simulated BIOS payload. They do not reach C handlers
because `biosfunc()` no longer dispatches those addresses. Removing or
regenerating the larger simulated-BIOS assembly payload is a separate cleanup;
it is not needed to remove these four C implementations.

## Per-file call graph and function

| Source | Entry route before removal | Function implemented | VA-only decision |
| --- | --- | --- | --- |
| `bios09.c` | `biosfunc(FD80:0088)` called `bios0x09()`; simulated reset/boot entries at `FD80:0080/0084` called `bios0x09_init()` | Resets the PC-98-style keyboard interface through port `43h`; the preceding assembly vector reads port `41h` and passes the code in `AL`; initializes the common BIOS key table and ring buffer, converts make/break codes, and updates shift state | Remove. The active VA keyboard device and IRQ are in [`io/serial.c`](../../io/serial.c); the VA ROM and PC-Engine never vector through `FD80:0088` in the measured boot. |
| `bios0c.c` | `biosfunc(FD80:008C)` called `bios0x0c()` | Reads a common 8251 receive/status path through `30h/32h/33h`, stores data/status pairs in the common BIOS RS buffer, performs SI/SO and XON/XOFF processing, then acknowledges the PIC | Remove. Tekumani assigns VA USART data/control to `20h/21h`; on VA, `30h` and `32h` are unrelated system/video controls. The active VA RS-232C device and IRQ path are in [`io/serial.c`](../../io/serial.c). |
| `bios12.c` | `biosfunc(FD80:0090)` called `bios0x12()` | Drains D765 result bytes from the PC-98 640-KiB FDD interface at `C8h/CAh`, updates common BIOS result flags, and acknowledges the slave/master PIC | Remove. `docs/98io/io_fdd.txt` identifies `C8h/CAh/CCh` as the PC-98 640-KiB interface. Tekumani assigns VA INT 12h to bus UINT4, not the FDC. |
| `bios13.c` | `biosfunc(FD80:0094)` called `bios0x13()` | Drains D765 result bytes from the PC-98 1-MiB FDD interface at `90h/92h`, updates common BIOS result storage, and acknowledges the PIC | Remove. `docs/98io/io_fdd.txt` identifies `90h/92h/94h` as the PC-98 interface. VA uses `1B8h/1BAh` in DMA mode and its FDC subsystem in intelligent mode; both active paths are implemented in [`io/fdc.c`](../../io/fdc.c) and [`io/fdsubsys.c`](../../io/fdsubsys.c). |
| [`biosboot.c`](../../bios/biosboot.c) | Direct calls from `bios.c`: `biosboot_fdd_equip()` during simulated BIOS initialization, `biosboot_load()` at physical `0xFFFE8/0xFFFEC`, and `biosboot_wait()` at `FD80:00B4` | Maintains the FDD equipment word; probes and reads FDD boot sectors through the emulator FDD backend; tries SASI/SCSI boot blocks through `sxsi_read()`; and stalls/retries until FDD motor/result completion | Retain. No public INT 1Bh dispatcher remains, but these internal helpers still have named callers. The selected boot did not exercise them; that bounded result does not prove all boot-switch/media fallback paths unreachable. |

## Hardware and implementation comparison

The primary sources separate valid VA interrupt assignments from obsolete
common-BIOS device access:

| Facility | Tekumani VA definition | Removed implementation | Active VAEG path |
| --- | --- | --- | --- |
| Keyboard | INT 09h; key-code port `1C1h` and keyboard matrix/control ports | Ports `41h/43h` and common BIOS buffer/key tables | `keyboard_bind()` and IRQ 1 in `io/serial.c` |
| RS-232C | INT 0Ch; D8251 data/control ports `20h/21h` | Ports `30h/32h/33h` and common BIOS RS work area | `rs232c_bind()` and IRQ 4 in `io/serial.c` |
| FDC | INT 13h; intelligent mode at reset, or D765 ports `1B8h/1BAh` in DMA mode | PC-98 D765 ports `C8h/CAh` and `90h/92h` | `fdsubsys` intelligent-mode path or `fdc_bind()` DMA-mode path |
| INT 12h | UINT4 (bus slot interrupt) | PC-98 640-KiB FDC result service | No VA FDC role |
| INT 1Bh | Reserved/default in the observed VA1 IVT; public VA disk BIOS uses INT 80h/81h | Public common INT 1Bh dispatcher already removed in M81 | Only the internal bootstrap/equipment/wait helpers remain in `biosboot.c` |

References used were `docs/tekumani/2.TXT`, `docs/98io/io_kb.txt`, and
`docs/98io/io_fdd.txt`. The emulator source agrees with Tekumani: the VA FDC
binds `1B0h-1BAh`, and the VA serial/keyboard implementation raises the PIC
lines that map to INT 0Ch and INT 09h.

## Bounded PC-Engine boot observation

A temporary, uncommitted debug-harness extension recorded selected IVT entries
and fixed-address execution counters. It did not change CPU, memory, BIOS,
I/O, or media behavior. Each run mounted a fresh disposable copy of the
task input D88; configuration and backup-memory persistence were disabled.
`cmp` verified both that no run modified its disposable D88 and that the repository input still matched the
master copy after the runs.

The IVT developed as follows. Targets are segment:offset values read from guest
address `0000:(interrupt * 4)` after the specified completed frame.

| Frame | INT 09h | INT 0Ch | INT 12h | INT 13h | INT 1Bh |
| ---: | --- | --- | --- | --- | --- |
| 1 | `0000:0000` | `0000:0000` | `0000:0000` | `0000:0000` | `0000:0000` |
| 60 | `F000:199B` | `F000:199B` | `F000:19A6` | `F000:19A6` | `F000:19A5` |
| 300 | `F000:69F1` | `F000:9200` | `F000:19A6` | `F000:0296` | `F000:19A5` |
| 1800 | `19E3:36DA` | `19E3:C050` | `F000:19A6` | `F000:0296` | `F000:19A5` |

`F000:19A5` is the VA1 default IRET target already identified by the M81 audit.
The transition of INT 09h and INT 0Ch to segment `19E3` shows PC-Engine/DOS
installing its own RAM handlers. INT 13h remains on the VA ROM handler. None
of these targets is in segment `FD80`.

At frames 1, 60, 300, and 1800, every one of the following counters was zero:

- simulated common-BIOS reset/initialization: `FD80:0080`, `FD80:0084`;
- removed handler entries: `FD80:0088`, `FD80:008C`, `FD80:0090`,
  `FD80:0094`;
- retained `biosboot.c` hooks: `FD80:00B4`, physical `0xFFFE8`, and physical
  `0xFFFEC`.

The reset-vector counter at `F000:FFF0` was one. Repeating the 1800-frame run
after removing the four C handlers produced the same final IVT and counter
values. This is evidence for this VA1 PC-Engine boot path, not a universal
claim about every ROM revision, boot switch, or disk controller mode.

## Source changes

The cleanup:

- deletes `bios/bios09.c`, `bios/bios0c.c`, `bios/bios12.c`, and
  `bios/bios13.c`;
- removes the matching `biosfunc()` dispatch cases and the obsolete keyboard
  initializer calls from `bios/bios.c`;
- removes their offsets and declarations from `bios/bios.h`;
- removes the four files from `CMakeLists.txt` and the clang-format manifest;
- leaves `bios/biosboot.c` and its three emulator-internal entry routes intact.

## Validation

The candidate was built and exercised in a clean copy of the evaluated tree:

```text
cmake --preset macos-macports \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_TESTS=ON
cmake --build --preset macos-macports -j4
build/macos-macports/sdl2/vaeg --selftest
```

The build completed, and the self-test ended with `selftest: all tests passed`.
The D88 boot was bounded at frames 1, 60, 300, and 1800 using a fresh
disposable D88 copy for every run plus `--debug-script`, `--no-cfg`,
`--no-bkupmem`, `--fdd2 none`, and `--nowait`. The diagnostic
counter and IVT results are reported above. A rendered BMP was emitted at the
1800-frame bound, but this audit does not count an automated screen dump as a
human visual gate.

## Limitations and follow-up boundary

- The bounded PC-Engine run does not exercise every VA1/VA2 ROM revision,
  direct-DMA FDC mode, serial receive case, boot switch, or alternate medium.
- Retaining `biosboot.c` is conservative because it has explicit internal
  callers and boot-media-dependent branches. Removing it requires a separate
  audit of the entire simulated bootstrap fallback, not inference from one
  zero-hit run.
- The fixed-offset common-BIOS assembly payload remains. Its broader removal
  should be handled with the rest of `BIOS_SIMULATE`, with regenerated payload
  evidence and dedicated fallback-boot testing.

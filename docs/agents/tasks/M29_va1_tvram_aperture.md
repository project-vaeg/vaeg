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
# M29: Correct the VA1 TVRAM aperture

Status: implementation complete; focused G29 boot check passed; VA2 regression corrected in M31

Date: 2026-07-13

## Scope

Correct the CPU-visible system-memory bank-1 range in the active C memory
implementation. PC-88VA TVRAM occupies 64KB at `A0000H-AFFFFH`; the remainder
of the 256KB system-memory window, `B0000H-DFFFFH`, is unused in bank 1.

This task does not change the frozen assembly reference, resize the legacy
`textmem` storage object, alter GVRAM or other system-memory banks, or add a
software-specific workaround for PC-Engine.

## User-visible failure

A clean PC-Engine 1.00 system disk behaved differently by boot model:

- VA1 did not complete startup.
- VA2/VA3 booted from the same disk image.
- PC-Engine 1.05 could boot in VA1, so the failure was specific to the older
  memory-detection and VA1 ROM path rather than a general FDD failure.

The compared D88 images did not establish a media CRC failure, and the same
PC-Engine 1.00 image booted in VA2/VA3 mode. This moved the investigation from
the FDC path to model-specific CPU memory behavior.

## Hardware memory map

The maintainer-provided PC-88VA technical material records these ranges:

| Resource | CPU range | Size |
|---|---:|---:|
| Main RAM | `00000H-7FFFFH` | 512KB |
| Optional/banked RAM window | `80000H-9FFFFH` | 128KB |
| TVRAM, system-memory bank 1 | `A0000H-AFFFFH` | 64KB |
| Unused part of bank 1 | `B0000H-DFFFFH` | 192KB |
| GVRAM, system-memory bank 4 | `A0000H-DFFFFH` | 256KB |

The system-memory window is 256KB wide because other banks use the full
window. It does not imply that every bank contains 256KB. In particular,
bank 1 has only the 64KB TVRAM aperture.

This distinction was already noted during M9 in
`reports/m9_memoryva_map.md`, but M9 deliberately transliterated the frozen
assembly behavior and kept its 256KB `textmem` access. M29 supplies the guest
software evidence that the legacy behavior was observably incorrect.

## PC-Engine 1.00 memory probe

The PC-Engine 1.00 boot code writes a segment-derived test word near the end
of successive 128KB regions and reads it back. The observed probe addresses
were:

```text
5FFF0H
7FFF0H
9FFF0H
BFFF0H
DFFF0H
```

Before M29, every write through `DFFF0H` read back successfully. The active
implementation dispatched all `A0000H-DFFFFH` bank-1 accesses to `textmem`,
whose legacy allocation is `0x40000` bytes. PC-Engine therefore mistook the
banked system-memory window for contiguous writable main RAM and selected a
stack at approximately `SS:SP=D000:FFFx`.

On hardware, the bank-1 probe must stop at `BFFF0H`: it is above the end of
TVRAM and reads as an unimplemented bus region rather than preserving the test
word.

## First destructive bank transition

The VA1 ROM later accesses backup memory by saving the current memory-map
register, selecting system-memory bank 9, reading the backup location, and
restoring the saved register. The relevant control flow observed around
`F000:B650` is:

```asm
mov dx,0152h
in  ax,dx
push ax
and ah,0f0h
or  ah,09h
mov es,0b000h
mov di,01fc4h
cli
out dx,ax
mov bl,es:[di]
pop ax
out dx,ax
sti
```

Port `0152H` is accessed as a word; its high byte corresponds to port
`0153H`, which contains the system-memory bank selection in its low nibble.

With the incorrectly selected D000 stack, this sequence proceeds as follows:

1. `PUSH AX` writes the saved map value through bank 1 at physical
   `DFFFxH`. The emulator incorrectly accepts this as TVRAM.
2. `OUT DX,AX` changes the system-memory bank from 1 to 9.
3. The same D000 stack address now refers to bank 9, not the bytes written in
   bank 1.
4. `POP AX` reads `FFFFH` from the bank-9 mapping at that address.
5. The restore `OUT DX,AX` therefore selects system-memory bank `0FH` instead
   of bank 1.
6. Subsequent stack writes and interrupt frames go to an unimplemented bank.
   The later repeated interrupt-vector 05 activity is a consequence of the
   corrupted execution state, not the initiating defect.

The trace captured the decisive transitions:

```text
system bank 01 -> 09 at F000:B665
system bank 09 -> 0F at F000:B66A
stack writes near DFFF0H while bank 0F is selected
interrupt vector 05 after the bank and stack corruption
```

VA2/VA3 did not enter this destructive VA1 ROM path in the comparison trace;
its system bank remained valid and the disk booted.

## Root cause

The root cause was not BEEP generation, sound scheduling, FDC media errors,
CPU interrupt entry, or an incorrect ROM checksum. It was an oversized
CPU-visible TVRAM aperture inherited from the frozen assembly implementation:

```text
Documented bank-1 range: A0000H-AFFFFH (64KB)
Legacy emulated range:   A0000H-DFFFFH (256KB)
```

The `textmem[0x40000]` allocation made the incorrect range appear stable to a
write/read memory probe. PC-Engine 1.00 then used a banked address for its
stack, exposing the error when VA1 ROM temporarily selected bank 9.

## Correction

`cpucva/memoryva.c` now applies a 64KB range check to all four bank-1 CPU
access helpers:

- byte write;
- word write;
- byte read;
- word read.

The behavior is:

| Address | Read | Write |
|---|---|---|
| `A0000H-AFFFFH` | TVRAM data | update TVRAM |
| `B0000H-DFFFFH` | `FFH`/`FFFFH` | ignored |
| word starting at `AFFFFH` | low byte from TVRAM, high byte `FFH` | low byte written, high byte ignored |

The backing object remains `textmem[0x40000]`. Shrinking it was unnecessary
for the CPU aperture correction and would create avoidable ABI, state, and
display-code risk. The range is enforced where CPU bank-1 accesses enter the
memory implementation.

## Post-G29 VA2 compatibility correction

Later M31 testing identified a regression that the focused G29 gate did not
cover. M28 commit `455c7d5` could enter and use VA2 V3 BASIC, while its direct
child M29 commit `c17d64a` froze after entering BASIC. The only runtime change
in that commit was the model-independent bank-1 TVRAM clamp.

The active implementation therefore applies the 64KB aperture correction only
to `PCMODEL_VA1`. `PCMODEL_VA2`, which is also the VA3 compatibility path,
exposes the full 256KB bank-1 backing. The local technical-manual text describes
the original VA, while NEC's product specifications list 64KB of text VRAM for
the [PC-88VA](https://support.nec-lavie.jp/support/product/data/spec/cpu/b047-1.html)
and 256KB for both the
[PC-88VA2](https://support.nec-lavie.jp/support/product/data/spec/cpu/b048-1.html)
and [PC-88VA3](https://support.nec-lavie.jp/support/product/data/spec/cpu/b049-1.html).
The [PC-88VA hardware comparison](http://www.pc88.gr.jp/~va/va-hard.html#mem)
independently records the same model split. The product specifications establish
the capacity; the active `A0000H-DFFFFH` decode is additionally supported by
the VA2 V3 BASIC regression result.

ROM-less coverage now selects both models explicitly: VA1 must return open bus
above `AFFFFH`, while VA2 must retain byte and word access in the legacy
`B0000H-DFFFFH` range. Human verification confirmed both VA2 V3 BASIC command
entry and VA1 PC-Engine 1.00 boot. The separate inherited VA1 V3 BASIC command
failure remains reproducible.

## Rejected workarounds

The following alternatives would hide the symptom without modeling hardware:

- preserving stack accesses across a system-bank switch;
- forcing bank 1 after the VA1 ROM routine;
- special-casing `F000:B650` or PC-Engine 1.00;
- suppressing interrupt vector 05;
- moving the guest stack from the frontend.

Memory bank selection applies uniformly to CPU data and stack cycles. The
correct fix is to make the initial RAM probe observe the real TVRAM boundary,
so guest software never classifies D000 as main RAM.

## Automated regression coverage

The ROM-less selftest verifies:

- a valid word round trip at `AFFFEH`;
- a word crossing `AFFFFH/B0000H` writes only the valid low byte and reads
  `FFH` for the invalid high byte;
- byte access at `B0000H` reads `FFH` and does not modify hidden backing
  storage;
- word access at `BFFF0H` reads `FFFFH` and does not modify hidden backing
  storage.

Verification completed on 2026-07-13:

```text
cmake --build --preset mingw-cross
cmake --build --preset linux-release
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --smoke
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

The selftest reported `selftest: VA TVRAM window ok` and all tests passed.
The ROM-less smoke run also exited successfully. The build retained only
pre-existing compiler warnings.

## Human gate

The maintainer tested the corrected MinGW binary with the previously failing
PC-Engine 1.00 disk and reported that both compared boot-model runs started
successfully. This passes the focused G29 compatibility check.

The normal release regression set still includes V3 boot, VA demo, OS/simple
operations, PC-Engine 1.05, and state save/load. These broader checks were not
reclassified as completed by the focused report.

## Frozen reference status

No files under `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, or `hlp/` were
modified. `cpuxva/memoryva.x86` remains a frozen record of the inherited
256KB behavior; the active C implementation narrows that behavior for the
64KB VA1 hardware while retaining it for the 256KB VA2/VA3 hardware.

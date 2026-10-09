# VA2 BASIC /87 direct-8087 ASM probe

Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Status: **LOCAL SOFTWARE QA PASS / PHYSICAL ROUTE OPEN**

This is a VA2 `/87` guest-seam probe, not 8087 architectural source
documentation. The Intel 8087 instruction and arithmetic contract remains
the supplied v7 evidence set and the exact vendored Berkeley SoftFloat
Release 3e source. The user-supplied VA N88-日本語BASIC V3 programmer guide is
used here only for the BASIC machine-language ABI: reserve a non-overlapping
area with `CLEAR`, select its segment with `DEF SEG`, load with `BLOAD`, and
return from `CALL` with a far return. It also documents the N88 BASIC FAC
format: a packed-BCD mantissa with a separate signed exponent, distinct from
the 8087 IEEE binary memory formats.

## Probe

The self-authored source is
[va2_basic87_asm_probe.asm](../../../../tools/8087/va2_basic87_asm_probe.asm)
and its ASCII BASIC companion is
[va2_basic87_asm_probe.bas](../../../../tools/8087/va2_basic87_asm_probe.bas).
The flat binary is loaded at `7FF0:0000` after `CLEAR ,&H7FF0`. The program
executes:

```text
FNINIT
FLD1
FLD1
FADD ST(0),ST(1)
FSTP dword [CS:0020]
FLD1
FLD1
FADD ST(0),ST(1)
FSTP qword [CS:0030]
FWAIT
RETF
```

The `CS` overrides intentionally keep the result inside the reserved
machine-language area. The `RETF` is required by the VA BASIC `CALL` ABI even
when the probe has no BASIC arguments.

## Reproduction boundary

The run used the user-supplied VA2/VA3 ROM fixture outside this repository,
`pc_model=88VA2`, `NDP8087=true`, `NDP8087Hz=10000000`, and a generated
temporary PC-Engine system/data disk containing only the system files,
`AUTOEXEC.BAT` with `BASIC /87`, the ASCII BASIC workload, and the generated
probe binary. No ROM, D88 image, or emulator implementation source from a
reference emulator was copied into the repository.

The relevant local commands were:

```text
nasm -f bin tools/8087/va2_basic87_asm_probe.asm -o X87RAW.BIN
python3 tools/pc88va/pcengine_disk.py vanilla --source <user-supplied-system-disk> --output <temporary-qa-disk>
python3 tools/pc88va/pcengine_disk.py install --image <temporary-qa-disk> --payload <temporary-payload>
build/linux-debug/sdl2/vaeg --model va2 --cfg <va2-config> --no-bkupmem --roms <user-supplied-va2-roms> --fdd1 <temporary-qa-disk> --headless-input-script <probe-input>
```

The emulator exited zero. Its diagnostic stream recorded `CS=7ff0` and real
native ESC dispatch for the probe, including the segment-prefixed `FSTP`
forms, with writes to physical addresses `7ff20` and `7ff30`.

## Observed result

The final VA2 screenshot showed:

```text
short-real (4 bytes):  0  0  0 64
long-real (8 bytes):   0  0  0  0
                       0  0  0 64
```

These are the little-endian 8087 encodings of `2.0` for short-real and
long-real. `CLEAR`, `DEF SEG`, `BLOAD`, `CALL`, `FSTP`, `FWAIT`, and `RETF`
all completed without a BASIC error. This proves the local VA2 `/87` seam for
direct 8087 raw-format arithmetic and storage; it does not prove the physical
VA2 BUSY/INT net, its polarity, PIC input, acknowledgement, or guest-handler
route. P17/P19 therefore remain `INTEGRATION_BLOCKED`.

The separate `X87TEST.BAS` arithmetic workload remains useful but is not a
numeric pass: its BASIC-side values use the N88 FAC packed-BCD representation,
so passing those bytes directly to 8087 m32/m64 memory operands is not a valid
IEEE conversion test. A future guest-level adapter test must explicitly
convert between the N88 FAC representation and the 8087 short/long-real
formats.

## ASCII BASIC `LOAD`/`RUN` rerun

The companion source was converted to CRLF for the guest ASCII file and placed
on a generated temporary QA disk as `VA2ASM.BAS`. The first attempt correctly
exposed a BASIC file-format mistake (`Direct statement in file`) because the
license comments had no line numbers. After making every comment a numbered
BASIC line, the exact guest sequence was rerun:

```text
AUTOEXEC.BAT: BASIC /87
LOAD "VA2ASM.BAS"
RUN
```

The VA2 screenshot showed `OK` after both `LOAD` and `RUN`, followed by:

```text
0  0  0 64
0  0  0  0
0  0  0 64
```

The emulator exited zero. This is the reproducible ASCII BASIC load/run
qualification for the direct 8087 seam; the generated binary, D88, ROMs, and
screenshots remain outside the repository.

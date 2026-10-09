<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# PC-88VA XMS and SMM memory-management investigation

**Status: investigation only. The SMM service ABI is not yet completely
identified.** This document records what is demonstrated by the original
`X8MAP.COM` binary and what still requires a resident-driver or hardware
trace. It does not define a new VAEG ABI and does not authorize changes to
the emulator.

## Scope and evidence

The examined program is the `X8MAP.COM` member of
[`x8map130.lzh`](/Users/maho/vaeg/docs/archives/pc88va-development-disk/x8map130.lzh).
The builder obtains that archive from Vector's `x8map130.lzh` URL and
installs the extracted diagnostic and its sample/documentation files in the
support disk ([`build-softlib-archive-disk.sh`](/Users/maho/vaeg/tools/pc88va/build-softlib-archive-disk.sh:293)).

Measured source artifacts:

```text
archive SHA-256: fc5bba93771e0fff3c8aa3d9ef942b80f65afba51b632a6336638ab70a7648a7
X8MAP.COM       : 10,373 bytes
X8MAP.COM SHA-256:
  4ee3f820f566265bcb714789567fd3604e51e5f9c16848e00d9a0274ba10d2ed
```

The generated distribution disk compresses this COM with DIET; the
uncompressed and compressed files must not be confused when comparing
hashes. The documented size change is 10,373 to 7,203 bytes
([utility-media note](/Users/maho/vaeg/docs/modernization/pc88va-utility-media.md:789)).

The disassembly offsets below are COM file offsets. A COM is loaded at
`PSP+0100h`; therefore an absolute operand such as `CS:17E1h` refers to
file offset `16E1h`. This matters for the self-modifying XMS far-call
operand.

## Terminology and separation

These are three different layers:

| Layer | Interface observed | Role |
| --- | --- | --- |
| XMS | DOS `INT 2Fh`, then an `ES:BX` far entry point | Extended-memory manager query |
| VA SMM command | PC-Engine resident `INT E8h` service | Enumerate and inspect resident memory-supervisor blocks |
| VA hardware memory map | `0152h/0153h` and RAM-bank control such as `01D0h` | Select physical memory mappings |

The word “SMM” in the X8MAP output is not the generic x86 SMI/System
Management Mode. It refers to the PC-88VA/PC-Engine resident software
supervisor. The BNN-derived manual documents the `0152h/0153h` MemMode
register, including the SMM/GMSP/SMBC fields
([manual](/Users/maho/vaeg/docs/tekumani/PC88VA_テクニカルマニュアル_BNN.md:8027));
it does not provide the complete `INT E8h` service ABI used below.

## XMS path

X8MAP uses XMS for observation only. It does not allocate, free, or move an
extended-memory block.

### Detection and entry-point lookup

At COM offsets `16B4h` and `16C6h` the code executes:

```asm
mov ax,4300h
int 2fh                 ; installation check
cmp al,80h              ; XMS present

mov ax,4310h
int 2fh                 ; return XMS entry point in ES:BX
```

The installation check is additionally gated by the `INT 2Fh` vector stored
at `0000:00BEh`. The returned `ES:BX` is written into the operands of two
`CALL FAR` instructions (the first call is at file offset `16E1h`, the
second at `1712h`). The music/graphics code is not involved; this is the
standard DOS XMS dispatch model.

### XMS calls actually made

| COM offset | Entry registers | Observed use | Allocation side effect |
| --- | --- | --- | --- |
| `16DBh` | `AH=00h` | Read XMS version/revision information and display it | None |
| `170Fh` | `AH=08h` | Read total and largest contiguous free EMB values | None |

For function `08h`, X8MAP formats `AX` as `EMB Free` and `DX` as `MAX`.
The sample output consequently shows values such as `EMB Free 919k
(MAX: 919k)`.

The code does not call the allocation, release, or move functions. In
particular, there is no observed `AH=09h`, `AH=0Ah`, or `AH=0Bh` call.

### HMA and DOS memory-policy observations

After function `08h`, X8MAP scans `FFFF:FFFFh` downward for zero bytes and
labels the result `HMA used`. This is a direct probe, not an XMS HMA
allocation request.

It also executes `INT 2Fh` with `AX=5800h` and interprets the returned DOS
memory-allocation policy as `Low` or `High`. That query is separate from the
XMS free-EMB query.

## VA SMM-command path

The SMM path is entered only after X8MAP recognizes a PC-Engine/VA system
signature in the DOS tables. The sample full-mode output lists
`BMSDRVA ... E8` among the resident hooks, which is consistent with the
following `INT E8h` calls being supplied by the resident VA memory driver.
This is an observed hook relationship, not a claim that all VA firmware
versions implement the same ABI.

### Initial query (`AX=FFFFh`)

At COM offset `179Bh`:

```asm
cli
mov ax,ffffh
int e8h
```

The code:

- formats `BX` as the SMM Command version;
- saves `DX` as a dynamically returned selector/command port;
- formats additional returned register bytes into the heading;
- compares `BX` against `0130h` and `0140h` to decide which later queries
  are supported.

The saved selector port is later used by `OUT DX,AL`; it is not a hard-coded
replacement for the hardware `0152h/0153h` MemMode register.

### Status query (`AX=FE00h`)

For sufficiently new reported versions, X8MAP calls:

```asm
mov ax,fe00h
mov es,cs
mov di,2403h
int e8h
```

The returned structure is inspected at byte offset `+6`, specifically bits
`20h`, `10h`, and `40h`. Those bits select the displayed SMM mode labels.
The repository contains no reviewed definition of this structure, so the
bit names and complete layout remain **UNRESOLVED**.

### Handle enumeration (`AX=0700h`)

The enumeration starts with `BX=0`:

```asm
xor bx,bx
mov ax,0700h
mov si,25d8h
mov di,25f2h
mov es,cs
int e8h
```

CF terminates the enumeration. On success X8MAP formats one record, then
increments `BX` and repeats while `BH=0`, allowing indices `0000h` through
`00FFh`.

The displayed record has the columns:

```text
handle  start  length  Atr.  name
```

The service also writes a returned record through the `ES:DI`/buffer path;
X8MAP compares the first eight bytes with `MEMRYBMS` and records unique type
bytes for a later bank inspection pass. The exact mapping of each returned
register and structure byte to the printed columns is **UNRESOLVED**.

### Summary query (`AH=12h`)

After enumeration, X8MAP executes:

```asm
mov ah,12h
int e8h
```

It uses the returned values to print SMM `free` and `total` capacity, then
restores interrupt state. The call's formal field-level ABI is not present
in the reviewed documentation; only its use as the summary query is
established by the binary and sample output.

### Selecting and inspecting mapped blocks

For every unique type byte collected from the enumeration, the program does:

```asm
cli
mov dx,[saved_smm_port]
out dx,al
mov ds,8000h
call 0442h
```

The routine at `0442h` reads the selected `8000h` window, recognizes known
resident-block signatures, and dispatches to the normal memory/MCB scanner.
The selector is reset to zero with another `OUT DX,AL` after all types have
been inspected.

There is also a literal `OUT 0ECh,0` in the `AH=12h` completion path. Its
purpose is not proven by the binary alone; it must not be assumed to be the
same port returned in `DX` by `AX=FFFFh`.

## What the bundled sample proves

`X8MAP130.SMP` contains captured PC-Engine output showing, in one full-mode
case:

- XMS version `2.00`, revision `3.01`;
- SMM Command version `1.1`;
- four SMM records with resident names and a total supervisor area of
  `8192` units;
- separate EMS and BMS sections.

This confirms the presentation model and the existence of resident records,
but it is not an independent specification of the `INT E8h` register ABI.

## Investigation status

### FACT

- XMS detection uses `INT 2Fh AX=4300h`; entry-point lookup uses
  `AX=4310h`.
- X8MAP invokes XMS functions `00h` and `08h` only.
- X8MAP does not perform XMS allocation, release, or block movement.
- The VA path invokes `INT E8h` with `AX=FFFFh`, `AX=FE00h`, `AX=0700h`,
  and `AH=12h`.
- `AX=0700h` is iterated with an increasing `BX` index and CF termination.
- The returned SMM selector is later used with `OUT DX,AL` and an `8000h`
  memory window.
- The BNN manual documents VA hardware memory-map controls separately from
  this software service.

### HYPOTHESIS

- The `INT E8h` SMM Command interface is provided by the resident BMSDRVA /
  PC-Engine memory-supervisor layer.
- The literal `OUT 0ECh,0` restores a legacy BMS selector state after the
  SMM summary query.

### UNRESOLVED / required next evidence

- Formal names and register contracts for `FFFFh`, `FE00h`, `0700h`, and
  `AH=12h`.
- The complete status structure at `ES:DI`, including byte `+6` bits.
- Whether the dynamically returned selector port is always `00ECh`,
  `01D0h`, or another value on VA and VA2.
- Whether VAEG's BMSDRVA implementation reproduces the `8000h` window and
  selector side effects.

The smallest useful next step is a guest-side trace that records, for each
`INT E8h`, the input `AX/BX/CX/DX`, output registers, CF, the returned
`DX`-selected port writes, and the first bytes visible at `8000:0000`.
That trace should be compared separately for VA and VA2 configurations; it
should not be replaced by a guessed SMM ABI.

## References

- [`X8MAP` provenance and installation note](/Users/maho/vaeg/docs/modernization/pc88va-utility-media.md:80)
- [`X8MAP` distribution build and DIET size note](/Users/maho/vaeg/docs/modernization/pc88va-utility-media.md:789)
- [`PC-88VA MemMode register description`](/Users/maho/vaeg/docs/tekumani/PC88VA_テクニカルマニュアル_BNN.md:8027)
- [`X8MAP130.TXT` in the archived package](https://www.vector.co.jp/soft/dos/hardware/se128128.html)

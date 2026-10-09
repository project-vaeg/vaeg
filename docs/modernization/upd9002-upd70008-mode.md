<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# μPD9002 uPD70008-Compatible Mode

Reference for the PC-88VA main CPU's non-native execution mode.

Merged from `upd9002-z80-emulation.md` and the provenance-tagged hardware
reference. Supersedes both.

**Active terminology.** Historical sources and the sections below use μPD780 or Z80-compatible for this main-CPU mode. The active implementation names it uPD70008-compatible. The separate FDD processor is uPD780C and remains an independent subsystem CPU.

**Scope.** The μPD9002 main CPU: its two execution modes, the
instructions and signals that move between them, the PC-88VA firmware
conventions layered on top, and what a correct emulation must contain.

**Not in scope.** The FDD subsystem uPD780C CPU. That is a separate physical
device with its own register file and ROM (`VASUBSYS.ROM`), reached in
vaeg through `cpu/z80_compat_cpu.cpp` and `io/subsystem.cpp`. It has
nothing to do with the main-CPU uPD70008-compatible mode, and the two must not share code or
state. The general PC-88VA boot trace lives in
`pc88va-boot-sequence.md`; only the parts bearing on mode transition are
repeated here.

---

## 0. How to read this document

Every factual claim carries a provenance tag. The tags determine what may
be implemented as fact, what must sit behind a flip point, and what must
not be implemented at all. Prior versions of these notes mixed authority
levels, and every error found so far lived on exactly that boundary.

| Tag | Meaning | Implementation rule |
|---|---|---|
| `[V30-MAN]` | NEC V20/V30-family documentation. The μPD9002's native mode *is* a V30 and its emulation mode is the same mechanism, but these are **analogy sources**, not μPD9002 manuals. | Implement as specified; flag anywhere the Z80 superset could plausibly differ. |
| `[VA-TM]` | *PC-88VA Technical Manual*. Directly describes the two CPU modes, compatible-mode instruction coverage, `CALLN 91h`/`95h`, `RETEM`, and the I/O trap. | Implement as specified. |
| `[VA-TEKU]` | 「てくまに」 — the *88VA Technical Manual* compiled independently by members of the PC-VAN "88VA Users Club" SIG and circulated as `TEKUMANI.LZH`. **Not** the BNN book: separately authored, community-edited, and its own distribution page notes that errors have been reported in it. | Implement, but treat as weaker than `[VA-TM]`. Corroborate against the manual scan or the ROM wherever possible. |
| `[VA-WIKI]` | *Inside PC-88VA* wiki, §1.5 CPU. Documents what the てくまに omits and carries its errata. Its I/O-trap section is CoBit's original 1992 post reproduced verbatim; its instruction-set section cites *Micom* Aug 1987. | Implement; note that CoBit's own caveat (ROM analysis plus experiment, possibly incomplete) applies to §9. |
| `[ROM]` | Extracted from PC-88VA ROM images: opcode tables, string pools, and disassembly. | Reliable for what the firmware *does*. A disassembler table is **not** proof that silicon executes something. |
| `[SRC]` | Period source or binaries that shipped and worked: CPMVA (Makichan, 1989), 98IOE/IOTRAP (CoBit, 1992), and the MS-DOS CP/M emulator v0.8 `.cpv` V30 path audited in M76. | Reliable for the path each program actually executes. |
| `[NEC-GIHO]` | *NEC 技報* (NEC Technical Journal) Vol. 40 No. 3, 1987, article 「パーソナルコンピュータ PC-88VA」: the manufacturer's own system description — V1/V2 memory maps (its Figures 3 and 4), I/O map (Figure 5), interrupt modes, and the V1/V2 supervisor. Consulted through a maintainer-supplied OCR transcription of three page images. | Implement as specified for system structure; it is an overview, so bit-level register behaviour still needs `[VA-TM]` or `[ROM]`. |
| `[DERIVED]` | Logically forced by the above; the derivation is stated inline. | Implement with a citing comment. |
| `[MEAS]` | Measured on a real PC-88VA2 by this project (the M101/M102 QA campaigns): CP/M probe programs run under CP/MVA in V3 mode, entering compatible mode through `BRKEM`. Byte-exact records, program and disk hashes are archived in [`m101_zexall_qa`](../agents/reports/m101_zexall_qa/README.md) and [`m102_zexund_qa`](../agents/reports/m102_zexund_qa/README.md). | Implement as specified; the records are the regression gate. Valid for what the probes exercised, nothing more. |
| `[UNKNOWN]` | Not determined by anything in hand. | Do not guess. Register in §15. |

**Sources.** Every source this document rests on, what it licenses, and
whether it is in hand, are collected in the **References** at the end.
The tags above are not one-to-one with works: several sources can share a
tag, and the mapping is given there explicitly.

---

# Part I — The CPU

## 1. Device overview

The μPD9002 is a PC-88VA-specific CPU with two execution modes.

| Mode | Role |
|---|---|
| **V30 mode** (native) | 16-bit execution; a V30 instruction set with VA-specific differences. |
| **uPD70008-compatible mode** (historically μPD780/Z80-compatible) | Compatible execution used by V1/V2 software. |

`[VA-TM]` `[VA-WIKI]` Both state that the relationship between the two
modes is the same as the V30's native and 8080-emulation modes, and that
the mode-transition method is likewise the same in principle. That is what licenses
the use of V30 `BRKEM`/`CALLN`/`RETEM` documentation as a behavioural
reference throughout this document.

`[UNKNOWN]` The designation "V52". Treat the μPD9002 as the VA's
V52-class CPU for lineage purposes; no source in hand confirms the name.

## 2. Mode model

### 2.1 The MD flag

`[V30-MAN]` Mode selection is controlled by the **MD flag, the most
significant bit of the PSW** — bit 15. `MD = 1` is native, `MD = 0` is
compatible mode.

Three consequences follow, and all three were missing from earlier
project documents:

1. `[DERIVED]` Mode is **architectural state carried in FLAGS**, not a
   side-channel implementation flag. Everything that saves or restores
   the PSW participates in mode switching: `PUSH PSW`/`POP PSW`,
   interrupt entry, `RETI`, `BRKEM`, `BRKEM2`, `CALLN`, `RETEM`.
2. `[DERIVED]` Mode transitions are structurally **identical to
   interrupt entry and return**.
3. `[DERIVED]` `PUSH PSW` in native mode pushes a word whose bit 15 is 1.
   An implementation that pushes 0 there is wrong in a guest-visible way,
   and visible to any FLAGS-comparing test oracle.

`[V30-MAN]` The instruction manual annotates `BRKEM`'s effect as
"MD ← 0: write enable status", and `POP PSW` does not modify the mode
flag outside a native call made from compatible mode. `[DERIVED]` So MD
is not an ordinary writable flag bit: there is a write-enable condition
around it. Model it as a latch with its own write rule, not as bit 15 of
a plain flags word (§13 for how MAME splits this into two fields).

### 2.2 Mode-transition instructions

Notation: `push(x)` is `SP ← SP − 2; (SS:SP) ← x`. `vec(n)` is the
real-mode interrupt vector table entry `n` — offset at physical `n×4`,
segment at `n×4+2`.

#### 2.2.1 BRKEM imm8 — `0F FF imm8` — enter compatible mode

```
Executed : native mode
Operation:
    push(PSW)
    MD ← 0                       ; write-enable status
    push(PS)                     ; PS = code segment (x86 CS)
    PS ← vec(imm8).segment
    push(PC)                     ; PC after the 3-byte instruction
    PC ← vec(imm8).offset
    flush prefetch; fetch as compatible-mode code
```

`[V30-MAN]` Confirmed against the NEC instruction manual. Notes:

- Pushes go to the **native stack** (`SS:SP`), not the compatible-mode
  stack. `SP` is untouched by compatible mode (§3.4), so the native
  return context survives.
- **`BRKEM` does not clear IE or BRK**, unlike the `BRK` (INT)
  instruction. Interrupts remain enabled across the transition.
- The vector is an ordinary real-mode IVT entry. `[SRC]` CPMVA installs
  it with DOS `INT 21h AH=25h` and restores it on exit.

`[SRC]` **`BRKEM` works on the μPD9002.** CPMVA emits `0F FF E1` and runs
CP/M-80 on real hardware. Earlier literature claiming the μPD9002 has no
`BRKEM` is wrong.

#### 2.2.2 BRKEM2 imm8 — `0F FE imm8` — the VA firmware's entry

```
Executed : native mode
Operation: presumed BRKEM-shaped; see below
```

`[ROM]` Present in the on-board monitor's opcode table in both ROM
images, immediately after `BRKEM`. **The two ROMs disagree on the
mnemonic:**

| ROM | Mnemonic for `0F FE` |
|---|---|
| `varom00.rom` (PC-88VA) | `BRKEM2` |
| `varom00_va2.rom` (PC-88VA2) | `BRKFEM` |

`[VA-WIKI]` §1.5.1 states it directly: `BRKEM2` was **added** as the
transition instruction into Z80 emulation mode, encoded as three bytes
`0F FE nn` where `nn` is an interrupt number, and the VA does **not use**
the V30-compatible `BRKEM` (`0F FF nn`).

`[DERIVED]` Read that wording carefully: "not used" is a statement about
the VA's own firmware, not about the silicon. `BRKEM` is present and
works — CPMVA uses it (§2.4). The wiki is silent on what `BRKEM` enters
on a μPD9002.

`[ROM]` Corroborated by firmware: `VAROM1.ROM:0x13B1` contains
`0F FE 90`, at a valid instruction boundary, immediately after the ROM
has installed vector `90h` (§8).

**The open question this creates.** `[DERIVED]` Both entries exist and
both work, and **both destinations run Z80 code** — `BRKEM` because
CPMVA's Z80 BIOS executes `JR` (§2.4), `BRKEM2` because V1/V2 mode runs
Debug 8800 (§4.4). The old hypothesis that one entry selects 8080 and the
other Z80 is therefore *not* supported. What distinguishes them is
`[UNKNOWN]`. The plausible candidates:

- `BRKEM2` additionally switches machine-level state — the V1/V2 I/O and
  memory maps — where `BRKEM` only switches the CPU decoder.
- They differ in the saved frame or in the mode-latch write rule.
- They differ in nothing architecturally, and the VA firmware simply
  uses the VA-specific opcode by convention.

`[ROM]` `[VA-TM]` **The first candidate is dead.** Machine-level state is
not switched by either instruction: the system memory mode is port `153H`
bit 6, and the firmware writes it with an ordinary `OUT` four instructions
before `BRKEM2` and again in the resume block (§8.2). Whatever
distinguishes `BRKEM2` from `BRKEM`, it is not that one of them carries
the machine along with it.

`[UNKNOWN]` Until this is settled, implement `BRKEM2` as a documented
default with a single flip point (§15).

Note for tooling: a general x86 disassembler will decode `0F FE` as an
MMX instruction. In PC-88VA context it is `BRKEM2`.

#### 2.2.3 CALLN imm8 — `ED ED imm8` — call a native routine

```
Executed : compatible mode
Operation:
    push(PSW)                    ; the pushed PSW has MD = 0
    MD ← 1
    push(PS)
    PS ← vec(imm8).segment
    push(PC)
    PC ← vec(imm8).offset
```

`[V30-MAN]` `[VA-TM]` Both confirm the encoding and the operation. The
called native routine **returns with `RETI`** (NEC's mnemonic for
`IRET`, opcode `CF`); restoring the pushed PSW restores `MD = 0` and
re-enters compatible mode.

`[SRC]` CPMVA's native handler is a `far` procedure ending in `iret`.
The byte sequence `ED ED E0` appears three times in the shipped
`CPMBIOS.COM` — `0FF34h`, `0FF3Fh`, `0FF82h` — matching `c$boot`,
`c$wboot` and `execbios` in `CPMBIOS.MAC`.

`[DERIVED]` `ED ED` and `ED FD` are both **undefined** `ED`-prefixed
opcodes on a real Z80: NEC placed these instructions in unused Z80
encoding space. This is also why neither can be a native-mode
instruction — `ED` in native mode is `IN AW, DW`, which NEC would not
have broken.

#### 2.2.4 RETEM — `ED FD` — leave compatible mode

```
Executed : compatible mode          <-- note
Operation:
    PC  ← pop()
    PS  ← pop()
    PSW ← pop()                     ; restores MD = 1 → native mode
```

`[V30-MAN]` Chapter 8 states it outright: `RETEM` is used exclusively as
the return from compatible mode to native mode when `BRKEM` caused the
shift, **it is executed in compatible mode**, execution returns from the
`BRKEM` interrupt routine to the main routine, `PS`/`PC`/`PSW` are
restored, and the `MD = 1` that `BRKEM` pushed is restored.

`[SRC]` Independently proven by CPMVA. `EXIT.MAC` is a `.z80` source
whose terminating sequence is a bare `retem` — that is how CP/M returns
control to the PC-Engine monitor. `V30.MAC`, which defines the `calln`
and `retem` macros with `defb`, is included only by Z80-side sources;
the 8086-side `CPMVA.H` defines `brkem` with `db`. `CPMVA.DOC` records
the build: V30 side with OPTASM 1.0 + TLINK 2.0, Z80 side with M80/L80
or SLR Z80ASM + SLRNK.

Two structural points:

- `RETEM` is the **only** instruction that ends a `BRKEM` session and
  resumes the instruction after it. Any model in which `RETEM` executes
  in native mode leaves compatible mode with no exit.
- Because `RETEM` executes in compatible mode, **the native decode table
  needs no `ED FD` special case at all.**

`RETEM` has **one** return target. `[ROM]` The `1000:C003` destination
sometimes attributed to `RETEM` is not part of the instruction: it is an
explicit `jmp 1000h:c003h` in the native code that runs *after* `RETEM`
has returned and V3 re-initialisation has completed (§8). `[VA-TM]` The
attribution is not anyone's invention — the technical manual describes it
under `RETEM` itself, saying that on a return from V1/V2 mode to V3 mode
the machine jumps to `1000H:C003H` after V3-mode initialisation. That is
an accurate description of what the machine does and a misleading one of
what the instruction does; see Appendix A item 16 and §8.1.

### 2.3 Transition summary

`[V30-MAN]` Chapter 8 enumerates both directions exhaustively.

| Path | Encoding | Executes in | Ends in | Returned from by |
|---|---|---|---|---|
| `BRKEM imm8` | `0F FF imm8` | native | compatible | `RETEM` |
| `BRKEM2 imm8` | `0F FE imm8` | native | compatible | `RETEM` (presumed) |
| `CALLN imm8` | `ED ED imm8` | compatible | native | `RETI` (`CF`) |
| `RETEM` | `ED FD` | compatible | native | — |
| `RETI` | `CF` | native | compatible, if the popped `MD = 0` | — |
| `NMI` / `INT` | — | compatible | native | `RETI` (`CF`) |
| `RESET` | — | compatible | native | emulation in progress is aborted |

**Nesting is prohibited.** `[V30-MAN]` Inside a native routine entered by
`CALLN`, or by an `NMI`/`INT` taken from compatible mode, compatible mode
cannot be entered again with `BRKEM`; if attempted, `MD` does not work
normally and behaviour is undefined. There is exactly one level of
compatible-mode context.

`[DERIVED]` For an emulator this means the mode state needs no stack of
its own — the single `MD` latch plus the ordinary interrupt stack is the
entire model, and a nested `BRKEM` is an error to diagnose rather than a
case to support.

**External mode indication.** `[V30-MAN]` The `PS3` processor-status
output is high during a compatible-mode bus cycle and low in native mode.

`[UNKNOWN]` Whether the PC-88VA's "88-mode emulator" gate array uses
`PS3` to switch peripheral decoding between the V1/V2 and V3 I/O maps.
If it does, that is the hardware link between CPU mode and I/O map, and
it would be a strong candidate answer to the `BRKEM` vs `BRKEM2`
question in §2.2. The VA1 schematics are in hand; tracing the μPD9002
`PS3` pin is a cheap, high-value check.

### 2.4 Compatible mode is Z80, not 8080

`[V30-MAN]` On the V20/V30, code fetched after `BRKEM` is interpreted as
μPD8080AF instructions.

`[VA-TM]` The technical manual states that μPD780 mode supports the
complete documented μPD780 instruction set and many undefined
instructions, plus `CALLN` and `RETEM`.

`[SRC]` **Directly proven**, from the shipped `CPMBIOS.COM`:

- `CPMBIOS.MAC` is assembled with `.z80` and uses `JR` on the primary
  BIOS dispatch path: `c$const: ld a,2 / jr execbios`.
- `CPMBIOS.COM` maps to `0FA00h–0FFFFh` (1536 bytes, ending with the
  ASCII system ID `VA` at `0FFFEh`, exactly as the source's
  `.phase`/`defs`/`defb` directives specify). The entry jump table at
  `0FA00h` reads `C3 2F FF  C3 3A FF  C3 45 FF …`, and at `0FF45h` —
  the `const` entry — the bytes are `3E 02 18 …`: `LD A,2` then **`JR`**.
- `JR` (`18`), `JR cc` (`20`/`28`/`30`/`38`) and `DJNZ` (`10`) are
  Z80-only; on an 8080 those opcodes are undocumented NOPs. If the mode
  were 8080, `c$const` would fall through into `c$conin` and CP/M could
  not run. The binary contains 14 `18`, 16 `20`, 2 `28`, 22 `30`, 1 `38`
  and 5 `10` opcode bytes.

For the record, the argument used in earlier project documents — that
`ED`-prefixed `CALLN`/`RETEM` proves Z80 — is **invalid**. Those are
`ED ED`/`ED FD` on the plain V20/V30 too, where the mode is 8080. The
`_IX equ si` / `_IY equ di` aliases in `CPMVA.H` are also not proof:
CPMVA never uses them.

## 3. Programming model in compatible mode

### 3.1 Registers

`[V30-MAN]` From Figure 8-1: the low 8 bits of `AW` serve as the
accumulator, and both halves of `BW`, `CW` and `DW` serve as the six
general-purpose registers. `SP` is the stack pointer in native mode;
**`BP` acts as the stack pointer in compatible mode.** The pairings are
`AW`→`AC`, `BW`→`H`,`L` = `HL`, `CW`→`B`,`C` = `BC`, `DW`→`D`,`E` = `DE`,
`BP`→`SP`, `PS`→code segment, `DS0`→data segment, `PC`→`PC`,
`PSW`→flags.

`[SRC]` The Z80-side view, from `CPMVA.H` — the same mapping extended
with the Z80-only index registers:

```
Z80    V30 (NEC / x86)         Z80    V30
A   ↔  AL                      BC  ↔  CW / CX
B   ↔  CH                      DE  ↔  DW / DX
C   ↔  CL                      HL  ↔  BW / BX
D   ↔  DH                      IX  ↔  IX / SI   [UNKNOWN — §15]
E   ↔  DL                      IY  ↔  IY / DI   [UNKNOWN — §15]
H   ↔  BH                      SP  ↔  BP        <- BP, not SP
L   ↔  BL                      PC  ↔  PC / IP
```

The `A`, `B`/`C`, `D`/`E`, `H`/`L`, `BC`, `DE`, `HL` and `SP` rows are
exercised by working CPMVA code: the native BIOS handler reads the Z80
function number as `AL`, returns file handles in `CX`, lengths in `DX`,
and reads buffer pointers from `BX`.

`[V30-MAN]` **`AH`, `SP`, `IX`, `IY` and all four segment registers
(`PS`, `SS`, `DS0`, `DS1`) are not addressable from compatible mode**,
and the manual gives the rationale for the split stack pointers: keeping
`SP` and `BP` independent prevents misuse of one mode's stack pointer
from destroying the other's.

`[DERIVED]` This is a **point of divergence for the μPD9002**. A Z80 has
`IX` and `IY`; on the V20/V30 they are explicitly unreachable from
emulation mode. The μPD9002 must therefore have changed this, and *how*
is precisely the open `ix-iy-share` question: it could expose the
existing `IX`/`IY` register-file entries — which is what `CPMVA.H`'s
`_IX equ si` / `_IY equ di` assumes — or it could have added separate
storage.

`[DERIVED]` The V30 register file has eight 16-bit registers. The Z80
main set plus `IX`/`IY` plus the compatible-mode `SP` consumes all of
them except `AH` and the native `SP`. That leaves at most three bytes of
visible register file, which cannot hold the eight bytes of
`AF'/BC'/DE'/HL'`. Either the alternate set has storage invisible to
native mode, or it does not exist.

`[SRC]` **It exists.** A register dump from a real PC-88VA settles it
(§17.2): the main and alternate sets hold *different* values in all four
pairs at once — `A=00` against `A'=FF`, `B=0000` against `B'=FFFF`,
`D=EDCC` against `D'=FF7B`, `H=0001` against `H'=0911`. Debug 8800 can
only capture the alternates by executing `EX AF,AF'` and `EXX`, so if
those were no-ops on a machine without alternate storage the monitor would
have captured the main set twice and displayed identical pairs.

`[DERIVED]` Combined with the counting argument, that is a statement about
the silicon: **the μPD9002 carries eight bytes of register storage that
native mode cannot address.** It is not a V30 register file with a Z80
decoder bolted on. `IX`/`IY` are *not* part of that extra storage — they
alias `SI`/`DI`, which the same dump shows (§17.2) — so the hidden
storage is exactly `AF'`, `BC'`, `DE'` and `HL'`.

### 3.2 Flags

`[V30-MAN]` The low 8 bits of the PSW **are** the compatible-mode flag
register. There is **no flag conversion at a mode transition**; the same
physical bits are read under a different interpretation. Figure 8-2 gives
both layouts and they are the same byte:

```
bit          9    8    7    6    5    4    3    2    1    0
V30  PSW:    IE   BRK  S    Z    0    AC   0    P     1    CY
8080 Flag:             S    Z    0    AC   0    P     1    C
```

This is not a coincidence of the V-series: the 8086 inherited its low
flag byte from the 8080 in both bit position and meaning.

`[DERIVED]` For the Z80 superset the alignment still holds, with the
three bits the 8080 leaves constant carrying the Z80-only flags:

```
bit          7    6    5    4    3    2     1    0
V30  PSW:    S    Z    0    AC   0    P     1    CY
Z80  F  :    S    Z    F5   H    F3   P/V   N    C
```

`S`, `Z`, `H`↔`AC` and `C`↔`CY` occupy identical positions; `F5`, `F3`
and `N` land on PSW bits 5, 3 and 1, which the 8080/V30 side holds at
constant `0`, `0`, `1`. The Z80's `P/V` shares bit 2 with the V30's `P`,
but the two differ in *meaning* for arithmetic operations (overflow vs
parity).

**Implementation consequence** `[DERIVED]`: the difference between the
modes is in **how flag values are computed**, not where they are stored.
A design that keeps a separate Z80 `F` and converts it at each transition
is modelling the hardware incorrectly, and the error is guest-visible.

`[ROM]` Corroboration from an unrelated direction: Debug 8800's flag
notation table (§4.4) encodes the Z80 `F` layout — including `N` at
bit 1 and undefined bits at 3 and 5 — exactly as above.

**Anomaly, unresolved.** `[SRC]` CPMVA's extended BIOS (functions
`80H`–`84H`) documents a carry return convention: `CPMVA.DOC` specifies
`Cy=0` for success and `Cy=1` for error, and `CPMVA.ASM` implements it
(`stc` on the `ExExec` error path; `xor _A,_A`, which also clears CY, on
success paths). But those handlers are reached through `CALLN` and return
through `iret`, and `RETI` restores the PSW that `CALLN` pushed — which
should discard the handler's carry. Either the μPD9002's `CALLN`/`RETI`
pair has flag semantics that are not a plain PSW restore, or the
convention never worked and callers relied on the error code in `A`
(also documented, and sufficient). **This is the cheapest experiment that
settles the flag model** — see §17.6.

### 3.3 Address space and segments

`[V30-MAN]` In compatible mode, **the segment base for memory operands,
including the stack, is determined by `DS0`** (the x86 `DS`), whose value
the program sets before entering. Instruction fetch uses `PS` (the x86
`CS`). The segment registers are not visible to compatible-mode code.

```
code   fetch  :  physical = (PS  << 4) + PC
data   access :  physical = (DS0 << 4) + addr16
stack  access :  physical = (DS0 << 4) + BP     <- DS0, not SS
```

The 64 KiB compatible-mode space can therefore sit anywhere in the 1 MiB
physical space. Two instances are documented:

`[ROM]` **V1/V2 mode is at physical `0x10000`.** Immediately before the
firmware handoff (§8) the ROM sets `DS = 1000h`, and vector `90h` is
installed as `1000:0000`, so `PS = DS0 = 1000h`. This is what makes
`1000:E000` "address `E000h` as seen from V1/V2 mode" and `1000:C003`
"`C003h` in V1/V2 space" — both are ordinary offsets in that window, not
special addresses.

`[SRC]` **CPMVA places its window wherever DOS allocated it.**
`CPMVA.ASM` declares a paragraph-aligned `emu_seg`, enlarges its DOS
memory block by `1000h` paragraphs ("64KB for Z80 emulation area"),
points the `BRKEM` vector at `emu_seg:0FA00h`, and sets `DS = emu_seg`
immediately before `brkem`. `SS` is never touched — it remains the
`.MODEL small` DGROUP stack — and this is correct precisely because
compatible-mode stack accesses use `DS0`.

`[V30-MAN]` Bus behaviour is unchanged by compatible mode: memory and I/O
cycles are identical to native mode, and the bus-hold and `HALT` standby
functions work as they do natively. `BUSLOCK` and `POLL` are
**unavailable** in compatible mode.

`[UNKNOWN]` How Z80 `IN`/`OUT` port numbers map to the V30 I/O space. The
bus cycles being identical suggests a direct mapping of the 8-bit (or
`BC`-supplied 16-bit) port number, but nothing in hand states it.

`[DERIVED]` What the port number *is* may be a different question from
what it *reaches* — but the reason is simpler than the bus. The I/O map
the machine presents follows the **system memory mode**, port `153H`
bit 6, which software sets independently of the CPU's execution mode
(§8.2). CPMVA never writes it, which is why it never executes a Z80 `IN`
or `OUT` at all and routes every device operation natively through
`CALLN` (§10): its compatible-mode code is running with the V3 map
underneath it. An earlier revision of this paragraph attributed that to
`PS3`-driven decoding; the register is the documented mechanism.

### 3.4 Stacks

Two distinct stacks; do not confuse them.

| Stack | Pointer | Segment | Used by |
|---|---|---|---|
| native | `SP` | `SS` | `BRKEM`, `BRKEM2`, `CALLN`, `RETEM`, `RETI`, interrupt entry |
| compatible | `BP` | `DS0` | Z80 `PUSH`/`POP`/`CALL`/`RET`/`RST` |

`[SRC]` `CPMBIOS.MAC` sets the compatible-mode stack with
`ld sp,CCPTOP` (`0E400h`), which lands in `BP`. `CPMVA.ASM`'s native
handler freely uses `push si`/`pop si` on the native stack while
compatible-mode code is suspended.

`[DERIVED]` Getting this backwards produces a "boots, then desyncs"
failure rather than a clean one. Verify it first.

### 3.5 Interrupts, RESET and HALT

`[V30-MAN]` A maskable interrupt or NMI taken in compatible mode is
**serviced in native mode**, exactly as it would be natively; the handler
returns with `RETI`, which restores the pushed PSW and re-enters
compatible mode.

`[DERIVED]` This is the `CALLN` mechanism again: the interrupt pushes the
compatible-mode PSW (`MD = 0`), hardware sets `MD = 1`, `RETI` undoes
both. No separate compatible-mode interrupt path is needed for the
V30-documented behaviour.

`[V30-MAN]` `RESET` is different: it resets the CPU exactly as in native
mode and **the emulation in progress is aborted** — there is no return.

`[V30-MAN]` `HALT`/standby has its own rules:

- If standby was entered *from compatible mode*, an `INT` — even with
  interrupts disabled — resumes **in compatible mode** at the instruction
  after `HALT`.
- `RESET` or `NMI` instead, or `INT` while interrupts are enabled, exits
  standby into **native mode**; from there the handler can return to
  compatible mode with `RETI`.
- If standby was entered from native mode, `RESET`, `NMI` or `INT`
  resumes native mode regardless of the interrupt-enable state.

`[ROM]` **This is not academic, and NEC treated it as a real problem.**
The VA's copy of Debug 8800 differs from a real PC-8801mkIISR's in exactly
one respect: **every `HALT` on the memory-test path has been removed**,
two of them replaced by `JR $` spin loops and one deleted outright (§4.5).
Nothing else in the 8 KiB bank was touched. `[DERIVED]` The rule above is
the reason to expect that: a `HALT` executed in compatible mode enters
standby, and an `INT` taken with interrupts enabled resumes in **native**
mode — which for V1/V2 code is not a recoverable event. Porting the
monitor to the VA meant getting rid of the `HALT`s.

`[UNKNOWN]` Whether the μPD780 superset adds Z80-style interrupt handling
on top — `IM 0`/`IM 1`/`IM 2`, the `I` register, `IFF1`/`IFF2`, `EI`/`DI`
semantics, `RETN`. CPMVA uses only synchronous `CALLN`. The V1/V2 BASIC
ROM does contain an `IM 1`-style vector at Z80 `0038h` (Appendix C),
which is suggestive but proves nothing about how the interrupt arrives.

This is a different question from the PC-8801-side and FDD-side Z80
interrupt work. Do not reuse conclusions across them.

## 4. Instruction set

### 4.1 The `0F` extension map

`[ROM]` The on-board monitor carries a table of
`{mask, opcode, operand-class}` triples for the `0F` extensions, with a
parallel mnemonic string pool (ASCII, bit 7 set on the final character).
Both ROMs contain the same 14 entries:

```
FF 20 00  ADD4S      FF 20 01  ADD4S      F6 10 03  TEST1
FF 22 00  SUB4S      FF 22 01  SUB4S      F6 16 03  NOT1
FF 26 00  CMP4S      FF 26 01  CMP4S      F6 12 03  CLR1
FF 28 02  ROL4       FF FF 04  BRKEM      F6 14 03  SET1
FF 2A 02  ROR4       FF FE 04  BRKEM2 (VA) / BRKFEM (VA2)
```

The `F6` mask covers the four encodings of each bit instruction
(`CL`/`imm` × byte/word), matching the documented V30 layout `0F 10`–`0F 1F`.
**`INS` and `EXT` are absent.**

### 4.2 The four deleted instructions

`[VA-WIKI]` The V30-mode instruction set is the V30's, minus four
instructions: `INS`, `EXT`, `OUTM`, `INM`. The wiki cites *Micom*,
August 1987 for this.

`[V30-MAN]` What those four actually are — and this was wrong in every
earlier project document:

| NEC mnemonic | Intel mnemonic | Encoding | Function |
|---|---|---|---|
| `INS` | — | `0F 31`, `0F 39` | **bit field insert** (`AW` → bit field at `DS1:IY`) |
| `EXT` | — | `0F 33`, `0F 3B` | **bit field extract** (bit field at `DS0:IX` → `AW`) |
| `INM` | `INSB`/`INSW` | `6C`, `6D` | primitive block input |
| `OUTM` | `OUTSB`/`OUTSW` | `6E`, `6F` | primitive block output |

NEC's classification places `EXT`/`INS` under *bit field manipulation*
and `INM`/`OUTM` under *primitive I/O*. **Only two of the four are block
I/O.** Any reasoning that treats all four as block I/O — in particular
the suggestion that the I/O trap exists to emulate them in software —
loses half its basis and should be withdrawn. The naming collision is the
likely origin of the error: NEC's `INS` is not Intel's `INS`.

`[ROM]` Consistent with removal: `INS`/`EXT` are absent from the `0F`
table. **Not** consistent, and worth stating plainly: `INM` and `OUTM`
(and their Intel-notation twins `INS`/`OUTS`) *are* present in the
monitor's mnemonic pool. A disassembler table is not an execution table,
so this neither proves nor disproves the deletion — but earlier notes
claiming they are absent from the string pool are wrong.

`[DERIVED]` Consequence for any V20-based test oracle: `6C`–`6F` and
`0F 31/33/39/3B` are expected divergences on a μPD9002 target, not
defects to fix.

### 4.3 What compatible mode decodes

`[VA-TM]` The complete documented μPD780 instruction set plus many
undefined instructions, plus `CALLN` and `RETEM`.

`[SRC]` Independently proven present: the 8080 subset used by CP/M, plus
`JR`, `JR cc`, `DJNZ` (§2.4).

`[ROM]` `LDIR` (`ED B0`) appears in live V1/V2-mode code (§4.4).

`[MEAS]` The undocumented `DD`/`FD` half-index registers exist and behave
as on a Zilog Z80 (plus the flag rules of `uPD9002-zex-results.md`), the
`DD CB`/`FD CB` space departs from the Zilog pattern in a systematic way,
and the *undefined* `ED` space turned out to be a sizeable, perfectly
repeatable instruction set of its own, including loads that read the
native side's interrupt vector table. §4.7 presents the full map.

`[ROM]` The **V3-mode PC-Engine monitor** carries no Z80 mnemonics in
either ROM image; its assembler and disassembler are NEC-native only.
That is a fact about the V3 tooling, not the silicon. **Debug 8800 is a
different matter** — see §4.4.

### 4.4 Debug 8800 — a corpus of compatible-mode Z80 code

`[ROM]` `varom00.rom` also contains **Debug 8800 version 1.0**,
banner-dated 11 Dec 1981, copyright NEC — a PC-8801-era Z80 monitor, six
years older than the PC-88VA, carried for V1/V2 compatibility. Banner at
ROM offset `0x1F058`.

The surrounding region is unambiguously Z80. Disassembling from
`0x1EE5F`:

```
21 FA F1      LD   HL,0F1FAh
01 03 00      LD   BC,0003h
11 C9 ED      LD   DE,0EDC9h
ED B0         LDIR
F3            DI
ED 7B F7 F1   LD   SP,(0F1F7h)
2A F5 F1      LD   HL,(0F1F5h)
3A F9 F1      LD   A,(0F1F9h)
D3 70         OUT  (70h),A
C3 8B 7E      JP   7E8Bh
```

Three data structures in this region matter:

- **Command dispatch table**, `0x1E0F5`: the letters `ALTRWVBM` then
  `EIOGFXDS` — sixteen command letters — then 16-bit little-endian
  handler addresses (`6135h`, `61A3h`, `6254h`, `63D8h`, `6406h`,
  `6491h`, `6479h`, `64D8h`, `746Fh`, `64A6h`, `695Eh`, `6806h`,
  `6754h`, …).
- **Flag notation table**, `0x1E3C8`: `PM-Z---H--OE-N-C` — eight
  two-character pairs, one per flag bit, matching the monitor's own help
  text (`Sign[p or m]`, `Zero[z or -]`, `Undef[-]`, `Half[h or -]`,
  `Undef[-]`, `Parity[o or e]`, `Sub[n or -]`, `Carry[c or -]`). The
  **Z80** `F` layout, matching §3.2.
- **Register name table**, `0x1E318`: `IY`, `IX`, `H'`, `D'`, `B'`, `F'`,
  `A'`, `H`, `D`, `B`, `F`, `A`. The `x` command displays and edits **the
  alternate register set and the index registers**.
- **Mnemonic pool**, `0x1EB00`–`0x1ED00`, high-bit-terminated: it carries
  **both notations**. Zilog — `LD`, `JP`, `JR`, `DJNZ`, `CALL`, `RET`,
  `NOP`, `PUSH`, `POP`, `EX`. Intel 8080 — `MOV`, `MVI`, `LXI`, `JMP`,
  `STA`, `LDA`, `XCHG`, `XTHL`, `SPHL`, `PCHL`, `RST`, `DAD`, `RLC`.
  So its `a` and `l` commands handle Z80 source directly; the tests in
  §17 can be typed as mnemonics rather than hexadecimal.

`[DERIVED]` The handler addresses, the internal `JP 7E8Bh` and the banner
offset are mutually consistent with *Z80 address = ROM offset − `0x18000`*
for this region, placing Debug 8800 at Z80 `6000h`–`7FFFh` — the PC-8801
banked extension-ROM window. The bank's entry shim confirms both the
mapping and the mechanism:

```
0x1E000:  44 42            "DB"  (bank signature)
          F3               DI
          C3 31 7A         JP   7A31h
          ED 73 F7 F1      LD   (0F1F7h),SP
          DB 70            IN   A,(70h)          ; save bank-select state
          32 F9 F1         LD   (0F1F9h),A
          CD 00 7B         CALL 7B00h
          21 F4 71         LD   HL,71F4h
          01 79 00         LD   BC,0079h
          ED B0            LDIR
```

with the exit path above restoring `SP` and the bank register. Port `70h`
is the PC-8801 extended-ROM bank select.

**Why this matters.** The PC-88VA has no second main-CPU Z80, and
**Debug 8800 has been run in V1/V2 mode on a real PC-88VA**. V1/V2-mode
Z80 code therefore executes on the μPD9002 in compatible mode, and the
whole of Debug 8800 and the N88-BASIC ROM (Appendix C) is
proven-executable compatible-mode code. In particular:

- `[DERIVED]` `LDIR` appears in live code, so the `ED` block instructions
  are implemented.
- `[DERIVED]` The `x` command reads and writes `A'/F'/B'/D'/H'`. There is
  no way to reach the alternate registers except by executing
  `EX AF,AF'` and `EXX`, so if `x` works the alternate set exists.
- `[DERIVED]` The same command handles `IX`/`IY`, requiring the `DD`/`FD`
  prefix spaces.

§17.3 turns these into a ten-minute confirmation.

### 4.5 The VA's Debug 8800 is a patched 8801 ROM

`[ROM]` A real PC-8801mkIISR `n80.rom` is now in hand, and its top 8 KiB
is the same Debug 8800 bank the VA carries. Comparing that against
`varom00_va2.rom[0x1E000:0x20000]`:

- **52 bytes differ, all inside Z80 `7357h`–`73C4h`** — a 110-byte window.
  The other 8082 bytes of the bank are byte-identical.
- The byte `76h` occurs **24 times in the 8801 copy and 21 times in the
  VA's**. Every one of the three missing occurrences is an executed
  `HALT`.

The three sites, with the surrounding code:

```
        8801mkIISR                     PC-88VA
7381    10 E9    DJNZ 736C             10 E9    DJNZ 736C
7383    76       HALT                  --       (deleted)
        3E 21    LD A,21h              3E 21    LD A,21h
        D3 40    OUT (40h),A           D3 40    OUT (40h),A
7388    76       HALT                  18 FE    JR $
        3E 02    LD A,02h              3E 02    LD A,02h
        D3 31    OUT (31h),A           D3 31    OUT (31h),A

7398    C2 3C C0 JP NZ,C03Ch           20 2B    JR NZ,+2Bh
        ...      (JR displacements adjusted by one)

73C4    76       HALT                  18 FE    JR $
        3E 04    LD A,04h              3E 04    LD A,04h
        D3 31    OUT (31h),A           D3 31    OUT (31h),A
```

`[DERIVED]` The patch is **byte-budget neutral**: deleting one `HALT`
frees a byte, `JP nn` → `JR d` frees another, and the two `JR $`
replacements spend them. Every address outside the window is preserved,
which is exactly how you patch a ROM you do not want to relink. The
adjusted `JR` displacements (`F7`→`F8`, `EF`→`F0`) confirm the shift was
tracked deliberately rather than being an independent reassembly.

`[DERIVED]` **Why `HALT` had to go** is §3.5: in compatible mode `HALT`
enters standby, and the exit path depends on the interrupt state — an
`INT` with interrupts enabled leaves standby in *native* mode. A monitor
that halts waiting for a keypress would, on the μPD9002, be one interrupt
away from dropping V1/V2 code on the floor. The replacement is a spin
loop, which has no such property.

`[DERIVED]` Three consequences for this document:

1. **The `HALT` divergence is confirmed by NEC's own workaround**, not
   just by the V-series manual — it is evidenced by the silicon vendor
   changing code to accommodate it. §4.6 now diffs the whole V1/V2 ROM
   set the same way, and these three `HALT`s remain the **only**
   CPU-level change in any of it.
2. **Test payloads must not end in `76`.** §17.5 has been changed
   to `18 FE` accordingly.
3. **This is a patch target for emulation fidelity.** A vaeg that runs
   the VA's ROM will never execute those `HALT`s; one that runs a stock
   8801 ROM image will, and will then need the standby semantics right.

### 4.6 The VA's N88-BASIC is the 8801mkIISR's, with additions

`[ROM]` A real PC-8801mkIISR `n88.rom`, its four extension banks and its
`disk.rom` are now in hand. Diffed against the VA's copies — which live at
`varom1.rom` `0x10000` and `0x18000`–`0x1FFFF` (Appendix C):

| bank | bytes differing | windows | of those, landing where the real ROM has `00` |
|---|---|---|---|
| `n88.rom` (32 KiB) | 225 | 31 | 59% |
| `n88_0.rom` | 21 | 8 | 0% |
| `n88_1.rom` | 4340 | 165 | 2% |
| `n88_2.rom` | 80 | 12 | 85% |
| `n88_3.rom` | 192 | 19 | 37% |

`[DERIVED]` **The VA did not rewrite N88-BASIC; it added to it.** Most
changes fall in zero-filled gaps the 8801 ROM left unused — in
`n88_2.rom`, 85% of them. Bank 1 is the apparent exception at 53% of the
bank, and it is not one: from Z80 `7000h` up, the VA's content is the
8801's **displaced by about five bytes**, and the dispatch entries in the
bank's first bucket are adjusted by `+5` to match. An insertion early in
the bank shifted everything after it. The other banks had room and did
not need shifting.

`[ROM]` **What was added is peripheral support, not CPU workaround.** The
largest single addition is 92 bytes at Z80 `3C08h`–`3C63h` in `n88.rom`,
where the 8801 has nothing but zeros:

```
3C08  DB 21        IN A,(21h)          ; 8251 status
      E6 05 / FE 05 / C9
      01 04 04     LD BC,0404h
      07 07 07 07 / 0F                 ; rotate a nibble into place
      D3 10        OUT (10h),A         ; calendar-clock data
      F5 / CD 27 3C / F1 / 10 F6       ; bit-bang loop, four bits
      3E 07 / D3 10 / 0E 02 / C9
3C27  F3           DI
      3A C1 E6     LD A,(0E6C1h)       ; shadow of port 40h
      E6 F9 / B1 / D3 40               ; strobe the clock line
      E6 F9 / D3 40
      32 C1 E6     LD (0E6C1h),A
      FB / C9
```

and two hooks in the low ROM — at `01B7h` and `01D7h`, again in
zero-fill — of which the second is `PUSH AF / CALL 3C56h / POP AF /
LD (HL),C / INC HL / LD (HL),B / RET`, calling into the block above.
`[DERIVED]` Ports `10h`, `21h` and `40h` with a shadow byte at `E6C1h`
is calendar-clock and serial handling: the VA wired those differently
from an 8801 and taught the BASIC ROM about it.

`[DERIVED]` **This closes a loop with Appendix C.** The single byte
separating the VA1 and VA2 Z80 halves is in `n88_0.rom`, in twelve-hour
clock arithmetic — and `n88_0.rom` is precisely the bank whose changes do
*not* land in zero-fill, i.e. the one NEC edited rather than extended.
The RTC support is a VA addition, VA1 shipped it with the `ADD HL,HL`
bug, and VA2 fixed it. Two findings arrived at independently and they
agree.

`[ROM]` **No `HALT` was removed anywhere in the BASIC.** The count of
byte `76h` is identical between the real ROM and the VA's copy in every
bank except bank 1, where it differs by `+2` — accounted for by the
displacement above, not by a patch. `[DERIVED]` So across the machine's
entire 8801-mode ROM complement, NEC found exactly **one** thing that had
to change for the CPU: the three `HALT`s in Debug 8800 (§4.5). Everything
else they touched was peripherals. That is a strong statement about how
close μPD9002 compatible mode is to a real Z80 — one instruction, in one
monitor.

`[ROM]` `disk.rom` differs between the two machines as well, but it is the
FDD sub-CPU's ROM and out of scope (§0).

### 4.7 The undocumented and undefined opcode space, measured

`[MEAS]` The M102 campaign measured the whole space on a real PC-88VA2
over eight disk runs — per-opcode probes with sandboxed pointers, three
input sets, an `RST`-sled instruction-length detector, one-variable-at-a-
time isolation and a full parameter sweep
([evidence](../agents/reports/m102_zexund_qa/README.md); rules R12–R19 in
[`uPD9002-zex-results.md`](uPD9002-zex-results.md) §4, §10). Everything
below is deterministic: repeated runs, shifted code layouts and varied
inputs reproduce the records byte for byte. The flag rules R1–R11 from
M101 apply throughout.

#### 4.7.1 `DD`/`FD`: half-index registers, and the `DD CB` surprise

`[MEAS]` `IXH`/`IXL`/`IYH`/`IYL` exist, for `INC`/`DEC` and the ALU forms,
with Zilog results (`ZEXIY`, `ZEXUND`). Redundant `DD`/`FD` prefix chains
behave as on a Zilog Z80. But the `DD CB`/`FD CB` four-byte space with a
register operand (bits 2–0 ≠ 6) is **not** Zilog:

- `BIT n,(IX+d),r` tests **the register `r`**, not the memory operand;
- `RES`/`SET n,(IX+d),r` modify **only `r`**; memory is untouched;
- the rotate/shift forms keep the Zilog behaviour (operate on
  `(IX+d)`, copy the result to `r`).

`[DERIVED]` The displacement byte is still fetched and consumed in all
forms; only its *use* disappears for BIT/RES/SET. All 1,024 encodings
were measured individually (`CBPRB`, two input sets each).

#### 4.7.2 The undefined `ED` map

`[MEAS]` Outside the documented μPD780 set and `CALLN`/`RETEM`
(§2.2.3–2.2.4), the `ED` page decodes as follows. "IVT word load" and the
other behaviours are defined in §4.7.3–§4.7.5.

| Second byte | Behaviour | Length |
|---|---|---|
| `00`–`3F` | IVT word load into `BC`/`DE`/`HL`/`SP` by bits 5–4 | 2 |
| `4C` | `RETN` (duplicate of `ED 45`) | 2 |
| `54`, `55` | leaves compatible mode; the OS reports an error | 2 |
| `5C` | hangs the machine (one observation) | — |
| `5D` | fault; corrupted the file system once (one observation) | — |
| `64`, `65` | `RRD` variant that also writes `(HL+1)` | 2 |
| `6C`, `6D` | `RLD` variant that also writes `(HL+1)` | 2 |
| `74`, `75`, `77` | IVT word load into `SP` | 2 |
| `7C`, `7D`, `7F`, `80`–`9F`, `A4`–`A7` | word block transfer, one step | 2 |
| `AC` | leaves compatible mode (noted "exit"; screen not recorded) | — |
| `AD`–`AF`, `B4`–`B7` | word block transfer repeated until `BC` = 0 | 2 |
| `BC`–`EC` | consumes **one operand byte**, no visible effect | 3 |
| `EE`–`FC` | leaves compatible mode for the native OS | — |
| `FE`, `FF` | no visible effect | 2 |

Lengths come from the `RST`-sled detector: the 3-byte forms resumed one
byte late, every other surviving form resumed at the next byte. The
mode-exit rows and `5C`/`5D` are observations of the *symptom* only; no
record of machine state across the exit exists, and vaeg does not emulate
them. `4E`/`66`/`6E`/`76`/`7E` (`IM` duplicates) and `LD A,R` were
excluded from the probes by design.

#### 4.7.3 `ED 00`–`3F`: reading the native IVT from Z80 code

`[MEAS]` The most consequential find. Each of these opcodes loads the
register pair selected by bits 5–4 (`00`–`0F`: `BC`; `10`–`1F`: `DE`;
`20`–`2F`: `HL`; `30`–`3F`: `SP`; `74`/`75`/`77`: `SP` again) with the
little-endian word at **bytes `C` and `(C+1) mod 256` of the native
side's physical page 0** — the x86-style interrupt vector table that §6
describes. F becomes: S = bit 7 of the high byte, Z = (word = 0), P =
parity of the low byte; H, N, C and the R1 bits are cleared. Nothing else
matters: A, F, B, the pointer registers and all Z80-visible memory were
varied one at a time without any effect on the value, and relocating the
probe by 100h bytes changes nothing. The index wraps within the page
(`C = FFh` reads bytes `FFh` and `00h`).

Worked example from the real machine, which also decodes the table: with
`C = 30h` the load returns `2800h` — IVT bytes `30h`–`31h`, the offset of
`INT 0Ch` — and with `C = 31h` it returns `0028h`, the same bytes shifted
by one.

`[ROM]` Independent corroboration from the VA2 boot ROM. The vector
installer at `0x13ED` (§6) begins:

```
13ED  33 C0      xor  ax,ax
13EF  8E C0      mov  es,ax        ; ES = 0000 — the IVT
13F1  8B F8      mov  di,ax
13F3  B8 29 23   mov  ax,2329h     ; default handler offset
13F6  8C CB      mov  bx,cs        ;   segment F000h
13F8  B9 00 01   mov  cx,0100h     ; all 256 vectors
13FB  E8 3A 00   call fill
13FE  B8 1F 23   mov  ax,231Fh     ; INT 08h-0Fh
1401  B9 08 00   mov  cx,8
1404  BF 20 00   mov  di,0020h
1407  E8 2E 00   call fill
140A  B8 2A 23   mov  ax,232Ah     ; INT 10h-17h
140D  B9 08 00   mov  cx,8
1410  E8 25 00   call fill
1413  BE 5E 0F   mov  si,0F5Eh     ; then the override table
```

The constants `2329h`, `231Fh`, `232Ah` and segment `F000h` are exactly
the values the Z80-side probes read back out of the hidden page on the
real VA2, vector by vector, including the per-vector overrides. The page
the undefined opcodes read *is* the live IVT.

`[MEAS]` Cross-check in the opposite direction: running the same probes
inside vaeg — whose implementation reads the guest's physical page 0 —
reproduces the real VA2 records except for exactly three vectors that the
guest OS had hooked into RAM at the time (runs 9–10), and on a VA1-ROM
guest returns the VA1 BIOS's different handler addresses with the same
flag rule. The value tracks the machine's actual IVT, not any constant.

#### 4.7.4 `ED 64`/`65`, `6C`/`6D`: three-nibble `RRD`/`RLD`

`[MEAS]` `A` and `(HL)` change exactly as in documented `RRD` (for
`64`/`65`) and `RLD` (for `6C`/`6D`) — and `(HL+1)` changes too:

- `64`/`65`: low nibble of `(HL+1)` ← old low nibble of `(HL)`; high
  nibble preserved.
- `6C`/`6D`: `(HL+1)` ← old `A` low nibble : old `(HL)` high nibble
  (both nibbles replaced).

F = parity of the new `A`; every other flag reads 0. Example
(`A = 5Ah`, `(HL) = 34h`, `(HL+1) = 35h`): `ED 64` gives `A = 54h`,
`(HL) = A3h`, `(HL+1) = 34h` — documented `RRD` plus one extra nibble.

#### 4.7.5 `ED 80`-family: word block transfers

`[MEAS]` `7C`/`7D`/`7F`, `80`–`9F` and `A4`–`A7` all perform the same
single step: the word at `(HL)` is copied to `(DE)`; then `HL += 2`,
`DE += 2`, `BC -= 1`; F is unchanged — an `LDI` that moves words and
counts words. `AD`–`AF` and `B4`–`B7` repeat the step until `BC` = 0, a
word `LDIR`. (`BC` = 0 on entry was not measured; vaeg mirrors `LDIR`
and copies 65,536 words.)

`[DERIVED]` These forms are why the first-generation probe destroyed
itself: with `BC` and `DE` pointing into the probe's own code, every
execution of `7C`–`A7` silently overwrote the program. The crash map of
the early runs (returns to CP/M, hangs, a corrupted directory entry) is
fully reproduced by vaeg running the same unsandboxed probe — a useful
negative control for the whole model.

#### 4.7.6 Reading the shape `[DERIVED]`

A speculative but consistent reading, stated once and not implemented
beyond the measured facts: compatible mode is microcode on a V-series
core whose emulation machinery already contains IVT access (`BRKEM`,
`CALLN`, native interrupts all read or push through page 0, §2.2). The
undefined `ED` rows look like fragments of that machinery reached
without their usual setup — IVT reads indexed by whatever `C` holds,
block moves, and three-byte encodings in `BC`–`EC` that consume an
operand the way `CALLN imm8` (`ED ED imm8`) does, sitting in the same
region of the map. The rows that leave compatible mode (`54`/`55`,
`AC`, `EE`–`FC`) bracket `RETEM` (`ED FD`) and behave like exits taken
with inconsistent state, which the OS then reports as an error. None of
this is verified beyond the guest-visible records; it is recorded as a
shape, not as a mechanism.

## 5. On-chip peripherals

`[VA-TEKU]` The μPD9002 integrates DMA-controller, interrupt-controller
and timer functions analogous to the V50 family.

`[V30-MAN]` Family analogy only, from the V40HL/V50HL data sheet
(`uPD70208H`/`70216H`):

| Data-sheet point | Why it matters |
|---|---|
| V40HL/V50HL are V20/V30 software-compatible. | Supports treating μPD9002 native mode as V30-like before applying VA differences. |
| They list a μPD8080AF emulation function. | Makes the `BRKEM` analogy plausible while leaving `BRKEM2` VA-specific. |
| On-chip CG, WCU, REFU, TCU, SCU, ICU, DMAU blocks. | Matches the kind of block the VA notes describe for μPD9002. |
| 1 MiB memory space, 64 KiB I/O space. | Matches the address-space assumptions in the VA ROM setup code. |
| Internal peripherals mapped into a system I/O area. | Supports reading VA ROM writes to `FFF0h`–`FFFFh` as CPU-internal control setup. |

`[VA-WIKI]` §1.5 gives the μPD9002's own list, "as in the V50":
DMA controller, interrupt controller, timer, serial interface; a
**programmable wait control unit** that inserts 0–3 wait clocks into
memory and I/O cycles; a clock generator; and a refresh unit. Plus the
I/O trap (§9).

`[VA-WIKI]` On the V50, `FFF0h`–`FFFFh` selects CPU pin functions, wait
control, and the I/O addresses of the DMA controller, interrupt
controller, timer and serial interface. The μPD9002 is presumed the same.
The **PC-88VA2 configuration**, as written by its firmware, is:

```
port  value        port  value        port  value
FFFE   11          FFF6   08          FFF2   08
FFFD   07          FFF5   80          FFF0   00
FFFC   01          FFF4   53
FFFB   60
FFFA   88
FFF9   A0
```

(VA1's values are in §5.8; they differ in two bytes.) The rest of §5 decodes
those eleven bytes against the μPD70216 data sheet, which is now in hand.
Every one of them reads, and the result is self-consistent on six
independent checks (§5.4).

### 5.1 The V50 system I/O area

`[V30-MAN]` μPD70216 data sheet, "System I/O Area" and figures 11–18.
The V50 reserves **the whole of `FF00h`–`FFFFh`** as the system I/O area
and populates twelve registers in it. **Byte I/O instructions must be
used.**

| Addr | Reg | Function | Reset |
|---|---|---|---|
| `FFFF` | — | reserved | — |
| `FFFE` | OPCN | on-chip peripheral connection — pin multiplexing (PF), ICU `INT1`/`INT2` source (IRSW) | `----0000` |
| `FFFD` | OPSEL | on-chip peripheral select — enable/disable DMAU, ICU, TCU, SCU | `----0000` |
| `FFFC` | OPHA | peripheral high address `A15`–`A8`, common to all four | **not initialised** |
| `FFFB` | DULA | DMAU low address | **not initialised** |
| `FFFA` | IULA | ICU low address | **not initialised** |
| `FFF9` | TULA | TCU low address | **not initialised** |
| `FFF8` | SULA | SCU low address | **not initialised** |
| `FFF7` | — | reserved | — |
| `FFF6` | WCY2 | wait cycles — DMA and refresh | `----1111` |
| `FFF5` | WCY1 | wait cycles — three memory partitions and I/O | `11111111` |
| `FFF4` | WMB | wait-state memory boundary — partition sizes | `-111-111` |
| `FFF3` | — | reserved | — |
| `FFF2` | RFC | refresh control — enable and interval | RE unaffected, RTM `01000` |
| `FFF1` | — | reserved | — |
| `FFF0` | TCKS | timer clock selection — source and prescaler | `---00000` |

`[V30-MAN]` The data sheet is explicit that devices absent from the reset
table "are not initialized on reset and must be initialized by software".
**The five relocation registers are therefore necessarily
firmware-written**, which is what makes the Explicit? column of §5.2
possible: some bytes prove intent and some cannot.

**Field encodings**, reproduced so this section stands without the PDF:

```
OPCN   b3-b2 IRSW   00 INT1=INTP1 pin, INT2=INTP2 pin
                    01 INT1=SCU,       INT2=INTP2 pin
                    10 INT1=INTP1 pin, INT2=TOUT1
                    11 INT1=SCU,       INT2=TOUT1
       b1-b0 PF     DMARQ3/RxD  DMAAK3/TxD  INTAK/TOUT1/SRDY
                    00  DMARQ3      DMAAK3      INTAK
                    01  DMARQ3      DMAAK3      TOUT1
                    10  RxD         TxD         INTAK
                    11  RxD         TxD         SRDY
OPSEL  b3 SS(SCU)  b2 TS(TCU)  b1 IS(ICU)  b0 DS(DMAU)   1 = enabled
OPHA   b7-b0 = A15-A8, so the block sits at (OPHA x 256)
DULA   b7-b4 = A7-A4          ; DMAU uses A3-A0 to select 16 registers
IULA   b7-b3 = A7-A3, b0 = A0 ; ICU uses A1
TULA   b7-b3 = A7-A3, b0 = A0 ; TCU uses A2,A1 -> TCT0/TCT1/TCT2/TMD
SULA   b7-b3 = A7-A3, b0 = A0 ; SCU uses A2,A1
WCY1   b7-b6 IOW  b5-b4 UMW  b3-b2 MMW  b1-b0 LMW  00..11 = 0..3 waits
WCY2   b3-b2 DMAW b1-b0 RFW                        00..11 = 0..3 waits
WMB    b6-b4 LMB  b2-b0 UMB
       000=32K 001=64K 010=96K 011=128K 100=192K 101=256K 110=384K 111=512K
       middle partition = whatever is left between them
RFC    b7 RE (1 = refresh enabled)   b4-b0 RTM
       RTM 00000..00011 -> N = 17,18,19,20 ; 00100..11111 -> N = 5..32
       refresh interval = 8 x N x tCYK
TCKS   b4,b3,b2 = CS2,CS1,CS0 (0 = internal clock, 1 = TCLK pin)
       b1-b0 PS: 00 = /2, 01 = /4, 10 = /8, 11 = /16  (of CLKOUT)
```

`[DERIVED]` Three consequences bear on the rest of this document:

1. **No VA peripheral, internal or external, may live in `FF00h`–`FFFFh`.**
   The V50 reserves the region wholesale.
2. **The μPD9002's I/O-trap registers at `FFE0h`–`FFEFh` (§9.1) occupy
   space the V50 reserved and left unpopulated.** NEC added the trap
   without displacing a single V50 register and without breaking the V50
   programming model — a much cheaper design story than the alternative,
   and consistent with `FFE0h`–`FFFFh` being exempt from trapping (§9.1).
   This replaces the vaguer earlier statement that the trap ports merely
   "sit in the same high I/O region".
3. **Access width is per register, not per region.** The V50 requires
   byte I/O throughout the system I/O area, yet CoBit's code writes
   `FFE0h`/`FFE2h`/`FFE4h`/`FFE6h` as words (§9.1) — correctly, since
   those hold 16-bit port numbers. The byte-only rule is a V50 rule about
   V50 registers; the 9002 additions need not obey it. Implement the
   width register by register.

`[VA-TM]` **The manual's own I/O port block map confirms the first two.**
Its §2.2 divides the 64 KiB I/O space as:

```
0000H - 00FFH   system area 0    (88MH/FH compatible)
0100H - 01FFH   system area 1
0200H - 02FFH   frame buffer control
0300H - 04FFH   colour palette control
0500H - 05FFH   GVRAM control
0600H - 0FFFH   system area 2    (reserved)
1000H - FEFFH   user area
FF00H - FFFFH   system area 3    (CPU internal use)
```

`[DERIVED]` `FF00h`–`FFFFh` is labelled **CPU internal use** — the V50's
system I/O area, reserved at the VA level exactly as consequence 1
inferred, and therefore the space into which the μPD9002's I/O-trap
registers were added without displacing anything (consequence 2).
`0100h`–`01FFh` is a documented **system** area, which is where OPHA puts
the on-chip peripherals (§5.2), so that placement is architectural rather
than a free choice by the firmware. And the `0000h`–`00FFh` row names the
8-bit compatible space that §5.4 check 3 rests on.

### 5.2 The PC-88VA2 configuration decoded

`[VA-WIKI]` supplies the bytes. `[V30-MAN]` supplies the layouts.
`[DERIVED]` is the reading, and it is sound only if the μPD9002 kept the
V50 layout — §5.4 is the evidence that it did.

**Reset** is the V50 power-on value; **Explicit?** follows from it, since
where the two differ the firmware demonstrably wrote the byte.

| Addr | Reg | Val | Reset | Explicit? | Bit decode | Reading |
|---|---|---|---|---|---|---|
| `FFFE` | OPCN | `11` | `00` | yes | `0001 0001` → IRSW=`00`, PF=`01`, **b4=1 (undefined on V50)** | `INT1` ← `INTP1` pin, `INT2` ← `INTP2` pin. DMA ch3 pins carry `DMARQ3`/`DMAAK3`; the `INTAK/TOUT1/SRDY` pin outputs **`TOUT1`** |
| `FFFD` | OPSEL | `07` | `00` | yes | SS=0, TS=1, IS=1, DS=1 | **SCU disabled**; TCU, ICU, DMAU enabled |
| `FFFC` | OPHA | `01` | uninit | yes | `A15`–`A8` = `01h` | internal peripheral block at I/O **`0100h`–`01FFh`** |
| `FFFB` | DULA | `60` | uninit | yes | `A7`–`A4` = `0110` | **DMAU = `0160h`–`016Fh`** (`A3`–`A0` select the register) |
| `FFFA` | IULA | `88` | uninit | yes | `A7`–`A3` = `10001`, `A0`=0 | **ICU = `0188h` / `018Ah`** (`A1` selects) |
| `FFF9` | TULA | `A0` | uninit | yes | `A7`–`A3` = `10100`, `A0`=0 | **TCU = `01A0h`/`01A2h`/`01A4h`/`01A6h`** = TCT0/TCT1/TCT2/TMD |
| `FFF8` | SULA | *unwritten* | uninit | — | — | consistent with SS=0 |
| `FFF6` | WCY2 | `08` | `0F` | yes | DMAW=`10`, RFW=`00` | DMA cycles 2 waits; refresh cycles 0 waits |
| `FFF5` | WCY1 | `80` | `FF` | yes | IOW=`10`, UMW=`00`, MMW=`00`, LMW=`00` | **I/O 2 waits; all three memory partitions 0 waits** |
| `FFF4` | WMB | `53` | `77` | yes | LMB=`101`, UMB=`011` | lower 256 KiB (`00000`–`3FFFF`), upper 128 KiB (**`E0000`–`FFFFF`**), middle = the remainder |
| `FFF2` | RFC | `08` | RE unaffected, RTM=`01000` | yes — §5.7 | RE=0, RTM=`01000` (N=9) | **internal DRAM refresh disabled**; RTM left at its reset value |
| `FFF0` | TCKS | `00` | `00` | yes — §5.7 | CS2=CS1=CS0=0, PS=`00` | all three counters take the internal clock, prescaled ÷2 |

`[ROM]` These are the VA2's values. VA1 differs in two bytes only —
WCY2 and WCY1 — and is otherwise identical; see §5.8.

`[ROM]` **All eleven are explicit.** Nine can be shown so from the reset
values alone, and the boot ROM's initialisation table settles the
remaining two independently: it writes `FFF2h` and `FFF0h` as well, whose
values merely happen to coincide with their reset state. See §5.7. An
earlier version of this section warned that those two might never have
been written; that caveat is withdrawn.

### 5.3 The resulting configuration

`[DERIVED]` The machine state those eleven bytes describe, in the form an
implementation needs.

**I/O map.** Internal peripherals occupy `0100h`–`01FFh`:

```
0160h - 016Fh   DMAU   (16 registers, A3-A0)
0188h / 018Ah   ICU    (A1 selects; see the alias note below)
01A0h / 01A2h   TCU    TCT0 / TCT1
01A4h / 01A6h   TCU    TCT2 / TMD
   (SCU absent)
```

`[DERIVED]` **ICU aliasing.** IULA stores `A7`–`A3` and `A0`; the ICU
itself uses `A1`. `A2` is neither stored nor used, so it cannot take part
in the compare, and the ICU must also answer at `018Ch`/`018Eh`. The same
reasoning gives the TCU no alias, since it uses both `A2` and `A1`.
Confirm before relying on it — §15.2 `icu-alias`.

**Clocking.** CLKOUT = 15.9744 MHz crystal ÷ 2 = 7.9872 MHz (§12), so
`tCYK` ≈ 125.2 ns. The TCU takes CLKOUT ÷ 2 = **3.9936 MHz** on all three
counters.

**Pins.** `TOUT1` is exported on the `INTAK/TOUT1/SRDY` pin, so the CPU's
`INTAK` is **not available** — see §5.4 check 6 for what that implies.
`DMARQ3`/`DMAAK3` are real DMA lines, not `RxD`/`TxD`.

**Waits.** Memory 0, I/O 2, DMA 2, refresh 0. `[V30-MAN]` The WCU inserts
the programmed count unconditionally and *then* honours `READY`, so zero
programmed memory waits means **the VA gates memory timing entirely with
external `READY`**. Any future cycle-accuracy work must model the external
logic, not this register. §11 item 10 still applies: do not copy timing
from another emulator.

**Memory partitions.** `00000`–`3FFFF` lower, `E0000`–`FFFFF` upper,
`40000`–`DFFFF` middle. With all three wait counts at zero the boundaries
are presently inert; they are set up as if for a configuration that uses
them.

**Refresh.** The on-chip RCU is off. Something else must refresh VA DRAM
— §5.5(b).

### 5.4 Why the V50 layout is the right key

`[DERIVED]` Seven checks, none of which was an input to the decode.
Checks 1–4 are internal to §5.2; 5 and 6 come from the IC78 pin scan
(§12, Appendix D) and are independent of the data sheet entirely; 7
comes from the boot ROM and is independent of both.

1. **Address accounting closes.** The eleven-byte list has exactly five
   gaps in `FFF0h`–`FFFFh`, and all five are explained without appeal:
   `FFFFh`, `FFF7h`, `FFF3h` and `FFF1h` are reserved on the V50, and
   `FFF8h` (SULA) is unwritten because OPSEL sets SS=0. A byte list keyed
   to some other register map would not have holes in exactly those five
   places.
2. **WMB reproduces the VA memory map.** The upper partition decodes to
   128 KiB, i.e. `E0000h`–`FFFFFh` — the ROM window, exactly. The lower
   decodes to 256 KiB of base RAM. Neither was an input to the arithmetic.
3. **OPHA puts the peripherals above the 8-bit I/O space.** V1/V2 mode
   addresses ports `00h`–`FFh`; placing the internal block at `01xxh` is
   what coexistence with compatible mode requires. This is the same design
   pressure that produced the I/O trap (§9). `[VA-TM]` The manual makes
   this one documented rather than inferred: `0000h`–`00FFh` is
   "system area 0 (88MH/FH compatible)" and `0100h`–`01FFh` is
   "system area 1" (§5.1).
4. **TCKS, OPCN and OPSEL tell one story.** SCU off + `TOUT1` on the pin +
   counters at 3.9936 MHz is external-USART baud generation and nothing
   else. The arithmetic is exact: 3.9936 MHz ÷ 26 = 153.6 kHz = 9600 × 16,
   ÷ 13 = 19200 × 16. Three registers written by three separate firmware
   `OUT`s land on a single coherent configuration.
5. **`IC78` has an `XDMAK3` pin (39).** That pin can only be fed by the
   CPU's `DMAAK3` output, which exists only when PF is `00` or `01` — with
   PF `10`/`11` the CPU pin is `TxD` and there is no `DMAAK3` in the
   machine. The decode says `01`. This does not distinguish `01` from
   `00`, but it eliminates half the field from the hardware side.
6. **`IC78` carries `RXRDY` (45) and `DCD` (74), and the CPU has given up
   `INTAK`.** An external asynchronous serial controller therefore exists,
   which is exactly what OPSEL SS=0 requires, and `TOUT1` is its baud
   clock — that is check 4 confirmed at the connector. It also explains
   the otherwise odd choice of PF=`01` over `00`: `01` trades `INTAK` away
   for `TOUT1`, which is only survivable if the interrupt acknowledge is
   generated elsewhere. `IC78` has `XAITAK` (68), `XPIC` (61), `PICINT`
   (51) and `IR3`/`IR5`/`IR7`, i.e. it sits between the CPU and the
   external μPD8259A of p.248 and is the obvious source. Pin directions
   are not in the scan, so treat the last step as a lead.

7. **The firmware exercises every decoded address.** The same boot ROM
   that writes the eleven bytes then writes an eight-step ICU
   initialisation to `0188h`/`018Ah`, a mode-and-count sequence to
   `01A6h`/`01A2h`, and a DMA-unit reset to `0160h` — the exact addresses
   the relocation registers decode to, with values that are meaningful
   only there. This is the strongest of the seven; see §5.7.

`[DERIVED]` Conclusion: **the μPD9002 keeps the V50 system I/O area
essentially unchanged.** With check 7 the on-chip peripheral map is no
longer an inference at all — the firmware uses the decoded addresses, so
the decode is confirmed by the machine rather than merely consistent with
it (§16). What remains open is one undefined bit (§5.5(a)) and who owns
refresh (§5.5(b)), neither of which touches the map.

### 5.5 Anomalies and loose ends

**(a) OPCN bit 4 is set, and the V50 defines bits 7–4 as unused.**
`[DERIVED]` OPCN resets to `00h`, so `11h` is a deliberate firmware write
*including* that bit — it is not a leftover. `[ROM]` And `11h` is a
**literal byte in the boot ROM's initialisation table** (§5.7), so the
wiki did not mistranscribe it and the value is not computed at run time.
Two candidates remain: a μPD9002 extension — the obvious guesses being
mode- or trap-related pin control, given what else the 9002 added in this
region — or a bit the firmware sets and the silicon ignores. One
discriminator is left, and it is cheap: write `11h` to `FFFEh` and read it
back; a stored 1 proves an extension. §15.2 `opcn-bit4`.

**(b) RFC RE=0 — the on-chip refresh unit is disabled, deliberately.**
`[ROM]` The boot ROM writes `08h` to `FFF2h` explicitly (§5.7). RE is
"unaffected by RESET", so before that write was found this value was
equally consistent with a bit that powered up clear and was never
touched; it no longer is. **NEC chose to disable the on-chip refresh
unit**, which means the VA refreshes DRAM externally — GAL-1 or the
display subsystem are the candidates. The cheap check is on the
schematics already in hand: **trace the μPD9002 `REFRQ` pin**. `[ROM]`
Done — and it is not wired at all: neither p.248 nor p.250 carries a
single label containing `REF` (§12). So the CPU contributes nothing to
refresh, by two independent decisions: the unit is disabled in software
and its output goes nowhere in hardware. What *does* refresh VA DRAM is
still open, but it is now a question about the memory and display sheets,
not about the CPU. §15.2 `refresh-owner`.

**(c) The firmware writer, located.** `[ROM]` It is a table walked at
reset in **`varom1_va2.rom`**, not in `varom00_va2.rom`. §5.7 gives the
tables, the walker and the full ordered write list.

`[ROM]` Recorded so the search is not repeated: `varom00_va2.rom` was
scanned in full and contains **no write to `FFF0h`–`FFFEh` by any
addressing route** — no `mov dx,imm16` with an immediate ≥ `FFF0h`, no
split `dh`/`dl` load, no port table in any plausible layout, no
occurrence of the distinctive value run `53 80 08`, and only two windows
in the whole 512 KiB holding four or more of the nine configuration
values as `mov al,imm8`, both of which disassemble to unrelated dispatch
code. Its last 32 bytes are `FFh`, so it does not carry the reset entry
either. That image is not the boot ROM; see Appendix C.

### 5.6 Implementation note

`[DERIVED]` For vaeg the operative facts are the three peripheral base
addresses (`0160h`, `0188h`, `01A0h`), the absent SCU, the 3.9936 MHz
timer clock, and `TOUT1` leaving the chip. Implement the relocation
registers rather than hardcoding the addresses — they are software-set,
VA1 has not been checked, and a V3-mode program is free to move them. See
§11 item 11.

`[DERIVED]` §5.7 is also a ready-made bring-up test. The reset path
programs the peripherals from a table in a fixed, known order, so an
emulator that boots this ROM can be checked write-for-write against it
before anything else in the machine has to work — and the ordering hazard
noted at the end of §5.7 is exactly the kind of thing a naive
implementation gets wrong silently.

### 5.7 The reset-time initialisation sequence

`[ROM]` From `varom1_va2.rom` (Appendix C). The reset vector at image
offset `0xFFF0` is `EA 00 00 00 F0` — `jmp F000:0000` — and offset 0 is a
near jump to **`0x12B8`**, the reset entry. Twelve instructions later the
CPU programs its own peripherals, from two tables with two different
formats and two separate walkers.

**Table A — `0x0F20`, walked at `0x12ED`.** Variable-length descriptors,
`count, port_word, value[count]`, ports written **descending** from
`port_word`, byte `OUT` only, terminated by a zero count:

```
        mov  si,0F20h
grp:    mov  cl,cs:[si]        ; count
        and  cx,cx
        je   done              ; 0 = end of table
        inc  si
        mov  dx,cs:[si]        ; first port
        inc  si
        inc  si
val:    mov  al,cs:[si]
        out  dx,al             ; byte OUT
        dec  dx                ; next port, descending
        inc  si
        loop val
        jmp  grp
```

Its five descriptors, expanded:

| Descriptor | Ports | Writes |
|---|---|---|
| `06 FE FF` | `FFFE`→`FFF9` | OPCN `11`, OPSEL `07`, OPHA `01`, DULA `60`, IULA `88`, TULA `A0` |
| `03 F6 FF` | `FFF6`→`FFF4` | WCY2 `08`, WCY1 `80`, WMB `53` |
| `01 F2 FF` | `FFF2` | RFC `08` |
| `01 F0 FF` | `FFF0` | TCKS `00` |
| `08 E7 FF` | `FFE7`→`FFE0` | I/O trap ranges — §9.1 |

`[ROM]` **This is the source of the eleven bytes in §5.2**, and it
reproduces the wiki's list exactly. Three things follow that the byte list
alone could not give:

1. **The group boundaries land on the reserved addresses.** The firmware
   skips `FFFFh`, `FFF8h` (SULA), `FFF7h`, `FFF3h` and `FFF1h` by
   *construction* — the descriptors stop and restart around them. §5.4
   check 1 was an inference from a byte list; it is now the firmware's own
   structure.
2. **All eleven writes are explicit**, `FFF2h` and `FFF0h` included.
3. **`11h` is a literal in the image**, so OPCN bit 4 is not a
   transcription error (§5.5(a)).

**Table B — `0x0F43`, walked at `0x1307`.** Nine fixed records of
`port_word, value`, byte `OUT`, no descending run — a plain
`mov dx,cs:[si] / mov al,cs:[si] / out dx,al` loop with `cx = 9`:

| # | Port | Value | Register | Meaning |
|---|---|---|---|---|
| 1 | `01A6` | `76` | TCU TMD | SC=`01` → TCT1; RWM=`11` → low byte then high; CMODE=`011` → **mode 3, square wave**; BD=0 → binary |
| 2 | `01A2` | `82` | TCU TCT1 | count, low byte |
| 3 | `01A2` | `06` | TCU TCT1 | count, high byte → **`0682h` = 1666** |
| 4 | `0188` | `11` | ICU IIW1 | D4=1 as the format requires; LEV=0 → edge-triggered; SNGL=0 → **slaves present**; I14=1 → IIW4 follows |
| 5 | `018A` | `00` | ICU IIW2 | vector base bits `V7`–`V3` |
| 6 | `018A` | `80` | ICU IIW3 | **`INT7` is a slave input** |
| 7 | `018A` | `03` | ICU IIW4 | bit 0 required; SFI=1 → self-finish mode; EXTN=0 → normal nesting |
| 8 | `018A` | `7F` | ICU IMKW | interrupt mask — **only `INT7` unmasked** |
| 9 | `0160` | `01` | DMAU DICM | RES=1 → software reset of the DMA unit |

`[DERIVED]` **Table B is the decisive confirmation of §5.2.** Every base
address the relocation registers decode to is exercised, and every value
is meaningful *only* at the address it is written to. An eight-step
μPD71059-style initialisation — IIW1 at `A1=0`, then IIW2, IIW3, IIW4 and
finally IMKW at `A1=1` — does not land by accident on a particular pair of
ports, and `DICM` with `RES` set is exactly what one writes first to a
DMAU at register offset 0. The `0188h`/`018Ah` split independently
confirms the `A1` decode of §5.3, and the slave-on-`INT7` cascade confirms
the external μPD8259A of §12. `[ROM]` Other routines in the same image
use `0188h`/`018Ah` (five and four sites) and `01A2h`/`01A4h`/`01A6h`, so
this is not a one-off.

`[DERIVED]` **Mode 3 on TCT1 closes §5.4 check 4.** The μPD70216 data
sheet notes that driving a baud-rate clock requires the TCU in mode 3 with
a square-wave output; that is exactly what the firmware programs, on the
one counter whose output OPCN routes off-chip. `[UNKNOWN]` The count
`0682h` = 1666 gives 3.9936 MHz ÷ 1666 ≈ 2.397 kHz, which is not a
recognisable baud multiple on its own. The external USART's own divisor is
not in this document, and a serial driver would presumably reprogram the
count when a port is opened.

`[DERIVED]` **One ordering hazard, worth knowing before implementing.**
OPSEL (`FFFDh` ← `07`) enables the DMAU, ICU and TCU **four writes before**
OPHA, DULA, IULA and TULA place them — and those four relocation registers
are *not* reset-initialised (§5.1). For four `OUT`s the three peripherals
are therefore enabled at indeterminate I/O addresses. Nothing performs I/O
in that window, so it is benign on hardware. But an emulator must not
assume a peripheral is unreachable before it is relocated, and the
plausible-looking "relocate first, then enable" ordering — which an earlier
version of this section recommended — is **not** what the machine does.

### 5.8 VA1 versus VA2

`[ROM]` The VA1 boot ROM `varom1.rom` carries the same two tables with the
same two walkers, at different offsets: reset entry `0x0A6F`, table A at
`0x0930` walked at `0x0A7D`, table B at `0x0953` walked at `0x0A97`. The
two images are **different builds**, not one build with different data —
48.8% of their bytes differ and every offset has moved.

**System I/O area: exactly two bytes differ.**

| Port | Reg | VA1 | VA2 | |
|---|---|---|---|---|
| `FFFE` | OPCN | `11` | `11` | |
| `FFFD` | OPSEL | `07` | `07` | |
| `FFFC` | OPHA | `01` | `01` | |
| `FFFB` | DULA | `60` | `60` | |
| `FFFA` | IULA | `88` | `88` | |
| `FFF9` | TULA | `A0` | `A0` | |
| `FFF6` | WCY2 | **`05`** | **`08`** | VA1 DMA 1 wait, refresh 1; VA2 DMA 2 waits, refresh 0 |
| `FFF5` | WCY1 | **`84`** | **`80`** | VA1 middle memory block **1 wait**; VA2 none. I/O 2 waits, lower and upper blocks 0, on both |
| `FFF4` | WMB | `53` | `53` | |
| `FFF2` | RFC | `08` | `08` | |
| `FFF0` | TCKS | `00` | `00` | |
| `FFE0`–`FFE7` | trap ranges | `50`–`5B` / `60`–`6F` | identical | |

`[DERIVED]` So the peripheral map, the pin multiplexing, the memory
partition boundaries, the timer clock, the refresh decision and the
I/O-trap windows are **identical across the two machines**. All of §5.2
and §5.3 applies to VA1 unchanged apart from the wait counts, and the only
memory-timing difference is the middle partition `40000h`–`DFFFFh`, where
VA1 inserts one wait state and VA2 none. RFW is inert on both, since RE=0
means no refresh cycles ever occur — and the two machines disagreeing
about a field that never applies is a small further sign that RE=0 is
deliberate rather than incidental (§5.5(b)).

**Internal peripheral initialisation: VA1 does more.** VA1's table B holds
fourteen records (`mov cx,0Eh`), VA2's nine (`mov cx,9`). The first nine
are byte-identical — the TCU, ICU and DMAU-reset sequence of §5.7. VA1
adds five, all in the DMAU block:

| Port | Value | Register | Meaning |
|---|---|---|---|
| `0161` | `01` | DMAU DCH | select **channel 1**, base and current |
| `016A` | `C0` | DMAU DMD | TMODE=`11` → **cascade mode**; increment; no autoinitialise; verify; byte |
| `0168` | `00` | DMAU DDC low | DMA enabled, fixed priority, normal write |
| `0169` | `02` | DMAU DDC high | bus release mode, wait enabled during verify |
| `016F` | `0D` | DMAU DMK | mask — **only channel 1 unmasked** |

`[DERIVED]` VA1 therefore brings up **DMA channel 1 as a cascade channel**
— that is, with a slave μPD71071 behind it — at reset. VA2 resets the
DMAU and stops. This is also a sixth confirmation of the DMAU base
address: five further registers in the `0160h`–`016Fh` block, each
decoding sensibly in the μPD71071 register model that the DMAU is
compatible with.

`[UNKNOWN]` Whether VA2 lacks the slave controller, or configures it later
from code not on the reset path. Worth settling before any DMA work,
because it is a machine-model difference rather than a timing tweak.

---

# Part II — PC-88VA firmware conventions

Everything in Part I is CPU behaviour. Everything here is convention
built on top of it by the VA firmware, and belongs in emulated firmware
rather than in the CPU model.

## 6. Interrupt vector table

`[ROM]` **Provenance note.** The offsets in this section and in §8 are
those of **`varom1_va2.rom`**, the VA2 boot ROM, although earlier drafts
labelled them `VAROM1.ROM`. The VA1 image in hand does not have them —
see Appendix A item 27. VA1's counterparts are given alongside below.

`[ROM]` The vector installer at `0x13ED`, called from `0x1364`, fills the
IVT with safe defaults and then applies an override table at `0x0F5E`
(VA1: table at `0x097D`). The entries that matter here:

```
        VA2 (varom1_va2.rom)   VA1 (varom1.rom)
7C  ->  F000:1944              F000:1024   I/O trap, IN
7D  ->  F000:1944              F000:1024   I/O trap, OUT  (same handler as 7C)
7E  ->  F000:1920              F000:1000
90  ->  1000:0000              1000:0000   BRKEM2 target — V1/V2 entry
91  ->  F000:24B0              F000:1640   CALLN 91h service
95  ->  1000:E000              1000:E000   CALLN 95h service
```

`[DERIVED]` The comparison is worth more than either column alone. The two
vectors whose targets are **ABI conventions** — `90h` and `95h` — are
*identical* on both machines, while all four that point into ROM code
moved with the build. That is direct evidence that `1000:0000` and
`1000:E000` are contracts rather than accidents of layout, which is what
§7.2 asserts about `CALLN 95h`.

Three further confirmations fall out of the table:

- `[DERIVED]` `7Ch` and `7Dh` point at **one shared handler**, exactly as
  98IOE's own installer does (§9.5). The handler distinguishes IN from
  OUT by decoding the trapped opcode, not by which vector fired.
- `[DERIVED]` Vector `90h` = `1000:0000` is the V1/V2 code entry, and it
  fixes `PS = 1000h` for compatible mode.
- `[DERIVED]` **`CALLN 91h` and `95h` are ordinary IVT entries.** They
  are *not* hardcoded CPU behaviour. `CALLN imm8` remains uniformly "call
  `vec(imm8)`", and the "fixed services" are firmware conventions. This
  is the single most important structural correction in this document:
  an implementation that special-cases `91h`/`95h` inside the CPU is
  modelling the wrong layer.

## 7. CALLN native services

`[VA-TM]` The VA provides native-service entry points so V1/V2-compatible
code can reach V3 memory and I/O or call native V30 code.

### 7.1 CALLN 91h — V3 memory and I/O access

Encoded `ED ED 91`. The caller selects the operation in `A`:

| `A` bit | Clear | Set |
|---|---|---|
| 2 | memory access | I/O access |
| 1 | read / input | write / output |
| 0 | byte | word |

| Register | Meaning |
|---|---|
| `HL` | segment for memory access; unused for I/O |
| `DE` | memory offset, or I/O port address |
| `BC` | value for write/output; result for read/input |

`[ROM]` Vector `91h` → `F000:24B0`, agreeing with the documented service
number.

### 7.2 CALLN 95h — user native routine

Encoded `ED ED 95`. Calls the user native routine at `1000:E000` — that
is, offset `E000h` in the V1/V2 window (§3.3). Compatible-mode software
prepares a native routine there and invokes it.

The routine returns to compatible mode with **`IRET`, not `RETEM`**
(§2.2). `[ROM]` Vector `95h` → `1000:E000`, independently agreeing with
the documented ABI.

## 8. The V1/V2 handoff in VAROM1.ROM

`[ROM]` **Provenance note.** As in §6, the offsets below are
`varom1_va2.rom`'s. The VA1 image has the same code at different
addresses: reset entry `0x0A6F`, `0F FE 90` at **`0x0B15`**, the `cli` it
returns to at `0x0B18`, and `ljmp 1000h:c003h` at `0x0B31` — the same
shape, so nothing in the argument below depends on which image is read.

`[ROM]` One branch precedes the handoff. After installing vectors, the
ROM tests port `000Dh` bit 2 through the helper at `0x13E6`:

```asm
1364: call 13edh       ; install vectors
1367: call 13e6h       ; read port 000Dh, mask 04h
136a: jz   13d2h       ; alternate path — no BRKEM2 in this block
```

The non-zero path enables I/O traps by calling `0x18E7`, which writes
`03h` to `FFEFh` (IN + OUT traps, byte-port matching), runs memory and
display setup, and reaches the handoff. The zero path sets `[2F07h]=1`,
writes `91h` to port `FFh`, runs the shared initialisation, and jumps
back into the ROM without executing `BRKEM2`.

The handoff itself:

```asm
13a8: mov ax,1000h
13ab: mov ds,ax        ; DS0 = 1000h — the V1/V2 window
13ad: xor ax,ax
13af: mov es,ax
13b1: 0f fe 90         ; BRKEM2 90h  ->  PS:PC = 1000:0000
13b4: cli              ; <- RETEM returns here
```

`[DERIVED]` Evidence that `0x13B1` is intended as `BRKEM2 90h` is
strong: the surrounding stream is valid 16-bit V30 setup code, the bytes
sit at an instruction boundary after `mov es,ax`, `0F FE nn` is the
VA-specific encoding, and the ROM has just installed vector `90h` as
`1000:0000`.

`[ROM]` **It is not reached in every configuration, and one bit decides.**
The full branch, disassembled:

```asm
1364: e8 86 00     call 13EDh          ; install vectors
1367: e8 7c 00     call 13E6h          ; mov dx,000Dh / in al,dx / and al,04h / ret
136a: 74 66        jz   13D2h          ; bit 2 clear -> V3 path, no BRKEM2
136c: e4 40        in   al,40h         ; secondary test, inside the V1/V2 path
136e: 24 08        and  al,08h
1370: 75 03        jnz  1375h
1372: e8 1b 0c     call 1F90h
1375: e8 6f 05     call 18E7h          ; enable the I/O trap
1378: b8 00 80     mov  ax,8000h       ; ... on to the handoff at 13A8/13B1

13d2: c6 06 07 2f 01  mov byte [2F07h],1
13d7: b0 91           mov al,91h
13d9: e6 ff           out 0FFh,al
13db: b8 00 30        mov ax,3000h
13de: 8e d0           mov ss,ax
13e0: e8 2d 0e        call 2210h       ; the shared initialiser
13e3: e9 6b ee        jmp  0251h       ; back into the ROM — 13B1 never executes
```

`[DERIVED]` So `doc-boot-cover` is answered: **no.** Whether the machine
enters compatible mode at all is decided by **port `000Dh` bit 2**, read
once, immediately after the vectors are installed. Clear takes the V3
path, which sets `[2F07h]`, writes `91h` to port `FFh`, moves the stack to
`3000:`, runs the same initialiser the resume block calls (§8's `0x2210`)
and re-enters the ROM at `0251h` — never touching `BRKEM2`. Set takes the
V1/V2 path, which brackets the handoff with the I/O trap.

`[VA-TM]` **The manual's own boot flowchart carries the same branch, and
it names a segment.** §1.4「システム起動プロセス」 draws three
destinations. `PC` key on → `V3 INIT` / `BIOS Set Up`. `PC` key off and
`SW7` off — or `SW7` on with no V3 IPL on the disk — → `IDP V1/V2` and
`I/O trap ON` → `Jmp V1/V2` → `N88-BASIC (ROM / Disk)`. `SW7` on with a
V3 IPL → `Load IPL`, one sector to `3000:0` → `V3 INIT` / `BIOS INIT` →
`Jmp IPL` with `CS:IP = 3000H:0` and `SS:SP` around `3000H:FFFEH` → DOS
boot.

`[DERIVED]` **`3000h` is the join.** The zero path at `0x13D2` sets
`SS = 3000h` before it calls the shared initialiser, and the flowchart's
IPL path loads to `3000:0` and enters with the stack in the same segment.
A shared constant is not proof — `3000:` is a plausible V3 stack wherever
it turns up — but it is the only segment either account fixes, and they
fix the same one. Read that way, `0x13D2` is the flowchart's **right-hand
column** rather than its `PC`-key column, which makes port `000Dh` bit 2
the **V1/V2-boot versus V3-IPL-boot** selector and puts the `PC`-key test
somewhere ahead of `0x1364`. `[UNKNOWN]` Which line actually drives the
bit is still open — `bootsel-000d` in §15.2, with a test in §17.1 that
needs no instrument.

**The resume block.** `[ROM]` The bytes after the handoff disassemble as
ordinary V30 code:

```asm
13b4: cli
13b5: call 18eeh       ; writes 00h to FFEFh — disable I/O trap
13bd: mov dx,0152h
13c0: in  ax,dx
13c1: or  ax,4000h     ; system memory mode -> V3   (see §8.2)
13c4: out dx,ax
13cd: jmp 1000h:c003h
```

`[DERIVED]` This only makes sense if compatible-mode code eventually
executes `RETEM` and returns to `0x13B4`. The trap-enable at `0x18E7`
before the handoff and the trap-disable at `0x18EE` after it bracket the
compatible-mode session exactly — **direct firmware evidence that the I/O
trap is the V1/V2 I/O compatibility mechanism** (§9).

`[DERIVED]` The far jump target `1000:C003` is physical `0x1C003` — an
offset in the V1/V2 window, i.e. RAM from the CPU's point of view, not
`VAROM1.ROM` offset `0xC003`. This is the post-V3-initialisation
destination the technical manual describes, and it is reached by an
explicit `jmp`, **not** by any special `RETEM` behaviour.

`[UNKNOWN]` What prepares the code at `1000:C003` — but the question has
been narrowed hard, and it turns out not to be a question on its own. See
§8.1.

### 8.1 Nothing in these ROMs ends the compatible-mode session

`[ROM]` Three searches across `varom1.rom`, `varom1_va2.rom` and
`varom00_va2.rom`:

1. **The native side does not prepare `1000:C003`.** In each boot ROM the
   immediate `1000h` is loaded exactly twice: once at the handoff, where
   it becomes `DS` for compatible mode, and once as a parameter to a
   bank-select call (`0x1348`/`0x1C71`, which passes `1000h` or `2800h`
   in `AX` and writes neither). No copy loop targets segment `1000h`, and
   the immediate `C003h` appears nowhere in any image except inside the
   `ljmp` itself.
2. **No image contains `ED FD`.** Not the VA1 boot ROM, not the VA2 boot
   ROM, not `varom00_va2.rom`, and not any of the seven PC-8801-mode ROMs
   — `n88.rom`, the four N88 extension banks, `n80.rom` or `disk.rom`
   (Appendix C). **Ten images, zero occurrences** of the `RETEM` encoding.
   The `ED ED` hits are the `CALL 0EDEDh` false positives already recorded
   in Appendix A item 14; they reappear in `n88_0.rom` at `0x31C` and
   `n88_1.rom` at `0xB90`, which is simply the same two sites seen through
   the per-bank dumps.

   `[DERIVED]` This is now close to exhaustive for the machine's ROM
   complement. The compatible-mode code the VA can execute without a disk
   is N88-BASIC, its four extension banks, N-BASIC and Debug 8800, and
   **none of it can leave compatible mode.**
3. **The code entered by `BRKEM2 90h` is an ordinary N88-BASIC cold
   start.** Vector `90h` is `1000:0000`, and both images hold
   `F3 31 A0 E1 C3 E5 3B` — `DI` / `LD SP,0E1A0h` / `JP 3BE5h` — at the
   ROM offset that maps there. VA1 additionally pre-loads `BP` (the
   compatible-mode `SP`) with `0E1A0h` at `0x0B09` before the handoff,
   duplicating what the Z80 code does for itself; VA2 dropped that
   instruction.

`[DERIVED]` Put together: **the resume block at `0x13B4` / `0x0B18` is
unreachable from anything in these images.** `RETEM` is the only
instruction that ends a `BRKEM`-shaped session (§2.2), no ROM in hand
contains one, and the code the handoff enters is BASIC. On a plain V1/V2
boot the machine therefore goes into N88-BASIC and stays there — which is
exactly what a PC-8801 does — and the V3 resume path never runs.

`[VA-TM]` **The manual's boot flowchart draws no return either.** §1.4
ends the V1/V2 branch at `N88-BASIC (ROM / Disk)` with a dashed arrow and
no path back, exactly as it ends the DOS branch. That is corroboration
rather than proof — a boot diagram is not obliged to show what loaded
software does afterwards — but the same manual documents the post-`RETEM`
sequence under `RETEM` (§8.1.1), so BNN had the resume block in view and
still drew the boot path as one-way.

`[DERIVED]` So `doc-c003` and "who executes the `RETEM`" are **the same
question with the same answer**. Both artifacts live in the V1/V2 window,
neither is in ROM, and both are needed by the same transition. Appendix C
already lists the likely source under "not located in any supplied image":
the **V1/V2 sub-ROM / disk BIOS**. That gives the missing image a concrete
signature — it must contain an `ED FD`, and it must write to `1000:C003`
— and it makes the resume block a *session* bracket rather than a step in
the boot sequence: loaded software hands control back to V3, and the
trap-enable/trap-disable pair around it (§9) brackets exactly that.

`[UNKNOWN]` Two escapes from this reasoning remain open. The `RETEM`
could be built at run time by self-modifying code, in which case no static
search would ever find it; or `BRKEM2` could have an exit that `BRKEM`
does not, which would fold this into `doc-brkem2` (§2.2). Neither is
supported by anything in hand, and neither changes the practical
conclusion: an emulator cannot exercise the resume path from ROM alone.

#### 8.1.1 What the technical manual settles, and what it does not

`[VA-TM]` The same page already cited in the References ([5]) for
`CALLN 91h`, `CALLN 95h` and `RETEM` also documents the `C003` jump.
Its two relevant entries:

- **`CALLN 95h`** calls a user native routine prepared at `1000H:E000H`.
  The manual is explicit that this address corresponds to `E000H` as seen
  from V1/V2 mode, that the caller **prepares the native program in V1/V2
  mode first** and only then uses the function, and that the native
  program returns to μPD780 mode with `IRET`.
- **`RETEM`** returns to V30 mode; on a return from V1/V2 mode to V3 mode,
  after V3-mode initialisation, the machine **jumps to `1000H:C003H`**.

`[DERIVED]` Three things follow.

1. **The resume block is documented behaviour, not vestigial code.** "After
   V3-mode initialisation" is a precise description of what §8 disassembles:
   `RETEM` lands at `0x13B4`/`0x0B18`, the block disables the I/O trap,
   restores the port `0152h` state, sets `SS`, calls the initialiser at
   `0x2210`/`0x18B0`, and only then jumps. The "left over from a design
   change" hypothesis is dead.
2. **`C003` is documented as a destination, never as a preparation duty**
   — and the contrast with `E000` on the same page is the evidence. For
   `95h` the manual spells out that the caller supplies the code; for
   `RETEM` it says only where control goes. Two readings survive, and the
   manual does not choose between them: either `C003` is the mirror image
   of `E000`, a caller-supplied contract the manual simply did not spell
   out twice, or the V1/V2 environment installs a standard stub there when
   it is set up, so that an application never has to think about it. The
   second reading is the one that makes the manual's wording exactly right
   from an application's point of view.
3. **It is not a search problem.** Under either reading, nothing in the
   ROMs prepares `C003`, which is what §8.1's scans found. The remaining
   discriminator is unchanged and unchanged in target: whether the V1/V2
   sub-ROM / disk BIOS contains an `ED FD` and a writer for `1000:C003`.

`[DERIVED]` Note also what the manual's phrasing costs. Documenting the
jump *under `RETEM`* is what makes it natural to read `1000:C003` as a
property of the instruction, which is the error Appendix A item 16
records. Mechanically the instruction pops `PS`, `PC` and `PSW` and does
nothing else.

### 8.2 The machine's mode is a separate, software-set register

`[VA-TM]` §3.3 of the technical manual: the memory map registers are
**ports `152H` and `153H`**, and

```
PORT 153H  bit 6   system memory mode
             0     V1/V2 mode
             1     V3 mode   (reset state)
```

`[ROM]` That is what the otherwise cryptic port-`152h` accesses around the
handoff are doing, and the two are exactly symmetric. Word access to
`152h` puts `153H` bit 6 at bit 14 of `AX`:

```asm
        ; immediately before BRKEM2 — VA2 0x13A0, VA1 0x0B01
13a0: ba 52 01     mov dx,0152h
13a3: ed           in  ax,dx
13a4: 25 ff bf     and ax,0BFFFh      ; bit 14 clear -> V1/V2 mode
13a7: ef           out dx,ax
13a8: b8 00 10     mov ax,1000h       ; DS0 for compatible mode
...
13b1: 0f fe 90     BRKEM2 90h

        ; in the resume block after RETEM — VA2 0x13BD, VA1 0x0B21
13bd: ba 52 01     mov dx,0152h
13c0: ed           in  ax,dx
13c1: 0d 00 40     or  ax,4000h       ; bit 14 set -> V3 mode
13c4: ef           out dx,ax
```

Both machines do it identically, and both use read-modify-write on the
full word so the rest of the memory-map register survives.

`[DERIVED]` **The machine-level mode and the CPU's execution mode are two
different things, set by two different mechanisms.**

| | what it is | how it changes |
|---|---|---|
| CPU execution mode | `MD`, PSW bit 15 (§2.1) | `BRKEM`/`BRKEM2` and `RETEM`/`RETI` |
| system memory mode | port `153H` bit 6 | an ordinary `OUT`, by software |

`[VA-TM]` **The manual's boot flowchart separates them too.** §1.4 draws
`IDP V1/V2` + `I/O trap ON` and `Jmp V1/V2` as two consecutive boxes, and
gives the second the same granularity as `Jmp IPL` on the DOS branch —
an ordinary far jump. Whoever drew it treated the memory-mode and
I/O-trap setup as the firmware's work and `BRKEM2` as the control
transfer alone. That is BNN corroborating the disassembly above,
arrived at independently of it. (The `[UNKNOWN]` expansion of `IDP` is
worth chasing in the てくまに, but the box's position carries the
argument whatever it stands for.)

Three things follow, and they tidy up questions this document had left
tangled.

1. **It kills the leading `BRKEM2` hypothesis.** §2.2's first candidate
   was that `BRKEM2` additionally switches machine-level state where
   `BRKEM` only switches the decoder. It does not: the machine-level
   switch is an `OUT` four instructions earlier, in code that can be read.
   Neither instruction touches it.
2. **It explains CPMVA exactly, and more simply than §3.3 did.** CPMVA
   issues `BRKEM` and never writes `153H`, so the machine stays in **V3**
   memory mode while the CPU decodes Z80. That is precisely why it must
   allocate its own 64 KiB window — there is no V1/V2 memory map to run
   in — and why every device operation goes native through `CALLN`:
   there is no 8801 I/O map either. No appeal to `PS3` is needed.
3. **It reframes `ps3-decode`.** The machine does not have to infer the
   mode from the bus, because software tells it. Whatever IC78 does with
   its `PS3` pin (§12) is therefore something narrower than "switch the
   machine mode" — qualifying bus cycles, or cross-checking the register.
   The pin is still there and still deliberate; its job is now more open,
   not less.

`[DERIVED]` One further constraint on `doc-c003`: the resume block
restores V3 memory mode **before** the far jump, so `1000:C003` is fetched
under the V3 map. Whatever prepares that code has to leave it visible at
physical `0x1C003` in V3 mode, not merely at Z80 `C003h` under the V1/V2
map. If the two maps differ there, "the compatible-mode program writes it"
becomes harder, not easier.

#### 8.2.1 Why the path is documented but not taken

`[DERIVED]` **Commercial V1/V2 software cannot use it, and would not want
to.** A PC-8801 game is 8801 software: `ED FD` is an undefined opcode on a
real Z80, and its authors had no reason to emit one and no knowledge that
the PC-88VA would ever exist. The return path is therefore not a
compatibility feature at all — it is only reachable by software written
*for the VA* that chooses to run in compatible mode. That is a much
smaller population than "V1/V2 software", and it may be empty.

`[DERIVED]` **Operation makes it smaller still.** Returning to V3 means
returning to an environment that needs its own medium; a machine that has
been running a V1/V2 title from that title's own disk cannot simply resume
V3 without a disk change. In practice a V1/V2 session ends at the power
switch or the reset button. (This premise is an operator observation, not
a documented fact — §0 has no tag for that; see the tag note in
Appendix D.3.) It is consistent with the port `000Dh` branch in §8, which
reads as a one-way selection made once at boot rather than as a state the
machine moves in and out of.

`[DERIVED]` So `1000:C003` is **live by the manual and dead in the
field**: documented behaviour, correctly implemented in ROM, and — on the
evidence available — never exercised. That is a different status from
either "vestigial" or "unknown", and it is the right one to record.

`[SRC]` **The pattern is not hypothetical, though. One shipped example is
already in this document: CPMVA (§10).** It is a V3-side program that uses
compatible mode as a subroutine and returns from it — the same shape as
the firmware path, with every fixed part replaced:

| | entry | window | return lands at |
|---|---|---|---|
| CPMVA, shipped 1989 | `BRKEM 0E1h`, its own IVT entry | wherever DOS allocated `emu_seg` | the instruction after `BRKEM` |
| firmware path, documented | `BRKEM2 90h`, vector installed by ROM | `1000:` fixed | `1000:C003` fixed |

`[DERIVED]` The comparison also explains why CPMVA did not simply use the
firmware path: a DOS program needs its own 64 KiB window and its own
vectors, and the firmware path hardcodes both. `[DERIVED]` And it changes
what `doc-c003` is actually asking. The question is not "which image holds
the writer" — §8.1 has shown none of them does — but "was software ever
shipped that takes this route at all". Absence of such software would
explain every observation in this section at once.

`[ROM]` The shared post-handoff initialiser at `0x2210` and its
subroutines (`0x2220` port table, `0x0E84` 51-vector install, `0x2233`
low-memory clear, `0x240B` backup-memory area, `0x2252` BIOS/device
init, conditional on `[2F07h]`) are boot-sequence material; see
`pc88va-boot-sequence.md`.

## 9. I/O trap subsystem

`[VA-WIKI]` A μPD9002/VA compatibility feature, not a PC-98 device.
Ordinary V-series software interrupts come from `INT`/`INTO` or a `DIV`
error; on the VA, `IN` and `OUT` can raise one too.

**The trap exists for V1/V2 mode**, to emulate a subset of the I/O ports
— `50h`–`53h`, `60h`–`68h`, `6Eh`–`6Fh`. V3 mode leaves it disabled, but
software can enable it through the control ports, which is exactly what
the firmware does around the V1/V2 handoff (§8). CoBit's own assessment
is that the facility is weak, existing essentially to patch V1/V2 I/O.

Provenance note: everything in this section traces to CoBit's post to the
VA Club "PC実験室" board (#4016, 31 Mar 1992), reproduced in the *Inside
PC-88VA* wiki. **The register names (`_IOTrap1S` and so on) are CoBit's
own coinage, not NEC's**, and CoBit states plainly that the whole
description comes from ROM analysis and experiment and may contain
errors or omissions. The 98IOE sources in hand are his sample program
for it, so §9 has one author, not two independent ones.

### 9.1 Control ports

```
FFE0h   trap block 1, start port number     word OUT
FFE2h   trap block 1, end   port number     word OUT
FFE4h   trap block 2, start port number     word OUT
FFE6h   trap block 2, end   port number     word OUT
FFEFh   control                             byte OUT
          bit 0   1 = IN traps enabled
          bit 1   1 = OUT traps enabled
          bits 2-3  reserved
          bit 4   0 = byte-port trapping, 1 = word-port trapping
```

`[SRC]` Bit assignments from the `RecIOTrapC` record in `IOTRAP.INC`;
access widths from `_setraport` (word) and `iotrap_on`/`iotrap_off`
(byte). Range registers accept word writes but the high byte is
effectively zero.

Interrupt vectors: `7Ch` for IN traps, `7Dh` for OUT traps.

`[VA-WIKI]` The I/O-trap ports occupy `FFE0h`–`FFE7h` plus `FFEFh`, and
**ports `FFE0h`–`FFFFh` must not themselves be trapped.**

`[ROM]` The VA2 boot ROM programs the ranges at reset, in table A's last
descriptor (§5.7): **block 1 = `0050h`–`005Bh`, block 2 =
`0060h`–`006Fh`**, written as eight byte `OUT`s from `FFE7h` down to
`FFE0h`. Three consequences:

- **Byte access to the range registers works.** CoBit's word `OUT`s
  (`[SRC]`, above) work as well, so both widths are usable. The V50's
  byte-only rule for the system I/O area (§5.1) is evidently a rule about
  V50 registers, not about the 9002's additions.
- **The hardware windows are wider than the emulated port list.** The
  introduction to §9 gives `50h`–`53h`, `60h`–`68h` and `6Eh`–`6Fh`; the
  ROM opens `50h`–`5Bh` and `60h`–`6Fh`. Two blocks is all the hardware
  has, so the narrower list describes the ports the *handler* acts on, not
  the ports that trap. Anything in the wider windows takes the trap
  penalty of §9.4 whether or not the handler does anything with it.
- **`FFEFh` is not written by this table.** The ranges are programmed once
  at reset and the enable is applied later, around the V1/V2 handoff
  (§8) — which is the ordering §9.5 describes, done by the firmware itself.

`[VA-TM]` **The trap windows sit inside "system area 0", the 88MH/FH
compatible block** (§5.1). `[DERIVED]` That bounds what the trap is for:
it patches ports in the 8-bit compatible space and nothing else. Not every
V1/V2-only port is trapped, either — the manual documents `0034h` and
`0035h`, the GVRAM control ports, as **V1/V2 mode only**, and they fall
outside both windows, so the VA implements them in hardware rather than in
the trap handler. `[UNKNOWN]` Where the line between hardware-implemented
and trap-emulated V1/V2 ports actually falls is a question for the
machine-level document, not this one; the two windows read out of the ROM
(§5.7) are the only part of it established here.

### 9.2 Trap semantics

`[SRC]`

- Normal interrupt stack frame — `FLAGS`, `CS`, `IP` — and **`CS:IP`
  points at the I/O instruction itself**, not the following instruction.
  `IOTRAP.ASM` relies on this: `les si,[bp].$IP` then reads the opcode
  from `es:[si]`.
- **The handler must advance the saved `IP` by the instruction length
  before `IRET`**, or the same instruction traps forever. 98IOE decodes
  the opcode itself: one `inc [bp].$IP` for the `DX` form (1 byte), two
  for the `imm8` form (2 bytes).
- Opcode bit layout used for that decode (`RecOp`): bit 0 = word/byte,
  bit 1 = out/in, bit 3 = port-in-`DX` / port-in-`imm8`. Matches the x86
  `E4`–`E7` / `EC`–`EF` encodings.
- **Entered with interrupts disabled.** `IOTRAP.ASM` executes `sti`
  immediately after entry, with a comment that this must be removed if
  I/O inside hardware interrupt handlers is also to be emulated.
- The trapped instruction does **not** perform the access; the handler
  does.
- The handler must disable trapping (`FFEFh ← 0`) before its own I/O and
  re-enable before `IRET`.
- The IN result is returned by patching the saved accumulator in the
  handler's `pusha` image. Flags are not returned.

`[DERIVED]` One incidental consequence of the deleted `INM`/`OUTM`
(§4.2): with no block I/O instructions, every trappable I/O instruction
is a plain `IN`/`OUT` with no memory operand and therefore no segment
override, so "the pushed address is the start of the instruction" is
unambiguous.

### 9.3 Word-port matching

`[SRC]` **Word-port trapping matches on the low byte only.** 98IOE
demonstrates it directly: it programs block 1 as `0037h`–`0038h` with
word-port trapping and successfully traps port `1038h`. Handlers must
therefore re-check the full 16-bit port number — trapping `52h` also
traps `152h`.

### 9.4 Performance

`[VA-WIKI]` Trapped I/O instructions run **tens to hundreds of times
slower**, because the emulation is entirely in software. The worked
example is instructive: word-trapping a range containing port `52h` also
traps port `152h` (§9.3), which the PC-Engine subsystem uses constantly,
and the slowdown becomes obvious to the eye. FDD ports are the same.
A handler must also disable trapping before any `STI`, not just around
its own I/O.

### 9.5 Installation sequence

`[SRC]` From `INIT.ASM`:

```
1.  FFEFh ← 0                       ; disable trapping first
2.  hook INT 7Ch -> trap_entry      ; save the old vectors
    hook INT 7Dh -> trap_entry      ; SAME handler for both
3.  FFE0h ← block-1 start port      ; word OUT
    FFE2h ← block-1 end   port
    FFE4h ← block-2 start port
    FFE6h ← block-2 end   port
4.  FFEFh ← control value           ; byte OUT; enables trapping
```

An unused trap block is parked on a harmless port — 98IOE's
`_setraport` macro defaults an omitted block to `0Fh`–`0Fh`. Removal
restores the two vectors and frees the resident block.

### 9.6 What 98IOE actually is

Worth stating because earlier project notes described it incorrectly.
98IOE is a **PC-9801 I/O emulator demonstration** for the PC-88VA. Its
entire emulated functionality is: `OUT 37h,AL` with `AL=6` produces a
beep (forwarded to VA system port `01CFh`), and `IN AX,1038h` returns
`7777h`. It traps `0037h`–`0038h` and `000Fh`. It is a reference
*implementation* of the mechanism, not an instance of V1/V2 emulation.

## 10. CPMVA — a worked V3-side example

`[SRC]` The complete verified transition sequence of a real program.
Useful as an acceptance trace, and the counterpart to §8: `BRKEM` from
V3-mode software rather than `BRKEM2` from firmware.

```
native (V30 mode, MS-DOS on PC-88VA V3):
  CPMVA.EXE starts, DS = DGROUP
  SETBLOCK to (emu_seg + 1000h) - PSP          ; reserve the 64 KiB window
  read CPM.SYS into a buffer in DGROUP
  save vectors 0E0h and 0E1h (INT 21h AH=35h)
  install vector 0E0h -> cs:nBIOSentry         ; far, native
  install vector 0E1h -> emu_seg:0FA00h        ; the Z80 BIOS entry
  copy the BIOS image to emu_seg:0FA00h
  DS = emu_seg, ES = DGROUP
  brkem 0E1h                                   ; 0F FF E1

compatible (Z80):
  PS = DS0 = emu_seg, PC = 0FA00h
  c$boot:  ld sp,CCPTOP    ; -> BP = 0E400h
           ld a,0
           calln 0E0h      ; ED ED E0
             -> native nBIOSentry: reads AL = 0, prints banner, copies
                CCP+BDOS into emu_seg, patches the warm-boot and BDOS
                entry stubs, returns with iret
           jp CCPTOP       ; enter CCP
  ... every BIOS call is  ld a,<fn> / calln 0E0h / ret ...

exit (EXIT.COM, Z80):
  check system ID 'VA' at 0FFFEh
  retem                    ; ED FD -> native, at the instruction after brkem

native:
  restore vectors 0E0h/0E1h, INT 21h AH=4Ch
```

`nativeBios` (`0E0h`, the `CALLN` target) and `emVector` (`0E1h`, the
`BRKEM` target) are **two different vectors**. Earlier project notes
recorded them as one shared vector; that is wrong and not even
self-consistent, since the same address cannot be both a native routine
and Z80 code.

### 10.1 CP/M emulator — a minimal V30 hard-emulation exerciser

`[SRC]` The MS-DOS CP/M emulator v0.8 distributed as [18a] is a useful transition
test asset because its hard path is narrower than CPMVA's. It uses the
V30 hard 8080 emulation mode only for `.cpv` programs, and only after its
runtime V30 probe succeeds. `.cpm` and `.com` use the software emulator
path instead. This is the artifact meant by the `.cpv` evidence; the
separate Vector Win32 v0.4 package is not interchangeable and does not
contain this `.cpv`/`BRKEM` path.

M76 identity: cpm08.zip SHA-256
691e51dda202ab97b7c8c947ca7c9bf2d93d822f3e315362fcc7840199b8d6f7,
CPM.ASM SHA-256
1172892475ed0852dc00795ef0af117c9ba120bc31d339c4b65e08631c5115f0,
CPM.EXE SHA-256
d9e8ba8e5322bd037186ef11da4f980c4d5999affce3ad4201221ca721947040.
The source was CP932 and was converted to UTF-8 only for the audit; no
CP932 source is added to the repository.

The probe is intentionally small:

```
mov ax,100h
db  0D5h,0      ; AAD 0
jz  soft_emu
```

The program depends on V30 `D5 imm8` behaviour: with `AX=0100h`,
`D5 00` produces `AX=000Ah` and clears the zero result path. If this
probe is wrong, the `.cpv` hard path silently falls back to software
emulation and never executes `BRKEM` at all. For vaeg this makes `D5 00`
a necessary preflight for any CP/M-emulator transition test, but it does
not by itself prove `D4`/`AAM` semantics.

The hard path uses only three transition instructions:

```
BRKEM imm8   0F FF imm8
CALLN imm8   ED ED imm8
RETEM        ED FD
```

The default vectors are `0F1h`, `0F2h` and `0F3h`, but the source makes
them movable through `V$START`. The program installs ordinary IVT entries:
`0F1h` points to the CP/M TPA at `_DATA:0100h`, `0F2h` to the native BDOS
handler, and `0F3h` to the native BIOS handler. `[DERIVED]` This is a
second independent worked example that `CALLN imm8` and `BRKEM imm8`
use normal interrupt-vector table entries. The immediate byte is not a
fixed CPU service number.

The execution shape is:

```
native:
  DS = _DATA
  BP = bdoscall - 2       ; the 8080 SP
  BRKEM 0F1h             ; PS:PC = _DATA:0100h

compatible:
  0005h  JP 0FE00h
  FE00h  CALLN 0F2h
         RET             ; C9

  FF00h  BIOS jump table
  FF77h  CALLN 0F3h
         RET             ; C9

  cold/warm boot:
         RETEM

native:
  resumes after BRKEM
```

Two implementation consequences are stronger here than in CPMVA because
the BDOS/BIOS glue is smaller:

1. **`CALLN` uses the native stack, not the emulated stack.** The native
   BIOS handler reads the compatible return address with an explicit
   `ds:[bp]` load. That value must be the return address pushed by the
   preceding compatible-mode `CALL 0FF77h`. If `CALLN` had pushed a
   `PSW`/`PS`/`PC` frame to `DS0:BP`, that load would see the transition
   frame instead and the BIOS dispatch index would be wrong. Therefore
   `BP`, the 8080 stack pointer, is preserved across `CALLN`; the
   transition frame belongs on native `SS:SP`.
2. **The native handler's return frame is interrupt-shaped.** The same
   far native handler can be shared by the software-emulation path through
   `pushf` plus far call plus `iret`. The hard `CALLN` path must therefore
   present the same `FLAGS`, `CS`, `IP` frame shape to the handler.

`[DERIVED]` These observations agree with 98IOE/IOTRAP's convention of
saving `BP` before treating native `SP` as a handler frame pointer, and
with CPMVA's `CALLN`/`iret` usage. They are transition-boundary evidence,
not a license to merge native and compatible stacks.

The CP/M emulator is deliberately **not** evidence for the VA-compatible
instruction set. Its documentation limits `.cpv` to programs known to use
only 8080 instructions. The VA's compatible mode is Z80-class by Debug
8800 and CPMVA evidence (§2.4, §4.4), so this program is useful for
`BRKEM`/`CALLN`/`RETEM` bring-up and for the V30 `D5 00` probe, but not
for `IX`, `IY`, alternate registers, `JR`, or `ED` block semantics.

Two harness caveats are worth recording before repeated automated use:
`restor_vct` restores the BDOS vector twice and leaves the BIOS vector
unrestored, and an unused `incsp` macro encodes `3Eh` even though 8080
`INC SP` is `33h`.

The hard-path source-level boundary is precise: after opening `.cpv`,
CPM.ASM executes D5 00 with AX=0100h and falls back when the V30 result
is not observed; it installs F1/F2/F3 through DOS vector services, emits
0F FF F1, and its generated compatible image contains ED ED F2, ED ED F3,
and ED FD. This proves that a working V30 can use the 8080 emulation mode
and that these three encodings form a transition contract. It does not prove
the VA's 0F FE BRKEM2, the uPD780/Z80 instruction superset, or uPD9002
specific frame and interrupt details.


---

# Part III — Implementation notes

## 11. What the model must contain

`[DERIVED]` from Parts I and II. This is the minimum for a correct
implementation; it is not a design document.

1. **A mode latch, not a boolean.** `MD` is PSW bit 15 with a
   write-enable rule (§2.1). Keep the decode selector and the latch state
   separable; a single "currently Z80" flag is too weak for correct
   return and interrupt behaviour.
2. **Mode transitions are interrupt-shaped.** `BRKEM`/`BRKEM2`/`CALLN`
   push `PSW`, `PS`, `PC` to the native stack; `RETEM`/`RETI` pop them.
   No separate transition-context object, and no return-context stack of
   its own — nesting is prohibited (§2.3), so there is exactly one level.
3. **Two decode tables, switched wholesale.** The encoding spaces are
   disjoint (one Z80, one x86); there is no shared front end to build.
   `ED FD` and `ED ED` belong to the **compatible-mode** table only; the
   native table needs no `ED` special case.
4. **One register file, plus eight bytes that are not in it.** Compatible
   mode reads and writes the same storage under the aliases in §3.1 — no
   separate Z80 register struct for the main set, and `SP` maps to the
   V30 `BP` slot, not the x86 `SP`. The **alternate set is the exception**
   and needs its own storage, invisible to native mode: hardware confirms
   `AF'/BC'/DE'/HL'` are distinct from the main set (§3.1, §17.2), and
   the V30 file has no room for them. `IX`/`IY` are *not* an exception —
   they alias `SI`/`DI`.
   **Lifetime (M103a, `[DERIVED]` policy):** the storage belongs to the
   machine, not to the Z80 executor: `upd9002_alt_regs` in `cpu/upd9002/`,
   its own `UPD9ALT` state section. It is set to `FFFFh` in all four pairs
   only by the hardware reset path (M103c; M103a cleared it) and survives
   CALLN/IRET round trips, RETEM→BRKEM re-entry and the adapter's own
   reset. The compatible-mode core holds a working
   copy, loaded on entry/resume and written back after every step, so
   native-side code (debugger, state save) always sees the current value.
   Whether the silicon preserves the set across a native round trip is
   what `ALTPRB` measures (§17.6, task M103a; the real VA2 keeps the set
   across `CALLN 91h`). `[DERIVED]` The reset value comes from the §17.2
   VA2 dump: vaeg, running the same ROM path to V1/V2 `MON`, reproduces
   `D'=FF7B` and `H'=0911`, which software writes, while `A'F'` and
   `B'C'` are never written on that path, and the real machine shows them
   as `FF`/`MZ-H-ENC` and `FFFF`. With the reset value `FFFFh` vaeg's `x`
   output matches the dump in every field. One third-party observation of
   unknown boot history: whether this is the power-on state, a reset
   state, or both, and the reset value of `DE'`/`HL'`, are not measured.
5. **One flag byte.** Do not convert at transitions. Make flag
   *computation* mode-aware: bit 2 is `P` natively and `P/V` in
   compatible mode, and bits 1/3/5 carry `N`/`F3`/`F5` (§3.2).
6. **Segment selection by mode.** Compatible-mode code fetch uses `PS`;
   **all** data and stack accesses use `DS0` (§3.3).
7. **Prefetch flush on every mode-changing control transfer** —
   `BRKEM`, `BRKEM2`, `CALLN`, `RETEM`, `RETI`, and interrupt entry from
   compatible mode.
8. **Firmware conventions stay in firmware.** `CALLN 91h`/`95h`, the
   `1000:` window placement, and `1000:C003` are IVT and BIOS
   conventions (§6, §7, §8) — not CPU behaviour. Do not special-case
   them in the instruction core.
9. **Never route any of this to the FDD subsystem uPD780-compatible CPU.** Separate
   device, separate state, no shared code without an explicit later
   decision.
10. **Separate functional correctness from cycle accuracy.** Do not copy
    timing from another emulator.
11. **Model the peripheral relocation registers, not the addresses they
    currently hold.** `FFF0h`–`FFFEh` decode fully (§5.2), but the
    resulting I/O map is firmware state: OPHA/DULA/IULA/TULA place the
    blocks, OPSEL gates them, and only VA2 has been read. Hardcoding
    `0160h`/`0188h`/`01A0h` will work until it does not. Honour the OPSEL
    enables — a disabled peripheral must not answer — and keep this with
    the CPU model rather than in `io/`, since it is CPU-internal state
    and not a VA device. For calibration: nothing in the 512 KiB of
    `varom00_va2.rom` writes those registers at all (§5.5(c)), so runtime
    relocation is a correctness concern, not an observed behaviour.

## 12. What the VA1 schematics show

`[ROM]` From the I/O Magazine Aug 1987 full VA1 schematics (pp. 241–264),
two sheets matter here.

**p.248「CPU割り込み周辺」** carries `IC83 = D9002` — the μPD9002 itself —
together with `IC38 = D8259A`, an **external** interrupt controller
alongside the CPU's built-in one (§5). The sheet exports two distinct
address paths off-page: the **latched** `SA0`–`SA19` (through `LS373`
latches and `ALS245` buffers) and, separately, the **unlatched** high
address/status lines, `XUBE`, the bus-status lines `BS0`–`BS2` and the
raw multiplexed `AD0`–`AD15`.

**p.250「PC-88V1/V2モード・エミュレータ」** carries **`IC78 = PCZ80-27`**,
the 88-mode emulator gate array — a 100-pin device — with
`IC36 = μPD7811` as the keyboard controller.

### 12.1 The `PS3` pin

`[ROM]` A pin-numbered magnification of IC78's pin box is now in hand and
is transcribed in full in Appendix D. The CPU-side pins are:

| Pin | Name | |
|---|---|---|
| **32** | **`PS3`** | **the CPU mode line** |
| 33 | `XUBE` | upper byte enable |
| 34, 35, 36 | `BS2`, `BS1`, `BS0` | bus status |
| 31–16 | `AD15`–`AD0` | multiplexed address/data (pin = 16 + *n*) |
| 39, 38, 37 | `XDMAK3`, `XDMAK2`, `XDMAK0` | DMA acknowledge 3, 2, 0 |
| 63 | `X8BMOD` | reads as "8-bit mode" |

**The gate array has a pin its own designers named `PS3`.** That is a
categorically better fact than the previous reading of this section,
which listed `AB19` and `AB18` among IC78's inputs: **there is no `AB18`
or `AB19` pin on IC78 at all.** The earlier reading took net labels from
p.248 for pin names on p.250 and, in doing so, understated its own case
— an address line admits the innocent explanation that it is there for
decoding above 256 KiB, whereas a pin named `PS3` does not. See
Appendix A item 22.

`[DERIVED]` So the 88-mode emulator watches the **raw, unlatched** CPU bus
including the status phase, and it takes exactly one status line: `PS3`,
which on a V30-class part is high during a compatible-mode bus cycle and
low in native mode (§2.3). `PS0`–`PS2` are not routed to it. There is no
second reason to bring that one line, and only that one line, into the
V1/V2 emulator.

`[DERIVED]` `X8BMOD` (63) looks like the output side of the same
decision — an "8-bit mode" signal for the rest of the machine. Pin
directions are not marked in the scan, so this is a reading of the name,
not of the schematic; §15.2 `ic78-direction`.

This resolves the routing half of the old `ps3-decode` question, which is
now `ps3-routed` in §15.4. What the gate array *does* with the pin is
still inferential and stays in §15.2. Two consequences are recorded
elsewhere: the effect on `BRKEM` vs `BRKEM2` (§2.2) and on how Z80
`IN`/`OUT` reaches the I/O space (§3.3).

### 12.2 Pins bearing on the on-chip peripheral configuration

`[DERIVED]` Three of IC78's pins independently corroborate the §5.2
decode, and the argument is given there as checks 5 and 6 of §5.4:
`XDMAK3` (39) requires OPCN PF ∈ {`00`,`01`}; `RXRDY` (45) and `DCD` (74)
require the external serial controller that OPSEL SS=0 implies; and
`XAITAK` (68) alongside `XPIC` (61) and `PICINT` (51) is the obvious
source of the interrupt acknowledge that PF=`01` costs the CPU.

### 12.3 What a 600 dpi re-read settles

`[ROM]` p.248 and p.250 have since been re-read at scan resolution, with
the net and pin labels recovered by OCR rather than by eye. Four results.

**`REFRQ` is not on either sheet.** A full-page word scan of p.248 returns
355 labels and **not one contains the string `REF`**; the same holds for
p.250. The μPD9002's refresh-request output is not wired to anything on
the CPU sheet or the emulator sheet. `[DERIVED]` Together with the
firmware writing RE=0 (§5.7), that is the whole answer to `refresh-owner`
that a schematic can give: **the CPU does not participate in DRAM refresh
at all** — not through a disabled unit whose pin still feeds decode
logic, but not at all. Whatever refreshes VA DRAM does so without a signal
from the CPU. (A pin left unconnected is often simply not drawn, so this
is "not connected here", which is exactly the question that was asked.)

**`BRATE` is confirmed.** It reads at 92% confidence in p.248's left-edge
connector column, next to `TMROUT`. `[DERIVED]` It does **not** appear
anywhere on p.250, so it does not go to IC78 — consistent with it being
the clock for the RS-232C circuit rather than anything in the V1/V2
emulator. The OCR-grade lead in the previous revision of this section can
now be stated plainly.

**`AB18` leaves IC83 and does not reach IC78.** It reads as a pin name in
IC83's own pin band on p.248, and appears nowhere on p.250. `[DERIVED]`
That closes `ab18-dest` from the other side: whatever consumes it is on a
sheet not in hand, and it is certainly not the 88-mode emulator. This is
the same conclusion Appendix A item 27 reached from IC78's pin list,
reached independently from IC83's.

**Two IC83 pin names corroborate §5.2 directly.** The chip's own pin band
carries **`TOUT1`** and **`DMARQ3`** — not `SRDY`/`INTAK`, and not `TxD`.
`[DERIVED]` NEC drew those pins under the function the machine actually
uses, and both are only available under OPCN PF=`01`: `DMARQ3` requires
PF ∈ {`00`,`01`} and `TOUT1` on the multiplexed pin requires exactly `01`.
That is §5.4 checks 5 and 6 confirmed a second time, from the CPU side
rather than the gate-array side, and it pins PF to `01` where the IC78
evidence alone left `00` open. The sheet also carries `TOUT2` as a
separate pin, as the V50 pinout requires.

### 12.4 Which IC78 signals leave the sheet

`[ROM]` Cross-referencing the two pages by label:

| signal | on p.250 | on p.248 | reading |
|---|---|---|---|
| `XAITAK` | IC78 pin, **and** a sheet-edge connector | in the connector column | crosses to the CPU/μPD8259A sheet |
| `XPIC` | IC78 pin | in the connector column | crosses |
| `PICINT` | IC78 pin, and a second occurrence | in the connector column | crosses |
| `X8BMOD` | IC78 pin only | **absent** | stays on p.250 |
| `BRATE`, `TMROUT` | absent | connector column | never reach IC78 |

`[DERIVED]` The interrupt trio crossing to p.248 is the strongest reading
available without arrowheads: IC83 cannot supply `INTAK` while PF=`01`
(§5.3), p.248 carries the external μPD8259A, and IC78 is the only part
with an `XAITAK` pin. `X8BMOD` staying on p.250 fits its reading as a
local "8-bit mode" decode rather than a signal the rest of the machine
consumes — though a second occurrence below the OCR confidence floor
cannot be excluded, so this one is weaker than the others.

`[UNKNOWN]` **The arrowheads themselves are still unread.** The off-page
connector symbols are legible as glyphs at 600 dpi but not reliably
resolvable into direction at the individual-pin level, so `ic78-direction`
stays open as originally posed. What replaces it in practice is the
topology above, which answers the question the direction was wanted for.

## 13. MAME V20/V30 comparison

MAME already models the standard V30 emulation path and is a useful
implementation reference — but it does **not** implement `BRKEM2` as a
compatible-mode entry, so it is not a drop-in answer.

| Topic | MAME V30 behaviour | Implication here |
|---|---|---|
| Instruction family | Documents `BRKEM`, `RETEM`, `CALLN` for V20/V30/V40/V50. | Supports the analogy; does not prove the `BRKEM2` frame or latch rules. |
| Native entry opcode | Dispatches `0F FF nn` to `BRKEM` and calls a common break-to-emulation helper. | A `BRKEM2` case is needed in addition, not instead. |
| Mode state | Keeps a decode selector (`m_MF`) **and** a separate latch-like state (`m_em`) affecting whether restored flags may leave emulation mode. | Matches the write-enable rule in §2.1. Two fields, not one. |
| Entry stack frame | `nec_brk()` clears the native flag, pushes flags/`PS`/post-immediate `IP`, loads `PS:IP` from `IVT[nn]`. | The working `BRKEM2 90h` model: save `IP = 13B4h`, load `PS:IP` from `vec(90h) = 1000:0000`. |
| Decoder switch | Chooses the native table when the mode flag is set, the 8080 table when clear. | The main CPU needs its own compatible-mode decoder. |
| Return | `RETEM` lives in the 8080-mode table as `ED FD`; `CALLN` as `ED ED nn`. | Confirms the table placement in §11 item 3. |
| Prefetch | Control-transfer macro clears prefetch state on `BRKEM`, `RETEM`, calls, jumps, returns. | §11 item 7. |
| Timing | Assigns a concrete cycle cost to `BRKEM`; source comments treat some prefetch details as approximate. | §11 item 10. |

## 14. Current vaeg status

- The FDD subsystem uPD780-compatible CPU is implemented separately — historically
  `cpucva/z80c.cpp`, currently `cpu/z80_compat_cpu.cpp`, with
  `io/subsystem.cpp` and `VASUBSYS.ROM`.
- The main CPU uPD70008-compatible mode now has a bounded Stage 1 implementation in
  `cpu/upd9002_upd70008.cpp`, using the vendored suzukiplan Z80 core. The stable save-state section remains UPD9Z80 for compatibility. It is
  installed through the uPD9002 core hook boundary and does not share the FDD
  instance or its state.
- `0F FF imm8` (`BRKEM`), `ED ED imm8` (`CALLN`), `ED FD` (`RETEM`), and
  native `IRET` return are implemented with the documented native
  `SS:SP` transition frame. Compatible `SP` maps to native `BP`, `IX`/`IY`
  map to native `SI`/`DI`, and the compatible Z80 state is saved in a
  versioned 68-byte `UPD9Z80` section.
- M103b adds `0F FE imm8` (`BRKEM2`) using the same transition helper as
  `BRKEM`. Both encodings pass the ROM-less round-trip test, including
  the saved native return frame. This is implementation policy, not
  silicon validation of the unknown frame/latch differences in §15.2.
  The `compat-entry` trace event records opcode byte FE/FF in `address`
  and vector in `value`; port `153H` is not changed by either instruction.
- Full VA I/O-trap semantics and silicon-specific interrupt/timing
  behavior remain outside this implementation.
- `BRKEM`/`BRKEM2` are modelled as mode-changing control transfers, not
  ordinary software interrupts.

### 14.1 Stage 1 implementation boundary

The production-path ROMless regression
`vaeg_upd9002_brkem_upd70008` executes both native entry encodings
(`BRKEM` and, since M103b, `BRKEM2`), Z80 `JR`, `IX` and `IY` loads, `CALLN` into native code, native `IRET`, `LD HL`, and `RETEM`. The
transition test uses the same C dispatch path as the main CPU and the same
Z80 wrapper used by the FDD, but owns an independent instance and state
section.

The MAME NEC V20/V30 core is used only as a comparison for the standard
V30 transition frame and opcode placement; its 8080 emulation decoder is
not used as the compatible decoder.

---

# Part IV — Open questions and tests

## 15. Open questions

### 15.1 Resolvable from documents not yet obtained

| id | question | where the answer is |
|---|---|---|
| `doc-9002-man` | A device-level μPD9002 specification: exact saved frame, mode-latch write protection, prefetch, interrupt interaction, cycle timing | a μPD9002 manual, if one exists |
| `doc-brkem2` | What distinguishes `BRKEM2` from `BRKEM`. Two candidates survive: a different saved frame or mode-latch write rule, or nothing architectural at all. The machine-state candidate is dead twice over — the port `153H` `OUT` four instructions earlier, and the manual's own flowchart drawing setup and jump as separate boxes (both §8.2) | μPD9002 manual; the `PS3` routing in §12 and the `[VA-TM]` flowchart have each narrowed it further — see §2.2 |
| `doc-c003` | What prepares the code at `1000:C003`. **Not a ROM-search problem**: the manual documents `C003` as a destination and never as a preparation duty, and §8.1's scans confirm no ROM prepares it. The open half is whether it is caller-supplied like `1000:E000` or installed by the V1/V2 environment — and possibly neither, if no software ever took the route (§8.1) | the V1/V2 sub-ROM / disk BIOS, which should contain an `ED FD`; or a live execution trace; or a single example of VA-aware V1/V2 software that returns to V3. A negative result here is informative and should be recorded as one |

### 15.2 Requires hardware or the schematics

| id | question | default to assume | test |
|---|---|---|---|
| `ps3-decode` | What IC78 *does* with `PS3`. It is no longer "switch the I/O map" — that follows port `153H` bit 6, set by software (§8.2) — so the pin's job is narrower and now genuinely open | qualifying bus cycles, or cross-checking the mode register | hardware, or gate-array-level analysis |
| `brkem2-target` | What mode/state does `0F FE imm8` establish? | trap and halt | §17.4 |
| `bootsel-000d` | What drives port `000Dh` bit 2. The `3000h` shared between the `0x13D2` branch and the manual's IPL path reads it as the V1/V2-versus-V3-IPL selector rather than the `PC` key (§8) | V1/V2-boot versus V3-IPL-boot selector; the `PC` key tested elsewhere | §17.1, Tier 1 — a boot matrix over `PC` key / `SW7` / V3-IPL disk. No instrument |
| `ix-iy-share` | `IX` shares `SI` — confirmed on hardware (§17.2). Open only for `IY`/`DI` | shared | §17.5.2, which now has a predicted value to check |
| `ed-block` | ~~Do `LDIR`/`CPIR`/`INIR`/`OTIR` exist?~~ **Resolved** `[MEAS]`: ZEXDOC/ZEXALL exercise them and pass (R5 flags) | — | §4.7 |
| `cb-dd-fd` | ~~`CB`/`DD`/`FD` prefix spaces; half-index registers~~ **Resolved** `[MEAS]`: half-index Zilog; `DD CB`/`FD CB` register forms diverge (R12/R13); undefined `ED` mapped | — | §4.7 |
| `ir-regs` | Do `I`/`R` exist? What does `LD A,R` return? `LD A,R` was deliberately excluded from the M102 probes | unknown | §17.5.4 |
| `undoc-flags` | ~~Do `F3`/`F5` behave as on a real Z80?~~ **Resolved** `[MEAS]`: they read 0 and cannot be stored (R1) | — | `uPD9002-zex-results.md` |
| `flag-callback` | Are flags set by a `CALLN`-invoked native routine visible after `RETI`? | no (plain PSW restore) | §17.6 |
| `z80-int-model` | Does the superset add `IM 0/1/2`, `I`, `IFF1/2`, `RETN`? | V30 behaviour only (§3.5) | hardware |
| `io-port-map` | How do Z80 `IN`/`OUT` ports reach the V30 I/O space? | direct | hardware |
| `iotrap-exempt` | Are `FFE0h`–`FFFFh` exempt from trapping? | exempt | program a range covering `FFEFh`, observe |
| `opcn-bit4` | What is OPCN bit 4 on the μPD9002? The V50 leaves bits 7–4 undefined; the VA2 boot ROM writes it set, as a literal in its table | unknown; implement as ignored but keep the bit in the stored value | write `11h` to `FFFEh`, read back — §5.5(a). This is now the only remaining discriminator |
| `refresh-owner` | What refreshes VA DRAM? The CPU is now ruled out twice over — unit disabled in firmware, `REFRQ` unwired (§12) | GAL-1 or the display subsystem | the memory and display sheets, pp. 241/249 — not the CPU sheet |
| `icu-alias` | Does the ICU also answer at `018Ch`/`018Eh`, `A2` being neither stored in IULA nor used by the ICU? | aliased | read an ICU register at both addresses — §5.3 |
| `ic78-direction` | Pin directions on IC78. The p.250 arrowheads are still unread at 600 dpi; the inter-sheet topology in §12 answers what the question was wanted for | `XAITAK`/`XPIC` outputs toward p.248, `X8BMOD` local | a cleaner scan, or continuity on hardware |
| `ab18-dest` | Where the unlatched `AB18` lands. Confirmed to leave IC83 and confirmed absent from p.250, so not the emulator (§12) | memory decode | a sheet not in hand |

### 15.3 What a minimal CPMVA bring-up still lacks

`[DERIVED]` CPMVA is the only shipped workload in hand that exercises the
full mode round trip (§17.7), which makes it this document's acceptance
target. It is also by some distance the cheapest such workload to
specify: its compatible-mode code is an 8080 subset plus `JR` and
`CALLN`, and it touches neither the alternate registers, nor the index
registers, nor the `ED` block instructions, nor the I/O trap, nor
`BRKEM2`. What stands between that and a running trace is short — and
most of it is not CPU work.

| id | what is missing | consequence | how it closes |
|---|---|---|---|
| `cpm-sys` | `CPM.SYS`, the CCP + BDOS image `CPMVA.EXE` reads before it issues `BRKEM` (References, "Not in hand") | the unmodified program stops short of §17.7 step 2 — it never reaches the mode switch at all | obtain it, or stub the load |
| `cpm-stub-reach` | a stub does **not** buy the whole trace. `nBIOSentry` copies the CCP and BDOS out of the loaded image and `c$boot` then does `jp CCPTOP`; with a stub, control enters garbage there. §17.7 step 4 is driven by a CCP console call and step 5 by CCP loading and running `EXIT.COM` — both need a real CP/M above the BIOS | a stub reaches steps 1–3 only | a real `CPM.SYS`; or test steps 4 and 5 separately — `CPMBIOS.COM` is in hand and its `c$const: ld a,2 / jr execbios` can be entered directly, and a bare `ED FD` is a one-instruction payload (§17.5). Neither substitutes for the integration, which is the reason to use CPMVA at all |
| `dos-v3` | MS-DOS running in V3 mode: the boot path at `0x13D2` (§8) through to `Jmp IPL`, plus the `INT 21h` services `CPMVA.ASM` uses — `SETBLOCK`, vector get/set (`AH=35h`/`AH=25h`), file open and read, and `AH=4Ch` (§10) | nothing in the trace runs | emulator work. This is the largest of the three and it is why §17.3–§17.6 remain the cheaper first tests |

`[DERIVED]` **What CPMVA does not need, and can therefore be deferred
past it.** Each of these is established elsewhere in the document, and
listing them is what makes the target *minimal* rather than merely
first:

- **The alternate register set, `IX`/`IY`, the `ED` block instructions
  and the `CB`/`DD`/`FD` prefix spaces.** `CPMBIOS.MAC` uses none of them
  (§3.1). So `alt-regs`, `ix-iy-share`, `ed-block`, `cb-dd-fd` and
  `ir-regs` are all off this path — including the two that are now
  answered.
- **`BRKEM2`.** CPMVA enters with `BRKEM` (§2.2.1), so `doc-brkem2` and
  `brkem2-target` do not gate it.
- **The whole of §9.** CPMVA never writes `153H` and never executes a
  Z80 `IN` or `OUT`; every device operation goes native through `CALLN`
  (§3.3, §8.2). The I/O trap and `io-port-map` are both off this path.
- **`1000:C003` and the firmware resume block.** CPMVA returns to the
  instruction after `BRKEM`, not to a fixed address (§8.2.1), so
  `doc-c003` does not gate it either.

`[DERIVED]` **What it does need on the CPU side** is then exactly: the
`MD` latch and the four interrupt-shaped transitions (§2.2); the 8080
subset plus `JR`, `JR cc` and `DJNZ` (§2.4); `SP` mapped to `BP` (§3.4);
`DS0` for data *and* stack (§3.3); the register aliases of §3.1; one
flag byte with mode-aware computation (§3.2); and a prefetch flush on
every mode-changing transfer (§11 item 7). Nothing in that list is open.

### 15.4 Resolved

| id | resolution |
|---|---|
| `retem-mode` | `RETEM` executes in compatible mode — §2.2 |
| `flag-layout` | Low PSW byte is the compatible-mode flag register — §3.2 |
| `emu-int` | Interrupt / `RESET` / `HALT` behaviour — §3.5 |
| `iotrap-init` | Vector and control-port installation order — §9.5 |
| `va-window` | V1/V2 window is at physical `0x10000`; `DS0 = 1000h` — §3.3, §8 |
| `calln-services` | `91h`/`95h` are IVT entries, not CPU behaviour — §6 |
| `retem-c003` | `1000:C003` is reached by an explicit `jmp` after `RETEM` — §8 |
| `v1v2-on-9002` | V1/V2 mode executes on the μPD9002 in compatible mode — §4.4 |
| `c003-not-in-rom` | Neither the code at `1000:C003` nor any `RETEM` exists in `varom1.rom`, `varom1_va2.rom` or `varom00_va2.rom`; the V3 resume path cannot be reached from ROM alone — §8.1 |
| `alt-regs` | **The alternate set exists on the μPD9002** and is distinct from the main set: a real-machine dump shows all four pairs differing at once, which `EX AF,AF'`/`EXX` as no-ops cannot produce. The chip therefore carries eight bytes of register storage native mode cannot address — §3.1, §17.2 |
| `ix-shares-si` | `IX` aliases `SI`: the observed `IX:0F7C` is exactly the `SI` the VA2 boot ROM's vector installer leaves behind — §17.2 |
| `refrq-unwired` | The μPD9002's `REFRQ` is not connected on either the CPU sheet or the emulator sheet — no label containing `REF` exists on p.248 or p.250 — §12 |
| `machine-mode-reg` | The machine's V1/V2-versus-V3 state is **port `153H` bit 6**, set by an ordinary `OUT`, orthogonal to the CPU's `MD` flag. The firmware clears it four instructions before `BRKEM2` and sets it again in the resume block, identically on VA1 and VA2 — §8.2 |
| `halt-diverges` | `HALT` is a real compatible-mode divergence on the μPD9002: NEC removed all three from the VA's Debug 8800 and substituted `JR $`, leaving the rest of the 8 KiB bank byte-identical to a real 8801mkIISR's — §3.5, §4.5 |
| `doc-8801-n88` | The VA's N88-BASIC is the 8801mkIISR's with additions, mostly written into the 8801 ROM's zero-fill; the additions are calendar-clock and serial support, and **not one `HALT` was removed** anywhere in the BASIC — §4.6 |
| `doc-boot-cover` | `0x13B1` is **not** reached in every configuration: port `000Dh` bit 2, tested at `0x136A`, selects between the V1/V2 handoff and a V3 path that never executes `BRKEM2`. The manual's §1.4 flowchart shows the same multi-destination boot, and its IPL path shares the `0x13D2` branch's `3000h` — §8 |
| `va2-dma-cascade` | VA2 does configure the DMAU, just not at reset. Its boot ROM never touches `0160h`–`016Fh` beyond the `DICM` reset, but `varom00_va2.rom` has 43 `mov dx,016xh` sites — `0161`, `0162`, `0164`, `0166`, `0168`/`0169`, `016A`, `016D`, `016E`, `016F` — in three driver-shaped clusters around `0x5675x`, `0x57F3x` and `0x5CE2x`, plus a separate user of `016D`/`016E` near `0x2BAD0`–`0x2C5B0`. The reset-time difference from VA1 (§5.8) is one of scheduling, not of hardware — §5.8 |
| `no-n80-mode` | The VA carries no N-BASIC, so V1/V2 mode does not extend to N80 mode. Absent from every VA image, and confirmed on hardware: `NEW ON 1` answers `Feature not available` on VA, VA2 and VA3 — Appendix C |
| `pf01-from-schematic` | IC83's own pin band names `TOUT1` and `DMARQ3`, which together are available only under OPCN PF=`01`; the schematic therefore fixes PF independently of the firmware table — §12, §5.4 |
| `ps3-routed` | The CPU mode line reaches the 88-mode emulator gate array: `IC78` pin 32 is named `PS3`, and `PS0`–`PS2` are not routed to it — §12, Appendix D |
| `ic78-id` | `IC78` is `PCZ80-27`, a 100-pin gate array, not `PC226B-27` — §12, Appendix D |
| `periph-io-map` | Decoded from the μPD70216 data sheet and then **confirmed by the firmware**, which initialises the ICU, TCU and DMAU at exactly those addresses: DMAU `0160h`–`016Fh`, ICU `0188h`/`018Ah`, TCU `01A0h`–`01A6h`, SCU disabled, timer clock 3.9936 MHz, `TOUT1` exported — §5.2, §5.3, §5.7 |
| `periph-init-source` | The `FFF0h`–`FFFEh` setup is a descriptor table at `0x0F20` in the VA2 boot ROM, walked at `0x12ED` on the reset path; VA1's is at `0x0930`, walked at `0x0A7D` — §5.7, §5.8 |
| `va1-periph` | VA1's configuration differs from VA2's in **two bytes only**, WCY2 (`05` vs `08`) and WCY1 (`84` vs `80`); the peripheral map, pin multiplexing, partitioning, timer clock, refresh setting and trap windows are identical. VA1 additionally brings up DMA channel 1 in cascade mode — §5.8 |
| `iotrap-width` | Both widths reach the `FFE0h`–`FFE7h` trap range registers: the boot ROM writes them as eight byte `OUT`s, 98IOE as four word `OUT`s — §9.1 |
| `iotrap-ranges` | The firmware's trap windows are `50h`–`5Bh` and `60h`–`6Fh`, wider than the emulated port list — §9.1, §5.7 |

## 16. Confidence summary

| Item | Confidence | Reason |
|---|---|---|
| `BRKEM`/`CALLN`/`RETEM` semantics | High | NEC manuals plus a working shipped program for each |
| Compatible mode is Z80 | High | `JR` in the shipped `CPMBIOS.COM`; Debug 8800 runs in V1/V2 |
| Flag sharing | High | Figure 8-2, plus Debug 8800's own flag table |
| Address space model | High | Chapter 8 plus two independent worked instances |
| `BRKEM2` opcode identity | High | ROM installs `vec(90h)` then executes `0F FE 90` at an instruction boundary; VA notes define the encoding |
| `BRKEM2` semantics | Medium | Frame and latch rules are analogy from `BRKEM`, not confirmed |
| Runtime path coverage of `0x13B1` | Medium-high | The `000Dh` branch can skip the block |
| I/O trap mechanics | High | Full working reference implementation with source |
| I/O trap port list | Medium | `[VA-TEKU]`; the ROM's bracketing use is consistent but does not enumerate ports |
| `1000:C003` preparation | Medium | Target confirmed by the manual and by the ROM's own `ljmp`; no copy loop and no `RETEM` in any image, so the preparer is provably outside the ROMs in hand. Which of the two remaining owners it is, is open — §8.1 |
| The resume block is live code | High | The manual describes the post-`RETEM` sequence, including V3 re-initialisation before the jump, and it matches the disassembly step for step — §8, §8.1 |
| The resume path is unexercised in the field | Medium-high | Commercial 88 titles cannot emit `ED FD`, returning to V3 needs a disk change, no example is known, and ten ROM images — the machine's entire ROM complement — contain no `RETEM` at all. Still an absence claim — §8.1, Appendix C |
| Alternate register set exists | High | A real-machine dump in which all four main/alternate pairs differ; no-op `EXX` cannot produce that — §3.1, §17.2 |
| `IX` aliases `SI` | High | The observed value is the exact `SI` a known ROM loop leaves; a chance match is 1 in 65536 and the address is meaningful — §17.2 |
| `IY` aliases `DI` | Low | Unresolved: the observed `IY` does not match `DI` at the point `IX` matched `SI` — §17.2 |
| On-chip peripheral map | High | Every VA2 configuration byte decodes against the μPD70216 data sheet, and the boot ROM then initialises the ICU, TCU and DMAU at exactly the decoded addresses with values meaningful only there (§5.4 check 7, §5.7) |
| Peripheral init sequence | High | Read directly out of the boot ROM: two tables, two walkers, on the reset path — §5.7 |
| VA1 vs VA2 configuration | High | Both boot ROMs parsed with the same walker semantics; the two differing bytes are isolated — §5.8 |
| OPCN bit 4, refresh ownership | Low | One firmware-written bit is undefined on the V50; RE=0 has no documented owner (§5.5) |
| `PS3` reaches IC78 | High | A pin-numbered scan; the transcription is arithmetically self-checking (Appendix D) |
| What IC78 does with `PS3` | Low | A dedicated pin and a plausibly-named output, but the obvious job — switching the machine mode — turns out to belong to a software register (§8.2), so the pin's purpose is now less constrained than before |
| Machine mode is a software register | High | `[VA-TM]` documents port `153H` bit 6, and both boot ROMs write it symmetrically around the handoff — §8.2 |
| OPCN PF = `01` | High | Two independent routes: the firmware table (§5.7) and IC83's own pin names `TOUT1` + `DMARQ3` on p.248 (§12) |
| The CPU does not drive refresh | High | `REFRQ` unwired on both sheets, and the firmware writes RE=0 — §5.5(b), §12 |
| `HALT` diverges in compatible mode | High | NEC patched it out of the VA's own monitor; the diff is 52 bytes in one window against a real 8801mkIISR ROM — §4.5 |
| `HALT` is the *only* CPU-level patch | Medium-high | The whole 8801-mode ROM set has now been diffed and everything else NEC changed is peripheral support. Strong, but an absence over ~57 KiB of ROM — §4.6 |
| VA has no N80 mode | High | Absent from all three VA images, and the machine reports it: `NEW ON 1` → `Feature not available` on VA, VA2 and VA3 — Appendix C |

## 17. Real-hardware test procedures

Two on-board instruments, dividing the work:

- **V3-mode PC-Engine monitor** (`varom00.rom`). Assembler and
  disassembler for NEC native mnemonics including `BRKEM` and
  `BRKEM2`/`BRKFEM`. Use it for the native side: vectors, issuing the
  mode-entry instruction, reading registers after `RETEM`. No Z80
  mnemonics, so compatible-mode payloads entered through it must be
  hexadecimal.
- **Debug 8800** — **this is the monitor `MON` gives you in V1/V2 mode.**
  It prints no banner, so it looks like "the usual monitor"; the
  `Debug 8800 version 1.0` string and the `Peeping Tom` message next to
  it are embedded text aimed at whoever dumps the ROM, not startup
  output. Four independent checks identify it (§17.2). A Z80 monitor with an
  assembler (`a`), disassembler (`l`), memory dump/fill/move/edit, `g`
  with two breakpoints, port `i`/`o`, and an `x` command covering the
  full Z80 register set **including `IX`, `IY` and the alternates**.
  Where a test is about Z80 semantics rather than mode transition, this
  is the better instrument.

### 17.1 Where to start

Ordered by cost, not by section number. The sections below are reference
material; this is the running order.

**Already answered — do not spend machine time on these.**

| id | why it is closed |
|---|---|
| `ed-block` | Debug 8800's own bank entry shim executes `LDIR` before the monitor runs (§4.4). Reaching the `h]` prompt is the proof |
| `cb-dd-fd`, `DD`/`FD` half | `x` displays and edits `IX`/`IY`, which needs both prefix spaces (§17.2) |
| `alt-regs` | The boot dump shows all four main/alternate pairs differing (§3.1, §17.2) |
| `ix-iy-share`, `IX` half | Same dump: `IX:0F7C` is the boot ROM's leftover `SI` (§17.2) |

**Tier 1 — from the `h]` prompt or the boot switches; seconds, no risk.**

1. **`I` round-trip.** Write `I:3C` with `x`, re-issue `x`. Closes the
   first half of `ir-regs` at essentially zero cost, since you are already
   at the prompt.
2. **Cold-boot from a different disk and re-take `x`.** If `IX` comes up
   `0F7C` again, the V3-residue reading in §17.2 is confirmed against a
   second boot; if it comes up different, the residue has another source
   and that reading needs revisiting.
3. **The boot matrix — `bootsel-000d`.** No instrument and no typing:
   vary the `PC` key, `SW7` and whether the disk carries a V3 IPL, and
   record which of the three destinations in §8 the machine reaches. One row
   in which the `PC` key is on and the machine still lands in N88-BASIC
   rules the `PC` key out as the driver of port `000Dh` bit 2; one in
   which `SW7` and the disk alone move it between N88-BASIC and DOS
   confirms the reading in §8. Reading `000Dh` back is the
   *confirmation*, not the discriminator — and reading it from V1/V2 mode
   is entangled with `io-port-map`, so take the observation first.

**Tier 2 — a few bytes typed in, minutes.**

4. **The 16-bit port probe. The highest-value single test here.**

   ```
   01 FE FF     LD BC,0FFFEh     ; OPCN, already holding 11h (§5.7)
   ED 78        IN A,(C)
   ```

   Break, then read `A` with `x`. All three outcomes are informative:

   | `A` | what it means |
   |---|---|
   | `11` | 16-bit port numbers reach the V3 I/O space unchanged **and** OPCN bit 4 is real storage → `opcn-bit4` closes as a μPD9002 extension, `io-port-map` closes as a direct mapping |
   | `01` | Ports reach, bit 4 is not retained → `opcn-bit4` closes as set-and-ignored |
   | `FF` | Compatible-mode I/O does not reach the system I/O area → `io-port-map` closes the other way, and that is itself a reason for the I/O trap to exist (§9) |

   Read-only, no side effects — the firmware has already written the value
   being read. **Terminate with a breakpoint under `g`, or with `18 FE`
   (`JR $`) — never with `76` (`HALT`).** That is not caution: NEC removed
   every `HALT` from the VA's own copy of this monitor and put `JR $` in
   their place (§4.5), because standby's exit path can leave compatible
   mode altogether (§3.5).

5. **If 4 says the ports reach, read the rest.** All eleven of
   `FFF0h`–`FFFEh` against the §5.2 table; `0188h` and `018Ch` compared
   for `icu-alias` (§5.3); `01A2h` twice for whether the TCU is actually
   counting.
6. **`undoc-flags` (§17.5.5).** The one thing `x` can never show, because
   the flag notation table hardcodes `--` at bits 5 and 3 (§17.2).
   `3E 28 / C6 00 / F5 / E1`, then read `L`: `28h` means the μPD9002
   reproduces a real Z80's `F3`/`F5`; `02h` or `00h` means it does not.
7. **`CB` prefix (§17.5.3).** `3E 01 / CB 27` → `A = 2` if `SLA A` works.
8. **`ir-regs`, second half (§17.5.4).** `ED 5F` twice with differing gaps.
   `[DERIVED]` There is a prediction attached: §5.7 shows the on-chip
   refresh unit is deliberately disabled, so if `R` exists at all it
   cannot be refresh-driven and must be a counter imitation. A divergence
   from real-Z80 `R` behaviour is expected, and that is the reason.

**Tier 3 — needs entry from the V3 side by `BRKEM`; set-up required.**

`IY`/`DI` (§17.5.2), `flag-callback` (§17.6), `brkem2-target` (§17.4). Per
§8.1 a plain V1/V2 boot cannot return to V3, so these have to be entered
from the PC-Engine monitor. Worth doing, but not first.

**Tier 4 — no machine needed at all.**

`refresh-owner` (trace `REFRQ` on IC83), `ic78-direction` (read the p.250
arrowheads at higher magnification), `ab18-dest`. These are schematic
work, so they fill the time when the machine is not available.

### 17.2 Confirming that the V1/V2 `MON` monitor is Debug 8800

`[ROM]` `[DERIVED]` The identification rests on four things:

1. The bank at `varom00.rom:0x1E000` opens with the ASCII signature
   `"DB"` and maps to Z80 `6000h`–`7FFFh` — the PC-8801 N88
   extended-ROM bank window — and its entry shim saves and restores
   port `70h`, the bank-select register (§4.4). That is precisely a ROM
   bank entered from N88-BASIC.
2. The memory-test messages (`Testing TEXT RAM.`,
   `Testing Graphics RAM (Blue).`, …) live inside that bank at
   `0x1F3E7`. If you have seen those from the V1/V2 monitor, you have
   been running this code.
3. The command help at `0x1F4BB` is in the same bank.
4. The dispatch table at `0x1E0F5` gives exactly sixteen commands:
   `A L T R W V B M E I O G F X D S`.

**Reference output.** `[SRC]` The `x` command prints, on a real PC-8801
and on a real PC-88VA respectively, after booting N88-DISK-BASIC and
typing `mon`:

```
8801  A :00 F :PZ---E-- B :0000 D :EDCC H :0001 A':FF F':MZ-H-ENC B':FFFF D':FF7B
      H':0911 IX:FFFF IY:FFFF I :F3 PC:0000 SP:E5F9

VA    A :00 F :PZ---E-- B :0000 D :EDCC H :0001 A':FF F':MZ-H-ENC B':FFFF D':FF7B
      H':0911 IX:0F7C IY:0101 I :F3 PC:0000 SP:E5F9
```

`[DERIVED]` **The two differ in `IX` and `IY` and in nothing else.** Every
other field — including all five alternate-register fields and `I` —
matches to the character, which is what one expects of a deterministic
boot path through the same monitor, and which makes the two that differ
worth reading closely.

**`IX:0F7C` is the V3 firmware's leftover `SI`.** `[ROM]` The VA2 boot
ROM's vector installer walks the override table with `SI`:
`mov si,0F5Eh`, `mov cx,6`, and each iteration advances `SI` by 1+2+2 = 5
bytes (§6). Six iterations leave `SI = 0F5E + 30 = 0F7Ch` — the observed
value exactly. `[DERIVED]` So `IX` aliases `SI`, `SI` survived from the
installer through to the `BRKEM2` handoff, and the compatible-mode code
that ran afterwards never touched `IX`. Three independent things follow:
`ix-iy-share` is answered for `IX`; the machine dumped was running the
**VA2** boot ROM, since VA1's table ends at `0x099B`; and `CPMVA.H`'s
`_IX equ si` (§3.1), which CPMVA never actually exercised, was right.

`[UNKNOWN]` `IY:0101` does not match `DI` at the installer's exit, which
is `0060h` — the fill loop leaves it there (`mov di,20h`, eight entries,
then eight more). Plenty of code runs between the installer and the
handoff, so `DI` may simply have moved on where `SI` did not; or `IY` may
not alias `DI`. The §17.5.2 payload still settles it, and it now has a
predicted value to check against rather than an open question.

`[DERIVED]` **The alternate set exists**, and this dump alone proves it —
see §3.1. `A'`, `B'`, `D'` and `H'` all differ from their main-set
counterparts, which cannot happen if `EX AF,AF'` and `EXX` are no-ops.
Note that the VA's alternate values are *identical to the 8801's*, so on
this path the μPD9002's alternate set holds the same content a real Z80's
does.

This confirms the ROM tables read out of `varom00.rom` (§4.4) exactly:
main `A F B D H`, alternates `A' F' B' D' H'`, then `IX`, `IY`, `I`,
`PC`, `SP`. The flag field decodes against the `PM-Z---H--OE-N-C` table
as eight two-character pairs where **the first character means clear and
the second means set**:

```
pair   PM   -Z   --   -H   --   OE   -N   -C
bit     7    6    5    4    3    2    1    0
flag    S    Z   (F5)  H  (F3)  P/V   N    C
```

so `PZ---E--` is `S=0, Z=1, H=0, P/V=1(even), N=0, C=0` and
`MZ-H-ENC` is all of `S Z H P/V N C` set.

`[DERIVED]` **A limitation worth knowing before planning tests:** bits 5
and 3 are hardcoded as `--` in the table, so `x` can *never* display
`F3`/`F5` whatever their real values. The `undoc-flags` question cannot
be answered with `x`; it needs the `PUSH AF` / `POP HL` route in §17.5.5.

At the prompt, three quick discriminators:

- `x` — should display `IX`, `IY` and the primed registers
  `A' F' B' D' H'` alongside the main set.
- `b` / `bh` — radix select, **hexadecimal or octal**. The octal option
  is a 1981 giveaway.
- `t` — memory test, printing the strings above.

If those match, no further setup is needed: V1/V2-mode code is already
compatible-mode code, so **no `BRKEM` or `BRKEM2` is required** to run
the tests below. You are inside the mode under test.

### 17.3 Z80 feature coverage — `alt-regs`, `ed-block`, `cb-dd-fd`

Debug 8800 does most of this for free:

1. Enter Debug 8800 in V1/V2 mode and issue `x`. Worth capturing the
   fresh-boot output verbatim before touching anything — §17.2 has a
   real PC-8801's for comparison, and a VA's costs nothing to record.

   **Display alone is not the test.** A fresh machine shows `FFFF`/`FF`
   in the alternates and index registers either way, and those values are
   indistinguishable from "no storage, reads float high". The test has two
   halves, and the second is the one that is easy to leave out:

   **(a) Round-trip.** Use `x` to write values that cannot be confused
   with a floating read — `A'=5A`, `B'=1234`, `D'=5678`, `H'=9ABC`,
   `IX=A55A`, `IY=C33C`, `I=3C`; avoid `FF` and `00` patterns — then
   re-issue `x` and check they read back. Round-tripping proves storage
   exists, since the monitor cannot reach the alternate set except by
   executing `EX AF,AF'` and `EXX`, nor `IX`/`IY` without the `DD`/`FD`
   prefixes. That answers `alt-regs` and the `DD`/`FD` part of
   `cb-dd-fd`, and — for `I` — the first half of `ir-regs`.

   **(b) Check the main set survives it.** Set `A=11` and `A'=22`, then
   re-read *both*. `A=11, A'=22` means separate storage. `A=22` means
   `EX AF,AF'` is a no-op and the monitor's "alternate" write went
   straight to the main register — a machine with no alternate set at all
   would pass (a) and fail (b), because the value would round-trip
   through the register it was aliased onto. Repeat for `BC`/`BC'`.
   Without (b) the round-trip is not conclusive.

   `[SRC]` **In the event, a plain boot dump did (b) for free** and
   `alt-regs` is answered: on the real VA the main and alternate sets
   already differ in all four pairs before anything is written (§17.2).
   The write test remains the way to confirm it deliberately, but it is no
   longer the thing standing between this document and an answer.

   `[DERIVED]` For `ix-iy-share`, **do not use this route.** Earlier
   revisions said to write `IX` here, leave V1/V2 mode, and read `SI`
   from the V3-mode monitor — but §8.1 established that a plain V1/V2
   boot cannot be left except by reset, which clobbers what the test
   wrote. Any test that has to come back must be entered from the V3
   side instead; see the note at the head of §17.5.
2. `l` a region known to contain `ED B0` — Z80 `6E5Fh` in the Debug 8800
   bank — and single-step it with `g`. That answers `ed-block`.
3. Assemble `SLA A` with `a` and step it. That answers the `CB` part.

### 17.4 `0F FE` target — `brkem2-target`

Do **not** use `LDIR` as the discriminator; a negative result would be
ambiguous. Use a Z80-only, single-opcode instruction outside the `ED`
space. On an 8080, `10h`, `18h`, `20h`, `28h`, `30h`, `38h` are all
undocumented NOPs:

```
DJNZ:   06 02        LD B,2
        10 FE        DJNZ $          ; Z80: loops once; 8080: NOP NOP
JR:     18 01        JR +1           ; Z80: skips the next byte
        3E FF        (skipped)       ; 8080: falls through, A = FFh
        76           HALT
```

Set up a `0F FE` vector pointing at the payload, execute
`0F FE <vec>`, inspect. Repeat with `0F FF` as a control.

Then the more interesting half: check whether `BRKEM2` also changed
machine state that `BRKEM` does not — read a V1/V2-only port before and
after each entry, and watch `PS3` if a probe is available.

### 17.5 Z80 feature payloads

`[DERIVED]` **Which side to enter from.** A payload that reports its
answer in a compatible-mode register, and is read back with Debug 8800's
`x`, can be run from a plain V1/V2 boot. A payload whose answer has to be
read *natively* — §17.5.2 is the only one — cannot: §8.1 established there
is no way back to V3 from a V1/V2 boot short of a reset, which destroys
the result. Enter those from the **V3 side**: the PC-Engine monitor
assembles `BRKEM`, so install a vector, `BRKEM` into the payload, end it
with `RETEM`, and read the native register when control returns. That is
the CPMVA shape (§8.1), and it is the only round trip actually available
on the machine.

`[ROM]` **Do not terminate a payload with `76` (`HALT`).** NEC removed
every `HALT` from the VA's own copy of Debug 8800 and replaced them with
`JR $` (§4.5); the standby exit path can leave compatible mode entirely
(§3.5). Use a breakpoint under `g`, or `18 FE` as shown.

```
17.5.1  alt-regs      3E AA / 08 / 3E 55 / 08 / 18 FE        ; A back to AAh?
                      01 34 12 / D9 / 01 78 56 / D9 / 18 FE  ; BC back to 1234h?
17.5.2  ix-iy-share   DD 21 CD AB / ED FD    ; enter by BRKEM from V3,
                      FD 21 .. .. / ED FD    ; then read SI / DI natively
17.5.3  cb-dd-fd      3E 01 / CB 27 / 18 FE                  ; A = 2 if SLA works
17.5.4  ir-regs       ED 5F / 18 FE      ; LD A,R, twice with differing gaps
                      ED 47 / ED 57      ; LD I,A / LD A,I round-trip
17.5.5  undoc-flags   3E 28 / C6 00 / F5 / E1 / 18 FE        ; F -> L, check bits 3,5
```

Test `CB` before `DD`/`FD`, since the index prefixes act on `CB` too.
`[V30-MAN]` For §17.5.2, note that `IX`/`IY` are documented as unreachable
from emulation mode on the plain V20/V30 (§3.1), so a negative result
there is a statement about the μPD9002's divergence, not a bug.

### 17.6 Flag propagation across `CALLN`/`RETI` — `flag-callback`

The most valuable single test: it settles the flag model and explains or
refutes the CPMVA anomaly in §3.2.

```
native routine at vector n:
        F9           STC
        CF           RETI

compatible-mode caller:
        A7           AND A            ; clears CY
        ED ED nn     CALLN n
        38 xx        JR C,<set>       ; did the carry survive?
```

Report the answer into a register, `RETEM`, read it natively.

### 17.7 Mode round-trip — CPMVA as the acceptance workload

`[DERIVED]` The firmware's own round trip cannot be used as a bring-up
milestone: §8.1 shows the resume path is unreachable from ROM, so an
emulator that never lands at `0x13B4`/`0x0B18` is behaving correctly. The
round trip still has to be tested, and **CPMVA is the only shipped
workload in hand that exercises it end to end** — `BRKEM` in, `CALLN` out
and back repeatedly, `RETEM` home.

Use §10 as the acceptance trace. It is ordered so that failures localise:

1. `SETBLOCK`, vector install, BIOS image copied to `emu_seg:0FA00h`,
   `DS = emu_seg` — all native, no mode change yet.
2. `brkem 0E1h` → `PS = DS0 = emu_seg`, `PC = 0FA00h`, and the first
   compatible-mode instruction is `ld sp,CCPTOP` landing in **`BP`**, not
   `SP` (§3.4). Getting this backwards is the "boots, then desyncs"
   failure — check it here, not later.
3. `calln 0E0h` → native `nBIOSentry`, which reads the function number as
   `AL`, and returns with `iret`.
4. `JR` executes correctly on the BIOS dispatch path (§2.4) — the cheapest
   proof the decoder is Z80 and not 8080.
5. `EXIT.COM`'s bare `retem` returns to the instruction after `brkem`.

`[UNKNOWN]` The gate: `CPM.SYS` is not in hand (References, "Not in
hand"), and `CPMVA.EXE` reads it before issuing `BRKEM`, so the
unmodified program stops short of step 2. Either obtain `CPM.SYS`, or
stub the load — but **a stub reaches steps 1–3 only.** Steps 4 and 5 run
under the CCP, which comes out of the loaded image: step 4 needs a
console call to reach the BIOS dispatch path, and step 5 needs the CCP to
load and run `EXIT.COM`. An earlier revision of this paragraph claimed
all of steps 2–5 were independent of the image's contents; that was
wrong, and §15.3 records what a stub does and does not buy. Note also
that this test needs MS-DOS running in V3 mode, so it is downstream of a
good deal of other work; §17.3 through §17.6 remain the cheaper first
tests.

---

# Appendices

## Appendix A. Corrections to earlier project documents

Recorded so the errors are not reintroduced.

1. **`RETEM` executes in native mode; `ED FD` must be added to the native
   decode table.** Wrong on both counts. → §2.2, §11 item 3.
2. **A `CALLN`-invoked native routine returns via `RETEM`.** It returns
   via `RETI`/`IRET`. → §2.2.
3. **Flags must be converted at each transition.** The low PSW byte *is*
   the compatible-mode flag register. → §3.2.
4. **The MD flag.** Absent from earlier documents entirely. → §2.1.
5. **The `ED` prefix proves the mode is Z80.** Invalid — those are the
   encodings on the plain V20/V30 too. The real proof is `JR` in the
   shipped CPMVA binary. → §2.4.
6. **Vector `0E0h` serves as both the `BRKEM` and `CALLN` entry.** They
   are `0E1h` and `0E0h`. → §10.
7. **The four deleted instructions are all block I/O.** `INS` and `EXT`
   are bit field insert/extract. → §4.2.
8. **`INM`/`OUTM` are absent from the ROM string pool.** They are
   present, in both notations. → §4.2.
9. **The mnemonic for `0F FE` is `BRKFEM`.** That is the VA2 ROM's name;
   the VA ROM calls it `BRKEM2`. → §2.2.
10. **The I/O trap emulates V1/V2 ports, as 98IOE demonstrates.** 98IOE
    emulates PC-9801 ports; the V1/V2 port list is from the VA notes and
    the ROM's bracketing use, not from 98IOE. → §9, §9.6.
11. **The address space model was missing.** Data *and stack* use `DS0`;
    code uses `PS`. → §3.3.
12. **Interrupt behaviour in compatible mode was listed as unknown.** The
    V30-documented behaviour is a documented default. → §3.5.
13. **"The VA ROM contains no Z80 mnemonics."** True of the V3-mode
    monitor only; `varom00.rom` also carries Debug 8800. → §4.4.
14. **`ED ED` occurrences in the ROM images are candidate `CALLN`
    sites.** All are false positives — `LD DE,0EDC9h` + `LDIR` at
    `varom00.rom:0x1EE67`, `CALL 0EDEDh` at `varom1.rom:0x1831C` and
    `0x1AB90`, and a doubled-byte data table in an x86 region. The real
    `CALLN` services are IVT entries. → §6.
15. **`CALLN 91h`/`95h` are fixed CPU services requiring dispatch on the
    immediate.** They are ordinary IVT entries; `CALLN` is uniformly
    "call `vec(imm8)`". → §6, §11 item 8.
16. **`RETEM` has a context-dependent return target of `1000:C003`.** It
    has one return target; `1000:C003` is reached by an explicit `jmp`
    in the native resume block. The error has a respectable source: the
    technical manual documents the `C003` jump in its `RETEM` entry, so
    anyone reading the manual alone will reach it. Keep the distinction
    between what the machine does and what the instruction does. →
    §2.2, §8, §8.1.
17. **`BRKEM` does not exist on the μPD9002.** It does — CPMVA uses it.
    The VA *firmware* uses `BRKEM2` instead. → §2.2.
18. **"Neither ROM contains Z80 mnemonics."** Wrong as stated. The
    earlier scan searched plain ASCII and a mnemonic list that omitted
    the relevant entries. Debug 8800's mnemonic pool is
    high-bit-terminated and carries **both** Zilog and Intel 8080
    notations. Only the V3-mode monitor is NEC-native-only. → §4.3, §4.4.
19. **The I/O-trap port list rests on the てくまに.** It does not — it is
    CoBit's, from his 1992 post, along with the rest of §9. That also
    means §9 has a single author and his own error caveat, rather than
    two independent sources. → §9.
20. **Debug 8800 must be reached somehow.** It is the monitor `MON`
    already gives you in V1/V2 mode; it simply prints no banner.
    → §17.2.
21. **`IC78 = PC226B-27`.** The part is **`PCZ80-27`**. → §12,
    Appendix D.
22. **IC78's input list includes `AB19` and `AB18`.** It includes
    neither; IC78 has no such pins. Pin 32 is named **`PS3`** and
    `PS0`–`PS2` are not routed to the part at all. The earlier reading
    took net labels from p.248 for pin names on p.250, and in doing so
    understated its own conclusion — it then hedged that "the scan
    resolution does not permit reading the individual pin numbers",
    which is no longer true either. → §12, Appendix D.
23. **The meaning of the `FFF0h`–`FFFEh` bytes is unknown.** Not an
    error, but no longer true: all eleven decode against the μPD70216
    data sheet. → §5.1–§5.6.
24. **`FFF2h` and `FFF0h` carry no evidence of an explicit write.** True
    of the reset values in isolation, false of the machine: the boot
    ROM's table writes both. → §5.2, §5.7.
25. **"The relocation registers should be written first, since OPSEL
    enables the peripherals."** A plausible-sounding recommendation in an
    earlier draft of §5.5(c), and the opposite of what the firmware does
    — OPSEL is written four `OUT`s *before* OPHA/DULA/IULA/TULA. →
    §5.7.
26. **"The VA1 and VA2 boot ROMs are structurally aligned / the same
    build."** Written one revision ago on the strength of
    `varom1_va2.rom` matching the offsets this document attributed to
    VA1. They are **different builds**: 48.8% of their bytes differ and
    every offset has moved. → §5.8, Appendix C.
27. **The `VAROM1.ROM` offsets in §6 and §8 are VA1's.** They are
    `varom1_va2.rom`'s. In the VA1 image the `BRKEM2` handoff is at
    `0x0B15`, not `0x13B1`; the configuration table is at `0x0930`, not
    `0x0F20`; the vector override table is at `0x097D`, not `0x0F5E`. The
    error was inherited from earlier project notes that did not
    distinguish the two boot ROMs, and it is harmless to the arguments —
    the same code is present in both — but it makes every offset in §6
    and §8 wrong for the image whose name they carry. → §6, §8,
    Appendix C.
28. **"Stubbing `CPM.SYS` buys steps 2–5 of the CPMVA trace."** It buys
    steps 1–3. Steps 4 and 5 are driven by the CCP, which is loaded from
    the image being stubbed: step 4 by a console call reaching the BIOS
    dispatch path, step 5 by the CCP loading `EXIT.COM`. The two can be
    exercised separately from hand-entered payloads, but not as part of
    the workload. → §15.3, §17.7.

## Appendix B. NEC / Intel mnemonic correspondence

`[ROM]` The V3-mode monitor carries two parallel mnemonic tables.
Extracted in table order.

```
NEC        Intel            NEC        Intel           NEC        Intel
ADDC       ADC              MOVBK      MOVS            BR         JMP
SUBC       SBB              CMPBK      CMPS            BRK        INT
MUL        IMUL             LDM        LODS            BRKV       INTO
MULU       MUL              STM        STOS            RETI       IRET
DIV        IDIV             CMPM       SCAS            DBNZ       LOOP
DIVU       DIV              INM        INS             DBNZE      LOOPE
ADJBA      AAA              OUTM       OUTS            DBNZNE     LOOPNE
ADJBS      AAS              PREPARE    ENTER           BCWZ       JCXZ
ADJ4A      DAA              DISPOSE    LEAVE           POLL       WAIT
ADJ4S      DAS              LDEA       LEA             BUSLOCK    LOCK
CVTBW      CBW              TRANS      XLAT            HALT       HLT
CVTWL      CWD              XCH        XCHG            NOP        NOP
CVTBD      AAM              SHRA       SAR             CHKIND     BOUND
CVTDB      AAD              ROLC       RCL             DS0/DS1    DS/ES
                            RORC       RCR             PS         CS
```

Registers: `AW BW CW DW` = `AX BX CX DX`; `IX IY` = `SI DI`;
`PS DS0 SS DS1` = `CS DS SS ES`; `PC` = `IP`; `PSW` = `FLAGS`.
Flags: `CY V P S Z AC DIR IE BRK MD` = `CF OF PF SF ZF AF DF IF TF` plus
the mode flag.

## Appendix C. ROM image map

`[ROM]` From 8 KiB block opcode-profile classification, banner strings
and internal address cross-checks. **Nothing is compressed**; every image
found so far is stored plain.

### C.1 `varom1.rom` — 128 KiB

The **PC-88VA1** boot ROM. A different build from `varom1_va2.rom`, not a
variant of it: 48.8% of the bytes differ and every offset has moved. Its
own landmarks:

| Offset | Content |
|---|---|
| `0x00000` | `E9` near jump to `0x0A6F`, the reset entry |
| `0x00930` | peripheral configuration table A — walker at `0x0A7D` (§5.7, §5.8) |
| `0x00953` | peripheral configuration table B, **fourteen** records — walker at `0x0A97` |
| `0x0097D` | IVT override table, six entries, consumed at `0x0B76` (§6) |
| `0x00B15` | `0F FE 90` — the `BRKEM2` handoff; `cli` at `0x0B18`, `ljmp 1000h:c003h` at `0x0B31` (§8) |
| `0x0FFF0` | `EA 00 00 00 F0` — `jmp F000:0000` |

| Offset | Size | Content |
|---|---|---|
| `0x00000`–`0x0FFFF` | 64 KiB | x86 (V3-side firmware). Opens with an `E9 disp16` near-jump table. Contains the vector installer and the `BRKEM2` handoff — but **not** at the offsets §6 and §8 quote, which are the VA2 image's (Appendix A item 27). |
| `0x10000`–`0x17FFF` | 32 KiB | **Z80. NEC N-88 BASIC Version 1.9**, banner at `0x179C7`, "Copyright (C) 1981 by Microsoft". Maps to Z80 `0000h`–`7FFFh`. |
| `0x18000`–`0x1FFFF` | 32 KiB | **Z80. NEC N-88 BASIC Version 2.4**, banner at `0x1CF97`. BASIC error strings at `0x1C000`; `C3 nnnn` entry-vector table at `0x1E000`. |

Verification: `varom1.rom:0x10000` reproduces a `0000h`–`00FFh` dump
taken from a real PC-8801 running N88-BASIC 1.93 byte for byte —
`F3 31 A0 E1` (`DI` / `LD SP,0E1A0h`), `C3 E5 3B` at `0004h`,
`C3 69 E6` at `0038h` (the `IM 1` vector).

### C.2 `varom00.rom` — 512 KiB

Predominantly x86 V3 firmware. Exactly one Z80 region:

| Offset | Size | Content |
|---|---|---|
| `0x1E000`–`0x1FFFF` | 8 KiB | **Z80. Debug 8800 v1.0** (§4.4). Maps to Z80 `6000h`–`7FFFh`, the PC-8801 N88 extended-ROM bank window. |

The V3-mode PC-Engine monitor, with its NEC-mnemonic assembler and
disassembler tables, is at `0x5CC80`–`0x5D1A0`.

### C.3 `varom00_va2.rom` — 512 KiB

The PC-88VA2 counterpart. Same structure; the `0F` extension table is at
`0x66AAF` and names `0F FE` as `BRKFEM`. Code starts at offset 0 with an
`E9` near jump, and there is a second `E9`/`EB` jump table at `0x70000`.
No 8 KiB block in the image is blank.

`[ROM]` **It carries Debug 8800 too, at byte-identical offsets to
`varom00.rom`**: bank signature `DB` at `0x1E000`, command dispatch table
at `0x1E0F5`, flag notation table at `0x1E3C8`, banner at `0x1F058`,
`Peeping Tom` at `0x1F0D3`, memory-test strings at `0x1F3E7`. So the
§17.2 identification and the §17.3 tests apply unchanged on a VA2 — the
`MON` monitor is the same one. Block profiling puts Z80-dense regions at
`0x10000`, `0x1E000` and `0x6E000`; only `0x1E000` has been identified.

`[ROM]` Two negative results from a full scan, recorded so the work is
not repeated:

- It contains **no write to the system I/O area** `FFF0h`–`FFFEh`, by any
  addressing route — the method and the five checks are in §5.5(c).
- Its last 32 bytes are `FFh`, so it does not carry the reset entry at
  `FFFF0h`.

**This is not the boot ROM.** The `varom1`-class image is.

### C.4 `varom1_va2.rom` — 128 KiB

The PC-88VA2 boot ROM, and **the image that initialises the CPU's on-chip
peripherals** (§5.7). Structurally aligned with `varom1.rom` throughout:

| Offset | Content |
|---|---|
| `0x00000` | `E9` near jump to `0x12B8`, the reset entry |
| `0x00F20` | peripheral configuration table A — `FFFEh`…`FFE0h` (§5.7) |
| `0x00F43` | peripheral configuration table B — ICU, TCU, DMAU (§5.7) |
| `0x00F5E` | IVT override table (§6) |
| `0x012B8` | reset entry; table A walker at `0x12ED`, table B walker at `0x1307` |
| `0x013B1` | `0F FE 90` — the `BRKEM2` handoff, at the **same offset** as in `varom1.rom` (§8) |
| `0x013ED` | vector installer, called from `0x1364` (§6) |
| `0x0FFF0` | `EA 00 00 00 F0` — `jmp F000:0000`, the reset vector |
| `0x10000` | Z80. N-88 BASIC, banner at `0x179C2`; opens `F3 31 A0 E1` = `DI` / `LD SP,0E1A0h` |
| `0x18000` | Z80. N-88 BASIC, second banner at `0x1CF92` |

`[ROM]` **This is the image §6 and §8 actually describe.** Their `0x13B1`,
`0x13B5`, `0x13ED`, `0x1364` and `0x0F5E` all match here and none of them
matches `varom1.rom`, whose reset entry is `0x0A6F`, whose handoff is at
`0x0B15`, and whose override table is at `0x097D` — consumed at `0x0B76`
with `cx = 6`, which is exactly the six entries listed in §6. See
Appendix A item 27.

`[ROM]` The two images share their Z80 halves almost exactly — both carry
the N-88 BASIC banners at `0x179C2` and `0x1CF92`, and the whole
`0x10000`–`0x1FFFF` range is byte-identical apart from **one byte**
(Appendix C.5 below). Their x86 halves are independently built.

### C.5 The PC-8801-mode ROM set

`[ROM]` Seven further images, dumped in the PC-8801 emulator naming
convention. Six of them place exactly, by byte-identical match, inside the
boot ROMs already described:

| file | size | where it sits | Z80 window |
|---|---|---|---|
| `n88.rom` | 32 KiB | `varom1.rom` **and** `varom1_va2.rom` @ `0x10000` | `0000`–`7FFF` — N88-BASIC, banner `N-88 BASIC Version 1.9` at its `0x79C2` |
| `n88_0.rom` | 8 KiB | `varom1.rom` @ `0x18000` (VA1 only — see below) | `6000`–`7FFF`, extension bank 0 |
| `n88_1.rom` | 8 KiB | both @ `0x1A000` | extension bank 1 |
| `n88_2.rom` | 8 KiB | both @ `0x1C000` | extension bank 2, banner `N-88 BASIC Version 2.4` at its `0x0F92` |
| `n88_3.rom` | 8 KiB | both @ `0x1E000` | extension bank 3 |
| `n80.rom` (VA's) | 32 KiB | nothing matches — see the note below | — |
| `disk.rom` | 8 KiB | no match in any image | FDD sub-CPU ROM — **out of scope** (§0) |

`[ROM]` **The Z80 halves of the two boot ROMs differ by exactly one
byte.** Every 8 KiB bank from `0x10000` to `0x1FFFF` is byte-identical
between `varom1.rom` and `varom1_va2.rom` except extension bank 0, which
differs in a single position: image offset `0x19377`, i.e. **Z80 address
`7377h`**, where VA1 has `29h` and VA2 has `19h`. This supersedes the
looser statement in the `varom1_va2.rom` entry above; the x86 halves are
independently built, the Z80 halves are the same ROM with one byte
changed.

`[DERIVED]` That byte is an opcode, and the change is a repair:

```
        B7          OR A
        ED 52       SBC HL,DE
        30 04       JR NC,skip
        11 C0 A8    LD DE,0A8C0h
        29 / 19     ADD HL,HL   (VA1)  /  ADD HL,DE   (VA2)
skip:   11 2C 01    LD DE,012Ch
        E5          PUSH HL
```

`0A8C0h` is 43200 — the number of seconds in twelve hours — and the
surrounding shape is a modular wrap after a subtraction went negative.
`ADD HL,DE` applies the constant just loaded; `ADD HL,HL` doubles `HL` and
leaves the `LD DE` pointless. **VA1 has the bug and VA2 has the fix**, in
what is almost certainly twelve-hour clock arithmetic. The identification
of the constant is inference; the opcode difference and its context are
not.

`[ROM]` The set is from a **PC-88VA1**; the register dump in §17.2 came
from a VA2 belonging to someone else, which is why `n88_0.rom` matches
`varom1.rom` and not `varom1_va2.rom`. The two differ by the single byte
described above, so nothing else in this appendix is affected.

`[ROM]` The VA's `n80.rom` is **not** an N-BASIC ROM despite the name, and
the reason is now established. Its lower 24 KiB is x86 code carrying
`SCL v1.` and `(C) 1987 NEC Corp.`, and that string is present in
`varom00_va2.rom` at `0x2E409` — i.e. it is **V3 main-ROM content showing
through the N80 window**, which is empty on the VA because there is no
N-BASIC (below). The exact 24 KiB block does not match `varom00_va2.rom`
only because the VA1 and VA2 main ROMs differ and the VA1's is not in
hand. `[DERIVED]` The dump tool read the N80 mode window, found nothing
mapped, and captured whatever lay underneath.

`[ROM]` Later finding (M103c, which contradicts the `[DERIVED]` reading
above): N-88 BASIC's MON statement (handler `02E4h`) sets port 31h RMODE
and keeps executing at `02ECh`–`02FCh` before checking for the `DB`
signature at `6000h`. MON works on a real VA2 (the register dump in §17.2
was taken with it), so with RMODE set
`0000h`–`5FFFh` must still read N88-BASIC, not V3 code; vaeg maps it that
way ([`v1v2-mode-decisions.md`](v1v2-mode-decisions.md) M3). The lower
24 KiB of this dump cannot have been read in that state, and how the dump
was made is unknown. The top-8 KiB Debug 8800 finding is unaffected.

`[ROM]` The **top** 8 KiB of that file is a different matter and is
genuine: it is the Debug 8800 bank, byte-identical to
`varom00_va2.rom[0x1E000:0x20000]` (§4.5). The VA implements the
`6000h`–`7FFF` bank window even though it has no N-BASIC below it — which
is why §4.5's comparison against a real 8801mkIISR was possible at all.

### C.6 A real PC-8801mkIISR `n80.rom`, for comparison

`[ROM]` A genuine 8801mkIISR N-BASIC ROM is also in hand — 32 KiB, opening
`F3 31 FF FF C3 3B 00` (`DI` / `LD SP,0FFFFh` / `JP 003Bh`), banner
`NEC PC-8001 BASIC Ver 1.5 / Copyright 1979 (C) by Microsoft` at `0x1838`,
and carrying **Debug 8800 in its top 8 KiB** at `0x6000`–`0x7FFF`, banner
at `0x7058`. `[ROM]` That bank is not equal to any of `n88_0.rom` through
`n88_3.rom` — Debug 8800 is a **fifth** bank, distinct from the four N88
extension banks, appearing in the same `6000h`–`7FFF` window.

`[ROM]` **The 24 KiB / 8 KiB split in that file is an architectural
boundary, not a dump artefact.** N-BASIC fills `0000h`–`5FFFh` to the last
byte — `0x5FF0`–`0x5FFF` is dense code and there is no padding anywhere
below `0x6000` — and `0x6000` begins abruptly with the `DB` bank
signature. 24 KiB is the PC-8001's ROM complement; `6000h`–`7FFF` is
space the PC-8801 added on top of it. `[DERIVED]` That is why the VA's
`n80.rom` divides exactly there: the VA has no PC-8001 ROM, so
`0000h`–`5FFFh` reads through to whatever lies beneath, while
`6000h`–`7FFF` is a bank window the VA does implement. (M103c showed that
on a VA what lies beneath is N88-BASIC; see the note in C.5.)

`[ROM]` The PC-8001 lineage is still visible inside: `JP 5C66h` occurs at
`0x5D0F`, `0x5DAB` and `0x5DDB`, and `0x5C66` opens
`LD SP,(0FF36h) / CALL 5FCAh` — a warm start that restores the stack
pointer. The 8801 replaced the cold entry (`0x0000` is
`DI / LD SP,0FFFFh / JP 003Bh`) but kept `5C66h` as an internal entry. Two results follow from comparing it with the VA:

- **The VA's Debug 8800 is a patched version of it** — 52 bytes, three
  `HALT`s, one 110-byte window. See §4.5, which is the substantive
  finding.
- **The PC-88VA carries no N-BASIC.** The banner
  `NEC PC-8001 BASIC Ver 1.5` appears in none of `varom1.rom`,
  `varom1_va2.rom` or `varom00_va2.rom`, and neither does any part of the
  real ROM's lower 24 KiB. `[DERIVED]` The VA therefore implements V1/V2
  (N88-BASIC) mode but **not** N80 / PC-8001 mode.

  **Confirmed on hardware.** `NEW ON 1` — the N88-BASIC command that
  switches to N-BASIC — answers `Feature not available` on the VA, and
  the same holds for VA2 and VA3. (Operator report; §0 has no tag for
  that, see the tag note in Appendix D.3.) The absence argument above is
  therefore not load-bearing: the machine says so itself, on all three
  models, which also covers `varom00.rom` not being in hand.

  `[ROM]` The message has two independent homes, so either mode can raise
  it: the V1/V2 BASIC keeps it in extension bank 2 at Z80 `6206h`
  (`n88_2.rom` `0x206`, `varom1*.rom` `0x1C206`), and the V3-side ROM
  keeps its own copy at `varom00_va2.rom:0x70377` — surrounded by code
  that shares no 256-byte run with any N88 bank, so it is a separate
  message table rather than a second copy of the BASIC. Which one the
  report came from is not determined and does not matter to the
  conclusion.

### C.7 `varom08.rom` — 128 KiB

Entirely `0xFF`. Unpopulated.

### C.8 Not located in any supplied image

- Any genuine `CALLN` instruction (all `ED ED` hits are false positives).
- **Any `RETEM` at all.** `ED FD` occurs zero times across **all ten
  images** — the four VA images and the seven PC-8801-mode ROMs above,
  including `n88.rom`, all four extension banks and `disk.rom` (§8.1).
  Since `RETEM` is the only exit from a `BRKEM`-shaped session, nothing in
  any ROM this machine contains can return it to V3 mode.
- The V1/V2 sub-ROM / disk BIOS. §8.1 gives it a signature: it should
  contain an `ED FD` and a writer for `1000:C003`.
- Whatever prepares `1000:C003` — the same artifact, on the same path.
- **Any V1/V2 software that returns to V3 at all.** Not a ROM image and
  possibly not a thing that exists; §8.1 argues that commercial 88 titles
  cannot be it. One example would settle `doc-c003` outright.


## Appendix D. `IC78` — `PCZ80-27` pin list

`[ROM]` Transcribed from a pin-numbered magnification of the IC78 pin box
on VA1 schematic p.250. 100-pin device. **The transcription is
arithmetically self-checking**: the 48 upper-row names and the 48
lower-row names between them account for pins 1–100 with no gap and no
duplicate, and `VDD` (15, 65, 66) and `VSS` (40, 89, 90) take three pins
each. A misread digit would almost certainly have produced a duplicate
and a hole. The *names* carry no such check — see Appendix D.3.

Pin directions are **not** marked in the scan. Nothing here says which
pins are inputs.

```
  1 XPSTB      21 AD5        41 S18OCLK    61 XPIC       81 XMNRST
  2 PBUSY      22 AD6        42 MINT6      62 XIDP       82 CPCLK
  3 CLDI       23 AD7        43 APSGIR     63 X8BMOD     83 CPCLK2
  4 CLD0       24 AD8        44 INTRO      64 XAIORD     84 DLCLK
  5 CLC2       25 AD9        45 RXRDY      65 VDD        85 XRESET
  6 CLC1       26 AD10       46 VRTC       66 VDD        86 KREQ
  7 CLC0       27 AD11       47 IR3        67 XAIOWR     87 XKDTWR
  8 CLCLK      28 AD12       48 CPCLK3     68 XAITAK     88 KALE
  9 CLSTB      29 AD13       49 IR5        69 XIOWIT     89 VSS
 10 AVC2       30 AD14       50 IR7        70 XNMIO      90 VSS
 11 AVC1       31 AD15       51 PICINT     71 NMI1       91 KBINT
 12 FBEEP      32 PS3        52 PSGINT     72 XIOCHK     92 XKGMRD
 13 BEEP       33 XUBE       53 XFPCI      73 XIFDSL     93 KD0
 14 JOPI       34 BS2        54 XFPC0      74 DCD        94 KD1
 15 VDD        35 BS1        55 XFPO2      75 CRTMD      95 KD2
 16 AD0        36 BS0        56 XPPORT     76 X82        96 KD3
 17 AD1        37 XDMAK0     57 XSOUND     77 X81        97 KD4
 18 AD2        38 XDMAK2     58 XEHSND     78 XTEST1     98 KD5
 19 AD3        39 XDMAK3     59 XPRINT     79 XTEST0     99 KD6
 20 AD4        40 VSS        60 XSPORT     80 XPWRST    100 KD7
```

### D.1 Pins that matter elsewhere in this document

| Pin | Name | Bears on |
|---|---|---|
| 32 | `PS3` | §12, §2.2, §3.3 — the CPU mode line, the whole point of this appendix |
| 33 | `XUBE` | §12 — upper byte enable |
| 34–36 | `BS2`–`BS0` | §12 — bus status; the gate array watches the status phase |
| 16–31 | `AD0`–`AD15` | §12 — pin = 16 + *n* |
| 37, 38, 39 | `XDMAK0`, `XDMAK2`, `XDMAK3` | §5.4 check 5 — `XDMAK3` requires OPCN PF ∈ {`00`,`01`}. Channel 1 is **not** routed here |
| 45, 74 | `RXRDY`, `DCD` | §5.4 check 6 — an external asynchronous serial controller exists, as OPSEL SS=0 requires |
| 51, 61, 68 | `PICINT`, `XPIC`, `XAITAK` | §5.4 check 6 — the interrupt-acknowledge path that PF=`01` costs the CPU |
| 63 | `X8BMOD` | §12 — reads as "8-bit mode"; the plausible output side of the `PS3` decode |
| 47, 49, 50 | `IR3`, `IR5`, `IR7` | interrupt request lines toward the external μPD8259A of p.248 |
| 69 | `XIOWIT` | reads as an I/O wait output; note that WCY1 programs only 2 I/O waits and leaves the rest to external `READY` (§5.3) |

### D.2 Functional groups

- **Keyboard** (to `IC36 = μPD7811`): `KD7`–`KD0` (100–93), `KALE` (88),
  `KBINT` (91), `XKGMRD` (92), `XKDTWR` (87), `KREQ` (86).
- **Sound**: `BEEP` (13), `FBEEP` (12), `XSOUND` (57), `PSGINT` (52),
  `APSGIR` (43), `AVC2`/`AVC1` (10, 11).
- **Printer**: `XPRINT` (59), `XPPORT` (56), `XPSTB` (1), `PBUSY` (2).
- **Calendar clock**: `CLD0` (4), `CLDI` (3), `CLC2`–`CLC0` (5–7),
  `CLCLK` (8), `CLSTB` (9).
- **Display**: `DLCLK` (84), `VRTC` (46), `CRTMD` (75).
- **FDD / disk**: `XFPC0` (54), `XFPCI` (53), `XFPO2` (55),
  `XIFDSL` (73), `XEHSND` (58).
- **Reset and clocks**: `XRESET` (85), `XPWRST` (80), `XMNRST` (81),
  `CPCLK` (82), `CPCLK2` (83), `CPCLK3` (48), `X81` (77), `X82` (76),
  `S18OCLK` (41).
- **Bus control toward the 88-side**: `XAIORD` (64), `XAIOWR` (67),
  `XAITAK` (68), `XIOCHK` (72), `XIDP` (62), `XSPORT` (60).
- **Interrupts**: `XNMIO` (70), `NMI1` (71), `MINT6` (42), `INTRO` (44).
- **Test**: `XTEST0` (79), `XTEST1` (78).

### D.3 Caveats

- **Names are OCR-grade, but a second independent read now agrees.** The
  pin *numbers* are cross-checked by the 1–100 accounting above. The
  *names* have since been re-read from a 600 dpi scan by OCR (§12),
  which independently returned `PCZ80-27`, `XTEST0`, `BS2`, `BS0`,
  `XUBE`, `PS3`, `AD15`–`AD0`, `KALE`, `XKDTWR`, `KREQ`, `KBINT`,
  `XRESET`, `XFPC2`, `BEEP`, `XAIOWR`, `XAITAK`, `XPWRST`, `XMNRST`,
  `XDMAK2`, `XDMAK0`, `XPIC`, `XSPORT`, `XPRINT`, `XSOUND`, `XPPORT`,
  `XIOCHK`, `RXRDY`, `IR3`, `IR5`, `PICINT`, `PSGINT`, `PBUSY`,
  `APSGIR`, `VSS` — matching the table above. Three provisional readings
  are resolved by it: `INTRO` is **`INTR0`**, `CLD0` is **`CLD0`**, and
  `XIFDSL` is confirmed. Still provisional and still carrying no
  argument: `XKGMRD`, `S18OCLK`, `XNMIO` (probably `XNMI0`), `X81`/`X82`,
  `AVC1`/`AVC2`.
- The pins the arguments rest on — `PS3`, `XUBE`, `BS2`–`BS0`,
  `AD15`–`AD0`, `XDMAK3`, `RXRDY`, `DCD`, `X8BMOD` — are unambiguous
  readings and sit in positionally coherent groups.
- **Tag note.** §0 has no tag for schematics. §12 and this appendix use
  `[ROM]` for them, which stretches its definition past breaking. Two
  other places rest on how the machine behaves or is operated in
  practice, which is neither a document nor a binary: §8.1 on how a V1/V2
  session actually ends, and Appendix C on `NEW ON 1` answering
  `Feature not available`. If §0 ever grows a `[SCH]` tag for schematics
  and something like `[FIELD]` for operator observation, those are the
  places to retag — and until then, each is flagged inline where it
  occurs.

---

# References

Sources are grouped by the provenance tag (§0) each one licenses. The
tags are not one-to-one with works — several sources share a tag — so the
mapping is given explicitly below. `[DERIVED]` and `[UNKNOWN]` are
reasoning states rather than sources and have no entry here.

| Tag | Entries |
|---|---|
| `[V30-MAN]` | [1] [2] [3] [4] |
| `[VA-TM]` | [5] |
| `[VA-TEKU]` | [6] |
| `[VA-WIKI]` | [7], reproducing [8] and citing [9] |
| `[ROM]` | [10] [11] [12] [13] [14] [15] |
| `[SRC]` | [16] [17] [18] [18a] |
| `[NEC-GIHO]` | [25] |
| *(no tag defined)* | [19] [20] — schematics and operator observation; see the tag note in Appendix D.3 |
| *(not a source)* | [21] [22] [23] [24] |

## NEC device documentation — `[V30-MAN]`

**[1]** NEC Corporation. *16-bit V Series Instruction Manual*. Document
`U11301EJ5V0UMJ1`. — `BRKEM` and `CALLN` operation, flag tables,
instruction classification, register and segment encodings.
<https://datasheets.chipdb.org/NEC/V20-V30/U11301EJ5V0UMJ1.PDF>
*In hand.*

**[2]** NEC Corporation. *V20/V30 User's Manual*. October 1986.
**Chapter 8 complete.** — mode shifting, register and flag
correspondence, segment usage, interrupt / `RESET` / `HALT` behaviour,
the nesting prohibition, and `PS3` indication. Its Figure 8-1 and
Figure 8-2 are cited by number in §3.1 and §3.2. *In hand.*

**[3]** NEC Corporation. *μPD70216 (V50) Data Sheet*. Document 50008
(NECEL-419) — the original V50 sheet, not the later HL variant. Carries
the **system I/O area register map** (`FFF0h`–`FFFFh`), the bit layouts
and reset values of OPCN, OPSEL, OPHA, DULA, IULA, TULA, SULA, WCY1,
WCY2, WMB, RFC and TCKS (its figures 11–18), and the register models of
the TCU, SCU, ICU and DMAU. §5.2 decodes the PC-88VA2 configuration
bytes against it. *In hand.*

**[4]** NEC Corporation. *V40HL/V50HL Data Sheet*
(`μPD70208H`/`μPD70216H`). Document `U13225EJ4V0DS00`. — family analogy
for the on-chip peripheral model only.
<https://datasheets.chipdb.org/NEC/V40-V50/> *In hand.*

## PC-88VA manufacturer documentation — `[VA-TM]`

**[5]** *PC-88VA テクニカルマニュアル* [*PC-88VA Technical Manual*]. BNN,
first edition 25 June 1987. ISBN 4-89369-024-8, ¥5,500. Long out of
print. — page 12 gives the two CPU modes, compatible-mode instruction
coverage, `CALLN 91h`, `CALLN 95h` and `RETEM`; its own §2.2 gives the
I/O port block map quoted in §5.1, and its own §3.3 the memory-map
registers `152H`/`153H` quoted in §8.2, and its own §1.4 the system
boot-process flowchart used in §8, §8.1 and §8.2. OCR of the scan is
noisy;
typography and register notation are normalised against the page image.
<https://archive.org/details/PC88VA/page/12/mode/2up> *Scan in hand.*

## Community documentation — `[VA-TEKU]`, `[VA-WIKI]`

**[6]** 「てくまに」 / `TEKUMANI.LZH` (249 KiB, LHA). A PC-88VA technical
manual written independently by members of the PC-VAN "88VA Users Club"
SIG — system overview, memory, I/O, display, hardware control and BIOS.
**Distinct from [5]**: separately authored, community-edited, and its own
distribution page states that bugs have been reported in it. Hence the
weaker tag.
<http://www.iris.dti.ne.jp/~nano/88va/tekumani.html> *In hand.*

**[7]** Shinra (ed.). *Inside PC-88VA* wiki. PukiWiki, 2005–2011.
**§1.5 CPU.** — device overview, V30-mode instruction set, built-in
peripheral control including the VA2 configuration bytes, and the I/O
trap. Documents what [6] omits and carries its errata.
<http://www.pc88.gr.jp/inside88va/wiki/index.php?CPU> — the query-string
URL does not fetch reliably; read manually.

**[8]** CoBit. Post #4016 to the PC-VAN "88VA Users Club" SIG board
「PC実験室」, 31 March 1992. Reproduced verbatim in [7]. — the sole source
for the whole of §9, including the control-register names
(`_IOTrap1S` and the rest), which are CoBit's own coinage and not NEC's.
CoBit states plainly that the description comes from ROM analysis and
experiment and may contain errors or omissions.

**[9]** *マイコン* [*Micom*], August 1987. — cited by [7] as the source
for the four deleted instructions (§4.2). *Not in hand; reached only
through [7].*

## Firmware and ROM images — `[ROM]`

**[10]** PC-88VA and PC-88VA2 ROM images: `varom00.rom`,
`varom00_va2.rom`, `varom1.rom`, `varom1_va2.rom`, `varom08.rom`.
Contents, offsets, landmarks and the identification of each are in
Appendix C. *In hand.*

**[11]** The PC-88VA PC-8801-mode ROM set, dumped from a **PC-88VA1** in
the PC-8801 emulator naming convention: `n88.rom`, `n88_0.rom` through
`n88_3.rom`, `n80.rom`, `disk.rom`. Placement and analysis in
Appendix C.5. *In hand.*

**[12]** A real PC-8801mkIISR `n88.rom` with its four N88 extension banks
and its `disk.rom`. — the comparison baseline for §4.6. *In hand.*

**[13]** A real PC-8801mkIISR `n80.rom` — 32 KiB, N-BASIC in the lower
24 KiB and Debug 8800 in the top 8 KiB. The comparison baseline for §4.5
and the subject of Appendix C.6. *In hand.*

**[14]** A `0000h`–`00FFh` dump taken from a real PC-8801 running
N88-BASIC 1.93. — the byte-for-byte check on `varom1.rom:0x10000` in
Appendix C.1.

**[15]** *Debug 8800 version 1.0.* NEC, 11 December 1981. A PC-8801-era
Z80 monitor carried inside [10] and [13]; the corpus of
proven-executable compatible-mode Z80 code analysed in §4.4 and §4.5, and
the instrument for most of §17.

## Period source and binaries — `[SRC]`

**[16]** CPMVA 1.3. Makichan, 1989. pc88.gr.jp software library,
<http://www.pc88.gr.jp/softlib/?action=list_file&anum=2&gnum=424> —
sources `CPMVA.ASM`, `CPMVA.H`, `CPMBIOS.MAC`, `V30.MAC`, `EXIT.MAC`,
`CHARDEV.ASM`, `CRTOUT.ASM`, `BLOCKDEV.ASM`, `MAKEFILE`, `CPMVA.DOC`;
binaries `CPMVA.EXE`, `CPMBIOS.COM`, `EXIT.COM`, `DO.COM`, `FCONV.COM`.
The worked V3-side transition example of §10, and the shipped binary that
proves compatible mode is Z80 (§2.4). *In hand.*

**[17]** 98IOE 2.4. CoBit, 1992. The I/O-trap sample program referenced
from [7] — `98IOE.ASM`, `IOTRAP.ASM`, `IOTRAP.INC`, `INIT.ASM`,
`STD.INC`, `DEBUG_C.ASM`, `98IOE.DOC`, makefiles; binary `98IOE.COM`.
The reference implementation behind §9.1–§9.6. *In hand.*

**[18]** Debug 8800 `x`-command register dumps, taken from a real
PC-8801 and a real PC-88VA2 after booting N88-DISK-BASIC and typing
`mon`. Reproduced in §17.2; the evidence that closes `alt-regs` and
`ix-shares-si`.

**[18a]** CP/M emulator for MS-DOS, version 0.8. Vector software page:
<https://www.vector.co.jp/soft/dos/util/se000015.html>; archive
<https://ftp.vector.co.jp/58/06/545/cpm08.zip>. Archive SHA-256
691e51dda202ab97b7c8c947ca7c9bf2d93d822f3e315362fcc7840199b8d6f7.
`CPM.ASM` and `CPM.EXE` identities are recorded in §10.1. This is the
`.cpv` V30 hard-emulation path discussed in §10.1. The separate Win32
package page <https://www.vector.co.jp/soft/win95/util/se378130.html>
describes a different v0.4 artifact and is not its source.

## Schematics and field observation — *(no tag defined)*

§0 defines no tag for either of these. §12 and Appendix D use `[ROM]` for
schematics, which stretches its definition; the operator observations are
flagged inline where they occur. See the tag note in Appendix D.3.

**[19]** *I/O* magazine, August 1987, pp. 241–264 — the full PC-88VA1
schematic set. p.248「CPU割り込み周辺」 carries `IC83 = D9002` and the
external `IC38 = D8259A`; p.250「PC-88V1/V2モード・エミュレータ」 carries
`IC78 = PCZ80-27` and `IC36 = μPD7811`. A pin-numbered magnification of
IC78's pin box, and a 600 dpi OCR re-read of both sheets, are the basis
of §12 and of the transcription in Appendix D. *In hand.*

**[20]** Operator observation on real hardware. Two claims rest on it and
on nothing else: how a V1/V2 session actually ends in practice (§8.2.1),
and `NEW ON 1` answering `Feature not available` on VA, VA2 and VA3
(Appendix C.6).

## Implementation comparison — *(not a source for silicon behaviour)*

**[21]** MAME, NEC V20/V30 CPU core. — the implementation comparison
point of §13. It models the standard V30 emulation path and does not
implement `BRKEM2`.

## Project documents

**[22]** `pc88va-boot-sequence.md` — the general PC-88VA boot trace. Only
the parts bearing on mode transition are repeated here.

**[23]** `docs/agents/reports/m9_v30_map.md` — records the main CPU's
compatible mode as an explicit future item (§14).

**[24]** `upd9002-z80-emulation.md`, and the earlier provenance-tagged
hardware reference. **Both superseded by this document**, which is their
merge. Errors carried over from them are listed in Appendix A.

## Manufacturer journal — `[NEC-GIHO]`

**[25]** NEC Corporation. 「パーソナルコンピュータ PC-88VA」. *NEC 技報*
Vol. 40 No. 3, 1987. — the N88V1/V2 memory map (Figure 3) and the
88-mode area map (Figure 4), the I/O interface section with the 8214
and 8259 interrupt modes, the I/O map (Figure 5), the 3301-compatible
text mode, and §4.1 on the V1/V2 supervisor. *Consulted through a
maintainer-supplied OCR transcription of three page images; the
transcription and images are not in the tree.* Figure boundaries not
written as numbers in the figures were not inferred by the transcriber.

## Not in hand

- **A μPD9002 device manual.** Nothing tagged `[VA-TM]` or `[VA-TEKU]`
  should be mistaken for one. This is the single largest gap; §15.1 lists
  what it would settle.
- **`CPM.SYS`** — the CCP + BDOS + BIOS image that `CPMVA.EXE` loads
  ([16]). Its absence is the gate on the §17.7 acceptance test.
- **The V1/V2 sub-ROM / disk BIOS.** §8.1 gives it a signature: it should
  contain an `ED FD` and a writer for `1000:C003`.
- **Remaining PC-88VA ROM images** beyond those listed in [10].
- *マイコン* August 1987 ([9]) in the original.

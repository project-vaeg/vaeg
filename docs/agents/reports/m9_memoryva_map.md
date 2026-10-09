<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M9 memoryva.x86 interface map

Scope: M9 step 1 only. No source files were changed outside this report.

## Normative order

For M9 transliteration, the normative source is `cpuxva/memoryva.x86`.
The behavior reference is the LEGACY v141 build. The PC-88VA technical
manual under `docs/tekumani/` is annotation only.

Where `memoryva.x86` and the technical manual diverge, M9 ports the asm
behavior as-is. Converging behavior to the manual is a behavior change
and is out of scope without an ADR and a separate milestone.

Inputs studied:

- `cpuxva/memoryva.x86`
- `cpuxva/memoryva.h`
- `i286x/memory.x86` and `i286c/memory.c`
- `i286x/dmap.x86` and `i286c/dmap.c`
- `i286x/egcmem.x86` and `i286c/egcmem.c`

## Transliteration conventions from i286x to i286c

The existing C counterparts establish these rules for the M9 port:

| Assembly pattern | C pattern already used in `i286c/` | M9 implication |
|---|---|---|
| Decorated public entry points such as `@i286_memoryread@4` receive arguments in registers. | Public C entry points use the same logical names without decoration, for example `REG8 MEMCALL i286_memoryread(UINT32 address)`. | VA entry points should become `i286_memoryread_va`, `i286_memoryread_va_w`, `i286_memorywrite_va`, `i286_memorywrite_va_w`, and `i286_memorymap_va`. |
| `ecx` is the address register; `dl`/`dx` carry byte/word write values; `al`/`ax` carry byte/word read returns. | `address` and `value` parameters replace register protocol; function return replaces `al`/`ax`. | Documented register contracts map cleanly to C signatures. |
| Computed jumps index arrays of routine addresses. | `i286c/memory.c` uses typed function-pointer tables (`MEM8READ`, `MEM8WRITE`, `MEM16READ`, `MEM16WRITE`). | VA dispatch tables should remain table-driven and keep the same order and masks. |
| Raw x86 word loads/stores read little-endian data directly from byte buffers. | `i286c/memory.c` uses `LOADINTELWORD` and `STOREINTELWORD` for multi-byte memory accesses. | Every `mov ax, [buffer + offset]` and `mov [buffer + offset], dx` in `memoryva.x86` must become those macros, not casts. |
| Saved scratch registers such as `ebx` are caller-invisible implementation detail. | C uses local variables and pointers. | EBX stack protocol is not reproduced; only the externally visible memory effects and return values are. |
| Public asm globals back C headers directly. | C defines the named data objects from headers. | `memoryva.h` defines the base memory ABI; mismatches with asm-only globals are listed below. |
| `i286x/dmap.x86` toggles `_dma_access` around CPU memory callbacks when `SUPPORT_PC88VA` is set. | `i286c/dmap.c` calls `i286_memoryread/write` directly and does not toggle a VA DMA flag. | The later C port must restore this behavior for VA DMA access, likely by setting `memoryva.dma_access` around DMA memory calls. |

Additional precedent:

- `i286c/egcmem.c` keeps large lookup tables as `static const` data in
  the same order as the asm source.
- Internal asm labels become `static` C helpers unless a header exposes
  them.
- C sources that already consume VA text memory use Intel-word macros in
  several places, but `iova/sgp.c` still contains direct `REG16 *` casts;
  that is outside step 1 and is only noted here as a future portability
  check.

## Technical manual annotations

These are spec-vs-implementation divergences found during maintainer
review against `docs/tekumani/`. They do not override the normative asm
source for M9.

| Topic | Technical manual | Legacy implementation | M9 action |
|---|---|---|---|
| TVRAM size | The original VA has 64KB at `A0000H-AFFFFH` (`docs/tekumani/4.TXT:202`, `docs/tekumani/4.TXT:215`). NEC product specifications list 256KB for VA2/VA3. | `memoryva.x86`/`memoryva.h` expose `textmem[0x40000]`. | Preserve the shared legacy buffer; enforce the model-specific CPU aperture at the access boundary. |
| Backup RAM size | Backup RAM is 8KB at `B0000H-B1FFFH`; the memory switch area is `B1FC0H-B1FFFH` (`docs/tekumani/3.TXT:224`, `docs/tekumani/3.TXT:229`, `docs/tekumani/3.TXT:231`). | `memoryva.x86`/`memoryva.h` expose `backupmem[0x4000]`. | Preserve the legacy buffer. Do not shrink storage in M9. |
| Backup RAM access width | Backup RAM memory access is byte-only (`docs/tekumani/3.TXT:251`). | `memoryva.x86` implements special word read/write behavior for this range. | Port the asm behavior and keep this note. |
| ROM0 bank range | The spec lists ROM00-ROM05, then reserve, then bus slots (`docs/tekumani/3.TXT:173`, `docs/tekumani/2.TXT:1476`). | VAEG legacy code uses ROM0 banks 0-9, backed by `VAROM00.ROM` and `VAROM08.ROM`. | Treat banks 0-9 as a VAEG implementation extension and keep as-is. |
| DMA to backup RAM | The manual forbids DMA access to backup RAM (`docs/tekumani/3.TXT:322`, `docs/tekumani/3.TXT:325`). | The asm path does not explicitly reject it; see the implementation checklist below. | Settle this before transliteration: preserve the legacy pass-through unless an ADR authorizes a behavior change. |

### Post-M9 correction: bank-1 TVRAM aperture

M29 demonstrated that preserving the legacy 256KB CPU-visible TVRAM range was
not compatible with PC-88VA hardware. PC-Engine 1.00 uses a write/read memory
probe and expects bank 1 to stop responding above `AFFFFH`. The transliterated
C implementation instead made `B0000H-DFFFFH` writable, causing the program to
place its stack in the banked system-memory window. A later VA1 ROM bank switch
then made that stack disappear and corrupted the restored bank number.

M29 retains the legacy `textmem[0x40000]` storage object to avoid an unrelated
ABI and renderer refactor. The 64KB aperture at `A0000H-AFFFFH` is enforced
for VA1; reads from the unused remainder return open-bus ones and writes are
ignored. A later M28/M29 A/B test found that applying this clamp to VA2/VA3
regressed V3 BASIC. NEC's
[VA](https://support.nec-lavie.jp/support/product/data/spec/cpu/b047-1.html),
[VA2](https://support.nec-lavie.jp/support/product/data/spec/cpu/b048-1.html),
and [VA3](https://support.nec-lavie.jp/support/product/data/spec/cpu/b049-1.html)
product specifications confirm that TVRAM increased from 64KB to 256KB in
VA2/VA3, so the model-specific active mapping retains the full 256KB CPU
exposure for those models. See
`tasks/M29_va1_tvram_aperture.md` for the trace, ROM control flow, correction,
regression, and validation record.

## Header surface and divergences

`cpuxva/memoryva.h` declares:

- `textmem`, `fontmem`, `backupmem`, `dicmem`, `rom0mem`, `rom1mem`
- `_MEMORYVA memoryva`
- `BOOL textmem_dirty`
- `void MEMCALL i286_memorymap_va(void)`

The assembly also exports the VA CPU memory read/write entry points and
an alias `_dma_access`. The four VA CPU memory read/write entry points
are not currently declared in `memoryva.h`.

## Exported data symbols

| Assembly symbol | Header-visible C symbol | Size / layout | Contract | C equivalent |
|---|---|---:|---|---|
| `_textmem` | `textmem` in `memoryva.h` | `0x40000` bytes | VA text/sprite RAM. Byte writes set `_textmem_dirty`. | `BYTE textmem[0x40000];` |
| `_fontmem` | `fontmem` in `memoryva.h` | `0x50000` bytes | Kanji ROM #1 and #2 backing store. | `BYTE fontmem[0x50000];` |
| `_backupmem` | `backupmem` in `memoryva.h` | `0x04000` bytes | Backup RAM. The asm comment requires it to be placed immediately after `_fontmem`; later C should verify whether any code depends on that contiguity. | `BYTE backupmem[0x04000];` |
| `_dicmem` | `dicmem` in `memoryva.h` | `0x80000` bytes | Dictionary ROM backing store. | `BYTE dicmem[0x80000];` |
| `_rom0mem` | `rom0mem` in `memoryva.h` | `0xa0000` bytes | Standard ROM0 banks 0-9. | `BYTE rom0mem[0xa0000];` |
| `_rom1mem` | `rom1mem` in `memoryva.h` | `0x20000` bytes | Standard ROM1 banks 0-1. | `BYTE rom1mem[0x20000];` |
| `_memoryva` | `memoryva` in `memoryva.h` | `_MEMORYVA` | Bank selectors, DMA bank selector, backup write-protect, ROM-existence masks. | `_MEMORYVA memoryva;` |
| `_dma_access` | No direct header symbol | Alias of `memoryva.dma_access` | Bit 7 marks DMA-origin memory accesses. `JMPSYSMBANK` tests it with `memoryva.dma_sysm_bank`. | Use `memoryva.dma_access`; do not create a separate object. |
| `_textmem_dirty` | `textmem_dirty` in `memoryva.h` | `BOOL`/dword in asm | Set to nonzero on text RAM writes. | `BOOL textmem_dirty;` |

## Exported routine symbols

| Assembly symbol | Register-level contract | Memory map dispatch | C-callable equivalent |
|---|---|---|---|
| `@i286_memorywrite_va@8` | Input: `ecx = address`, `dl = byte value`. No return. Calls `MEMTRACE`, then `pccore_debugmem(0, address, value)` when debug calls are enabled. | Uses dword-table byte offset `(address >> 14) & 0x3c`, equivalent to `((address >> 16) & 0x0f) * 4`, into `_membyte_write`. | `void MEMCALL i286_memorywrite_va(UINT32 address, REG8 value);` This prototype is not in `memoryva.h` yet. |
| `@i286_memorywrite_va_w@8` | Input: `ecx = address`, `dx = word value`. No return. Calls `MEMTRACE_W`, then `pccore_debugmem(1, address, value)`. The boundary path is entered only when `address + 1` wraps to zero because the original `test ebx, 7fffh` is commented out. | Normal path uses dword-table byte offset `((address + 1) >> 14) & 0x3c`. Boundary path writes high byte at `address + 1`, then low byte at `address`, through byte handlers. | `void MEMCALL i286_memorywrite_va_w(UINT32 address, REG16 value);` This prototype is not in `memoryva.h` yet. |
| `@i286_memoryread_va@4` | Input: `ecx = address`. Return: `al = byte value`. | Uses dword-table byte offset `(address >> 14) & 0x3c`, equivalent to `((address >> 16) & 0x0f) * 4`, into `_membyte_read`. | `REG8 MEMCALL i286_memoryread_va(UINT32 address);` This prototype is not in `memoryva.h` yet. |
| `@i286_memoryread_va_w@4` | Input: `ecx = address`. Return: `ax = word value`. The boundary path is entered only when `address + 1` wraps to zero because the original `test ebx, 7fffh` is commented out. | Normal path uses dword-table byte offset `((address + 1) >> 14) & 0x3c`. Boundary path reads high byte at `address + 1`, then low byte at `address`, through byte handlers. | `REG16 MEMCALL i286_memoryread_va_w(UINT32 address);` This prototype is not in `memoryva.h` yet. |
| `@i286_memorymap_va@0` | No input. No return. Reads the legacy extension enable flag. | Patches table slot `0xf` in system write/read, ROM0 read, and ROM1 read tables from the extension map. Disabled uses normal bank-f behavior; enabled uses the handlers documented in the VAEG/legacy extension section. | `void MEMCALL i286_memorymap_va(void);` Declared in `memoryva.h`. |

## Dispatch tables and handler contracts

Top-level address dispatch uses 16 entries of `0x10000` bytes, covering
the 1MB CPU address space. The asm computes a byte offset into a dword
function table as `(address >> 14) & 0x3c`, which is equivalent to
`((address >> 16) & 0x0f) * 4`.

| Table | 00000-7ffff | 80000-9ffff | a0000-dffff | e0000-effff | f0000-fffff |
|---|---|---|---|---|---|
| `_membyte_write` | `@i286_wt` | `@bms_wt` | `@sysm_wt` | `@i286_wn` | `@i286_wn` |
| `_memword_write` | `@i286w_wt` | `@bmsw_wt` | `@sysmw_wt` | `@i286_wn` | `@i286_wn` |
| `_membyte_read` | `@i286_rd` | `@bms_rd` | `@sysm_rd` | `@rom0_rd` | `@rom1_rd` |
| `_memword_read` | `@i286w_rd` | `@bmsw_rd` | `@sysmw_rd` | `@rom0w_rd` | `@rom1w_rd` |

System-memory bank dispatch uses these selectors:

- Normal system banks use `memoryva.sysm_bank & 0x0f`.
- DMA-origin accesses use `memoryva.dma_sysm_bank & 0x0f` when
  `memoryva.dma_access & memoryva.dma_sysm_bank` is nonzero.
- ROM0 uses `memoryva.rom0_bank & 0x1f`.
- ROM1 uses `memoryva.rom1_bank & 0x0f`.

System bank handlers:

| Bank | Byte write | Word write | Byte read | Word read |
|---:|---|---|---|---|
| 1 | `@tvram_wt` | `@tvramw_wt` | `@tvram_rd` | `@tvramw_rd` |
| 4 | `@gvram_wt` | `@gvramw_wt` | `@gvram_rd` | `@gvramw_rd` |
| 8 | none | none | `@knj1_rd` | `@knj1w_rd` |
| 9 | `@knj2_wt` | `@knj2w_wt` | `@knj2_rd` | `@knj2w_rd` |
| c | none | none | `@dic1_rd` | `@dic1w_rd` |
| d | none | none | `@dic2_rd` | `@dic2w_rd` |
| f | patched by `i286_memorymap_va` | patched by `i286_memorymap_va` | patched by `i286_memorymap_va` | patched by `i286_memorymap_va` |
| other | no write | no write | `0xff` | `0xffff` |

ROM handlers:

- Standard ROM0: legacy `memoryva.rom0_bank` must be below `0x0a`;
  offset is `(bank << 16) + address - 0xe0000`; otherwise reads return
  all ones. This is the VAEG implementation extension noted above; the
  technical manual lists ROM00-ROM05 plus reserve/bus-slot ranges.
- Standard ROM1: `memoryva.rom1_bank & 0x03` selects the bank. If bit
  1 is set, the invalid-bank handler returns a pattern derived from
  `address & 0xfffe`.

## Endianness-sensitive places

The C port must use `LOADINTELWORD` or `STOREINTELWORD` for:

- `textmem` word reads/writes (`@tvramw_rd`, `@tvramw_wt`)
- `dicmem` word reads (`@dic1w_rd`, `@dic2w_rd`)
- `rom0mem` and `rom1mem` word reads

Do not use raw `UINT16 *` casts in `memoryva.c`.

Special byte-composed word behavior to preserve:

- Backup RAM word writes at even addresses write only the low byte.
- Backup RAM word writes at odd addresses write high byte at
  `address + 1`, then low byte at `address`.
- Kanji/backup word reads at even addresses duplicate the byte into
  both halves of the word.
- Kanji/backup word reads at odd addresses read `address + 1` as the
  high byte and `address` as the low byte.
- `rom1invalidw_rd` returns `address & 0xfffe`; for even addresses it
  swaps the bytes and adds 2 to the low byte.

## Implementation checklist before transliteration

- DMA to backup RAM: the technical manual forbids it, but the legacy asm
  path passes through to backup RAM when the DMA system bank selects bank
  9. `i286x/dmap.x86` sets `_dma_access` to `0x80` around memory reads
  and writes (`i286x/dmap.x86:106`, `i286x/dmap.x86:121`).
  `JMPSYSMBANK` then chooses `_dma_sysm_bank` when
  `_dma_access & _dma_sysm_bank` is nonzero and masks it to 4 bits
  (`cpuxva/memoryva.x86:101`, `cpuxva/memoryva.x86:102`,
  `cpuxva/memoryva.x86:105`). Bank 9 dispatches to `@knj2_wt`,
  `@knj2w_wt`, `@knj2_rd`, and `@knj2w_rd`
  (`cpuxva/memoryva.x86:828`, `cpuxva/memoryva.x86:882`,
  `cpuxva/memoryva.x86:936`, `cpuxva/memoryva.x86:1040`). The byte
  write/read handlers accept the backup range via `BACKUPMEMORY_SIZE`
  checks (`cpuxva/memoryva.x86:189`, `cpuxva/memoryva.x86:332`), with
  word handlers delegating to the special byte-composed behavior
  (`cpuxva/memoryva.x86:255`, `cpuxva/memoryva.x86:513`). Therefore M9
  must transliterate this pass-through behavior, not insert a manual
  rejection.
- `memoryva.x86` jumps directly to i286x internal handlers:
  `@i286_wt`, `@i286w_wt`, `@i286_rd`, `@i286w_rd`, `@i286_wn`,
  `@bms_wt`, `@bmsw_wt`, `@bms_rd`, and `@bmsw_rd`. In `i286c/memory.c`,
  comparable handlers are `static`, and there is no visible BMS handler
  equivalent. The C port needs either local equivalents or an explicit
  hook from the C memory core.
- `i286x/memory.x86` conditionally dispatches all CPU memory calls to
  VA routines when `_memmode_va` is set. The portable C core does not
  yet have the equivalent VA dispatch path.
- `i286x/dmap.x86` sets `_dma_access` around memory callbacks under
  `SUPPORT_PC88VA`; `i286c/dmap.c` does not. VA DMA bank selection
  depends on this flag.
- The four C-callable VA read/write entry points are exported by asm
  but absent from `cpuxva/memoryva.h`. Later wiring should add
  prototypes or keep them private behind a new C-core dispatch layer.
- `pccore_debugmem` is called by `DEBUGCALL`/`DEBUGCALL_W` in the asm.
  The C port must decide whether the portable build should preserve
  these debug callbacks or compile them out in the same conditions.

## VAEG/legacy extension (not in the technical manual)

No VA91 material was found in the technical manual corpus under
`docs/tekumani/`. Treat this section as a VAEG/legacy implementation
extension, still normative for M9 because it is implemented by
`memoryva.x86`. Additional source files studied for this extension:
`iova/va91.h` and `iova/va91.c`.

VA91 header/layout notes:

- `memoryva.x86` exports the VA91 ROM buffers, `va91`, and `va91cfg`.
  The corresponding C declarations are in `iova/va91.h`, not
  `memoryva.h`.
- `memoryva.x86` defines `va91cfg_t.enabled` as `resd 1`, while
  `iova/va91.h` defines `_VA91CFG.enabled` as `UINT8` followed by three
  dummy bytes. The effective C-visible first four bytes match for the
  enabled flag, and `memoryva.x86` only reads that flag. The C port
  should follow the C header layout.

VA91 exported data:

| Assembly symbol | Header-visible C symbol | Size / layout | Contract | C equivalent |
|---|---|---:|---|---|
| `_va91rom0mem` | `va91rom0mem` in `iova/va91.h` | `0xa0000` bytes | VA91 ROM0 banks 0-9. | `BYTE va91rom0mem[0xa0000];` |
| `_va91rom1mem` | `va91rom1mem` in `iova/va91.h` | `0x20000` bytes | VA91 ROM1 banks 0-1. | `BYTE va91rom1mem[0x20000];` |
| `_va91dicmem` | `va91dicmem` in `iova/va91.h` | `0x80000` bytes | VA91 dictionary ROM. | `BYTE va91dicmem[0x80000];` |
| `_va91` | `va91` in `iova/va91.h` | `_VA91` | Runtime VA91 bank registers plus copied configuration. | `_VA91 va91;` |
| `_va91cfg` | `va91cfg` in `iova/va91.h` | `_VA91CFG` | User/configuration copy used by `va91_reset`. | `_VA91CFG va91cfg;` |

VA91 dispatch:

- `i286_memorymap_va` patches bank `0xf` entries to VA91 handlers when
  `va91.cfg.enabled` is nonzero.
- VA91 system bank dispatch uses `va91.sysm_bank & 0x0f`.

| Bank | Byte write | Word write | Byte read | Word read |
|---:|---|---|---|---|
| 9 | `@va91knj2_wt` | `@va91knj2w_wt` | `@va91knj2_rd` | `@va91knj2w_rd` |
| c | none | none | `@va91dic1_rd` | `@va91dic1w_rd` |
| d | none | none | `@va91dic2_rd` | `@va91dic2w_rd` |
| other | no write | no write | `0xff` | `0xffff` |

VA91 ROM handlers:

- VA91 ROM0: `va91.rom0_bank` must be below `0x0a`; data comes from
  `va91rom0mem`.
- VA91 ROM1: `va91.rom1_bank` must be below `0x02`; data comes from
  `va91rom1mem`.
- VA91 dictionary and ROM word reads must use `LOADINTELWORD` in the C
  port.

## Integration risks for later M9 steps

All known implementation risks are now listed in the implementation
checklist above so they are resolved before transliteration begins.

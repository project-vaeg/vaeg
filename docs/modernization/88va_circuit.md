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
# PC-88VA / PC-88VA2 Circuit Notes

LSI inventory and functional identification of the NEC PC-88VA main board,
compiled from the published full schematic of the original PC-88VA (VA1) and
cross-checked against reverse-engineered PC-88VA2 schematics.

## Sources

1. **[VA1]** "全回路図公開" (*Full schematics disclosed*), **I/O Magazine**,
   Kohgakusha, August 1987 issue, pp. 241–264. Complete schematic of the
   PC-88VA (VA1) main board and disk subsystem. The magazine notes the
   schematic was published on the editors' responsibility for reference only
   and is not vendor-guaranteed.
2. **[VA2]** **tomo_retro** — reverse-engineered PC-88VA2 schematics by tomo:
   `Design1_main.sch` ("PC-88VA2 (MAIN)") and `Design1_text_sub.sch`
   ("PC-88VA2 (SUB1)"), DesignSpark PCB 8.1 format, rev. 2019/01A.
   <https://tomoretropc.blogspot.com>
   Partial schematics drawn from continuity checks during board repair;
   they themselves reference [VA1].

Note: the schematic files themselves ([VA1] scans, [VA2] `.sch` files) are
**not** redistributed in this repository; only factual findings (part
numbers, reference designators, net names) derived from them are documented
here.

Part numbers taken from [VA1] were read from scanned magazine pages, so
low-confidence readings are marked with `?`. Readings later confirmed or
corrected by review or by [VA2] cross-check are noted as such.

## Methodology

- **[VA1]**: visual reading of the printed pages.
- **[VA2]**: the `.sch` files are OLE compound documents whose `Contents`
  stream holds MFC-serialized DesignSpark data (code page 932). Reference
  designators, part numbers and net names were extracted losslessly by
  scanning for length-prefixed CP932 strings. Pin-level netlist
  reconstruction has not been done yet; opening the files in DesignSpark PCB
  (freeware) and exporting a netlist remains the authoritative route.
- **Cross-check**: the [VA2] net-name vocabulary (`GVD00–GVD77`, `GA00–GA17`,
  `XCSW0–7`, `SCLK0–7`/`ISCLK0–7`, `ILCLK0/1`, `TXVD0–3`, `EXVD0–15`,
  `IOAD0–15`, `MA00–19`, `BS0–2`, `XUBE`/`XBHE`, `XJE0`/`XJCS`,
  `XRE0/1`/`XRCS`, `RA15/16`, `RENK`, `SDBD`, …) matches the [VA1] reading
  throughout, validating the print interpretation.

## Review status

- The **large custom LSI** section (§1) has been reviewed and confirmed
  correct.
- **SGP debate settled**: the VA1 "VDP" (IC75, `D92017-002`) is the SGP,
  implemented as a single chip; GAL-3 corrected to `D65200GD-054` and
  identified as the GVRAM sequencer. See §1a.
- IC82 marking corrected after review to `SLA6050C0R` (vendor identification
  still open, see §9).
- `D65101GD-055` (GAL-2) and IDP = `μPD72022` were confirmed via [VA2].

## Page map of the I/O schematic ([VA1], pp. 241–264)

| Page | Title (ja) | Content |
|------|------------|---------|
| 241 | 標準RAM256Kバイト | Main DRAM, 256 KB bank |
| 242 | メインROM, 辞書ROM | Main ROM / dictionary ROM |
| 243 | GDC, FDDインターフェイス・モード切り替え | IDP + FDD interface mode switch |
| 244 | ディスク・サブシステム | Z80 disk subsystem |
| 245 | CPUクロック, FDDコネクタ周辺 | Sub-system clocks, FDD connectors |
| 246 | FM音源, 拡張用インターフェイス | FM sound, expansion interface |
| 247 | プリンタ・マウスインターフェイス, ディスク・サブシステムコネクタ | Printer / mouse I/F, subsystem connector |
| 248 | CPU割り込み周辺 | CPU and interrupt logic |
| 249 | GAL-1 | Gate array 1 (memory/bus controller) |
| 250 | PC-88V1/V2モード・エミュレータ | PC-88 V1/V2 mode emulator LSI |
| 251 | RS-232Cスキャナ・インターフェイス | RS-232C and scanner interface |
| 252 | GAL-2 | Gate array 2 (video output mixer) |
| 253 | GAL-3 | Gate array 3 (GVRAM sequencer) |
| 254 | IDPアトリビュート・コントロール | IDP attribute control gate arrays |
| 255 | 漢字ROM, キャラクタ・ジェネレータRAM | Kanji ROM, CG RAM |
| 256 | テキストVRAM | Text VRAM |
| 257 | VDP | SGP (Super Graphic Processor, §1a) |
| 258–259 | グラフィックVRAM-1/2 | Graphics VRAM (dual-port) |
| 260 | 拡張バス, ビデオボード・コネクタ | Expansion bus, video board connector |
| 261 | アナログRGB周辺 | Analog RGB output |
| 262 | コンポジット, オーディオ出力, カレンダ・クロック | Composite, audio, RTC |
| 263 | デジタルRGB, ディスプレイクロック・ジェネレータ | Digital RGB, display clock generator |
| 264 | メインボード・ディスクサブシステム・コネクタ | Board-to-board connectors |

## 1. Large custom LSIs ([VA1]) — reviewed, confirmed

| Ref | Marking (as read) | Page | Function |
|-----|-------------------|------|----------|
| IC83 | `D9602` (misprint/misread of **μPD9002**) | 248 | Main CPU. 15.9744 MHz crystal ÷ 2 = 7.9872 MHz. Pin set (`BS0–2`, multiplexed `A16/PS0`-style status, `TOUT`, `BRATE`, 4-ch `DMARQ/DMAAK`, multiple `INTP`, `BUSLOCK`, `BUFEN`) indicates V50-like integrated peripherals (timer, serial baud, DMA, ICU) on a V30-compatible core with the documented Z80 mode. |
| IC79 | `D72022G` — **μPD72022, confirmed by [VA2]** (`D72022GF` in `Design1_text_sub.sch`) | 243 | **IDP** — text / attribute / sprite display processor. 8-bit AD bus, raster address `RA0–4`, `VAD0–15`, light-pen input. The page title says "GDC" but the device is the IDP. |
| IC75 | `D92017-002` | 257 | **SGP** (Super Graphic Processor; the magazine labels the page "VDP"). CPU-bus slave (`AD0–15`, `BS0–2`), main-memory bus master (`XMADH`/`XMADL`, `MHDIR`), GVRAM bus master (`GA00–17`, `XGRAS`/`XGCAS` ×2, `XGWE0–3`, `XDTOE0–3`, `XCSW0–7`), drawing/vertical interrupt (`VINT`). Same NEC "9xxx" custom family as the CPU. See §1a. |
| IC78 | `PCZ80-27`? (reading difficult) | 250 | **PC-88 V1/V2 mode emulator.** Synthesizes PC-8801 I/O semantics: keyboard DMA (`KDMAK`/`XKOMPD`), `VRTC`, PSG-side signals (`XPSG*`), printer (`XPRINT`/`PBUSY`), mode straps `X88V1`/`X88V2`, `KIFDSL`. Z80 *instruction* execution itself is inside the μPD9002; this LSI is the peripheral-level compatibility layer. |

### 1a. SGP identification (settled)

The PC-88VA's blitter — the **SGP** (スーパーグラフィックプロセッサ /
Super Graphic Processor), whose part number is listed as unknown in MAME —
is implemented **as a single chip on the VA1**: IC75, `D92017-002`, the
device on the page the magazine titles "VDP". The identification rests on
the pin profile: a CPU-bus slave interface plus *two* bus-master interfaces
— main memory via `XMADH`/`XMADL`/`MHDIR`, and GVRAM via
`GA00–17`/`XCSW0–7` — plus `VINT`. This matches the SGP as emulated in
MAME (22-bit address space, 16-bit data, BITBLT into main RAM, interrupt
level 8); the `XMADH`/`XMADL`/`MHDIR` main-memory DMA path is the physical
backing for the 22-bit reachability.

GAL-3 (`D65200GD-054`, see §2) is a separate device on the same schematic
and is the **GVRAM sequencer** (display-refresh side). Both the SGP and
GAL-3 drive `GA*`/`XCSW*` because the GVRAM address/select bus is
**shared**: drawing cycles (SGP) interleave with display-refresh cycles
(GAL-3). The same shared-bus structure carries over to the VA2, where
address generation consequently appears on more than one device of the
`D92044`/`D92046` pair.

- MAME SGP device (part number listed as unknown there):
  <https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_sgp.cpp>

## 2. NEC gate arrays, μPD65xxx family ([VA1])

Naming convention (inferred): the numeric part scales with the gate count of
the master slice, `-0xx` is the metallization (option) code, `GD`/`GF`
distinguish packages. The GAL-3 marking was initially misread as
`D65030GD-054`; on review it is **`D65200GD-054`**, consistent with the
VA2's `D65200GD-076` being a later option of the same master.

| Ref | Marking | Page / block | Function |
|-----|---------|--------------|----------|
| IC77 | `D65042GD-093` | 249, "GAL-1" | Memory/bus controller: address decode, DRAM RAS/CAS/refresh (`XRFSH`), `ALE`/`XMW`/`XMR`/`XWAH`/`XWAL`, ROM banking (`RBNK`/`RENK`), `XCAS0–3`. |
| IC74 | `D65101GD-055` (**65101 confirmed by [VA2]**) | 252, "GAL-2" | Video output stage: merges graphics serial data `GVD00–77` (64 lines), text `TXVD0–3` and external video `EXVD0–15`; priority/palette; outputs 6-bit `VR`/`VG`/`VB` and sync. |
| IC76 | `D65200GD-054` (corrected; earlier misread as 65030) | 253, "GAL-3" | **GVRAM sequencer** (settled, §1a): display-refresh side of the shared GVRAM bus — dual-port VRAM serial clocks `SCLK0–7`, RAS/CAS/WE/DT-OE generation, refresh-cycle interleave against SGP drawing cycles. |
| IC1 | `D65030GF-067` | 245 | Z80 subsystem glue: sub-DRAM address mux (`ZMA4–7`, `XZRAS`/`XZCAS`/`XZDOE`), clock distribution, FDD connector signals. |
| IC14 | `D65013GF-124` | 243 | **FDD interface mode switch**: routes between `ZD` (Z80 sub) / `FD` (FDC) / `UD` / `PD0–15` buses, implementing the VA's two FDD access modes (direct FDC vs. 8801-compatible sub-CPU). A key device for emulator implementation. |
| IC80 | `D65019GD-084` | 254 | IDP attribute control A: text-VRAM control (`XVRAS`/`XVCAS`/`XVWH`/`XVWL`), `TAB`/`TDB` buses, CG/kanji-ROM address generation (`CGA0–11`). |
| IC81 | `D65012GF-042` | 254 | IDP attribute control B: receives `VAD0–15`, raster `RA1–4`, `TD`/`CDB` shifter and font fetch pipeline. |
| IC82 | `SLA6050C0R` (corrected reading; vendor open, §9) | 263 | Keyboard interrupt/control glue as read (`KD0–3`, `KBINT`/`NKBINT`/`XKBINT`, `XKOMRD`, `KALE`), 24 pins. |

Note: the magazine's block names "GAL-1/2/3" denote gate-array blocks, not
Lattice GAL devices.

## 3. Standard (well-known) LSIs ([VA1])

| Part | Function | Notes |
|------|----------|-------|
| D780C-1 | Z80A, 4 MHz | Disk-subsystem CPU (PC-8801-compatible architecture carried over) |
| D765A | Floppy disk controller | |
| D71066CT? | FDD VFO / data separator + write precompensation | `RGATE`/`MFM`/`RCLK`/`FDCCLK`/`MIN-STD` pins; 2DD/2HD dual mode |
| D8251AF | USART (RS-232C) | 75188 / 75189A level shifters, DIP switch settings |
| D8255AC-5 ×2 | PPI | IC20 = scanner interface (CN9, all PA/PB/PC brought out); IC19 = internal ports (`CRTMD`, status readback) |
| D8259A | Interrupt controller | Aggregates `MSINT` (mouse), `FDDINT`, VDP-side interrupts, etc. |
| D7811-family (D78C11?), mask `CW-655` | Keyboard-interface MCU | 8-bit MCU with on-chip A/D; 3-wire serial keyboard connector |
| YM2203C + YM3014 | OPN FM synthesis + serial DAC | 3 FM + 3 SSG channels |
| MC1377 | RGB → NTSC encoder | Composite output; 74HC74 in the subcarrier path |
| D4990C (μPD4990A) | Calendar clock (RTC) | 32.768 kHz crystal, battery backup shared with `MSWCS`/`BACKUP` |
| D4584 | Hex Schmitt trigger | Reset generation |
| 7406 ×5 / 7407 ×6 | Open-collector drivers | FDD connectors CN1/CN2 |
| 74F157 ×2 | Main-DRAM row/column address mux | With 47 Ω ×4 damping resistor packs |

Plus extensive 74LS/ALS/F/S glue (ALS244/245, LS373, ALS374, LS273,
LS164/161, LS157, LS75, LS14, ALS32/08/04/10/00, F241, S74/S00) and
DSS-271M EMI filter arrays on keyboard, digital RGB and audio lines.

## 4. Memory devices ([VA1])

| Device | Organization | Capacity / role |
|--------|--------------|-----------------|
| D41464C-15 ×8 | 64K×4 DRAM, 150 ns | **Main RAM, 256 KB** (16-bit ×2 banks, `XCAS0/1`). The machine spec is 512 KB; the published page is titled "standard RAM 256 KB", so the second 256 KB is presumably outside the published pages (see §9). |
| D41416C-12 ×8 | 16K×4 DRAM, 120 ns | **Text/IDP VRAM, 64 KB** (16-bit ×2 banks). Holds text, attributes and sprite patterns. |
| VRAM1–8 (no marking printed) | 64K×4 dual-port | **GVRAM, 256 KB.** Pinout (`SC`, `SIO0–3`, `DT/OE`, `SE`) identifies μPD41264. |
| HN62402 ×3 | 128K×16 mask ROM (`A0–A16`, `O0–O15`) | 768 KB total: system ROM (selected by `XRDS`/`XRE0`) and dictionary ROM (`XJDS`/`XJE0`; J = *jisho*), banked via `RA15/16` + `RBNK`. Exact split between the three devices not resolved from print. |
| D23C2000 (printed `D24C2000`?) | 256K×8 mask ROM | Kanji ROM, JIS level 1+2 (16×16 font ≈ 224 KB, consistent) |
| HN623256 | 32K×8 mask ROM | Secondary font ROM (ANK/half-width, inferred) |
| D4364C-15LL | 8K×8 SRAM | CG (PCG) RAM doubling as memory-switch store; battery-backed (`MSWCS`/`BACKUP`) |
| D2364-family (printed `D20364E`?) | 8K×8 ROM | Sub-CPU program ROM |
| D41416 ×2 | 16K×4 DRAM | Sub-side RAM, 16 KB |

## 5. Analog and clocks ([VA1])

- **D6901C ×3** — 6-bit video DACs (R/G/B), 2SC2785/2SA1175 buffers, 75 Ω
  back-termination, LC filters, to the analog RGB connector (CN4). `YS`/`AVC`
  lines indicate superimpose support was designed in.
- Audio stages: `C2002`? (μPC2002 power amp, inferred), `C1408HA`?, `IR9403`?
  — readings unconfirmed. YM3014 sample/hold buffer and output amplifier.
- Oscillators: 15.9744 MHz (CPU, ÷2 = 7.9872 MHz); 16 MHz? (sub system /
  FDC VFO); 19.2 MHz? (reading unconfirmed, serial side?); OSC1 ≈ 42.1 MHz?
  and OSC2 28.6363 MHz (÷2 ≈ 21 M / 14.318 M dot clocks → 24 kHz / 15 kHz
  display modes); 32.768 kHz (RTC).

## 6. Signal naming conventions

Prefixes readable from the schematic: `X` = active low, `Z` = Z80 subsystem
domain, `G` = graphics, `T` = text, `K` = keyboard, `J` = dictionary.

## 7. Architecture summary

The design is explicitly dual-plane: the μPD9002 main system and an
8801-compatible Z80 subsystem (FDD) are joined by the IC14 mode-switch gate
array. Video is likewise dual-path — the TSP/IDP side (text, attributes,
sprites: μPD72022 plus the attribute gate arrays) and the SGP side (bitmap
drawing into GVRAM and main RAM) — merged in GAL-2 and fed to three 6-bit
DACs. `EXVD0–15` runs from the
expansion/video-board connector straight into the video mixer, i.e. external
video injection (video art board class, superimpose) was designed in from
the start. Four large customs plus 7–8 gate arrays is an unusually heavy
custom-silicon budget for a 1987 consumer machine and is consistent with the
VA's cost structure.

### 7a. Functional units vs. silicon (VA1)

| VA architecture term | Silicon |
|----------------------|---------|
| **SGP** (Super Graphic Processor — blitter, BITBLT into GVRAM/main RAM) | IC75 `D92017-002` (§1a) |
| **TSP** (text screen processor — text strips, attributes, sprites) | Register domain of IC79 μPD72022 (IDP), with IC80/IC81 attribute gate arrays; documented in the 1990 V-Series data book (§7b) |
| CPU (V3 native / V1–V2 Z80 mode) | IC83 μPD9002 |

### 7b. External references

- VAEG SGP implementation: [`io/sgp.c`](../../io/sgp.c) and
  [`io/sgp.h`](../../io/sgp.h)
- PC-88VA Technical Manual (てくまに):
  <http://www.iris.dti.ne.jp/~nano/88va/tekumani.html>
- Inside PC-88VA Wiki — グラフィック:
  <http://www.pc88.gr.jp/inside88va/wiki/index.php?%A5%B0%A5%E9%A5%D5%A5%A3%A5%C3%A5%AF>
- MAME SGP device: <https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_sgp.cpp>
- MAME PC-88VA video (TSP/IDP handling): <https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va_v.cpp>
- MAME PC-88VA driver: <https://github.com/mamedev/mame/blob/master/src/mame/nec/pc88va.cpp>
- **TSP (μPD72022)** — documented in the *1990 NEC 16-bit V-Series
  Microprocessor Data Book*:
  <https://bitsavers.trailing-edge.com/components/nec/_dataBooks/1990_NEC_16-bit_V-Series_Microprocessor_Data_Book.pdf>
- NEC V30 (μPD70116) datasheet: <https://datasheets.chipdb.org/NEC/V20-V30/NEC_uPD70116.pdf>
- NEC V20 (μPD70108) datasheet: <https://datasheets.chipdb.org/NEC/V20-V30/NEC_uPD70108.pdf>
- NEC 16-bit V-Series Microprocessor Data Book, 1991 (V20/V30/V40/V50 —
  peripheral context for the μPD9002):
  <https://bitsavers.trailing-edge.com/components/nec/_dataBooks/1991_16_bit_V-Series_Microprocessor_Data_Book.pdf>

## 8. VA1 → VA2 differences (from [VA2])

| Item | VA1 ([VA1]) | VA2 ([VA2]) | Remark |
|------|-------------|-------------|--------|
| GAL master D65042 | `D65042GD-093` | `D65042GD-246` | Same master, revised metal option |
| D65101 usage | `D65101GD-055` (GAL-2) | `D65101GD-055` **+** `D65101GD-110` (main) **+** `D65101GD-107` (text sub) | Smaller VA1 arrays apparently consolidated onto the 65101 master |
| GAL master D65200 | `D65200GD-054` (GAL-3) | `D65200GD-076` | Same master, revised option; GVRAM sequencer (settled, §1a) |
| "9xxx" custom graphics | `D92017-002` — **SGP, single chip** | `D92044GD-001` + `D92046GD-001` | SGP function carried by the 92-family on VA2 as a two-chip arrangement; because the GVRAM address/select bus is shared (drawing vs. refresh cycles), address generation appearing on more than one device is expected and does not by itself locate the drawing engine |
| IDP | `D72022G` (read) | `D72022GF` (extracted) | **μPD72022 confirmed** |
| Sound | YM2203C (OPN) | **YM2608B (OPNA)** | VA2/VA3 sound upgrade |
| GVRAM | μPD41264 ×8 (64K×4 dual-port) | `D42232` ×8 | Device change, same 256 KB total |
| Main DRAM | D41464C-15, DIP | `D41464V`, ZIP (ZIP-20 footprints) | Packaging change |
| NDP | (not read from print) | `8087-1`, DIP-40 | Corroborates the documented optional 8087 coprocessor |
| CG/memory-switch SRAM | D4364C-15LL | `D4364C-12` | Same device family |

## 9. Open questions

1. **IC82 = `SLA6050C0R`.** The only publicly documented SLA6050 is a Sanken
   power Darlington array, which is functionally inconsistent with the logic
   signals attributed to IC82 on p. 263 (`SA7–9`, `KD0–3`, `KBINT` group).
   Either the pin attribution in the print reading is wrong (dense page,
   adjacent-block bleed) or the marking collides with another vendor's small
   logic device. Package type and vendor logo on actual hardware would settle
   it. The string does not appear in the extracted [VA2] data.
2. **Main RAM.** p. 241 shows one 256 KB bank against the 512 KB machine
   spec; locate the second bank (unpublished page or separate sheet).
3. **ROM split.** Allocation of the three HN62402 devices between system
   (`XR*`) and dictionary (`XJ*`) selects.
4. **Audio-stage part numbers** (`C2002`?, `C1408HA`?, `IR9403`?).
5. **[VA2] netlist.** Pin-level connectivity not yet reconstructed from the
   DesignSpark files; export via DesignSpark PCB or write an MFC
   deserializer.
6. **VA2 92-family split.** Division of labor between `D92044` and `D92046`
   (drawing engine vs. display/sequencer side) to be confirmed from the
   [VA2] netlist; shared-bus visibility (§1a) means pin-level tracing, not
   bus presence, is the discriminator.

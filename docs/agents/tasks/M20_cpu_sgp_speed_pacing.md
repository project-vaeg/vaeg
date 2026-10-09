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
# M20 - Separate CPU/SGP execution speed from machine time

Status: complete; G20 passed

Branch: `topic/m20-cpu-sgp-speed-pacing`

Gate: G20 human

Depends on: G19 passed.

## Initial worktree

Work began from `main` at `fbd0345`. The pre-existing untracked research
material was left untouched throughout M20:

```text
## main...origin/main
?? docs/modernization/PCEPAT.DOC
?? docs/tekumani/
```

## Scope

The active SDL2/Dear ImGui frontend gains:

- a transactional Configure dialog for V30 execution multiplier x1-x32;
- independent SGP Model default, Follow CPU, and Custom x1-x16 modes;
- Screen-menu No Wait and frame-skip controls;
- hold-F11 fast-forward, implemented as temporary host-pacing removal.

CPU and SGP settings alter execution capacity per unit of emulated machine
time. They must not alter display scan timing, TSP timing, sound clocks, FDD
timing, RTC/calendar time, communication timing, or the normal host-time
pacing rate. Only No Wait and F11 permit machine time to advance faster than
host time.

The milestone number in the original request was M19. M19 already exists as
the completed portable-runtime milestone, so this work is M20 and does not
replace `M19_portable_runtime.md`.

## Non-goals

- Do not guess SGP frequencies beyond the documented VA 4 MHz and VA2/VA3
  8 MHz model distinction.
- Do not rewrite the SGP command-cost table.
- Do not redesign the existing approximate CPU/SGP bus-contention model.
- Do not change guest framebuffer, renderer, audio backend, OPN hardware, or
  keyboard mapping.
- Do not modify `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, or `hlp/`.
- Do not make F11 persistent or send it to the guest keyboard.

## Existing behavior

Before M20, `np2cfg.multiple` becomes both `pccore.multiple` and part of
`pccore.realclock`. The same clock therefore controls CPU instruction budget,
nevent time, display/TSP frame time, sound timers, serial transfer periods,
FDC mechanics, I/O waits, and the FDD subsystem Z80 compensation. Raising
`clk_mult` accelerates the entire machine clock rather than only V30 work.

`sgp_step()` measures elapsed time using
`CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK`. Its command work therefore shares
that same global clock. The SDL2 outer loop separately paces complete guest
frames against host milliseconds through `timing_getcount()`.

## Clock-domain audit

The table groups repeated references with the same meaning. Individual
instruction tables contain many `I286_WORKCLOCK()` calls, and the device
files contain many reads of the same machine-clock expression; their common
boundaries are the important classification points.

| File | Function/site | Symbol | Current meaning | Required domain | Required change | Verification |
|---|---|---|---|---|---|---|
| `pccore.c` | `pccore_set` | `np2cfg.baseclock`, `np2cfg.multiple` | Selects base and global multiplier | configuration | Fix base to 3993600 in GUI; validate CPU x1-x32 | selftest + config gate |
| `pccore.c` | `pccore_set` | `pccore.multiple`, `pccore.realclock` | Global CPU/device clock | machine/peripheral | Normalize to standard x2 and 7987200 Hz | invariant selftest |
| `i286c/i286c.mcr` | `I286_WORKCLOCK` | `I286_REMCLOCK` | Instruction-cycle consumption | CPU execution | Convert V30/i286 cycles by ratio `2 / clk_mult` with remainder | ratio and CPU-budget tests |
| `i286c/*.c` | instruction handlers | `I286_WORKCLOCK` | Per-instruction native cycle cost | CPU execution | Keep costs; scale only at common macro | x1/x2/x3/x4/x32 tests |
| `i286c/memory.c`, `cpucva/gvramva.c` | memory access | `CPU_REMCLOCK -= wait` | VRAM/TRAM/GRCG/bus wait | bus/contention | Keep in machine ticks; do not CPU-scale | code audit + boot gate |
| `io/iocore.c`, `iova/iocoreva.c` | I/O dispatch | `pccore.multiple`, `CPU_REMCLOCK` | I/O bus wait | bus/contention | Standard x2 machine multiplier | invariant selftest |
| `nevent.c` | event queue | `CPU_CLOCK`, `CPU_BASECLOCK`, `CPU_REMCLOCK` | Canonical event timeline | machine/peripheral | Keep as machine ticks | event-period tests |
| `nevent.c` | `nevent_setbyms` | `pccore.realclock` | ms to event ticks | machine/peripheral | Use fixed nominal machine clock | invariant selftest |
| `io/gdc.c`, `io/gdc_sub.c` | GDC timing | base/multiple/event clocks | scan and frame periods | machine/peripheral | Standard x2, independent of CPU/SGP | VBlank tests + human gate |
| `iova/tsp.c` | `tsp_update`, display events | base/multiple/event clocks | VA scan/TSP frame periods | machine/peripheral | Standard x2 | VBlank/TSP gate |
| `sound/*.c`, `cbus/board14.c` | sound setup/sync | realclock/multiple/current clock | chip timer, sample and BEEP time | machine/peripheral | Standard x2 | pitch/timer gate |
| `sound/fmtimer.c` | FM timer setup | `pccore.multiple` | FM timer event scale | machine/peripheral | Standard x2 | timer regression |
| `io/pit.c`, `io/pic.c` | PIT/PIC | realclock/multiple | timer and IRQ periods | machine/peripheral | Standard x2 | timer regression |
| `io/fdc.c`, `fdd/fdd_mtr.c` | FDC mechanics | realclock/multiple/current clock | rotation, byte, head and motor time | machine/peripheral | Standard x2 | disk timing gate |
| `iova/subsystem.cpp` | `ClockCounter::SetMultiple` | `pccore.multiple` | Main/FDD-Z80 clock conversion | machine/peripheral | Keep standard x2; avoid double compensation | x1/x2/x4/x8 disk gate |
| `io/serial.c`, `cbus/pc9861k.c`, `cbus/mpu98ii.c` | communication setup | `pccore.realclock` | serial/MIDI transfer time | machine/peripheral | Standard nominal clock | regression gate |
| `sound/beepc.c`, `sound/sound.c` | stream synchronization | current CPU clock expression | elapsed emulated time | machine/peripheral | Keep canonical machine timeline | audio gate |
| `iova/boardsb2.c` | access wait | realclock/current clock | chip I/O wait | bus/contention | Standard machine ticks | OPN regression |
| `iova/sgp.c` | `sgp_step` | current clock, `lastclock` | Supplies machine elapsed ticks as SGP budget | SGP execution boundary | Scale elapsed budget by selected SGP ratio with remainder | budget-ratio tests |
| `iova/sgp.c` | command functions | `sgp.remainclock -= cost` | SGP native command costs | SGP execution | Keep costs unchanged | code audit + human completion gate |
| `iova/sgp.c` | memory helpers | `CPU_REMCLOCK -= 4` | Approximate CPU wait for SGP memory access | bus/contention | Do not multiply by SGP speed | audit + corruption gate |
| `sdl2/timing.c` | `timing_getcount` | host milliseconds/frame rate | Host-to-guest pacing | host pacing | Keep independent from CPU/SGP scale | pacing tests |
| `sdl2/np2.c` | `runloop` | `NOWAIT`, `DRAW_SKIP` | Pacing and presentation policy | host pacing | Use per-iteration effective values | selftest + manual gate |
| `sdl2/taskmng.c` | SDL event pump | keyboard events | Frontend shortcut/input routing | host pacing/input | Consume F11 before guest forwarding | event tests |
| `sdl2/ini.c` | INI table | `clk_base`, `clk_mult` | CPU configuration persistence | configuration | Preserve keys; add `sgp_mode`, `sgp_mult` | round-trip tests |
| `statsave.tbl` | `PCCORE`, `SGP` sections | persisted runtime structs | Raw state-file layout | persistence | Do not resize either struct | size/static assertions + statsave test |

## Adopted clock-domain design

### Machine time

`pccore.baseclock` remains 3993600 Hz for VA. `pccore.multiple` becomes the
nominal machine multiplier and remains 2. `pccore.realclock` consequently
means nominal machine-clock Hz and remains 7987200. Existing peripheral code
continues to consume the same machine ticks as the pre-M20 standard x2 setup.

The raw persisted `PCCORE` layout is not extended. CPU execution-ratio state
is separate runtime state and is rebuilt from `np2cfg.multiple` after reset
or state load.

### CPU execution scaling

`np2cfg.multiple` remains the persisted x1-x32 V30 setting. The common
`I286_WORKCLOCK()` boundary converts native instruction cycles into machine
ticks using numerator 2 and denominator `np2cfg.multiple`. A 64-bit remainder
accumulator preserves fractional ticks. Direct memory, I/O, DMA, and bus-wait
deductions are not scaled as instruction execution.

### SGP execution scaling

`sgp_step()` remains driven by elapsed machine ticks. Before adding them to
the SGP command budget, a second ratio accumulator applies:

- Model default: VA 1/1, VA2/VA3 2/1;
- Follow CPU: `np2cfg.multiple / 2`;
- Custom: `sgp_mult / 1`.

The existing command costs, including historical `* 2` terms, remain
unchanged. The VA model default preserves the established 3.9936 MHz timing;
VA2/VA3 applies a 2/1 model factor for 7.9872 MHz. These nominal emulator
values implement the 4 MHz (VA) and 8 MHz (VA2/VA3) distinction documented by
the [Inside PC-88VA Wiki, section 4.4.6 SGP](http://www.pc88.gr.jp/inside88va/wiki/index.php?%A5%B0%A5%E9%A5%D5%A5%A3%A5%C3%A5%AF),
attributed there to Shinra. Follow CPU and Custom remain relative to each
model's default.

The V30/uPD9002 CPU model default remains 7.9872 MHz for VA, VA2, and VA3.
Only the SGP model-default execution clock has the 4 MHz versus 8 MHz model
distinction.

### Bus contention limitation

The SGP memory helpers currently deduct fixed CPU waits and explicitly label
the model approximate. M20 preserves those waits as machine-domain bus costs
and does not multiply them with SGP speed. A faster SGP can perform more
memory operations per machine time and may therefore cause more aggregate
wait naturally, but no second explicit speed multiplier is applied. Precise
simultaneous CPU/SGP/display contention remains a later hardware-research
task.

### Host pacing

Normal pacing remains one guest-machine second per host second. No Wait
removes host waiting without modifying stored clock settings. Hold-F11 uses
effective No Wait plus draw skip 16 only while held; it does not change CPU,
SGP, or peripheral ratios.

## Configuration and UI

The Configure modal uses pending values. `clk_base` is displayed read-only as
3.9936 MHz. CPU multiplier has an always-visible numeric input accepting
x1-x32, plus presets 1, 2, 4, 5, 6, 8, 10, 12, 16, and 20. SGP provides Model
default, Follow CPU, and Custom; Custom has an always-visible numeric input
accepting x1-x16 plus presets 1, 2, 4, 8, and 16.

Only valid OK commits settings, raises `SYS_UPDATECFG | SYS_UPDATECLOCK`, and
uses the existing media-preserving guest reset. Cancel, Escape, close, and
shutdown discard pending edits. An unchanged OK does not reset.

The existing `clk_base` and `clk_mult` keys remain. New keys are:

```text
sgp_mode=0|1|2
sgp_mult=1..16
```

Missing or invalid SGP values select Model default and x1.

## No Wait, frame skip, and F11

Screen exposes No Wait and Auto/Full/1/2/1/3/1/4 frame-skip values mapped to
the existing `s_NOWAIT` and `SkpFrame` settings. They take effect immediately
and raise `SYS_UPDATEOSCFG`.

F11 is a frontend-global hold shortcut. Non-repeat keydown sets the transient
state; keyup clears it. Focus loss, reset, state load, quit, and shutdown also
clear it. Both F11 edges are consumed and never reach the guest, including
when ImGui captures keyboard input. Right Alt behavior is unchanged.

## G20 sound-menu correction

Human testing found that the former Sound on/off control cleared `SNDboard`,
which removed the selected OPN/OPNA hardware, cleared both hardware checks,
and could stall VA software polling the FM timer. Sound on/off now persists a
separate `sound_enabled` host-audio setting and only pauses the SDL audio
device. `SNDboard` remains the selected guest hardware (`100` or `200`) while
muted. An old or invalid `SNDboard=0` is replaced at load time with the model
default and produces a warning.

## Automated verification

- ratio validation, fractional carry, long-run totals, reset, and overflow;
- CPU x1/x2/x3/x4/x8/x32 budgets and rejection of 0/33;
- SGP mode/custom validation and Model/Follow/Custom ratios;
- nominal machine clock and peripheral-scale invariants;
- F11 event/repeat/focus/reset behavior and effective pacing values;
- existing statsave zeroed round-trip, keyboard, OPN, smoke, and screen
  detector tests;
- gcc, clang, ASan/UBSan, and MinGW cross builds;
- encoding, EOL, case, diff, and frozen-tier checks.

The ROM-less SGP selftest verifies validation, Model/Follow/Custom budget
ratios, fractional carry, and long-run totals at the common `sgp_step()`
boundary. It does not claim hardware verification of a particular command's
busy duration or interrupt edge; those remain in G20.

### Agent results

The following commands passed on the Linux development host:

```text
cmake --preset linux-debug
cmake --build --preset linux-debug --target vaeg_sdl2
SDL_AUDIODRIVER=dummy ./build/linux-debug/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-debug/sdl2/vaeg --smoke

cmake --preset linux-release
cmake --build --preset linux-release
SDL_AUDIODRIVER=dummy ./build/linux-release/sdl2/vaeg --selftest
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/linux-release/sdl2/vaeg --smoke

cmake --preset linux-ci-gcc
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure

cmake --preset linux-ci-clang
cmake --build --preset linux-ci-clang
ctest --test-dir build/linux-ci-clang --output-on-failure

cmake --preset linux-ci-asan
cmake --build --preset linux-ci-asan
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan \
  --output-on-failure
ASAN_OPTIONS=detect_leaks=0 SDL_AUDIODRIVER=dummy \
  ./build/linux-ci-asan/sdl2/vaeg --selftest
ASAN_OPTIONS=detect_leaks=0 SDL_VIDEODRIVER=dummy \
  SDL_AUDIODRIVER=dummy ./build/linux-ci-asan/sdl2/vaeg --smoke

cmake --preset mingw-cross
cmake --build --preset mingw-cross

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
git diff main...HEAD --stat -- win9x i286x cpuxva/memoryva.x86 hlp
```

Both GCC and Clang CTest runs reported one of one ROM-less test passed. The
release and sanitizer selftests reported all tests passed, and ROM-less smoke
completed using the SDL software renderer. ASan reported no memory errors.
UBSan emitted only the four pre-existing shared-core findings recorded in
`docs/agents/reports/ubsan_backlog.md`; none of those files changed in M20.

The plain `linux-debug` preset does not enable CTest registration, so `ctest
--test-dir build/linux-debug` correctly reports no tests. The CI presets are
the CTest-bearing configurations. A plain sanitizer CTest invocation also
cannot use LeakSanitizer under this sandbox's ptrace environment; the accepted
run used `ASAN_OPTIONS=detect_leaks=0` as documented above.

`macos-release` was not run: this agent host is Linux and has no Darwin SDK or
native macOS toolchain. Running that preset here would produce another Linux
binary and would not constitute macOS verification. Native macOS build and
runtime coverage therefore remains part of G20.

## Gate G20

G20 is a human timing and regression gate. It must cover:

- Configure transaction behavior, validation, CPU x1/x2/x3/x4/x32 display,
  reset, and persistence;
- SGP Model default, Follow CPU, Custom x1/x2/x4/x8/x16, reset, and
  persistence;
- unchanged VBlank/TSP, sound pitch/timer, FDD rotation, and RTC rates across
  CPU x2/x4/x8 and SGP Model/Custom x2/x4;
- CPU benchmark scaling without peripheral-time scaling;
- SGP completion/busy scaling without CPU benchmark or scan-time scaling;
- No Wait and all five frame-skip choices with persistence;
- F11 hold/release/repeat/focus-loss behavior, responsive GUI/audio, and no
  modification of stored pacing or execution settings;
- unchanged Right Alt guest behavior;
- V3 boot, VA demo, OS boot/simple operation, media retention, clean exit,
  and no graphics or sound regression.

The maintainer reported G20 passed on 2026-07-11.

## Remaining hardware uncertainty

The model clock distinction is recorded as VA 4 MHz and VA2/VA3 8 MHz by the
Inside PC-88VA Wiki source cited above and is implemented as 3.9936 MHz and
7.9872 MHz nominal clocks. Exact CPU/SGP/display bus arbitration and the
derivation of individual command costs remain unverified. Exact contention
still requires technical-manual or hardware evidence in a later milestone.

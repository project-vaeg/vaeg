# M9 — VA subsystem on the portable core (memoryva C port)

Status: not-started
Branch: topic/m9-va-portable
Gate: G9 (human: FULL standard VA gate — V3 boot, VA demo, OS ops, on Linux)
Depends on: G8 passed. May run in parallel with M10 (separate session).

## Goal

The VA machine builds and runs in the portable build. This requires the
single missing piece: a C implementation of the VA memory layer.

## The hard part, stated precisely

`cpuxva/memoryva.x86` (1,256 lines NASM) is the ONLY implementation of
the interface in `cpuxva/memoryva.h`. It is included by 10+ files across
`iova/` and `vramva/` (tsp.c, cgromva.c, sgp.c, bkupmemva.c,
memctrlva.c, fdsubsys.c, videova.c, maketextva.c, makesprva.c, ...).
The LEGACY build pairs it with the `i286x` assembly CPU core; the
portable build must pair a new `memoryva.c` with `i286c`.

Precedent inside this repo: `i286c/memory.c`, `i286c/dmap.c`,
`i286c/egcmem.c` are exactly this kind of C counterpart to
`i286x/{memory,dmap,egcmem}.x86`. Study those PAIRS first — they define
the house style for asm→C transliteration (naming, mem-access macros,
how the C core's calling conventions replace the register protocol).

## Strategy (default; deviations need an ADR)

Faithful transliteration, not re-derivation:

1. Produce a routine inventory of memoryva.x86: every exported symbol,
   its register-level contract, and its C-callable equivalent per
   memoryva.h. Write to docs/agents/reports/m9_memoryva_map.md.
2. Transliterate routine by routine into cpucva/memoryva.c
   (original copyright header retained + ported-by line, per
   CONVENTIONS). Keep the memory map tables bit-identical.
3. Wire the VA directories into CMake: vaeg_va target
   (iova/ vramva/ biosva/ cpucva/ incl. z80c.cpp, gvramva.c) linked
   into vaeg_sdl2; machine selection VA vs PC-98 as it is done in the
   LEGACY build (find and mirror the switch, do not invent one).
4. Verify no OPNGENX86 / i286x path is reachable in the portable build.

## Constraints

- iova/ and vramva/ sources should compile unmodified. If one needs a
  change to build on gcc/clang, it goes in its own commit with a
  one-line justification; behavior changes are forbidden.
- The LEGACY v141 build must remain green: memoryva.x86 stays, the .dsp
  is untouched, memoryva.c is only in the CMake build.
- Endianness: the x86 asm assumes little-endian byte access into shared
  structures. memoryva.c must go through the same access macros the
  i286c core uses (LOADINTELWORD etc.), not raw casts, or big-endian
  correctness dies silently. Flag every place this matters.

## Machine checks (paste into PR)

    cmake build of vaeg_sdl2 with VA enabled, gcc + clang
    SDL_VIDEODRIVER=dummy ./vaeg --smoke   (VA machine selected)
    tools/repo checkers

## Gate G9 (human, Linux) — the standard VA gate

1. Clean checkout, build.
2. Boot in V3 mode.
3. Run the bundled VA demo.
4. Boot an OS; DIR; launch a program.
Behavior reference on any anomaly: the same media on the LEGACY v141
build on Windows.

## Do not do

- Do not "optimize" while transliterating. Same tables, same order,
  same masks. Cleverness here is a defect.
- Do not modify i286c/ beyond what wiring strictly requires.
- Do not touch memoryva.x86, i286x/, or the LEGACY project files.

## Risks to report

- Any memoryva routine whose register contract has no clean C
  equivalent (self-modifying code, computed jumps into tables).
- Divergence between memoryva.h prototypes and what the asm actually
  exports.
- Places where i286c's memory hooks and memoryva's expectations
  disagree (the i286x pairing may rely on asm-side dispatch).

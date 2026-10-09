# M0 — Inventory (read-only)

## Goal
Produce a factual snapshot of the repository so that M1–M6 operate on
verified data, not assumptions. This task mutates NOTHING outside
`docs/agents/reports/`.

## Steps
1. **Project files.** List every `*.dsp *.dsw *.sln *.vcproj *.vcxproj
   CMakeLists.txt Makefile*` in the tree. The canonical Win32 project
   is `win9x/np2.dsp` (VC6, 261 sources: root core + io, sound, iova,
   cbus, bios, vramva, i286x, generic, font, common, vram, fdd, lio,
   cpucva, cpuxva, biosva) — verify this count and dump the full
   source list into the report. Record the non-canonical build roots
   too: `Win9xC/np2c.dsp` (plain NP2, I286C C core, no VA subsystem),
   `sdl/Makefile*`, and any Makefiles in frozen backends; note for
   each whether it should count as a reachability root in M3 or be
   proposed for pruning itself.
2. **File census.** Run and save output of:
   - `python tools/repo/check_encoding.py --report`
   - `python tools/repo/check_eol.py --report`
   - `python tools/repo/check_case.py --report`
3. **Reference graph.** Run
   `python tools/repo/find_unreferenced.py --report`
   using the canonical project file(s) plus `sdl/` build files as
   roots. This produces the M3 deletion CANDIDATE list.
4. **Assembly & generated code.** The canonical project assembles four
   NASM inputs via .dsp custom build steps with a hardcoded path
   (`c:\bin\nasm\nasmw -f win32`): `cpuxva/memoryva.x86`,
   `i286x/dmap.x86`, `i286x/egcmem.x86`, `i286x/memory.x86`.
   No portable-C memoryva replacement exists in this tree yet — the
   .X86 files are live build dependencies. Enumerate any other asm
   (`I286A/`, `win9x/dclockd.x86`, ...) and whether anything builds it.
5. Write everything to `docs/agents/reports/inventory.md` with the raw
   script outputs under `docs/agents/reports/raw/`.

## Forbidden
- Any change outside `docs/agents/reports/`.
- "Cleanups" noticed along the way. Record them in the report instead.

## Done when
`inventory.md` exists and the user has reviewed it. No build gate.

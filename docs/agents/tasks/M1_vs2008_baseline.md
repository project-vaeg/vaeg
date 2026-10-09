# M1 — VS2008 baseline build (code as-is)

## Goal
The canonical Win32 project builds under Visual C++ 2008 (VC9),
Win32 / Release, with the legacy sources UNMODIFIED except where
compilation is impossible otherwise.

## Preconditions
M0 inventory reviewed; canonical project identified.

## Steps
1. Convert `win9x/np2.dsp` (VC6) once with VS2008 and COMMIT the
   resulting `.vcproj` + `.sln` (CRLF). Keep `np2.dsp`/`np2.dsw` in
   place — they become M3 prune candidates, not M1 deletions.
2. **NASM custom build steps — NINE, not four.** The .dsp assembles,
   via custom build rules hardcoding `c:\bin\nasm\nasmw -f win32`:
   `cpuxva/memoryva.x86`, `i286x/dmap.x86`, `i286x/egcmem.x86`,
   `i286x/memory.x86`, `win9x/x86/parts.x86`, `win9x/x86/opngeng.x86`,
   `win9x/x86/cputype.x86`, `win9x/x86/makegrph.x86`,
   `win9x/dclockd.x86` (see inventory.md, Assembly section).
   Verify the conversion preserved ALL NINE custom build steps
   (VC6→VS2008 conversion is known to drop or mangle these — check
   each one in the .vcproj). Replace the hardcoded path with plain
   `nasm -f win32` (NASM on PATH) and state the required NASM version
   in the report. This path fix is an expected, documented deviation.
   Note: `memoryva.x86`, `egcmem.x86`, `memory.x86`, `makegrph.x86`
   `%include 'x86/np2asm.inc'` — the assembler working directory must
   make that resolve (the .dsp rules run with `$(InputPath)`; keep the
   converted rules' working-directory semantics identical).
3. Pin the configuration explicitly in the project file (do not rely on
   IDE defaults): Win32 platform, Release (the .dsp also carries Trace/
   Debug configs — convert them but only Release gates), multi-byte
   character set (`_MBCS`), Windows SDK that ships with VS2008.
4. Build. For each error, apply the MINIMAL fix, prefer guards over
   edits:
   - missing legacy libs/SDK headers → adjust project settings first;
   - genuine code errors → smallest possible change, each one listed in
     `docs/agents/reports/m1_deviations.md` with file, line, reason.
   Zero silent changes. Warnings are recorded, not fixed.
5. Confirm the build also links (not just compiles) and the produced
   binary starts to the point of showing its window without ROMs
   (ROM-dependent behavior belongs to the human gate).

## Machine checks (paste into PR)
- Full build log, 0 errors.
- `git diff --stat` against the pre-M1 commit: only project files plus
  the deviations documented in `m1_deviations.md`.

## GATE G1 (human)
User builds from clean checkout with VS2008 and runs the checklist
(`docs/agents/gates/GATE_CHECKLIST.md`): V3-mode boot, VA demo, OS
operation. On pass: tag `baseline-vs2008`.

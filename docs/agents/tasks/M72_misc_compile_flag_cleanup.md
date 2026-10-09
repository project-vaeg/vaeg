# M72 - Inactive code audit and VAEG-specific cleanup

M72 starts from the formally approved and main-integrated G71 candidate:

```text
24950894eca79e308afae8d574d43c8f393bb483
```

Branch: `topic/m72-misc-compile-flag-cleanup`

Commit prefix: `M72:`

Candidate gate: `G72`

Report: `docs/agents/reports/m72_misc_compile_flag_cleanup.md`

Do not start M73. Do not merge M72 to `main` before G72 approval. Do not
declare G72 passed.

## Scope

M72 is an inactive-code audit milestone with narrowly scoped cleanup for code
and UI surface that no longer belongs to the VAEG active product.

M72 owns:

1. Audit inactive code that is not relevant to the active VAEG target.
   - The active product is the PC-88VA emulator, not a general PC-98/98x1
     emulator.
   - Keep compatibility scaffolding only when the active VA boot path,
     shared portable frontend, ROM-less tests, or current build still require
     it.
   - Classify each audited item as active-required, inactive-removable,
     inactive-but-deferred, or blocked-by-evidence-gap.
   - For each retained legacy-looking file or branch, record the active VAEG
     dependency that prevents removal.
   - Delete only code proven inactive by the audit and kept within this task's
     explicitly listed cleanup scope.
2. Fold `VAEG_FIX` as always enabled in the active CMake tree.
   - Remove the public compile definition from CMake targets.
   - Remove source `#if defined(VAEG_FIX)` conditionals by keeping the
     currently built active behavior.
   - Preserve the current runtime behavior and validation baselines.
3. Audit and remove inactive `SUPPORT_PC9821` guarded code from the active
   tree where the audit proves it is unbuilt and not required by VAEG.
   - Do not introduce PC-9821 support.
   - Do not preserve dead PC-9821 drawing, BIOS, PCI, GDC, FDC, palette or
     state-save branches as active code.
   - Preserve the supported PC-88VA active behavior.
4. Audit and remove inactive `PCMODEL_EPSON` / PC-286 model branches where the
   audit proves they are not required by VAEG.
   - Do not introduce EPSON PC compatibility.
   - Preserve the supported PC-88VA active behavior.
   - Keep only generic non-VA scaffolding required by current ROM-less tests
     or shared frontend initialization.
5. Audit and remove inactive `CPUCORE_IA32` branches.
   - The active uPD9002 core rejects `CPUCORE_IA32`; do not preserve the
     inactive IA32 CPU-core path as a build option.
   - Keep the current portable C CPU and I/O behavior.
6. Audit and remove inactive IDE I/O support controlled by `SUPPORT_IDEIO`.
   - VAEG does not model a VA IDE interface.
   - Do not remove SASI, SCSI, host FAT, or other current storage paths.
7. Audit and remove PC-9861K expansion RS-232C support controlled by
   `SUPPORT_PC9861K`.
   - Preserve the built-in VA RS-232C path.
   - Do not retain the PC-9861K two-channel expansion board UI, state, event,
     or C-Bus registration paths as active VAEG code.
8. Audit and remove `DISABLE_SOUND` branches.
   - The active VAEG target includes VA sound hardware.
   - Preserve the current sound-enabled behavior and state-save coverage.
   - Do not remove sound devices, sound output, MIDI, seek sounds, or sound
     configuration; remove only the inactive no-sound compile-time path.
9. Remove 98x1-only information from the SDL2 GUI About/More details.
   - Remove the `[98x1]` section and related PC-98-only fields from the
     `About -> More` output.
   - Keep VA model, VA ROM, sound, rhythm, screen, and other current VAEG
     information that remains useful.
   - Do not change emulator behavior to make the About dialog simpler.
10. Audit `VAEG_EXT` and remove obsolete active-tree references only where the
   inactive-code audit proves they are unbuilt and not required by VAEG.
   - Do not blindly enable the former extension/debug/SCSI paths.
   - Preserve the current non-`VAEG_EXT` behavior unless a specific branch is
     proven to be the active intended behavior.
   - Do not change state-save format or SCSI/SASI behavior without explicit
     evidence and tests.
11. Audit frontend asset embedding and font stubs only to classify future work.
   - Do not remove embedded GUI assets in M72.
   - Do not modify ROM/font payloads.
   - Remove the legacy `embed/` menu-source directory when proven unused by
     the active SDL2/ImGui frontend.
   - Do not remove `cmake/embed_binary.cmake`; it is the active build-time
     asset embedding helper for the splash, GUI font, and application icon.
   - Remove only a source stub if it is proven unused by the active build and
     does not affect guest font ROM loading, GUI font loading, asset
     embedding, or packaging.
12. Fold the active build to UTF-8 / LF text handling.
   - Remove inactive `OSLANG_SJIS`, `OSLANG_EUC`, `OSLINEBREAK_CR`, and
     `OSLINEBREAK_CRLF` branches.
   - Remove inactive `SUPPORT_EUC` and `SUPPORT_ANK` string backends after
     proving the active `milstr_*` path is UTF-8.
   - Fold `SUPPORT_UTF8` to the enabled side and remove it as a compile-time
     switch.
   - Preserve the current `OSLANG_UTF8` and `OSLINEBREAK_LF` behavior.
13. Fold `BEEPCOUNTEREX` as always enabled.
   - Preserve the currently built BEEP idle-counter extension behavior.
14. Treat SCSI HDD support as required for the active VAEG tree.
   - Fold SCSI conditional code to the enabled side.
   - Remove `SUPPORT_SCSI` as a compile-time switch after the fold.
   - Fold SASI conditional code to the enabled side and remove
     `SUPPORT_SASI` as a compile-time switch.
   - Preserve HOSTFAT support.
15. Remove legacy HOSTDRV.
   - `HOSTDRV` is the old NP2 DOS host-shared-drive system-port service, not
     the current VAEG HOSTFAT read-only host-folder feature.
   - Remove HOSTDRV implementation, state-save section, system-port commands,
     and legacy tool sources.
   - Preserve `io/hostfat.c`, `sdl2/hostfat_*`, HOSTFAT configuration,
     HOSTFAT state identity checks, and HOSTFAT guest-driver support.
16. Fold the active display output to the SDL2 RGB565 path.
   - Remove inactive `SUPPORT_8BPP`, `SUPPORT_24BPP`, `SUPPORT_32BPP`, and
     `SUPPORT_NORMALDISP` branches.
   - Remove `SUPPORT_16BPP` and `SCREEN_BPP` as compile-time switches after
     folding the active 16bpp path.
   - Fold `SUPPORT_CRT15KHZ` to the enabled side; the inactive 31kHz branch
     remains separately audited.
   - Preserve VA guest 16-bit color composition and SDL2 `RGB565` output.
17. Fold FDD seek sound support to the enabled side.
   - Remove `SUPPORT_SWSEEKSND` as a compile-time switch.
   - Preserve the active seek-sound behavior.
18. Remove inactive media, logging, and input-overlay support.
   - Remove `SUPPORT_CRT31KHZ` as an inactive Fellow-style 31kHz CRT branch.
   - Remove inactive MP3 and OGG sample decoders controlled by `SUPPORT_MP3`
     and `SUPPORT_OGG`.
   - Remove inactive S98 sound-register logging controlled by `SUPPORT_S98`.
   - Remove inactive key display and software keyboard overlays controlled by
     `SUPPORT_KEYDISP`, `SUPPORT_SOFTKBD`, and `SUPPORT_PC9801_119`.
   - Preserve active VA sound output, VA keyboard input, WAV sample loading,
     and SDL2 display behavior.

Explicitly deferred from M72:

- `SUPPORT_WAVEREC` remains out of scope for M72. Audio recording is not part
  of the current inactive-cleanup closure and must be audited in a later
  task before deletion or folding.
- `SUPPORT_OPRECORD` remains out of scope for M72. Operation-record support
  has state-save and device-observation hooks and must be audited in a later
  task before deletion.
- `io/fdd320.c` and `io/fdd320.h` remain out of scope for M72 deletion.
  They are legacy-looking, but 5-inch 2D behavior may still be relevant to
  the PC-88 side of the VA environment. Future I/O cleanup must audit that
  dependency before any removal.

## Non-goals

M72 must not:

- modify uPD9002 instruction semantics, SST policies, or generated evidence;
- modify M68, M69, M70, or M71 approved artifacts in place;
- change guest-visible FDD, SASI, GDC, BIOS, TVRAM, audio, keyboard,
  mouse, save-state, or display behavior;
- remove ROM or font payloads;
- remove embedded GUI font, splash, or icon assets without a separate
  maintainer-approved task;
- enable `VAEG_EXT` globally;
- preserve inactive `CPUCORE_IA32` as a supported build path;
- implement or preserve VA IDE support without hardware evidence;
- remove the built-in VA RS-232C path while removing PC-9861K expansion code;
- remove VA sound hardware or sound state while removing the inactive
  `DISABLE_SOUND` path;
- remove or weaken HOSTFAT while removing legacy HOSTDRV;
- remove VA sound output while removing S98 logging or optional compressed
  sample decoders;
- remove `SUPPORT_WAVEREC` or `SUPPORT_OPRECORD`;
- remove active VA keyboard input while removing inactive keyboard overlays;
- remove `io/fdd320.c` or `io/fdd320.h`;
- implement PC-9821 support;
- turn the active SDL2 frontend back into a general PC-98/98x1 frontend;
- start any unrelated cleanup.

## Required startup audit

Before production changes, record:

```bash
git status --short --branch
git rev-parse HEAD
git rev-parse origin/main
git diff --check
rg -n "VAEG_FIX|VAEG_EXT|SUPPORT_PC9821|PCMODEL_PC9821|PCMODEL_EPSON|CPUCORE_IA32|SUPPORT_IDEIO|SUPPORT_PC9861K|DISABLE_SOUND|PC-9861K|PC9861K|PC-9821|PC9821|98x1" .
```

Confirm:

- the branch starts from `24950894eca79e308afae8d574d43c8f393bb483`;
- the worktree is clean;
- no active M72 task already exists;
- `VAEG_FIX` is currently defined by the active CMake build;
- `VAEG_EXT` is not currently defined by the active CMake build;
- `SUPPORT_PC9821` is not currently defined by the active CMake build.
- `SUPPORT_IDEIO`, `SUPPORT_PC9861K`, and `CPUCORE_IA32` are not currently
  defined by the active CMake build.
- `DISABLE_SOUND` is not currently defined by the active CMake build.

Stop if an apparently dead conditional owns current guest-visible behavior.
Stop if an item is only plausibly irrelevant but cannot be proven inactive
from build, reference, runtime, or repository-policy evidence.

## Implementation rules

Keep one concern per commit:

1. task authority and roadmap update;
2. inactive-code audit tooling or inventory;
3. About/More 98x1 information removal;
4. `VAEG_FIX` constant-fold;
5. `SUPPORT_PC9821` removal, if proven inactive;
6. `PCMODEL_EPSON` removal, if proven inactive;
7. `CPUCORE_IA32` removal, if proven inactive;
8. `SUPPORT_IDEIO` removal, if proven inactive;
9. PC-9861K expansion-board removal, if proven inactive;
10. `DISABLE_SOUND` removal, if proven inactive;
11. UTF-8/LF and BEEP conditional folding;
12. SCSI enable-side folding;
13. legacy HOSTDRV removal while preserving HOSTFAT;
14. display BPP cleanup;
15. inactive media, logging, and input-overlay cleanup;
16. VAEG-relevance cleanup for items proven inactive;
17. `VAEG_EXT` cleanup, if proven inactive;
18. optional unused-source-stub cleanup, if proven inactive;
19. report and evidence.

For every removed conditional, document which side is retained and why.

For every removed file or block, prove that the active build no longer
references it.

If an audit item cannot be removed safely, leave it in place and record the
blocker in the report instead of broadening the milestone.

Do not delete a binary payload.

Do not hide a behavior change by changing tests or baselines.

## Validation

Run, at minimum:

```bash
git diff --check
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure
```

Also run GCC, Clang, ASan/UBSan, MinGW, and hosted CI where available or
record the exact local blocker.

Run M68, M69, M70, and M71 protected checks if their repository commands remain
available after the cleanup.

## Report

Write `docs/agents/reports/m72_misc_compile_flag_cleanup.md` with:

- starting SHA;
- branch;
- commit list;
- removed compile definitions;
- retained conditional sides;
- files changed;
- inactive-code audit inventory;
- inactive-code removals and deferred items;
- About/More 98x1 removal result;
- PC-9821 removal inventory;
- EPSON model removal inventory;
- `VAEG_EXT` disposition;
- font/embed audit result;
- validation commands and exit statuses;
- hosted CI URL and result;
- deviations and remaining risks;
- G72 human-review checklist.

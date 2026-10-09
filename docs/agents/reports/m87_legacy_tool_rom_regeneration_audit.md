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
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M87: legacy tool and ROM regeneration audit

## Status

This is the final M87 report. The audit started from the G86-approved `main`
checkpoint and was developed on
`topic/m87-legacy-tool-rom-regeneration-audit`. G87 human gate passed on
2026-08-12 for candidate
[d2d1a13167ccd094d0fae180c775ad5e1d7eb78e](https://github.com/nakatamaho/vaeg/commit/d2d1a13167ccd094d0fae180c775ad5e1d7eb78e).
The resulting M87 implementation and hotfix chain was merged to `main`
at [f876dbb](https://github.com/nakatamaho/vaeg/commit/f876dbbfe4e69f0a2ad2021b289962d15754812d).

The first M87 cleanup is recorded in
[6838b4c2e2f27f5d39e5dc639f9d47b8e5d40db9](https://github.com/nakatamaho/vaeg/commit/6838b4c2e2f27f5d39e5dc639f9d47b8e5d40db9).
It removes only the unbuilt legacy utility sources listed below. No ROM,
font, icon, disk image, generated resource, or `romimage/` payload was
modified.

## Audit contract

M87 covers the remaining `accessories/` and `np2tool/` sources, the legacy
`romimage/` assembly and regeneration makefiles, the current CMake binary
embedding helper, its SDL2 consumers, and release packaging assumptions.
The task explicitly excludes replacement ROM generation and all binary
payload changes. A path is removable only when the current CMake build,
tests, CI, release package, and documented/manual operation do not depend on
it.

## Findings and decisions

| Path or flow | Classification | Evidence and decision |
|---|---|---|
| `accessories/bin2txt.c` | Inactive-removable | A legacy binary-to-C-array converter. No current CMake target, test, CI workflow, release package, or manual gate invokes it. The only remaining textual references are in the legacy `romimage/` makefiles. Removed in the M87 cleanup commit. |
| `accessories/lzxpack.c` | Inactive-removable | A legacy LZX compressor for generated C data. It is not part of the active build or release flow and has no current caller. Removed in the M87 cleanup commit. |
| `accessories/textout.c` and `accessories/textout.h` | Inactive-removable | Support code used by the removed legacy utility path only. No active target or test includes it. Removed with that path. |
| Remaining `np2tool/` sources | Inactive-removable | `getbios.asm` and `pwoff.asm` are old DOS/8086 utilities; their include, makefile, and x86 metadata have no current target or workflow consumer. The former HOSTDRV utility path was already removed by M72. The five remaining files were removed in the M87 cleanup commit. |
| `cmake/embed_binary.cmake` | Active-required | `CMakeLists.txt` invokes it from three explicit custom commands to generate C arrays for the splash bitmap, GUI font, and application icon. It remains in the active tree. |
| `assets/vaeg.bmp` and `sdl2/splash.c` | Active-required | The splash bitmap is embedded at configure/build time and consumed by the SDL2 splash and About UI. The runtime does not load a source image from the working directory. |
| `assets/NotoSansJP-Regular.ttf` and `sdl2/gui/gui.cpp` | Active-required | The GUI font is embedded by CMake and registered by the Dear ImGui frontend. Removing the helper or source asset would break the current GUI build or its Japanese text rendering. |
| `assets/vaeg.ico`, `sdl2/appicon.c`, and `sdl2/vaeg.rc` | Active-required | The icon is embedded for SDL use and referenced by the native Windows resource file. It remains outside the M87 deletion set. |
| `romimage/` assembly sources and makefiles | Deferred; legacy-only regeneration | The directory contains historical PC-98/VA assembly inputs and old MASM/EXE2BIN/NASMW-style makefiles. It is not named by the active CMake source lists, CI build, or release workflow. Its old makefiles still describe a non-portable regeneration path, but M87 has no approved replacement pipeline and does not alter those sources. |
| Generated resources included by current sources | Active-required | Current code directly includes resources such as `bios/keytable.res`, `bios/itfrom.res`, `bios/startup.res`, `bios/biosfd80.res`, `cbus/sasibios.res`, `fdd/hddboot.res`, `font/fontdata.res`, `generic/dipswbmp.res`, and `generic/minifont.res`. They are retained as protected generated payloads. |
| Legacy-only generated-resource entries | Deferred; evidence gap | `cbus/idebios.res`, `cbus/scsibios.res`, and `fdd/fdd_mtr.res` have no current source include found in this audit, but their old regeneration metadata is part of the protected `romimage/` boundary. M87 does not delete or rewrite them. Any future removal requires a separate explicit change and a complete consumer/provenance decision. |
| `.github/workflows/build.yml` | Active-required build contract | CI builds the current CMake Linux, MinGW, and macOS targets. It does not invoke `accessories/`, `np2tool/`, or the `romimage/` makefiles. |
| `.github/workflows/release.yml` | Active-required release contract | Release packaging ships the executable, HOSTFAT driver, licenses, changelog, and README. It intentionally does not package user-supplied ROMs or disks; embedded GUI assets are already inside the executable. |

## Current embedding and release path

The active asset path is:

```text
assets/vaeg.bmp --------------------+
assets/NotoSansJP-Regular.ttf ------+--> cmake/embed_binary.cmake
assets/vaeg.ico --------------------+          |
                                             v
                                  generated C arrays in the build tree
                                             |
                         sdl2/splash.c / sdl2/gui/gui.cpp / sdl2/appicon.c
```

The custom commands are explicit in
[`CMakeLists.txt`](../../../CMakeLists.txt:402), and the generated arrays are
consumed by the SDL2 frontend. This is independent of the removed
`bin2txt`/`lzxpack` tools. The release workflow consequently needs only the
executable and its documented external runtime files; it does not need to
run a legacy resource converter at packaging time.

The old `romimage/` flow is different: its makefiles assemble historical
binary inputs and use external DOS-era tools to turn selected outputs into
`.res` files. Some resulting `.res` files are active runtime inputs, but the
current build consumes the checked-in resources and has no portable,
reproducible invocation of those old makefiles. Therefore the generator
sources are retained as deferred provenance, while the active resource
payloads remain protected. M87 does not claim that the old generator can
currently reproduce every checked-in resource.

## Deletion boundary

The M87 cleanup deletes exactly these nine tracked files:

- `accessories/bin2txt.c`
- `accessories/lzxpack.c`
- `accessories/textout.c`
- `accessories/textout.h`
- `np2tool/getbios.asm`
- `np2tool/makefile.w32`
- `np2tool/np2tool.inc`
- `np2tool/np2tool.x86`
- `np2tool/pwoff.asm`

No active CMake list, CMake test, CI workflow, release package, or current
manual operation references these files. The legacy makefile references
that remain under `romimage/` are historical regeneration metadata, not
current build dependencies. No replacement utility was introduced because
that is outside M87's scope.

## Verification

| Check | Result |
|---|---|
| `git diff --check` | PASS |
| `tools/repo/check_case.py` | PASS; 0 findings |
| `tools/repo/check_encoding.py --expect utf8` | PASS; 0 violations |
| `tools/repo/check_eol.py` | PASS |
| `cmake --preset linux-debug` and `cmake --build --preset linux-debug -j4` | PASS; 213/213 build steps |
| Linux Debug VA self-test | PASS; all tests passed |
| `cmake --preset linux-ci-gcc` and `cmake --build --preset linux-ci-gcc -j4` | PASS; 283/283 build steps |
| `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null ctest --test-dir build/linux-ci-gcc --output-on-failure` | PASS; 83/83 tests passed, 1 external corpus test skipped |
| MinGW cross configure/build | PASS; 759/759 build steps, PE32+ x86-64 |
| MinGW artifact | `build/mingw-cross/sdl2/vaeg.exe`; SHA-256 `5a8984133db101434300b14a1103b29fb728df515909c2ffae34cb14fcafdeec` |
| Release-package smoke | PASS; exact Linux workflow payload shape, 7 files, and `check_zex_archive.py` passed |
| Active reference scan | PASS; no current CMake/test/CI/release reference to removed utility sources; remaining mentions are historical documentation/evidence or M87 report text |

The initial CTest invocation without the isolated Git configuration produced
false failures in existing Git-history validators because the sandbox denied
access to the maintainer's global Git configuration. Re-running the unchanged
CTest with `GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null` passed all
83 tests; this is the result recorded above.

## Gate state

M87 machine validation and the G87 human gate are complete. G87 human gate
passed on 2026-08-12 for the candidate identified above. M87 is closed at
[d2d1a13167ccd094d0fae180c775ad5e1d7eb78e](https://github.com/nakatamaho/vaeg/commit/d2d1a13167ccd094d0fae180c775ad5e1d7eb78e).
The M87 branch work is complete and represented on `main` by the merge
commit above. M88 is the follow-on VA-only source-tree audit.

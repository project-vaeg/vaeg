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
# M99 final report

Status: BLOCKED

Windows follow-up: the maintainer confirmed guest display recovery, but
clarified that menus remain tiny and reported no CRT effect. [M99z6](m99z6_windows_crt_gui.md)
corrects the Windows native GUI/activation path and records new local QA.
Its physical CRT and lifecycle retest remains pending; previous UI-completion
claims do not establish native GUI coverage on Metal/OpenGL.

M99a through M99z were executed on `topic/m99-native-crt-rebuild`. The
optional native CRT implementation, licensing boundary, package checks,
fallback path, and environment-independent QA are complete. The required
real-GPU lifecycle and 1920x1080 performance gates could not be demonstrated
from this macOS/Colima environment, so this report does not claim `DONE`.

## Commits, branch, and remote state

- Starting rewritten-main baseline: `0b6e14883035cb17073c5c24c8c9d4b5c22b9162`.
- The lease-protected rewrite of `main` succeeded before M99 work began; the
  rejected implementation is unreachable from ordinary repository-owned refs,
  and its old commit log/hashes are intentionally not reproduced here.
- During M99, `origin/main` advanced to `b25d151236ddfe093e4c161bbf00ce0b7d8d5e74`
  with one unrelated retained change. It was merged into the topic by
  `5d2e63b5bfa52f5e84609b36a02d4fd2a161db5e` without altering that change.
- Last runtime implementation commit:
  `aa84d543649056236e939c6cb66376e0f0df5ccc`.
- Last machine-validated non-report commit before this report closure:
  `origin/topic/m99-native-crt-rebuild` = `3578d97a9ce54c1f77f89bc1fc6cbd4e21445310`.
- The final report commit is the containing commit for this file; its exact
  full ID is the final `HEAD` shown by `git log -1` after this commit.
- No merge into `main`, release, binary publication, or remote-history rewrite
  was performed after the authorized main reconstruction.

## Sub-milestone results

| Milestone | Result | Evidence commit(s) |
| --- | --- | --- |
| M99a | PASS — history boundary recorded | `9bcc6866` |
| M99b | PASS — removal boundary verified | `c36d429e` |
| M99c | PASS — main reconstructed | `04d9fe24` |
| M99d | PASS — lease-protected main publication | `cc5aed3c` |
| M99e | PASS — clean baseline recorded | `09fbab2e` |
| M99f | PASS — frontend inventory | `13064088` |
| M99g | PASS — dependency and shader audit | `33fe683a`, `b07b9c50` |
| M99h | PASS — common contracts | `92b4ad5e` |
| M99i | PASS — raw capture boundary | `aac27e1c` |
| M99j | PASS — common pass-through tests | `20bb4897` |
| M99k | PASS — macOS/Metal build bridge | `007ba166` |
| M99l | PASS — Metal pass-through implementation | `bb90b1ef` |
| M99m | PASS — Metal filter chain | `0ca99abc` |
| M99n | PASS — Metal lifecycle handling | `544eea1a` |
| M99o | PASS — D3D11 backend | `a3f29cef` |
| M99p | PASS — D3D11 pass-through | `c68ed39c` |
| M99q | PASS — D3D11 librashader chain | `34cafc26` |
| M99r | PASS — D3D11 lifecycle handling | `4f5ee593` |
| M99s | PASS — Linux OpenGL backend | `fda01ee1` |
| M99t | PASS — OpenGL librashader chain | `971d32bc` |
| M99u | PASS — common presets and parameters | `ab9246b9` |
| M99v | PASS — settings UI | `a7587921` |
| M99w | PASS — fallback and headless behavior | `1f141693` |
| M99x | PASS — shader and release packaging | `cfdd41d7`, `2b2da357` |
| M99y | PARTIAL — automated QA passed; physical GPU evidence deferred | `12e33893`, `7fda0663`, `ed339397` |
| M99z | PASS — documentation, evidence, and SDL smoke correction assembled | `b07b9c50`, `265f1582`, `5d2e63b5`, `1bd5330f`, `01d31199` |
| M99z1 | PASS — zero-sized drawable viewport regression corrected; physical Windows confirmation pending | `fa2a3d78`, `8abf339f` |
| M99z2 | PASS — macOS FetchContent language initialization corrected | `2aeaf519` |
| M99z3 | PASS — Windows save-state selftest boundary stabilized | `aa84d543` |
| M99z4 | PASS — governing M99 specification published at its required path | `af1edaa8` |
| M99z5 | PASS — BSD header and packaged-provenance integrity audit completed | `891ceef6`, `a1e994b2`, `3578d97a` |

## Gate status

| Gate | Status | Evidence boundary |
| --- | --- | --- |
| G99-1 | PASS | Rewritten main was lease-published and later unrelated mainline work was retained; ordinary refs were checked. |
| G99-2 | PASS | Backend-neutral contract, core isolation, raw-capture tests, and static audits passed. |
| G99-3 | BLOCKED | No Windows host/D3D11 hardware was available for real lifecycle and performance evidence. |
| G99-4 | BLOCKED | Colima's virtual arm64 Linux container had no real OpenGL display/GPU; its software/virtual result is smoke evidence only. |
| G99-5 | BLOCKED | macOS feature-on build passed, but no usable Cocoa/Metal display was available. |
| G99-6 | PASS | Runtime-free and optional-runtime staged archives for all three platforms passed inspection. |
| G99-7 | BLOCKED | No representative real-hardware 60 Hz benchmark; the macOS FetchContent CI failure is corrected. |

## Implementation and ownership

The emulation framebuffer remains in the SDL2 shadow buffer and is submitted
as `VAEG_FRAME_INPUT`. Raw guest-frame and TVRAM capture remain upstream of all
GPU processing. A native presenter owns exactly one platform path:

- macOS: `MetalPresenter` and `CAMetalLayer`/Metal bridge;
- Windows: `D3D11Presenter` and D3D11/DXGI bridge bound to the SDL window;
- Linux: `GLPresenter` and an OpenGL 3.3 core bridge.

The common contract carries borrowed pixels, format, pitch, dimensions,
aspect, frame number, and timing only. It carries no platform backend type.
Preset parsing and metadata enumeration occur at initialization, while GPU
resources are recreated only for size/context/device recovery. Filter failure
first falls back to native pass-through; native device/resource failure
recreates the existing SDL renderer from the same shadow framebuffer.

Headless dummy video bypasses native presentation. Native rendered capture is
explicitly unavailable and reports that fact; deterministic `--screenshot`
and TVRAM capture remain the QA path.

## Dependency, binary, and license provenance

The pinned dependency is official `librashader-v0.12.0`, peeled commit
`87e8a97b50516d997defeaa168173dcd185d4022`, source archive SHA-256
`4bf8cf2489d00848dcabbf2163204093776082da4217d5a5db45e4cbf335cedf`.
The C API is 5 and C ABI is 2. M99 QA and release builds statically link the
official C API archive. The dynamic loader remains available only for explicit
non-release developer diagnostics and is not required by release binaries.

The tracked header SHA-256 values are:

```text
librashader.h    5d478897c391af3f60015810b67785ae1a286d262a845485276e36ded9f21e62
librashader_ld.h bcffcbc854afb287c9f935c1a0e3b569f5e6775ef85914bd7f7025ad1f6bde33
```

VAeg remains BSD-2-Clause. librashader implementation files are MPL-2.0 and
the upstream C headers/loader are MIT as recorded in ADR-0014. The single
default `crt-lottes-fast.slang` shader is recorded as Unlicense/public-domain
provenance, with no include, LUT, or secondary-pass dependency. CRT-Geom,
CRT-Royale, Mega Bezel, the complete `slang-shaders` tree, GPL material, and
unknown-license shader content are not tracked or shipped.

The staged shader closure is:

```text
assets/shaders/crt/vaeg_crt_default.slangp
assets/shaders/crt/shaders/crt-lottes-fast.slang
assets/shaders/crt/licenses/crt-default-license.txt
assets/shaders/crt/licenses/crt-default-provenance.md
licenses/librashader-MPL-2.0.txt
licenses/librashader-headers-MIT.txt
licenses/THIRD_PARTY_NOTICES.md
```

Package runtime inputs are optional and platform-specific. The audited staged
runtime hashes are recorded in the M99x report and ADR-0014; no runtime binary
is committed.

## Build and test evidence

Clean-baseline evidence is recorded in M99e: the old SDL renderer built, 83 of
84 registered CTest cases passed, one external SSTS test was skipped, and the
ROM-less smoke invocation used for the baseline completed in that local setup.

M99y ran the feature-on Linux build in Colima/Docker with the trace build
option, then:

```sh
cmake -S /src -B /tmp/vaeg-m99-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON \
  -DVAEG_ENABLE_LIBRASHADER=ON -DVAEG_ENABLE_ARCHIVE_DROP=OFF
cmake --build /tmp/vaeg-m99-linux --parallel 2
ctest --test-dir /tmp/vaeg-m99-linux --output-on-failure \
  -R '^(vaeg_librashader_|vaeg_romless_tests$)'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  /tmp/vaeg-m99-linux/sdl2/vaeg --selftest
```

Results: feature-on Linux build PASS, focused M99/ROM-less CTest 7/7 PASS,
and full executable selftest exit 0. macOS feature-on build and focused
fallback/lifecycle tests passed. Feature-off full app/selftest also passed.
The source archive checker passed after the explicit librashader allow-list
correction; package staging passed for Linux, macOS, and Windows with and
without the optional runtime, including a deliberate wrong-platform negative
case.

The hosted run for the earlier M99y tip is
[Actions run 33864023294](https://github.com/nakatamaho/vaeg/actions/runs/33864023294)
and completed with failure. The failure set was separately diagnosed:

- Linux gcc/clang/ASAN and Windows smoke jobs exited 1 after explicitly
  reporting ROM-less mode. The ROM-less return value in `sdl2/np2.c` is
  intentional: it keeps screen-uniform detection disabled. The actual
  startup failure was an inverted `SUCCESS == 0` test in
  `sdl2/scrnmng.c`, which treated successful SDL resource creation as a
  failure; M99z corrects both affected startup/fallback checks.
- The standalone archive check rejected `external/librashader` until the
  allow-list fix in `265f1582`.
- macOS FetchContent configuration failed inside hosted CMake with missing
  internal CMake variables; this is a host/toolchain configuration failure
  after checkout and before compilation.

The subsequent topic run for the corrected tip was
[Actions run 33867116878](https://github.com/nakatamaho/vaeg/actions/runs/33867116878).
It completed with the following result: repo invariants, guest-driver,
Ubuntu gcc/clang/ASAN, SST, standalone compatibility, and Windows MinGW
build/smoke/unit-test/release-artifact jobs passed. The only failure was
macOS FetchContent configure, with CMake reporting missing
`CMAKE_OBJCXX_COMPILE_OBJECT`, `CMAKE_OBJCXX_ARCHIVE_CREATE`, and related
internal variables before compilation. Hosted CI is not used as real-GPU
evidence.

M99z2 moved Apple Objective-C and Objective-C++ language initialization ahead
of the FetchContent dependency graph. In
[Actions run 33877195164](https://github.com/nakatamaho/vaeg/actions/runs/33877195164),
the previously failing macOS FetchContent configure, build, smoke, unit tests,
and artifact staging all passed. The run's only failure was an independent
Windows-only save/load/save selftest race; the Windows release build, smoke,
viewport regression, runtime import check, and release artifact passed.

M99z3 stabilized that selftest boundary. The final
[Actions run 33879771189](https://github.com/nakatamaho/vaeg/actions/runs/33879771189)
at `aa84d543649056236e939c6cb66376e0f0df5ccc` passed all ten jobs,
including both Windows selftest registrations and the macOS FetchContent job.
This hosted result is automated compatibility evidence, not real-GPU evidence.

The subsequent M99z5 integrity update passed all ten jobs in
[Actions run 33882272840](https://github.com/nakatamaho/vaeg/actions/runs/33882272840)
at `3578d97a9ce54c1f77f89bc1fc6cbd4e21445310`, including repository
invariants, guest-driver distribution, gcc/clang/ASAN, architectural SST,
standalone conformance, both Windows jobs, and macOS FetchContent.

## Performance and manual evidence

No acceptable benchmark numbers were obtained. Average, p95, maximum
presentation time, GPU resource growth, and 60 Hz sustain at 1920x1080 remain
unmeasured on representative real Windows, Linux, and macOS hardware. No
fidelity or shader policy was silently changed to manufacture a performance
result.

Deferred manual action: on one real host for each platform, run the default
CRT preset and native pass-through through resize, HiDPI/Retina, fullscreen,
minimize/restore, toggle, context/device/drawable recovery, and a sustained
1920x1080 60 Hz workload. Record hardware, driver/OS, preset identity, average,
p95, maximum frame/presentation time, and bounded-resource observations. This
is the smallest remaining action to convert G99-3/G99-4/G99-5/G99-7 from
BLOCKED to PASS.

## Final status

M99z7 follow-up: the maintainer still reports unavailable CRT and black native
output. [The candidate report](m99z7_windows_crt_fallback.md) records no-cull
pass-through rendering, explicit SDL/librashader selection, and dependency/error
diagnostics. This does not close physical Windows acceptance. The M99z6 hosted
[run](https://github.com/nakatamaho/vaeg/actions/runs/33935164203) passed, but its
mock-based tests did not exercise D3D11 rasterization or target DLL loading.

The implementation and all safe environment-independent M99 work are
complete. Overall status is **BLOCKED**, not DONE, because required physical
GPU lifecycle/performance evidence is unavailable. The SDL smoke startup and
macOS FetchContent conditions have been corrected and passed in subsequent
hosted jobs, and all ten jobs in the latest hosted run passed.

Working-tree status at the last machine-validated non-report commit before
this report update:

```text
[clean]
```

The earlier untracked backup file belonged to a previous task worktree and was
not copied into, modified by, or published from this clean M99 follow-up clone.

## M99z1 Windows black-screen follow-up

A real Windows MinGW run after M99z displayed the menu and continued to
produce emulated audio and floppy-drive sounds, but no guest video. Its
startup and scale-change diagnostics showed valid window and drawable sizes
while the guest viewport remained `0,0 0x0`. The same result with the SDL-only
build excluded librashader loading and shader compilation as the immediate
cause.

The demonstrated defect was a second set of inverted status checks in
`sdl2/scrnmng.c`. `scrnmng_get_drawable_size()` returns the repository status
value `SUCCESS == 0`; four callers incorrectly used logical negation. This
made every successful drawable query fail viewport calculation and also
affected menu high-DPI placement, pointer mapping, and native-presenter
resize/presentation. Commit
[`fa2a3d78`](https://github.com/nakatamaho/vaeg/commit/fa2a3d78749d84fc08d90ee749bc32f10b2f08fa)
uses explicit `!= SUCCESS` checks, rejects an invalid startup viewport, and
adds the `vaeg_sdl_startup_viewport` ROM-less CTest.

Local verification at that commit:

```text
macOS MacPorts build (librashader off): PASS
macOS CTest: 90 executed PASS; 1 external SSTS case SKIP
dummy SDL smoke: guest=0,22 640x400
MinGW SDL-only console build: PASS
MinGW librashader-enabled D3D11 console build: PASS
encoding, EOL, and path-case checks: PASS
```

The repository-wide clang-format checker still reports formatting debt in
unchanged pre-existing lines; only the modified line ranges were formatted
with `clang-format-mp-22`. A physical Windows rerun with the corrected binary
is still required before claiming that the observed black screen is cleared,
and native D3D11/CRT lifecycle and performance evidence remains part of the
existing G99-3/G99-7 blocker.

## M99z2 macOS FetchContent follow-up

The hosted `macos-ci` preset originally failed during CMake generation before
compilation because FetchContent dependencies were processed before CMake had
initialized its Objective-C++ rules. Moving Objective-C++ alone was
insufficient: SDL's Objective-C sources then inherited C++ language options.
Commit
[`2aeaf519`](https://github.com/nakatamaho/vaeg/commit/2aeaf519e60695ce2f9f9bd4c32e3a115421f51a)
enables both Objective-C and Objective-C++ before the dependency graph when
the Apple librashader path is selected.

A clean local `cmake --preset macos-ci` configure and complete build passed.
The dummy-driver smoke reported `guest=0,22 640x400`; all 103 executed CTests
passed and six environment/dependency cases were skipped. The macOS job in
[Actions run 33877195164](https://github.com/nakatamaho/vaeg/actions/runs/33877195164)
then passed configure, build, smoke, unit tests, package validation, and
artifact upload.

## M99z3 Windows save-state selftest follow-up

Two hosted Windows runs intermittently failed one of the two identical
`vaeg --selftest` registrations while the other passed. Both failures reported
that the `FMBOARD` save-state section differed at section offset `0x404ab`.
An ABI-layout probe mapped this byte to offset 28 of the ymfm backend state,
the low byte of the first slot's 64-bit `phase` field.

`test_statsave()` opened and started SDL dummy audio through `pccore_init()`
and `pccore_reset()`, but unlike the production GUI it did not stop host audio
before saving. `statsave_load()` also resumed audio, allowing the Windows dummy
callback to advance ymfm state between the restore and second save. Commit
[`aa84d543`](https://github.com/nakatamaho/vaeg/commit/aa84d543649056236e939c6cb66376e0f0df5ccc)
calls `soundmng_stop()` immediately before both selftest saves. Production
save-state behavior and serialized data are unchanged.

Focused macOS CTest ran both registrations successfully. The MinGW cross-build
passed, and two complete MinGW Release selftests passed under Linux/amd64 Wine.
Most importantly, the Windows compatibility job in
[Actions run 33879771189](https://github.com/nakatamaho/vaeg/actions/runs/33879771189)
passed its smoke and complete unit-test steps, including both selftest
registrations. All ten jobs in that run passed; the run also reconfirmed the
macOS FetchContent correction. Hosted CI does not substitute for the deferred
physical GPU lifecycle and performance gates.

## M99z4 governing specification follow-up

The clean reconstruction clone did not contain the governing task at the path
required by M99, leaving the final report's specification reference broken.
Commit
[`af1edaa8`](https://github.com/nakatamaho/vaeg/commit/af1edaa866d2f8d27f316be15c52b74be0a6e029)
publishes the maintainer-provided specification as
`docs/agents/tasks/M99_librashader_crt_pipeline.md`, with the required
BSD-2-Clause header. A byte comparison from the title through end of file
confirmed that the specification body is otherwise unchanged. The bug-fix
ledger now resolves to that tracked task file.

## M99z5 license-header and package-integrity follow-up

An audit against the repository's new-file policy found 23 VAeg-owned M99
reports/decision records without the required BSD-2-Clause header. Commit
[`891ceef6`](https://github.com/nakatamaho/vaeg/commit/891ceef66e225dc4929240ca41dfd217d37608d3)
prepends headers without changing their bodies, and commit
[`a1e994b2`](https://github.com/nakatamaho/vaeg/commit/a1e994b24b0a748e649411e1039f3d1c05628bef)
registers that mechanical update in `.git-blame-ignore-revs`.

The VAeg-authored CRT provenance record also received its required header.
Because release staging pins that document, commit
[`3578d97a`](https://github.com/nakatamaho/vaeg/commit/3578d97a9ce54c1f77f89bc1fc6cbd4e21445310)
updates both independent checks to its new SHA-256
`2750c3e592acaa38ada456fc30c7993cca3a35e8ab9b24793c4b2747a36ef063`.
The audited upstream preset, shader, and Unlicense notice remain byte-identical
at their recorded hashes.

Fresh runtime-free staging and package validation passed for Linux, macOS, and
Windows. Shell syntax, Python compilation, encoding, EOL, path-case, and
whitespace checks passed. Finally,
[Actions run 33882272840](https://github.com/nakatamaho/vaeg/actions/runs/33882272840)
passed all ten jobs. This closes the environment-independent policy defect but
does not change the deferred physical GPU gates.

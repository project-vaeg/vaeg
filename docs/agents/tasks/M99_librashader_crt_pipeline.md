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
# M99 — Native librashader CRT Presentation Pipeline

Status: Approved for complete reimplementation  
Scope: SDL2 frontend presentation only  
VAeg license: BSD-2-Clause  
Shader runtime: [librashader](https://github.com/SnowflakePowered/librashader)  

## Codex one-shot instruction

Use this command after placing this file at
`docs/agents/tasks/M99_librashader_crt_pipeline.md`:

```text
/goal Implement M99 exactly as specified in docs/agents/tasks/M99_librashader_crt_pipeline.md. First remove the rejected M99 commits from main history and all repository-owned refs using a verified history reconstruction and force-with-lease; do not retain an archive, backup, or recovery ref. Prove the clean baseline, then rebuild the feature from scratch with D3D11 on Windows, OpenGL on Linux, and Metal on macOS. Execute through the final gates without asking routine questions. Preserve unrelated work, stop before any unverified destructive operation, and finish with a complete evidence report that does not reproduce the old commit log or hashes.
```

The task file is authoritative. Read every applicable `AGENTS.md`, repository
convention, roadmap, build, release, and test document before acting. Continue
autonomously through all sub-milestones. Ask the user only if credentials,
branch protection, an ambiguous history boundary, or an unexpected destructive
target makes safe completion impossible.

## 1. Decision and objective

The previous M99 implementation is rejected in full. Remove it from `main`
history before writing the replacement. Do not copy, cherry-pick, port, or use
any old M99 source, tests, build logic, assets, reports, or completion evidence.

Rebuild the optional CRT presentation path with these fixed native backends:

| Platform | Required graphics API | Required librashader linkage |
| --- | --- | --- |
| Windows | Direct3D 11 | Static C API linkage with D3D11 features |
| Linux | OpenGL 3.3+ core | Static C API linkage with OpenGL features |
| macOS | Metal | Static C API linkage with Metal features |

The emulation core and canonical framebuffer remain renderer-independent. SDL2
continues to own windows, events, and input; the selected native presentation
backend owns its GPU device/context, source texture, output target, resize, and
present operations. Do not design a generic `SDL_Texture` bridge and do not
make one native backend reach through another renderer's private resources.

When the native presenter is active, CRT-off is a native pass-through draw. Do
not switch renderer ownership every time the user toggles CRT. If native
initialization fails, cleanly fall back to the existing SDL presentation path.

## 2. Destructive history rewrite — explicit authorization and limits

The user authorizes removing the rejected M99 commits from `main` and updating
the remote `main` with `--force-with-lease`. This authorization applies only to
the proven M99 commit set. It does not authorize deleting unrelated commits,
working-tree files, tags, releases, branches, or user changes.

Do not create or retain a local or remote backup, archive, safety branch, tag,
bundle, patch series, report, or manifest that preserves the rejected commit
history. Keep the expected old `main` object ID only for the duration of the
lease-protected update; do not write it into a tracked file or final report.

Required procedure:

1. Fetch the remote and record the exact local and remote `main` object IDs.
2. Inspect first-parent and full history, merge parents, reflogs, changed paths,
   commit messages, and patch contents. Identify:
   - `PRE_M99`: the last commit before any rejected M99 change;
   - `OLD_MAIN`: the current remote `main` tip;
   - every rejected M99 commit and merge;
   - every later non-M99 commit that must be retained.
3. Construct a new candidate mainline whose first new parent is `PRE_M99`.
   Replay only retained non-M99 changes in original order. No rejected M99
   commit may be included, cherry-picked, merged, or retained as a parent or
   ancestor of the candidate. Replayed retained changes receive new commit IDs.
4. Do not use `git revert` for removal: a revert would undo the files but leave
   the rejected M99 commits visible in `main` history. Replace `main` with the
   newly constructed ancestry only after it passes verification. Never use
   `git reset --hard` on a dirty worktree, and never use plain `--force`;
   publish only with the exact lease specified below.
5. Prove that the candidate contains all retained work and no rejected M99
   content. Review range-diffs, name-status diffs, build files, release files,
   generated manifests, shader assets, documentation, and binary artifacts.
6. Build and test the candidate as the old renderer-only baseline.
7. Immediately before publishing, fetch again. If remote `main` is no longer
   `OLD_MAIN`, stop and report `BLOCKED`; do not overwrite new remote work.
8. Update `main` only with an explicit lease tied to the recorded old object ID,
   equivalent to `--force-with-lease=refs/heads/main:<OLD_MAIN>`.
9. Delete every local and remote branch or tag owned by this repository whose
   sole purpose is the rejected M99 and which makes a rejected commit reachable.
   If such a ref also contains unrelated work, reconstruct and preserve that
   work first. Do not delete provider-managed pull-request refs blindly.
10. Verify that no rejected M99 commit is reachable from `main`, any ordinary
    repository-owned local/remote branch, or any repository-owned tag. Also
    verify that no tracked report contains the old commit list or hashes.
11. Verify the remote object ID and clean baseline after the push. Do not bypass
    branch protection. If policy rejects the rewrite, report the exact blocker.
12. Create `topic/m99-native-crt-rebuild` from the rewritten `main`. All new M99
    work occurs on this branch and returns through the repository's normal
    review/merge process.

If old M99 changes are mixed inside a commit that also contains unrelated work,
recreate the unrelated patch manually and document the split. Do not drop the
whole mixed commit. If the M99 boundary cannot be proven, stop before rewriting
history and report the candidate commits and ambiguity.

“Discard in full” means no rejected implementation, evidence, commit, ordinary
repository-owned ref, or old-hash report is present in the rewritten `main` or
the new M99 branch.

Git history rewriting cannot guarantee physical erasure from third-party
clones, forks, provider caches, closed pull-request pages, provider-managed
pull refs, audit logs, or backups outside this repository's control. Do not
claim otherwise. If the requirement is provider-wide physical erasure rather
than removal from `main` and repository-owned refs, report `BLOCKED` with the
specific hosting-provider administration or support action required.

## 3. Mandatory clean-baseline proof

Before the new implementation begins:

- search tracked files and generated release inputs for `M99`, `librashader`,
  old CRT assets, old loader files, old tests, and old packaging scripts;
- distinguish legitimate pre-M99 text from rejected additions;
- confirm the original SDL presentation path builds and runs;
- run the repository's normal unit, headless, screenshot, and packaging tests;
- record the rewritten `main` object ID and a clean `git status`; and
- commit a short removal/baseline report before adding new feature code.

No previous CI run, benchmark, shader audit, or real-hardware claim counts as
evidence for this replacement M99.

## 4. Fixed architecture

```text
Emulation framebuffer
        |
        v
Backend-neutral FrameInput
        |
        v
NativePresenter interface
        |
        +-- Windows: D3D11 device/context/swap chain/textures
        +-- Linux: SDL GL context/textures/FBOs
        +-- macOS: MTLDevice/queue/CAMetalLayer/textures
        |
        v
librashader filter chain or native pass-through
        |
        v
Native present
```

`FrameInput` must explicitly carry pixels, width, height, pitch, pixel format,
row origin, source aspect ratio, frame count, source frame rate, and frame-time
delta. It must not expose D3D11, GL, Metal, SDL texture, or librashader types.

Define one backend-neutral lifecycle:

```text
Unavailable -> Initializing -> PassThrough -> Filtered
                         |          ^           |
                         +----------+-----------+
                              recover/fallback
```

All device creation, filter-chain calls, parameter changes, frame submission,
resize, recovery, and destruction must occur on the documented presentation
thread. Backend resources must not cross API boundaries.

### Windows/D3D11

- Obtain the native window handle through the supported SDL2 system-window API.
- Own the D3D11 device, immediate context, DXGI swap chain, render-target views,
  source texture, and required synchronization.
- Use the pinned librashader D3D11 C API directly.
- Handle resize, occlusion/minimize, device-removed/reset, fullscreen changes,
  and swap-chain recreation without leaving a black screen.
- WARP may be used for smoke tests, but never as performance acceptance.

### Linux/OpenGL

- Request and verify an OpenGL 3.3+ core context before enabling the feature.
- Load functions with `SDL_GL_GetProcAddress` or the established project loader.
- Own the source texture, framebuffer objects, output viewport, swap interval,
  and GL state restoration required by librashader.
- Handle drawable-size changes and context recreation.
- llvmpipe may be used for smoke tests, but never as performance acceptance.

### macOS/Metal

- Build the bridge as Objective-C++ (`.mm`) where Objective-C Metal types are
  required by the librashader C header.
- Create an SDL Metal view, obtain its `CAMetalLayer`, and explicitly associate
  the selected `MTLDevice` with that layer.
- Own the command queue, source texture, drawable acquisition, command buffers,
  output texture, synchronization, and presentation.
- Use drawable pixel size, not logical window size, for Retina output.
- Handle nil drawables, resize, minimize, fullscreen, display changes, and
  device/resource recreation. Test Apple Silicon and Intel where supported.
- Do not create or require an OpenGL context on macOS.

## 5. librashader and licensing policy

- Maintainer amendment: static linking through the official C API is required
  for every M99 QA and release build. Dynamic loading remains available only
  as an explicitly non-release developer diagnostic configuration.
- Use an exact, tested stable release. Record version, commit,
  C API, ABI, binary hashes, upstream URL, and platform artifact names.
- Select librashader under MPL-2.0. VAeg remains BSD-2-Clause.
- Keep upstream implementation sources separate from VAeg-owned BSD files.
- For static builds, record the target toolchain, selected runtime features,
  dependency licenses and corresponding-source access; embed required notices.
- Validate the pinned C API and ABI at the static-build audit boundary. Static
  builds resolve required symbols at link time and must not require a
  `librashader.dll`, `librashader.so`, or `librashader.dylib` at startup.
- Standard shader assets may be embedded and extracted to a content-addressed
  cache under the working directory. User parameters remain in vaeg.cfg.
- Treat a missing library, unsupported GPU API, compile error, invalid preset,
  or runtime failure as an optional-feature failure, never an emulator crash.
- Do not inherit the previous M99 version pin or audit result without repeating
  and recording the checks for this implementation.

The bundled preset candidate is `crt-lottes-fast.slang`, including scanlines,
mask simulation, and curvature. Pin and audit the exact source, includes,
textures, preset, and license closure before bundling it. If the complete closure
is not permissive, bundle only a VAeg-owned BSD-2-Clause pass-through shader or
a newly written BSD-2-Clause CRT shader.

Do not commit or distribute CRT-Geom, CRT-Royale, Mega Bezel, the complete
`slang-shaders` tree, GPL shader code, or unknown-license shader content. A user
may explicitly select a separately obtained `.slangp`; user files are not copied
into VAeg releases.

## 6. Capture, emulation, and fallback boundaries

Canonical deterministic capture remains upstream of all GPU processing:

```text
Emulation framebuffer -> raw capture -> golden comparison
```

Filtered capture, if implemented, is a distinct user-facing operation:

```text
Native GPU output -> filtered capture -> visual evidence only
```

M99 must not change VRAM, CPU timing, emulated scan timing, framebuffer bytes,
headless behavior, or raw golden-image semantics. Do not use post-shader frames
as byte-exact cross-GPU goldens.

CRT disabled, unavailable, or failed must still display the correct image. A
total native-backend initialization failure returns to the existing SDL
renderer. A filter failure while the native backend remains healthy returns to
that backend's pass-through path.

## 7. Proposed file placement

Follow a proven existing repository convention if it conflicts with this
proposal; document any deviation.

```text
docs/
├── agents/tasks/M99_librashader_crt_pipeline.md
├── agents/reports/m99-clean-main-report.md
├── agents/reports/m99-final-report.md
├── architecture/native-crt-presentation.md
└── licenses/THIRD_PARTY_NOTICES.md

sdl2/
└── librashader/
    ├── native_presenter.h
    ├── native_presenter.cpp
    ├── presenter_factory.cpp
    ├── frame_input.h
    ├── librashader_loader.h
    ├── librashader_loader.cpp
    ├── shader_preset.h
    ├── shader_preset.cpp
    ├── shader_parameters.h
    ├── shader_parameters.cpp
    └── backends/
        ├── d3d11_presenter.h
        ├── d3d11_presenter.cpp
        ├── gl_presenter.h
        ├── gl_presenter.cpp
        ├── metal_presenter.h
        └── metal_presenter.mm

third_party/librashader/
├── README.vaeg.md
├── LICENSE-MPL-2.0.txt
└── include/librashader_ld.h

assets/shaders/crt/
├── vaeg_crt_default.slangp
├── shaders/crt-lottes-fast.slang
└── licenses/
    ├── CRT_DEFAULT_LICENSE.txt
    └── CRT_DEFAULT_PROVENANCE.md

tests/frontend/librashader/
├── test_frame_input.cpp
├── test_presenter_state.cpp
├── test_runtime_loader.cpp
├── test_shader_parameters.cpp
├── test_fallback.cpp
└── platform_smoke/
```

Platform binaries belong in generated build and release staging directories,
not architecture-specific source directories. Release archives must include
the exact runtime license, source offer/reference, shader notice, provenance,
and third-party notices.

For a static single-executable package these notices may be embedded in About
instead of distributed as separate files. Keep the archive/source/notice audit
manifest as build evidence. M99 packages must start without a librashader
shared library or external standard shader assets; system graphics components
may remain external. Dynamic-runtime absence tests are not an M99 release
acceptance condition because dynamic loading is not a release configuration.

## 8. Small sub-milestones

Each sub-milestone receives its own focused commit or a clearly identified
commit section. Do not claim it complete without its acceptance evidence.

### M99a — Map the rejected history
Identify `PRE_M99`, `OLD_MAIN`, rejected commits/merges, and retained later work.

### M99b — Prove the removal boundary
Verify the rejected set and retained work without creating a persistent old-log ref.

### M99c — Reconstruct main without M99
Replay only retained work onto `PRE_M99`; exclude every rejected M99 patch.

### M99d — Prove and publish clean main
Build/test the candidate, verify the lease, update remote `main`, and re-verify.

### M99e — Record the clean baseline
Add `m99-clean-main-report.md`; prove the old renderer and QA paths are intact.

### M99f — Inventory the current frontend
Trace framebuffer, capture, headless, resize, HiDPI, fullscreen, and threads.

### M99g — Pin and audit dependencies
Select and record the new librashader pin, loader header, binaries, and licenses.

### M99h — Define common contracts
Implement `FrameInput`, presenter states, results, errors, and backend factory.

### M99i — Protect capture semantics
Add tests proving raw captures and the emulation framebuffer are unchanged.

### M99j — Add common pass-through tests
Test conversion, aspect, viewport, failure transitions, and repeated teardown.

### M99k — Establish the macOS build bridge
Add Objective-C++ compilation, Metal frameworks, loader macros, and stubs.

### M99l — Implement Metal pass-through
Create the device/layer/queue/textures and present correct Retina output.

### M99m — Integrate librashader Metal
Create, render, parameterize, and destroy a Metal filter chain on one thread.

### M99n — Harden Metal lifecycle
Cover resize, fullscreen, nil drawable, minimize, and resource recreation.

### M99o — Establish the Windows D3D11 backend
Create the HWND-bound device/context/swap chain and robust output resources.

### M99p — Implement D3D11 pass-through
Upload every supported source mode and present with correct aspect and resize.

### M99q — Integrate librashader D3D11
Create, render, parameterize, and destroy the D3D11 filter chain safely.

### M99r — Harden D3D11 lifecycle
Cover resize, fullscreen, occlusion, device removal, and recreation.

### M99s — Establish the Linux OpenGL backend
Create and verify a 3.3+ core context, loader, textures, FBOs, and state rules.

### M99t — Integrate librashader OpenGL
Implement pass-through and filtered rendering, resize, swap, and context rebuild.

### M99u — Implement common presets and parameters
Enumerate metadata, clamp values, apply live changes, reset, and persist safely.

### M99v — Add the settings UI
Provide enable, preset, parameter, reset, reload, and clear failure status.

### M99w — Complete fallback and headless behavior
Test missing/incompatible runtime, bad presets, GPU failures, and headless bypass.

### M99x — Finish shader and release packaging
Complete the new license audit and inspect each staged platform archive.

### M99y — Run the cross-platform QA matrix
Run CI plus real Windows, Linux, and macOS GPU lifecycle/performance tests.

### M99z — Finalize documentation and evidence
Write architecture, user, troubleshooting, provenance, and final goal reports.

## 9. Required gates

### G99-1 — Rejected M99 absent from main

- Remote `main` has the verified rewritten object ID.
- All unrelated commits and working-tree content are preserved.
- No rejected M99 code, test, asset, package entry, report claim, or commit
  remains in the rewritten mainline.
- No ordinary repository-owned branch or tag retains the rejected commits, and
  no tracked file reproduces their hashes or commit log.
- The old renderer-only baseline builds and passes its applicable tests.

### G99-2 — Architecture and emulation isolation

- No backend type crosses the common presenter contract.
- The emulation core has no D3D11, OpenGL, Metal, SDL-renderer, or librashader
  dependency.
- Raw captures match the clean baseline for representative video modes.

### G99-3 — Windows/D3D11 complete

- Real D3D11 hardware renders pass-through and the default CRT preset.
- Resize, HiDPI, fullscreen, minimize/restore, toggle, and device recovery pass.
- Runtime/preset failures visibly fall back without crash or black screen.

### G99-4 — Linux/OpenGL complete

- A real OpenGL 3.3+ GPU renders pass-through and the default CRT preset.
- Resize, HiDPI, fullscreen, minimize/restore, toggle, and context rebuild pass.
- Mesa software rendering is smoke evidence only, not the hardware gate.

### G99-5 — macOS/Metal complete

- Real Metal hardware renders pass-through and the default CRT preset.
- Retina drawable sizing, resize, fullscreen, minimize/restore, toggle, nil
  drawable handling, and resource recreation pass.
- The macOS build and runtime do not create an OpenGL context.

### G99-6 — Licensing and packaging complete

- VAeg remains BSD-2-Clause and librashader is consumed under MPL-2.0.
- The static archive, source/license audit, preset, shader closure, hashes, and
  notices are recorded.
- No GPL or unknown-license shader is tracked or shipped.
- A clean-machine staged static archive starts without any librashader shared
  library and without external standard shader assets.

### G99-7 — Performance and release readiness

- The default CRT preset sustains the emulator's 60 Hz target at 1920x1080 on
  representative real hardware for all three platforms without unbounded GPU
  resource growth or presentation-thread stalls.
- Report average, p95, maximum frame/presentation time, hardware, driver/OS,
  resolution, preset, and measurement method. Do not substitute software GPU
  results for this gate.
- CI, unit, headless, raw-capture, platform smoke, lifecycle, and package tests
  pass, or unrelated pre-existing failures are proven and documented.

## 10. Non-goals

- Vulkan, D3D12, wgpu, OpenGL on Windows/macOS, or Metal on non-Apple systems.
- A CPU CRT implementation.
- Reusing or repairing the rejected M99 implementation.
- Bundling GPL shader suites or an online shader downloader.
- A shader source editor.
- Replacing deterministic raw screenshots with filtered screenshots.
- Rewriting unrelated history or bypassing repository protection.

## 11. Final report contract

Finish with `docs/agents/reports/m99-final-report.md` containing:

- the rewritten `main` object ID, confirmation that the lease-protected rewrite
  succeeded, and confirmation that rejected commits are unreachable from all
  ordinary repository-owned refs; do not reproduce old commit hashes or logs;
- clean-baseline build/test evidence;
- final branch and commit range;
- backend ownership and lifecycle summary for D3D11, OpenGL, and Metal;
- librashader version/commit/API/ABI, binary hashes, and artifact names;
- shader dependency/license closure and shipped file manifest;
- CI URLs and real-hardware evidence for every gate;
- benchmark method and results;
- staged archive inspection for all platforms;
- final `git status --short`; and
- one of `DONE` or `BLOCKED`.

Do not report `DONE` while any required platform gate relies only on a virtual
display, WARP, llvmpipe, unavailable hardware, or an uninspected package. Use
`BLOCKED` with completed work, exact evidence, and the smallest remaining action.

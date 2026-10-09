# AGENTS.md — vaeg (88VA Eternal Grafx)

PC-88VA emulator derived from Neko Project II. Upstream (project-vaeg)
is abandoned; this fork is the living tree.

The active tree is the portable CMake build: C core, SDL2 frontend under
`sdl2/`, Dear ImGui GUI, and macOS / Linux / Windows-MinGW support. It
uses `cpu/upd9002/` for the main CPU instruction core, the built-in CPU
register model in `io/upd9002_regs.*`, the uPD70008-compatible main-CPU
adapter in `cpu/upd9002_upd70008.*`, the uPD780-compatible FDC instance in
`io/subsystem.cpp`, the shared suzukiplan-backed instruction implementation
in `cpu/z80_compat_*`, `sound/opngenc.c` for OPN generation (never define
`OPNGENX86`), and `memoryva/memoryva.c` for the VA memory layer.

## CPU role and file naming

The hardware-facing names are deliberately distinct:

- `cpu/upd9002_upd70008.*` is the uPD9002 main-CPU adapter's uPD70008-
  compatible mode. It is not the FDC CPU.
- `io/subsystem.cpp` owns the FDC CPU and exposes it as `UPD780C`.
- `cpu/z80_compat_cpu.*`, `z80_compat_bus.h`, `z80_compat_registers.h`, and
  `z80_compat_state.*` are the common Z80 compatibility implementation and
  compatibility layer shared by those two adapters. `cpu/upd780/upd780_disasm.*`
  is the FDC-facing uPD780 disassembler. The remaining Z80 terminology refers
  only to instruction-set, vendor, and historical save-state compatibility; it
  does not identify a separate FDC device.

Keep the shared `z80_compat_*` backend separate from the hardware-specific uPD780C and
uPD70008 layers. When changing hardware-facing code, use the role-specific
uPD70008/uPD780 names above. The stable `UPD9Z80` save-state section name is
also retained for existing state-file compatibility.

The former frozen reference tier (`win9x/`, `i286x/`,
`cpuxva/memoryva.x86`, and `hlp/`) was removed from the current tree in
M57. Its exact G56 snapshot remains available at the annotated tag
`archive/frozen-win9x-i286x-g56` for behavior archaeology and provenance.
Normal fixes land in the active CMake/C/SDL2 tree.

## How work is organized

All work is milestone-driven. Read, in this order:

1. `docs/agents/ROADMAP.md`       — milestone sequence and gates
2. `docs/agents/CONVENTIONS.md`   — repo invariants and new-code rules
3. `docs/agents/tasks/M*.md`      — the single task file you were assigned

Do exactly ONE milestone task per session. Every milestone ends at a
HUMAN GATE unless the task file states the gate is machine-verifiable.
The standard human gate is: build from clean checkout, boot in V3 mode,
run the bundled VA demo, boot an OS and perform simple operations.
The clean-checkout build is the fully static MinGW build described under
"Clean-build handoff"; give the maintainer the absolute path of `vaeg.exe`.
Never begin milestone N+1 until the user states that gate N passed.
Always push the branch and report the exact commit SHAs when done.

## CI and expensive-test discipline

Do not use hosted CI as an iterative debugger. Before requesting a hosted CI
run, complete the applicable local builds, tests, static validators, repository
checks, and final-commit scope checks. Confirm that the intended commit chain
and generated-evidence layout are final.

Preserve expensive intermediate results in commits or untracked, task-specific
work directories. Record the evaluated commit, worker-binary digest, corpus
identity, comparison-contract identity, target-policy identity, exact command,
exit status, and output digests. Reuse a completed SST or other expensive run
when these identities are unchanged and the task contract permits reuse. A
documentation-only, report-only, evidence-only, or validator-only edit does
not by itself justify rerunning unchanged emulator behavior; prove that the
build-relevant inputs and worker digest are unchanged instead. Never rerun an
unchanged failing hosted job without first retrieving its exact log and
addressing or explaining the failure locally.

Push and run hosted CI only after local validation and commit ordering are
settled. Prefer one final hosted run for the review candidate. If a hosted-only
failure requires another attempt, preserve the prior results, make the
smallest justified correction, rerun the affected local check, and avoid
repeating unrelated jobs or profiles.

## BIOS analysis documentation — canonical placement

**Publish BIOS analysis documents under `docs/bios/`.** Do not place new
interrupt/service reference documents under `docs/modernization/`, in the
emulator-source `bios/` directory, or only in a private work directory.
Use lowercase Markdown filenames based on the interrupt number, for example
`docs/bios/bios_int80h.md`; keep AH subfunctions in that interrupt's document.
Manual chapter numbers such as `601` are source references, not canonical
filenames. Include conventional-memory work layouts when established, and
clearly separate manual specifications, static ROM findings, and runtime
verification or unresolved hypotheses.

Publish independently written analysis only. Private source paths, asset
filenames and hashes, raw disassemblies, manual copies, ROMs, disk images,
and raw captures remain outside Git under the existing private-asset policy.

## Permanent bug-fix ledger

`docs/modernization/bug-fixes.md` is the permanent correctness ledger for
this fork. Update it in the same milestone and normally in the same commit
whenever a change fixes incorrect guest-visible behavior, a portability
crash or data-corruption risk, persistence/media handling, or a regression.

Each entry must identify the symptom, affected scope, demonstrated root
cause, correction, verification, milestone/task, and commit. Do not record a
hypothesis as a root cause. Keep unresolved defects in the ledger's open
section until evidence and a tested correction exist. Feature additions and
cosmetic restorations do not belong there unless they also correct a concrete
defect. Evidence documents and fixing commits must be clickable Markdown
links; commit links use the repository's GitHub commit URL with the full SHA.
Release notes may summarize the ledger but do not replace it.

## Repository invariants (steady state since phase 1)

- Sources and documentation are UTF-8 without BOM. Never introduce CP932
  content.
- EOL is LF throughout the current tree. Tool-mandated project formats remain
  declared in `.gitattributes`; do not add new CRLF files without an explicit
  repository-policy exception.
- Tracked paths are lowercase except tool- or project-mandated names listed
  by `tools/repo/check_case.py`, including top-level `CHANGES*.md` release
  notes. New paths must otherwise be lowercase.
- Never modify binary payloads: `romimage/`, ROM images, fonts, icons,
  cursors, or wave data.  A narrowly scoped distribution exception permits
  committing a reproducibly generated `.d88.xz` only when its directory has
  an explicit allow-list, the image listing proves that every stored file is
  a source-built and freely distributable payload, and the compressed image
  round-trips byte-for-byte to its generated raw D88.  A disk may be bootable
  if it meets these conditions; a disk that needs PC-Engine, NEC system files,
  or other non-redistributable payloads to boot does not.  The raw D88,
  PC-Engine system files, private media, ROMs, and source templates remain
  untracked and must never be committed.
- Treat private integration asset identities as sensitive. Tracked files must
  use neutral stable test identifiers; do not record private filenames,
  absolute paths, or hashes unless the maintainer explicitly authorizes that
  exact metadata. Keep raw private screenshots, logs, traces, and save files
  outside Git, and never stage, commit, or push them.
- Never re-encode, re-wrap, or reformat lines you are not changing.

## Hard rules

- One concern per commit. Renames (`git mv`) go in rename-only commits;
  reference fixups go in a separate, immediately following commit.
- Deletions happen only from a list the user has explicitly approved.
- New files created in phase 2 carry a 2-clause BSD header
  `Copyright (c) 2026 Nakata Maho` (see CONVENTIONS.md §New code).
  Never alter copyright headers of existing files.
- Do not restore the M57-deleted reference-tier paths (`win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, or `hlp/`) unless a task explicitly requires it.
  Use tag `archive/frozen-win9x-i286x-g56` for historical comparison.
- Core code (root, `io/`, `sound/`, `cbus/`, `vram/`, `*va/`,
  `cpu/upd9002/`)
  stays C. C++17 is allowed under `sdl2/` (frontend + GUI) and, when an
  approved third-party CPU core requires it, for CPU backends and thin
  compatibility adapters under `cpu/`. C++ and STL types must not cross
  existing C-facing subsystem or state-save interfaces. Other newly written
  emulator-core code remains C99 unless an ADR separately approves it.
- Vendored third-party code lives under `external/` with the exact
  version recorded in `docs/agents/DECISIONS/`. Never hand-edit it.
- Run the machine checks named in your task file (`tools/repo/*.py`,
  cmake builds) and paste their output into the PR description.

## Validator design and negative-test isolation

Design new evidence and repository validators as three independently callable
layers:

1. content validation for schemas, counts, hashes, classifications, and
   semantic evidence;
2. protected-history validation for explicitly enumerated approved artifacts;
3. Git-topology validation for evaluated/evidence commit ordering and
   evidence-only commit scope.

The production entry point may compose all three layers, but focused tests
must call the smallest relevant layer. Do not make a content-mismatch test
depend on the current Git history, and do not make a commit-topology test
depend on a large real evidence corpus. Exercise Git-topology checks in a
purpose-built temporary repository.

Every fail-closed negative test must:

- start from a fixture that passes all checks relevant to that test;
- apply exactly one controlled mutation;
- assert a stable, machine-readable error code for the intended invariant,
  not merely a nonzero exit status or message substring;
- fail if an unrelated validator masks the intended failure;
- retain one final integration test that composes every layer.

Use explicit protected artifact manifests or closed path prefixes. Never
protect an open-ended parent directory when later milestones legitimately add
new children beneath it. Generators must write out of tree and be validated as
generated output before repository-placement or commit-scope checks run.
Generation must not require the future evidence commit to exist.

Keep stable error codes separate from explanatory text, for example
`M60D_FRAME_SET_MISMATCH` versus
`M60D_EVIDENCE_COMMIT_SCOPE`. A test for one code must not pass when it
receives the other. Do not add production bypass flags merely to make a
negative test reach its intended branch; isolate the underlying validator
function instead.

## Build reference

- Active: CMake >= 3.20. `cmake --preset linux-debug` etc. SDL2 is
  discovered via find_package/pkg-config by default, with the
  ADR-0006-pinned FetchContent path only where a preset opts into it.
  Dear ImGui is vendored under `external/imgui`.
- Archived reference: the former VS2017 v141/Win32 tree is available only at
  tag `archive/frozen-win9x-i286x-g56`; it is not a current build target and
  has no CI or compile-guarantee coverage.

### Clean-build handoff

Whenever a clean build is needed, including the "build from clean
checkout" step of a human gate, produce a fully static MinGW-w64 build of
the pushed commit with:

```sh
tools/release/build-mingw-static-container.sh --commit <full-sha>
```

The script clones the commit into a new `../vaeg-mingw-<short-sha>/`
directory, builds in a pinned Ubuntu 24.04 container with the `mingw-cross`
preset (static SDL2, LibArchive, zlib, xz, librashader, and MinGW runtimes),
verifies that the commit and date are embedded, and audits the PE imports
down to Windows system DLLs. Never reuse an existing build tree for this.
Hand the maintainer the absolute path of the resulting `vaeg.exe`, its
SHA-256, and the absolute path of `BUILD-REPORT.md` in the same `output/`
directory.

### Maintainer release handoff

When the maintainer says to copy release builds to "the usual place", the
destination is:

`/mnt/c/Users/maho/Dropbox/Documents/Emulators/PASOCON/NEC PC-88VA/vaeg_new/`

- Copy `build/linux-release/sdl2/vaeg` there as `vaeg`.
- Copy the MinGW release executable there as `vaeg.exe`; its source is
  `build/mingw-release/sdl2/vaeg.exe` for a native build or
  `build/mingw-cross/sdl2/vaeg.exe` for the Linux cross-build.
- Verify each copied executable against its build artifact with `cmp` or
  SHA-256.

This is a maintainer-local directory outside Git. Unless explicitly requested,
replace only `vaeg` and `vaeg.exe`; do not alter or inventory its ROMs, disk
images, configuration, save data, or other private integration assets.

## Commit messages

UTF-8, LF, English subject, canonical `M<id>:` prefix. The ID is an integer
or an integer followed by one lowercase letter and an optional nonzero decimal
suffix, such as `M58:`, `M60a:`, or `M62b1:`.

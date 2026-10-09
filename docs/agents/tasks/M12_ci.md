# M12 — CI: GitHub Actions 3-OS matrix + ROM-less tests

Status: not-started
Branch: topic/m12-ci
Gate: G12 (machine: green pipeline on the fork; user review of workflow)
Depends on: G11 passed.

## Goal

Every push/PR builds the portable tree on ubuntu-latest, macos-latest,
and windows-latest (MSYS2/MinGW), runs the headless smoke, the repo
invariant checkers, and a small ROM-less unit-test suite.

## Pipeline scope

Per-OS job:
1. Install SDL2 (apt / brew / MSYS2 pacboy — pin package names in the
   workflow, comment the known drift risk for MSYS2).
2. cmake --preset <os> && cmake --build.
3. SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./vaeg --smoke
   (VA machine, GUI frame included).
4. ctest (see below).
5. Upload the binary as a build artifact (no release automation).
   Windows artifacts must include `SDL2.dll` next to `vaeg.exe`; the
   M11 `VAEG_FETCH_SDL2` option is only a build/link acquisition path
   and does not solve runtime distribution.

One additional lint job (ubuntu):
- tools/repo/check_encoding.py, check_eol.py, check_case.py — the
  phase-1 invariants become permanently enforced here.

## ROM-less unit tests (VAEG_ENABLE_TESTS=ON)

No ROM, no disk image, no guest execution. Candidates, smallest first:
- codecnv round-trip: CP932↔UTF-8 including wave-dash 0x8160↔U+FF5E
  (the M6 invariant, now a permanent test).
- statsave: struct round-trip on a zeroed core (save → load →
  byte-compare the sections that must be stable).
- memoryva.c table sanity: map-register decode against a fixture table
  extracted from the asm during M9 (if M9 produced one; else skip and
  note).
- ini reader: UTF-8 config parse/write round-trip.
Use plain assert-style C tests or a single-header framework vendored
under external/ (ADR if vendoring).

## Constraints

- NOTHING in CI touches ROMs or disk images; the human gate remains the
  only behavioral verification. State this in a comment at the top of
  the workflow file.
- No code signing, no release/tag automation, no scheduled jobs.
- Workflow files: .github/workflows/build.yml (matrix) + lint steps.
  Keep it to the two files; no reusable-workflow abstraction yet.
- Badge in README.md; CI scope and limits documented in
  docs/modernization/BUILD.md.

## Machine checks (paste into PR)

    URL of a green run on all three OSes on the fork
    ctest output
    tools/repo checkers locally

## Do not do

- Do not add caching contraptions before the pipeline is proven green.
- Do not gate merges on VAEG_WERROR.
- Do not let CI auto-fix anything (no format bots, no auto-commit).

## Risks to report

- MSYS2 package name drift; macOS SDL2 pkg-config path issues.
- Any test that is timing-dependent (there should be none; a flaky
  test is a defect to remove, not retry).

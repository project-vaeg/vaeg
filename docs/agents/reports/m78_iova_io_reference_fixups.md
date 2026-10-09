# M78: VA I/O reference-fixup report

## Scope

M78 continues from the current `main` checkout after the M77 `iova/` to `io`/
move. It normalizes only current-tree path references; it does not change
guest behavior, production symbols, dispatcher ownership, or state-save names.

Base: `9dc8cba6d5ea7ec190040f64f6b2cd3753267808` (`main`)
Candidate branch: `topic/m78-iova-io-reference-fixups`
Candidate gate: G78

## Changes

- `CMakeLists.txt` now adds `io/` to the active include search paths.
- `docs/modernization/virtual-machine-architecture.md` now names the active
  I/O layer as `io`.
- `docs/agents/ROADMAP.md` records G78 approval and main integration while
  keeping M79 and M80 status explicit.

## Deliberate exclusions

- Approved historical reports, task archives, and generated campaign
  inventories retain their original path evidence.
- `tools/qa/upd9002_rename.py` retains `iova/...` entries that assert retired
  paths or reject obsolete CMake tokens; these are negative checks, not active
  source references.
- Existing untracked user documents were not edited or staged.

## Validation

Validation completed on the current `main` candidate.

- `git diff --check`: PASS.
- Current-path audit: PASS; remaining matches are historical evidence or intentional retired-path checks.
- Repository validators and native build: PASS (`check_case`, encoding, EOL, `upd9002_rename.py`, `linux-debug`).
- Runtime selftest: PASS (`build/linux-debug/sdl2/vaeg --selftest`, all tests passed).
- `ctest --test-dir build/linux-debug`: no tests were registered in this preset.

## Gate status

G78 human gate passed by the maintainer on 2026-08-11 for candidate
`a86365584ffd86973b618bdaf55c26214798a1f0`. The candidate is merged to
`main` by the gate-closure commit; M79 may now start and M80 remains
unstarted.

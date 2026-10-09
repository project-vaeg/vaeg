# M13 — Retirement of legacy paths + phase 2 closeout

Status: not-started
Branch: topic/m13-retire
Gate: G13 (human sign-off on the deletion list, then final VA gate)
Depends on: G12 passed.

## Goal

Reduce the tree to the sustainable core: one build system (CMake), one
frontend (sdl2/), C-only cores. Everything removed is removed from a
user-approved list, after tagging, never before.

## Protocol (identical to M3)

1. Produce the candidate list with evidence (nothing references it in
   the CMake build; CI green without it):
   - sdl/ (SDL1 frontend, superseded by sdl2/)
   - win9x/ (Win32 frontend + its 5 NASM helpers) — DECISION POINT:
     the user may keep it frozen as behavioral reference. Present both
     options with disk/maintenance cost; do not pre-decide.
   - i286x/ (asm CPU core) and cpuxva/memoryva.x86 — same decision
     point; note that deleting them deletes the M9 behavior reference.
   - Leftover .dsp/.dsw/.vcxproj/.sln if win9x/ goes.
   - hlp/ (CP932 HTML Help) — only if win9x/ goes; the SDL2 build
     never shipped it.
2. Wait for explicit user approval item by item.
3. Tag `pre-retirement` on the last commit containing everything.
4. Delete in one commit per top-level directory; no content changes
   mixed in.
5. Update AGENTS.md, ROADMAP.md, README.md to the post-phase-2 state;
   drop the LEGACY-build language; record final ADR summarizing what
   was kept and why.
6. Tag `phase2-complete` after the final gate.

## Final gate G13 (human)

Full VA checklist on all three OSes from a clean clone of the pruned
tree, following only README/BUILD.md instructions (documentation test:
if the user needs tribal knowledge to build, the docs fail the gate).

## Do not do

- Nothing is deleted before the user approves the list AND the
  pre-retirement tag is pushed.
- No refactoring rides along with deletions.
- .gitattributes CRLF exceptions are removed only if the last CRLF
  file types actually left the tree (verify, do not assume).

# M3 — Remove unreferenced files (approved list only)

## Goal
Delete files reachable from no build (VS2008 project, v141 project,
`sdl/` build) and no documentation purpose, so that M4–M6 mechanical
passes touch the minimum set.

## Protocol — deletion is two-phase
1. **Propose.** Regenerate the candidate list:
   `python tools/repo/find_unreferenced.py --report`
   Then triage it manually into `docs/agents/reports/m3_prune_list.md`
   with three sections:
   - DELETE: not referenced by any build root, not documentation, not a
     data payload, and (worth checking) no in-tree textual references
     (`git grep -il <basename>`).
   - KEEP: unreferenced but retained — say why (docs, licenses, HLP
     help sources, ROMIMAGE/bios/font payloads, frozen-backend files
     the user wants kept, historical readme.txt).
   - UNSURE: anything the reference scanner cannot classify (NASM
     %include chains, files pulled in via `.tbl` inclusion, generated
     headers). UNSURE items are NOT deleted this round.
2. **Wait.** The user approves/edits the DELETE section. No deletion
   before explicit approval.
3. **Execute.** Single commit, `git rm` only, no content changes:
   `M3: remove unreferenced files (approved list)`. Update project
   files in the same commit ONLY if they referenced a deleted file
   (this is the one allowed mixed change; call it out in the PR).

## Build roots shield their sources
`find_unreferenced.py` treats every project/build file as a root, so
`Win9xC/np2c.dsp` keeps `I286C/` and other non-VA files "referenced".
Whether `Win9xC/` (and `I286A/`, frozen backends) are themselves pruned
is a user decision to raise explicitly in the triage document; if the
user approves deleting a build root, delete it first, re-run the
scanner, and fold the newly exposed files into the same triage round.

## Standing exclusions (never propose)
`ROMIMAGE/`, `bios/`, `font/` payloads; licenses; `docs/`;
`AGENTS.md`; `.gitignore`; anything under `sdl/`.

## Machine checks
- Both builds (VS2008, v141) pass after deletion.
- `find_unreferenced.py` output shrinks accordingly; DELETE section is
  now empty.

## GATE G3 (human)
Standard checklist on both binaries.

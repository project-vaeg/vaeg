# PROMPTS_PHASE2 — per-session Codex prompts

Operating pattern (unchanged from phase 1): one milestone per session;
the prompt stays short and delegates everything to the task file.
Codex reads AGENTS.md automatically from the repo root; the prompt only
needs to name the task file and the branch, and restate the two rules
Codex most often drops (push + report SHAs; stop at ambiguity).

Template:

    Read AGENTS.md, docs/agents/ROADMAP.md, docs/agents/CONVENTIONS.md,
    then execute exactly docs/agents/tasks/<TASK>.md on branch <BRANCH>.
    Do nothing outside that task file. If the task file and the repo
    disagree, or a decision point is reached, STOP and report instead of
    choosing. When done: run the machine checks listed in the task file,
    push the branch, and reply with the commit SHAs and the check output.

## Session prompts

M7:
    Read AGENTS.md, docs/agents/ROADMAP.md, docs/agents/CONVENTIONS.md,
    then execute exactly docs/agents/tasks/M7_cmake_core.md on branch
    topic/m7-cmake-core. Do nothing outside that task file. If the task
    file and the repo disagree, or a decision point is reached, STOP and
    report. When done: run the machine checks, push, reply with SHAs and
    check output.

M8:
    Same preamble; execute docs/agents/tasks/M8_sdl2_frontend.md on
    topic/m8-sdl2-frontend. Port module by module in the order given,
    one commit per module. Gate G7 has passed.

M9 (highest risk — split into two sessions):
  Session A (analysis only, no code):
    Execute step 1 of docs/agents/tasks/M9_va_portable.md only: produce
    docs/agents/reports/m9_memoryva_map.md (routine inventory of
    cpuxva/memoryva.x86 with register contracts and C-callable
    equivalents per memoryva.h). Study the i286x/*.x86 ↔ i286c/*.c pairs
    first and describe the transliteration conventions they establish.
    No source changes outside the report. Push and report.
  Session B (after the user reviews the map):
    Execute the remainder of M9_va_portable.md on topic/m9-va-portable.
    The reviewed m9_memoryva_map.md is authoritative. Faithful
    transliteration only; no optimization; STOP on any routine flagged
    in "Risks to report".

M10 (two sessions — ADRs first):
  Session A:
    Execute steps 1–2 of docs/agents/tasks/M10_imgui.md: GUI-PARITY.md
    plus the two ADR proposals (rendering backend, Japanese font) with
    trade-offs. No implementation. Push and report.
  Session B (after user decides both ADRs):
    Execute the remainder of M10_imgui.md on topic/m10-imgui per the
    decided ADRs.

M11:
    Same preamble; execute docs/agents/tasks/M11_mingw_macos.md on
    topic/m11-mingw-macos. Agent-verifiable scope is Linux + MinGW
    cross/link checks; native Windows and macOS verification is the
    user's. Centralize the UTF-8→UTF-16 boundary in one shim.

M12:
    Same preamble; execute docs/agents/tasks/M12_ci.md on topic/m12-ci.
    Iterate on a ci/ throwaway branch until green, then squash onto the
    topic branch. Paste the green run URL.

M13 (two sessions):
  Session A: produce the deletion candidate list with evidence and the
    two decision-point write-ups (win9x/, i286x/+memoryva.x86). Nothing
    is deleted. Push and report.
  Session B (after per-item user approval and the pre-retirement tag):
    execute the deletions and doc updates per M13_retire_legacy.md.

## Why this shape

- The task files carry all detail; prompts that duplicate detail drift
  out of sync and Codex follows the stale copy.
- STOP-on-ambiguity is restated in every prompt because it decays over
  long sessions.
- M9 and M13 are split because their first half produces an artifact
  the user must review before the second half is safe; M10 is split
  because two ADRs need user decisions.
- Push+SHA reporting is restated because it was the most frequently
  dropped step in phase 1.

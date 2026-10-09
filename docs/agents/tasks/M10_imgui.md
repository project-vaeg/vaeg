# M10 — Dear ImGui GUI on sdl2/

Status: not-started
Branch: topic/m10-imgui
Gate: G10 (human)
Depends on: G8 passed. Parallel with M9 allowed (rebase onto M9 before gate).

## Goal

A Dear ImGui GUI on the SDL2 frontend covering the must-have set, with
a parity document tracking everything the Win32 GUI offers.

## Decisions to make FIRST (each is an ADR in docs/agents/DECISIONS/)

1. Rendering backend: imgui_impl_sdlrenderer2 vs imgui_impl_opengl3.
   Propose with trade-offs (dependency weight on macOS/MinGW, HiDPI,
   texture interop with scrnmng); the user picks.
2. Japanese font: GUI labels are Japanese-capable UTF-8. Reading the
   guest font ROM is FORBIDDEN. Options: bundle a permissively-licensed
   font (record license) vs platform font lookup. Propose; user picks.

## Constraints

- Dear ImGui vendored at a pinned release under external/imgui
  (docking branch NOT used unless ADR says so). Record version.
- GUI code is C++17 under sdl2/gui/ only. It talks to the core through
  the same seams the Win32 menu handlers use (study win9x/ menu +
  dialog code to find the operations, then locate their non-Win32
  equivalents; no new globals into the core).
- Input routing must be explicit and documented: when ImGui wants the
  keyboard/mouse (io.WantCaptureKeyboard/Mouse), the guest does not
  receive the event, and vice versa. This rule lives in sdl2/gui/README.md.
- Menu structure replicates the existing Win32 menu ordering first;
  UX redesign is out of scope.

## Must-have set (gate scope)

- FDD image mount/unmount per drive (file dialog: ImGui-based simple
  browser is acceptable; no native dialogs in this milestone)
- Reset
- State save/load
- Display scale (x1/x2/x3, aspect)
- Sound on/off + volume
- Key/joystick config minimal
- Exit

Everything else from the Win32 menu goes into
docs/modernization/GUI-PARITY.md with status: done / stub-visible /
later. Stubs are shown disabled with "(not implemented)" — visible
gaps, not hidden ones.

## Steps

1. GUI-PARITY.md from win9x menu/dialog inventory.
2. ADR-backend + ADR-font; wait for user decision.
3. ImGui integration into the main loop; input routing.
4. Must-have set; config persisted via the existing ini layer.
5. Update --smoke to render one GUI frame headless.

## Machine checks

    build gcc+clang; --smoke with GUI frame; tools/repo checkers

## Gate G10 (human, Linux)

Full walk of the must-have set with a real disk image; Japanese labels
render; input routing correct (typing in GUI does not leak into guest).

## Do not do

- No debugger UI (debuguty) — list as "later" in parity doc.
- No IME text input work in this milestone; note it as a known gap.
- Do not modify win9x/ GUI.

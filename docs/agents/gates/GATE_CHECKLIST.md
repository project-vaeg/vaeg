# GATE CHECKLIST (manual, performed by the user)

Copy this block into the PR for each gate and fill it in.

```
Gate: G_    Commit: ________    Date: ________
Toolchain(s) verified: [ ] VS2008   [ ] VS2017/v141

Build
[ ] Clean checkout, clean build directory
[ ] Win32 / Release builds with 0 errors
[ ] Binary launches (window appears)

V3 mode
[ ] Boots in V3 mode
[ ] N88-BASIC prompt reachable / expected boot behavior

VA demo
[ ] Bundled VA demo disk runs
[ ] Graphics: no corruption vs previous gate
[ ] Sound: plays, no obvious regression

OS
[ ] OS boots (PC-Engine / DOS as usual)
[ ] DIR / file listing works
[ ] Launch one program, exit cleanly

Japanese text (mandatory at G6, optional before)
[ ] Menus / dialogs / title bar render correctly
[ ] Loading from a path containing Japanese characters works

Result: PASS / FAIL
Notes / regressions:
```

Rules:
- FAIL at any line = gate fails; the milestone branch does not merge.
- Comparisons are against the previous PASSED gate, not against memory.

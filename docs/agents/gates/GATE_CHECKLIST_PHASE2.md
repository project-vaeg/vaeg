# GATE_CHECKLIST_PHASE2 — human verification per gate

General rule: the agent's build logs and smoke runs never substitute
for these steps. A gate passes only when the user states it in the
conversation/PR.

## G7 (M7, machine + review)
- [ ] Agent pasted gcc AND clang full build logs, zero errors
- [ ] tools/repo checkers clean
- [ ] Review m7_source_list.md and every "minimal fix" commit — each
      must be individually justified and behavior-preserving
- [ ] LEGACY v141 build unaffected (statement in PR; spot-build if in doubt)

## G8 (M8, human, Linux) — frontend gate (PC-98 scaffold)
- [ ] Clean checkout, `cmake --preset linux-release`, build
- [ ] Window opens; machine starts (with ROMs if present; defined
      error message without — no crash)
- [ ] Keyboard reaches the guest; audio initializes; clean exit
- [ ] `--smoke` exits 0 headless
- [ ] Tag `portable-pc98`

## G9 (M9, human, Linux) — STANDARD VA GATE
- [ ] Clean checkout build, VA machine
- [ ] V3 mode boot
- [ ] Bundled VA demo runs
- [ ] OS boot; DIR; launch a program
- [ ] Anomalies cross-checked against LEGACY v141 on Windows with the
      same media before filing as a port bug
- [ ] Tag `portable-va`

## G10 (M10, human, Linux)
- [ ] Both ADRs (backend, font) were decided by the user BEFORE code
- [ ] Full walk of the must-have set with a real disk image
- [ ] Japanese labels render; no guest font ROM access
- [ ] GUI typing does not leak into the guest and vice versa
- [ ] GUI-PARITY.md status column is honest (stubs visible, not hidden)

## G11 (M11, human, Windows + macOS)
- [ ] Windows build follows `BUILD.md` from a clean checkout
- [ ] macOS build follows `BUILD.md` with MacPorts SDL2 under `/opt/local`
- [ ] `vaeg.cfg` has `pc_model=88VA1` with `SNDboard=100`/`200`, or
      `pc_model=88VA2` with `SNDboard=200`, plus `clk_base=3993600` and
      `clk_mult=2`
- [ ] Stale-config startup warnings are visible if those VA settings are
      deliberately wrong
- [ ] Standard VA gate executed on real Windows (MinGW build)
- [ ] Standard VA gate executed on real macOS
- [ ] Non-ASCII (Japanese) path to a disk image mounts on Windows
- [ ] GUI must-have walk on both
- [ ] WSLg pacing jitter is not used as the Windows timing reference;
      native Windows is the measurement platform

## G12 (M12, machine + review)
- [ ] Green run URL, all three OSes, on the fork
- [ ] ctest passing; codecnv wave-dash test present
- [ ] Invariant checkers wired as a required job
- [ ] No ROM/disk-image material anywhere in CI

## G13 (M13, human)
- [ ] Deletion list approved item by item (win9x/, i286x/ decisions
      made explicitly)
- [ ] `pre-retirement` tag pushed before any deletion
- [ ] Clean-clone builds on all three OSes following docs only
- [ ] Standard VA gate one final time per OS
- [ ] Tag `phase2-complete`

## G17 (M17, human FM/audio comparison)
- [ ] Vendor metadata and BSD-3-Clause license match ADR-0009; no MAME
      GPL device code is present
- [ ] `OPNGENX86` remains undefined
- [ ] ROM-less selftest exercises NP2, ymfm YM2203, and ymfm YM2608
- [ ] Standard VA gate passes with the NP2 backend
- [ ] Standard VA gate passes with the ymfm backend
- [ ] Backend switching performs a clean reset and retains configured
      FDD/SASI media
- [ ] Known music has correct pitch, tempo, envelopes,
      algorithm/feedback timbre, pan, and relative FM volume under ymfm
- [ ] Channel-3 special mode/CSM is checked when suitable software is
      available
- [ ] SSG, ADPCM-B, and rhythm remain on the NP2 path and do not regress
- [ ] `opn_backend` persists across restart; missing/invalid values use
      ymfm and explicit `np2` still selects NP2
- [ ] VA defaults to YM2203/OPN; selecting Sound Board II switches to
      YM2608/OPNA and survives restart
- [ ] VA2/VA3 defaults to YM2608/OPNA and does not expose OPN-only selection

## G18 (M18, human model/ROM boot)
- [ ] Unsuffixed VA ROMs and extra `vasubsys.rom` are beside the executable
- [ ] The five MAME-compatible VA2 `*_va2.rom` files are beside the executable
- [ ] Emulate -> Boot model -> VA selects `88VA1`, resets, and boots VA
- [ ] Emulate -> Boot model -> VA2/VA3 selects `88VA2`, resets, and boots
      VA2/VA3-compatible behavior
- [ ] FDD and SASI selections survive both model changes
- [ ] The selected model persists across restart and resolves its matching
      ROM filename set
- [ ] A missing model ROM reports the model, expected root, and missing file
- [ ] VA2 does not fall back to unsuffixed VA ROM names
- [ ] A deliberately changed ROM emits a MAME size/CRC32/SHA-1 warning and
      startup continues
- [ ] A stale `biospath=romimage` setting does not reactivate flat lookup

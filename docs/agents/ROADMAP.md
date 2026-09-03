# ROADMAP — vaeg modernization

## Phase 1 (COMPLETE, tag `phase1-complete`)

Toolchain + repo hygiene: VS2008 baseline (M1, tag `baseline-vs2008`,
frozen at `vs2008-final`), VS2017/v141 (M2, tag `baseline-v141`),
prune (M3), lowercase (M4), LF (M5), UTF-8 without BOM (M6, Option A
charset flags). Task files `tasks/M0..M6` are historical record; do not
re-run them.

## Phase 2 — cross-platform (macOS / Linux / Windows-MinGW)

Goal: SDL2 frontend + Dear ImGui GUI + CMake, C-only cores, sustainable
tree. M7-M12 achieved the portable build, VA support on `i286c/`,
three-platform CMake coverage, and CI.

M13 closed phase 2 by removing retired paths and documenting the tier split
that existed at that gate:

- Active tree: CMake/C/SDL2/Dear ImGui; main CPU in `cpu/upd9002/`; VA memory
  in `cpucva/memoryva.c`; Z80 side in the suzukiplan-backed
  `cpucva/z80_compat_cpu.cpp` wrapper with `cpucva/upd780_disasm.cpp`.
- Then-frozen reference tier: `win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, and `hlp/`. The v141 build was decisive in the G9
  defect chain: differential FDC traces, the V30 DMA pump comparison, and
  same-tree A/B isolated the portable defect.
- Removed in M13: retired `sdl/` SDL1 frontend and leftover accessories
  Visual Studio project metadata.

M57 later removed that reference tier from the current tree after preserving
its legal evidence. Its exact G56 snapshot is protected by annotated tag
`archive/frozen-win9x-i286x-g56` and source history, not by a current CI or
compile guarantee.

## Milestone table

Task files whose gates have passed are historical records, not runnable
instructions. See [`tasks/README.md`](tasks/README.md), including the explicit
M36–M41 archive status.

| ID  | Task file                  | Deliverable | Gate |
|-----|----------------------------|-------------|------|
| M0  | tasks/M0_inventory.md      | Inventory report (no repo mutation) | review only |
| M1  | tasks/M1_vs2008_baseline.md | VS2008 Win32 Release builds as-is | **G1** human |
| M2  | tasks/M2_vs2017_v141.md    | v141 build of unmodified code | **G2** human |
| M3  | tasks/M3_prune.md          | Unreferenced files deleted from approved list | **G3** human |
| M4  | tasks/M4_lowercase.md      | All tracked paths lowercase | **G4** human |
| M5  | tasks/M5_eol_lf.md         | LF everywhere except declared CRLF exceptions; `.gitattributes` | **G5** human |
| M6  | tasks/M6_utf8.md           | UTF-8 without BOM sources; charset flags decided | **G6** human |
| M7  | tasks/M7_cmake_core.md     | CMake skeleton; NP2 core libs compile with gcc+clang on Linux; portable `sdl2/compiler.h` | **G7** machine + review |
| M8  | tasks/M8_sdl2_frontend.md  | `sdl2/` SDL2 frontend (video/audio/input/timer/main loop) runs the PC-98 core on Linux | **G8** human |
| M9  | tasks/M9_va_portable.md    | `cpucva/memoryva.c`; VA machine builds and runs on i286c; V3 boot + VA demo on Linux | **G9** human (standard VA gate) |
| M10 | tasks/M10_imgui.md         | Dear ImGui GUI: mount/reset/state/display/sound/exit; GUI-PARITY.md | **G10** human |
| M11 | tasks/M11_mingw_macos.md   | MinGW + macOS builds via CMake presets; UTF-8 path boundary on Windows | **G11** human per OS |
| M12 | tasks/M12_ci.md            | GitHub Actions 3-OS matrix; ROM-less tests; repo invariant checks | **G12** machine |
| M13 | tasks/M13_retire_legacy.md | Delete retired `sdl/`; keep frozen `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, `hlp/`; docs | **G13** human sign-off |
| M14 | tasks/M14_keyboard_mapping.md | PC-88VA/PC-8801-style SDL2 keyboard mapping; JIS physical, US keytop, and custom presets; Kana/Roman-Kana input; tenkeyless overlay; GUI binding table | **G14 passed** |
| M15 | tasks/M15_support_pc88va_constant_fold.md | Fold the always-enabled `SUPPORT_PC88VA` compile-time flag in the active tree while retaining runtime model selection | **G15 passed** |
| M16 | tasks/M16_sasi_hdd_gui.md | Reactivate SASI in active CMake; expose SASI HDI creation and SASI-1/SASI-2 Open/Remove in the SDL2 ImGui HardDisk menu | **G16 passed** |
| M17 | tasks/M17_opn_backend.md | Keep NP2 OPN/OPNA FM selectable; add BSD-3-Clause ymfm YM2203/YM2608 as the default backend with GUI/config selection | **G17 passed** |
| M18 | tasks/M18_rom_layout.md | Use executable-relative MAME ROM names/checksums, with VA2 `*_va2.rom` names and GUI VA/VA2 selection | **G18 passed** |
| M19 | tasks/M19_portable_runtime.md | Embed frontend assets, consolidate portable state under `vaeg.cfg`, align backup-memory lookup, and model VA OPN/OPNA hardware explicitly | **G19 passed** |
| M20 | tasks/M20_cpu_sgp_speed_pacing.md | Separate V30 and SGP execution capacity from fixed machine/peripheral time; add Configure, No Wait, frame skip, and hold-F11 fast-forward | **G20 passed** |
| M21 | tasks/M21_sdl2_display_effects.md | SDL2-only display effects, resizable common viewport, simplified fullscreen, and embedded historical application icon | **G21 passed** |
| M22 | tasks/M22_disk_image_drop.md | Direct and ZIP/7z/LZH disk-image drag and drop plus FDD-picker archive open, with sorted assignment and bounded safe extraction | **G22 human** |
| M23 | tasks/M23_formatted_fdd_images.md | Create formatted blank FAT12 D88 images as Japanese MS-DOS 2HD (1.232 MB) or 2DD (640 KB), with optional persisted FDD1/FDD2 mounting | **G23 passed** |
| M24 | tasks/M24_host_clipboard_paste.md | Paste host clipboard printable ASCII and line breaks through a paced guest keyboard make/break queue | **G24 passed** |
| M25 | tasks/M25_fdd_raw_images.md | Create formatted FAT12 FDD images as D88 or mtools-compatible IMG raw containers | **G25 passed** |
| M26 | tasks/M26_mouse_input.md | Port original relative mouse capture to SDL2 and expose the VA joystick/mouse controller-port choice | **G26 human** |
| M27 | tasks/M27_frame_display.md | Restore the original measured guest-draw FPS display in the native window title | **G27 passed** |
| M28 | tasks/M28_sound_output_settings.md | Select common output sampling rate and sound buffer plus ymfm FM fidelity from the SDL2 Sound menu | **G28 human** |
| M29 | tasks/M29_va1_tvram_aperture.md | Enforce the VA1 bank-1 64KB TVRAM aperture and restore PC-Engine 1.00 boot compatibility | **G29 focused human passed; VA2 regression corrected in M31** |
| M30 | tasks/M30_va_bms_window.md | Restore the VA `80000H-9FFFFH` BMS window semantics lost in the portable C memory port | **G30 accepted** |
| M31 | tasks/M31_cli_boot_model.md | Select the VA or VA2/VA3 boot model with a session-only command-line override | **G31 passed** |
| M32 | tasks/M32_cli_startup_overrides.md | Add session-only CLI overrides for sound, media, execution, display, and input; remove positional FDD syntax | **G32 passed** |
| M34 | tasks/M34_z80_migration_contract.md | Verify the legacy Z80 contract, select the migration design, and retain revision-1 fixtures | **G34 passed** |
| M35 | tasks/M35_suzukiplan_irq_extension.md | Add the approved interrupt-acknowledge, level-IRQ, and raw-IM0 extension upstream or in a minimal fork | **G35 passed** |
| M36 | tasks/M36_z80_vendor_conformance.md | Vendor the approved Z80 revision and add standalone conformance and ZEX CI | **G36 passed** |
| M37 | tasks/M37_z80_wrapper.md | Add independently authored vaeg interfaces, revision-1 codec, and wrapper unit tests without integration | **G37 passed** |
| M38 | tasks/M38_z80_differential.md | Compare normalized externally observable legacy and replacement traces | **G38 passed** |
| M39 | tasks/M39_z80_integration.md | Integrate an opt-in replacement Z80 path and run private-system regressions | **G39 passed** |
| M40 | tasks/M40_z80_disassembler.md | Replace active legacy disassembly consumers and close the dual-core evidence period | **G40 passed** |
| M41 | tasks/M41_z80_cutover.md | Select the replacement exclusively, delete the seven approved files, and audit releases | **G41 passed** |
| M42 | tasks/M42_upd9002_adr_inventory_harness.md | Record uPD9002 ownership, dispatch/state inventory, behavior-neutral trace/harness infrastructure, and reproducible baselines | **G42 passed** |
| M43 | tasks/M43_upd9002_singlestep_v20_baseline.md | Pin and classify the external V20 corpus and freeze deterministic comparison baselines without changing CPU behavior | **G43 passed** |
| M44 | tasks/M44_upd9002_state_boundary.md | Separate runtime and serialized CPU state while preserving G41 payload compatibility and adding atomic validation | **G44 passed** |
| M45 | tasks/M45_upd9002_native_dispatch_fold.md | Make V30-compatible execution unconditional and remove the per-instruction 286/V30 selector and `i286c_step()` | **G45 passed** |
| M46 | tasks/M46_upd9002_dispatch_normalization.md | Normalize the one-time V30 dispatch constructor, prove post-construction immutability, and remove the obsolete block executors | **G46 passed** |
| M47 | tasks/M47_upd9002_rep0f_correctness.md | Determine correct uPD9002/V52 REP-prefixed 0x0F semantics and protected-state policy from pinned documents, V20 corpus evidence, and a safe PC-88VA probe without changing behavior | **G47 passed** |
| M48 | tasks/M48_upd9002_rep0f_implementation.md | Implement only the exact REP+0F semantic rule, state policy, and baseline transition explicitly approved at G47 | **G48 passed** |
| M49 | tasks/M49_upd9002_isolate_np2_286_protected_mode.md | Inventory the remaining NP2 286 protected-mode dependency closure after the approved correctness transition | **G49 passed** |
| M50 | tasks/M50_remove_np2_286_protected_mode.md | Remove only dependency-closed protected-mode groups explicitly approved at G49 | **G50 passed** |
| M51 | tasks/M51_upd9002_rename.md | Perform pure uPD9002 moves, public API renames, and final repository guards | **G51 passed** |
| M52 | tasks/M52_io_bank_memory.md | Restore portable I/O Bank Memory configuration and correct bank-zero main-RAM pass-through | **G52 passed** |
| M53 | tasks/M53_host_pacing.md | Add configurable non-blocking host pacing that slows guest execution without slowing the UI | **G53 passed** |
| M54 | tasks/M54_hostfat_readonly_prototype.md | Add a session-only read-only HOSTFAT block-device prototype backed by a fixed FAT snapshot | **G54 passed** |
| M55 | tasks/M55_hostfat_integration.md | Add PC-Engine-compatible FAT12-max HOSTFAT geometry, GUI/configuration, save-state identity, refresh policy, and hardened host-path handling | **G55 passed** |
| M56 | tasks/M56_hostfs_readonly_redirector.md | Probe the PC-Engine DOS redirector bridge before a clean-room read-only HOSTFS implementation | **G56 administratively closed at `b72e641733ddea6f0e8faef2507093f7c3aee5a4`: prerequisite absent; no HOSTFS implementation** |
| M57 | tasks/M57_remove_frozen_reference_tier.md | Preserve legal provenance, archive the exact G56 tier, then remove `win9x/`, `i286x/`, `hlp/`, and `cpuxva/memoryva.x86` without changing behavior | **G57 passed at `72322d5c9b8e40e4a988312aebe163a8190e2aa5`** |
| M58 | tasks/M58_upd9002_ssts_ratchet.md | Add immutable hash-level SST epochs, separate architectural/fingerprint profiles, strict classification governance, and lettered-milestone tooling support without changing CPU semantics | **G58 passed at `bc8a55c6da1082b85b794068e0d933e31fe46b13`** |
| M59 | tasks/M59_upd9002_semantics_evidence_pack.md | Produce deterministic expected/actual evidence for the first uPD9002 semantic milestones without changing CPU behavior | **G59 passed at `e7f2325bc81310532091a8ca82914030fdb8b6ba`** |
| M60a | tasks/M60a_upd9002_flags_materialization.md | Correct SST-observed guest-visible FLAGS materialization and load rules without changing interrupt-frame placement or IRET | **G60a passed at `ba2b7d3f5c76646b30d63fd8951f4a1964817b15`** |
| M60b | tasks/M60b_upd9002_rom_authority_epoch.md | Bind monitor-ROM and debugger authority to a content-addressed target-policy epoch and correct only the authorized 6C–6F and exact 0F gap classifications | **G60b passed at `4e5d74d0d9f675df2342353b8bfdbb2e5cded768`** |
| M60c | tasks/M60c_upd9002_fpo2_main_dispatch_audit.md | Audit the main dispatch and FPO2/66/67 target authority without changing instruction semantics | **G60c passed at `e425e55fc17117000ba5178a796de4444d897234`** |
| M60d | tasks/M60d_upd9002_interrupt_frame.md | Resolve only an independently proven synchronous interrupt-frame residual, or close with evidence if none remains | **G60d passed at `8736f8afe6d8eeb58e58c7afdaf5951e2306cb63`** |
| M60e | tasks/M60e_upd9002_iret.md | Correct only evidence-supported IRET semantics after interrupt-frame governance is closed | **G60e passed at `a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b`** |
| M61 | tasks/M61_upd9002_mov_immediate_register.md | Correct the C6/C7 register-form MOV-immediate family while leaving F7 `/2` for residue planning | **G61 passed at `829f314bb0d363ec5b6e9aa738e948b1a3adb365`** |
| M62 | tasks/M62_upd9002_semantics_bundle.md | One-time maintainer-approved bundle: correct AAM, ROR4, ROL4 activation, BCD/ASCII adjust, and shifts through independently reviewable phase commits | **G62 passed at `70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`** |
| M64 | tasks/M64_upd9002_div_idiv.md | Correct DIV/IDIV and requested SST-covered monitor-authorized 0F families; bind BRKEM's accepted zero-case SST status in a separate authority checkpoint | **G64 passed at `9b151923f9468555043152ffe8651c97b9ecac5b`** |
| M65 | tasks/M65_upd9002_residue_replan.md | Serial residue campaign M65j then M65a–M65m; terminal-only formal approval at terminal campaign gate | **G65 passed; terminal campaign approved** |
| M66a | tasks/M66a_upd9002_drop_cpu286_state_compat.md | Remove obsolete CPU286 save-state compatibility as the first internal checkpoint in the combined M66 bundle | **Internal checkpoint only; no independent G66a approval** |
| M66b | tasks/M66b_upd9002_remove_i286_identity.md | Remove the remaining active I286/i286c identity and close the combined M66 bundle | **G66b passed at `97f760e8da573888edf089c2875c623895a3c2c9`** |
| M67 | tasks/M67_upd9002_divergence_consolidation.md | Consolidate approved divergences, hardware questions, and the final target-policy evidence into `tests/ssts/divergence/g67/registry.json` plus generated compatibility views | **G67 passed at `f8f350e1aadec4b6c79c20192d14c50bd39934be`** |
| M68 | tasks/M68_upd9002_segmented_word_mapped_dispatch.md | Restore canonical mapped-memory dispatch ownership for uPD9002 segmented word access while preserving M65e segment wrapping | **G68 passed at `d1e0225c4edb716893fe5579283fbf0915db72b9`** |
| M69 | tasks/M69_upd9002_idp_0142_status_composition.md | Correct only the IDP/TSP `0142H` status-bit Boolean composition, preserving stored flags while adding dynamic VB | **G69 passed at `680308a603b24341c5b9649657f01791b79002f7`** |
| M70 | tasks/M70_upd9002_prefix_string_closure.md | Implement the maintainer-approved `64H`/`65H` REPNC/REPC prefix plus string-instruction closure for the exact 19-group, 5,908-hash population, with negative protection for prefixed `6C`-`6F` | **G70 passed at `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`** |
| M71 | tasks/M71_upd9002_core_dispatch_fold.md | Fold the obsolete standalone uPD9002 dispatch translation unit into `upd9002_core.c` and remove current `v30` dispatch/core naming without changing behavior | **G71 passed at `24950894eca79e308afae8d574d43c8f393bb483`** |
| M72 | tasks/M72_misc_compile_flag_cleanup.md | Audit inactive VAEG-irrelevant code, remove only proven-safe inactive cleanup targets, remove About/More 98x1 UI details, fold always-enabled `VAEG_FIX`, keep required SCSI/HOSTFAT paths, remove legacy HOSTDRV, and audit inactive PC-9821/EPSON/`CPUCORE_IA32`/IDE/PC-9861K/`DISABLE_SOUND`/`VAEG_EXT`/font/embed boundaries without changing active behavior outside the explicitly approved cleanup | **G72 passed at `643d9f7289d817c67f343bf01be368b546bc1438`** |
| M73 | tasks/M73_upd9002_post_m49_performance_regression.md | Isolate and, if evidence permits, correct the runtime performance regression observed between the approved M49 and M50 checkpoints before later guest-behavior and source-tree restructuring work | **G73 human gate passed; M73 closed at `d7f1fd4b642ffa1bf71e855502e00341e9f37152`** |
| M74 | tasks/M74_debug_harness.md | Build a reusable, deterministic, default-off emulator debug harness with bounded captures and private-input isolation, without changing guest-visible behavior | **G74 human gate passed; M74 closed at `3785cc115155c52928817b8c95d38b40268a7bde`** |
| M75 | tasks/M75_scsi_support.md | Clean up, validate, and document active PC-9801-55-compatible VA SCSI support with the driver-installed support disk while preserving SASI and HOSTFAT | **G75 passed at `4ddba36f28dbfbe35a52117964b99b5685fdaa3d`** |
| M76 | tasks/M76_upd9002_upd780_emulation_mode_authority.md | Audit uPD9002 main-CPU uPD780 emulation-mode authority and decide whether a later production implementation is safe without repaired-hardware evidence | **G76 passed at `2ef9716d9628ce8eefdf61a1feedca0be5921077`** |
| M77 | tasks/M77_iova_to_io_rename.md | Move `iova/*` into `io/` with rename-only semantics and no behavior change | **G77 passed at `630e8f27fc4f2d574daf7cdc630836964a4247dc`; merged to `main`** |
| M78 | tasks/M78_iova_to_io_reference_fixups.md | Normalize include paths, CMake source lists, and current documentation after the `iova` to `io` move | **G78 passed at `23e9f4673e2e122835a5ad2fb256e6961f860866`** |
| M79 | tasks/M79_va_io_dispatcher_consolidation.md | Make the VA I/O dispatcher canonical and remove the `iocore` / `iocoreva` split where behavior-neutral | **G79 passed at `1e19c4c539fd99dcc7dcd4a92770a51aef93aad1`** |
| M80 | tasks/M80_98_only_io_cleanup.md | Audit and remove proven 98-only `io/` implementations while retaining C-bus boards and deferring FDD320 until 5-inch 2D evidence is resolved | **G80 passed at `c44569bd8c47c87c19c6e59bfb735ce7431102bd`** |
| M81 | tasks/M81_va_bios_reachability_cleanup.md | Audit VA BIOS reachability and remove only proven 98-only BIOS handlers | **G81 human gate passed; M81 closed at `027cd761df98ce00fa1c24501d6233d7faaa0110`** |
| M82 | tasks/M82_upd780_subsystem_cpu_audit.md | Audit the FDC subsystem uPD780-compatible CPU boundary currently implemented through the suzukiplan-backed wrapper | **G82 human gate passed; M82 closed at `788cd90aa07bf1619c47b2f130a2183d4fd7111c`** |
| M83 | tasks/M83_move_upd780_subsystem_cpu.md | Create `cpu/upd780/` and move the FDC subsystem uPD780-compatible CPU wrapper/backend there | **G83 human gate passed; M83 closed at `d90c8721d6120af9994cedb63685e8a60546513e`** |
| M84 | tasks/M84_cpucva_boundary_cleanup.md | M84a: retire the approved non-VA C-bus sound-board dependency closure (`amd98`, `board26k`, `board86`, `board118`, `pcm86io`, and `cs4231io`); M84b: clean up the remaining `cpucva/` boundary while keeping uPD9002 instruction execution and VA memory ownership separate | **G84 human gate passed; M84 closed at `9aeb6512e59da7e794ffede50b7a184f601d137e`** |
| M85 | tasks/M85_state_save_section_cleanup.md | Audit retired state-save sections, remove only approved obsolete sections, and document compatibility behavior | **G85 human gate passed; M85 closed at `0b6633041e2fb8bae8de7efa1a1768dc6c3e5cba`** |
| M86 | tasks/M86_machine_core_relocation.md | Move active root machine-core sources such as `pccore`, `nevent`, `timing`, `calendar`, `keystat`, `statsave`, `debugsub`, and `clockscale` under `machine/` without behavior change | **G86 human gate passed; M86 closed at `74a5eac8bc0fa145fc0c4bf5ed66e3ff5368c0ae`** |
| M87 | tasks/M87_legacy_tool_rom_regeneration_audit.md | Audit remaining legacy tools, ROM/resource regeneration flows, and `lio/` BIOS/LIO compatibility hooks before the final VA-only source-tree audit | **G87 human gate passed; M87 closed at `d2d1a13167ccd094d0fae180c775ad5e1d7eb78e`; merged to `main` at `f876dbbfe4e69f0a2ad2021b289962d15754812d`** |
| M88 | tasks/M88_final_va_only_source_tree_audit.md | Final VA-only active source-tree audit after performance, BASIC, SCSI, uPD9002 emulation-mode authority, I/O, BIOS, uPD780, `cpucva`, state-save, machine-core relocation, legacy tool cleanup, and `lio/` disposition | **G88 human gate passed; M88 closed at [98d7343](https://github.com/nakatamaho/vaeg/commit/98d7343df9c763354e0775bd04a7b6d8d9c6a291); merged to `main` at [b142bc3](https://github.com/nakatamaho/vaeg/commit/b142bc37c4fe0cc50381727eac5766a5b3843e71)** |
| M89 | tasks/M89_merge_va_source_directories.md | Consolidate the active VA BIOS, VRAM, and CPU adapter/backend directories into `bios/`, `vram/`, and `cpu/` without behavior change | **G89 human gate passed; M89 closed at [665877a](https://github.com/nakatamaho/vaeg/commit/665877ab7e0961907a255796b30e7438115c6e51); Hosted CI [31577266904](https://github.com/nakatamaho/vaeg/actions/runs/31577266904) passed; merged to `main` at [5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97)** |
| M90 | tasks/M90_va_ems_board.md | Enable the retained VA EMS page-frame board with 1MB capacity units, GUI/configuration support, an EMMVA/SQEMM98/RDEMS bootable supplemental-disk workflow, and redistributable HOSTFAT/SQEMM98 release assets | **G90 human gate passed; M90 closed at [f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9); final 10-job Hosted CI [31679653128](https://github.com/nakatamaho/vaeg/actions/runs/31679653128) passed** |
| M91 | tasks/M91_va_single_path.md | Remove retired PC-98 machine-mode routing and use one native V3 VA I/O, memory, CGROM, and initialization path | **G91 human gate passed; M91 closed at [66fbf20](https://github.com/nakatamaho/vaeg/commit/66fbf20f6f5a8fbe107884ca1223acc53d352cbd)** |
| M92 | tasks/M92_clang_format.md | Establish a pinned clang-format 22 policy and mechanically normalize the active first-party C/C++ source set | **G92 human gate passed; M92 closed at [caa1f40](https://github.com/nakatamaho/vaeg/commit/caa1f403cd0c1f6ce7673d6f839de7d3932c5316)** |
| M96 | tasks/M96_va_only_structural_cleanup.md | Audit and simplify the VA-only source tree in staged, evidence-backed cleanup stages while preserving live C-bus and VA hardware boundaries | **G96 human gate passed; M96 closed at `74c859111951f22e86b2bd6453a5f07b040fd6da`** |
| M97 | tasks/M97_sgp_tekumani_commands.md | Complete manual-derived SGP descriptor, LINE, SCAN, ROP, and transparency semantics and add an isolated LINE visual test without timing or real-hardware claims | **G97 human gate passed on 2026-08-24; M97 closed after candidate `7ab4fabfff8ace9f7ae8648cee4004ad6d507b94`** |
| M98 | tasks/M98_zundamon_orbit_master_plan.md | Build an isolated 320x200 billboard-orbit demo around a generic local indexed-image input, a shared 30-scale single-bank BMS atlas, and deterministic 1-16 instance SGP composition | **M98 reassigned to Zundamon orbit; G98a, G98e, G98j, G98k, G98l, G98o, G98p, G98q, G98r, G98s, G98t, G98v, G98w, G98x, G98y, G98z, and G98aa human gates and G98b-G98d, G98f-G98i, and G98u machine gates passed; M98aa closed (IDA 1-64 animation), M98m/M98n remain absorbed reservations** |
| M99999 | tasks/M99999_readme_attribution.md | Record the maintained-fork attribution and original project link in the top-level README | **G99999 documentation check** |

Phase 2 dependencies: M7 → M8 → {M9, M10 parallel} → M11 → M12 → M13.
Post-phase dependency: M13 → M14 → M15 → M16 → M17 → M18 → M19 → M20 → M21 → M22 → M23 → M24 → M25 → M26 → M27 → M28 → M29 → M30 → M31 → M32. The required Z80 migration sequence M34 → M35 → M36 → M37 → M38 → M39 → M40 → M41 is complete. The separately authorized uPD9002 preparation sequence passed G42 through G51. M52–M56 were consumed by unrelated work and retain their historical meanings. The renumbered semantics campaign passed G57 at exactly `72322d5c9b8e40e4a988312aebe163a8190e2aa5`, G58 at exactly `bc8a55c6da1082b85b794068e0d933e31fe46b13`, G59 at exactly `e7f2325bc81310532091a8ca82914030fdb8b6ba`, G60a at exactly `ba2b7d3f5c76646b30d63fd8951f4a1964817b15`, G60b at exactly `4e5d74d0d9f675df2342353b8bfdbb2e5cded768`, G61 at exactly `829f314bb0d363ec5b6e9aa738e948b1a3adb365`, G62 at exactly `70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`, G64 at exactly `9b151923f9468555043152ffe8651c97b9ecac5b`, terminal G65m at exactly `81887aae14f718d7d4d0f2a7bd3fe05d5ea80630`, G66b at exactly `97f760e8da573888edf089c2875c623895a3c2c9`, G67 at exactly `f8f350e1aadec4b6c79c20192d14c50bd39934be`, G68 at exactly `d1e0225c4edb716893fe5579283fbf0915db72b9`, G69 at exactly `680308a603b24341c5b9649657f01791b79002f7`, G70 at exactly `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`, G71 at exactly `24950894eca79e308afae8d574d43c8f393bb483`, and G72 at exactly `643d9f7289d817c67f343bf01be368b546bc1438`. M73 starts from the approved and main-integrated G72 candidate and owns only the post-M49 runtime performance regression. The broader IDP timing and buffer semantics remain deferred. See [`UPD9002_SEMANTICS_MIGRATION.md`](UPD9002_SEMANTICS_MIGRATION.md).
M9 must pass before M11 (all three OSes must ship the VA machine, not
the PC-98 scaffold).

The current approved gate ledger is:

- G73 passed at exactly
  `d7f1fd4b642ffa1bf71e855502e00341e9f37152`. This closes the M73
  post-M49 performance-regression investigation after the diagnostic pending
  check was made inline without changing the fail-closed CPU behavior.
- G74 passed at exactly
  `3785cc115155c52928817b8c95d38b40268a7bde`. This closes the deterministic
  debug-harness milestone.
- G75 passed at exactly
  `4ddba36f28dbfbe35a52117964b99b5685fdaa3d`. This is the M75 integration
  and release-documentation checkpoint for the PC-9801-55-compatible SCSI
  path, including the validated SCSI/SASI/HOSTFAT storage workflows.
- G76 passed at exactly
  `2ef9716d9628ce8eefdf61a1feedca0be5921077`. This is the M76 Stage 1
  uPD70008-compatible Z80 emulation-mode checkpoint, including the CP/MVA
  validation evidence. BRKEM2, full Z80 compatibility, and FDC/SCSI changes
  remain outside that approval.
- G77 passed at exactly
  `630e8f27fc4f2d574daf7cdc630836964a4247dc`. This is the M77 final
  `iova/` to `io/` tree move plus the required build, include, CMake, QA, and
  current-documentation reference updates. The commit is merged to `main`.
- G78 passed at exactly
  `23e9f4673e2e122835a5ad2fb256e6961f860866`. This closes the M78
  `iova/` to `io/` reference, CMake, and current-documentation normalization.
- G79 passed at exactly
  `1e19c4c539fd99dcc7dcd4a92770a51aef93aad1`. This closes the VA I/O
  dispatcher consolidation.
- G80 passed at exactly
  `c44569bd8c47c87c19c6e59bfb735ce7431102bd`. This closes the 98-only I/O
  cleanup while retaining VA-supported C-bus and storage paths.
- G81 passed at exactly
  `027cd761df98ce00fa1c24501d6233d7faaa0110`. This closes the VA BIOS
  reachability cleanup.
- G82 passed at exactly
  `788cd90aa07bf1619c47b2f130a2183d4fd7111c`. This closes the FDC
  subsystem uPD780-compatible CPU boundary audit.
- G83 passed at exactly
  `d90c8721d6120af9994cedb63685e8a60546513e`. This closes the move of the
  FDC subsystem uPD780-compatible CPU under `cpu/upd780/`.
- G84 passed at exactly
  `9aeb6512e59da7e794ffede50b7a184f601d137e`. This closes the approved
  M84a non-VA board retirement and M84b `cpucva/` boundary cleanup.
- G85 passed at exactly
  `0b6633041e2fb8bae8de7efa1a1768dc6c3e5cba`. This closes the state-save
  section cleanup and compatibility audit.
- G86 human gate passed on 2026-08-12 after the clean-checkout V3/demo/OS/
  simple-operation validation. M86 is closed at the implementation merge
  [74a5eac8](https://github.com/nakatamaho/vaeg/commit/74a5eac8bc0fa145fc0c4bf5ed66e3ff5368c0ae);
  rename-only, reference-fixup, and machine-validation details are recorded in
  the M86 report.
- G87 human gate passed on 2026-08-12 for the M87 candidate
  [d2d1a13](https://github.com/nakatamaho/vaeg/commit/d2d1a13167ccd094d0fae180c775ad5e1d7eb78e).
  This closes the legacy-tool and ROM/resource-regeneration audit; the
  deletion, deferred-boundary, and machine-validation details are recorded in
  the M87 report. The resulting M87 implementation and hotfix chain was
  merged to `main` at
  [f876dbb](https://github.com/nakatamaho/vaeg/commit/f876dbbfe4e69f0a2ad2021b289962d15754812d).
- M88 source cleanup is recorded in
  [2fe49c9](https://github.com/nakatamaho/vaeg/commit/2fe49c944797ca8508c3cfc53ed39ffdef5014b0).
  It removes the retired VM/VX, GDC/CRTC, generic non-VA renderer, and
  FM7/X1/X68K font surfaces while retaining VA/VA2 display, MPU98II,
  SASI/SCSI, FDD, HOSTFAT, and the shared CPU-memory compatibility layer.
  The detailed disposition and machine validation are in
  [`m88_final_va_only_source_tree_audit.md`](reports/m88_final_va_only_source_tree_audit.md);
  hosted run [31573711804](https://github.com/nakatamaho/vaeg/actions/runs/31573711804)
  passed all jobs except the Windows MinGW compatibility Configure step;
  G88 human validation passed against candidate `98d7343df9c763354e0775bd04a7b6d8d9c6a291`; M88 was merged to `main` at
  [b142bc3](https://github.com/nakatamaho/vaeg/commit/b142bc37c4fe0cc50381727eac5766a5b3843e71).

-
  M89 source-directory consolidation completed at candidate
  [665877a](https://github.com/nakatamaho/vaeg/commit/665877ab7e0961907a255796b30e7438115c6e51).
  Hosted run [31577266904](https://github.com/nakatamaho/vaeg/actions/runs/31577266904)
  passed all nine jobs against that exact candidate. The maintainer passed
  G89 human validation on 2026-08-12. The approved history was fast-forwarded
  to `main` at [5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97);
  M89 is closed.

- G90 human gate passed on 2026-08-13 against the final M90 history at
  [f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9).
  This closes the VA EMS board, bootable EMMVA/SQEMM98/RDEMS workflow,
  guest-driver distribution, and EMS-enabled development-disk work.

- G91 human gate passed on 2026-08-15 against candidate
  [66fbf20](https://github.com/nakatamaho/vaeg/commit/66fbf20f6f5a8fbe107884ca1223acc53d352cbd).
  This closes the native V3 VA single-path consolidation after the required
  clean-checkout boot, demo, OS, and simple-operation validation.

- G97 human gate passed on 2026-08-24 against the extended M97 candidate
  `7ab4fabfff8ace9f7ae8648cee4004ad6d507b94`. This closes the documented
  SGP descriptor/LINE/SCAN work, the staged visual-test family, and the
  GLASS P4 regression evidence. G97 remains a VAEG visual-regression gate;
  it does not establish real-PC-88VA equivalence or timing behavior.

M73 is closed after the post-M49 performance-regression investigation and
its approved runtime correction. M74 is a separate diagnostic-infrastructure
milestone for a deterministic debug harness and is closed at
`3785cc115155c52928817b8c95d38b40268a7bde`. M75 through M77 remain completed
with their approved gate SHAs above. M78 through M85 are now also completed
with the approved gate SHAs recorded above. M86 is now closed after its
implementation merge and G86 human gate. M87 is now closed after G87 human
validation and its merge to `main`; M88 is closed after G88 human validation
and is merged to `main` at `b142bc37c4fe0cc50381727eac5766a5b3843e71`.
M89 is closed after G89 human validation. The approved source-directory
consolidation history was fast-forwarded to `main` at
[5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97).
M90 is closed after G90 human validation against the final M90 commit
[f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9).
M91 is closed after G91 human validation against the native V3 single-path
candidate [66fbf20](https://github.com/nakatamaho/vaeg/commit/66fbf20f6f5a8fbe107884ca1223acc53d352cbd).

M90 starts from the G89-integrated `main` tree. It restores the retained EMS
page-frame implementation as a VA-configurable expansion board, using the
existing `ExMemory` and save-state identities with disabled or 1-13MB capacity
in 1MB units. It installs the redistributable EMMVA adapter, SQEMM98 manager,
and dependent RDEMS utility in the generated supplemental-disk workflow. It
also builds a hash-pinned HOSTFAT/SQEMM98 guest-driver bundle in CI and includes
the same drivers, licenses, instructions, and checksums in normal artifacts and
tagged release packages. M90 does not change I/O Bank Memory semantics, bundle
commercial software, or track generated media or driver binaries.

M90 implementation is complete at
[498b283](https://github.com/nakatamaho/vaeg/commit/498b283b67bc0e68dcd6f507260e190974c07f9f),
with local validation recorded in
[`m90_va_ems_board.md`](reports/m90_va_ems_board.md) and hosted CI
[31675702844](https://github.com/nakatamaho/vaeg/actions/runs/31675702844)
passing all nine jobs for the preceding implementation candidate. The final
13MB/832-page behavior follow-up hosted run
[31678186077](https://github.com/nakatamaho/vaeg/actions/runs/31678186077)
also passed all nine jobs. The guest-driver distribution candidate then passed
all ten jobs in hosted run
[31679653128](https://github.com/nakatamaho/vaeg/actions/runs/31679653128),
including its dedicated clean-build distribution job. The maintainer passed
G90 on 2026-08-13 against the final M90 history at
[f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9),
and M90 is closed.

M72 closed the inactive compile-flag cleanup while intentionally leaving
`SUPPORT_WAVEREC`, `SUPPORT_OPRECORD`, and FDD320 for later focused audits.
M87-M89 define the final VA-only source-tree consolidation sequence: after
the completed dispatcher, 98-only I/O, BIOS, uPD780, `cpu/`, state-save,
and machine-core work, M87 closed the legacy tool and ROM/resource
regeneration audit while retaining deferred provenance boundaries. M88
closed the final VA-only source-tree audit and M89 consolidates the active
VA BIOS, VRAM, and CPU adapter/backend directories into `bios/`, `vram/`,
and `cpu/`. `cbus/` is not treated as 98-only; VA-supported expansion
boards remain in scope for retention.

M14 is complete. The SDL2 frontend now has a named VA key inventory,
normal guest make/break injection for physical and synthetic input,
JIS physical and US keytop modes, custom scancode-name bindings,
guest-visible Kana lock, Roman-Kana input, and a tenkeyless game overlay.
The implementation and human-gate record are in
`tasks/M14_keyboard_mapping.md`; detailed mapping evidence remains in
`../modernization/keyboard-mapping.md`.

M15 is complete. `SUPPORT_PC88VA` is no longer an active-tree feature
flag or CMake definition. The runtime `pccore.model_va` checks remain
because VA1/VA2 and non-VA guest behavior are runtime state, not build
configuration. The implementation scope, release-integration adjustment,
verification commands, and G15 record are in
`tasks/M15_support_pc88va_constant_fold.md`.

M19 is complete. Active
executables embed the GUI font and startup splash, the portable frontend uses
only `vaeg.cfg`, executable-local configuration and existing backup memory
override user-state copies, and VA sound hardware distinguishes built-in OPN
from OPNA in Sound Board II and VA2/VA3. The implementation record and passed
G19 checklist are in `tasks/M19_portable_runtime.md`.

M20 is complete and G20 passed. V30 instruction execution
and SGP command execution now have independent scaling while the existing
standard-x2 machine/peripheral timeline remains fixed. The SDL2 frontend adds
transactional CPU/SGP configuration, persisted No Wait and frame skip, and a
non-persistent hold-F11 fast-forward shortcut. The clock-domain audit,
automated results, remaining hardware uncertainty, and human checklist are in
`tasks/M20_cpu_sgp_speed_pacing.md`.

The V30/uPD9002 model default remains 7.9872 MHz for VA, VA2, and VA3. SGP
Model default follows the documented model distinction: 3.9936 MHz for VA and
7.9872 MHz for VA2/VA3.

M21 is implemented and G21 passed as an SDL_Renderer-only display milestone.
It adds a shared viewport, resizable windows, immediate
Windowed/current-desktop Exclusive switching, and procedural lightweight
effects without bgfx, custom shaders, MAME renderer code, or new graphics
dependencies. Its packaging follow-up embeds the unchanged historical VAEG
ICO for SDL runtime window icons on all platforms and as a native Windows PE
resource. Borderless and detailed monitor/mode selection are not exposed in
the simplified current GUI. The scope, icon provenance, verification, and G21
checklist are in
`tasks/M21_sdl2_display_effects.md`.

M22 adds SDL2 disk-image drag and drop. Direct images and archive contents are
sorted by basename and assigned to FDD1/FDD2. ZIP, 7z, and LZH extraction uses
bounded LibArchive streaming with traversal and link rejection; MinGW builds
the pinned archive stack statically, as do macOS release builds. Extracted
images are persistent managed user state and are pruned only when neither FDD
references them. The FDD1/FDD2 Open browser also accepts those archive formats
through the same extraction path. The implementation and G22 checklist are in
`tasks/M22_disk_image_drop.md`.

M23 is complete and G23 passed. It adds FDD-menu creation of empty, formatted
FAT12 data disks in D88 containers. It covers the Japanese MS-DOS 1.232 MB
2HD geometry and standard 640 KB 2DD geometry, refuses overwrite, and can
mount the new image through the existing persistent FDD path. The
implementation and G23 checklist are in `tasks/M23_formatted_fdd_images.md`.

M24 is complete and G24 passed. It adds host-to-guest ASCII clipboard paste
through the existing keyboard injection path. The Edit menu and platform
paste shortcut enqueue printable ASCII and Return actions at a conservative
rate. GUI text capture pauses the queue; reset, state load, focus loss, and
shutdown cancel it safely.
Japanese/IME paste and guest-to-host copy remain later work. The implementation
and G24 checklist are in `tasks/M24_host_clipboard_paste.md`.

M25 is complete and G25 passed. It extends M23 creation with a D88/IMG
container choice. IMG is a contiguous FAT12 sector image suitable for mtools
and normal vaeg mounting. Both the 1.232 MB 2HD and 640 KB 2DD raw geometries
are recognized. The implementation and G25
checklist are in `tasks/M25_fdd_raw_images.md`.

M26 is complete and G26 passed. The guest-side generic and PC-88VA mouse
I/O paths remain unchanged; the active SDL2 frontend now supplies relative
motion and active-low buttons through their existing `mousemng_getstat()`
seam. Capture uses SDL relative mode with focus/ImGui safety, original
F12/middle-button controls, and persisted VA joystick/mouse port selection.
The implementation record and G26 checklist are in
`tasks/M26_mouse_input.md`.

M27 is complete and G27 passed. It restores the original Frame Disp semantics
in the SDL2 frontend. It measures the core guest-draw counter over an
approximately two-second window and appends `N.NFPS` to the native window
title. Frame display defaults to enabled when no saved `DspClock` setting
exists, while an explicitly saved off setting is preserved. It does not report
ImGui present rate or change frame skip, VBlank, CPU/SGP speed, or host
pacing. The implementation scope and G27 checklist are in
`tasks/M27_frame_display.md`.

M28 adds common audio-output controls to the Sound menu. Sampling rate and
requested sound-buffer length apply to both NP2 and ymfm through the existing
sound rebuild path. ymfm also exposes Minimum, Medium, and Maximum native OPN
fidelity, with Minimum retained as the compatibility default. The scope,
backend boundary, automated checks, and G28 checklist are in
`tasks/M28_sound_output_settings.md`.

M29 corrects the VA1 system-memory bank-1 aperture. TVRAM remains backed by the
legacy `textmem` object, but VA1 CPU access is limited to the documented 64KB
`A0000H-AFFFFH` range; the unused `B0000H-DFFFFH` range reads as open bus and
ignores writes. This prevents PC-Engine 1.00 from misdetecting banked system
memory as main RAM and placing its VA1 stack where a ROM bank switch hides it.
M31 testing found that applying the same clamp to VA2/VA3 regressed V3 BASIC,
so that model retains its 256KB bank-1 behavior. NEC's VA, VA2, and VA3 product
specifications confirm the model split: 64KB of TVRAM in VA1 and 256KB in
VA2/VA3. The root-cause trace, rejected workarounds, automated boundary tests,
regression record, and human results are in
`tasks/M29_va1_tvram_aperture.md`.

M30 restores the frozen implementation's BMS behavior in the portable VA
memory layer. The `80000H-9FFFFH` aperture now reads as open bus and ignores
writes when BMS is disabled; when enabled it accesses the selected 128KB bank.
The M9 C port had temporarily routed the aperture to ordinary main RAM because
the assembly-only BMS handlers were not callable from `i286c`. That made a
nonexistent 128KB region pass software memory probes. The code evidence,
ROM-less bank tests, and G30 acceptance are in
`tasks/M30_va_bms_window.md`. Human testing showed that this correction does
not resolve the separate VA1 N88 BASIC V3.0 `FILES`/`BEEP` failure; that
investigation is deferred and is not part of M30.

M31 adds `--model va` and `--model va2` to the active SDL2 command line. The
override is applied after loading `vaeg.cfg`, then uses the same canonical
model, ROM-set, and sound-hardware transition policy as the GUI. It is restored
before configuration save, so command-line boot selection does not rewrite
the user's persistent `pc_model` or `SNDboard`. G31 also corrected the M29
VA2/VA3 TVRAM regression while preserving the VA1 PC-Engine 1.00 fix. The
implementation and gate are in `tasks/M31_cli_boot_model.md`.

M32 extends the session-only command line across the existing sound, FDD/SASI,
CPU/SGP pacing, display, controller, and keyboard settings. It validates the
complete effective machine configuration before video, audio, and machine
initialization, including the VA2/VA3 OPNA requirement and actual SASI image
classification. Named
`--fdd1`/`--fdd2` options replace the retired positional FDD syntax. CLI-owned
values are restored before configuration save while GUI changes made during
the run remain persistable. The implementation and G32 checklist are in
`tasks/M32_cli_startup_overrides.md`. G32 passed after maintainer verification
of the deployed MinGW build.

M52 restores the active SDL2 configuration path for the optional PC-88VA I/O
Bank Memory device and corrects the M30 bank-zero interpretation using RDBMS
1.21 source and guest evidence. Port value zero restores ordinary main RAM at
`80000H-9FFFFH`; values 1 through N select the N allocated 128KB banks. Native
PC-88VA port `01D0H` is the default; `00ECH` remains the PC-9801 mode. RDBMS's
compiled `00ECH` default must be overridden with `-P1D0` in PC-88VA mode.
The implementation scope and G52 checklist are in
`tasks/M52_io_bank_memory.md`.

A post-G90 follow-up preserves M52's bank-zero/pass-through policy while
retaining the clean capacity defaults. The current model exposes 640KB of
conventional RAM through `9FFFFH`: selector zero restores ordinary main RAM at
`80000H-9FFFFH`, selectors 1 through N map the N independent 128KB BMS banks,
and invalid nonzero selectors remain open bus. Clean configurations enable
128 banks (16MB) at `01D0H` and 13MB of EMS. Existing persisted values remain
explicit user choices. The correction is
[c52bd8d](https://github.com/nakatamaho/vaeg/commit/c52bd8dbc62dfabc5d7bbbc50b4fbfe7bd6deef4).

M53 adds an optional `PacingMs` host delay in Configure. The implementation
does not alter emulated clock accounting. It schedules guest frames at the
chosen interval while continuing to process and render the host UI, so large
values such as 64ms leave menus and input responsive. The implementation and
G53 checklist are in `tasks/M53_host_pacing.md`.

M54 adds the first read-only HOSTFAT path for PC-Engine. A session-only command
line option converts a deliberately constrained host directory into one fixed
FAT12 snapshot before machine startup. A small clean-room CONFIG.SYS
block driver reads that snapshot through the versioned emulator-private
07EDH/07EFH channel. It is a virtual block disk, not an INT 2FH redirector;
host changes are not visible until a new emulator session, and all guest write
commands return write-protect. The exact prototype boundary and G54 checklist
are in `tasks/M54_hostfat_readonly_prototype.md`. G54 passed for the original
driver at `19626dc`, after which its source provenance was found insufficient
for two-clause BSD redistribution. The independently authored replacement,
factual contract, and attestation require supplemental human revalidation
before M55; no history rewrite is part of that correction. The maintainer
accepted the clean-room replacement and its supplemental gate at
`e0bafbaa3cc0b12f945e18c231c843fc17ff0392`.

M55 adds persistent GUI selection, snapshot refresh and identity policy,
save-state handling, broader deterministic 8.3 mapping, and the final
host-path containment checks. It also expands the fixed image to the practical
PC-Engine CONFIG.SYS-driver FAT12 limit: 1024-byte logical sectors, 16 KiB
clusters, 4084 DOS-visible data clusters, and at most 63.71875 MiB of
allocatable cluster payload. The read-only block-device contract established
by M54 remains unchanged. The G55 run rejected the original 2048-byte/
32 KiB proposal: a 96 KiB file was truncated to 6144 bytes. The corrected
16 KiB geometry copied that file byte-identically and also read a marker
allocated beyond 60 MiB. Historical SASI HDD and SCSI MO support use their
dedicated storage paths and do not prove larger clusters for this independent
CONFIG.SYS driver path.

M56 first tested the prerequisite for an independently authored read-only DOS
network redirector. The non-resident probe reports PC-Engine's DOS interface
as 2.00; `INT 21H/AX=5F02H` and `5F03H` both return `AX=0001, CF=1` without a
single call to the temporary `INT 2FH/AH=11H` hook. The conventional redirector
design therefore cannot expose transparent `DIR`, `TYPE`, or program loading
in the accepted environment. M56 stopped at this fail-closed evidence gate.
HOSTFAT remains unchanged and visible. Any PC-Engine file-service patch or
non-transparent utility protocol requires a separately approved design. See
`tasks/M56_hostfs_readonly_redirector.md`,
`research/m56_pcengine_redirector_probe.md`, and
`reports/m56_hostfs_readonly_redirector.md`. On 2026-07-23 the maintainer
administratively closed G56 at approved SHA
`b72e641733ddea6f0e8faef2507093f7c3aee5a4` because the required DOS
redirector bridge is absent. This closure accepts the prerequisite-stop
disposition only; it is not evidence of a successful HOSTFS implementation.

M57 preserves the former Win9x/i286x tier's legal and lineage evidence, then
removes exactly `win9x/`, `i286x/`, `hlp/`, and
`cpuxva/memoryva.x86` from the current tree. The byte-identical legacy notice
is retained at `LICENSES/legacy-vaeg.txt`, with its relationship to the
archived source recorded in `docs/legal/legacy-source-provenance.md`. The
complete pre-deletion tree remains at annotated tag
`archive/frozen-win9x-i286x-g56`. The maintainer passed G57 at exactly
`72322d5c9b8e40e4a988312aebe163a8190e2aa5`.

M58 adds immutable references to the M43 summaries and failure sidecars,
separate blocking architectural and diagnostic all-FLAGS fingerprint
profiles, a hash-level predecessor ratchet, gap taxonomy, and strict
lettered-milestone parsing. It changes no uPD9002 instruction semantics and
stops at the G58 human gate.

## Gate protocol

Agent side (pasted into PR): CMake build logs, `tools/repo/` check
output, and for M8+ a headless smoke run
(`SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/sdl2/vaeg
--smoke` or documented equivalent). The archived reference tier has no
current-tree build target.

User side (manual, per `gates/GATE_CHECKLIST_PHASE2.md`): clean-checkout
build, V3-mode boot, bundled VA demo, OS boot + simple operations
(DIR, launch a program). G7/G12 are machine gates; G8 is the frontend
gate on the PC-98 scaffold; G9 onward use the full VA checklist.

A gate passes only when the user says so. Pushed tags are immutable.
Tag `portable-pc98` after G8, `portable-va` after G9,
`phase2-complete` after G13. M14-M19 currently have no tag
assignment.

## Resolved decision points

- **memoryva porting strategy (M9).** Faithful transliteration of
  `cpuxva/memoryva.x86` into `cpucva/memoryva.c`.
- **ImGui rendering backend (M10).** ADR-0002 selected
  `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2`.
- **Japanese font for ImGui (M10).** ADR-0003 selected
  `assets/NotoSansJP-Regular.ttf`, now embedded in active executables.
- **Fate of `win9x/` and assembly references (M13).** ADR-0007 keeps
  `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, and `hlp/` frozen as
  references; deletes retired `sdl/` and leftover accessories project
  metadata.
- **Archive and removal of the reference tier (M57).** The exact G56 tree is
  retained at annotated tag `archive/frozen-win9x-i286x-g56`; legal evidence
  is preserved in the current tree before the four frozen paths are removed.
- **SDL2 keyboard policy (M14).** ADR-0008 keeps guest input scancode
  based, distinguishes JIS physical from US keytop behavior, routes all
  synthetic input through normal guest make/break handling, stores custom
  bindings by scancode name, and forbids text or guest-memory injection.
- **PC-88VA active-tree invariant (M15, updated by M72).** The active
  CMake/SDL2 build always includes VA support, so `SUPPORT_PC88VA` is folded
  true. M72 folds the always-enabled `VAEG_FIX` behavior, removes inactive
  PC-9821 and EPSON guarded/model code plus stale 98x1 About/More UI details,
  audits inactive IA32 CPU-core, IDE I/O, PC-9861K expansion serial,
  no-sound, and other VAEG-irrelevant legacy code, keeps required SCSI and
  HOSTFAT paths, removes the separate legacy HOSTDRV path, and treats
  `VAEG_EXT` as obsolete active-tree cleanup rather than a feature to enable
  blindly when audit evidence proves the target is inactive.
- **Selectable OPN/OPNA synthesis (M17).** ADR-0009 keeps NP2 as a
  compatibility option and selects the BSD-3-Clause ymfm YM2203/YM2608
  implementation as the default FM-operator backend. NP2 retains timer/IRQ,
  SSG, ADPCM, rhythm, board-I/O, and mixer ownership in this milestone.
- **Model-specific ROM names (M18).** The active SDL2 frontend reads ROMs
  beside the executable. VA uses unsuffixed names; VA2 and VA3 use MAME's
  `pc88va2` `*_va2.rom` names without fallback to VA files. Executable-relative
  lookup is primary; cwd is a development fallback. Size, CRC32, and SHA-1
  are checked against MAME, including the extra `vasubsys.rom`, with warning-
  only mismatch handling. The frozen reference layout is unchanged.
- **Portable runtime identity and VA sound hardware (M19).** The active
  frontend has one configuration name, `vaeg.cfg`, selected executable-local
  first and user-state second. Existing executable-local `vabkupmem.dat`
  similarly selects portable backup state; otherwise user-state storage is
  used. Frontend assets are embedded, while `SNDboard` independently models
  VA built-in YM2203/OPN or YM2608/OPNA hardware; NP2/ymfm remains a separate
  synthesis-backend choice.

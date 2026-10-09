<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98y private IDA profile integration

Status: **G98y PASS**

This milestone adds a private, local-only IDA asset profile to the accepted
M98x renderer.  It remains a camera-facing billboard orbit; it is not a true
3-D or yaw-rotated model.  The public ZUNDAMON profile remains the
distributable default.

## Authority and commits

- Branch: `topic/m98y-private-ida-integration`
- Starting M98x approval head: `f42d25dc71e40ffc6ff9f963520cfb4b23d11508`
- M98x implementation: `e833b977f671c921d9ad247249d2217c6782cc52`
- M98x report/head resolved from the supplied prefix: `78c166f2c0282d39b03d51104aeac7e38f4de202`
- M98x approval: maintainer stated `Human gate passed`; approval is recorded
  by the starting head and its remote is equal.
- M98y implementation commit: `de0af9d73b429d1087751d9344d87396fe6c57a7`
- M98y report commit: this report commit (the final hash is supplied by the
  Git handoff after commit and push).
- Pushed remote head: this report commit on the M98y topic branch.
- `REAL_HW_PENDING` remains in force.

The supplied private atlas was accepted through the fast path. The current
independent inspector reported `M98H_ATLAS_PASS`; it contains exactly 30
scale IDs (1 through 30), uses one 128 KiB BMS bank, has transparent byte 00h,
and passed descriptor/source/padding/CRC checks. Private provenance, pixels,
palette values, paths, and hashes are intentionally absent from this tracked
report.

## Worktree preservation and changed files

The pre-existing dirty baseline was captured before branch creation and is
unchanged at the end of this milestone:

```text
 M docs/modernization/bug-fixes.md
 M docs/modernization/pc88va-archive-binary-extraction.md
 M tools/pc88va/build-development-disk.sh
 M tools/pc88va/build-softlib-archive-disk.sh
 M tools/pc88va/stage-development-tools.sh
?? .dosbox-colima-bin/
?? docs/98io/
?? docs/agents/reports/m97f_bms_selected_port.md
?? docs/cpmva/
?? docs/disks/
?? docs/neon/
?? docs/roms/
?? docs/tekumani/
?? tools/pc88va/softlib-fdd-manifest.tsv
?? va2bkupmem.dat
?? vabkupmem.dat
```

Only the following tracked files were changed by M98y:

| File | Purpose |
|---|---|
| `demos/zundamon-orbit/256/zundamon_orbit_256.asm` | Select the private filename/radius/HUD profile at assembly time while retaining one renderer and one control path. |
| `demos/zundamon-orbit/256/build.sh` | Validate an explicit local private profile and build the untracked private guest. |
| `demos/zundamon-orbit/build-local-d88.sh` | Package untracked `IDAORB.COM`/`IDAORB.BIN` candidates without changing the public path. |
| `demos/zundamon-orbit/tools/generate_zundamon_orbit_hud.py` | Generate the neutral `IDA CNT:` label variant with the existing fixed-width count tiles. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_hud.py` | Validate both public and IDA subject labels. |
| `demos/zundamon-orbit/tools/validate_zundamon_orbit_depth_table.py` | Validate profile-owned radii while retaining the public defaults. |
| `demos/zundamon-orbit/tools/verify_m98y_private_profile.py` | Independent external-atlas state, transparency, dirty-union, transition, and synthetic oracle. |
| `demos/zundamon-orbit/tools/test_m98y_private_profile.py` | Public/synthetic regression tests for profile selection and oracle wiring. |
| `docs/agents/reports/m98y_private_ida_integration_visual_gate.md` | This neutral milestone report. |

No pre-existing dirty path was staged, reformatted, overwritten, stashed, or
removed. No private atlas, manifest, ROM, image, trace, binary, D88, or
generated private report is tracked.

## Profile and renderer contract

The private build is selected only by the local build environment
`M98Y_PROFILE=private`, `M98Y_PRIVATE_PROFILE_DIR`, and
`M98Y_PRIVATE_ATLAS`; there is no arbitrary runtime file override. The
profile directory supplies generated depth and HUD includes, and the atlas is
validated before assembly. The guest still has one statically bounded 16-entry
state/list capacity, one external atlas load, one BMS bank, transparent
BITBLTs, page-local old-footprint unions, complete-frame publication, `/N1` to
`/N16`, UP/DOWN, `/V1` to `/V8`, pause, and ESC. No IDA-specific drawing fork,
decoder, runtime scaling, bank switch, heap, or duplicated payload was added.

The private descriptor geometry requires the same deterministic 64-phase
M98u path with profile-owned safe orbit radii. The generated profile table is
validated for all phases, scale/depth relations, descriptor anchors,
320x200/HUD bounds, and one-bank source ranges. The draw key remains signed
depth ascending followed by instance ID ascending. Source byte 00h remains
transparent and all near/far/equal-depth semantics are inherited unchanged.

The HUD retains `[4,4,70,20)`, `[34,4,52,12)`, and `[58,12,70,20)` and uses the
private `IDA CNT:` label. Count updates remain publication-synchronous and
the public profile still says `ZUNDAMON`.

## Automated evidence

Public baseline and regression:

- accepted M98x public build reproduced twice: `ZUNDORB.COM`, 36,320 bytes,
  SHA-256 `c8edcca160f6b1a8d96e6d119a54bcaa5af987224a5da252f690bb17d6d47d18`;
- public HUD and atlas validators: PASS, with public bytes unchanged;
- host suite before edits: 225 tests PASS (the accepted M98x suite in this
  checkout); after edits: 228 tests PASS;
- `cmake --build build/macos-macports -j2`: PASS (no work required);
- `build/macos-macports/sdl2/vaeg --selftest`: all tests PASS;
- repository case, encoding, EOL, and `git diff --check`: PASS.

Private fast-path oracle, run only against the maintainer-supplied external
atlas and generated profile includes:

```text
PRIVATE_IDA_ASSET_VALIDATED
PRIVATE_IDA_ONE_BANK_PASS
PRIVATE_IDA_30_SCALE_PASS
private_state_combinations=1024
private_transition_cases=32768
private_first_use_cases=2048
private_synthetic_union_cases=25
private_mismatches=0
```

The oracle independently composes nonzero source bytes, clears each old
physical-page footprint using rounded row unions, and compares the result to
full-clear composition. It does not parse guest command output. It also
rebuilds every private state twice for deterministic composition and checks
first-use pages, both page identities, all counts, all phases, and synthetic
overlap/adjacency/containment topologies.

Private guest build evidence is local-only: two deterministic `IDAORB.COM`
builds matched byte-for-byte, each remained 36,320 bytes, and the local D88
builder installed exactly one private COM and one private atlas. The private
candidate is deliberately outside the repository. A dummy VAEG launch with
the supplied arcade directory reached the emulator startup harness but could
not execute the guest because that directory lacks the complete VA2 platform
ROM set; this is not claimed as private renderer or visual PASS.

A follow-up loader correction (commit `cc324bee`) preserves the 32-bit payload
size while testing its nonzero high word. The earlier private candidate could
therefore reject a valid payload before entering video mode; the corrected
local candidate reaches the normal publication checkpoints in VAEG. The full
private transition/load and maintainer visual gates remain pending.

## Public/private boundary and limitations

The public profile is byte-identical to M98x and does not require private
inputs. The private atlas is external to the COM and is loaded through the
existing bounded BMS loader. Staged/tracked scans contain no private names,
paths, hashes, images, palette words, ROM bytes, D88, traces, or save data.

The maintainer explicitly completed the private visual gate and stated
`G98y passed`. The effect is accepted as a coherent billboard orbit, not true
3-D rotation. No physical timing claim is made; VAEG/VA2 timing is diagnostic
only.

Private extraction from ROM/viewer was not needed because the supplied atlas
passed the fast-path inspector. Generic palette/VA8 conversion and profile
schema code remain public and use only synthetic fixtures in tests. No
multiple IDA poses, gameplay, projectiles, sound, or M98z work was started.

Required neutral tokens:

```text
HOST_PUBLIC_PROFILE_PASS
PRIVATE_IDA_ASSET_VALIDATED
HOST_PRIVATE_IDA_PASS
PRIVATE_IDA_ONE_BANK_PASS
PRIVATE_IDA_30_SCALE_PASS
PRIVATE_IDA_32768_TRANSITIONS_PASS
VAEG_PRIVATE_IDA_MULTI_PASS
G98y passed
REAL_HW_PENDING
```

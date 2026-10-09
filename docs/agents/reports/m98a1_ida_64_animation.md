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

# M98aa IDA 64-instance animation

Status: **G98aa human gate passed on 2026-09-02; M98aa closed**

`REAL_HW_PENDING` remains in force. This milestone is a private local
candidate extension and does not make private pixels, manifests, hashes,
paths, binaries, or disk images distributable.

## Authority and scope

- Starting accepted G98z head: `8d0ce6c5696661e7aba66ca5e36bf75760660d71`.
- Branch: `topic/m98aa-ida-64-animation`.
- Implementation commits: `aa910563b56cadd647c2058758e37a3d1f50cf71`,
  `12365710b5d63dc8e53683828851ce35f130cc71`, and
  `ce99d8243267807d59b3fabefb8d70e9014cab52`, followed by corrective
  launch fix `ee65ac701dec5d9d759b3efed245b76cf4686052`.
- Report commit: this report commit; the exact pushed head is supplied at
  handoff and is remotely equal.
- Predecessor: G98z was explicitly passed by the maintainer and its accepted
  head is the starting point.
- Scope: private IDA profile only; public ZUNDAMON behavior is unchanged.
- The later 128-instance extension is not included.

The unrelated pre-existing worktree state was preserved exactly:

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

## Implementation

The private build selects a capacity of 64 rectangles, draw indices, dirty
interval candidates, and merged intervals. Page-local rectangle offsets and
instance-ID storage use capacity-derived shifts. A four-word (64-bit) seen
mask validates every generated draw permutation (including counts below 64);
the public path retains its original 16-bit code and binary identity. Record
storage remains the accepted 50-byte ABI and uses one shared external atlas.

The private parser accepts exactly `/N1` through `/N64`, rejects malformed and
duplicate tokens, and keeps the existing `/V1` through `/V8` grammar. UP/DOWN
continue from requested state and saturate without wrapping. The private HUD
generator/validator supports 64 fixed two-cell count tiles; the public default
remains 16. Private speed status uses thirteen 0.25X steps from 1.00X through
4.00X, independently of the FPS divisor.

When enabled by the private build, a bounded VBLANK-driven camera demonstrator
steps speed and distance once per nominal 60-edge interval in a deterministic
triangle. Requests still apply only at complete transaction boundaries; no
clear/draw batch is relabelled in flight. A/Z and Q/E remain available for
manual changes. No renderer fork, runtime scaling, extra atlas bank, or
full-page steady-state clear was added.

## Neutral verification

The accepted public guest was rebuilt twice from the unchanged public path:

```text
M98X_GUEST_BUILD_PASS size=52656 runtime_counts=1..16
public guest SHA-256: e0f2111e4da5d0723633f6ac11658fad49f825c2bc6e6346578ac72ff67aa93f
```

The private candidate was built twice from the previously validated local
IDA atlas and 64-count HUD profile. Both builds were 60,848 bytes and
byte-identical. The private output remains untracked. The neutral private
checks passed:

```text
M98H_ATLAS_PASS
M98T_DEPTH_TABLE_VALIDATION_PASS
M98X_HUD_VALIDATION_PASS count_tiles=64
M98AA_PRIVATE_GUEST_BUILD_PASS runtime_counts=1..64
```

The bootable private candidate contains the external atlas and the 60,848-byte
guest; it is available only through the local private build workflow.

## Corrective launch fix

The first 64-instance candidate exited before graphics mode. The demonstrated
root cause was private draw-order validation clearing its four-word seen mask
through the stale BMS `ES` mapping; subsequent phase validation therefore
reported a duplicate even though the generated permutation was valid. The
correction temporarily binds `ES` to the guest data segment for that bounded
clear and restores the prior segment before continuing. The SGP destination
descriptor's fixed four-bit x-word field is also kept independent from the
capacity-derived footprint index, preserving the accepted wire format.

The corrected private guest remains 60,848 bytes and deterministic. VAEG
startup now reaches the complete-frame flip checkpoint for counts 1, 4, 16,
and 64 using the validated private atlas, with the private profile HUD and
rendered instances present. The public guest remains byte-identical to its
accepted M98z identity. The private candidate and its media remain local-only.

Focused host and shell checks passed:

```text
python3 demos/zundamon-orbit/tools/test_m98aa_ida64.py       4 tests OK
python3 demos/zundamon-orbit/tools/test_m98z_orbit_controls.py 15 tests OK
python3 demos/zundamon-orbit/tools/test_m98y_private_profile.py 3 tests OK
sh -n demos/zundamon-orbit/256/build.sh demos/zundamon-orbit/build-local-d88.sh
```

The public binary compares byte-for-byte with the accepted M98z public
identity. The new neutral host test covers all 64 parser values, invalid
forms, 64 unique phase offsets, the 1.00X..4.00X ladder, and source-level
capacity/auto-camera guards. Detailed private framebuffer and timing evidence
stays outside Git; VAEG/VA2 timing is diagnostic only.

## Human gate

The maintainer inspected the local private IDA64 candidate and explicitly
stated `G98aa passed` on 2026-09-02. M98aa is closed. This remains VAEG/VA2
evidence; physical PC-88VA/VA2 evidence remains `REAL_HW_PENDING`.

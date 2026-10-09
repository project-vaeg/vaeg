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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98k guest bring-up result

Accepted candidate: `6ed575dedc5da8827704c33a274ac72e480ce420`

Status: **G98k human/VAEG gate passed on 2026-08-31; M98k closed**

## Result

`M98k RESULT: PASS`

The automated VA2 candidate boots through PC-Engine, enters the established
320x200 G0/G1 8-bpp direct-color mode, completes one bounded SGP submission,
and leaves one exact embedded 16x16 marker on G1 at `(152, 92)`. The independent
indexed-GVRAM oracle passed with no errors. This is an automated VAEG result;
the maintainer subsequently inspected the human-gate result and explicitly
stated that G98k passed on 2026-08-31. No physical-PC-88VA claim is made.

Branch: `topic/m98k-zundamon-guest-bringup`

Starting commit: `8b8c5ceeac5445ba1eb0d3aa804974db09de6809`

Task-definition commit: `58444c4f71109a3b4c8f950f87cf77212b130988`

Implementation ending commit: `6ed575dedc5da8827704c33a274ac72e480ce420`

Push status at report generation: not yet attempted. The final handoff records
the evidence-document commit and pushed remote tip because a commit cannot
contain its own SHA.

## Files changed

The task-definition commit changes only:

- `docs/agents/ROADMAP.md`
- `docs/agents/tasks/M98_zundamon_orbit_master_plan.md`
- `docs/agents/tasks/M98k_zundamon_guest_bringup.md`

The implementation commit changes only:

- `demos/zundamon-orbit/256/build.sh`
- `demos/zundamon-orbit/256/zundamon_orbit_256.asm`
- `demos/zundamon-orbit/README.md`
- `demos/zundamon-orbit/build-local-d88.sh`
- `demos/zundamon-orbit/run-vaeg.sh`
- `demos/zundamon-orbit/tools/build_zundamon_orbit_boot_disk.py`
- `demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py`
- `demos/zundamon-orbit/tools/verify_zundamon_orbit_guest.py`
- `demos/zundamon-orbit/zundamon_orbit_m98k.debug`

This result commit adds this report and advances only the three M98 status
documents named above. It does not include generated media or captures.

## Reference behavior reused

All six required `demos/sgp-pseudo-sprite/256/` references were read in full:

- `build.sh` supplied the script-relative NASM and COM-size convention.
- `build-scroll.sh` confirmed that build variants remain source-defined rather
  than dependent on the caller's current directory.
- `generate_raytrace.py` supplied deterministic generated-data discipline; no
  generated ray-trace content is used by M98k.
- `orb_raytrace8_24.inc` confirmed indexed row/pitch and transparent-zero data
  conventions; none of its bytes is reused.
- `README.md` supplied the documented 320x200 direct-color, G0/G1, FB1, and
  local boot-disk workflow.
- `sgp_sprite_256.asm` supplied the proven BIOS mode sequence, composition
  registers, 320-byte FB1 pitch, CPU-written G0 path, SGP command ports,
  `0105h` transparent BITBLT, and bounded busy/VBLANK waits.

The fixed-segment loader convention was also checked in
`demos/glass-orbit/src/glass_orbit_payload_loader.asm`. PC-Engine may load a
COM at a variable segment, so the M98k COM relocates its small source-built
image to conventional-RAM segment `3000h` before using the fixed debug
checkpoint. This does not add external loading or banked memory.

## Fixed guest contract

| Item | M98k value |
|---|---:|
| BIOS mode | `e00eh` |
| G0/G1 pixel-size word | `0808h` |
| Logical display | 320x200 |
| Pixel format | 8-bpp direct `GGGRRRBB` |
| G0 | Opaque, deterministic nonzero checkerboard |
| G1 | Value `00h` transparent |
| G1 backing | 320x400, 320-byte pitch |
| G1 page A | SGP `0220000h`, DSA `0020000h` |
| Marker | 16x16, stride 16, zero row padding |
| Marker destination | `(152, 92)` on G1 |
| Marker nonzero values | `03h`, `1ch`, `e0h` |
| SGP transfer | One command-list submission, BITBLT `0105h` |
| Idle checkpoint | `3000:0800` |

The SGP list first clears the complete 320x400 G1 backing, then performs the
one marker BITBLT. Page A is published only after the bounded completion wait.
The idle loop waits for VBLANK and never submits another draw.

## Reproduction commands

The live CMake preflight found these build trees; none was assumed:

```text
./build/mingw-cross/CMakeCache.txt
./build/macos-macports/CMakeCache.txt
```

The selected worker was `build/macos-macports/sdl2/vaeg`. The local integration
arguments below remain neutral by policy:

```sh
cmake --build build/macos-macports --target vaeg
build/macos-macports/sdl2/vaeg --selftest

PYTHONPYCACHEPREFIX=/tmp/vaeg-m98k-pyc \
  python3 demos/zundamon-orbit/tools/test_zundamon_orbit_guest.py

NASM=/opt/local/bin/nasm \
  demos/zundamon-orbit/256/build.sh \
  build/generated/zundamon-orbit/m98k/ZUNDORB.COM

demos/zundamon-orbit/build-local-d88.sh \
  <local-bootable-2hd-template> \
  build/generated/zundamon-orbit/m98k/zundamon-orbit-m98k.d88

NASM=/opt/local/bin/nasm VAEG_ZUNDAMON_MODEL=va2 \
  demos/zundamon-orbit/run-vaeg.sh \
  <local-bootable-2hd-template> \
  build/macos-macports/sdl2/vaeg \
  <local-rom-directory> \
  build/generated/zundamon-orbit/m98k-va2-final-2

python3 demos/zundamon-orbit/tools/verify_zundamon_orbit_guest.py \
  build/generated/zundamon-orbit/m98k-va2-final-2

python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

Tool versions were NASM 3.01 and CMake 3.31.12. The D88 builder fixes only the
new root entry's FAT timestamp to `2026-01-01 00:00:00`; two clean builds from
equal inputs compared byte-for-byte.

## Boot and oracle evidence

The debug event stream reached settled checkpoints at completed guest frames
2203 and 2204. Both captures have `CS:IP=3000:0800`, `AX=984bh`,
`BX=0140h`, `CX=00c8h`, `DX=0808h`, and `SI=0101h`. `SI=0101h` is set only
after the sole SGP list returns from its bounded completion wait. No
`frame-limit` event occurred.

The oracle reported:

```text
status=PASS
errors=[]
mode=320x200, 8-bpp, G0+G1
sgp_submissions_completed=1
marker_occurrences=1
g1_nonzero_count=90
g1_nonzero_bbox=(152,92)-(167,107)
marker_transparent_count=166
marker_nonzero_values=03h,1ch,e0h
gvram_stable=true
screen_stable=true
```

Each complete GVRAM image is 262,144 bytes. The oracle compares G0 against an
independently generated nonzero checkerboard and G1 against a complete
128,000-byte expected surface. Therefore transparent marker pixels expose the
known nonzero G0 result, every row and the 16-byte stride are checked exactly,
and any extra marker or nonzero G1 residue fails closed. The two raw GVRAM
images and two rendered BMP images are pairwise identical and nonblack.

Eight focused tests start from a passing synthetic capture and independently
exercise mode-signature, marker-pixel, duplicate-marker, frame-instability,
black-screen, frame-limit, and forbidden-source failures. All eight passed.

## Artifact identities

All files below remain under ignored `build/generated/` storage. The bootable
D88 and captures are validation artifacts and are not tracked.

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `ZUNDORB.COM` | 2,280 | `5db08b8256ccde889fb121f0404303ae8887dafb39cd9e55711df06830519b77` |
| `zundamon-orbit-m98k-pristine.d88` | 1,338,960 | `157cf6f1a8e8774e655c09ff3b0ca81df1c3b37b15d829d018af44ed14ebcd90` |
| `events.tsv` | 209 | `bb3c3d1f83624e18a75b55afd3612aab7b51064ee013a1eea49b6502a0cb8103` |
| `m98k-settled-a.registers.tsv` | 245 | `a54e3d4351c42679d125ab0cef55f2c388e7bc1893a97ce4a5639aa71e03bc35` |
| `m98k-settled-b.registers.tsv` | 245 | `fa66b0454ae4aa43edb2dbbadf70a70b70045d49520870ea65d112bbb3db59ed` |
| `m98k-settled-a.gvram.bin` | 262,144 | `16a368a5bd4638dfcfd7d0ddbe18515a2adf90853302a39c53835c7a9ff695da` |
| `m98k-settled-b.gvram.bin` | 262,144 | `16a368a5bd4638dfcfd7d0ddbe18515a2adf90853302a39c53835c7a9ff695da` |
| `m98k-settled-a.screen.bmp` | 1,080,442 | `b99c3b74eed135dbd27a31a3b7db2ee42044909f3900d4d4f93aead468f13e48` |
| `m98k-settled-b.screen.bmp` | 1,080,442 | `b99c3b74eed135dbd27a31a3b7db2ee42044909f3900d4d4f93aead468f13e48` |
| `m98k-oracle.json` | 2,025 | `8e9f7bedf7a6a78bd76c50dad05b763ba95b58c114c01de31e97a3f2635cf450` |
| macOS VAEG worker | 8,155,976 | `88c15c79a7ad7abde048e5afbd9d543003f39353d024d8446fa0a0be58bffc98` |

## Limitations and preserved state

The automated run used the SDL software renderer with
`SDL_VIDEODRIVER=dummy`. Unlike a PNG-exists smoke check, its PASS is based on
exact indexed GVRAM, completion registers, event chronology, and stable
nonblack composed frames. The maintainer subsequently passed the separate
human/VAEG gate. Physical hardware remains untested and is not implied.

The final worktree retains the exact pre-existing unrelated entries:

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

No M98k file overlaps those paths. No private input name, path, identity, or
payload was tracked. G98k passed and M98k is closed. ROM extraction, external
assets, BMS/EMS/XMS, atlas loading, scaling, animation, multiple instances,
and real artwork remain deferred. M98l remains unassigned and is not started.

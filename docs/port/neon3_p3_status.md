<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# NEON3 P3-B status

P2 was approved as **GO WITH RESTRICTIONS**.  This P3-B increment creates a
buildable geometry/SGP payload without changing the original NEON source.

## Current diagnostic update

The VAEG capture path was corrected before judging the scene.  Earlier debug
scripts waited at a boot-side address also used by the loader, so those black
captures were not evidence about the injected payload.  The payload now has
an optional unique idle halt for deterministic capture.

The minimal SGP path has been observed at that halt with a visible CLS/LINE
rectangle.  A one-frame faithful NEON scene also produced a visible city
wireframe in VAEG; its idle registers were `AX=0001`, `BX=0000`, `CX=0001`,
and `DX=0001`.  This is emulator evidence only, not a real-hardware gate.

The status text path uses the known-good INT 83h/AH=02 attribute convention
(`DX=8000h`), clears all 25 main rows, and composes the text plane afterward.
Startup now stops the soft-key producer with INT 83h/AH=2Fh, `AL=00h` and
hides the complete system-line display with INT 94h/AH=01h, `AL=FFh` before
graphics entry.  TEXT remains enabled; the function-key guide is not left to
the loader/editor environment.

## Implemented

- `demos/neon3/src/neon_counter.asm` includes the original 80286 projection,
  faithful scene geometry, and nine-scene selector by relative include.
- The PC-98 raster entry points are replaced by a shared VA counter/SGP
  backend.  It counts LINE, triangle scanline spans, CLS rows, SET_COLOR
  transitions, and command-list words using the VA SGP word sizes documented
  in the P2 design.
- P3-B emits SET_WORK, SET_COLOR, complete-word CLS, LINE, and END records
  into one bounded list per frame, submits the physical list address through
  the SGP command port, and waits for idle before advancing the timeline.
- The harness iterates the complete `TOTAL_FRAMES` (6144) timeline and then
  wraps back to frame zero for continuous playback.  Per-frame maxima remain
  available in plain words.  Startup clears all TVRAM rows once through the VA
  Text BIOS, hides the cursor blink, and writes the static overlay labels.
  After each completed page is presented, only fixed-width values are updated;
  the scene title is rewritten on scene changes.  The live scene overlay is
  composed as text above G0 (`CX=0031h`), so the title/profile remain visible
  while the SGP scene is moving.
- The payload enters the selected VA G0 mode through INT 8Fh.  Water-raster
  pixel callbacks remain no-ops in this increment, and partial packed-word
  endpoint RMW is intentionally deferred.
- `demos/neon3/build.sh` builds the same source for the 640x200 and 640x400
  profiles.  The profiles change only the physical video-height state; the
  logical scene remains 640x400.

## Verification

Local NASM builds passed for both profiles.  The generated files are local
transient artifacts and are not repository deliverables.  The default build
plays the complete original timeline of 6144 logical frames (indices 0 through
6143) and then wraps to frame zero, continuing until `ESC`.  A stop-at-limit
diagnostic build can still be selected with `NEON_BUILD_LOOP=0`.

```text
profile 200: 52644 bytes
profile 400: 52644 bytes
profile 200 SHA-256: 09d7e8ee7121dfc5533129119bb8734ad4ea0c3711fb63e6ebd66d4f8f776157
profile 400 SHA-256: 3c3144be17b33927a0a5c58b8b9ca6f009919ddb51cc6f3d7451f2f933d0a85d
```

Absolute local build paths used for this run:

```text
/private/tmp/neon3-full-d88.a9rMBx/raw/neon200.bin
/private/tmp/neon3-full-d88.a9rMBx/raw/neon400.bin
```

The loader COM and bootable D88 are local validation artifacts.  Current
one-frame VAEG evidence is:

```text
/private/tmp/neon3-fullscene.OK5sAl/run2/neonpic.png
/private/tmp/neon3-one-status-clear.uhqlD6/run/status.png
```

The full-timeline payloads for the human run are recorded below.  A one-frame
VAEG capture for each profile shows the text overlay and moving SGP geometry
together; the full 6144-frame VAEG run is intentionally not claimed as a
completed automated PASS because the faithful scene is expensive in the debug
build.  `NEON_FRAME_LIMIT` remains a build-time override; 6144 is the
original NEON timeline, not an emulator limit.

```text
200/400 full bootable validation disk:
/private/tmp/neon3-full-d88.a9rMBx/neon3-full-bootable.d88
SHA-256: dffda9cefa1d0544b709337610f3c2ac95969ef339354a9cd4c1b2cb6224394e
```

The full COM payloads on that disk are:

```text
/private/tmp/neon3-full-d88.a9rMBx/payload/root/NEON200.COM
/private/tmp/neon3-full-d88.a9rMBx/payload/root/NEON400.COM
```

The VAEG overlay captures are:

```text
/private/tmp/neon3-overlay-test.RRXsjA/run/overlay.png
/private/tmp/neon3-overlay400-run.CHRUfd/overlay.png
```

The first capture is a 640x200 scene and the second is the 640x400 profile;
both visibly contain the VA text title/profile above the SGP wireframe.
The scripted VAEG return smoke also injected `NEONONE.COM`, sent the mapped
ESC key, and injected `NEONONE.COM` again; its final status capture is:

```text
/private/tmp/neon3-esc-screen2.GQBZld/final.png
```

That is emulator input evidence, not a replacement for the requested
real-hardware ESC/post-return check.

## Restrictions carried forward

- The harness has no DOS `INT 21h` service and does not include the original
  PC-98 GRCG write path.
- The original data include contains dormant legacy audio state needed by the
  unmodified geometry/data include to assemble; no audio routine or OPL/OPNA
  I/O is linked or called by this counter payload.  The production VA audio
  path remains OPNA-only and is a later milestone.
- Complete-word-only endpoint handling is intentionally incomplete for exact
  face coverage.  SGP command-list limits, completed VAEG SGP capture, and
  OPNA timing remain unverified.

## Next gate

The next increment will export list/counter state through a verified
debugger-readable path and add exact partial-word endpoint handling.  Color/
line quality, ESC return on real VA hardware, post-ESC VA1 keyboard behavior,
and a completed long-frame VAEG run remain open.  The full-timeline D88 is
ready for the human gate; this report does not turn emulator evidence into a
real-hardware pass.

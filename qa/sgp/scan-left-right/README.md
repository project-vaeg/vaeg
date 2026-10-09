# SGP SCAN_LEFT / SCAN_RIGHT sanity test

`SCANLR.COM` is a small emulator-side QA payload for the VAEG SGP scan
commands.  It constructs all source boundaries with SGP `LINE` commands; no
`SCAN_LEFT` or `SCAN_RIGHT` command is used to create the image being scanned.
The payload then scans from `x=150` and uses the returned spans with `PATBLT`.

The test uses 640x400 Graphic 0 in 4-bpp mode, initializes SGP `SET WORK`, and
clears the target page before drawing.  It contains three independent regions:

* primary boundaries at x=100 and x=200, y=80..159;
* nearest-boundary boundaries at x=80, 100, 200, and 220, y=170..199;
* adjacent boundaries at x=149 and x=151, y=210..239.

The expected fill bands are y=90..99, 110..119, 130..139, and 175..184,
between x=100 and x=200.  The adjacent test fills only x=150 at y=215..224.
The black gaps deliberately expose an incorrect height, stale descriptor, or
screen-wide PATBLT.

The source is assembled with NASM:

```sh
nasm -f bin -o SCANLR.COM src/scanlr.asm
```

For a local bootable validation disk, install the payload into a private copy
of the PC-Engine template with `tools/pc88va/pcengine_disk.py`.  The bootable
disk is a local test artifact and is not a distributable repository payload.

A headless capture can be reproduced with the repository's Linux debug build:

```sh
printf '@wait 1200\nSCANLR\n@wait 600\n' > /tmp/scanlr-input
VAEG_SGP_SCAN_TRACE=1 VAEG_SCREEN_EXIT_MS=30000 \
  build/linux-debug/sdl2/vaeg --model va --roms docs/roms \
  --fdd1 /absolute/path/to/scanlr.d88 \
  --headless-input-script /tmp/scanlr-input \
  --screen-dump /tmp/scanlr-screen.bmp 2>/tmp/scanlr-vaeg.log
/opt/local/bin/convert /tmp/scanlr-screen.bmp PNG24:/tmp/scanlr.png
```

The command intentionally uses the bootable local image only for the loader;
the payload itself performs direct SGP work after DOS has loaded it.

An optional VAEG trace is enabled only for a test run:

```sh
VAEG_SGP_SCAN_TRACE=1 .../vaeg ...
```

The trace reports each scan start and result.  For the primary region the
documented result is a width of 50 from x=150 to the x=200 boundary; the left
scan also moves the destination start to x=101 and reports width 50.  The
nearest case must return the x=100/x=200 pair rather than x=80/x=220.  The
adjacent case must report width 1.

The host validator is independent of VAEG's SGP implementation:

```sh
python3 tools/validate_scanlr.py /absolute/path/to/scanlr.png
```

It checks the white LINE boundaries, the four separated fill regions, the
black gaps, nearest-boundary geometry, adjacent behavior, and the absence of a
full-width component.  It does not use a non-black-pixel count as the PASS
criterion.

The gated trace can also be checked without changing the emulator result:

```sh
python3 tools/validate_trace.py /absolute/path/to/vaeg.log
```

It expects 100 SCAN starts/results: 80 results with width 50 for the primary
and nearest-boundary rows, and 20 results with width 1 for the adjacent rows.

This is an emulator-side internal-coherence test only.  A PASS does not prove
silicon-level PC-88VA compatibility; real-hardware golden comparison is a
separate later task.

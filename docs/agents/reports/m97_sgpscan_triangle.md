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

# M97 SGP scan triangle diagnosis

This report covers the line-only `SGPSCANB` (Graphics BIOS) and `SGPSCANP`
(direct SGP) tests. `AH=15h` PAINT and SCAN/PATBLT filling remain disabled.
The test image is a local VAEG diagnostic artifact, not a real-hardware
golden result.

## Sources and contract

The `INT 87h` ABI was checked against the PC-88VA Extended Graphics BIOS
description in `docs/tekumani/607ADVG.TXT` (local maintainer reference). The
manual specifies `AH=01h` SETFRAME with `ES:DX`, `AH=02h` VIEW with `ES:BX`,
`AH=10h` CLS with `CX`, and `AH=11h` LINE with a direct seven-word line record
at `ES:BX` and `AL=0` for a PSET line without fill or a tile. The SGP command
implementation and descriptor decoding are in `io/sgp.c` and `io/sgp.h`.

## Root cause: BIOS path

The line records and coordinates were not the first failure. The test checked
`AX` after `AH=01h` SETFRAME and treated any nonzero value as an error. The
manual defines no AX status for SETFRAME; the BIOS leaves the operation value
in AX. That false failure branched to the initialization error path before any
`AH=11h` call, leaving the captured graphics page black. A controlled build
with that check removed reached the three line calls and produced the intended
triangle. The fix removes the undefined-status check, keeps `ES:DX` for
SETFRAME, uses `ES:BX` for each direct line record, and leaves the documented
`AH=11h` record layout unchanged.

The 1-bpp setup uses the documented single-plane framebuffer descriptor
(`A000:0000`, 80 bytes per row, 400 rows) and selects the visible G0 output
after drawing. No emulator graphics behavior was changed.

## Root cause: direct SGP path

The initial direct-SGP program cleared and drew at the GVRAM base but did not
select the matching DSA0 display page. The captured image therefore showed a
different page: a broad gray band and horizontal artifacts rather than the
written geometry. Selecting DSA0 with the word writes required by the VA port
contract fixed the page ownership. A second controlled capture then exposed a
separate 4-bpp color issue: `SET COLOR=000fh` repeats only the low nibble in
the SGP packed pixel word and produced sparse/magenta-looking lines. The line
color is now `ffffh`, and palette entry 15 is initialized to white. The final
direct list begins with SET WORK, clears the selected page, and emits only
SGP LINE commands. The direct test uses the known-working single-plane
4-bpp SGP descriptor and a white-only color value; the resulting visible
geometry is monochrome. The BIOS test uses the separate documented 1-bpp
graphics descriptor. A trial conversion of the direct list to the SGP 1-bpp
descriptor produced no visible pixels in the current VAEG path, so it was not
substituted for the proven 4-bpp direct test.

No special-case triangle path or host framebuffer write was added.

## Diagnostic trace

`io/sgp.c` now emits compile-gated `TRACEOUT` records for the SGP command
address bytes, control and execution-attention writes, and status/control
reads. Each record contains the byte port, value, width, and CPU `CS:IP`.
The normal build leaves `TRACE` undefined, so this does not add runtime output
or alter SGP behavior. The command-decoder trace already records SET WORK,
CLS, SET COLOR, and LINE descriptors in a trace-enabled build.

## Primitive ladder

The two assembly sources accept compile-time selectors so the intermediate
tests remain reproducible without maintaining unrelated copies:

| test | selector | geometry | result |
|---|---|---|---|
| BIOS-B1 | `SGPSCAN_B1` | `(100,100)->(140,100)` | PASS |
| BIOS-B2 | `SGPSCAN_B2` | `(100,100)->(100,140)` | PASS |
| BIOS-B3 | `SGPSCAN_B3` | `(100,100)->(140,140)` | PASS |
| BIOS-B4 | default | triangle outline | PASS |
| SGP-P1 | `SGPSCAN_P1` | four-pixel horizontal segment | PASS |
| SGP-P2 | `SGPSCAN_P2` | `(100,100)->(140,100)` | PASS |
| SGP-P3 | `SGPSCAN_P3` | `(100,100)->(100,140)` | PASS |
| SGP-P4 | `SGPSCAN_P4` | `(100,100)->(140,140)` | PASS |
| SGP-P5 | default | triangle outline | PASS |

The ladder images were opened as a montage. Each contains only its intended
small primitive; none has a full-width line or band. The machine validator
intentionally rejects a ladder segment when asked to validate it as a
triangle.

## Final PNG inspection

The SDL2 screen dump is 640x422; the first 22 rows are the host menu. The
validator crops that strip and analyzes the 640x400 guest viewport. It checks
8-connected geometry, endpoint neighborhoods, sampled pixels along all three
edges, bounding box, and the absence of a full-width component.

| path | image size | viewport bbox | foreground pixels | components | triangle | visual |
|---|---:|---|---:|---:|---|---|
| `qa/artifacts/sgp/bios-triangle.png` | 640x422 | 80,80 - 240,240 | 480 | 1 | YES | PASS |
| `qa/artifacts/sgp/direct-triangle.png` | 640x422 | 80,80 - 240,240 | 480 | 1 | YES | PASS |

Both images were opened and visibly show a closed white triangle with connected
vertices. There is no full-width abnormal line, giant band, or unrelated fill.

Recheck with:

```sh
python3 qa/sgp/validate_triangle.py qa/artifacts/sgp/bios-triangle.png
python3 qa/sgp/validate_triangle.py qa/artifacts/sgp/direct-triangle.png
```

The validator returns `visual_result=FAIL` for a single ladder segment and
`visual_result=PASS` only for the two complete triangle images above.

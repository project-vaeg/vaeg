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
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# SGP triangle plates (16-bit direct color)

`sgptri.asm` builds the DOS 8.3 program `SGPTRI.COM` with NASM:

```sh
NASM=/opt/local/bin/nasm ./build.sh /tmp/SGPTRI.COM
```

The program selects a 320x200 logical G0 display with a 16-bpp direct-color
source surface. Two contiguous 320x200 pages are stored in a 320x400 source
framebuffer. SGP clears the hidden page, emits the grid and scan probes, and
draws the projected triangle outlines. The CPU exchanges DSA0 only after SGP
completion and VBLANK synchronization.

The CPU performs rotation, projection, and outline shade selection. No
polygon fill is active in this version. `SCAN_RIGHT` and `SCAN_LEFT` are
emitted as non-destructive SGP probes before the outline commands. The
Graphics BIOS polygon and Paint services are intentionally not called while
their direct-color behavior is being evaluated.

Up/Down (and `+`/`-`) changes the active plate count from one to four. ESC
restores the previous video state and exits. This is a VAEG visual/command
exercise; real PC-88VA equivalence and timing are not claimed.

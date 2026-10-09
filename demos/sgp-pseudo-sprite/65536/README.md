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

# 65536-color double-buffered pseudo-sprite

`SGP655S.COM` is a compact direct-color pseudo-sprite teaching demo. It uses
two 320x200 Graphic 0 pages backed by a 320x400, 16-bpp source surface.
The pages are exchanged by writing the FB0 display-start registers after SGP
has finished rendering the hidden page.

The frame is rebuilt in place with SGP commands:

1. one linear `CLS` of the hidden 320x200 page;
2. moving 24x24 transparent 16-bpp spheres through `BITBLT`; each sphere
   selects one of 16 HSV hue bitmaps;
3. one word-wise FB0 DSA page exchange at the next synchronization point.

There is no G1 page and no CPU pixel loop. The two-page G0 arrangement keeps
the clear and sprite transfer off the displayed page, eliminating the severe
single-page tearing seen in the earlier version. Each hue bitmap uses
supersampled Phong-style diffuse/specular shading while remaining
monochromatic. SPACE toggles a direct-SGP white square grid whose cells are
16x16 logical pixels. UP/DOWN (or `+`/`-`) changes the active sphere count from
one to 128. If SGP misses a field, presentation can repeat an intact page, but
it does not expose partially drawn rows.

The position and velocity table is deterministic. Its logical y values are
selected from 0 through 199, including the lower third, and each record uses
one of sixteen integer velocity directions. The runtime clamps the 24-pixel
top edge to the safe visible range before emitting each BITBLT.

Build:

```sh
NASM=/opt/local/bin/nasm demos/sgp-pseudo-sprite/65536/build.sh /tmp/SGP655S.COM
```

The program uses the same verified direct-color setup as the 65536-color
wireframe track: `GRMODE=0xb462`, `GRRES=0x1313`, `FBW=640`, `FBL=400`, and
`DSH=200`. SGP command and display-start registers are written as words.

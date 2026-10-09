<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# 88VA Eternal Grafx Rel.20260908

This release records the changes from Rel.20260830 through Rel.20260908.
The main addition is an optional CRT shader presentation path for the
portable SDL2 frontend on Windows, Linux, and macOS.

<div align="center">
<table align="center" width="100%">
<tr>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20260908/vaeg-20260908-111655-0000093767-000.png" alt="CRT effect on"><br><em>CRT effect on</em></td>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20260908/vaeg-20260908-135639-0000032205-000.png" alt="CRT effect off"><br><em>CRT effect off</em></td>
</tr>
<tr>
<td colspan="2" align="center"><img width="50%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20260908/vaeg-20260908-111712-screenshot.png" alt="CRT effect setting"><br><em>CRT effect setting</em></td>
</tr>
</table>
</div>

## CRT shader presentation

- Native CRT presentation is available through the audited librashader C API.
  Standard SDL presentation remains available as the fallback, and an
  unprocessed librashader pass-through mode is available for comparison.
- The CRT path preserves the source pixels with padding before filtering,
  supports screen-size and curvature controls, and keeps scanline and RGB-mask
  processing in the display stage.
- The recommended setup is a 4K display with an integer scale of x3 or
  larger. At lower resolutions or x1/x2 scales, the RGB mask and scanline
  sampling can produce visible moiré; use a larger integer scale or standard
  SDL presentation when that is undesirable.
- Native display screenshots capture the displayed result, including enabled
  display information overlays. Deterministic guest-frame capture remains
  available for QA.
- CRT settings are stored in `vaeg.cfg`. If the host graphics backend or
  filter chain is unavailable, VAEG falls back to SDL without changing guest
  emulation timing.

## Frontend and platform work

- Window scaling now supports x1 through x4 and custom integer sizes, with
  clearer fullscreen and renderer choices.
- Startup splash sizing, menu visibility, native Metal presentation, and
  renderer switching were stabilized across SDL, D3D11, OpenGL, and Metal.
- Static release builds embed the audited CRT preset and shader closure. No
  `librashader.dll`, `librashader.so`, or `librashader.dylib` is required.
  Windows system graphics libraries remain normal OS dependencies.
- The release packages are built for Linux x86_64, Windows x86_64, and macOS
  arm64, with SDL fallback retained for unsupported native filter paths.

## Existing emulator and QA changes retained from Rel.20260830

- The native PC-88VA path, SGP rendering and visual/contract tests, and the
  PC-9801-55-compatible SCSI work remain included.
- Screenshot, media handling, frontend menu, archive-drop, and PC-Engine
  compatibility improvements from Rel.20260830 remain available.
- ROM images, fonts, disk images, and other private integration media are not
  included. Supply the model-specific PC-88VA ROM files separately.

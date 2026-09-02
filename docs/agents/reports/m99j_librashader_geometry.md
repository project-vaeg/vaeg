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
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M99j: preserve geometry across the optional filter path

The filtered path reuses `scrnmng_calculate_viewport()`, so it receives the
same 4:3/aspect, integer-scaling, fit, stretch, menu-inset, fullscreen, and
HiDPI drawable dimensions as the existing SDL path. The librashader output
texture is resized only when the calculated drawable viewport changes, and the
final OpenGL quad maps that texture to the same viewport rectangle.

The source texture remains the fixed 640x400 VAeg canvas, which covers both
640x200 and 640x400 guest content without changing the raw framebuffer. Window
resize events and fullscreen transitions continue to update the SDL drawable
size; the next filtered frame recreates only the output texture at the new
viewport size. No absolute executable path is used for the bundled preset:
the default is the release-relative `assets/shaders/crt/` path, while
`VAEG_CRT_PRESET` is an explicit local override.

Missing or deleted preset files fail chain creation and leave filtering off;
the existing SDL renderer remains available. GPU context-loss recovery is
covered by the later lifecycle milestone rather than being inferred from this
geometry audit.

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
# M99i: present filtered output with librashader

The feature-on SDL path now performs the following presentation sequence on
the same thread:

1. the existing VAeg code composes and uploads the raw guest framebuffer;
2. the persistent librashader OpenGL chain renders the source texture into a
   persistent `GL_RGBA8` output texture; and
3. a small persistent OpenGL 3.3 output program draws that texture to the
   default framebuffer, after which the existing ImGui SDL_Renderer overlay
   and `SDL_RenderPresent` run.

The filter chain and output program are created once, not per frame. A chain
frame failure disables the optional filter and the same `present_begin` call
falls through to the existing SDL texture presentation. `scrnmng_set_librashader_enabled`
supports runtime disable and re-enable while the OpenGL presentation adapter
exists; the environment variable `VAEG_CRT_FILTER=1` selects that adapter at
startup, and `VAEG_CRT_PRESET` selects an explicit local `.slangp` path.

The output quad flips only the final OpenGL texture-coordinate convention so
the top-to-bottom VAeg framebuffer remains top-to-bottom on screen. The raw
guest capture continues to read the shadow framebuffer and is not affected by
the filtered path.

Runtime absence, a non-OpenGL SDL renderer, missing OpenGL entry points, preset
failure, and filter-frame failure all leave the normal renderer active. A
usable physical OpenGL display and the pinned shared runtime were not
available in this environment; visual CRT smoke evidence remains deferred.

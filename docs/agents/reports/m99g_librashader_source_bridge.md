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
# M99g: bridge the VAeg framebuffer to OpenGL

`LibrashaderGlBridge` accepts the existing top-to-bottom RGB565 shadow
framebuffer and uploads it directly to a persistent OpenGL `GL_RGBA8` source
texture using the packed `GL_UNSIGNED_SHORT_5_6_5` upload type. The caller's
pitch is passed through `GL_UNPACK_ROW_LENGTH`; no intermediate full-frame CPU
conversion is made. The bridge reports explicit source and output image
metadata to the librashader C API and keeps one output texture sized to the
drawable viewport.

The bridge refuses a non-OpenGL SDL renderer or a missing current context and
leaves the existing renderer available. It does not create a second window or
read pixels back to the CPU. Texture destruction is safe only while the owning
SDL GL context is current; presentation shutdown must therefore destroy this
object before the SDL renderer/window.

The bridge is feature-gated and is not present in feature-off builds. Visual
verification of 640x200 and 640x400 content is deferred until a usable OpenGL
display is available; the source format, pitch, orientation, and texture
lifetime are deterministic from the current `scrnmng` contract.

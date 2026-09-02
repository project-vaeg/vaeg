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
# librashader input for VAeg

This directory contains the unmodified librashader C API and dynamic-loader
headers from tag `librashader-v0.12.0`, dereferenced at commit
`87e8a97b50516d997defeaa168173dcd185d4022`.

The headers are MIT-licensed under their retained upstream notices. The
implementation is not copied into VAeg and is not statically linked. A
separately distributed librashader runtime must use the MPL-2.0 licensing
option. The alternative GPL implementation is not an accepted VAeg runtime.

The loader searches the platform's normal runtime name (`librashader.dylib`,
`librashader.so`, or `librashader.dll`). The VAeg integration checks the
runtime ABI, API, and required OpenGL symbols before using it. A missing or
incompatible runtime leaves the existing renderer active.

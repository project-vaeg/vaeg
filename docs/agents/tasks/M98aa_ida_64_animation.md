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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M98aa IDA 64-instance animation

Status: **G98aa human gate passed on 2026-09-02; M98aa closed**

M98aa extends the accepted private IDA profile to a bounded runtime count of
1 through 64 using the existing renderer, one 30-scale atlas bank, complete
dirty-union transactions, and publication-safe page state. The public
ZUNDAMON profile remains byte-identical and limited to 1 through 16.

The private candidate defaults to four instances. `/N1` through `/N64` are
accepted before graphics mode, UP/DOWN saturate at 64/1, and the M98u phase
offset and far-to-near ordering rules are unchanged. Its private demonstration
speed is 1.00X through 4.00X in 0.25X steps; a bounded VBLANK-driven triangle
advances speed and distance once per nominal second while A/Z and Q/E remain
available. Private atlas, manifest, binary, D88, and detailed evidence remain
outside Git.

The implementation stops at the M98aa human visual gate. It does not add 128
instances, private poses, gameplay, or a second atlas bank.

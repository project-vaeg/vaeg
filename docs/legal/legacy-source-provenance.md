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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Legacy vaeg source provenance

[`LICENSES/legacy-vaeg.txt`](../../LICENSES/legacy-vaeg.txt) is a
byte-identical preservation of
[`win9x/readme.txt` at the approved G56 commit](https://github.com/nakatamaho/vaeg/blob/b72e641733ddea6f0e8faef2507093f7c3aee5a4/win9x/readme.txt).
Both files have SHA-256
`9c15b317020a58cf58341b00959ffa9e9dd220b0d574aa0c4900c477aeaa6cbc`.
The preserved file is UTF-8 without a byte-order mark, uses LF line endings,
and is retained without transcription or other byte changes.

The document records the original vaeg lineage, its statement that use and
source terms follow Neko Project II, its modified-BSD source-license
statement, and the historical Neko Project II and M88 acknowledgements. It
also includes the Neko Project II v0.80 readme text that accompanied the
original vaeg distribution. Preserving the document records those statements;
it does not reinterpret them, remove existing notices, or make a new
relicensing claim.

M57 removes the frozen Win9x frontend, assembly CPU and VA-memory references,
and HTML Help payload from the current tree. Their exact source remains
available at the immutable annotated
[`archive/frozen-win9x-i286x-g56` tag](https://github.com/nakatamaho/vaeg/tree/archive/frozen-win9x-i286x-g56)
and throughout repository history. The current-tree copy above keeps the
legal, attribution, and lineage evidence directly available after that
removal.

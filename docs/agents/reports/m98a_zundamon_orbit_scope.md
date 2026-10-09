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

# M98a - Zundamon orbit scope report

Evaluated baseline: `cca03134731467a8f7f10769b5430d4ae0878723`

Status: **G98a human gate passed on 2026-08-31**

## Result

The maintainer assigned M98 to the generic Zundamon orbit demonstration. The
new family starts from the evaluated `main` baseline and exposes only a
source-neutral local BMP, palette, and manifest interface.

Public documentation contains no source-specific acquisition, inspection, or
selection workflow. Maintainer inputs and all generated integration media
remain outside Git. The fixed result is a deterministic camera-facing
billboard compositor over BMS and SGP, not genuine multi-angle rotation.

No image processing, guest implementation, emulator change, or runtime claim
belongs to M98a. M98b owns only the deterministic synthetic fixture.

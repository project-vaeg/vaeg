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

# GLASS ORBIT

The authoritative PC-88VA port is the final SGP scene with OPNA/YM2608
audio. Earlier GA, P0, and standalone P4 milestone payloads are intentionally
not kept in this directory.

Build the final verification loader with:

```sh
NASM=nasm demos/glass-orbit/build.sh /absolute/path/GLASS.COM
```

`build-bootable-d88.sh` creates a local bootable validation disk from
a supplied template. The bootable image is a local artifact and must not be
committed. `run-vaeg.sh` is the final VAEG capture/temporal-QA entry
point.

Create the distributable non-bootable data disk with:

```sh
NASM=nasm demos/glass-orbit/build-d88.sh \
    /path/to/2hd-template.d88 /absolute/path/glass-orbit.d88
```

The builder uses the template only for 2HD geometry, creates a blank data
disk, installs `GLASS.COM`, and writes the compressed companion to
`demos/disks/glass-orbit.d88.xz`. The raw D88 is a local artifact; only the
`.d88.xz` companion is committed.

The shared SGP backend and exact 4bpp span/convex-polygon helpers under
`src/` are implementation dependencies of the final scene, not separate demo
variants.
The audio path is OPNA/YM2608 only; OPL is intentionally out of scope.

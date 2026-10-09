<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# Open Watcom build environment

The VAEG guest-driver and SQEMM 8086/V30 builds use the 2026-08-01 Linux x64
distribution of Open Watcom v2 and Open Watcom WASM in TASM-compatible mode.
It does not require the proprietary Borland Turbo Assembler, and the image is
kept generic so other 16-bit guest-driver builds can reuse it.

## Reproducible local image

The image definition is
[`tools/openwatcom/containerfile`](../../tools/openwatcom/containerfile).
Build and export it with:

```sh
CONTAINER_ENGINE=docker \
  tools/openwatcom/export-image.sh
```

The script writes the local-only artifacts below:

```text
docs/openwatcom-image.tar
docs/openwatcom-image.tar.sha256
```

Both files are explicitly ignored by Git. They must not be committed or
uploaded to GitHub. The repository stores only the Containerfile, export
script, pinned download checksum, and this procedure.

Podman can be used without changing the script:

```sh
CONTAINER_ENGINE=podman \
  tools/openwatcom/export-image.sh
```

On macOS, Colima must provide a Linux `linux/amd64` container runtime. On an
Apple-silicon host, use the QEMU/x86_64 Colima profile when native VZ is not
available. The image itself is deliberately `linux/amd64`, matching the
Open Watcom Linux x64 host binary. With MacPorts, the x86_64 Lima guest
agent must also be installed once:

```sh
sudo port install lima +additional_guestagents
```

QEMU must be installed separately (for example, `sudo port install qemu`).

## Using the exported image

Load the exported image into the selected engine. The default tag records the
pinned toolchain date:

```sh
docker load --input docs/openwatcom-image.tar
docker image inspect vaeg/openwatcom:2026-08-01
```

The image only supplies the host toolchain. It does not bundle VAEG ROMs,
disk images, guest drivers, or generated `.SYS` files.

## Building SQEMM98 for PC-88VA

[`tools/openwatcom/build-sqemm98.sh`](../../tools/openwatcom/build-sqemm98.sh)
builds the M90 PC-88VA EMS manager. It fetches and verifies SQEMM 0.8 commit
[`47a03a8903d11e0a748ad702574cb12c730e7966`](https://github.com/sqpat/SQEMM/commit/47a03a8903d11e0a748ad702574cb12c730e7966),
prepares the source out of tree, overlays the PC-88VA hardware backend, and
assembles the MAX driver with Open Watcom WASM:

```sh
tools/openwatcom/build-sqemm98.sh \
  --output /tmp/SQEMM98.SYS \
  --license-output /tmp/SQEMM.LIC
```

The PC-88VA backend maps the four 16KB EMS windows through ports `08E1H`,
`08E3H`, `08E5H`, and `08E7H`, with the 1MB target selected through
`08E9H`. It detects the configured 1 through 13MB capacity and exposes up to
832 logical EMS pages. Initialization and diagnostic text uses the PC-Engine
Text BIOS service `INT 83H/AH=02H`, with `DS:SI` addressing a NUL-terminated
string and `DX=8000H`; it does not use IBM video BIOS `INT 10H` or DOS
`AH=09H` string output.

The build helper validates the DOS character-device name `EMMXXXX0`, the
expected PC-Engine output path, the absence of IBM/DOS string-output calls,
and the VA port-selection code. Generated objects, `SQEMM98.SYS`, and the
combined upstream/port license remain outside Git. A pinned local SQEMM
checkout can be used without downloading another source archive:

```sh
tools/openwatcom/build-sqemm98.sh \
  --source /path/to/SQEMM \
  --output /tmp/SQEMM98.SYS \
  --license-output /tmp/SQEMM.LIC
```

## CI relationship

The same redistributable pair can be generated locally as one distribution
directory when NASM and the configured container engine are available:

```sh
tools/pc88va/build-guest-driver-bundle.sh \
  --output build/pc88va-guest-drivers
```

Canonical CI builds the same pinned image, which downloads the dated Open
Watcom v2 host artifact and verifies its SHA-256. A dedicated Linux job then
builds and structurally validates `SQEMM98.SYS` together with `HOSTFAT.SYS`,
checks their fixed output hashes, and publishes one guest-driver bundle. All
normal Linux, Windows, and macOS build artifacts plus every tagged platform
binary package consume that exact bundle together with its licenses,
installation README, and `SHA256SUMS`. The local image remains the matching
way to reproduce the toolchain under Colima.

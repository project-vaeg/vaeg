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
# ADR-0010: LibArchive for disk-image archive drops

## Status

Accepted for M22.

## Decision

Use LibArchive's streaming reader for ZIP, 7z, and LZH disk-image drops.
Do not invoke `unzip`, `7z`, `lha`, or another shell command. Linux release,
Windows MinGW, and macOS release/CI presets use pinned FetchContent sources
and static linking. Linux and macOS development presets prefer a
CMake-discovered system LibArchive.

Pinned release sources, retrieved 2026-07-13:

| Project | Release URL | SHA-256 | License |
|---|---|---|---|
| LibArchive 3.8.7 | `https://github.com/libarchive/libarchive/releases/download/v3.8.7/libarchive-3.8.7.tar.xz` | `d3a8ba457ae25c27c84fd2830a2efdcc5b1d40bf585d4eb0d35f47e99e5d4774` | BSD 2-Clause |
| zlib 1.3.1 | `https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.xz` | `38ef96b8dfe510d42707d9c781877914792541133e1870841463bfa73f883e32` | zlib License |
| XZ Utils 5.8.3 | `https://github.com/tukaani-project/xz/releases/download/v5.8.3/xz-5.8.3.tar.xz` | `fff1ffcf2b0da84d308a14de513a1aa23d4e9aa3464d17e64b9714bfdd0bbfb6` | 0BSD for liblzma |

zlib supplies deflate decoding for normal ZIP archives. liblzma supplies
LZMA/LZMA2 decoding for normal 7z archives. LibArchive contains its LHA/LZH
decoder. Command-line tools, tests, shared libraries, installation rules, and
unneeded crypto/XML/compression dependencies are disabled in fetched builds.

FetchContent verifies every release archive with the recorded SHA-256. To
update, change one release at a time, verify its official release and license,
replace its URL and hash, then rebuild and manually test all three archive
formats. Fetched upstream sources are never hand-edited.

## Consequences

- Linux release, MinGW, and macOS release configure and first build take
  longer, and their executable is larger.
- The Linux release artifact needs no LibArchive, zlib, or liblzma shared
  library.
- The MinGW artifact needs no LibArchive, zlib, or liblzma DLL.
- The macOS release artifact needs no separately bundled LibArchive, zlib, or
  liblzma dylib. macOS system frameworks remain normal OS dependencies.
- A Linux or macOS development build without system LibArchive still supports
  direct disk drops but reports archive drops as unavailable.
- Extraction remains application-controlled: only supported floppy images
  are written, with traversal/link rejection and explicit size/count limits.
- POSIX archive extraction uses a thread-local UTF-8 `LC_CTYPE` locale while
  LibArchive converts entry names. This supports Unicode ZIP/7z names without
  changing the process-global locale or depending on the startup C locale.
- Extracted disk images are durable managed state under the platform
  `archive-drop/` directory. Mounted paths are persisted in `FDD1FILE` and
  `FDD2FILE`; unreferenced managed image directories are removed after eject
  or replacement, while an image still used by either drive is retained.

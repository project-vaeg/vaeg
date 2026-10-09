#!/bin/sh
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

if [ "$#" -ne 1 ]; then
    printf 'usage: %s OUTPUT.COM\n' "$0" >&2
    exit 2
fi

assembler=${NASM:-nasm}
command -v "$assembler" >/dev/null 2>&1 || {
    printf 'error: NASM is unavailable: %s\n' "$assembler" >&2
    exit 127
}

profile=${NEON4_P5_PROFILE:-}
if [ "$profile" ]; then
    case "$profile" in
        16) bpp=4 ;;       # 640x400, 16-colour packed G0
        256) bpp=8 ;;      # 320x200, packed RGB332
        65536) bpp=16 ;;   # 320x200, direct 16bpp
        *) printf 'error: NEON4_P5_PROFILE must be 16, 256, or 65536\n' >&2; exit 2 ;;
    esac
else
    bpp=${NEON4_P5_BPP:-8}
    case "$bpp" in
        4|8|16) ;;
        *) printf 'error: NEON4_P5_BPP must be 4, 8, or 16\n' >&2; exit 2 ;;
    esac
fi

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir=$(CDPATH= cd -- "$(dirname -- "$1")" && pwd)
output_path=$output_dir/$(basename -- "$1")
raw_path=$output_dir/.$(basename -- "$1").raw

"$assembler" -f bin -O2 \
    -dNEON4_STAGE=8 \
    -dNEON4_P5_BPP="$bpp" \
    -I "$script_dir/src/" \
    "$script_dir/src/neon4.asm" -o "$raw_path"

"$assembler" -f bin -O2 \
    -dNEON_PAYLOAD_FILE="\"$raw_path\"" \
    -I "$script_dir/../neon3/src/" \
    "$script_dir/../neon3/src/neon_payload_loader.asm" -o "$output_path"

rm -f "$raw_path"
printf '%s\n' "$output_path"

#!/usr/bin/env bash
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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
set -euo pipefail
if [[ $# != 1 ]]; then
    echo "Usage: $0 /out-of-tree/output-directory" >&2
    exit 2
fi
source_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
audio=${VOYAGE_AUDIO:-1}
case "$audio" in 0|1) ;; *) echo 'VOYAGE_AUDIO must be 0 or 1' >&2; exit 2 ;; esac
python3 "$source_dir/generate.py" "$1"
output=$(CDPATH='' cd -- "$1" && pwd)
"${NASM:-nasm}" -f bin -O2 -DVOYAGE_AUDIO="$audio" \
    -I "$output/" -I "$source_dir/src/" \
    -l "$output/voyage.lst" "$source_dir/src/voyage.asm" \
    -o "$output/VOYAGE.raw.bin"
# Reuse, rather than copy/change, the established local PC-Engine loader.
"${NASM:-nasm}" -f bin -O2 \
    -DNEON_PAYLOAD_FILE=\""$output/VOYAGE.raw.bin"\" \
    -I "$source_dir/../neon3/src/" \
    "$source_dir/../neon3/src/neon_payload_loader.asm" \
    -o "$output/VOYAGE.COM"
sha256sum "$output/VOYAGE.raw.bin" "$output/VOYAGE.COM"

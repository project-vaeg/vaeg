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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

if [ "$#" -ne 1 ]; then
    printf 'usage: %s OUTPUT.COM\n' "$0" >&2
    exit 2
fi

output=$1
output_dir=$(CDPATH= cd -- "$(dirname -- "$output")" && pwd)
output_name=$(basename -- "$output")
raw_output=$output_dir/GLASS.BIN
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$script_dir/src
assembler=${NASM:-nasm}
python=${PYTHON:-python3}

command -v "$assembler" >/dev/null 2>&1 || {
    printf 'error: NASM is unavailable: %s\n' "$assembler" >&2
    exit 1
}
command -v "$python" >/dev/null 2>&1 || {
    printf 'error: Python is unavailable: %s\n' "$python" >&2
    exit 1
}

"$python" "$script_dir/tools/verify-opna-source.py" "$script_dir"

"$assembler" -f bin -O2 -I "$source_dir/" \
    "$source_dir/glass_orbit.asm" -o "$raw_output"
payload_size=$(wc -c < "$raw_output" | tr -d ' ')
if [ "$payload_size" -gt 57088 ]; then
    printf 'error: GLASS raw payload exceeds fixed stack boundary: %s bytes\n' "$payload_size" >&2
    exit 1
fi

(
    cd "$output_dir"
    "$assembler" -f bin -O2 -I "$source_dir/" \
        -dGLASS_PAYLOAD_FILE=\"$(basename -- "$raw_output")\" \
        "$source_dir/glass_orbit_loader.asm" -o "$output_name"
)
com_size=$(wc -c < "$output" | tr -d ' ')
if [ "$com_size" -gt 65280 ]; then
    printf 'error: GLASS loader exceeds the DOS COM size limit: %s bytes\n' "$com_size" >&2
    exit 1
fi

printf 'Built GLASS ORBIT raw payload: %s\n' "$raw_output"
printf 'Built GLASS ORBIT verification loader: %s\n' "$output"

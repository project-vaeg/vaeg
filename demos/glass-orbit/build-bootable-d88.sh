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

if [ "$#" -ne 2 ]; then
    printf 'usage: %s SOURCE_BOOTABLE_2HD.d88 OUTPUT.d88\n' "$0" >&2
    exit 2
fi

source_image=$1
output_image=$2
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)

[ -f "$source_image" ] || {
    printf 'error: source D88 does not exist: %s\n' "$source_image" >&2
    exit 1
}
[ ! -e "$output_image" ] || {
    printf 'error: refusing to overwrite existing output: %s\n' "$output_image" >&2
    exit 1
}
[ -d "$(dirname -- "$output_image")" ] || {
    printf 'error: output directory does not exist: %s\n' "$(dirname -- "$output_image")" >&2
    exit 1
}

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/glass-orbit.XXXXXX")
cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$work_dir/root"
NASM=${NASM:-nasm} "$script_dir/build.sh" "$work_dir/root/GLASS.COM"
rm "$work_dir/root/GLASS.BIN"

python3 "$repo_root/tools/pc88va/pcengine_disk.py" vanilla \
    --source "$source_image" \
    --output "$output_image"
python3 "$repo_root/tools/pc88va/pcengine_disk.py" install \
    --image "$output_image" \
    --payload "$work_dir"
python3 "$repo_root/tools/pc88va/pcengine_disk.py" list \
    --image "$output_image"

printf 'Created local bootable GLASS ORBIT disk: %s\n' "$output_image"
printf '  Run GLASS to render the complete SGP scene.\n'
printf '  This bootable image is local-only and must not be committed.\n'

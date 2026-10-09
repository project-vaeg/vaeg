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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

if [ "$#" -ne 3 ]; then
    printf 'usage: %s SOURCE_2HD_TEMPLATE.d88 ATLAS.BIN OUTPUT.d88\n' "$0" >&2
    exit 2
fi

source_image=$1
atlas_image=$2
output_image=$3
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
distribution_dir=$repo_root/demos/disks
mkdir -p "$distribution_dir"
output_name=${output_image##*/}
compressed_image=$distribution_dir/${output_name}.xz

[ -f "$source_image" ] || {
    printf 'error: source D88 does not exist: %s\n' "$source_image" >&2
    exit 1
}
[ -f "$atlas_image" ] || {
    printf 'error: atlas does not exist: %s\n' "$atlas_image" >&2
    exit 1
}
[ ! -e "$output_image" ] || {
    printf 'error: refusing to overwrite existing output: %s\n' "$output_image" >&2
    exit 1
}
[ ! -e "$compressed_image" ] || {
    printf 'error: refusing to overwrite existing output: %s\n' "$compressed_image" >&2
    exit 1
}
output_parent=$(dirname -- "$output_image")
[ -d "$output_parent" ] || {
    printf 'error: output directory does not exist: %s\n' "$output_parent" >&2
    exit 1
}
command -v python3 >/dev/null 2>&1 || {
    printf 'error: required host command is missing: python3\n' >&2
    exit 127
}
command -v xz >/dev/null 2>&1 || {
    printf 'error: required host command is missing: xz\n' >&2
    exit 127
}

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/zundamon-orbit-d88.XXXXXX")
compressed_temporary=$(mktemp "${compressed_image}.tmp.XXXXXX")
cleanup() {
    rm -rf "$work_dir"
    rm -f "$compressed_temporary"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$work_dir/payload/root"
M98Y_PROFILE=private \
M98Y_PRIVATE_PROFILE_DIR=${M98Y_PRIVATE_PROFILE_DIR:-} \
M98Y_PRIVATE_ATLAS="$atlas_image" \
M98X_RUNTIME_MODE=1 NASM=${NASM:-nasm} \
    "$script_dir/256/build.sh" "$work_dir/payload/root/ZUNDAORB.COM" \
    "$work_dir/ZUNDAORB.LST"
cp -- "$atlas_image" "$work_dir/payload/root/ZUNDAMON.BIN"

python3 "$repo_root/tools/pc88va/pcengine_disk.py" data \
    --source "$source_image" \
    --output "$work_dir/empty-data.d88"
python3 "$repo_root/tools/pc88va/pcengine_disk.py" install \
    --image "$work_dir/empty-data.d88" \
    --payload "$work_dir/payload"
cp -- "$work_dir/empty-data.d88" "$output_image"
xz -c -9e "$output_image" > "$compressed_temporary"
mv -- "$compressed_temporary" "$compressed_image"
python3 "$repo_root/tools/pc88va/pcengine_disk.py" list \
    --image "$output_image"

printf 'Created non-bootable ZUNDAMON distribution disk: %s\n' "$output_image"
printf 'Created compressed ZUNDAMON distribution disk: %s\n' "$compressed_image"
printf '  ZUNDAORB.COM\n  ZUNDAMON.BIN\n'

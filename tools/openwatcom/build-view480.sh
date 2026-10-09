#!/usr/bin/env bash
#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

set -euo pipefail

program_name=${0##*/}
script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
engine=${CONTAINER_ENGINE:-docker}
image_tag=${VAEG_OPENWATCOM_IMAGE_TAG:-vaeg/openwatcom:2026-08-01}
source_file=
output_file=
output_tmp=
work_dir=

usage() {
	printf '%s\n' \
		"Usage: $program_name --source VIEW480.ASM --output VIEW480.COM" \
		'' \
		'Assemble and link VIEW480 with the pinned Open Watcom image.'
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

cleanup() {
	if [[ -n ${output_tmp} && -f ${output_tmp} ]]; then
		rm -f -- "$output_tmp"
	fi
	if [[ -n ${work_dir} && ${work_dir} == "$repo_root"/build/view480.* && -d ${work_dir} ]]; then
		rm -rf -- "$work_dir"
	fi
}

trap cleanup EXIT HUP INT TERM

while (($#)); do
	case $1 in
	--source)
		(($# >= 2)) || die '--source requires a path'
		source_file=$2
		shift 2
		;;
	--output)
		(($# >= 2)) || die '--output requires a path'
		output_file=$2
		shift 2
		;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		die "unknown argument: $1"
		;;
	esac
done

[[ -n ${source_file} ]] || die '--source is required'
[[ -n ${output_file} ]] || die '--output is required'
[[ -f ${source_file} && -r ${source_file} ]] || die 'source file is not readable'
[[ ! -e ${output_file} ]] || die 'output already exists; refusing to overwrite it'
[[ -d ${output_file%/*} || ${output_file} != */* ]] || die 'output directory does not exist'
command -v "$engine" >/dev/null 2>&1 || die "required command is missing: $engine"

source_name=${source_file##*/}
build_name=${source_name%.*}
mkdir -p -- "$repo_root/build"
work_dir=$(mktemp -d "$repo_root/build/view480.XXXXXX")
cp -- "$source_file" "$work_dir/$source_name"

DOCKER_BUILDKIT=${DOCKER_BUILDKIT:-0} "$engine" build --platform linux/amd64 \
	--file "$script_dir/containerfile" \
	--tag "$image_tag" "$script_dir"

"$engine" run --rm --platform linux/amd64 -v "$work_dir:/src" -w /src \
	"$image_tag" sh -c \
	"wasm -zcm=tasm -fo=${build_name}.obj ${source_name} && wlink format dos com name ${build_name}.com file ${build_name}.obj"

compiled_file=$work_dir/${build_name}.com
[[ -s ${compiled_file} ]] || die 'Open Watcom did not create a COM executable'
output_tmp=$(mktemp "$output_file.tmp.XXXXXX")
cp -- "$compiled_file" "$output_tmp"
chmod 0644 "$output_tmp"
mv -- "$output_tmp" "$output_file"
output_tmp=
printf 'Created VIEW480 COM: %s\n' "$output_file"

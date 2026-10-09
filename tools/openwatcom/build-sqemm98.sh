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
cache_dir=${HOME}/.cache/vaeg/auto-generated-pc88va-utility-media
output=
license_output=
source_dir=
work_dir=
output_tmp=
license_tmp=
download_tmp=

sqemm_commit=47a03a8903d11e0a748ad702574cb12c730e7966
sqemm_archive=SQEMM-$sqemm_commit.tar.gz
sqemm_sha256=a566ed2016a8d35a86653612d46e9343e0bfd546d063621a869e0b7c261279de
sqemm_url=https://codeload.github.com/sqpat/SQEMM/tar.gz/$sqemm_commit

usage() {
	printf '%s\n' \
		"Usage: $program_name --output SQEMM98.SYS [options]" \
		'' \
		'Options:' \
		'  --license-output FILE  Write the upstream and PC-88VA port licenses.' \
		'  --source DIR           Build the pinned commit from a local Git checkout.' \
		'  --cache DIR            Select the download cache.' \
		'' \
		'Build the PC-88VA SQEMM98 MAX driver with the pinned Open Watcom image.'
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

cleanup() {
	if [[ -n ${output_tmp} && -f ${output_tmp} ]]; then
		rm -f -- "$output_tmp"
	fi
	if [[ -n ${license_tmp} && -f ${license_tmp} ]]; then
		rm -f -- "$license_tmp"
	fi
	if [[ -n ${download_tmp} && -f ${download_tmp} ]]; then
		rm -f -- "$download_tmp"
	fi
	if [[ -n ${work_dir} && ${work_dir} == "$repo_root"/build/sqemm98.* && -d ${work_dir} ]]; then
		rm -rf -- "$work_dir"
	fi
}

trap cleanup EXIT HUP INT TERM

while (($#)); do
	case $1 in
	--output)
		(($# >= 2)) || die '--output requires a path'
		output=$2
		shift 2
		;;
	--license-output)
		(($# >= 2)) || die '--license-output requires a path'
		license_output=$2
		shift 2
		;;
	--source)
		(($# >= 2)) || die '--source requires a path'
		source_dir=$2
		shift 2
		;;
	--cache)
		(($# >= 2)) || die '--cache requires a path'
		cache_dir=$2
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

[[ -n ${output} ]] || die '--output is required'
[[ ! -e ${output} ]] || die 'output already exists; refusing to overwrite it'
if [[ -n ${license_output} ]]; then
	[[ ! -e ${license_output} ]] || die 'license output already exists; refusing to overwrite it'
fi
for required_command in "$engine" python3 sha256sum tar; do
	command -v "$required_command" >/dev/null 2>&1 || die "required command is missing: $required_command"
done

mkdir -p -- "$repo_root/build" "$cache_dir"
work_dir=$(mktemp -d "$repo_root/build/sqemm98.XXXXXX")
source_root=$work_dir/source

if [[ -n ${source_dir} ]]; then
	command -v git >/dev/null 2>&1 || die 'git is required with --source'
	[[ -d ${source_dir} ]] || die 'SQEMM source directory does not exist'
	actual_commit=$(git -C "$source_dir" rev-parse HEAD) || die 'could not read SQEMM source commit'
	[[ ${actual_commit} == "${sqemm_commit}" ]] || die "SQEMM source must be commit $sqemm_commit"
	mkdir -p -- "$source_root"
	git -C "$source_dir" archive "$sqemm_commit" | tar -x -C "$source_root"
else
	command -v curl >/dev/null 2>&1 || die 'curl is required without --source'
	archive_path=$cache_dir/$sqemm_archive
	if [[ ! -f ${archive_path} ]]; then
		download_tmp=$(mktemp "$cache_dir/$sqemm_archive.part.XXXXXX")
		curl --fail --location --silent --show-error --retry 3 --connect-timeout 20 \
			--output "$download_tmp" "$sqemm_url"
		actual_sha=$(sha256sum -- "$download_tmp")
		actual_sha=${actual_sha%% *}
		[[ ${actual_sha} == "${sqemm_sha256}" ]] || die 'downloaded SQEMM archive has the wrong SHA-256'
		mv -- "$download_tmp" "$archive_path"
		download_tmp=
	fi
	actual_sha=$(sha256sum -- "$archive_path")
	actual_sha=${actual_sha%% *}
	[[ ${actual_sha} == "${sqemm_sha256}" ]] || die 'cached SQEMM archive has the wrong SHA-256'
	tar -xzf "$archive_path" -C "$work_dir"
	mv -- "$work_dir/SQEMM-$sqemm_commit" "$source_root"
fi

python3 "$script_dir/prepare-sqemm98.py" "$source_root"

"$engine" build \
	--platform linux/amd64 \
	--file "$script_dir/containerfile" \
	--tag "$image_tag" \
	"$script_dir"
"$engine" run --rm --platform linux/amd64 \
	-v "$source_root:/src" -w /src "$image_tag" \
	sh -c 'wasm -zcm=tasm -fo=sqemm98.obj sqemm.asm && wlink format dos com name sqemm98.sys file sqemm98.obj'

python3 "$script_dir/check-sqemm98.py" "$source_root/sqemm98.sys"
output_tmp=$(mktemp "$output.tmp.XXXXXX")
cp -- "$source_root/sqemm98.sys" "$output_tmp"
chmod 0644 "$output_tmp"
mv -- "$output_tmp" "$output"
output_tmp=

if [[ -n ${license_output} ]]; then
	license_tmp=$(mktemp "$license_output.tmp.XXXXXX")
	{
		printf '%s\n' 'SQEMM upstream license:' ''
		cat "$source_root/LICENSE"
		printf '%s\n' '' 'PC-88VA port license:' '' \
			'Copyright (c) 2026 Nakata Maho' \
			'' \
			'Redistribution and use in source and binary forms, with or without' \
			'modification, are permitted provided that the following conditions' \
			'are met:' \
			'1. Redistributions of source code must retain the above copyright' \
			'   notice, this list of conditions and the following disclaimer.' \
			'2. Redistributions in binary form must reproduce the above copyright' \
			'   notice, this list of conditions and the following disclaimer in the' \
			'   documentation and/or other materials provided with the distribution.' \
			'' \
			'THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR' \
			'IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED' \
			'WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE' \
			'DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,' \
			'INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES' \
			'(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR' \
			'SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)' \
			'HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,' \
			'STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING' \
			'IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE' \
			'POSSIBILITY OF SUCH DAMAGE.'
	} > "$license_tmp"
	chmod 0644 "$license_tmp"
	mv -- "$license_tmp" "$license_output"
	license_tmp=
fi

output_sha=$(sha256sum -- "$output")
printf 'Created SQEMM98 driver: %s\n' "$output"
printf 'SHA-256: %s\n' "${output_sha%% *}"

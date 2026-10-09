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
output=
cache_dir=
hostfat_driver=
sqemm_driver=
sqemm_license=
work_dir=

hostfat_sha256=393226edcde6b0cc8648ce9f8b380804c44e2bec7c3d762cb60f0bc211b1767e
sqemm_sha256=eb3d443d7c12b6eb204e03a7ebac4b68a69d4690d0899ca93a37ad7a546d4930

usage() {
	printf '%s\n' \
		"Usage: $program_name --output DIR [options]" \
		'' \
		'Options:' \
		'  --cache DIR             Select the SQEMM source-download cache.' \
		'  --hostfat FILE          Use and validate an existing HOSTFAT.SYS.' \
		'  --sqemm-driver FILE     Use and validate an existing SQEMM98.SYS.' \
		'  --sqemm-license FILE    License paired with --sqemm-driver.' \
		'' \
		'Build the redistributable PC-88VA guest-driver bundle for vaeg.'
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

cleanup() {
	case ${work_dir} in
	*/.vaeg-guest-drivers.*)
		rm -rf -- "$work_dir"
		;;
	esac
}

trap cleanup EXIT HUP INT TERM

while (($#)); do
	case $1 in
	--output)
		(($# >= 2)) || die '--output requires a path'
		output=$2
		shift 2
		;;
	--cache)
		(($# >= 2)) || die '--cache requires a path'
		cache_dir=$2
		shift 2
		;;
	--hostfat)
		(($# >= 2)) || die '--hostfat requires a path'
		hostfat_driver=$2
		shift 2
		;;
	--sqemm-driver)
		(($# >= 2)) || die '--sqemm-driver requires a path'
		sqemm_driver=$2
		shift 2
		;;
	--sqemm-license)
		(($# >= 2)) || die '--sqemm-license requires a path'
		sqemm_license=$2
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
if [[ -n ${sqemm_driver} || -n ${sqemm_license} ]]; then
	[[ -n ${sqemm_driver} && -n ${sqemm_license} ]] ||
		die '--sqemm-driver and --sqemm-license must be used together'
fi
for required_command in python3 sha256sum; do
	command -v "$required_command" >/dev/null 2>&1 ||
		die "required command is missing: $required_command"
done

output_parent=${output%/*}
if [[ ${output_parent} == "${output}" ]]; then
	output_parent=.
fi
mkdir -p -- "$output_parent"
work_dir=$(mktemp -d "$output_parent/.vaeg-guest-drivers.XXXXXX")
stage_dir=$work_dir/bundle
mkdir -p -- "$stage_dir/licenses"

if [[ -n ${hostfat_driver} ]]; then
	[[ -f ${hostfat_driver} ]] || die 'HOSTFAT input does not exist'
	cp -- "$hostfat_driver" "$stage_dir/HOSTFAT.SYS"
else
	command -v nasm >/dev/null 2>&1 || die 'nasm is required to build HOSTFAT.SYS'
	nasm -f bin -o "$stage_dir/HOSTFAT.SYS" \
		"$script_dir/hostfat/hostfat.asm"
fi
python3 "$script_dir/hostfat/check_driver.py" \
	--input "$stage_dir/HOSTFAT.SYS"

if [[ -n ${sqemm_driver} ]]; then
	[[ -f ${sqemm_driver} ]] || die 'SQEMM98 input does not exist'
	[[ -f ${sqemm_license} ]] || die 'SQEMM98 license input does not exist'
	cp -- "$sqemm_driver" "$stage_dir/SQEMM98.SYS"
	cp -- "$sqemm_license" "$stage_dir/licenses/SQEMM98.txt"
else
	sqemm_args=(
		--output "$stage_dir/SQEMM98.SYS"
		--license-output "$stage_dir/licenses/SQEMM98.txt"
	)
	if [[ -n ${cache_dir} ]]; then
		sqemm_args+=(--cache "$cache_dir")
	fi
	"$repo_root/tools/openwatcom/build-sqemm98.sh" "${sqemm_args[@]}"
fi
python3 "$repo_root/tools/openwatcom/check-sqemm98.py" \
	"$stage_dir/SQEMM98.SYS"

cp -- "$script_dir/hostfat/license.txt" "$stage_dir/licenses/HOSTFAT.txt"
cp -- "$repo_root/dist/pc88va-guest-drivers.txt" \
	"$stage_dir/README-PC88VA-drivers.txt"

actual_hostfat=$(sha256sum -- "$stage_dir/HOSTFAT.SYS")
actual_hostfat=${actual_hostfat%% *}
[[ ${actual_hostfat} == "${hostfat_sha256}" ]] ||
	die "HOSTFAT.SYS SHA-256 changed: $actual_hostfat"
actual_sqemm=$(sha256sum -- "$stage_dir/SQEMM98.SYS")
actual_sqemm=${actual_sqemm%% *}
[[ ${actual_sqemm} == "${sqemm_sha256}" ]] ||
	die "SQEMM98.SYS SHA-256 changed: $actual_sqemm"

(
	cd "$stage_dir"
	sha256sum \
		HOSTFAT.SYS \
		SQEMM98.SYS \
		README-PC88VA-drivers.txt \
		licenses/HOSTFAT.txt \
		licenses/SQEMM98.txt > SHA256SUMS
	sha256sum --check SHA256SUMS
)
mv -- "$stage_dir" "$output"
printf 'Created PC-88VA guest-driver bundle: %s\n' "$output"

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
source_d88=
output_d88=
scsi_id=0
cache_dir=${HOME}/.cache/vaeg/auto-generated-pc88va-utility-media
work_dir=
output_tmp=
download_tmp=

usage() {
	printf '%s\n' \
		"Usage: $program_name --source SOURCE.d88 --output OUTPUT.d88" \
		'       [--scsi-id 0..7]' \
		'' \
		'Create a bootable PC-Engine 1.1 SCSI support disk containing' \
		'PCPLUS, SCHD, VBUFF, SCFORM, and their original documentation.' \
		'The SCSI ID defaults to 0. Source and output D88 images are never' \
		'added to the repository.'
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

cleanup() {
	if [[ -n ${download_tmp} && -f ${download_tmp} ]]; then
		rm -f -- "$download_tmp"
	fi
	if [[ -n ${output_tmp} && -f ${output_tmp} ]]; then
		rm -f -- "$output_tmp"
	fi
	if [[ -n ${work_dir} && ${work_dir} == "${TMPDIR:-/tmp}"/vaeg-pc88va-scsi.* && -d ${work_dir} ]]; then
		rm -rf -- "$work_dir"
	fi
}

trap cleanup EXIT HUP INT TERM

while (($#)); do
	case $1 in
	--source)
		(($# >= 2)) || die '--source requires a path'
		source_d88=$2
		shift 2
		;;
	--output)
		(($# >= 2)) || die '--output requires a path'
		output_d88=$2
		shift 2
		;;
	--scsi-id)
		(($# >= 2)) || die '--scsi-id requires a number'
		scsi_id=$2
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

[[ -n ${source_d88} ]] || die '--source is required'
[[ -n ${output_d88} ]] || die '--output is required'
[[ ${scsi_id} == [0-7] ]] || die '--scsi-id must be one digit from 0 through 7'
[[ -f ${source_d88} && -r ${source_d88} ]] || die 'source D88 is not a readable file'
[[ ! -e ${output_d88} ]] || die 'output already exists; refusing to overwrite it'
[[ -d ${output_d88%/*} || ${output_d88} != */* ]] || die 'output directory does not exist'

for required_command in curl dosbox lha python3 sha256sum; do
	command -v "$required_command" >/dev/null 2>&1 ||
		die "required host command is missing: $required_command"
done

mkdir -p -- "$cache_dir"

verify_sha256() {
	local path=$1
	local expected=$2
	local actual

	actual=$(sha256sum -- "$path")
	actual=${actual%% *}
	[[ ${actual} == "${expected}" ]]
}

move_with_retry() {
	local source=$1
	local destination=$2
	local attempt

	for attempt in 1 2 3; do
		if mv -- "$source" "$destination"; then
			return 0
		fi
		sleep 1
	done
	return 1
}

fetch_package() {
	local name=$1
	local expected=$2
	local url=$3
	local destination=$cache_dir/$name

	if [[ -f ${destination} ]]; then
		verify_sha256 "$destination" "$expected" ||
			die "cached package has the wrong SHA-256: $name"
		printf 'Using cached %s\n' "$name"
		return
	fi

	download_tmp=$(mktemp "$cache_dir/$name.part.XXXXXX")
	printf 'Fetching %s\n' "$name"
	if ! curl --fail --location --silent --show-error --retry 3 --connect-timeout 20 \
		--output "$download_tmp" "$url"; then
		die "download failed: $name"
	fi
	verify_sha256 "$download_tmp" "$expected" ||
		die "downloaded package has the wrong SHA-256: $name"
	move_with_retry "$download_tmp" "$destination" ||
		die "could not finalize cached package after three attempts: $name"
	download_tmp=
}

fetch_softlib_package() {
	local name=$1
	local expected=$2
	local group=$3
	local remote_name=$4
	local url="http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=$group&fname=$remote_name"

	fetch_package "$name" "$expected" "$url"
}

fetch_softlib_package pcp108.lzh \
	4561df318cdfb08bdf8276741058ca2b0d4a4eeaf084a8c5b587f520f6c9e3f0 \
	378 PCP108.LZH
fetch_softlib_package pcp108p.lzh \
	25f1d9432247c88667b880f4153966725a757b4dcd1a062c88497d32b0c8eef7 \
	451 PCP108P.LZH
fetch_softlib_package bdiff128.lzh \
	0ba491ee4829a6f292cfbcad25371a98c2161c1a92d028b0d2fd5dd9d9011153 \
	328 BDIFF128.LZH
fetch_softlib_package schd155t.lzh \
	87aebcf7c9bc9c6170a40d0e6ddcce5afdcbb1fa55f1fdeeec815458f7ef065f \
	448 SCHD155T.LZH
fetch_softlib_package vbuff102.lzh \
	c51d2f9bd04efeda77760a2c8e476777c07edb8775a7120793dd98bc0a8ff01f \
	452 VBUFF102.LZH
fetch_package scf124.lzh \
	a62183d66da90546d19d81f8adad32a2df2485d619badcaf2c167668b7603aad \
	'http://www.pc88.gr.jp/forum/download.php?id=15'

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-pc88va-scsi.XXXXXX")

extract_archive() {
	local archive=$1
	local destination=$2

	mkdir -p -- "$destination"
	lha xfw="$destination" "$archive" >/dev/null
}

extract_archive "$cache_dir/pcp108.lzh" "$work_dir/pcplus"
extract_archive "$cache_dir/pcp108p.lzh" "$work_dir/pcplus-patch"
extract_archive "$cache_dir/bdiff128.lzh" "$work_dir/bdiff"
extract_archive "$cache_dir/schd155t.lzh" "$work_dir/schd"
extract_archive "$cache_dir/vbuff102.lzh" "$work_dir/vbuff"
extract_archive "$cache_dir/scf124.lzh" "$work_dir/scform"

patch_dir=$work_dir/pcplus-patched
mkdir -p -- "$patch_dir"
cp -p -- "$work_dir/pcplus/PCP108/PCPLUS.SYS" "$patch_dir/"
cp -p -- "$work_dir/pcplus-patch/PCPLUS.BDF" "$patch_dir/"
cp -p -- "$work_dir/bdiff/BUPDATE.EXE" "$patch_dir/"

dosbox_conf=$work_dir/dosbox.conf
printf '%s\n' \
	'[midi]' \
	'mpu401=none' \
	'mididevice=none' \
	'[sblaster]' \
	'sbtype=none' \
	'[gus]' \
	'gus=false' \
	'[speaker]' \
	'pcspeaker=false' \
	'tandy=off' \
	'disney=false' >"$dosbox_conf"

printf '%s\n' 'Applying the published PCPLUS 1.08 DMA-mask bug-fix'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox -conf "$dosbox_conf" -exit \
	-c "mount c $patch_dir" \
	-c 'c:' \
	-c 'bupdate -x -i -o pcplus.bdf > pcp.log' \
	-c 'exit' >/dev/null 2>&1

[[ -f $patch_dir/PCPLUS.SYS ]] ||
	die 'DOS patch did not create PCPLUS.SYS'
verify_sha256 "$patch_dir/PCPLUS.SYS" \
	f86d03201a2fa6c0dab13345df55f3bb929f41ec3c7c6d03efb4dbd7935f1b06 ||
	die 'DOS patch created unexpected contents: PCPLUS.SYS'

payload_dir=$work_dir/payload
mkdir -p -- "$payload_dir/root" "$payload_dir/bin" "$payload_dir/doc"

copy_payload() {
	cp -- "$1" "$payload_dir/$2"
}

copy_payload "$patch_dir/PCPLUS.SYS" root/PCPLUS.SYS
copy_payload "$work_dir/schd/SCHD.SYS" root/SCHD.SYS

copy_payload "$work_dir/vbuff/VBUFF.COM" bin/VBUFF.COM
copy_payload "$work_dir/scform/SCFORM.COM" bin/SCFORM.COM

copy_payload "$work_dir/pcplus/PCP108/PCPLUS.DOC" doc/PCPLUS.DOC
copy_payload "$work_dir/pcplus/PCP108/PCPLUS.TXT" doc/PCPLUS.TXT
copy_payload "$work_dir/pcplus/PCP108/SCSIVA/SCSI55.TXT" doc/SCSI55.TXT
copy_payload "$work_dir/schd/SCHD.DOC" doc/SCHD.DOC
copy_payload "$work_dir/schd/SCHD.LOG" doc/SCHD.LOG
copy_payload "$work_dir/schd/SCHD.TXT" doc/SCHD.TXT
copy_payload "$work_dir/vbuff/VBUFF.DOC" doc/VBUFF.DOC
copy_payload "$work_dir/vbuff/VBUFF.LOG" doc/VBUFF.LOG
copy_payload "$work_dir/scform/SCFORM.DOC" doc/SCFORM.DOC
copy_payload "$work_dir/scform/SCFORM.LOG" doc/SCFORM.LOG

printf '%s\r\n' \
	'FILES = 20' \
	'BUFFERS = 10' \
	'DEVICE = A:\PCPLUS.SYS' \
	"DEVICE = A:\\SCHD.SYS -I$scsi_id" >"$payload_dir/root/CONFIG.SYS"

printf '%s\r\n' \
	'PATH A:\BIN' \
	'SET COMSPEC=A:\PCENGINE.COM' >"$payload_dir/root/AUTOEXEC.BAT"

vanilla_d88=$work_dir/vanilla.d88
"$script_dir/create-vanilla-system-disk.sh" \
	--source "$source_d88" \
	--output "$vanilla_d88"

output_tmp=$(mktemp "$output_d88.tmp.XXXXXX")
cp -- "$vanilla_d88" "$output_tmp"
python3 "$script_dir/pcengine_disk.py" install \
	--image "$output_tmp" \
	--payload "$payload_dir"
python3 "$script_dir/pcengine_disk.py" list \
	--image "$output_tmp" >/dev/null
chmod 0644 "$output_tmp"
move_with_retry "$output_tmp" "$output_d88" ||
	die 'could not finalize output after three attempts'
output_tmp=

printf 'Created bootable SCSI support disk for target ID %s: %s\n' \
	"$scsi_id" "$output_d88"

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
source_d88=
output_d88=
cache_dir=${HOME}/.cache/vaeg/auto-generated-pc88va-utility-media
sqemm_driver=
sqemm_license=
work_dir=
prj_work_dir=
output_tmp=
download_tmp=

usage() {
	printf '%s\n' \
		"Usage: $program_name --source SOURCE.d88 --output OUTPUT.d88" \
		'' \
		'Create a bootable PC-Engine 1.1 disk containing pinned PC-88VA' \
		'Softlib downloads in A:\ARCHIVE, plus extracted 16-bit tools.' \
		'EMMVA/SQEMM98/RDEMS drivers are installed in A:\SYS. A root' \
		'CONFIG.SYS records the required load order.' \
		'' \
		'Options:' \
		'  --cache DIR             Select the public-package download cache.' \
		'  --sqemm-driver FILE     Use an already built SQEMM98.SYS.' \
		'  --sqemm-license FILE    Combined licenses paired with that driver.' \
		'The source and generated D88 images are never added to the repository.'
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
	if [[ -n ${work_dir} && ${work_dir} == "${TMPDIR:-/tmp}"/vaeg-pc88va-softlib.* && -d ${work_dir} ]]; then
		rm -rf -- "$work_dir"
	fi
	if [[ -n ${prj_work_dir} && ${prj_work_dir} == "${TMPDIR:-/tmp}"/vaeg-prj-plus.* && -d ${prj_work_dir} ]]; then
		rm -rf -- "$prj_work_dir"
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

[[ -n ${source_d88} ]] || die '--source is required'
[[ -n ${output_d88} ]] || die '--output is required'
[[ -f ${source_d88} && -r ${source_d88} ]] || die 'source D88 is not a readable file'
[[ ! -e ${output_d88} ]] || die 'output already exists; refusing to overwrite it'
[[ -d ${output_d88%/*} || ${output_d88} != */* ]] || die 'output directory does not exist'
if [[ -n ${sqemm_driver} || -n ${sqemm_license} ]]; then
	[[ -n ${sqemm_driver} && -n ${sqemm_license} ]] ||
		die '--sqemm-driver and --sqemm-license must be used together'
	[[ -f ${sqemm_driver} && -r ${sqemm_driver} ]] || die 'SQEMM98 driver is not readable'
	[[ -f ${sqemm_license} && -r ${sqemm_license} ]] || die 'SQEMM license is not readable'
fi

for required_command in curl lha python3 sha256sum unzip; do
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
	local gnum=$3
	local url="http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=$gnum&fname=$name"

	fetch_package "$name" "$expected" "$url"
}

fetch_prj_plus_file() {
	local path=$1
	local expected=$2
	local destination=$prj_work_dir/source/$path
	local url="https://raw.githubusercontent.com/mazone-ma3/PRJ_PLUS/ed4036bf70a8e03d926d0b8a943208e909810f2a/$path"

	mkdir -p -- "${destination%/*}"
	printf 'Fetching PRJ_PLUS/%s\n' "$path"
	if ! curl --fail --location --silent --show-error --retry 3 --connect-timeout 20 \
		--output "$destination" "$url"; then
		die "download failed: PRJ_PLUS/$path"
	fi
	verify_sha256 "$destination" "$expected" ||
		die "downloaded PRJ_PLUS file has the wrong SHA-256: $path"
}

fetch_prj_plus_archive() {
	local name=PRJVA.ZIP
	local expected=e804bdae01bad0701aac8ffd62d415288bed8cdf65cb9ef0acafc994e2d2a61b
	local destination=$cache_dir/$name

	if [[ -f ${destination} ]]; then
		verify_sha256 "$destination" "$expected" ||
			die "cached package has the wrong SHA-256: $name"
		printf 'Using cached %s\n' "$name"
		return
	fi

	prj_work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-prj-plus.XXXXXX")
	fetch_prj_plus_file LICENSE \
		fd44c187957ca28c4c2b5407a108d8db5ab3a8f92cd9cafa35734067b1be7456
	fetch_prj_plus_file README.md \
		40808e6c2567ae3c0df3295cb04c437c0cf7b00847b50fbe21366af3551d890b
	fetch_prj_plus_file PC88VA/BIN/FONTYOKO.SC5 \
		57b45acf19ad0761fe04e5628f3fa4e1c8ae536a9145793e038e6084d76a2d0a
	fetch_prj_plus_file PC88VA/BIN/PLUSTAKE.SC5 \
		0cdbdb68b8bdcdca873d6d9a09fbfe3a67bcc746e2e9c76ed4c2181b6cec8e4a
	fetch_prj_plus_file PC88VA/BIN/plustakerva.exe \
		e391c142af17963bd803d382cc37ecbff5de04375247029f91096c107af20f51
	fetch_prj_plus_file PC88VA/WIN64.BAT \
		a8dcc21df6f9268bcacd281a9e1155e81631e5335cab3eac93cee0ce7f228f28
	fetch_prj_plus_file PC88VA/inkey.h \
		0a8a9b2bf7f7ed0decc38ea267a309c68da9095c9661b617f28248d16948f49e
	fetch_prj_plus_file PC88VA/makefile \
		64e01bac977d3c8dc9860bc132d2085f97482fc0089b6a31f6d1833c93fee983
	fetch_prj_plus_file PC88VA/plustakerva.c \
		d6266e23cc704c77c191a71f1a876eb9c7172751a1a8a17dfbdc010f1857dec4

	download_tmp=$prj_work_dir/$name
	python3 "$script_dir/create-stored-zip.py" \
		--source "$prj_work_dir/source" \
		--output "$download_tmp" \
		--comment 'PRJ_PLUS ed4036bf70a8e03d926d0b8a943208e909810f2a'
	verify_sha256 "$download_tmp" "$expected" ||
		die "generated package has the wrong SHA-256: $name"
	move_with_retry "$download_tmp" "$destination" ||
		die "could not finalize cached package after three attempts: $name"
	download_tmp=
	rm -rf -- "$prj_work_dir"
	prj_work_dir=
}

fetch_softlib_package VBUFF102.LZH \
	c51d2f9bd04efeda77760a2c8e476777c07edb8775a7120793dd98bc0a8ff01f 452
fetch_softlib_package ALGO_VA.DOC \
	47af2b89f09b3123fdc874ac5569d9c151baf1e791f440690715d85d159913a4 390
fetch_softlib_package ALGO_VA.LZH \
	7fab7bd72912560969da0a829faff4d7d9c4aabc7dfaf5be63e6e2abecb86965 390
fetch_softlib_package 2HCDRSRC.LZH \
	69a380af1ee74ee9d4e2fc6d536d4aa87aba0280c52808863e6cc3f41be2331e 400
fetch_softlib_package EMACSVA.LZH \
	64d496d67668f7d5bd071ff304ed33a689b0795fa793afac9e631346070c8a8a 435
fetch_softlib_package EMACSVA.DOC \
	e77458e3f9a10d82c4dec6aad86b5e635b930f669012ce73fbbeff6e7f21d73a 435
fetch_softlib_package CPMVA.LZH \
	c5188efa73c80609e2184890d5a1ee5f0b274f8d29a3d73ae370fe7526d9dccd 424
fetch_softlib_package FDFRMSRC.LZH \
	d81358cbcfc1d6175359059d9c01fb75e5585993c3bc3d3e1fc988d7aa7c3e5a 401
fetch_softlib_package RDPCM001.LZH \
	a823296e0fc56927f9cf332cf3da6d1670469fada79675ba968fda1fa351891b 396
fetch_softlib_package RDPCM001.DOC \
	7321b1818176afb08c1492809b26f774518b2c730b69fc2c1340ff7ee67ce524 396
fetch_softlib_package 2HCDRV.ZIP \
	1da4d799b1aaf3a2fc94f8872eb3fd2cf6eb788fb907e8ba8c85cc79b0487e39 306
fetch_softlib_package EMMVA15A.LZH \
	1ec9bb379291f1402475afc1f6e2784b5aeda922a8310bde230e29d18c2c493c 351
fetch_softlib_package JFPPAT.ZIP \
	900e2ee9b7a3562ff1f8f9f0a4bbbd82bbd248a35f49768d5dc34607b9c194b0 307
fetch_softlib_package RDEMS152.LZH \
	0ba023a9f82defca085dc13d7103fe5b2a788ef9217d660686f2a20d8b0e70f9 270
fetch_softlib_package TDC10.LZH \
	c6c31cf6a604b07220c88a010bcd2e40cdb009dd4601e7d8a0ad903a4f2df23e 201
fetch_softlib_package BENCH003.DOC \
	a2034c794d7d3e974b77091341a5a883240a72560eb5d6cb505e1fcfcd18e9b7 389
fetch_softlib_package BENCH003.LZH \
	40f5fbf391d416a79d843c13e11797abfd8ff49ea45a7d6f8a627cb389d9c79c 389
fetch_package LSIC330C.LZH \
	c8c4c49aed600fb2413cf5707ef01b2f4057de69196c3478d5226bf1b224081b \
	'https://ftp.vector.co.jp/00/11/980/lsic330c.lzh'
fetch_package X8MAP130.LZH \
	fc5bba93771e0fff3c8aa3d9ef942b80f65afba51b632a6336638ab70a7648a7 \
	'https://ftp.vector.co.jp/27/37/386/x8map130.lzh'
fetch_prj_plus_archive
fetch_package UNZ532X3.EXE \
	cb55dee22473caf143353938da76e61d5574c5edead7b321c14b8900c0b493ce \
	'https://www.ibiblio.org/pub/micro/pc-stuff/freedos/mirrors/gnuish/dos_only/unz532x3.exe'
fetch_package ZIP22X.ZIP \
	f0048e0003d0a115624c086e9570355b8689cdd1376b4f6fc7dc4f55cb6eb9a5 \
	'https://www.ibiblio.org/pub/micro/pc-stuff/freedos/mirrors/gnuish/dos_only/zip22x.zip'

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-pc88va-softlib.XXXXXX")
payload_dir=$work_dir/payload
mkdir -p -- "$payload_dir/root" "$payload_dir/archive" "$payload_dir/bin" "$payload_dir/doc" \
	"$payload_dir/sys" "$work_dir/emmva" "$work_dir/rdems" \
	"$work_dir/infozip/unzip" "$work_dir/infozip/zip" \
	"$work_dir/emacsva" "$work_dir/cpmva" "$work_dir/tdc10" \
	"$work_dir/bench003" "$work_dir/2hcdrv" "$work_dir/jfppat" \
	"$work_dir/vbuff" "$work_dir/rdpcm" "$work_dir/prjva"

unzip -q "$cache_dir/UNZ532X3.EXE" \
	unzip.exe unzip.doc COPYING README.DOS \
	-d "$work_dir/infozip/unzip"
unzip -q "$cache_dir/ZIP22X.ZIP" \
	zip.exe MANUAL README \
	-d "$work_dir/infozip/zip"
mkdir -p -- "$work_dir/x8map"
lha xfw="$work_dir/x8map" "$cache_dir/X8MAP130.LZH" >/dev/null
for file in X8MAP.COM X8MAP130.SMP X8MAP130.TXT; do
	cp -- "$work_dir/x8map/$file" "$payload_dir/bin/$file"
done
lha xfw="$work_dir/emmva" "$cache_dir/EMMVA15A.LZH" >/dev/null
lha xfw="$work_dir/rdems" "$cache_dir/RDEMS152.LZH" >/dev/null
lha xfw="$work_dir/emacsva" "$cache_dir/EMACSVA.LZH" >/dev/null
lha xfw="$work_dir/cpmva" "$cache_dir/CPMVA.LZH" >/dev/null
lha xfw="$work_dir/tdc10" "$cache_dir/TDC10.LZH" >/dev/null
lha xfw="$work_dir/bench003" "$cache_dir/BENCH003.LZH" >/dev/null
unzip -q "$cache_dir/2HCDRV.ZIP" 2HCDRV.COM FDFORM.COM -d "$work_dir/2hcdrv"
unzip -q "$cache_dir/JFPPAT.ZIP" JFPPAT.SYS -d "$work_dir/jfppat"
lha xfw="$work_dir/vbuff" "$cache_dir/VBUFF102.LZH" >/dev/null
lha xfw="$work_dir/rdpcm" "$cache_dir/RDPCM001.LZH" >/dev/null
unzip -q "$cache_dir/PRJVA.ZIP" 'PC88VA/BIN/plustakerva.exe' -d "$work_dir/prjva"
cp -- "$work_dir/emmva/EMMVA01.SYS" "$work_dir/emmva/EMMVA02.SYS" "$payload_dir/sys/"
cp -- "$work_dir/rdems/RDEMS.SYS" "$payload_dir/sys/RDEMS.SYS"

if [[ -n ${sqemm_driver} ]]; then
	python3 "$repo_root/tools/openwatcom/check-sqemm98.py" "$sqemm_driver"
	cp -- "$sqemm_driver" "$payload_dir/sys/SQEMM98.SYS"
	cp -- "$sqemm_license" "$payload_dir/doc/SQEMM.LIC"
else
	"$repo_root/tools/openwatcom/build-sqemm98.sh" \
		--output "$payload_dir/sys/SQEMM98.SYS" \
		--license-output "$payload_dir/doc/SQEMM.LIC" \
		--cache "$cache_dir/sqemm98"
fi

printf '%s\r\n' \
	'DEVICE=A:\SYS\EMMVA01.SYS' \
	'DEVICE=A:\SYS\SQEMM98.SYS' \
	'DEVICE=A:\SYS\EMMVA02.SYS' \
	'DEVICE=A:\SYS\RDEMS.SYS -P40 -A' \
	> "$payload_dir/root/CONFIG.SYS"

printf '%s\r\n' \
	'SQEMM98 MAX v0.8 for PC-88VA' \
	'' \
	'SQEMM98 is built from pinned SQEMM 0.8 source with Open Watcom.' \
	'It drives the vaeg PC-88VA EMS board and reports initialization' \
	'messages through the PC-Engine Text BIOS INT 83H/AH=02H service.' \
	'' \
	'This supplemental disk retains the PC-Engine 1.1 IPL and system' \
	'files from its source image and is intended to boot directly.' \
	> "$payload_dir/doc/SQEMM98.TXT"

for package in \
	VBUFF102.LZH ALGO_VA.DOC ALGO_VA.LZH 2HCDRSRC.LZH \
	EMACSVA.LZH EMACSVA.DOC CPMVA.LZH FDFRMSRC.LZH \
	RDPCM001.LZH RDPCM001.DOC 2HCDRV.ZIP EMMVA15A.LZH \
	JFPPAT.ZIP RDEMS152.LZH TDC10.LZH BENCH003.DOC BENCH003.LZH \
	LSIC330C.LZH X8MAP130.LZH PRJVA.ZIP; do
	cp -- "$cache_dir/$package" "$payload_dir/archive/$package"
done

cp -- "$work_dir/infozip/unzip/unzip.exe" "$payload_dir/bin/UNZIP.EXE"
cp -- "$work_dir/infozip/zip/zip.exe" "$payload_dir/bin/ZIP.EXE"
cp -- "$work_dir/emacsva/EMACS.EXE" "$payload_dir/bin/EMACS.EXE"
cp -- "$work_dir/cpmva/CPMBIOS.COM" "$payload_dir/bin/CPMBIOS.COM"
cp -- "$work_dir/cpmva/CPMVA.EXE" "$payload_dir/bin/CPMVA.EXE"
cp -- "$work_dir/cpmva/DO.COM" "$payload_dir/bin/DO.COM"
cp -- "$work_dir/cpmva/EXIT.COM" "$payload_dir/bin/EXIT.COM"
cp -- "$work_dir/cpmva/FCONV.COM" "$payload_dir/bin/FCONV.COM"
cp -- "$work_dir/cpmva/RDCPM.EXE" "$payload_dir/bin/RDCPM.EXE"
cp -- "$work_dir/tdc10/TDC.COM" "$payload_dir/bin/TDC.COM"
cp -- "$work_dir/bench003/BENCH.EXE" "$payload_dir/bin/BENCH.EXE"
cp -- "$work_dir/2hcdrv/2HCDRV.COM" "$payload_dir/bin/2HCDRV.COM"
cp -- "$work_dir/2hcdrv/FDFORM.COM" "$payload_dir/bin/FDFORM.COM"
cp -- "$work_dir/vbuff/VBUFF.COM" "$payload_dir/bin/VBUFF.COM"
cp -- "$work_dir/prjva/PC88VA/BIN/plustakerva.exe" "$payload_dir/bin/PLUSTAKE.EXE"
cp -- "$work_dir/jfppat/JFPPAT.SYS" "$payload_dir/sys/JFPPAT.SYS"
cp -- "$work_dir/rdpcm/RDPCM.SYS" "$payload_dir/sys/RDPCM.SYS"
cp -- "$work_dir/infozip/unzip/COPYING" "$payload_dir/doc/COPYING"
cp -- "$work_dir/infozip/unzip/unzip.doc" "$payload_dir/doc/UNZIP.DOC"
cp -- "$work_dir/infozip/unzip/README.DOS" "$payload_dir/doc/UNZDOS.TXT"
cp -- "$work_dir/infozip/zip/MANUAL" "$payload_dir/doc/ZIP.DOC"
cp -- "$work_dir/infozip/zip/README" "$payload_dir/doc/ZIPREAD.TXT"

cp -- "$work_dir/emmva/EMMVA150.DOC" "$payload_dir/doc/EMMVA150.DOC"

cp -- "$work_dir/rdems/RDEMS152.MAN" "$payload_dir/doc/RDEMS152.MAN"
system_d88=$work_dir/system.d88
python3 "$script_dir/pcengine_disk.py" vanilla \
	--source "$source_d88" \
	--output "$system_d88"

output_tmp=$(mktemp "$output_d88.tmp.XXXXXX")
cp -- "$system_d88" "$output_tmp"
python3 "$script_dir/pcengine_disk.py" install \
	--image "$output_tmp" \
	--payload "$payload_dir"
chmod 0644 "$output_tmp"
move_with_retry "$output_tmp" "$output_d88" ||
	die 'could not finalize output after three attempts'
output_tmp=

printf 'Created PC-88VA Softlib archive disk: %s\n' "$output_d88"

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
work_dir=
output_tmp=
download_tmp=

usage() {
	printf '%s\n' \
		"Usage: $program_name --source SOURCE.d88 --output OUTPUT.d88" \
		'' \
		'First create a vanilla PC-Engine 1.1 system disk, then add PCPLUS,' \
		'SCHD, HOSTFAT, PCEPAT, RESET, TSCLVA, MSE 3.52b, RDBMS, RDPCM,' \
		'BMSDRVA, EMMVA/SQEMM98/RDEMS,' \
		'development tools, ISHVA/PKPAK, TENIM3, TFD, SCFORM, VIEW480,' \
		'JFPPAT, 2HCDRV, FDFORM, X8MAP,' \
		'and K-Launcher.' \
		'The FDD uses the documented compact profile;' \
		'EMACS/CPMVA/TDC/BENCH and other large collections are SASI-only.' \
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
	if [[ -n ${work_dir} && ${work_dir} == "${TMPDIR:-/tmp}"/vaeg-pc88va-devdisk.* && -d ${work_dir} ]]; then
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

for required_command in awk curl dd dosbox lha nasm od python3 sha256sum tar unzip; do
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

patch_rdbms_default_port() {
	local path=$1
	local before
	local after

	verify_sha256 "$path" \
		7ead949be781303f12c3fc1bf499de3d59a504acea69747d90e21bb4109d5d49 ||
		die 'RDBMS.SYS has unexpected original contents'
	before=$(od -An -tx1 -j 26 -N 2 "$path" | tr -d '[:space:]')
	[[ ${before} == ec00 ]] ||
		die 'RDBMS.SYS does not contain the expected 00ECH default port'
	printf '\320\001' | dd of="$path" bs=1 seek=26 conv=notrunc status=none
	after=$(od -An -tx1 -j 26 -N 2 "$path" | tr -d '[:space:]')
	[[ ${after} == d001 ]] ||
		die 'could not set the RDBMS.SYS default port to 01D0H'
	verify_sha256 "$path" \
		8a4e09f9f2b1b1363a3d07a1edeb36ae744665324a7de9a1c628e6480a5f0289 ||
		die 'patched RDBMS.SYS has unexpected contents'
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
	mv -- "$download_tmp" "$destination"
	download_tmp=
}

fetch_package pcepat.com \
	59296bcb77b158ce072a7f62bdbdca420305fb43004f69845345efc73c276945 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=330&fname=PCEPAT.COM'
fetch_package reset.zip \
	e6e18f8f0766f6dbf04e91a51d964ce0d50ee067ad57fe1f098428351b8ffe04 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=340&fname=RESET.ZIP'
fetch_package pcp108.lzh \
	4561df318cdfb08bdf8276741058ca2b0d4a4eeaf084a8c5b587f520f6c9e3f0 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=378&fname=PCP108.LZH'
fetch_package pcp108p.lzh \
	25f1d9432247c88667b880f4153966725a757b4dcd1a062c88497d32b0c8eef7 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=451&fname=PCP108P.LZH'
fetch_package schd155t.lzh \
	87aebcf7c9bc9c6170a40d0e6ddcce5afdcbb1fa55f1fdeeec815458f7ef065f \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=448&fname=SCHD155T.LZH'
fetch_package rdpcm001.lzh \
	a823296e0fc56927f9cf332cf3da6d1670469fada79675ba968fda1fa351891b \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=388&fname=RDPCM001.LZH'
fetch_package tsclva.zip \
	ca5250865b05f2e31342b84418cf2720ad42297e1760bfaa19fda6bdf191aa5e \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=309&fname=TSCLVA.ZIP'
fetch_package tsclbdf.zip \
	99e0c2ccc755ccbd3b4d2ee7301bc9b8655de66160afe794b3ee110d7850e4f1 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=346&fname=TSCLBDF.ZIP'
fetch_package rdbms121.lzh \
	bf198dbf104a9ddf4b0309f53b3f8e7266ac83f9810162cee51c735403e9559c \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=80&fname=RDBMS121.LZH'
fetch_package bdiff128.lzh \
	0ba491ee4829a6f292cfbcad25371a98c2161c1a92d028b0d2fd5dd9d9011153 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=328&fname=BDIFF128.LZH'
fetch_package mse352a.lzh \
	bdbe863b4eb451692d5450b9ae754260fec8b3481a4936f1c38ffa850b4a9cbd \
	'https://web.archive.org/web/20060220170035id_/http://hp.vector.co.jp:80/authors/VA015636/mse352a.lzh'
fetch_package mse352bf.lzh \
	46007ecf062fc32c3be04f3a6715ca38e3f086f570f817b11a5183aef9254b9a \
	'https://web.archive.org/web/20060517104746id_/http://hp.vector.co.jp:80/authors/VA015636/mse352bf.lzh'
fetch_package wsp150.lzh \
	e2c9ebfcf2aea495baab186cab7a1ac790027f7ea93e41650f1744c5ccb594b3 \
	'https://ftp.vector.co.jp/00/08/531/wsp150.lzh'
fetch_package lha255.exe \
	70c9fb00d4d5e272662d1f25316ad59007c426894802ea30b61dd729706e715e \
	'https://ftp.vector.co.jp/00/24/521/lha255.exe'
fetch_package lha255b_.lzh \
	f081e1203ad695a608a091a3c3c48422a8934f55e9c0c43ad12de6edd40d8f1e \
	'https://ftp.vector.co.jp/00/24/521/lha255b_.lzh'
fetch_package diet144.lzh \
	e4012ca98f010d3120afc04deccb87b61e67e3c0428c7692c7850b86ce6299d9 \
	'https://ftp.vector.co.jp/00/03/527/diet144.lzh'
fetch_package x8map130.lzh \
	fc5bba93771e0fff3c8aa3d9ef942b80f65afba51b632a6336638ab70a7648a7 \
	'https://ftp.vector.co.jp/27/37/386/x8map130.lzh'
fetch_package emmva15a.lzh \
	1ec9bb379291f1402475afc1f6e2784b5aeda922a8310bde230e29d18c2c493c \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=351&fname=EMMVA15A.LZH'
fetch_package rdems152.lzh \
	0ba023a9f82defca085dc13d7103fe5b2a788ef9217d660686f2a20d8b0e70f9 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=270&fname=RDEMS152.LZH'
fetch_package kl130.lzh \
	8b8e2b23d3da27cf4089e283f49d923e884611a11e213532afa77b5fb4246dfb \
	'https://toroidj.github.io/dos/KL130.LZH'
fetch_package bms15020.tgz \
	b0ee1dc6679ecad155ed9aabc2aa66f253c9d5f1c0190cf2110cc68e59f7b405 \
	'https://ftp.vector.co.jp/09/04/385/bms15020.tgz'
fetch_package teen030p.lzh \
	9b6bdd4b2dbc4908d5a749994cdb87c63e99b13cb294c108608ce4f04248c71e \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=470&fname=TEEN030P.LZH'
fetch_package vbuff102.lzh \
	c51d2f9bd04efeda77760a2c8e476777c07edb8775a7120793dd98bc0a8ff01f \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=452&fname=VBUFF102.LZH'
fetch_package fatmap11.lzh \
	9e25c73df9d589306ae24c3908fb3b8e4ee2b1c6f306a1b8eb07155a60e2e701 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=430&fname=FATMAP11.LZH'
fetch_package forg203.lzh \
	1315141e7e6c37d010ef9a725a927fa2ba71e4e086d630d2aa942a704a7ae5c4 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=431&fname=FORG203.LZH'
fetch_package ramdisk.com \
	e0cf4510f4f54ee2825c866ee3a2b07fb2e5f60b7e8d10bfa34401a29e7e4b51 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=398&fname=RAMDISK.COM'
fetch_package fut312bx.zip \
	49df5a5f68b91f64affc9f305a328f0925e07cbe88604e17687c653a523eabe5 \
	'https://www.ibiblio.org/pub/micro/pc-stuff/freedos/mirrors/gnuish/dos_only/fut312bx.zip'
fetch_package scf124.lzh \
	a62183d66da90546d19d81f8adad32a2df2485d619badcaf2c167668b7603aad \
	'http://www.pc88.gr.jp/forum/download.php?id=15'
fetch_package v480src.lzh \
	ddb5623a46169e9e8bc6dd8394d4ad1051ce571b51662a78210b40bfe9e46b20 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=404&fname=V480SRC.LZH'
fetch_package jfppat.zip \
	900e2ee9b7a3562ff1f8f9f0a4bbbd82bbd248a35f49768d5dc34607b9c194b0 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=307&fname=JFPPAT.ZIP'
fetch_package 2hcdrv.zip \
	1da4d799b1aaf3a2fc94f8872eb3fd2cf6eb788fb907e8ba8c85cc79b0487e39 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=306&fname=2HCDRV.ZIP'
fetch_package fdfrmsrc.lzh \
	d81358cbcfc1d6175359059d9c01fb75e5585993c3bc3d3e1fc988d7aa7c3e5a \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=401&fname=FDFRMSRC.LZH'
fetch_package isharc.com \
	c51ffc66551f55872532b0ebde186ac7e9f76ef9c8287c731190f7e433bba61f \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=69&fname=ISHARC.COM'
fetch_package isharc.doc \
	d3ec746ac0ce93bd9b4995a0fd1cb53de3289b914a7019c47e7b01d440657fc3 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=69&fname=ISHARC.DOC'
fetch_package tenim3.com \
	f7b89107dc0a6a7c4ce53547418400f68a62bc4d2206a97f02489dfb1d556eb8 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=74&fname=TENIM3.COM'
fetch_package tenim3.doc \
	e369fd63be567e19080fc89be41e43787bb32d9742a8be2f3995826f8d731fb6 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=74&fname=TENIM3.DOC'
fetch_package tfd12.lzh \
	bf970f49d787ac2870399db5020d3936ad36626bf0c87dd1535fb62e556794c1 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=348&fname=TFD12.LZH'
fetch_package tfd12.doc \
	65380f66c08120fed9e94c508ec491faff8bedc86cd2b48af3d7126c81ec499f \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=348&fname=TFD12.DOC'

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-pc88va-devdisk.XXXXXX")

extract_archive() {
	local archive=$1
	local destination=$2

	mkdir -p -- "$destination"
	lha xfw="$destination" "$archive" >/dev/null
}

add_uppercase_aliases() {
	local root=$1
	local path
	local parent
	local name
	local uppercase
	local alias_path

	while IFS= read -r -d '' path; do
		parent=${path%/*}
		name=${path##*/}
		uppercase=$(printf '%s' "$name" | LC_ALL=C tr '[:lower:]' '[:upper:]')
		[[ ${name} != "${uppercase}" ]] || continue
		alias_path=$parent/$uppercase
		[[ -e ${alias_path} ]] || ln -s -- "$name" "$alias_path"
	done < <(find "$root" -depth -mindepth 1 -print0)
}

extract_archive "$cache_dir/pcepat.com" "$work_dir/pcepat"
extract_archive "$cache_dir/pcp108.lzh" "$work_dir/pcp108"
extract_archive "$cache_dir/pcp108p.lzh" "$work_dir/pcp108p"
extract_archive "$cache_dir/schd155t.lzh" "$work_dir/schd"
extract_archive "$cache_dir/rdpcm001.lzh" "$work_dir/rdpcm"
mkdir -p -- "$work_dir/reset"
unzip -q "$cache_dir/reset.zip" -d "$work_dir/reset"
mkdir -p -- "$work_dir/tsclva"
unzip -q "$cache_dir/tsclva.zip" -d "$work_dir/tsclva"
mkdir -p -- "$work_dir/tsclbdf"
unzip -q "$cache_dir/tsclbdf.zip" -d "$work_dir/tsclbdf"
extract_archive "$cache_dir/rdbms121.lzh" "$work_dir/rdbms"
patch_rdbms_default_port "$work_dir/rdbms/RDBMS.SYS"
extract_archive "$cache_dir/bdiff128.lzh" "$work_dir/bdiff"
extract_archive "$cache_dir/mse352a.lzh" "$work_dir/mse352a"
extract_archive "$cache_dir/mse352bf.lzh" "$work_dir/mse352bf"
extract_archive "$cache_dir/wsp150.lzh" "$work_dir/wsp"
extract_archive "$cache_dir/lha255.exe" "$work_dir/lha"
extract_archive "$cache_dir/lha255b_.lzh" "$work_dir/lha_patch"
extract_archive "$cache_dir/diet144.lzh" "$work_dir/diet"
extract_archive "$cache_dir/x8map130.lzh" "$work_dir/x8map"
extract_archive "$cache_dir/emmva15a.lzh" "$work_dir/emmva"
extract_archive "$cache_dir/rdems152.lzh" "$work_dir/rdems"
extract_archive "$cache_dir/kl130.lzh" "$work_dir/kl"
extract_archive "$cache_dir/teen030p.lzh" "$work_dir/teen"
extract_archive "$cache_dir/vbuff102.lzh" "$work_dir/vbuff"
extract_archive "$cache_dir/fatmap11.lzh" "$work_dir/fatmap"
extract_archive "$cache_dir/forg203.lzh" "$work_dir/forg"
extract_archive "$cache_dir/ramdisk.com" "$work_dir/ramdisk"
mkdir -p -- "$work_dir/bms"
tar -xzf "$cache_dir/bms15020.tgz" -C "$work_dir/bms"
mkdir -p -- "$work_dir/fut312bx"
unzip -q "$cache_dir/fut312bx.zip" 'BIN/*' -d "$work_dir/fut312bx"
extract_archive "$cache_dir/scf124.lzh" "$work_dir/scform"
extract_archive "$cache_dir/v480src.lzh" "$work_dir/v480"
mkdir -p -- "$work_dir/jfppat"
unzip -q "$cache_dir/jfppat.zip" -d "$work_dir/jfppat"
mkdir -p -- "$work_dir/2hcdrv"
unzip -q "$cache_dir/2hcdrv.zip" -d "$work_dir/2hcdrv"
extract_archive "$cache_dir/fdfrmsrc.lzh" "$work_dir/fdfrmsrc"
extract_archive "$cache_dir/tenim3.com" "$work_dir/tenim3"
extract_archive "$cache_dir/tfd12.lzh" "$work_dir/tfd12"
common_stage_dir=$work_dir/common-tools
common_stage_manifest=$work_dir/common-tools.manifest.tsv
"$script_dir/stage-development-tools.sh" --profile fdd \
	--output "$common_stage_dir" \
	--manifest "$common_stage_manifest" \
	--ish-archive "$cache_dir/isharc.com" --ish-doc "$cache_dir/isharc.doc"

manifest_contains() {
	local relative=$1
	awk -F '\t' -v wanted="$relative" \
		'$1 == "fdd" && $2 == wanted {found = 1} END {exit !found}' \
		"$common_stage_manifest"
}

for staged_path in \
	BIN/ISHVA.COM BIN/PKPAK.EXE BIN/PKUNPAK.EXE \
	DOC/ISHARC.DOC DOC/ISHVA.DOC ARCHIVE/ISHARC.COM; do
	manifest_contains "$staged_path" ||
		die "common staging manifest is missing $staged_path"
done
add_uppercase_aliases "$work_dir"

stage_dir=$work_dir/stage
mkdir -p -- "$stage_dir"
cp -p -- "$work_dir/wsp/wsp.com" "$stage_dir/WSP.COM"
cp -p -- "$work_dir/mse352a/ALIAS.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE350.DEF" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE352A.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE352A.DOC" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE352A.HIS" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSECUST.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSET.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/README.DOC" "$stage_dir/"
cp -p -- "$work_dir/mse352bf/MSE352BF.WUP" "$stage_dir/"
cp -p -- "$work_dir/bdiff/BUPDATE.EXE" "$stage_dir/"
cp -p -- "$work_dir/pcp108/PCP108/PCPLUS.SYS" "$stage_dir/"
cp -p -- "$work_dir/pcp108p/PCPLUS.BDF" "$stage_dir/"
cp -p -- "$work_dir/bms/bmsdrva.com" "$stage_dir/BMSDRVA.COM"
cp -p -- "$work_dir/bms/bmsdrsys.wup" "$stage_dir/BMSDRSYS.WUP"
cp -p -- "$work_dir/tsclva/TSCLVA.DOC" "$stage_dir/"
cp -p -- "$work_dir/tsclva/TSCLVA.SYS" "$stage_dir/"
cp -p -- "$work_dir/tsclbdf/TSCLVA.BDF" "$stage_dir/"
cp -p -- "$work_dir/lha/LHA.EXE" "$stage_dir/"
cp -p -- "$work_dir/lha/HISTORY.DOC" "$stage_dir/"
cp -p -- "$work_dir/lha_patch/LHA255B@.BDF" "$stage_dir/"
cp -p -- "$work_dir/kl/KL.COM" "$stage_dir/"
cp -p -- "$work_dir/kl/KLV.EXE" "$stage_dir/"
cp -p -- "$work_dir/kl/KLCUST.EXE" "$stage_dir/"
cp -p -- "$work_dir/kl/KLVA.COM" "$stage_dir/"

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

printf '%s\n' 'Applying LHA, MSE, PCPLUS, BMS, TSCLVA, and K-Launcher patches'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox -conf "$dosbox_conf" -exit \
	-c "mount c $stage_dir" \
	-c 'c:' \
	-c 'bupdate -x -i -o lha255b@ > lha255b.log' \
	-c 'wsp -t -b mse352bf.wup > mse.log' \
	-c 'bupdate -x -i -o pcplus.bdf > pcp.log' \
	-c 'wsp -t -b bmsdrsys.wup > bms.log' \
	-c 'bupdate -x -i -o tsclva.bdf > tscl.log' \
	-c 'wsp -t -b klva.com > kl.log' \
	-c 'exit' >/dev/null 2>&1

verify_generated() {
	local name=$1
	local expected=$2

	[[ -f $stage_dir/$name ]] || die "DOS patch did not create $name"
	verify_sha256 "$stage_dir/$name" "$expected" ||
		die "DOS patch created unexpected contents: $name"
}

verify_generated ALIAS.COM de06d39557440dc1a296b5d5ef80fda9fb7b57f63c67a2d339bc51db9bc12ed7
verify_generated MSE352B.COM 794375496c62bf8f508ccbf57c8ceeb2ab439606d31638eea397eaac6bd3e68a
verify_generated PCPLUS.SYS f86d03201a2fa6c0dab13345df55f3bb929f41ec3c7c6d03efb4dbd7935f1b06
verify_generated BMSDRVA_.SYS 327692d21731b200313f1ccae7167464fdcd479e4cf60c148cba2a069c152974
verify_generated TSCLVA.DOC 6712aa90951acfe03890259b415e0c9389a4c1394d2471eed641f5febe0b612d
verify_generated TSCLVA.SYS 574e8e5a1a5ae72f6d6ae0198f7aa760fb70e6de379d7bd10b1ec0cd2f6eb74e
verify_generated KLL.COM 752600dfb9809432310046047f6142b8edce47c25e25a14c1baa2d91bda87910
verify_generated KLVA.EXE c6ad097435111398f1c1ebc90e9f35cd15caded6b5d6bed49d92c354ab7f3c43
verify_generated KLCUST.EXE 72376b967fe51d4f40759f5d875762fa3b2b09a353afb1a9ea3c957f5a9c87bf
verify_generated LHA.EXE 0794c20ce820c687687fe49285758f026765f4fad9ffb2ff4d78e6a46f7fb452

hostfat_sys=$work_dir/HOSTFAT.SYS
nasm -f bin -o "$hostfat_sys" "$script_dir/hostfat/hostfat.asm"
python3 "$script_dir/hostfat/check_driver.py" --input "$hostfat_sys"

payload_dir=$work_dir/payload
mkdir -p -- "$payload_dir/root" "$payload_dir/bin" "$payload_dir/doc" \
	"$payload_dir/sys" "$payload_dir/archive" "$payload_dir/tmp"

copy_payload() {
	cp -- "$1" "$payload_dir/$2"
}

copy_payload "$work_dir/bms/bmsdrva.com" bin/BMSDRVA.COM
copy_payload "$work_dir/bms/bmsaddva.com" bin/BMSADDVA.COM
copy_payload "$stage_dir/BMSDRVA_.SYS" sys/BMSDRVA.SYS
copy_payload "$stage_dir/PCPLUS.SYS" sys/PCPLUS.SYS
copy_payload "$work_dir/schd/SCHD.SYS" sys/SCHD.SYS
copy_payload "$hostfat_sys" sys/HOSTFAT.SYS
copy_payload "$work_dir/pcepat/PCEPAT.SYS" sys/PCEPAT.SYS
copy_payload "$work_dir/jfppat/JFPPAT.SYS" sys/JFPPAT.SYS
copy_payload "$work_dir/reset/RESET.SYS" sys/RESET.SYS
copy_payload "$stage_dir/MSE352B.COM" sys/MSE352B.COM
copy_payload "$work_dir/rdbms/RDBMS.SYS" sys/RDBMS.SYS
copy_payload "$work_dir/ramdisk/RAMDISK.SYS" sys/RAMDISK.SYS
copy_payload "$work_dir/rdpcm/RDPCM.SYS" sys/RDPCM.SYS
copy_payload "$stage_dir/TSCLVA.SYS" sys/TSCLVA.SYS
copy_payload "$work_dir/emmva/EMMVA01.SYS" sys/EMMVA01.SYS
copy_payload "$work_dir/emmva/EMMVA02.SYS" sys/EMMVA02.SYS
copy_payload "$work_dir/rdems/RDEMS.SYS" sys/RDEMS.SYS

"$repo_root/tools/openwatcom/build-sqemm98.sh" \
	--output "$payload_dir/sys/SQEMM98.SYS" \
	--license-output "$payload_dir/doc/SQEMM.LIC" \
	--cache "$cache_dir"

copy_payload "$stage_dir/LHA.EXE" bin/LHA.EXE
copy_payload "$work_dir/diet/DIET.EXE" bin/DIET.EXE
copy_payload "$work_dir/bdiff/BUPDATE.EXE" bin/BUPDATE.EXE
copy_payload "$work_dir/wsp/WSP.COM" bin/WSP.COM
copy_payload "$stage_dir/MSET.COM" bin/MSET.COM
copy_payload "$stage_dir/ALIAS.COM" bin/ALIAS.COM
copy_payload "$stage_dir/MSECUST.COM" bin/MSECUST.COM
copy_payload "$stage_dir/MSE350.DEF" bin/MSE350.DEF
copy_payload "$work_dir/pcp108/PCP108/BIN/SMSTAT.COM" bin/SMSTAT.COM
copy_payload "$work_dir/pcp108/PCP108/BIN/SETDMA.COM" bin/SETDMA.COM
copy_payload "$work_dir/x8map/X8MAP.COM" bin/X8MAP.COM
copy_payload "$stage_dir/KLL.COM" bin/KLL.COM
copy_payload "$stage_dir/KLVA.EXE" bin/KLVA.EXE
copy_payload "$stage_dir/KLCUST.EXE" bin/KLCUST.EXE
copy_payload "$work_dir/kl/KL.CFG" bin/KL.CFG
copy_payload "$work_dir/kl/KLJPN.HLP" bin/KLJPN.HLP
copy_payload "$work_dir/teen/TEEN.COM" bin/TEEN.COM
copy_payload "$work_dir/teen/TEENM.COM" bin/TEENM.COM
copy_payload "$work_dir/teen/TEEN.DEF" bin/TEEN.DEF
copy_payload "$work_dir/teen/TOPEN.EXE" bin/TOPEN.EXE
copy_payload "$work_dir/teen/TCLOSE.EXE" bin/TCLOSE.EXE
copy_payload "$work_dir/teen/TLOG.COM" bin/TLOG.COM
copy_payload "$work_dir/teen/TLOGBMS.COM" bin/TLOGBMS.COM
copy_payload "$work_dir/vbuff/VBUFF.COM" bin/VBUFF.COM
copy_payload "$work_dir/fatmap/FATMAP.EXE" bin/FATMAP.EXE
copy_payload "$work_dir/fatmap/FATMAP_E.COM" bin/FATMAP_E.COM
copy_payload "$work_dir/forg/FORG.EXE" bin/FORG.EXE
copy_payload "$work_dir/forg/FORG.DAT" bin/FORG.DAT
copy_payload "$work_dir/scform/SCFORM.COM" bin/SCFORM.COM
copy_payload "$work_dir/2hcdrv/2HCDRV.COM" bin/2HCDRV.COM
copy_payload "$work_dir/2hcdrv/FDFORM.COM" bin/FDFORM.COM
copy_payload "$common_stage_dir/BIN/ISHVA.COM" bin/ISHVA.COM
copy_payload "$common_stage_dir/BIN/PKPAK.EXE" bin/PKPAK.EXE
copy_payload "$common_stage_dir/BIN/PKUNPAK.EXE" bin/PKUNPAK.EXE
copy_payload "$work_dir/tfd12/TFD.SYS" sys/TFD.SYS
"$repo_root/tools/openwatcom/build-view480.sh" \
	--source "$work_dir/v480/VIEW480.ASM" \
	--output "$payload_dir/bin/VIEW480.COM"
copy_payload "$work_dir/ramdisk/BIOSFREE.COM" bin/BIOSFREE.COM
copy_payload "$work_dir/ramdisk/SETID.COM" bin/SETID.COM
copy_payload "$work_dir/ramdisk/SETIPL.COM" bin/SETIPL.COM
copy_payload "$work_dir/fut312bx/BIN/CHMOD.EXE" bin/CHMOD.EXE
copy_payload "$work_dir/fut312bx/BIN/COPYING" bin/COPYING
copy_payload "$work_dir/fut312bx/BIN/CP.EXE" bin/CP.EXE
copy_payload "$work_dir/fut312bx/BIN/DD.EXE" bin/DD.EXE
copy_payload "$work_dir/fut312bx/BIN/DF.EXE" bin/DF.EXE
copy_payload "$work_dir/fut312bx/BIN/DI.EXE" bin/DI.EXE
copy_payload "$work_dir/fut312bx/BIN/DU.EXE" bin/DU.EXE
copy_payload "$work_dir/fut312bx/BIN/INSTALL.EXE" bin/INSTALL.EXE
copy_payload "$work_dir/fut312bx/BIN/LS.EXE" bin/LS.EXE
copy_payload "$work_dir/fut312bx/BIN/MKD.EXE" bin/MKD.EXE
copy_payload "$work_dir/fut312bx/BIN/MV.EXE" bin/MV.EXE
copy_payload "$work_dir/fut312bx/BIN/RM.EXE" bin/RM.EXE
copy_payload "$work_dir/fut312bx/BIN/RMD.EXE" bin/RMD.EXE
copy_payload "$work_dir/fut312bx/BIN/TOUCH.EXE" bin/TOUCH.EXE
copy_payload "$work_dir/fut312bx/BIN/VDIR.EXE" bin/VDIR.EXE

copy_payload "$work_dir/teen/TEEN.DOC" doc/TEEN.DOC
copy_payload "$work_dir/teen/TEENUPDT.DOC" doc/TEENUPDT.DOC
copy_payload "$work_dir/teen/README.DOC" doc/TEENREAD.DOC
copy_payload "$work_dir/teen/TLOG.DOC" doc/TLOG.DOC
copy_payload "$work_dir/vbuff/VBUFF.DOC" doc/VBUFF.DOC
copy_payload "$work_dir/vbuff/VBUFF.LOG" doc/VBUFF.LOG
copy_payload "$work_dir/fatmap/FATMAP.MAN" doc/FATMAP.MAN
copy_payload "$work_dir/fatmap/README.DOC" doc/FATMREAD.DOC
copy_payload "$work_dir/forg/FORG.DOC" doc/FORG.DOC
copy_payload "$work_dir/forg/README.DOC" doc/FORGREAD.DOC
copy_payload "$work_dir/ramdisk/RAMDISK.DOC" doc/RAMDISK.DOC
copy_payload "$work_dir/ramdisk/README" doc/RAMREAD.ME
copy_payload "$work_dir/diet/DIET144.DOC" doc/DIET144.DOC
copy_payload "$work_dir/diet/README.DOC" doc/DIETREAD.DOC
copy_payload "$work_dir/schd/SCHD.DOC" doc/SCHD.DOC
copy_payload "$work_dir/schd/SCHD.LOG" doc/SCHD.LOG
copy_payload "$work_dir/schd/SCHD.TXT" doc/SCHD.TXT
copy_payload "$work_dir/rdbms/RDBMS.DOC" doc/RDBMS.DOC
copy_payload "$work_dir/rdpcm/RDPCM.DOC" doc/RDPCM.DOC
copy_payload "$work_dir/reset/RESET.DOC" doc/RESET.DOC
copy_payload "$stage_dir/TSCLVA.DOC" doc/TSCLVA.DOC
copy_payload "$work_dir/bms/bms15020.doc" doc/BMS15020.DOC
copy_payload "$work_dir/bms/bms15020.hed" doc/BMS15020.HED
copy_payload "$work_dir/bms/bms15020.his" doc/BMS15020.HIS
copy_payload "$work_dir/pcp108/PCP108/PCPLUS.DOC" doc/PCPLUS.DOC
copy_payload "$work_dir/pcp108/PCP108/PCPLUS.TXT" doc/PCPLUS.TXT
copy_payload "$work_dir/pcp108/PCP108/SCSIVA/SCSI55.TXT" doc/SCSI55.TXT
copy_payload "$work_dir/x8map/X8MAP130.SMP" doc/X8MAP130.SMP
copy_payload "$work_dir/x8map/X8MAP130.TXT" doc/X8MAP130.TXT
copy_payload "$work_dir/emmva/EMMVA150.DOC" doc/EMMVA150.DOC
copy_payload "$work_dir/rdems/RDEMS152.MAN" doc/RDEMS152.MAN
copy_payload "$work_dir/scform/SCFORM.DOC" doc/SCFORM.DOC
copy_payload "$work_dir/scform/SCFORM.LOG" doc/SCFORM.LOG
copy_payload "$common_stage_dir/DOC/ISHARC.DOC" doc/ISHARC.DOC
copy_payload "$common_stage_dir/DOC/ISHVA.DOC" doc/ISHVA.DOC
copy_payload "$cache_dir/tenim3.doc" doc/TENIM3.DOC
copy_payload "$work_dir/tenim3/NEC_MAIL.DOC" doc/TENMAIL.DOC
copy_payload "$cache_dir/tfd12.doc" doc/TFD12.DOC
copy_payload "$work_dir/tfd12/TFD12.MAN" doc/TFD12.MAN
copy_payload "$work_dir/jfppat/JFPPAT.DOC" doc/JFPPAT.DOC
copy_payload "$work_dir/2hcdrv/2HCDRV.DOC" doc/2HCDRV.DOC
copy_payload "$work_dir/2hcdrv/FDFORM.DOC" doc/FDFORM.DOC
copy_payload "$cache_dir/v480src.lzh" archive/V480SRC.LZH
copy_payload "$cache_dir/jfppat.zip" archive/JFPPAT.ZIP
copy_payload "$cache_dir/2hcdrv.zip" archive/2HCDRV.ZIP
copy_payload "$cache_dir/fdfrmsrc.lzh" archive/FDFRMSRC.LZH
copy_payload "$common_stage_dir/ARCHIVE/ISHARC.COM" archive/ISHARC.COM
copy_payload "$cache_dir/tenim3.com" archive/TENIM3.COM
copy_payload "$cache_dir/tfd12.lzh" archive/TFD12.LZH

printf '%s\r\n' \
	'SQEMM98 MAX v0.8 for PC-88VA' \
	'' \
	'SQEMM98 is built from pinned SQEMM 0.8 source with Open Watcom.' \
	'It drives the vaeg PC-88VA EMS board and reports initialization' \
	'messages through the PC-Engine Text BIOS INT 83H/AH=02H service.' \
	> "$payload_dir/doc/SQEMM98.TXT"

printf '%s\n' 'Compressing BIN executables with DIET 1.44'
diet_manifest=$work_dir/diet-before.tsv
: > "$diet_manifest"
for executable in "$payload_dir/bin"/*.EXE "$payload_dir/bin"/*.COM; do
	[[ -f ${executable} ]] || continue
	executable_size=$(wc -c < "$executable")
	executable_size=${executable_size//[[:space:]]/}
	printf '%s\t%s\n' "${executable##*/}" "$executable_size" >> "$diet_manifest"
done

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox -conf "$dosbox_conf" -exit \
	-c "mount c $payload_dir/bin" \
	-c 'c:' \
	-c 'diet -b *.exe > dietexe.log' \
	-c 'exit' >/dev/null 2>&1

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox -conf "$dosbox_conf" -exit \
	-c "mount c $payload_dir/bin" \
	-c 'c:' \
	-c 'diet -b -xc *.com > dietcom.log' \
	-c 'exit' >/dev/null 2>&1

[[ -f $payload_dir/bin/DIETEXE.LOG ]] ||
	die 'DIET did not produce the EXE compression log'
[[ -f $payload_dir/bin/DIETCOM.LOG ]] ||
	die 'DIET did not produce the COM compression log'

diet_processed=0
diet_saved=0
while IFS=$'\t' read -r executable_name before_size; do
	executable_path=$payload_dir/bin/$executable_name
	case $executable_name in
	*.EXE) diet_log=$payload_dir/bin/DIETEXE.LOG ;;
	*.COM) diet_log=$payload_dir/bin/DIETCOM.LOG ;;
	*) die "internal DIET manifest error: $executable_name" ;;
	esac
	grep -Fq "Compress '$executable_name'" "$diet_log" ||
		die "DIET did not process $executable_name"
	after_size=$(wc -c < "$executable_path")
	after_size=${after_size//[[:space:]]/}
	((after_size <= before_size)) ||
		die "DIET increased the size of $executable_name"
	((diet_processed += 1))
	diet_saved=$((diet_saved + before_size - after_size))
done < "$diet_manifest"
((diet_processed > 0)) || die 'no BIN executables were available for DIET'

rm -- "$payload_dir/bin/DIETEXE.LOG" "$payload_dir/bin/DIETCOM.LOG"
printf 'DIET processed %u executables and saved %u bytes\n' \
	"$diet_processed" "$diet_saved"

printf '%s\r\n' \
	'FILES   = 20' \
	'BUFFERS = 30' \
	'DEVICE = A:\SYS\EMMVA01.SYS' \
	'DEVICE = A:\SYS\SQEMM98.SYS' \
	'DEVICE = A:\SYS\EMMVA02.SYS' \
	'DEVICE = A:\SYS\PCPLUS.SYS' \
	'DEVICE = A:\SYS\BMSDRVA.SYS /P' \
	'DEVICE = A:\SYS\SCHD.SYS -I0' \
	'DEVICE = A:\SYS\HOSTFAT.SYS' \
	'DEVICE = A:\SYS\PCEPAT.SYS' \
	'DEVICE = A:\SYS\JFPPAT.SYS' \
	'DEVICE = A:\SYS\RESET.SYS' \
	'DEVICE = A:\SYS\TSCLVA.SYS' \
	'DEVICE = A:\SYS\MSE352B.COM /A /B' \
	'DEVICE = A:\SYS\RDBMS.SYS -P1D0 -S2' \
	'DEVICE = A:\SYS\RDEMS.SYS -P128 -A' \
	'DEVICE = A:\SYS\RDPCM.SYS' >"$payload_dir/root/CONFIG.SYS"

grep -Fqx $'DEVICE = A:\\SYS\\MSE352B.COM /A /B\r' "$payload_dir/root/CONFIG.SYS" ||
	die 'generated CONFIG.SYS is missing the MSE BMS /A /B switches'
grep -Fqx $'DEVICE = A:\\SYS\\RDBMS.SYS -P1D0 -S2\r' "$payload_dir/root/CONFIG.SYS" ||
	die 'generated CONFIG.SYS is missing the RDBMS bank-start switch'
grep -Fqx $'DEVICE = A:\\SYS\\RDEMS.SYS -P128 -A\r' "$payload_dir/root/CONFIG.SYS" ||
	die 'generated CONFIG.SYS is missing the 2MB RDEMS setting'

printf '%s\r\n' \
	'PATH A:\BIN' \
	'SET TEEN=A:\BIN\TEEN.DEF' \
	'SET TMP=A:\TMP' \
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
chmod 0644 "$output_tmp"
output_moved=false
for move_attempt in 1 2 3; do
	if mv -- "$output_tmp" "$output_d88"; then
		output_moved=true
		break
	fi
	sleep 1
done
[[ ${output_moved} == true ]] || die 'could not finalize output after three attempts'
output_tmp=

printf 'Created bootable development disk: %s\n' "$output_d88"

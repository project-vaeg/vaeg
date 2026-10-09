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
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -euo pipefail

program_name=${0##*/}
output_dir=
manifest_file=
profile=sasi
ish_archive=
ish_doc=
infozip_unzip_archive=
infozip_zip_archive=
emacs_archive=
cpmva_archive=
tdc_archive=
bench_archive=
unix_tools_archive=
two_hc_source_archive=
two_hc_driver_archive=
pcepat_source_archive=
tsclv_source_archive=
s88valsi_archive=
s88valsi_doc=
s88va250_archive=
s88va250_doc=
stest_source_archive=
work_dir=

usage() {
	cat <<EOF
Usage: $program_name --output DIR --profile fdd|sasi \
       [--manifest FILE] \
       --ish-archive FILE --ish-doc FILE \
       --infozip-unzip-archive FILE --infozip-zip-archive FILE \
	       --emacs-archive FILE --cpmva-archive FILE \
	       --tdc-archive FILE --bench-archive FILE \
	       --unix-tools-archive FILE \
	       [--2hc-source-archive FILE --2hc-driver-archive FILE] \
	       [--pcepat-source-archive FILE --tsclv-source-archive FILE] \
		       [--s88valsi-archive FILE --s88valsi-doc FILE] \
		       [--s88va250-archive FILE --s88va250-doc FILE]
		       [--stest-source-archive FILE]

Create the normalized development-tool tree consumed by both the FDD and
SASI injectors.  The output contains BIN, DOC, ARCHIVE, and UNIX subtrees.
The FDD profile stages the compact ISH archive tools used by the established
development floppy.  The SASI profile also stages the larger Info-ZIP,
EMACSVA, CPMVA, TDC, BENCH, and UNIX-like collections below their respective
directories.  The caller chooses which subtrees fit its target medium;
extraction and destination naming are shared.  The additional source and
library package archives are SASI-only and are retained without expansion;
they are rejected for the compact FDD profile.
EOF
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

cleanup() {
	if [[ -n ${work_dir} && ${work_dir} == "${TMPDIR:-/tmp}"/vaeg-stage-tools.* && -d ${work_dir} ]]; then
		rm -rf -- "$work_dir"
	fi
}

trap cleanup EXIT HUP INT TERM

while (($#)); do
	case $1 in
	--profile)
		(($# >= 2)) || die '--profile requires fdd or sasi'
		profile=$2
		[[ $profile == fdd || $profile == sasi ]] || die '--profile must be fdd or sasi'
		shift 2
		;;
	--output)
		(($# >= 2)) || die '--output requires a path'
		output_dir=$2
		shift 2
		;;
	--manifest)
		(($# >= 2)) || die '--manifest requires a path'
		manifest_file=$2
		shift 2
		;;
	--ish-archive)
		(($# >= 2)) || die '--ish-archive requires a path'
		ish_archive=$2
		shift 2
		;;
	--ish-doc)
		(($# >= 2)) || die '--ish-doc requires a path'
		ish_doc=$2
		shift 2
		;;
	--infozip-unzip-archive)
		(($# >= 2)) || die '--infozip-unzip-archive requires a path'
		infozip_unzip_archive=$2
		shift 2
		;;
	--infozip-zip-archive)
		(($# >= 2)) || die '--infozip-zip-archive requires a path'
		infozip_zip_archive=$2
		shift 2
		;;
	--emacs-archive)
		(($# >= 2)) || die '--emacs-archive requires a path'
		emacs_archive=$2
		shift 2
		;;
	--cpmva-archive)
		(($# >= 2)) || die '--cpmva-archive requires a path'
		cpmva_archive=$2
		shift 2
		;;
	--tdc-archive)
		(($# >= 2)) || die '--tdc-archive requires a path'
		tdc_archive=$2
		shift 2
		;;
	--bench-archive)
		(($# >= 2)) || die '--bench-archive requires a path'
		bench_archive=$2
		shift 2
		;;
	--unix-tools-archive)
		(($# >= 2)) || die '--unix-tools-archive requires a path'
		unix_tools_archive=$2
		shift 2
		;;
	--2hc-source-archive)
		(($# >= 2)) || die '--2hc-source-archive requires a path'
		two_hc_source_archive=$2
		shift 2
		;;
	--2hc-driver-archive)
		(($# >= 2)) || die '--2hc-driver-archive requires a path'
		two_hc_driver_archive=$2
		shift 2
		;;
	--pcepat-source-archive)
		(($# >= 2)) || die '--pcepat-source-archive requires a path'
		pcepat_source_archive=$2
		shift 2
		;;
	--tsclv-source-archive)
		(($# >= 2)) || die '--tsclv-source-archive requires a path'
		tsclv_source_archive=$2
		shift 2
		;;
	--s88valsi-archive)
		(($# >= 2)) || die '--s88valsi-archive requires a path'
		s88valsi_archive=$2
		shift 2
		;;
	--s88valsi-doc)
		(($# >= 2)) || die '--s88valsi-doc requires a path'
		s88valsi_doc=$2
		shift 2
		;;
	--s88va250-archive)
		(($# >= 2)) || die '--s88va250-archive requires a path'
		s88va250_archive=$2
		shift 2
		;;
	--s88va250-doc)
		(($# >= 2)) || die '--s88va250-doc requires a path'
		s88va250_doc=$2
		shift 2
		;;
	--stest-source-archive)
		(($# >= 2)) || die '--stest-source-archive requires a path'
		stest_source_archive=$2
		shift 2
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		die "unknown argument: $1"
		;;
	esac
done

[[ -n ${output_dir} ]] || die '--output is required'
[[ ! -e ${output_dir} ]] || die 'output directory already exists; refusing to overwrite it'
if [[ -n ${manifest_file} ]]; then
	[[ ! -e ${manifest_file} ]] || die 'manifest already exists; refusing to overwrite it'
fi

for path in "$ish_archive" "$ish_doc"; do
	[[ -n ${path} && -f ${path} && -r ${path} ]] ||
		die "package is not readable: ${path:-<missing>}"
done
if [[ $profile == sasi ]]; then
	for path in "$infozip_unzip_archive" "$infozip_zip_archive" \
		"$emacs_archive" "$cpmva_archive" "$tdc_archive" "$bench_archive" \
		"$unix_tools_archive"; do
		[[ -n ${path} && -f ${path} && -r ${path} ]] ||
		die "package is not readable: ${path:-<missing>}"
	done
	for path in "$two_hc_source_archive" "$two_hc_driver_archive" \
		"$pcepat_source_archive" "$tsclv_source_archive" \
		"$s88valsi_archive" "$s88valsi_doc" \
		"$s88va250_archive" "$s88va250_doc" \
		"$stest_source_archive"; do
		[[ -n ${path} && -f ${path} && -r ${path} ]] ||
			die "package is not readable: ${path:-<missing>}"
	done
else
	for path in "$two_hc_source_archive" "$two_hc_driver_archive" \
		"$pcepat_source_archive" "$tsclv_source_archive" \
		"$s88valsi_archive" "$s88valsi_doc" \
		"$s88va250_archive" "$s88va250_doc" \
		"$stest_source_archive"; do
		[[ -z ${path} ]] || die 'new source/library packages are SASI-only'
	done
fi

for required_command in lha tar unzip; do
	command -v "$required_command" >/dev/null 2>&1 ||
		die "required host command is missing: $required_command"
done

	mkdir -p -- "$output_dir/BIN" "$output_dir/DOC" "$output_dir/ARCHIVE" \
	"$output_dir/UNIX/BIN" "$output_dir/UNIX/DOC" "$output_dir/UNIX/MAN"
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-stage-tools.XXXXXX")

extract_lha() {
	local archive=$1
	local destination=$2
	mkdir -p -- "$destination"
	lha xfw="$destination" "$archive" >/dev/null
}

extract_zip() {
	local archive=$1
	local destination=$2
	shift 2
	mkdir -p -- "$destination"
	unzip -q "$archive" "$@" -d "$destination"
}

copy_required() {
	local source=$1
	local destination=$2
	[[ -f ${source} ]] || die "package member is missing: $source"
	cp -p -- "$source" "$destination"
}

ish_tree=$work_dir/ish
extract_lha "$ish_archive" "$ish_tree"
copy_required "$ish_archive" "$output_dir/ARCHIVE/ISHARC.COM"
copy_required "$ish_doc" "$output_dir/DOC/ISHARC.DOC"
copy_required "$ish_tree/ISHVA.COM" "$output_dir/BIN/ISHVA.COM"
copy_required "$ish_tree/PKPAK.EXE" "$output_dir/BIN/PKPAK.EXE"
copy_required "$ish_tree/PKUNPAK.EXE" "$output_dir/BIN/PKUNPAK.EXE"
copy_required "$ish_tree/README.DOC" "$output_dir/DOC/ISHVA.DOC"

if [[ $profile == sasi ]]; then
	unzip_tree=$work_dir/unzip
	zip_tree=$work_dir/zip
	extract_zip "$infozip_unzip_archive" "$unzip_tree" \
		unzip.exe unzip.doc COPYING README.DOS
	extract_zip "$infozip_zip_archive" "$zip_tree" zip.exe MANUAL README
	copy_required "$infozip_unzip_archive" "$output_dir/ARCHIVE/UNZ532X3.EXE"
	copy_required "$infozip_zip_archive" "$output_dir/ARCHIVE/ZIP22X.ZIP"
	copy_required "$unzip_tree/unzip.exe" "$output_dir/BIN/UNZIP.EXE"
	copy_required "$zip_tree/zip.exe" "$output_dir/BIN/ZIP.EXE"
	copy_required "$unzip_tree/COPYING" "$output_dir/DOC/COPYING"
	copy_required "$unzip_tree/unzip.doc" "$output_dir/DOC/UNZIP.DOC"
	copy_required "$unzip_tree/README.DOS" "$output_dir/DOC/UNZDOS.TXT"
	copy_required "$zip_tree/MANUAL" "$output_dir/DOC/ZIP.DOC"
	copy_required "$zip_tree/README" "$output_dir/DOC/ZIPREAD.TXT"
fi

if [[ $profile == sasi ]]; then
	emacs_tree=$work_dir/emacs
	cpmva_tree=$work_dir/cpmva
	tdc_tree=$work_dir/tdc
	bench_tree=$work_dir/bench
	extract_lha "$emacs_archive" "$emacs_tree"
	extract_lha "$cpmva_archive" "$cpmva_tree"
	extract_lha "$tdc_archive" "$tdc_tree"
	extract_lha "$bench_archive" "$bench_tree"
	copy_required "$emacs_archive" "$output_dir/ARCHIVE/EMACSVA.LZH"
	copy_required "$cpmva_archive" "$output_dir/ARCHIVE/CPMVA.LZH"
	copy_required "$tdc_archive" "$output_dir/ARCHIVE/TDC10.LZH"
	copy_required "$bench_archive" "$output_dir/ARCHIVE/BENCH003.LZH"
	copy_required "$emacs_tree/EMACS.EXE" "$output_dir/BIN/EMACS.EXE"
	for member in EMACS.HLP EMACS.RC EMACSJ.HLP EMACSVA.DOC README.1ST REFMAN.TXT; do
		case $member in
		README.1ST) destination=EMACS1ST.1ST ;;
		REFMAN.TXT) destination=EMACSREF.TXT ;;
		*) destination=$member ;;
		esac
		copy_required "$emacs_tree/$member" "$output_dir/DOC/$destination"
	done
	for member in CPMBIOS.COM CPMVA.EXE DO.COM EXIT.COM FCONV.COM RDCPM.EXE; do
		copy_required "$cpmva_tree/$member" "$output_dir/BIN/$member"
	done
	copy_required "$cpmva_tree/CPMVA.DOC" "$output_dir/DOC/CPMVA.DOC"
	copy_required "$cpmva_tree/README.DOC" "$output_dir/DOC/CPMREAD.DOC"
	copy_required "$tdc_tree/TDC.COM" "$output_dir/BIN/TDC.COM"
	copy_required "$tdc_tree/TDC.DOC" "$output_dir/DOC/TDC.DOC"
	copy_required "$bench_tree/BENCH.EXE" "$output_dir/BIN/BENCH.EXE"
	copy_required "$bench_tree/BENCH.C" "$output_dir/DOC/BENCH.C"
	copy_required "$bench_tree/BENCH.DOC" "$output_dir/DOC/BENCH.DOC"
	copy_required "$bench_tree/README.1ST" "$output_dir/DOC/BENCHRD.1ST"
fi

if [[ $profile == sasi ]]; then
	# These PC-88VA Softlib packages are installed only on the spacious SASI
	# image.  Source/library distributions are retained verbatim under
	# ARCHIVE; only the runnable 2HCDRV package is expanded.
	two_hc_driver_tree=$work_dir/2hc-driver
	extract_zip "$two_hc_driver_archive" "$two_hc_driver_tree"

	copy_required "$two_hc_source_archive" "$output_dir/ARCHIVE/2HCDRSRC.LZH"
	copy_required "$two_hc_driver_archive" "$output_dir/ARCHIVE/2HCDRV.ZIP"
	copy_required "$pcepat_source_archive" "$output_dir/ARCHIVE/PCPATSRC.ZIP"
	copy_required "$tsclv_source_archive" "$output_dir/ARCHIVE/TSCLVSRC.LZH"
	copy_required "$s88valsi_archive" "$output_dir/ARCHIVE/S88VALSI.LZH"
	copy_required "$s88valsi_doc" "$output_dir/DOC/S88VALSI.DOC"
	copy_required "$s88va250_archive" "$output_dir/ARCHIVE/S88VA250.LZH"
	copy_required "$s88va250_doc" "$output_dir/DOC/S88VA250.DOC"
	copy_required "$stest_source_archive" "$output_dir/ARCHIVE/ST115SRC.LZH"

	copy_required "$two_hc_driver_tree/2HCDRV.COM" "$output_dir/BIN/2HCDRV.COM"
	copy_required "$two_hc_driver_tree/FDFORM.COM" "$output_dir/BIN/FDFORM.COM"
	copy_required "$two_hc_driver_tree/2HCDRV.DOC" "$output_dir/DOC/2HCDRV.DOC"
	copy_required "$two_hc_driver_tree/FDFORM.DOC" "$output_dir/DOC/FDFORM.DOC"
fi

if [[ $profile == sasi ]]; then

unix_tree=$work_dir/unix
mkdir -p -- "$unix_tree"
tar -xzf "$unix_tools_archive" -C "$unix_tree"
unix_root=$unix_tree/uxtl412h
[[ -d ${unix_root}/bin && -d ${unix_root}/man ]] ||
	die 'UNIX-like tools archive has an unexpected layout'
copy_required "$unix_tools_archive" "$output_dir/ARCHIVE/UXTL412H.TGZ"
while IFS= read -r -d '' member; do
	name=${member##*/}
	cp -p -- "$member" "$output_dir/UNIX/BIN/${name^^}"
done < <(find "$unix_root/bin" -type f -print0 | sort -z)
if [[ -f ${unix_root}/Readme.1st ]]; then
	cp -p -- "$unix_root/Readme.1st" "$output_dir/UNIX/README.1ST"
fi
while IFS= read -r -d '' member; do
	relative=${member#"$unix_root/"}
	case $relative in
	man/*)
		destination=$output_dir/UNIX/MAN/${relative#man/}
		;;
	doc/*)
		destination=$output_dir/UNIX/DOC/${relative#doc/}
		;;
	*)
		continue
		;;
	esac
	mkdir -p -- "${destination%/*}"
	cp -p -- "$member" "$destination"
done < <(find "$unix_root/man" "$unix_root/doc" -type f -print0 | sort -z)
fi

write_manifest() {
	[[ -n ${manifest_file} ]] || return 0
	local manifest_parent
	manifest_parent=${manifest_file%/*}
	if [[ $manifest_parent == "$manifest_file" ]]; then
		manifest_parent=.
	fi
	mkdir -p -- "$manifest_parent"
	{
		printf '%s\n' '# vaeg development-tool staging manifest v1'
		printf '%s\n' '# profile<TAB>relative_path<TAB>sha256<TAB>bytes'
		while IFS= read -r -d '' member; do
			relative=${member#"$output_dir/"}
			digest=$(sha256sum -- "$member")
			digest=${digest%% *}
			size=$(wc -c < "$member")
			size=${size//[[:space:]]/}
			printf '%s\t%s\t%s\t%s\n' "$profile" "$relative" "$digest" "$size"
		done < <(find "$output_dir" -type f -print0 |
			sort -z -f)
	} > "$manifest_file"
}

write_manifest
printf 'Staged common development tools in %s\n' "$output_dir"

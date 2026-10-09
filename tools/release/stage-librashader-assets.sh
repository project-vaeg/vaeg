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
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -euo pipefail

program_name=${0##*/}
script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
output=
platform=
runtime=

usage() {
	printf '%s\n' \
		"Usage: $program_name --output DIR --platform PLATFORM [--runtime FILE]" \
		'' \
		'PLATFORM is one of: linux, macos, windows.' \
		'--runtime is optional and must be the exact platform runtime basename.'
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

while (($#)); do
	case $1 in
	--output)
		(($# >= 2)) || die '--output requires a path'
		output=$2
		shift 2
		;;
	--platform)
		(($# >= 2)) || die '--platform requires linux, macos, or windows'
		platform=$2
		shift 2
		;;
	--runtime)
		(($# >= 2)) || die '--runtime requires a file'
		runtime=$2
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
case ${platform} in
linux)
	runtime_name=librashader.so
	;;
macos)
	runtime_name=librashader.dylib
	;;
windows)
	runtime_name=librashader.dll
	;;
*)
	die "unsupported platform: ${platform}"
	;;
esac

[[ -d ${output} ]] || die "output directory does not exist: ${output}"
for required_command in install; do
	command -v "$required_command" >/dev/null 2>&1 ||
		die "required command is missing: $required_command"
done

hash_file() {
	local path=$1
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum -- "$path" | awk '{print $1}'
	else
		shasum -a 256 -- "$path" | awk '{print $1}'
	fi
}

copy_checked() {
	local source=$1
	local destination=$2
	local expected=$3
	local actual
	[[ -f ${source} ]] || die "required source is missing: ${source}"
	actual=$(hash_file "$source")
	[[ ${actual} == "${expected}" ]] ||
		die "source hash changed: ${source} (${actual})"
	install -m 644 "$source" "$destination"
}

crt_root=$repo_root/assets/shaders/crt
install -d "$output/assets/shaders/crt/shaders" \
	"$output/assets/shaders/crt/licenses" "$output/licenses"
copy_checked "$crt_root/vaeg_crt_default.slangp" \
	"$output/assets/shaders/crt/vaeg_crt_default.slangp" \
	6965dca041d248013a1b52f353da5e19ddc4fae9d0efa379c9eb344af5a5e31f
copy_checked "$crt_root/shaders/vaeg-screen-size.slang" \
	"$output/assets/shaders/crt/shaders/vaeg-screen-size.slang" \
	9a521d7ecf3ead998a5039d33d144903c5764793eba5415dc98ac4ae7ec5c361
copy_checked "$crt_root/shaders/crt-lottes-fast.slang" \
	"$output/assets/shaders/crt/shaders/crt-lottes-fast.slang" \
	576eddc662ac4f77909c0c14dbd5a16ac4164e50c67527fff634316f4441c482
copy_checked "$crt_root/shaders/vaeg-crt-aa.slang" \
	"$output/assets/shaders/crt/shaders/vaeg-crt-aa.slang" \
	7cef376e3f8d4c76112842b155cf642b8a32eb4b89921dd026ba55f07390b9ee
copy_checked "$crt_root/shaders/vaeg-scanline-aa.inc" \
	"$output/assets/shaders/crt/shaders/vaeg-scanline-aa.inc" \
	6461786e3d74abd56a4be789018224bd41ff834d0d9a40c3d4f67e2bf471c408
copy_checked "$crt_root/licenses/crt-default-license.txt" \
	"$output/assets/shaders/crt/licenses/crt-default-license.txt" \
	6b36a9fe4618402e929fb3403d4724d1b707934f2d1db8483fbf0ebfbccb26bc
copy_checked "$crt_root/licenses/crt-default-provenance.md" \
	"$output/assets/shaders/crt/licenses/crt-default-provenance.md" \
	6e536ecceac77f736de33aa4730c321d3ec8c0c4edf10ca12f32ed25e7f9aa77
copy_checked "$repo_root/external/librashader/LICENSE.md" \
	"$output/licenses/librashader-MPL-2.0.txt" \
	69c15395f33bc9ce8e1d8b6cef42b7e49cdec4c6f5233d4b9cfc4bfa335f97f9
copy_checked "$repo_root/external/librashader/include/README.md" \
	"$output/licenses/librashader-headers-MIT.txt" \
	f2b103e6d0dbff9ea3cebe848f3b10c099215231a3d5edc99fa1fa2b9bba13a3
copy_checked "$repo_root/docs/licenses/THIRD_PARTY_NOTICES.md" \
	"$output/licenses/THIRD_PARTY_NOTICES.md" \
	7e1e234b6e9ef077779bdb3e1486fcbb567903202fdf2f56e38ceacf1500282d

if [[ -n ${runtime} ]]; then
	[[ -f ${runtime} ]] || die "runtime file does not exist: ${runtime}"
	[[ ${runtime##*/} == ${runtime_name} ]] ||
		die "runtime basename must be ${runtime_name}"
	install -m 755 "$runtime" "$output/$runtime_name"
	printf '%s  %s\n' "$(hash_file "$runtime")" "$runtime_name" \
		> "$output/licenses/librashader-runtime.sha256"
	printf 'Staged optional librashader runtime: %s\n' "$runtime_name"
else
	printf 'No shared librashader runtime staged; static builds use the embedded API, dynamic builds fail closed.\n'
fi

printf 'Staged librashader CRT assets for %s in %s\n' "$platform" "$output"
install -m 644 "$repo_root/docs/modernization/native-crt-user-guide.md" "$output/README-native-crt.md"
if [[ ${platform} == windows ]]; then
	install -m 644 "$script_dir/start-native-crt.cmd" "$output/start-native-crt.cmd"
	install -m 644 "$repo_root/external/imgui/LICENSE.txt" "$output/licenses/imgui-MIT.txt"
fi

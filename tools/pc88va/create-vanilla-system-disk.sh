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

usage() {
	printf '%s\n' \
		"Usage: $program_name --source SOURCE.d88 --output OUTPUT.d88" \
		'' \
		'Create a FORMAT /S-like PC-Engine 1.05 or 1.1 boot disk.' \
		'Only the IPL and four required PC-Engine system files are retained.'
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

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
[[ -f ${source_d88} && -r ${source_d88} ]] || die 'source D88 is not readable'
[[ ! -e ${output_d88} ]] || die 'output already exists; refusing to overwrite it'
command -v python3 >/dev/null 2>&1 || die 'required host command is missing: python3'

python3 "$script_dir/pcengine_disk.py" vanilla \
	--source "$source_d88" \
	--output "$output_d88"

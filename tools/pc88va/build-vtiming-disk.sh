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

# Build a PC-Engine 1.1 test disk for the M104 display-timing experiments:
# a vanilla copy of the given PC-Engine 1.1 system disk with V480PAT.COM and
# TSPMODE.COM, TSPFILL.COM, G160.COM and G256.COM (tools/pc88va/vtiming/, assembled here) and, optionally, a
# separately obtained VIEW480.COM. The source disk is private media and the
# output must stay outside Git.

set -euo pipefail

program_name=${0##*/}
script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source_d88=
output_d88=
view480=

usage() {
	cat <<USAGE
usage: $program_name --source PC-ENGINE-1.1.d88 --output NEW.d88 [--view480 VIEW480.COM]
USAGE
}

while [[ $# -gt 0 ]]; do
	case $1 in
	--source) source_d88=${2:?}; shift 2 ;;
	--output) output_d88=${2:?}; shift 2 ;;
	--view480) view480=${2:?}; shift 2 ;;
	-h | --help) usage; exit 0 ;;
	*) usage >&2; exit 2 ;;
	esac
done
if [[ -z $source_d88 || -z $output_d88 ]]; then
	usage >&2
	exit 2
fi
if [[ -e $output_d88 ]]; then
	echo "$program_name: output exists: $output_d88" >&2
	exit 1
fi
command -v nasm >/dev/null || { echo "$program_name: nasm not found" >&2; exit 1; }

work_dir=$(mktemp -d)
trap 'rm -rf -- "$work_dir"' EXIT
mkdir -p "$work_dir/payload/root"
nasm -f bin -o "$work_dir/payload/root/V480PAT.COM" "$script_dir/vtiming/v480pat.asm"
nasm -f bin -o "$work_dir/payload/root/TSPMODE.COM" "$script_dir/vtiming/tspmode.asm"
nasm -f bin -o "$work_dir/payload/root/TSPFILL.COM" "$script_dir/vtiming/tspfill.asm"
nasm -f bin -o "$work_dir/payload/root/G160.COM" "$script_dir/vtiming/g160.asm"
nasm -f bin -o "$work_dir/payload/root/G256.COM" "$script_dir/vtiming/g256.asm"
if [[ -n $view480 ]]; then
	cp -- "$view480" "$work_dir/payload/root/VIEW480.COM"
fi
python3 "$script_dir/pcengine_disk.py" vanilla --source "$source_d88" --output "$work_dir/disk.d88"
python3 "$script_dir/pcengine_disk.py" install --image "$work_dir/disk.d88" --payload "$work_dir/payload"
mv -- "$work_dir/disk.d88" "$output_d88"
python3 "$script_dir/pcengine_disk.py" list --image "$output_d88"

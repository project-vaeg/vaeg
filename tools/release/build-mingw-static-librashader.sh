#!/usr/bin/env bash
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_root="${VAEG_MINGW_STATIC_DIR:-${repo_root}/build/mingw-static}"
source_dir="${build_root}/librashader-git"
audit_dir="${build_root}/windows-audit"
source_pin="87e8a97b50516d997defeaa168173dcd185d4022"
target="x86_64-pc-windows-gnu"

if [[ "$(rustc --version)" != "rustc 1.88.0 "* ]]; then
	echo "librashader must be built with Rust 1.88.0" >&2
	exit 1
fi

mkdir -p "${build_root}"
if [[ ! -d "${source_dir}/.git" ]]; then
	git clone https://github.com/SnowflakePowered/librashader.git "${source_dir}"
fi
git -C "${source_dir}" checkout --detach "${source_pin}"
actual_pin="$(git -C "${source_dir}" rev-parse HEAD)"
if [[ "${actual_pin}" != "${source_pin}" ]]; then
	echo "librashader source pin mismatch: ${actual_pin}" >&2
	exit 1
fi
if [[ -n "$(git -C "${source_dir}" status --porcelain --untracked-files=no)" ]]; then
	echo "librashader source is modified" >&2
	exit 1
fi

export CC_x86_64_pc_windows_gnu="${CC_x86_64_pc_windows_gnu:-x86_64-w64-mingw32-gcc}"
export CXX_x86_64_pc_windows_gnu="${CXX_x86_64_pc_windows_gnu:-x86_64-w64-mingw32-g++}"
export AR_x86_64_pc_windows_gnu="${AR_x86_64_pc_windows_gnu:-ar}"
export RANLIB_x86_64_pc_windows_gnu="${RANLIB_x86_64_pc_windows_gnu:-ranlib}"
export CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER="${CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER:-${CC_x86_64_pc_windows_gnu}}"

mkdir -p "${audit_dir}"
python3 "${repo_root}/tools/release/build-static-librashader.py" \
	--source "${source_dir}" \
	--target "${target}" \
	--output "${audit_dir}"

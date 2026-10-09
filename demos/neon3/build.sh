#!/bin/sh
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
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

set -eu

if [ "$#" -ne 2 ]; then
    printf 'usage: %s PROFILE(200|400) OUTPUT.BIN\n' "$0" >&2
    exit 2
fi

profile=$1
output=$2
case "$profile" in
    200) define=NEON_PROFILE_200 ;;
    400) define=NEON_PROFILE_400 ;;
    *) printf 'error: profile must be 200 or 400\n' >&2; exit 2 ;;
esac

extra_define=
if [ "${NEON_BUILD_FRAME_LIMIT:-}" ]; then
    case "${NEON_BUILD_FRAME_LIMIT}" in
        *[!0-9]*|'') printf 'error: NEON_BUILD_FRAME_LIMIT must be decimal\n' >&2; exit 2 ;;
        *) extra_define="-d"; extra_define="$extra_define NEON_FRAME_LIMIT=${NEON_BUILD_FRAME_LIMIT}" ;;
    esac
fi

if [ "${NEON_BUILD_SGP_LIST_CAPACITY:-}" ]; then
    case "${NEON_BUILD_SGP_LIST_CAPACITY}" in
        *[!0-9]*|'') printf 'error: NEON_BUILD_SGP_LIST_CAPACITY must be decimal\n' >&2; exit 2 ;;
        *) extra_define="$extra_define -d NEON_SGP_LIST_CAPACITY=${NEON_BUILD_SGP_LIST_CAPACITY}" ;;
    esac
fi

if [ "${NEON_BUILD_SGP_EXTERNAL_LIST:-}" ]; then
    case "${NEON_BUILD_SGP_EXTERNAL_LIST}" in
        1|yes|true) extra_define="$extra_define -d NEON_SGP_EXTERNAL_LIST" ;;
        *) printf 'error: NEON_BUILD_SGP_EXTERNAL_LIST must be 1, yes, or true\n' >&2; exit 2 ;;
    esac
fi

if [ "${NEON_BUILD_LOOP:-}" ]; then
    case "${NEON_BUILD_LOOP}" in
        0|1) extra_define="$extra_define -d NEON_LOOP=${NEON_BUILD_LOOP}" ;;
        *) printf 'error: NEON_BUILD_LOOP must be 0 or 1\n' >&2; exit 2 ;;
    esac
fi

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir=$(CDPATH= cd -- "$(dirname -- "$output")" && pwd)
output_path=$output_dir/$(basename -- "$output")
assembler=${NASM:-nasm}

command -v "$assembler" >/dev/null 2>&1 || {
    printf 'error: NASM is unavailable: %s\n' "$assembler" >&2
    exit 127
}

set -- -f bin -O2 -d "$define"
if [ "$extra_define" ]; then
    # shellcheck disable=SC2086
    set -- "$@" $extra_define
fi
set -- "$@" \
    -I "$script_dir/src/" \
    -I "$script_dir/../neon3_1_5/98/" \
    "$script_dir/src/neon_counter.asm" -o "$output_path"

"$assembler" "$@"

printf '%s\n' "$output_path"

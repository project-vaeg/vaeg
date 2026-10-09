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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

if [ "$#" -ne 4 ]; then
    printf 'usage: VAEG_ZUNDAMON_MODEL=va2 %s SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY\n' "$0" >&2
    exit 2
fi

source_image=$1
vaeg=$2
rom_directory=$3
output_directory=$4
model=${VAEG_ZUNDAMON_MODEL:-va2}
initial_page=${VAEG_ZUNDAMON_INITIAL_PAGE:-a}
divisor=${VAEG_ZUNDAMON_DIVISOR:-1}
revolutions=${VAEG_ZUNDAMON_REVOLUTIONS:-1}
scenario=${VAEG_ZUNDAMON_SCENARIO:-static}
runtime_count=${VAEG_ZUNDAMON_COUNT:-4}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

[ -f "$source_image" ] || { printf 'error: source D88 does not exist\n' >&2; exit 1; }
[ -f "$vaeg" ] && [ -x "$vaeg" ] || { printf 'error: VAEG executable is unavailable\n' >&2; exit 1; }
[ -d "$rom_directory" ] || { printf 'error: ROM directory does not exist\n' >&2; exit 1; }
[ ! -e "$output_directory" ] || { printf 'error: refusing to overwrite output directory\n' >&2; exit 1; }
case "$model" in va2) ;; *) printf 'error: M98x candidate requires VA2 mode\n' >&2; exit 2 ;; esac
case "$runtime_count" in 1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16) ;; *) printf 'error: VAEG_ZUNDAMON_COUNT must be 1 through 16\n' >&2; exit 2 ;; esac
case "$divisor" in 1|2|3|4|5|6|7|8) ;; *) printf 'error: VAEG_ZUNDAMON_DIVISOR must be 1 through 8\n' >&2; exit 2 ;; esac
case "$revolutions" in 1|2) ;; *) printf 'error: VAEG_ZUNDAMON_REVOLUTIONS must be 1 or 2\n' >&2; exit 2 ;; esac
case "$scenario" in static|ladder|pause|missed) ;; *) printf 'error: unknown M98x scenario\n' >&2; exit 2 ;; esac
case "$initial_page" in a|b) ;; *) printf 'error: VAEG_ZUNDAMON_INITIAL_PAGE must be a or b\n' >&2; exit 2 ;; esac

mkdir -p "$output_directory/payload/root" "$output_directory/out"
python3 "$script_dir/tools/build_zundamon_orbit_pipeline.py" \
    --fixture-output "$output_directory/public-atlas" >/dev/null
cp "$output_directory/public-atlas/zundorb.bin" \
   "$output_directory/payload/root/ZUNDORB.BIN"
M98X_RUNTIME_MODE=1 "$script_dir/256/build.sh" \
    "$output_directory/payload/root/ZUNDORB.COM" \
    "$output_directory/ZUNDORB.LST"
M98X_RUNTIME_MODE=1 "$script_dir/build-local-d88.sh" \
    "$source_image" "$output_directory/public-atlas/zundorb.bin" \
    "$output_directory/zundamon-orbit-m98x-pristine.d88"
cp "$output_directory/zundamon-orbit-m98x-pristine.d88" \
   "$output_directory/zundamon-orbit-m98x.d88"
python3 "$script_dir/tools/generate_zundamon_orbit_multi_debug.py" \
    --active-count "$runtime_count" --runtime-count "$runtime_count" \
    --initial-page "$initial_page" --divisor "$divisor" \
    --revolutions "$revolutions" --scenario "$scenario" \
    --milestone m98x --output "$output_directory/zundamon-orbit-m98x.debug"

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy VAEG_SGP_SCAN_TRACE=1 \
    "$vaeg" --model "$model" --roms "$rom_directory" \
    --fdd1 "$output_directory/zundamon-orbit-m98x.d88" --no-cfg \
    --no-bkupmem --nowait --mute \
    --debug-script "$output_directory/zundamon-orbit-m98x.debug" \
    --debug-output-dir "$output_directory/out" \
    >"$output_directory/vaeg.stdout.log" \
    2>"$output_directory/sgp-trace.log"

printf 'M98X_VAEG_CAPTURE_PASS count=%s divisor=%s initial_page=%s output=%s\n' \
    "$runtime_count" "$divisor" "$initial_page" "$output_directory"

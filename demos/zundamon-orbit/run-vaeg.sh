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
active_count=${VAEG_ZUNDAMON_ACTIVE_COUNT:-4}
clear_mode=${VAEG_ZUNDAMON_CLEAR_MODE:-dirty}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
pristine_disk_image=$output_directory/zundamon-orbit-m98w-pristine.d88
disk_image=$output_directory/zundamon-orbit-m98w.d88
guest_image=$output_directory/ZUNDORB.COM
guest_listing=$output_directory/ZUNDORB.LST
atlas_directory=$output_directory/public-atlas
atlas_image=$output_directory/ZUNDORB.BIN
trace_log=$output_directory/sgp-trace.log
debug_script=$output_directory/zundamon-orbit-m98w.debug

[ -f "$source_image" ] || { printf 'error: source D88 does not exist\n' >&2; exit 1; }
[ -f "$vaeg" ] && [ -x "$vaeg" ] || { printf 'error: VAEG executable is unavailable\n' >&2; exit 1; }
[ -d "$rom_directory" ] || { printf 'error: ROM directory does not exist\n' >&2; exit 1; }
[ ! -e "$output_directory" ] || { printf 'error: refusing to overwrite output directory\n' >&2; exit 1; }
case "$model" in
    va2) ;;
    *) printf 'error: M98v automated validation requires VAEG_ZUNDAMON_MODEL=va2\n' >&2; exit 2 ;;
esac
case "$divisor" in
    1|2|3|4|5|6|7|8) ;;
    *) printf 'error: VAEG_ZUNDAMON_DIVISOR must be 1 through 8\n' >&2; exit 2 ;;
esac
case "$revolutions" in
    1|2) ;;
    *) printf 'error: VAEG_ZUNDAMON_REVOLUTIONS must be 1 or 2\n' >&2; exit 2 ;;
esac
case "$active_count" in
    1|2|4|8|16) ;;
    *) printf 'error: VAEG_ZUNDAMON_ACTIVE_COUNT must be 1, 2, 4, 8, or 16\n' >&2; exit 2 ;;
esac
case "$scenario" in
    static) scenario_define=0 ;;
    ladder) scenario_define=1 ;;
    pause) scenario_define=2 ;;
    missed) scenario_define=3 ;;
    *) printf 'error: VAEG_ZUNDAMON_SCENARIO must be static, ladder, pause, or missed\n' >&2; exit 2 ;;
esac
case "$clear_mode" in
    full) clear_mode_define=0 ;;
    dirty) clear_mode_define=1 ;;
    *) printf 'error: VAEG_ZUNDAMON_CLEAR_MODE must be full or dirty\n' >&2; exit 2 ;;
esac
if [ "$scenario" != static ] && { [ "$divisor" -ne 1 ] || [ "$revolutions" -ne 2 ]; }; then
    printf 'error: dynamic scenarios require divisor 1 and two revolutions\n' >&2
    exit 2
fi
case "$initial_page" in
    a) initial_page_define=0 ;;
    b) initial_page_define=1 ;;
    *) printf 'error: VAEG_ZUNDAMON_INITIAL_PAGE must be a or b\n' >&2; exit 2 ;;
esac

mkdir -p "$output_directory"
python3 "$script_dir/tools/build_zundamon_orbit_pipeline.py" \
    --fixture-output "$atlas_directory"
cp "$atlas_directory/zundorb.bin" "$atlas_image"
M98T_BOUNDED_QA=1 M98T_QA_CYCLES=$revolutions \
M98T_QA_SCENARIO=$scenario_define \
    M98T_INITIAL_VISIBLE_PAGE=$initial_page_define \
    M98V_ACTIVE_COUNT=$active_count \
    M98W_CLEAR_MODE=$clear_mode_define \
    NASM=${NASM:-nasm} "$script_dir/256/build.sh" "$guest_image" "$guest_listing"
M98T_BOUNDED_QA=1 M98T_QA_CYCLES=$revolutions \
M98T_QA_SCENARIO=$scenario_define \
    M98T_INITIAL_VISIBLE_PAGE=$initial_page_define \
    M98V_ACTIVE_COUNT=$active_count \
    M98W_CLEAR_MODE=$clear_mode_define \
    NASM=${NASM:-nasm} "$script_dir/build-local-d88.sh" \
        "$source_image" "$atlas_image" "$pristine_disk_image"
cp "$pristine_disk_image" "$disk_image"
cmp "$pristine_disk_image" "$disk_image"
python3 "$script_dir/tools/generate_zundamon_orbit_multi_debug.py" \
    --active-count "$active_count" \
    --initial-page "$initial_page" --divisor "$divisor" \
    --revolutions "$revolutions" \
    --scenario "$scenario" \
    --milestone m98w \
    --output "$debug_script"

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy VAEG_SGP_SCAN_TRACE=1 \
    "$vaeg" \
        --model "$model" \
        --roms "$rom_directory" \
        --fdd1 "$disk_image" \
        --no-cfg \
        --no-bkupmem \
        --nowait \
        --mute \
        --debug-script "$debug_script" \
        --debug-output-dir "$output_directory" \
        >"$output_directory/vaeg.stdout.log" 2>"$trace_log"

set -- python3 "$script_dir/tools/verify_zundamon_orbit_multi_guest.py" \
    --atlas "$atlas_image" \
    --table "$script_dir/256/zundamon_depth_table.inc" \
    --hud "$script_dir/256/zundamon_hud_table.inc" \
    --trace "$trace_log" \
    --active-count "$active_count" \
    --initial-page "$initial_page" --divisor "$divisor" \
    --revolutions "$revolutions" --scenario "$scenario" \
    --clear-mode "$clear_mode" \
    --milestone m98w \
    --report "$output_directory/m98w-oracle.json"
"$@" "$output_directory"
printf 'M98W_VAEG_CAPTURE_PASS active_count=%s initial_page=%s divisor=%s revolutions=%s scenario=%s clear_mode=%s output=%s\n' \
    "$active_count" "$initial_page" "$divisor" "$revolutions" "$scenario" "$clear_mode" "$output_directory"

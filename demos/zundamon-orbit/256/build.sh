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

if [ "$#" -gt 2 ]; then
    printf 'usage: %s [OUTPUT.COM [OUTPUT.LST]]\n' "$0" >&2
    exit 2
fi

nasm_command=${NASM:-nasm}
profile=${M98Y_PROFILE:-public}
bounded_qa=${M98T_BOUNDED_QA:-${M98S_BOUNDED_QA:-0}}
qa_cycles=${M98T_QA_CYCLES:-${M98S_QA_CYCLES:-1}}
qa_scenario=${M98T_QA_SCENARIO:-${M98S_QA_SCENARIO:-0}}
initial_visible_page=${M98T_INITIAL_VISIBLE_PAGE:-${M98S_INITIAL_VISIBLE_PAGE:-0}}
clear_mode=${M98W_CLEAR_MODE:-1}
auto_camera=${M98AA_AUTO_CAMERA:-1}
if [ "${M98X_RUNTIME_MODE+x}" = x ]; then
    runtime_mode=$M98X_RUNTIME_MODE
elif [ "${M98V_ACTIVE_COUNT+x}" = x ]; then
    # The legacy variable is retained only for M98v/M98w golden builds.
    runtime_mode=0
else
    runtime_mode=1
fi
active_count=${M98V_ACTIVE_COUNT:-4}
if [ "$profile" = private ]; then
    default_output=ZUNDAORB.COM
else
    default_output=ZUNDORB.COM
fi
output=${1:-$default_output}
listing=${2:-${output%.*}.LST}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_parent=$(dirname -- "$output")

case "$profile" in
    public) private_profile=0; profile_dir= ;;
    private)
        private_profile=1
        profile_dir=${M98Y_PRIVATE_PROFILE_DIR:-}
        private_atlas=${M98Y_PRIVATE_ATLAS:-}
        [ -n "$profile_dir" ] || {
            printf 'error: M98Y_PRIVATE_PROFILE_DIR is required for the private profile\n' >&2
            exit 2
        }
        [ -n "$private_atlas" ] || {
            printf 'error: M98Y_PRIVATE_ATLAS is required for the private profile\n' >&2
            exit 2
        }
        [ -d "$profile_dir" ] || {
            printf 'error: private profile directory does not exist\n' >&2
            exit 1
        }
        [ -f "$private_atlas" ] || {
            printf 'error: private atlas is unavailable\n' >&2
            exit 1
        }
        [ -f "$profile_dir/zundamon_depth_table.inc" ] || {
            printf 'error: private depth table is unavailable\n' >&2
            exit 1
        }
        [ -f "$profile_dir/zundamon_hud_table.inc" ] || {
            printf 'error: private HUD table is unavailable\n' >&2
            exit 1
        }
        ;;
    *)
        printf 'error: M98Y_PROFILE must be public or private\n' >&2
        exit 2
        ;;
esac

[ -d "$output_parent" ] || {
    printf 'error: output directory does not exist: %s\n' "$output_parent" >&2
    exit 1
}
command -v "$nasm_command" >/dev/null 2>&1 || {
    printf 'error: NASM command is unavailable\n' >&2
    exit 127
}
case "$bounded_qa" in
    0|1) ;;
    *) printf 'error: M98T_BOUNDED_QA must be 0 or 1\n' >&2; exit 2 ;;
esac
case "$qa_cycles" in
    1|2) ;;
    *) printf 'error: M98T_QA_CYCLES must be 1 or 2\n' >&2; exit 2 ;;
esac
case "$qa_scenario" in
    0|1|2|3) ;;
    *) printf 'error: M98T_QA_SCENARIO must be 0, 1, 2, or 3\n' >&2; exit 2 ;;
esac
case "$initial_visible_page" in
    0|1) ;;
    *) printf 'error: M98T_INITIAL_VISIBLE_PAGE must be 0 or 1\n' >&2; exit 2 ;;
esac
case "$runtime_mode" in
    0|1) ;;
    *) printf 'error: M98X_RUNTIME_MODE must be 0 or 1\n' >&2; exit 2 ;;
esac
case "$auto_camera" in
    0|1) ;;
    *) printf 'error: M98AA_AUTO_CAMERA must be 0 or 1\n' >&2; exit 2 ;;
esac
if [ "$runtime_mode" -eq 0 ]; then
    case "$active_count" in
        1|2|4|8|16) ;;
        *) printf 'error: M98V_ACTIVE_COUNT must be 1, 2, 4, 8, or 16\n' >&2; exit 2 ;;
    esac
else
    active_count=runtime
fi
case "$clear_mode" in
    0|1) ;;
    *) printf 'error: M98W_CLEAR_MODE must be 0 (full) or 1 (dirty)\n' >&2; exit 2 ;;
esac
if [ "$private_profile" -eq 1 ]; then
    [ "$runtime_mode" -eq 1 ] || {
        printf 'error: private profile requires runtime count mode\n' >&2
        exit 2
    }
    [ "$clear_mode" -eq 1 ] || {
        printf 'error: private profile requires dirty clear mode\n' >&2
        exit 2
    }
fi
if [ "$bounded_qa" -eq 1 ] && [ "$runtime_mode" -eq 0 ] && [ "${M98V_ACTIVE_COUNT+x}" != x ]; then
    printf 'error: bounded M98v QA requires an explicit M98V_ACTIVE_COUNT\n' >&2
    exit 2
fi

if [ "$private_profile" -eq 0 ]; then
    table_check_dir=$(mktemp -d "${TMPDIR:-/tmp}/zundamon-orbit-m98t-table.XXXXXX")
    trap 'rm -rf "$table_check_dir"' EXIT HUP INT TERM
    python3 "$script_dir/../tools/build_zundamon_orbit_pipeline.py" \
        --fixture-output "$table_check_dir/public-atlas" >/dev/null
    python3 "$script_dir/../tools/generate_zundamon_orbit_depth_table.py" \
        --atlas "$table_check_dir/public-atlas/zundorb.bin" \
        --output "$table_check_dir/zundamon_depth_table.inc" >/dev/null
    python3 "$script_dir/../tools/validate_zundamon_orbit_depth_table.py" \
        --input "$script_dir/zundamon_depth_table.inc" \
        --atlas "$table_check_dir/public-atlas/zundorb.bin" >/dev/null
    cmp "$table_check_dir/zundamon_depth_table.inc" \
        "$script_dir/zundamon_depth_table.inc"
    python3 "$script_dir/../tools/generate_zundamon_orbit_hud.py" \
        --output "$table_check_dir/zundamon_hud_table.inc" \
        --status-output "$table_check_dir/zundamon_status_table.inc" >/dev/null
    python3 "$script_dir/../tools/validate_zundamon_orbit_hud.py" \
        --input "$script_dir/zundamon_hud_table.inc" >/dev/null
    cmp "$table_check_dir/zundamon_hud_table.inc" \
        "$script_dir/zundamon_hud_table.inc"
    cmp "$table_check_dir/zundamon_status_table.inc" \
        "$script_dir/zundamon_status_table.inc"
    "$nasm_command" -f bin \
        -dM98T_BOUNDED_QA="$bounded_qa" \
        -dM98T_QA_CYCLES="$qa_cycles" \
        -dM98T_QA_SCENARIO="$qa_scenario" \
        -dM98Q_INITIAL_VISIBLE_PAGE="$initial_visible_page" \
        -dM98W_CLEAR_MODE="$clear_mode" \
        -dM98Q_CLEAR_MODE="$clear_mode" \
        -dM98X_RUNTIME_MODE="$runtime_mode" \
        -dM98V_ACTIVE_COUNT="${M98V_ACTIVE_COUNT:-4}" \
        -I "$script_dir/" \
        -l "$listing" \
        "$script_dir/zundamon_orbit_256.asm" -o "$output"
else
    python3 "$script_dir/../tools/inspect_zundamon_orbit_atlas.py" \
        --input "$private_atlas" >/dev/null
    python3 "$script_dir/../tools/validate_zundamon_orbit_depth_table.py" \
        --input "$profile_dir/zundamon_depth_table.inc" \
        --atlas "$private_atlas" --radius-x 96 --radius-y 16 >/dev/null
    python3 "$script_dir/../tools/validate_zundamon_orbit_hud.py" \
        --input "$profile_dir/zundamon_hud_table.inc" --subject IDA \
        --count-max 64 >/dev/null
    "$nasm_command" -f bin \
        -dM98Y_PRIVATE_PROFILE=1 \
        -dM98T_BOUNDED_QA="$bounded_qa" \
        -dM98T_QA_CYCLES="$qa_cycles" \
        -dM98T_QA_SCENARIO="$qa_scenario" \
        -dM98Q_INITIAL_VISIBLE_PAGE="$initial_visible_page" \
        -dM98W_CLEAR_MODE="$clear_mode" \
        -dM98Q_CLEAR_MODE="$clear_mode" \
        -dM98X_RUNTIME_MODE=1 \
        -dM98V_ACTIVE_COUNT=4 \
        -dM98AA_AUTO_CAMERA="$auto_camera" \
        -I "$profile_dir/" -I "$script_dir/" \
        -l "$listing" \
        "$script_dir/zundamon_orbit_256.asm" -o "$output"
fi

size=$(wc -c < "$output" | tr -d ' ')
[ "$size" -lt 65280 ] || {
    printf 'error: generated COM exceeds the 64-KiB DOS payload limit\n' >&2
    exit 1
}

if [ "$private_profile" -eq 1 ]; then
    printf 'M98AA_PRIVATE_GUEST_BUILD_PASS size=%s runtime_counts=1..64 dirty_profile=1 listing=%s\n' \
        "$size" "$listing"
elif [ "$runtime_mode" -eq 1 ]; then
    printf 'M98X_GUEST_BUILD_PASS size=%s default_count=4 runtime_counts=1..16 bounded_qa=%s revolutions=%s scenario=%s initial_page=%s clear_mode=%s listing=%s\n' \
        "$size" "$bounded_qa" "$qa_cycles" "$qa_scenario" "$initial_visible_page" "$clear_mode" "$listing"
else
    printf 'M98W_GUEST_BUILD_PASS size=%s active_count=%s bounded_qa=%s revolutions=%s scenario=%s initial_page=%s clear_mode=%s listing=%s\n' \
        "$size" "$active_count" "$bounded_qa" "$qa_cycles" "$qa_scenario" "$initial_visible_page" "$clear_mode" "$listing"
fi

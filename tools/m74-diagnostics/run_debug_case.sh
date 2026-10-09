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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

case_id=${1-}
case "$case_id" in
    ''|*[!a-z0-9_-]*|[-_]*)
        echo "usage: VAEG_M74_WORKER=... VAEG_M74_SCRIPT_DIR=... VAEG_M74_OUTPUT_DIR=... $0 case-id" >&2
        exit 2
        ;;
esac
if [ "$#" -ne 1 ]; then
    echo "error: the runner accepts exactly one neutral case identifier" >&2
    exit 2
fi

: "${VAEG_M74_WORKER:?set VAEG_M74_WORKER to the trace-enabled vaeg executable}"
: "${VAEG_M74_SCRIPT_DIR:?set VAEG_M74_SCRIPT_DIR to the local script directory}"
: "${VAEG_M74_OUTPUT_DIR:?set VAEG_M74_OUTPUT_DIR to the local output root}"
model=${VAEG_M74_MODEL-va}
case "$model" in
    va|va2) ;;
    *) echo "error: VAEG_M74_MODEL must be va or va2" >&2; exit 2 ;;
esac

script_path=${VAEG_M74_SCRIPT_DIR}/${case_id}.debug
case_output=${VAEG_M74_OUTPUT_DIR}/${case_id}
if [ ! -f "$VAEG_M74_WORKER" ] || [ ! -x "$VAEG_M74_WORKER" ]; then
    echo "error: configured worker is not executable" >&2
    exit 2
fi
if [ ! -f "$script_path" ]; then
    echo "error: no local debug script exists for case-id=${case_id}" >&2
    exit 2
fi
if ! guest_frame_bound=$(awk '
    $1 == "limit-frame" { count++; value = $2 }
    END {
        if ((count == 1) && (value ~ /^[0-9]+$/) && (value + 0 > 0)) {
            print value
        } else {
            exit 1
        }
    }' "$script_path"); then
    echo "error: local debug script needs one positive limit-frame declaration" >&2
    exit 2
fi
if [ -e "$case_output" ]; then
    echo "error: output already exists for case-id=${case_id}" >&2
    exit 2
fi
mkdir -p "$case_output"

runner_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "${runner_dir}/../.." && pwd)
source_sha=$(GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null \
    git -C "$repo_root" rev-parse HEAD)
if command -v sha256sum >/dev/null 2>&1; then
    worker_sha=$(sha256sum "$VAEG_M74_WORKER" | awk '{print $1}')
    runner_sha=$(sha256sum "$0" | awk '{print $1}')
else
    worker_sha=$(shasum -a 256 "$VAEG_M74_WORKER" | awk '{print $1}')
    runner_sha=$(shasum -a 256 "$0" | awk '{print $1}')
fi
{
    printf 'field\tvalue\n'
    printf 'schema\tvaeg-debug-run-v1\n'
    printf 'case\t%s\n' "$case_id"
    printf 'model\t%s\n' "$model"
    printf 'guest_frame_bound\t%s\n' "$guest_frame_bound"
    printf 'source_sha\t%s\n' "$source_sha"
    printf 'worker_sha256\t%s\n' "$worker_sha"
    printf 'runner_sha256\t%s\n' "$runner_sha"
} >"${case_output}/identity.tsv"

set -- "$VAEG_M74_WORKER" --model "$model" \
    --debug-script "$script_path" --debug-output-dir "$case_output"
if [ -n "${VAEG_M74_ROMS_DIR-}" ]; then
    set -- "$@" --roms "$VAEG_M74_ROMS_DIR"
fi
printf 'm74-debug-run case=%s model=%s source=%s worker=%s\n' \
    "$case_id" "$model" "$source_sha" "$worker_sha"
exec "$@"

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

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)
ENGINE=${CONTAINER_ENGINE:-docker}
IMAGE_TAG=${VAEG_OPENWATCOM_IMAGE_TAG:-vaeg/openwatcom:2026-08-01}
OUTPUT=${VAEG_OPENWATCOM_IMAGE_OUTPUT:-$REPO_ROOT/docs/openwatcom-image.tar}

if ! command -v "$ENGINE" >/dev/null 2>&1; then
    printf 'container engine not found: %s\n' "$ENGINE" >&2
    exit 1
fi

case "$OUTPUT" in
    /*) ;;
    *) OUTPUT=$REPO_ROOT/$OUTPUT ;;
esac

mkdir -p "$(dirname -- "$OUTPUT")"

"$ENGINE" build \
    --platform linux/amd64 \
    --file "$SCRIPT_DIR/containerfile" \
    --tag "$IMAGE_TAG" \
    "$SCRIPT_DIR"

"$ENGINE" run --rm --platform linux/amd64 "$IMAGE_TAG" \
    sh -c 'command -v wasm >/dev/null && command -v wlink >/dev/null && command -v wmake >/dev/null'

"$ENGINE" save --output "$OUTPUT" "$IMAGE_TAG"

if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$OUTPUT" > "$OUTPUT.sha256"
elif command -v openssl >/dev/null 2>&1; then
    openssl dgst -sha256 -r "$OUTPUT" | sed 's/ \*/  /' > "$OUTPUT.sha256"
elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$OUTPUT" > "$OUTPUT.sha256"
else
    printf 'no SHA-256 utility found\n' >&2
    exit 1
fi

printf 'exported image: %s\n' "$OUTPUT"
printf 'digest file: %s\n' "$OUTPUT.sha256"

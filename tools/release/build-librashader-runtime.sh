#!/bin/sh
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

if [ "$#" -ne 2 ]; then
	echo "usage: $0 <linux|macos|windows> <package-directory>" >&2
	exit 2
fi

platform=$1
package_dir=$2
repository=https://github.com/SnowflakePowered/librashader.git
revision=87e8a97b50516d997defeaa168173dcd185d4022

case "$platform" in
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
	echo "unsupported librashader runtime platform: $platform" >&2
	exit 2
	;;
esac

source_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-librashader.XXXXXX")
trap 'rm -rf "$source_dir"' EXIT HUP INT TERM

git clone --filter=blob:none --no-checkout "$repository" "$source_dir"
git -C "$source_dir" fetch --depth 1 origin "$revision"
git -C "$source_dir" checkout --detach "$revision"
test "$(git -C "$source_dir" rev-parse HEAD)" = "$revision"

cargo build --locked --package librashader-capi --profile optimized \
	--no-default-features --features runtime-opengl \
	--manifest-path "$source_dir/Cargo.toml"

runtime_path="$source_dir/target/optimized/$runtime_name"
test -f "$runtime_path"
mkdir -p "$package_dir"
install -m 755 "$runtime_path" "$package_dir/$runtime_name"

echo "staged librashader runtime: $package_dir/$runtime_name"

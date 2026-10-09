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

# Clean, fully static MinGW-w64 build of vaeg.exe in an isolated container.
#
# usage: tools/release/build-mingw-static-container.sh [--commit REV] [--work-dir DIR]
#
# REV defaults to HEAD. DIR defaults to ../vaeg-mingw-<short-sha> beside the
# repository and must not exist. The script clones REV into DIR/source (no
# existing build tree is reused), builds in a pinned Ubuntu 24.04 container
# with the POSIX-thread MinGW-w64 compilers and Rust 1.88.0 using the
# `mingw-cross` preset (static SDL2, LibArchive, zlib, xz, librashader and
# MinGW runtimes), audits the PE imports, and writes DIR/output/vaeg.exe,
# pe-imports.txt, third-party-notices.txt, the logs and BUILD-REPORT.md.
# It prints the absolute path of vaeg.exe on success.

set -euo pipefail

image="ubuntu@sha256:008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
revision="HEAD"
work_dir=""

while [[ $# -gt 0 ]]; do
	case "$1" in
	--commit) revision="$2"; shift 2 ;;
	--work-dir) work_dir="$2"; shift 2 ;;
	*) echo "usage: $0 [--commit REV] [--work-dir DIR]" >&2; exit 2 ;;
	esac
done

commit="$(git -C "${repo_root}" rev-parse --verify "${revision}^{commit}")"
if [[ -z "${work_dir}" ]]; then
	work_dir="$(dirname "${repo_root}")/vaeg-mingw-${commit:0:8}"
fi
if [[ -e "${work_dir}" ]]; then
	echo "work directory already exists: ${work_dir}" >&2
	exit 1
fi
mkdir -p "${work_dir}/output"
work_dir="$(cd "${work_dir}" && pwd)"

git clone --quiet --no-local --no-checkout "${repo_root}" "${work_dir}/source"
git -C "${work_dir}/source" checkout --quiet --detach "${commit}"
if [[ -n "$(git -C "${work_dir}/source" status --porcelain)" ]]; then
	echo "clean checkout is not clean" >&2
	exit 1
fi
branches="$(git -C "${repo_root}" branch -r --contains "${commit}" 2>/dev/null | tr -d ' ' | paste -sd, -)"

cat > "${work_dir}/inner.sh" <<'INNER'
#!/usr/bin/env bash
set -euo pipefail
trap 'chown -R "${HOST_UID}:${HOST_GID}" /work' EXIT
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
	ca-certificates curl git python3 cmake ninja-build pkg-config file gcc libc6-dev \
	mingw-w64 gcc-mingw-w64-x86-64-posix g++-mingw-w64-x86-64-posix \
	binutils-mingw-w64-x86-64 > /dev/null
update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal --default-toolchain 1.88.0 \
	--target x86_64-pc-windows-gnu > /dev/null
export PATH="${HOME}/.cargo/bin:${PATH}"
# CMake reads the commit and date with git under an empty git config, so
# git only trusts the clone if the container user owns it. The exit trap
# returns ownership to the host user.
chown -R 0:0 /work/source

cd /work/source
tools/release/build-mingw-static-librashader.sh > /work/output/librashader-build.log 2>&1
cmake --preset mingw-cross > /work/output/configure.log 2>&1
cmake --build --preset mingw-cross > /work/output/build.log 2>&1
python3 tools/release/check-static-windows-imports.py --binary build/mingw-cross/sdl2/vaeg.exe \
	> /work/output/import-audit.txt 2>&1
x86_64-w64-mingw32-objdump -p build/mingw-cross/sdl2/vaeg.exe | grep 'DLL Name' \
	> /work/output/pe-imports.txt
if ! grep -q "VAEG_BUILD_COMMIT \"$(git rev-parse HEAD)\"" build/mingw-cross/generated/vaeg_build_info.h ||
	grep -q 'VAEG_BUILD_DATE "unknown"' build/mingw-cross/generated/vaeg_build_info.h; then
	echo "build identity was not embedded" >&2
	exit 1
fi
cp build/mingw-cross/generated/vaeg_build_info.h /work/output/
cp build/mingw-cross/sdl2/vaeg.exe /work/output/vaeg.exe
cp build/mingw-static/windows-audit/third-party-notices.txt /work/output/
{
	echo "gcc=$(x86_64-w64-mingw32-gcc --version | head -1)"
	echo "gxx=$(x86_64-w64-mingw32-g++ --version | head -1)"
	echo "binutils=$(x86_64-w64-mingw32-ld --version | head -1)"
	echo "cmake=$(cmake --version | head -1)"
	echo "ninja=$(ninja --version)"
	echo "rustc=$(rustc --version)"
	echo "file=$(file -b /work/output/vaeg.exe)"
} > /work/output/toolchain.txt
INNER
chmod +x "${work_dir}/inner.sh"

echo "building ${commit} in ${work_dir}" >&2
docker run --rm -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" \
	-v "${work_dir}:/work" "${image}" /work/inner.sh

exe="${work_dir}/output/vaeg.exe"
digest="$(sha256sum "${exe}" | cut -d' ' -f1)"
size="$(stat -c %s "${exe}")"
{
	echo "# VAEG MinGW-w64 fully static build"
	echo
	echo "- Source commit: \`${commit}\`"
	echo "- Remote branches containing it: ${branches:-none}"
	echo "- Clean clone in \`${work_dir}/source\`; no existing build tree was reused."
	echo "- Container image: \`${image}\` (Ubuntu 24.04)"
	echo "- Preset: \`mingw-cross\`, Release, tests disabled; static SDL2, LibArchive,"
	echo "  zlib, xz, librashader and MinGW gcc/libstdc++/winpthread runtimes."
	sed 's/^/- /' "${work_dir}/output/toolchain.txt"
	echo
	echo "## Artifact"
	echo
	echo "- Path: \`${exe}\`"
	echo "- Size: ${size} bytes"
	echo "- SHA-256: \`${digest}\`"
	echo "- Import audit (\`check-static-windows-imports.py\`):"
	sed 's/^/  /' "${work_dir}/output/import-audit.txt"
	echo "- Imported DLLs: see \`pe-imports.txt\`."
	echo
	echo "Compile and link only. Windows execution, self-tests and guest behaviour"
	echo "were not run by this script."
} > "${work_dir}/output/BUILD-REPORT.md"
echo "${exe}"

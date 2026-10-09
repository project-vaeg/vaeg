#
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
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""Build the pinned C API and collect source identities and license notices."""
import argparse
import hashlib
import html
import json
import pathlib
import re
import shutil
import subprocess
import urllib.error
import urllib.request

PIN = "87e8a97b50516d997defeaa168173dcd185d4022"
FEATURES = {"x86_64-pc-windows-gnu": "runtime-d3d11",
            "aarch64-apple-darwin": "runtime-metal",
            "x86_64-apple-darwin": "runtime-metal",
            "x86_64-unknown-linux-gnu": "runtime-opengl"}


def read_notice(path):
    data = path.read_bytes()
    encoding = "utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    return data.decode(encoding).replace("\r\n", "\n")


def run(args, cwd):
    return subprocess.check_output(args, cwd=cwd, text=True)


def mingw_gcc_release(source):
    compiler = "x86_64-w64-mingw32-g++"
    reported = run([compiler, "-dumpfullversion"], source).strip()
    match = re.fullmatch(r"([0-9]+(?:\.[0-9]+){0,2})(?:-(?:posix|win32))?", reported)
    version = match.group(1) if match else ""
    if len(version.split(".")) == 3:
        return version

    # Debian's MinGW wrappers report only "<major>-posix". Use the exact
    # installed compiler package version so the corresponding GCC notices
    # still come from a release source tree, rather than guessing a patch.
    if reported.endswith("-posix") or reported.endswith("-win32"):
        suffix = reported.rsplit("-", 1)[1]
        package = "g++-mingw-w64-x86-64-" + suffix
        dpkg_query = shutil.which("dpkg-query")
        if dpkg_query:
            try:
                package_version = subprocess.check_output(
                    [dpkg_query, "-W", "-f=${Version}", package],
                    cwd=source, text=True, stderr=subprocess.DEVNULL).strip()
            except subprocess.CalledProcessError:
                package_version = ""
            match = re.match(r"([0-9]+\.[0-9]+\.[0-9]+)", package_version)
            if match and match.group(1).split(".", 1)[0] == reported.split("-", 1)[0]:
                return match.group(1)
    raise SystemExit("LIBRA_GCC_VERSION_UNRECOGNIZED: " + reported)


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument("--source", type=pathlib.Path, required=True)
    parser.add_argument("--target", choices=FEATURES, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--notices-only", action="store_true")
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    if run(["git", "rev-parse", "HEAD"], source).strip() != PIN:
        raise SystemExit("LIBRA_SOURCE_PIN_MISMATCH")
    if run(["git", "status", "--porcelain", "--untracked-files=no"], source).strip():
        raise SystemExit("LIBRA_SOURCE_MODIFIED")
    output.mkdir(parents=True, exist_ok=True)
    common = ["--locked", "-p", "librashader-capi", "--no-default-features",
              "--features", FEATURES[args.target], "--target", args.target]
    if not args.notices_only:
        subprocess.run(["cargo", "rustc", "--release", *common, "--",
                        "--print", "native-static-libs"], cwd=source, check=True)
    tree = run(["cargo", "tree", *common, "--edges", "normal,build",
                "--prefix", "none", "--format", "{p}|{l}"], source)
    identities = set(re.findall(r"^([^ ]+) v([^ |]+)", tree, re.M))
    metadata = json.loads(run(["cargo", "metadata", "--locked", "--format-version", "1",
        "--filter-platform", args.target, "--no-default-features",
        "--features", "librashader-capi/" + FEATURES[args.target]], source))
    notices, packages, unresolved = [], [], []
    for package in sorted(metadata["packages"], key=lambda p: (p["name"], p["version"])):
        if (package["name"], package["version"]) not in identities:
            continue
        root = pathlib.Path(package["manifest_path"]).parent
        license_id = package["license"] or ""
        # Explicit choice of the MPL side of librashader's dual license.
        selected = "MPL-2.0" if package["name"].startswith("librashader") else license_id
        tokens = set(re.findall(r"[A-Za-z0-9][A-Za-z0-9.+-]*", selected))
        allowed = {"MPL-2.0", "MPL-2.0+", "MIT", "MIT-0", "Apache-2.0", "BSD-2-Clause", "BSD-3-Clause",
                   "0BSD", "ISC", "Zlib", "Unicode-3.0", "Unicode-DFS-2016", "CC0-1.0",
                   "Unlicense", "OR", "AND", "WITH", "LLVM-exception"}
        if not tokens or tokens - allowed:
            raise SystemExit("LIBRA_LICENSE_REVIEW_REQUIRED: " + package["name"] + " " + selected)
        texts = []
        files = sorted(f for f in root.rglob("*") if f.is_file() and
                       f.name.lower().startswith(("license", "licence", "copying", "notice", "unlicense")))
        for file in files:
            try:
                texts.append((file.relative_to(root).as_posix(), read_notice(file)))
            except UnicodeDecodeError:
                continue
        if package["name"].startswith("librashader"):
            texts = [("LICENSE.md (MPL selected)", (source / "LICENSE.md").read_text())]
        # Some crates omit workspace-root licenses from their source archive.
        # Retrieve only at the source revision recorded by Cargo, never at HEAD.
        if not texts:
            vcs_file = root / ".cargo_vcs_info.json"
            repository = (package.get("repository") or "").removesuffix(".git")
            if vcs_file.exists() and repository.startswith("https://github.com/"):
                revision = json.loads(vcs_file.read_text())["git"]["sha1"]
                prefix = repository.replace("https://github.com/", "https://raw.githubusercontent.com/") + "/" + revision + "/"
                for name in ("LICENSE-MIT", "LICENSE-APACHE", "LICENSE", "LICENSE.md", "LICENSE.txt", "COPYING", "LICENSES/MIT.txt", "LICENSES/Apache-2.0.txt"):
                    cached = output / "upstream-notices" / (package["name"] + "-" + revision + "-" + name.replace("/", "_"))
                    try:
                        if not cached.exists():
                            data = urllib.request.urlopen(prefix + name, timeout=20).read()
                            cached.parent.mkdir(parents=True, exist_ok=True)
                            cached.write_bytes(data)
                        texts.append((prefix + name, read_notice(cached)))
                    except (urllib.error.HTTPError, UnicodeDecodeError):
                        continue
        if not texts and package["name"] == "winapi-x86_64-pc-windows-gnu":
            sibling = root.parent / "winapi-0.3.9" / "LICENSE-MIT"
            texts.append(("winapi-rs workspace MIT notice", read_notice(sibling)))
        if not texts and package["name"] in {"sptr", "vec_extract_if_polyfill"}:
            # These exact crate releases declare MIT in Cargo.toml but supply
            # no standalone notice, including at their recorded Git revisions.
            # Preserve the declaration and source reference, without inventing
            # a copyright holder, and include the standard MIT grant text.
            mit = (pathlib.Path(__file__).resolve().parents[2] / "external/imgui/LICENSE.txt").read_text()
            texts.append(("Package license declaration", (root / "Cargo.toml.orig").read_text()))
            texts.append(("MIT license text (no standalone upstream notice)", mit[mit.index("Permission is hereby granted"):]))
        if not texts:
            unresolved.append(package["name"] + " " + package["version"])
        # Preserve native-vendor source notices, including Mesa's per-file grants.
        if (root / "native").exists():
            seen = {text for _, text in texts}
            for file in sorted((root / "native").rglob("*")):
                if not file.is_file() or file.suffix not in {".c", ".h", ".cpp", ".hpp"}:
                    continue
                text = file.read_text(encoding="utf-8", errors="replace")
                for comment in re.findall(r"/\*.*?\*/", text[:20000], re.S):
                    if re.search(r"copyright|permission is hereby granted|SPDX-License-Identifier", comment, re.I) and comment not in seen:
                        texts.append((file.relative_to(root).as_posix(), comment))
                        seen.add(comment)
        title = package["name"] + " " + package["version"]
        section = title + "\nDeclared license: " + license_id + "\nSelected terms: " + selected
        section += "\nSource: " + (package.get("repository") or package.get("source") or "")
        if (package.get("source") or "").startswith("registry+"):
            section += "\nCorresponding source archive: https://crates.io/api/v1/crates/" + package["name"] + "/" + package["version"] + "/download"
        section += "\nAuthors: " + ", ".join(package["authors"]) + "\n"
        for name, text in texts:
            section += "\n--- " + name + " ---\n" + text + "\n"
        notices.append(section)
        packages.append({"name": package["name"], "version": package["version"],
                         "license": license_id, "selected": selected,
                         "notice_sha256": hashlib.sha256(section.encode()).hexdigest()})
    rust_version = run(["rustc", "--version"], source).strip()
    sysroot = pathlib.Path(run(["rustc", "--print", "sysroot"], source).strip())
    rust_notice = next((candidate for candidate in (
        sysroot / "share/doc/rust/COPYRIGHT-library.html",
        sysroot / "share/doc/rustc/COPYRIGHT-library.html",
    ) if candidate.exists()), None)
    if rust_notice is None:
        raise SystemExit("LIBRA_RUST_LIBRARY_NOTICE_MISSING")
    notices.append(rust_version + "\nRust standard library: https://github.com/rust-lang/rust\n" +
                   html.unescape(re.sub(r"<[^>]*>", "", rust_notice.read_text())))
    if args.target == "x86_64-pc-windows-gnu":
        gcc_version = mingw_gcc_release(source)
        for name in ("COPYING3", "COPYING.RUNTIME"):
            url = "https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-" + gcc_version + "/" + name
            cached = output / "upstream-notices" / ("gcc-" + gcc_version + "-" + name)
            if not cached.exists():
                cached.parent.mkdir(parents=True, exist_ok=True)
                cached.write_bytes(urllib.request.urlopen(url, timeout=20).read())
            notices.append("GCC " + gcc_version + " runtime libraries; GCC Runtime Library Exception applies.\n" +
                           "Corresponding source: https://gcc.gnu.org/releases.html\n" + url + "\n" + read_notice(cached))
    (output / "third-party-notices.txt").write_text(
        "librashader static build; includes normal and build dependencies.\n"
        "Source revision: " + PIN + "\n\n" + "\n\n".join(notices), encoding="utf-8")
    archive = source / "target" / args.target / "release" / "liblibrashader_capi.a"
    manifest = {"source_revision": PIN, "target": args.target,
                "rustc_version": rust_version,
                "feature": FEATURES[args.target], "packages": packages,
                "archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                "cargo_lock_sha256": hashlib.sha256((source / "Cargo.lock").read_bytes()).hexdigest(),
                "unresolved_notices": unresolved}
    manifest["notices_sha256"] = hashlib.sha256((output / "third-party-notices.txt").read_bytes()).hexdigest()
    (output / "static-build.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if unresolved:
        raise SystemExit("LIBRA_MISSING_NOTICES: " + ", ".join(unresolved))


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Verify and install the VAEG 8087 v6 document pack without overwriting files."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys

PREFIX = PurePosixPath("docs/8087/v6")

class InstallError(Exception):
    """A validation or safe-installation failure."""

def digest(path: Path) -> str:
    with path.open("rb") as stream:
        h = hashlib.sha256()
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
        return h.hexdigest()

def no_symlinks(base: Path, relative: PurePosixPath) -> Path:
    current = base
    for component in relative.parts:
        current = current / component
        if current.is_symlink():
            raise InstallError(f"Symlink path component is not allowed: {current}")
    return current

def safe_relative(value: object) -> PurePosixPath:
    if not isinstance(value, str) or "\\" in value or ":" in value:
        raise InstallError("Invalid manifest path")
    path = PurePosixPath(value)
    if path.is_absolute() or not path.parts or any(p in (".", "..") for p in path.parts):
        raise InstallError(f"Unsafe manifest path: {value}")
    if path.as_posix() != value:
        raise InstallError(f"Non-normalized manifest path: {value}")
    return path

def verify_pack(base: Path) -> list[tuple[PurePosixPath, Path]]:
    manifest_path = base / "MANIFEST.json"
    if manifest_path.is_symlink():
        raise InstallError("Manifest must not be a symlink")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("format_version") != 1 or manifest.get("pack_version") != "v6":
        raise InstallError("Unsupported manifest version")
    entries = manifest.get("files")
    if not isinstance(entries, list) or not entries:
        raise InstallError("Manifest contains no files")
    seen: set[str] = set()
    documents: list[tuple[PurePosixPath, Path]] = []
    for item in entries:
        if not isinstance(item, dict):
            raise InstallError("Invalid manifest entry")
        relative = safe_relative(item.get("path"))
        key = relative.as_posix()
        if key in seen:
            raise InstallError(f"Duplicate manifest path: {key}")
        seen.add(key)
        is_document = relative.parts[:len(PREFIX.parts)] == PREFIX.parts
        if not is_document and key not in ("INSTALL.md", "install-pack.py"):
            raise InstallError(f"Unexpected pack file: {key}")
        source = no_symlinks(base, relative)
        if not source.is_file():
            raise InstallError(f"Missing source file: {source}")
        if source.stat().st_size != item.get("size") or digest(source) != item.get("sha256"):
            raise InstallError(f"Checksum or size mismatch: {key}")
        if is_document:
            documents.append((relative, source))
    actual = set()
    for path in base.rglob("*"):
        if path.is_symlink():
            raise InstallError(f"Symlink in pack: {path}")
        if path.is_file() and path != manifest_path:
            actual.add(path.relative_to(base).as_posix())
    if actual != seen:
        raise InstallError("Pack contents do not exactly match the manifest")
    if not documents or f"{PREFIX}/goal-contract.md" not in seen:
        raise InstallError("Missing v6 contract")
    return sorted(documents)

def repository_root(value: str) -> Path:
    repo = Path(value).expanduser().resolve(strict=True)
    if not repo.is_dir():
        raise InstallError("Repository path is not a directory")
    completed = subprocess.run(
        ["git", "-C", str(repo), "rev-parse", "--show-toplevel"],
        check=False, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    if completed.returncode:
        raise InstallError(f"Not a Git worktree: {completed.stderr.strip()}")
    root = Path(completed.stdout.strip()).resolve(strict=True)
    if root != repo:
        raise InstallError(f"Use the worktree root, not a subdirectory: {root}")
    return repo

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, help="Existing VAEG Git worktree root")
    parser.add_argument("--apply", action="store_true", help="Copy new files; otherwise preview only")
    args = parser.parse_args()
    base = Path(__file__).resolve().parent
    target: Path | None = None
    target_created = False
    try:
        documents = verify_pack(base)
        repo = repository_root(args.repo)
        target = no_symlinks(repo, PREFIX)
        if os.path.lexists(target):
            raise InstallError(f"Destination already exists; nothing will be overwritten: {target}")
        for relative, _ in documents:
            no_symlinks(repo, relative)
        print(f"Verified {len(documents)} document files.")
        print(f"Destination: {target}")
        for relative, _ in documents:
            print(f"  CREATE {relative}")
        if not args.apply:
            print("Preview only. Re-run with --apply to create this new directory.")
            return 0
        parent = repo
        for part in PREFIX.parts[:-1]:
            parent = parent / part
            if parent.is_symlink():
                raise InstallError(f"Symlink destination is not allowed: {parent}")
            parent.mkdir(exist_ok=True)
            if not parent.is_dir():
                raise InstallError(f"Destination parent is not a directory: {parent}")
        target.mkdir(exist_ok=False)
        target_created = True
        for relative, source in documents:
            destination = no_symlinks(repo, relative)
            destination.parent.mkdir(parents=True, exist_ok=True)
            # Exclusive creation prevents overwriting a preexisting file.
            with source.open("rb") as inp, destination.open("xb") as out:
                while True:
                    chunk = inp.read(1024 * 1024)
                    if not chunk:
                        break
                    out.write(chunk)
            if digest(destination) != digest(source):
                raise InstallError(f"Installed checksum mismatch: {destination}")
        print("Installed new v6 documents. Existing files, the Git index, and commits were not modified.")
        print("Next: read docs/8087/v6/README.md and send activation-acquire.txt to Codex; stop for user OCR.")
        return 0
    except (InstallError, OSError, ValueError, TypeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        if target_created:
            print(f"Partial new installation may remain at {target}; no files were deleted.", file=sys.stderr)
        return 2

if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from install_cpmva import InstallerError, atomic_write, download_bytes, load_lock, resolve_download_url


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Explicitly refresh the CP/MVA source lock after maintainer review"
    )
    parser.add_argument("--lock", type=Path, default=Path(__file__).with_name("sources.lock.json"))
    parser.add_argument("--confirm", action="store_true")
    parser.add_argument("--write", action="store_true", help="write reviewed changes after --confirm")
    args = parser.parse_args()
    lock = load_lock(args.lock)
    if not args.confirm:
        print("Refusing to refresh sources.lock.json without --confirm.", file=sys.stderr)
        return 2
    changes = []
    for name, spec in lock["sources"].items():
        first = (
            resolve_download_url(spec, lock)
            if spec.get("discovery_url") and spec.get("archive_type")
            else spec["resolved_url"]
        )
        candidates = [first]
        fallback = spec.get("http_fallback_url")
        if fallback and fallback not in candidates:
            candidates.append(fallback)
        last_error = None
        for candidate in candidates:
            try:
                data, final_url = download_bytes(
                    candidate, lock["limits"]["max_download_bytes"], lock
                )
                break
            except InstallerError as error:
                last_error = error
        else:
            raise last_error
        new_size = len(data)
        new_digest = __import__("hashlib").sha256(data).hexdigest()
        changes.append((name, spec, new_size, new_digest, final_url))
    changed = False
    for name, spec, new_size, new_digest, final_url in changes:
        print(f"{name}:")
        print(f"  old size={spec['size']} sha256={spec['sha256']} url={spec['resolved_url']}")
        print(f"  new size={new_size} sha256={new_digest} url={final_url}")
        changed = changed or spec["size"] != new_size or spec["sha256"] != new_digest or spec["resolved_url"] != final_url
    if not changed:
        print("All locked source values are unchanged; lock file was not modified.")
        return 0
    if not args.write:
        print("Changes require explicit review; no lock update was performed.")
        print("Re-run with --confirm --write only after reviewing the values.")
        return 3
    for name, spec, new_size, new_digest, final_url in changes:
        spec["size"] = new_size
        spec["sha256"] = new_digest
        spec["resolved_url"] = final_url
    atomic_write(
        args.lock,
        (json.dumps(lock, indent=2, sort_keys=False) + "\n").encode("utf-8"),
    )
    print(f"Updated {args.lock}; review and commit the visible lock-file diff.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

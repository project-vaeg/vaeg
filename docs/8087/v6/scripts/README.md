# Reference acquisition helper

Python 3.9+ and Git are required. No pip dependencies, OCR tools, PDF text converters,
archive extractors or native compiler are required. Code and diagnostics are English.

## Preview and execute

From the VAEG worktree root:

```sh
python3 docs/8087/v6/scripts/acquire-sources.py --repo "$PWD"
python3 docs/8087/v6/scripts/acquire-sources.py --repo "$PWD" --apply
```

The default evidence root is a sibling of that worktree named `vaeg-8087-evidence-v6`.
To use another directory, pass `--evidence-root /absolute/external/directory` on both calls.
It must be outside Git worktrees and have no symlink path components. Preview makes no
network requests and creates no directories. It is not a successful download.

The default catalog is [../sources/catalog.json](../sources/catalog.json).
A reviewed local replacement may be selected with `--catalog /absolute/path/catalog.json`.
Keep the original catalog and both hashes. Do not broaden excluded source actions.

## What the helper does

Approved PDF/HTML/ZIP bytes are downloaded over verified HTTPS. Redirects are restricted
to catalog hosts. No recursive page assets, cookies, login, source-tree clone, binary
execution, archive extraction, OCR or content transcription is performed. ZIP member CRC
checks do read decompressed bytes in memory but do not extract files to the filesystem.
Only approved upstream archives are accepted; emulator code/microcode is excluded.

The helper bounds requests to two attempts per candidate URL, at most four candidates,
30-second inactivity and 180-second elapsed attempt limits. A timeout is checked at read
boundaries, so a blocking read can delay the deadline check by its inactivity timeout.
The hard cap is 256 MiB per file and 512 MiB transferred per invocation, including failed
attempts. Large originals are streamed. Successful files receive SHA-256 receipts; HTML
identity and PDF header/EOF checks are not full semantic or bibliographic verification.

An observed first-download hash records received bytes; it is not a publisher signature.
Expected trusted hashes remain null in the distributed catalog. P02 must audit and freeze
runtime dependency identity before vendoring. Never change a pin merely to accept a mismatch.

## Resume and conflict behavior

Original files are exclusively created, not overwritten. Matching existing receipts are
rehash-verified and reused without a network request. Unmanaged or modified originals are
rejected. Do not delete them to hide a conflict; inspect and record the cause first.
The helper only removes its own transfer temporary file and its own active lock.

An existing `.acquisition.lock` blocks another run. After a crashed process, inspect whether
it is still running before manually resolving that stale lock. Never remove a lock from an
active process. Do not modify the same workspace concurrently with another tool.

Each run retains a fresh report directory. The owned `latest.json` points to the last
handoff; previous reports and user OCR are not overwritten. A filesystem/permission error
may leave partial new files and cannot count as successful handoff. Report the error and
actual paths; the agent must not proceed to implementation to work around it.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | Preview succeeded, or acquisition completed with no required item missing; optional failures may remain |
| 2 | Catalog, workspace, filesystem or safety validation failed |
| 3 | Acquisition report written, but at least one required item failed |
| 130 | Interrupted; when possible, a partial report is written |

Always inspect the JSON report and its workflow state, not just an exit code. All acquisition
states have `implementation_authorized=false` and `ocr_performed=false`. A successful Stage A
run stops at user handoff. It is not an emulator test or authorization for Stage B.

## Reproducible offline tests

```sh
PYTHONDONTWRITEBYTECODE=1 python3 docs/8087/v6/scripts/test-acquire-sources.py
```

Tests use synthetic PDF/ZIP/HTML fixtures, mocked HTTP responses and temporary Git trees.
They cover hash/receipt matching, corrupted files, byte limits, unsafe paths/redirects,
error pages, queue separation, report preservation, preview and explicit stop states.
They do not prove that remote hosts are available or that a scanned manual's tables are
correct. Live retrieval is performed later in the authorized Codex environment.

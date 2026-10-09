# Stage A — reference acquisition and mandatory OCR handoff

Normative with goal-contract.md section 0. This is a bounded filesystem/network task, not
implementation. The user performs OCR after this stage, so the agent must stop at handoff.

## A00: establish a safe workspace

Read applicable AGENTS instructions and this v6 contract. Record worktree root, HEAD, dirty
paths and the installed catalog hash without changing production files or running a build.
Respect previous work. Do not execute P00's baseline/gate creation yet.

Default external evidence root: a sibling of the current worktree named
`vaeg-8087-evidence-v6`. For a worktree located inside another Git tree, select an explicit
user-approved directory outside all Git worktrees instead. The supplied helper rejects
Git-contained evidence roots and symlink paths. Do not silently fall back to `docs/`, `/tmp`,
or a location inside the repository. Do not search unrelated user directories.

The repository gets v6 instructions and short sanitized progress only. Scans, downloads,
OCR results, retrieval receipts with local paths and private sources remain external.

## A01: retrieve approved originals

Read sources/catalog.json and source-policy.md. Run the supplied helper from the worktree:

```sh
python3 docs/8087/v6/scripts/acquire-sources.py --repo "$PWD"
python3 docs/8087/v6/scripts/acquire-sources.py --repo "$PWD" --apply
```

The first command previews only. Set `--evidence-root /absolute/external/path` on BOTH
commands when overriding the default. Python 3.9+ and Git are needed; network permission
comes from the actual Codex environment. Ask for required tool permission rather than
claiming a successful download. Do not install a new browser/plugin just to evade a denial.

Download all catalog entries with FETCH_DOCUMENT, FETCH_ARCHIVE, or FETCH_EVIDENCE_ONLY.
Do not recurse through sites or clone repositories. METADATA_ONLY and DISCOVERY_ONLY rows
are not download authorizations. The E8087 binary and excluded emulator sources are not
included in the automatic acquisition set.

The helper does not discover URLs itself. For unavailable allowed documents (especially
I05), Codex may make at most three focused searches per document for a public direct copy
with matching manufacturer identifier. Do not guess filenames. Record the discovery URL,
actual new URL and apparent edition. Add an allowed entry to a reviewed local catalog copy,
then rerun the helper with `--catalog`; preserve both catalog hashes. New documentation
hosts require an explicit catalog entry. Never broaden the action class of an excluded
source or use a login/paywall workaround. A newer edition gets a distinct source ID, not a
silent replacement under an old edition's ID.

Defaults bound each URL attempt and file size. The helper uses up to two attempts per URL,
verified TLS, a 30-second network inactivity timeout, a 180-second attempt deadline checked between reads,
256 MiB maximum per file and 512 MiB total transferred per invocation. A source may set a
smaller cap. Large I02 is approximately 93 MB at the reviewed host; do not mistake a browser
preview size limit for proof that the original is unavailable. Download the file as bytes.

## A02: verify and inventory, without OCR

For each successful file retain source ID, requested/final URL, UTC retrieval time, byte
count, actual SHA-256, content type and validation level. PDFs must have a PDF header and
end-of-file marker; those checks are NOT a full PDF structural or bibliographic validation.
ZIPs receive a bounded CRC check without extraction. HTML receives a basic identity/error-
page check and is stored unchanged. Do not execute scripts in a downloaded page.

The helper writes exclusive originals and an adjacent receipt, using temporary files only
inside the evidence workspace. Resume rehashes existing files and requires matching receipts;
unknown/conflicting files are not overwritten or silently adopted. A hash mismatch requires
inspection. Do not modify the expected hash to make an untrusted archive pass. No publisher
signature is claimed for an observed first-download hash.

Optional `pdfinfo`/`qpdf --check` may be used ONLY if already installed and only for structural
metadata, with output recorded outside Git. No OCR, text extraction, page rasterization batch,
PDF-to-Markdown conversion, AI transcription, or source-archive extraction is permitted.
A preexisting PDF text layer does not waive the stop. Publication dates in filenames remain
host labels until the actual edition is checked; I03's cover carries copyright 1981 despite
its mirror filename containing Nov83.

## A03: emit the user's handoff and stop

The helper creates a unique `runs/<run-id>/` report set and atomically updates `latest.json`:

- `download-report.json` and `download-report.md`: successes, failures, skipped actions,
  expected versus observed hashes, exact filenames, and evidence-only classifications.
- `CHECKSUMS.sha256`: successful originals only, paths relative to the evidence root.
- `ocr-queue.tsv`: only successfully retrieved, implementation-readable PDF originals;
  source ID, original path/hash, priority and suggested user OCR output directory.
- `OCR-HANDOFF.md`: absolute local workspace and report paths, user instructions and limitations.

A successful HTTP response, an entry in the catalog, or a filename is not a successful file.
If required downloads failed, report each real error and use
`ACQUISITION_INCOMPLETE_AWAITING_USER`; still hand over all successful files and stop.
Otherwise use `AWAITING_USER_OCR`. Optional failures must remain visible but do not create
an infinite retrieval loop. Print exact paths in the local user response; do not copy those
private absolute paths to committed progress.

Update records/progress.md with the stage, catalog hash, safe run ID, counts and next human
action. Do not put fake source hashes or private file contents there. Stage A never writes
`SOFTWARE_VERIFIED`, creates a fake implementation gate or starts P00-P19. Do not poll OCR
folders, schedule a continuation, or infer permission from new files. The next actor is the user.

## What is forbidden before handoff

No OCR tool invocation (local, cloud or subagent); no pdftotext/AI manual transcription;
no writing manual-content .md from scans; no dependency extraction/build/install/vendor;
no production code, tests, CMake, CI, shader, CPU, FPU or emulator configuration changes;
no baseline builds or implementation gates; no source-code browsing of excluded references;
no ROM, disk image, full-source-tree or raw microcode acquisition; no publish/push/merge.
The metadata/report .md files generated by this helper are not OCR substitutes.

## Stop versus completion

Stage A's allowed outcome is a completed acquisition handoff (or a truthful incomplete
handoff). The emulator remains NOT_IMPLEMENTED/NOT_VERIFIED by this run. Continuation into
Stage B requires the distinct user instruction in activation-implement.txt and OCR acceptance.

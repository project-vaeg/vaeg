# v6 activation: two distinct goals

## Stage A: acquire and stop

Start Codex in the VAEG worktree root. Send `activation-acquire.txt` (also provided as
`activation.txt`). This goal intentionally ends at the user's OCR handoff.

```text
/goal Execute only Stage A (A00-A03) of docs/8087/v6/goal-contract.md in the checked-out VAEG repository. Download the allowlisted public reference documents and approved upstream archives from docs/8087/v6/sources/catalog.json into the external evidence workspace using docs/8087/v6/scripts/acquire-sources.py, following docs/8087/v6/acquisition.md and docs/8087/v6/source-policy.md. Verify the actual files, record source URLs, byte counts, SHA-256 hashes and failures, and generate the OCR handoff and queue. Perform no OCR, PDF-to-text or PDF-to-Markdown conversion, source-archive extraction, dependency build, vendoring, emulator implementation, or P00-P19 work. The user will perform OCR. Stop after the acquisition handoff, with AWAITING_USER_OCR or ACQUISITION_INCOMPLETE_AWAITING_USER; never continue into implementation automatically, even if a PDF already has text or a prior OCR file exists. Preserve existing files, do not push or merge, and report exact local handoff paths. This goal is complete only as a reference-acquisition handoff, not as completed 8087 emulation.
```

Do not send the implementation goal at the same time. Acquisition is not a license to
start P00. A previously active v5 implementation goal must first be paused or replaced
using the installed Codex goal controls; do not leave both objectives active.

## Human interval

The user creates OCR Markdown outside Git. No background polling, automatic OCR, automatic
resume, or implementation occurs. An OCR file appearing on disk is not authorization.
The source scans and original checksums remain unchanged. See [ocr-handoff.md](ocr-handoff.md).

## Stage B: only after the user supplies OCR and explicitly authorizes implementation

The user may identify a non-default evidence/OCR directory in the accompanying message.
Use the following new objective (also in `activation-implement.txt`):

```text
/goal The user has completed the manual OCR handoff and explicitly authorizes Stage B of docs/8087/v6/goal-contract.md. Read docs/8087/v6/records/progress.md, docs/8087/v6/ocr-handoff.md and the external acquisition report, validate the supplied OCR files against their original source IDs and hashes, and record the acceptance before P00. Do not perform or rerun OCR. Then implement and locally verify documented-instruction-complete Intel 8087 emulation in the checked-out VAEG repository through P00-P19. Preserve the independent configurable default clock of 10000000 Hz, reset-time application, at most one optional 8087 per machine, no 8087 on FPO2, SERIAL_TIMED_V1, SoftFloat-3e provenance, C99/no-host-floating-point boundaries, independent semantic tests, private-source separation and all final gates. Use E8087 only as an optional external black-box reference with a separately validated harness; no emulator-source or raw-microcode import. Do not weaken mandatory instruction coverage or invent VA interrupt routing. Preserve existing work, do not push or merge, continue all unblocked packages, and claim SOFTWARE_VERIFIED only after the actual mandatory local gates pass.
```

A separate objective is deliberate: blindly resuming Stage A still authorizes only Stage A.
The words above must not be sent before the user has actually completed the handoff.
No self-created file, token, or model-written `approved=true` substitutes for that message.

## Interrupted work

For a budget or system pause inside a stage, read `records/progress.md`, inspect the actual
worktree and external report, and resume only that same authorized stage. Reuse originals
whose receipt/hash still matches; do not reinstall this pack over progress. In Stage B,
revalidate changed prerequisites and continue the earliest invalidated/incomplete package.

Codex lifecycle controls depend on the installed version and permissions. Official commands
include `/goal`, `/goal pause`, `/goal resume`, and `/goal clear`; see C01 in the source register.
A prompt cannot override a system pause, tool permission, or run budget.

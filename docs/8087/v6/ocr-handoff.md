# Human OCR handoff and Stage B entry gate

## What the user receives

Open `latest.json` in the external evidence root to locate the most recent run's
`OCR-HANDOFF.md` and `ocr-queue.tsv`. The queue lists only successfully fetched PDF
originals. Original files and SHA-256 receipts are preserved; do not OCR in-place over them.

The user may use any OCR workflow. The default output layout is:

```text
vaeg-8087-evidence-v6/
  originals/I02/<original-name>.pdf
  originals/I02/<original-name>.pdf.receipt.json
  user-ocr/I02/document.md
  user-ocr/I02/page-map.tsv           # optional if document.md already maps pages
  user-ocr/I02/assets/               # optional figures, remain outside Git
```

For a split document, `user-ocr/I02/*.md` is also acceptable. Equivalent files in a user-
identified external directory are acceptable; do not force the user to move existing work.
The agent must not generate placeholder document.md files or treat empty templates as OCR.

Prefer a source ID and PDF page markers in the Markdown, for example:

```text
Source ID: I02

<!-- pdf-page: 1; printed-page: cover -->
...
<!-- pdf-page: 17; printed-page: 2-3 -->
...
```

Page numbering here is 1-based PDF page index. Printed labels can be Roman numerals,
chapter-page labels, or unknown. Do not invent a single fixed page offset for mixed front
matter and body. If OCR software cannot retain page markers, say so; after explicit resume,
Codex may build a limited page map by visual inspection of the original and the supplied
text, without invoking OCR. Do not silently treat every OCR line as source-verified.

The user is not required to calculate checksums or author JSON approval files. Codex records
those facts on resume. Correct critical tables before numerical/encoding gates are frozen.

## Before implementation can start

Both conditions are required:

1. An explicit user message authorizes Stage B after OCR, identifying the OCR/evidence
   directory when non-default. The appearance of files or a model-authored receipt is not
   authorization. The agent records the actual message reference/date, not an invented one.
2. The accepted source set is usable for the pending implementation work. By default the
   core PDFs are I01, I02, I03, I04 and N01. Verify each original against its receipt, find
   the actual user text, hash that text, and map the source ID/edition and critical pages.
   Optional N02/I06 and the unresolved early I05 may remain deferred. Equivalent user-
   supplied primary editions can satisfy a core source only with an explicit recorded
   source-substitution decision; never relabel a later manual as the 8087 original.

Stage B creates an external `ocr-acceptance.json` with source ID, original SHA-256, OCR
file paths/hashes, page-map status, critical-table review status, authorizing user-message
reference and remaining uncertainties. A sanitized records/ocr-acceptance.md links opaque
IDs/hashes only. This receipt records acceptance; it does not create authorization.

Empty, missing, wrong-edition or mismatched files keep Stage B at OCR_INPUT_BLOCKED for the
relevant sources. Make a compact request listing exactly what is missing. Do not run OCR
as a repair, fall back silently to third-party emulator source, or invent prose from memory.
User-authorized limited preprocessing of their Markdown is allowed only in derivative files;
never overwrite the user's original OCR or the scanned PDF.

## Critical review before freezing P01

Compare the actual printed tables/figures for: opcode bytes and ModR/M; WAIT/NOP prefixes;
PC/RC/IC/IEM bits and reset values; status/tag layouts; 14/94-byte images; old 80-bit classes;
strict versus non-strict domain endpoints; exponent signs/superscripts; ST(0)/ST(1) order;
result delivery and pop/store behavior under exceptions; NEC FPO1/FPO2/POLL CPU applicability.

Flag uncertain characters with source page and a concrete question. OCR confidence scores,
clean spelling, or agreement with a later x87 manual do not prove a table correct.
The original page remains authoritative; diagrams can be reviewed visually after resume.
All original/OCR images and full transcriptions stay outside the public repository.

## Suggested user resume message

```text
OCR is complete. The files are in the evidence workspace under user-ocr/<source-id>/.
Use the actual download receipts to verify source identities. I authorize Stage B
according to docs/8087/v6/goal-contract.md. Do not run OCR. Validate the supplied text
and page mapping, then implement and test P00-P19 without overwriting existing work.
```

The separate full `/goal` is in activation-implement.txt. Sending a generic `/goal resume`
while Stage A remains the active objective does not grant Stage B permission.

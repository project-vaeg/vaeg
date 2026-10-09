# VAEG 8087 v6 progress

## Initial delivery state — not an executed result

```text
workflow_stage: NOT_STARTED
next_action: A00-A03 acquisition, then human OCR handoff
implementation_authorized: false
user_ocr_received: false
acquisition: NOT_RUN
implementation_P00_P19: NOT_STARTED
mandatory_local_emulator_gates: NOT_RUN
external_reference_comparisons: NOT_RUN
```

This describes only delivery of instructions/helpers. It does not mean PDFs were downloaded
on the user's machine, OCR occurred, source code was implemented, or VAEG passed a test.

## Stage transitions

`NOT_STARTED -> ACQUIRING -> AWAITING_USER_OCR` (or
`ACQUISITION_INCOMPLETE_AWAITING_USER`) is the entire default activation.
Only a subsequent explicit user message and accepted OCR/source mapping permit
`OCR_ACCEPTED -> IMPLEMENTING_P00_P19 -> SOFTWARE_VERIFIED`.
No agent-created approval, existing text layer or file-change event opens that transition.

## Record actual execution compactly

Per attempt: stage/package, date, actual HEAD/dirty digest, changed paths, commands/exit
codes/test counts if applicable, catalog/source artifact hashes, safe run ID, failures,
next action and next actor. Use external evidence IDs rather than private absolute paths.
Do not mark future packages PASS or repeat initial NOT_RUN entries as observed results.

At A03 the next actor is USER. Record the terminal handoff status and stop. In Stage B,
revalidate changed prerequisites and resume the earliest incomplete package. Preserve older
v2-v5 work and completed records. A missing production VA route is INTEGRATION_BLOCKED;
no optional E8087/silicon result is required to hide that failure.

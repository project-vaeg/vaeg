# A03 — Human handoff and mandatory stop

**Initial status:** NOT_STARTED. **Stage:** acquisition only.

Follow [acquisition.md](../acquisition.md) and [goal-contract.md](../goal-contract.md) section 0.

Generate and inspect the external OCR-HANDOFF.md, queue, checksums and download reports. Give exact paths to the user and sanitized progress inside the repo. Set AWAITING_USER_OCR, or ACQUISITION_INCOMPLETE_AWAITING_USER with missing required sources. Stop the active acquisition goal. Do not start P00, poll for OCR or manufacture approval.

Record actual commands, results and next actor in records/progress.md. Network/budget failures
are not emulator completion. Preserve successful downloads and existing work on every stop.

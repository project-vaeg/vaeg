# v5 to v6 review and migration

The supplied v5 ZIP was inspected and its actual SHA-256 was
`4dd2d33bf5b6b3eeb28a2979847dc92ab7c619872dbf8bc4346be6453e165f21`.
This v6 pack preserves all twenty implementation package bodies and the numerical,
architectural, clock, instruction-completeness and local gate contracts, with explicit
stage-entry guards and targeted source/evidence additions. No existing v5 file is changed.

## Changes that take precedence

1. Replace immediate implementation activation with acquisition-only A00-A03.
2. Stop for the user's OCR. The agent performs neither OCR nor PDF-to-text/Markdown
   conversion. Stage B needs explicit subsequent authorization plus usable source text.
3. Supply a download catalog, an executable safe acquisition helper, verification receipts,
   OCR queue and human handoff. Expected first-retrieval hashes are null, never invented.
4. Add original Intel Numerics Supplement download discovery, full E8087/support-library
   documentation, ASM86, V30 hardware material, and explicit source-tier separation.
5. Keep optional black-box E8087 and silicon studies out of the mandatory completion path.
   Neither software-library marketing nor same-byte harness assumptions prove silicon parity.
6. Exclude additional third-party implementation inputs; quarantine article downloads that
   may include microcode snippets. No raw microcode or excluded source-tree download.
7. Maintain the latest user hardware policy: default 10000000 Hz, independently editable,
   at most one 8087 per emulated machine. FPO2 does not expose another device.

## What was not changed

Every documented instruction/form remains required. The 14/94-byte guest images, raw80
preservation, no host floating point in new runtime, serial-timed integer accounting,
early exception transaction, independent test inventory, production CPU integration,
certified numerical test corpus and honest local-versus-hardware evidence remain binding.
The inherited 1-20 MHz editor range and default-disabled choice are project policies,
not additional claims about real board electrical limits.

## Corrections to the preceding survey

I03's host filename says Nov83 but the inspected cover is copyright 1981 and order
121725-001. Preserve both observations and verify the edition rather than assigning a
publication year from the URL alone. I05 (early 121627-001) remains discovery-only until
an actual online file is found; do not fabricate its download link. E8087 is an optional
corroborating reference, not an established universally bit-exact hardware oracle. A
hardware .COM test and a library-linked E8087 test may require different executables.

## Installation and ongoing work

Install only docs/8087/v6, using the no-overwrite installer. Keep earlier versions and
worktree changes. If v5 implementation already exists, acquire evidence and pause as
requested, then reconcile and test that work after Stage B authorization. Do not erase
history, reset the branch or infer that a new version number authorizes a rewrite.

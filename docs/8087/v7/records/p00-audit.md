# P00 source and baseline audit

Status: PASS (local audit completed 2026-09-20)

The audit reads the six supplied PDF/OCR pairs and the supplied Berkeley
SoftFloat Release 3e evidence under the external evidence root. It does not
run OCR and no manual or OCR document is copied into this repository.

The executable gate is `python3 tools/8087/gate.py --package P00`.

## Source identities

| ID | PDF / SHA-256 | user OCR / SHA-256 | title/order marker |
| --- | --- | --- | --- |
| I01 | `205835-007.pdf` / `cf13deaea2acbfe1dccf6dcca6edf8eace03370c1e7b3770e5eb74cd2592429c` | `205835-007.md` / `08bcb3f4b62155eb12bafca4ffc25cf3cf66d7d2f2ae59019920e4185eeb13fb` | Intel 8087 Math Coprocessor / 205835-007 |
| I02 | `121586-001_Numerics_Supplement_Jul80.pdf` / `d2d8557a8e9c24cdc4613ea5d51cd3cf745fbd586c6eaab1a1af1ccfc977aa04` | same stem `.md` / `ff6ff296935cc9d60ad7f6cc7659617fa649c51e2a53543ffe3f3d1d75481b34` | Numerics Supplement / 121586-001 Rev A |
| I03 | `121725-001_8087_Support_Library_Reference_Nov83.pdf` / `af3f774a8d0deae63699ac0a532057c7c6616418b0e41ce189ca951086bde2cd` | same stem `.md` / `1d59e940bee1dfe15b8d770cf8265520305e269519c966bbf4e3224827c41673` | 8087 Support Library Reference Manual / 121725-001 |
| I04 | `121703-003_ASM86_Language_Reference_Manual_Nov83.pdf` / `a91ad945e71625cd6425f036a28faa20bbc9f57344bb107031b333b4e34fcf32` | same stem `.md` / `1f043b9a4eca22ef42a4420789ebbe8aed759b3326fdc657b7b758ee805c4289` | ASM86 Language Reference Manual / 121703-003 |
| I05 | `1981_iAPX_86_88_Users_Manual.pdf` / `3eea6ca77ad4046ae7ade731410793206eebe8ec9a3f8ae75895685d38f4ffe5` | same stem `.md` / `088f332736a1e4200a54393d939cea4264716e10bbd94407d03368ea618b5403` | iAPX 86,88 User's Manual / 210201-001 |
| N01 | `NEC_uPD70116.pdf` / `d5f47d87c519a9e3906dc035797ae4fbfd1a75a3655c7d7c841f4aca41517a5e` | `NEC_uPD70116.md` / `14f2876696c93000659e62e7da34c8e2522362c60de87499673fffd6b6395969` | V30 / uPD70116 |

The OCR page-marker counts match the PDF page counts: 22, 132, 164, 404,
803, and 82 respectively. The companion source-hash record contains every
PDF and OCR digest above.

## Arithmetic substrate

The supplied `upstream/S03/SoftFloat-3e.zip` has SHA-256
`21130ce885d35c1fe73fc1e1bf2244178167e05c6747cad5f450cc991714c746`; the
archive passes a complete ZIP integrity test. The supplied extracted
`README.md`, `COPYING.txt`, `SOURCE.txt`, and `SHA256SUMS` records also match
their recorded digests and identify Berkeley SoftFloat Release 3e. The archive
is the only permitted arithmetic implementation source for the emulator.

## Critical visual checks

The supplied PDF pages were visually checked against the matching OCR at the
following printed pages: I01 architecture/system-configuration page 3-93;
I02 transfer table S-30 and FSCALE/FPREM page S-34; I03 instruction-summary
appendix; I05 WAIT/ESC page 2-48; and N01 V30 control-instruction page 64.
These checks establish the source boundary and CPU bus forms without placing
page images or manuals in the repository.

## Baseline

The baseline checkout was `main` at `1544a1b8`, with pre-existing untracked
files preserved. `cmake --preset linux-debug` and
`cmake --build build/linux-debug -j2` completed successfully before P00
implementation work. The repository encoding, EOL, and case validators are
part of the executable P00 gate.

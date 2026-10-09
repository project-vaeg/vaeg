# External evidence layout

The implementation uses this external read-only root:

`/Users/maho/work/vaeg-8087-evidence-v6`

Expected files:

| ID | Original PDF | OCR Markdown | Role |
|---|---|---|---|
| I01 | `originals/I01/205835-007.pdf` | `user-ocr/I01/205835-007.md` | Intel 8087 datasheet |
| I02 | `originals/I02/121586-001_Numerics_Supplement_Jul80.pdf` | `user-ocr/I02/121586-001_Numerics_Supplement_Jul80.md` | primary 8087 numerics semantics |
| I03 | `originals/I03/121725-001_8087_Support_Library_Reference_Nov83.pdf` | `user-ocr/I03/121725-001_8087_Support_Library_Reference_Nov83.md` | Intel support library / E8087 corroboration |
| I04 | `originals/I04/121703-003_ASM86_Language_Reference_Manual_Nov83.pdf` | `user-ocr/I04/121703-003_ASM86_Language_Reference_Manual_Nov83.md` | assembler spelling and encoding cross-check |
| I05 | `originals/I05/1981_iAPX_86_88_Users_Manual.pdf` | `user-ocr/I05/1981_iAPX_86_88_Users_Manual.md` | CPU/NDP interface corroboration |
| N01 | `originals/N01/NEC_uPD70116.pdf` | `user-ocr/N01/NEC_uPD70116.md` | NEC V30/uPD70116 CPU-side FPO/POLL evidence |
| S03 | `upstream/S03/SoftFloat-3e.zip` | n/a | exact official SoftFloat 3e upstream archive |

Also inspect:

- `upstream/S03/SHA256SUMS`
- `upstream/S03/SOURCE.txt`
- `upstream/S03/COPYING.txt`
- `runs/manual-sync/SOURCE-SHA256.txt` when present

## Mandatory initial validation

Before coding:

1. hash every listed original and OCR file;
2. confirm each OCR corresponds to the matching PDF title/order number/revision;
3. confirm N01's exact title, publication number, edition/date, and CPU applicability;
4. spot-check OCR against PDF images for all tables or symbols used to freeze behavior;
5. record printed-page and PDF-page mappings independently;
6. never infer a comparison operator, bit number, opcode field, or table cell from garbled OCR.

Do not modify the evidence workspace during ordinary implementation.

# Source register

Preparation status: source files already exist externally; v7 has not yet verified
their local hashes or OCR quality.

| ID | Source | Authority |
|---|---|---|
| U01 | user requirement | one optional 8087; default 8 MHz; configurable |
| I01 | Intel `205835-007`, 8087 Math Coprocessor | primary/corroborating |
| I02 | Intel `121586-001`, Numerics Supplement | primary semantic authority |
| I03 | Intel `121725-001`, 8087 Support Library Reference | Intel E8087 and library corroboration |
| I04 | Intel `121703-003`, ASM86 Language Reference | spelling/encoding cross-check |
| I05 | 1981 iAPX 86/88 User's Manual | CPU/NDP interface corroboration |
| N01 | NEC uPD70116/V30 manual in evidence root | CPU-side interface authority after edition/applicability verification |
| S03 | Berkeley SoftFloat Release 3e official archive | shipping arithmetic substrate |
| V01 | checked-out VAEG repository | implementation source of truth |
| V02 | existing VAEG/PC-88VA private/public machine documentation | VA-specific routing/model evidence |
| V03 | [VA2 8087-2 field report](https://ameblo.jp/framgate/entry-12146188742.html) | VA2 19-pin oscillator measurement: 8 MHz; hardware-informed default policy |

The implementation ledger created in P00/P01 must record actual SHA-256 hashes,
page/table identifiers, availability, conflicts, and linked test IDs.

# v6 source register

Prepared 2026-09-09. Actual acquisition state starts at NOT_RUN. This register and
`sources/catalog.json` provide locators, not a claim that the user has downloaded anything.
The v5 implementation requirements are retained; source/evidence workflow changes are in
[migration-review.md](migration-review.md). See [source-policy.md](source-policy.md).

## User and repository evidence

| ID | Authority / status |
|---|---|
| U01 | User: 10000000 Hz default, configurable, maximum one installed 8087 |
| U02 | Prior requirements: VA2/VA3 native-mode policy, complete documented instructions |
| U03 | Current user: Codex downloads/places references, user performs subsequent OCR |
| V01 | Checked-out VAEG and docs/88va: NOT_INSPECTED by pack preparation |
| V02 | User-supplied VA board/service/BIOS traces: NOT_SUPPLIED by this pack |

## Public document and reference inventory

All locations below are explicit source locators. The action controls acquisition; an
information URL for an excluded implementation is NOT a download/read authorization.

### I01 — Intel 8087 Math Coprocessor, order 205835-007, October 1989

Action: `FETCH_DOCUMENT`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `true`.
Preparation observation: `COVER_AND_ORDER_NUMBER_REVIEWED`. Expected SHA-256: **not pre-established**.

Primary chip reference. Verify fallback edition before use.

```text
https://kib.kiev.ua/x86docs/Intel/486/205835-007.pdf
https://hercules.syntax.com.tw/upload/pdf/IC-8087-2.pdf
```

External relative destination: `originals/I01/205835-007.pdf`.

### I02 — Intel The 8086 Family Users Manual: Numerics Supplement, 121586-001

Action: `FETCH_DOCUMENT`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `true`.
Preparation observation: `INDEX_AND_TARGET_LOCATED_PREVIEW_TOO_LARGE`. Expected SHA-256: **not pre-established**.

Original-manual locator found in host index. HTTP content length reported as 93425392; web preview exceeded its limit. Full local retrieval and edition verification are still required.

```text
https://mark-ogden.uk/files/intel/publications/121586-001%20The%208086%20Family%20Users%20Manual%20Numerics%20Supplement-Jul80.pdf
```

External relative destination: `originals/I02/121586-001_Numerics_Supplement_Jul80.pdf`.

### I03 — Intel 8087 Support Library Reference Manual, order 121725-001

Action: `FETCH_DOCUMENT`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `true`.
Preparation observation: `COVER_AND_CHAPTER2_REVIEWED`. Expected SHA-256: **not pre-established**.

Cover shows copyright 1981 and order 121725-001. Nov83 is a host filename label, not a verified publication date. Chapter 2 covers full E8087 and interfaces; distinguish library functions from chip instructions.

```text
https://bitsavers.trailing-edge.com/pdf/intel/ISIS_II/121725-001_8087_Support_Library_Reference_Nov83.pdf
https://bitsavers.org/pdf/intel/ISIS_II/121725-001_8087_Support_Library_Reference_Nov83.pdf
```

External relative destination: `originals/I03/121725-001_8087_Support_Library_Reference_Nov83.pdf`.

### I04 — Intel ASM86 Language Reference Manual, order 121703-003

Action: `FETCH_DOCUMENT`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `true`.
Preparation observation: `INDEX_AND_TARGET_LOCATED_PREVIEW_TOO_LARGE`. Expected SHA-256: **not pre-established**.

Host lists Nov83 and Mar85 files. Keep edition identity; do not merge them automatically. Browser preview reported a size limit.

```text
https://bitsavers.trailing-edge.com/pdf/intel/ISIS_II/121703-003_ASM86_Language_Reference_Manual_Nov83.pdf
https://bitsavers.org/pdf/intel/ISIS_II/121703-003_ASM86_Language_Reference_Manual_Nov83.pdf
```

External relative destination: `originals/I04/121703-003_ASM86_Language_Reference_Manual_Nov83.pdf`.

### I05 — Intel early 8086/8087/8088 Macro Assembly Language Reference, 121627-001

Action: `DISCOVERY_ONLY`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `false`.
Preparation observation: `DIRECT_FILE_NOT_ESTABLISHED`. Expected SHA-256: **not pre-established**.

No verified direct file URL in this pack. Make a bounded document search; preserve NOT_LOCATED if no original is found. Do not invent a URL. I04 is already required.

No verified direct download URL supplied.

### I06 — Intel iAPX 86/88 Users Manual (1981), supplement-containing edition

Action: `FETCH_DOCUMENT`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `false`.
Preparation observation: `PUBLIC_PDF_LOCATOR_FOUND`. Expected SHA-256: **not pre-established**.

Optional separate edition for corroboration. Never replace I02 under the same source ID. Full original preserved; OCR may prioritize its Numerics Supplement.

```text
https://www.dosdays.co.uk/media/intel/1981_iAPX_86_88_Users_Manual.pdf
```

External relative destination: `originals/I06/1981_iAPX_86_88_Users_Manual.pdf`.

### N01 — NEC 16-Bit V Series Instruction User Manual, U11301EJ5V0UMJ1

Action: `FETCH_DOCUMENT`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `true`.
Preparation observation: `COVER_AND_EDITION_REVIEWED`. Expected SHA-256: **not pre-established**.

Cover: fifth edition, September 2000. Check V20/V30 applicability; later CPU-specific behavior is not automatically applicable.

```text
https://datasheets.chipdb.org/NEC/V20-V30/U11301EJ5V0UMJ1.PDF
```

External relative destination: `originals/N01/U11301EJ5V0UMJ1.PDF`.

### N02 — NEC uPD70116 / V30 documentation

Action: `FETCH_DOCUMENT`. Tier: `PRIMARY_DOCUMENT`. Required acquisition item: `false`.
Preparation observation: `PDF_LOCATED_IDENTITY_PENDING`. Expected SHA-256: **not pre-established**.

Optional CPU hardware evidence. The scan is located; establish exact title/revision during user-assisted source review.

```text
https://datasheets.chipdb.org/NEC/V20-V30/NEC_uPD70116.pdf
```

External relative destination: `originals/N02/NEC_uPD70116.pdf`.

### S01 — Berkeley SoftFloat Release 3e Library Interface

Action: `FETCH_DOCUMENT`. Tier: `APPROVED_UPSTREAM`. Required acquisition item: `true`.
Preparation observation: `OFFICIAL_DOCUMENT_REVIEWED`. Expected SHA-256: **not pre-established**.

Library semantics, not an 8087 device specification.

```text
https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat.html
```

External relative destination: `originals/S01/SoftFloat.html`.

### S02 — Berkeley SoftFloat Release 3e Source Documentation

Action: `FETCH_DOCUMENT`. Tier: `APPROVED_UPSTREAM`. Required acquisition item: `true`.
Preparation observation: `OFFICIAL_DOCUMENT_REVIEWED`. Expected SHA-256: **not pre-established**.

Build/specialization/license documentation. No source archive unpacking during Stage A.

```text
https://www.jhauser.us/arithmetic/SoftFloat-3/doc/SoftFloat-source.html
```

External relative destination: `originals/S02/SoftFloat-source.html`.

### S03 — Berkeley SoftFloat Release 3e official archive

Action: `FETCH_ARCHIVE`. Tier: `APPROVED_UPSTREAM`. Required acquisition item: `true`.
Preparation observation: `OFFICIAL_RELEASE_LINK_CONFIRMED`. Expected SHA-256: **not pre-established**.

Official release link confirmed. Cache only; no extraction/build/vendor until Stage B/P02. First downloaded hash must be computed, not assumed.

```text
https://www.jhauser.us/arithmetic/SoftFloat-3e.zip
```

External relative destination: `upstream/S03/SoftFloat-3e.zip`.

### T01 — Berkeley TestFloat Release 3e General Documentation

Action: `FETCH_DOCUMENT`. Tier: `APPROVED_UPSTREAM`. Required acquisition item: `true`.
Preparation observation: `OFFICIAL_DOCUMENT_REVIEWED`. Expected SHA-256: **not pre-established**.

Backend testing scope is separate from 8087 device semantics.

```text
https://www.jhauser.us/arithmetic/TestFloat-3/doc/TestFloat-general.html
```

External relative destination: `originals/T01/TestFloat-general.html`.

### T02 — Berkeley TestFloat Release 3e official archive

Action: `FETCH_ARCHIVE`. Tier: `APPROVED_UPSTREAM`. Required acquisition item: `true`.
Preparation observation: `OFFICIAL_RELEASE_LINK_CONFIRMED`. Expected SHA-256: **not pre-established**.

External developer-only archive, cached unextracted before OCR handoff.

```text
https://www.jhauser.us/arithmetic/TestFloat-3e.zip
```

External relative destination: `upstream/T02/TestFloat-3e.zip`.

### M01 — GNU MPFR 4.2.2 manual (version-specific)

Action: `FETCH_DOCUMENT`. Tier: `APPROVED_UPSTREAM`. Required acquisition item: `false`.
Preparation observation: `OFFICIAL_DOCUMENT_REVIEWED`. Expected SHA-256: **not pre-established**.

Oracle documentation only. No runtime MPFR linkage.

```text
https://www.mpfr.org/mpfr-4.2.2/mpfr.html
```

External relative destination: `originals/M01/mpfr-4.2.2.html`.

### C01 — OpenAI Using Goals in Codex

Action: `FETCH_DOCUMENT`. Tier: `WORKFLOW_DOCUMENT`. Required acquisition item: `false`.
Preparation observation: `OFFICIAL_DOCUMENT_REVIEWED`. Expected SHA-256: **not pre-established**.

Workflow documentation, not chip evidence. Controls remain subject to installed version.

```text
https://developers.openai.com/cookbook/examples/codex/using_goals_in_codex
```

External relative destination: `originals/C01/using-goals-in-codex.html`.

### D01 — PCjs Intel 8087 documentation transcription

Action: `FETCH_DOCUMENT`. Tier: `DOCUMENT_TRANSCRIPTION`. Required acquisition item: `false`.
Preparation observation: `DOCUMENT_PAGE_REVIEWED`. Expected SHA-256: **not pre-established**.

Document-only corroboration. Not the original scan, not independent of Intel source, and not permission to read the PCjs FPU implementation. No linked assets or source files fetched.

```text
https://www.pcjs.org/documents/manuals/intel/8087/
```

External relative destination: `originals/D01/pcjs-intel-8087-documentation.html`.

### H01 — Ken Shirriff: Microcode inside the Intel 8087 floating-point chip (May 2026)

Action: `FETCH_EVIDENCE_ONLY`. Tier: `EVIDENCE_ONLY`. Required acquisition item: `false`.
Preparation observation: `AUTHOR_ARTICLE_LOCATED`. Expected SHA-256: **not pre-established**.

May contain microcode excerpts. Transfer bytes only; implementation agent must not use its content. No raw microcode, decoder code, linked images or repositories fetched.

```text
https://www.righto.com/2026/05/microcode-inside-intel-8087-floating.html
```

External relative destination: `evidence-only/H01/8087-microcode-article.html`.

### X01 — Intel full E8087 binary candidate

Action: `METADATA_ONLY`. Tier: `EXCLUDED_OR_OPTIONAL_REFERENCE`. Required acquisition item: `false`.
Preparation observation: `METADATA_NOT_EXECUTION_EVIDENCE`. Expected SHA-256: **not pre-established**.

No verified binary download, version or permission record supplied. Optional separately authorized black-box harness only; never an automatic dependency.

No verified direct download URL supplied.

### X02 — 86Box implementation

Action: `METADATA_ONLY`. Tier: `EXCLUDED_OR_OPTIONAL_REFERENCE`. Required acquisition item: `false`.
Preparation observation: `METADATA_NOT_EXECUTION_EVIDENCE`. Expected SHA-256: **not pre-established**.

Source excluded. Optional separately authorized binary comparison only.

Metadata location only:
```text
https://github.com/86Box/86Box
```

### X03 — Open Watcom fpuemu

Action: `METADATA_ONLY`. Tier: `EXCLUDED_OR_OPTIONAL_REFERENCE`. Required acquisition item: `false`.
Preparation observation: `METADATA_NOT_EXECUTION_EVIDENCE`. Expected SHA-256: **not pre-established**.

Source excluded. No clone or source-subtree download.

Metadata location only:
```text
https://github.com/open-watcom/open-watcom-v2
```

### X04 — PCjs FPU implementation

Action: `METADATA_ONLY`. Tier: `EXCLUDED_OR_OPTIONAL_REFERENCE`. Required acquisition item: `false`.
Preparation observation: `METADATA_NOT_EXECUTION_EVIDENCE`. Expected SHA-256: **not pre-established**.

FPU source excluded. D01 is a separate documentation page.

Metadata location only:
```text
https://github.com/jeffpar/pcjs
```

### X05 — Granite / tools/8087mc

Action: `METADATA_ONLY`. Tier: `EXCLUDED_OR_OPTIONAL_REFERENCE`. Required acquisition item: `false`.
Preparation observation: `METADATA_NOT_EXECUTION_EVIDENCE`. Expected SHA-256: **not pre-established**.

Implementation/decoder/raw microcode excluded; no clone/download.

Metadata location only:
```text
https://github.com/a-mcego/granite
```

### X06 — DOSBox-X implementation

Action: `METADATA_ONLY`. Tier: `EXCLUDED_OR_OPTIONAL_REFERENCE`. Required acquisition item: `false`.
Preparation observation: `METADATA_NOT_EXECUTION_EVIDENCE`. Expected SHA-256: **not pre-established**.

Source excluded. Optional separately authorized binary comparison only.

Metadata location only:
```text
https://github.com/joncampbell123/dosbox-x
```

## Edition, bibliography and source identities

Manufacturer order numbers identify Intel/NEC manuals here. No DOI or ISBN has been
established for these exact scans in this pack; none is invented. No Palmer paper or
proceedings ISBN is needed. If a paper/book is added later, verify title, edition, year,
pages and DOI/ISBN against the actual item rather than carrying over an unrelated record.

The I03 filename includes Nov83 while the inspected cover says copyright 1981. I02 was
located in the archive index but exceeded web preview size limits; this is not a claim
that its complete contents were read. I05 remains unresolved. Page counts, if observed
by a web preview, must be checked against the actual downloaded file before using page IDs.

## Implementation evidence records

After the explicit OCR handoff, P01 records exact PDF/printed pages, original and OCR hashes,
edition, subject, applicability, conflicts and linked independent tests. Availability and
semantic confidence are distinct fields. A correctly transferred PDF is not necessarily the
correct edition. A text transcription is not an independent chip implementation.

The 10 MHz default is U01, not a claim about the real VA socket. Optional E8087, other
emulators and silicon studies use reference-validation.md. No absence of optional evidence
waives a documented instruction or invents a production interrupt route.

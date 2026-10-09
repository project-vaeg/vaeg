<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M106 — Publish the INT 80H BIOS analysis

## Maintainer request and scope

Convert the existing private static FDD BIOS analysis to Markdown under
`docs/bios/`, commit it, and emphasize that canonical placement in `AGENTS.md`.
This task publishes independently written analysis only. It does not change
emulator behavior, resolve a guest defect, or approve another implementation
milestone.

## Follow-up scope authorized by the maintainer

Continue this documentation-only task with interrupt-based static references
for manual chapters 6.5 through 6.20: 33 interrupt documents, including 17
separate numerical-operation interrupts and separate INT 9EH / 9FH documents.
Record verified conventional-memory references, caller-owned buffers, model
and OS differences, and unresolved ABI/work-layout details without claiming
runtime verification or full implementation closure. Add a public index.
Generate the requested `BIOS_XXH.TXT` equivalents outside Git; publish the
independently authored Markdown under the canonical lowercase filenames.
The pre-existing INT 81H / 82H drafts and unfinished INT 83H investigation
were excluded from the chapter 6.5–6.20 commit. The subsequent maintainer
request to commit and push authorizes publication of the INT 81H / 82H drafts
and their index links in a separate documentation-only commit.
The maintainer subsequently authorizes the INT 83H detailed static reference,
its conventional-memory work analysis, and a complete 37-interrupt index on a
new branch from `origin/main`, with a separate PR targeting `main`. This
completes publication coverage for chapters 6.1–6.20, not full runtime ABI
verification. The next maintainer request authorizes a static disassembly/manual
investigation of INT 20H and INT 21H, including DOS registration, function
coverage, internal OS hooks, private TXT equivalents, and canonical public
references with index links. The maintainer then requests detailed INT 90H
V1/V2 supervisor analysis from disassembly and PC-Engine, including which
instruction set applies. This authorizes one additional static reference and
index entry, not changes to mode switching or emulator behavior. No emulator
changes or new milestone implementation are authorized.

## Deliverables

- `docs/bios/bios_int80h.md`: functions, ROM entry points, conventional-memory
  work layout, PC-Engine replacement path, and explicit verification limits.
- `docs/bios/bios_int81h.md` and `bios_int82h.md`: detailed static HDD and
  keyboard analyses, including work layouts and documented/ROM ABI differences.
- `docs/bios/bios_int83h.md`: 44 documented text functions, 51 ROM dispatch
  slots, GET BOOK, conventional-memory descriptors and hooks, TVRAM separation,
  manual discrepancies, and the VA PC-Engine wrapper with verification limits.
- `docs/bios/bios_int20h.md` and `bios_int21h.md`: process termination,
  individual DOS registration, 40 documented AH values (41 specifications),
  both 256-slot ROM tables, DOS work ownership and VA/VA2 internal hook paths.
- `docs/bios/bios_int90h.md`: separate compatible-mode vector90 entry from
  native supervisor code; compare both six-vector tables, BRKEM2 boundary,
  native trap decoders/port tables, conventional-memory work and PC-Engine
  candidates. Retain unresolved CALLN/ABI/timing questions.
- `docs/bios/README.md` must link every published interrupt reference, including
  20H/21H and 80H–83H, without claiming full interrupt-list coverage.
- `docs/bios/README.md` and 33 `bios_intXXh.md` references for chapters
  6.5–6.20; distinguish partial static evidence from documented contracts.
- `AGENTS.md`: canonical directory and lowercase interrupt-based naming.
- A documentation-only milestone entry in the roadmap.

Keep private paths, asset filenames and hashes, raw disassemblies, ROMs,
manual copies, and disk images outside Git. Retain neutral source roles and
code offsets so the findings remain distinguishable from runtime evidence.

## Machine-verifiable gate

Run `tools/repo/check_encoding.py`, `tools/repo/check_eol.py`,
`tools/repo/check_case.py`, and `git diff --cached --check`.
Review the staged scope and Markdown tables, and confirm that no private
asset identities or payloads are staged. Before placement, check generated
output out of tree: the closed set of 33 interrupt files, chapter mapping,
function counts, selected ROM bytes/entry routing, work-address arithmetic,
local links, encoding/EOL, and absence of private identities. No build, boot, or hosted CI is
needed: the commit must change documentation only. Push the review branch
and report its exact commit SHA; do not claim a merge to `main`.

## Follow-up local validation record

- Out-of-tree content check: PASS; 33 interrupt documents, 244 function rows,
  140 work-address calculations, 33 private TXT equivalents, local links,
  privacy scan, UTF-8/LF, and selected ROM registration/GET BOOK bytes.
- Correct screen-editor bank offsets checked separately: VA `8003H`,
  VA2 `E403H`; do not reuse unadjusted `0003H` disassembly.
- `python3 tools/repo/check_encoding.py --expect utf8`: `0 violation(s)`.
- `python3 tools/repo/check_eol.py --enforce`: `0 violation(s)`.
- `python3 tools/repo/check_case.py`: `0 finding(s)`.
- `git diff --cached --check`: exit 0, no output.
- Follow-up scope: 33 new interrupt references, one new index, this task file,
  and the roadmap. INT 81H / 82H drafts and unrelated untracked items excluded.
- No build/hosted CI: documentation only. No ROM/OS runtime execution,
  destructive media tests, or changes to archived-reference behavior or
  provenance. Full ABI/work-layout closure and runtime verification remain
  open; passing documentation checks does not establish BIOS conformance.

## INT 83H / complete-index follow-up

The new `main`-based PR changes only this task, the roadmap, the index, and
`docs/bios/bios_int83h.md`. Local static content verification passed: 44
published functions against both 51-slot ROM tables, IVT registration and
GET BOOK bytes, selected initialization/ABI bytes, 24 work-address
calculations, privacy and UTF-8/LF checks, and all 37 interrupt references
uniquely reachable from the index. The requested TXT equivalents stay outside
Git. Repeat the repository encoding/EOL/case and staged-diff checks before
push. No build, hosted CI request, runtime probe, or archived-reference change
is part of this documentation-only follow-up.

## INT 20H / 21H static follow-up

Out-of-tree generation and content verification passed: two references,
40 documented AH values / 41 individual specifications, 256 table slots per
ROM, 50/55 nondefault VA/VA2 entries, individual DOS vector registration,
INT20H's zero-code AH=4CH call, version/vector ABI bytes, the shared internal
hook address alias, and 14/5 VA/VA2 OS patch-table pairs. Verified findings
include the VA hook's AH=18H/47H selection, not a full DOS patch inventory.
The generated Markdown was validated before placement and copied byte-for-byte;
all 39 published interrupt references are linked from the index table. Raw
inputs, disassembly, output manifest and TXT equivalents remain outside Git.
The intended scope is exactly two new interrupt references, the index, this
task, and the roadmap. Repository invariant and staged-diff checks must pass
before push. No emulator build, runtime call, hosted CI request, guest-visible
bug fix, or archived-reference change is included.

## INT 90H static follow-up

Out-of-tree content verification passed for six individual vector records per
ROM, 128 IN/OUT port-table entries, 16 block-I/O entries, 13 conventional-memory
address calculations, BRKEM2 boundaries, trap-range data, and both compatible
entrypoints. A standalone offline instruction-set decoder was locally compiled
and confirmed DI / LD SP,E1A0H / JP 3BE5H; it instantiates no emulator device.
The PC-Engine AL=90H candidate was traced to sound-register output, not an INT90H
hook. No OS vector90 replacement was established; this is not an exhaustive
absence proof. Worker/input/output identities and disassembly remain private.
Generated Markdown was validated before placement; all 40 interrupt references
are linked once in the index table. Intended scope: one new interrupt reference,
index, task and roadmap only. Run repository invariant and staged-diff checks
before push. No emulator build, runtime mode switch, hosted CI request,
guest-visible bug fix, or archived-reference change is included.

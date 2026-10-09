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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M71 - Fold uPD9002 dispatch into the core translation unit

## Fixed predecessor

M71 starts from the formally approved and main-integrated G70 candidate:

`53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`

Approved predecessor gate: `G70`

Branch:
`topic/m71-upd9002-core-dispatch-fold`

Commit prefix: `M71:`

Candidate gate: `G71`

Report:
`docs/agents/reports/m71_upd9002_core_dispatch_fold.md`

Do not start M72. Do not merge M71 to `main` before G71 approval. Do not
declare G71 passed.

## Scope

M71 is a source-organization cleanup for the active uPD9002 core. It removes
the obsolete split between the core and dispatch construction files now that
the active CPU no longer shares a 286 implementation.

M71 must:

- remove `cpu/upd9002/upd9002_dispatch.c` as an independent compilation unit;
- remove `cpu/upd9002/upd9002_dispatch.h`;
- fold the dispatch constructor away, replace dispatch-era handlers in their
  canonical handler translation units, and keep `upd9002_core_step()` in
  `cpu/upd9002/upd9002_core.c`;
- keep folded root tables in the translation unit that can legally name their
  static handlers without exposing those handlers across translation-unit
  boundaries;
- remove obsolete `v30` naming from current production symbols and current
  QA metadata, for example `v30_idiv_ea8` becomes `_idiv_ea8`;
- keep externally consumed uPD9002 test seams available through existing
  core-facing headers;
- preserve generated-dispatch validation with the new canonical names.

## Non-goals

M71 must not change uPD9002 instruction semantics, target policy,
SST fixtures, corpus records, state format, memory mapping, TSP/IDP behavior,
Z80 behavior, frontend behavior, or manual-runtime acceptance criteria.

M71 must not revive generic V20/V30/i286/i386 compatibility or reinterpret
`64H`/`65H` as FS/GS prefixes.

Historical approved artifacts remain immutable. If current generated views or
golden files need name-only updates, the report must identify them as M71
metadata refreshes and not as semantic-policy changes.

## Required validation

Run focused validation that proves the fold is behavior-neutral:

- dispatch graph/provenance/support-map validation;
- dispatch normalization runtime test;
- direct uPD9002 harness manifest validation;
- M68 mapped-memory protection;
- M69 IDP status composition protection;
- M70 prefix/string directed and campaign validators available locally;
- protected M65a-M65e tests;
- native non-external CTest subset covering uPD9002;
- normal production build;
- GCC or repository default build;
- Clang build if available;
- MinGW build if available;
- repository invariant checks for milestone IDs, encoding, EOL, path case;
- `git diff --check`.

Unavailable platform checks must be reported with the command, exit status,
and missing dependency or platform.

## Closure

The final M71 report must include:

- starting SHA and branch;
- commit list;
- files removed;
- files modified;
- naming transition summary;
- confirmation that `upd9002_dispatch.c` and `upd9002_dispatch.h` are absent;
- confirmation that no current production symbol keeps the obsolete `v30`
  prefix in the folded dispatch/core path;
- behavior-neutral validation results and command exit statuses;
- generated metadata updates, if any;
- remaining risks and the G71 human-review checklist.

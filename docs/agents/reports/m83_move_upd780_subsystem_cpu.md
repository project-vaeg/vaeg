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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
-->

# M83: move the FDC uPD780 disassembler into `cpu/upd780/`

## Status

M83 is closed after the maintainer confirmed that the G83 human gate passed
on 2026-08-11. The evidence-backed source move, reference fixups, Linux and
MinGW builds, hosted CI, and manual FDD boot/access validation are complete.

Evaluated branch: `topic/m83-move-upd780-subsystem-cpu`

Evaluated source commit: [`4cf317a0bdf04b3da045f472f17f0dd8d08e3b2f`](https://github.com/nakatamaho/vaeg/commit/4cf317a0bdf04b3da045f472f17f0dd8d08e3b2f)

## Scope and boundary

M82 identified exactly two FDC-facing files for this move:

- `cpucva/upd780_disasm.cpp` → `cpu/upd780/upd780_disasm.cpp`
- `cpucva/upd780_disasm.h` → `cpu/upd780/upd780_disasm.h`

The first M83 implementation commit is the rename-only commit
[`b69716764d2e314db9600e5bc28553ac813a8a6e`](https://github.com/nakatamaho/vaeg/commit/b69716764d2e314db9600e5bc28553ac813a8a6e); Git records both files as 100% renames with no content change. The following reference-fixup commit [`96d39b7c4ce2e6aea279370db2b141d6f6cbfdf1`](https://github.com/nakatamaho/vaeg/commit/96d39b7c4ce2e6aea279370db2b141d6f6cbfdf1) updates the production/test CMake source paths, the subsystem and focused-test includes, the header guard, and current path documentation.

The shared `cpucva/z80_compat_*` backend and state codec remain in place. No
FDC protocol, scheduler, interrupt, WAIT, save-state payload, or uPD780
instruction behavior was changed. Historical M82 and Z80 migration documents
retain their original paths as historical evidence; current active documents
use `cpu/upd780/`.

The M83 start and ROADMAP correction are recorded in
[`383bfeb8cad8c33d66569bfa0409db8d4190dd92`](https://github.com/nakatamaho/vaeg/commit/383bfeb8cad8c33d66569bfa0409db8d4190dd92) and
[`4cf317a0bdf04b3da045f472f17f0dd8d08e3b2f`](https://github.com/nakatamaho/vaeg/commit/4cf317a0bdf04b3da045f472f17f0dd8d08e3b2f). The latter changes the M83 table status to the validator-compatible `G83 human; in progress` form.

## Validation

| Check | Result |
| --- | --- |
| `tools/repo/check_case.py` | 0 findings |
| `tools/repo/check_encoding.py --expect utf8` | 0 violations |
| `tools/repo/check_eol.py --enforce` | 0 violations |
| `tools/qa/upd9002_rename.py` | PASS |
| `git diff --check` | PASS |
| `cmake --preset linux-ci-gcc` and `cmake --build --preset linux-ci-gcc -j4` | PASS |
| `cmake --preset mingw-cross` and `CCACHE_DISABLE=1 cmake --build --preset mingw-cross -j4` | PASS |
| focused wrapper/disassembler/FDC/save-load tests | 6/6 passed |
| full `ctest --test-dir build/linux-ci-gcc --output-on-failure` | 83/83 passed; one external SST test skipped |

The focused six-test run covered `vaeg_idp_m69_status_composition`, both
uPD780 wrapper variants, `vaeg_upd780_disasm`, the state regression test, and
`vaeg_romless_tests`. The full CTest run passed the repository, wrapper, FDC,
SST, storage, and state suites; `vaeg_upd9002_ssts_ci_external` was skipped
because its external corpus is not present.

The pre-M83 CI baseline repair was [`0a2608351d4e301e9729d7d4ab25d662b98d8c74`](https://github.com/nakatamaho/vaeg/commit/0a2608351d4e301e9729d7d4ab25d662b98d8c74), which only updated stale M69 test API references and selected the current VA I/O mode in that test. Its final nine-job run was GitHub Actions run `31496082527`, with all jobs successful before M83 began.

The M83 path-move candidate also triggered GitHub Actions run `31498223361`; all nine jobs succeeded, including Windows MinGW compatibility. This hosted result validates the candidate build and tests but does not replace the required G83 human gate.

## Gate disposition

G83 human gate passed on 2026-08-11. The maintainer verified the standard
clean-checkout build, V3 boot, bundled VA demo, OS boot, and basic FDD
operations. M83 is closed; M84 may now start, but has not been merged to
`main` and G84 remains pending.

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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M91 - Native V3 VA single path

Status: **G91 human gate passed; M91 closed**

Predecessor: approved G90.

Branch: topic/m91-va-single-path

Commit prefix: M91:

Gate result: G91 passed on 2026-08-15 against
`66fbf20f6f5a8fbe107884ca1223acc53d352cbd`.

Report: docs/agents/reports/m91_va_single_path.md

M91 is closed after maintainer approval. M92 remains separately authorized
work and is not started by this closure.

## Goal

Make the native PC-88VA V3 implementation the only active machine path. Remove
the inherited PC-98 compatibility selectors instead of layering VA dispatch on
top of an inactive PC-98 dispatcher.

## Authority and terminology

- Use the maintainer-provided docs/tekumani material as the primary source for
  built-in PC-88VA I/O and memory semantics.
- Use docs/98io only to identify inherited PC-98 functions and same-number
  semantic collisions.
- A matching port number is not evidence of matching hardware behavior.
- Keep VA-supported expansion protocols such as SASI, SCSI, EMS, MPU, and the
  emulator HOSTFAT interface when their ownership is established separately.
- Program symbols, strings, diagnostics, and comments added or corrected by
  M91 must be English. Comments must describe the documented VA function, not
  merely translate a legacy PC-98 label.

## Scope

M91 must:

1. replace the common/VA I/O selector with one canonical VA port map;
2. remove inherited PC-98-only port registrations from mixed device modules;
3. remove the emulator-private memory/I/O mode selector;
4. route public CPU memory access through the VA memory implementation and
   retain explicit raw main-RAM helpers only where the VA map requires them;
5. remove the old PC-98 CGROM/window implementation and retain the VA V3
   014CH-014FH CGROM path backed by fontmem;
6. collapse reset and bind tables to the devices used by the VA route;
7. remove the PC-98 ARTIC implementation at 005CH-005FH; and
8. update focused tests and selftests for an unconditional VA route.

## Explicit deferral

Correct VA1/V2 compatibility mode is a later, separately authorized hardware
milestone. M91 does not replace the removed PC-98 ARTIC handler with guessed
VA1/V2 GVRAM behavior at 005CH-005FH. Native V3 is the only M91 target.

## Non-goals

- Do not change uPD9002 instruction semantics.
- Do not change SGP timing or graphics behavior.
- Do not remove VA-supported expansion boards solely because a built-in-port
  manual does not describe their board-specific or emulator-private ports.
- Do not modify ROM, disk, font, icon, wave, or other binary payloads.

## Validation

Run the repository invariant checks, normal CMake build, CTest, emulator
selftest, focused uPD9002 and device validators, and an available cross-build.
G91 then requires a clean-checkout native V3 boot, bundled VA demo, OS boot,
and simple FDD, storage, keyboard, display, sound, and state-save operations.

## Gate result

The maintainer explicitly reported G91 passed on 2026-08-15 against candidate
`66fbf20f6f5a8fbe107884ca1223acc53d352cbd`. This closes M91. The gate covers
the required clean-checkout native V3 boot, bundled VA demo, OS boot, and
simple FDD, storage, keyboard, display, sound, and state-save operations.

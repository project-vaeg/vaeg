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
# M35: Minimal suzukiplan Z80 interrupt extension

Status: complete; G35 passed by maintainer approval on 2026-07-15

Branch: `topic/m35-z80-upstream-extension`

The approved provenance is:

| Item | Value |
|---|---|
| Upstream base SHA | `e3926769a790fab0af1c34a5540e317f8d4f0ddc` |
| [Downstream patch](../reports/m35_suzukiplan_irq_extension.patch) SHA-256 | `d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4` |
| Tested resulting commit | `b4a0a5a238fecc280781e6fe5719faf0eafcd667` |
| Tested resulting tree | `8a606eb39332a6e79b69bb62d9dedca042b923dc` |
| Clean `git am` reproduction | Passed |
| MIT license verification | Passed |

The complete record is in the
[evidence report](../reports/m35_suzukiplan_irq_extension.md).
The maintainer approved this reproducible downstream patch as sufficient M35
provenance; an immutable public upstream or fork commit is not required. Do
not begin M36 until the maintainer explicitly instructs it.

Direct execution found that the current full `test/` suite cannot be compiled
wholesale with `Z80_NO_FUNCTIONAL`: the unchanged
`test/test-checkreg-on-callback.cpp:88` uses a capturing lambda where that
configuration requires a function pointer. The exact selected base fails at
the same line. The full suite is green in its normal configuration, the M35
focused suite is green in both configurations, and the upstream ZEX harness is
green in its declared `Z80_NO_FUNCTIONAL` configuration. The maintainer
accepted this test matrix. Legacy tests that already fail at the approved base
under `Z80_NO_FUNCTIONAL` do not require an M35 rewrite when the limitation is
reproduced and documented.

## Entry and scope

Read the repository rules, roadmap, conventions, master plan, ADR-0011, and
M34 evidence in the prescribed order. Record branch, SHA, and status. Work
against a clean `suzukiplan/z80` checkout based on
`e3926769a790fab0af1c34a5540e317f8d4f0ddc`, not a vaeg vendored copy. vaeg
changes are limited to patch/fork provenance and evidence. Do not vendor,
integrate, write the wrapper, replace disassembly, or change emulator behavior.

## Required extension

Add a general-purpose level-sensitive IRQ setter without changing existing
one-shot API semantics. Assertion persists until explicitly deasserted;
acceptance clears CPU IFF state but not the external line. Add an unsigned
eight-bit acknowledge callback invoked exactly once at actual maskable-
interrupt acceptance, after EI inhibition, never for assertion alone or NMI.
Support normal `std::function` and `Z80_NO_FUNCTIONAL` builds.

For IM0, route the callback byte through the normal decoder as the raw first
opcode. Do not fetch that byte or advance PC; fetch operands and following
prefix bytes from current memory PC. IM1 invokes and ignores the byte before
entering `0x0038`; IM2 invokes once and uses it as vector low byte. Make the
level state inspectable/restorable. Add no broader state API unless direct
implementation evidence disproves ADR-0011's public-state finding.

## Required upstream/fork tests

Cover DI assertion, EI delay, acknowledge value changed by the inhibited
instruction, deassertion before acceptance, persistent assertion across later
EI, HALT wake, and NMI non-acknowledge. Cover IM0 RST, `0x00`, `0x7f`, a
multi-byte control transfer, CB, ED, DD, FD, DDCB, and FDCB with remaining
bytes fetched from memory. Cover IM1 acknowledge and IM2 callback/vector read
order. Use the maintainer-approved matrix: run the existing full suite in its
supported default callback configuration; run every M35 interrupt-extension
test in both normal and `Z80_NO_FUNCTIONAL` configurations; and run ZEXDOC and
ZEXALL in their supported harness configuration. Reproduce and document any
legacy test that already fails to compile with `Z80_NO_FUNCTIONAL` at the
approved base; do not rewrite that harness in M35.

## Upstream-first evidence

An upstream pull request remains preferable but is not required for G35. The
maintainer approved the directly applicable downstream `git format-patch` as
sufficient provenance when its base SHA, patch SHA-256, tested result commit,
tested result tree, clean-application result, license, scope, commands, and
complete results are recorded. M36 must reproduce the expected tree from the
approved base and patch before vendoring, then record the base SHA, patch hash,
and resulting tree hash in the vendored provenance file.

## Gate G35

G35 passed on maintainer review of the focused patch, unchanged MIT license,
clean `git am` reproduction, approved downstream provenance, and accepted test
matrix. Push the M35 vaeg evidence branch, report exact SHAs and status, and
stop. Do not begin M36 without an explicit maintainer instruction.
